#define DN_NET2_CURL_CPP

#include "../dn_base_inc.h"
#include "../dn_os_inc.h"
#include "dn_net2_curl.h"

static void DN_NET2_MarkRequestDone_(DN_NET2Core *net, DN_NET2RequestInternal *request)
{
  for (DN_OS_MutexScope(&net->free_or_done_mutex))
    DN_DLList_Append(net->done_list, request);
  DN_OS_SemaphoreIncrement(&net->completion_sem, 1);
  DN_OS_SemaphoreIncrement(&request->completion_sem, 1);
}

static DN_NET2Response DN_NET2_WaitForAnyResponse(DN_NET2Core *net, DN_Arena *arena, DN_U32 timeout_ms)
{
  DN_NET2Response          result = {};
  DN_OSSemaphoreWaitResult wait   = DN_OS_SemaphoreWait(&net->completion_sem, timeout_ms);
  if (wait == DN_OSSemaphoreWaitResult_Success) {
    for (DN_OS_MutexScope(&net->free_or_done_mutex)) {
      DN_NET2RequestInternal *request = nullptr;
      DN_DLList_Dequeue(net->done_list, request);
      DN_Assert(request);

      // NOTE: Fill in the result
      DN_NET2ResponseInternal const *response = &request->response;
      result.request                          = {DN_CAST(DN_U64) request};
      result.success                          = response->status != DN_NET2RequestStatus_Error;
      result.ws_type                          = response->ws_type;
      result.http_status                      = response->http_status;
      result.body                             = DN_Str8Builder_Build(&response->body, arena);
      if (response->error.size)
        result.error = DN_Str8_FromStr8(arena, response->error);

      // NOTE: Deallocate the memory used in the request
      DN_Arena_PopTo(&request->arena, request->start_response_arena_pos);
      request->response.body = DN_Str8Builder_FromArena(&request->arena);

      // NOTE: For websocket requests, notify the NET thread we've read data from it and it can go
      // back to polling the socket for more data
      if (request->type == DN_NET2RequestType_WS && request->response.status != DN_NET2RequestStatus_Error) {
        DN_NET2RingEvent event = {};
        event.type             = DN_NET2RingEventType_ReceivedWSReceipt;
        event.request          = {DN_CAST(DN_U64)request};
        for (DN_OS_MutexScope(&net->ring_mutex))
          DN_Ring_WriteStruct(&net->ring, &event);
        curl_multi_wakeup(net->curlm);
      }
    }
  }
  return result;
}

static DN_NET2Response DN_NET2_WaitForResponse(DN_NET2Core *net, DN_NET2Request request, DN_Arena *arena, DN_U32 timeout_ms)
{
  DN_NET2Response result = {};
  if (request.handle != 0) {
    DN_NET2RequestInternal  *request_ptr = DN_CAST(DN_NET2RequestInternal *) request.handle;
    DN_OSSemaphoreWaitResult wait        = DN_OS_SemaphoreWait(&request_ptr->completion_sem, timeout_ms);
    if (wait == DN_OSSemaphoreWaitResult_Success) {
      // NOTE: Fill in the result
      DN_NET2ResponseInternal const *response = &request_ptr->response;
      result.request                          = request;
      result.success                          = response->status != DN_NET2RequestStatus_Error;
      result.ws_type                          = response->ws_type;
      result.http_status                      = response->http_status;
      result.body                             = DN_Str8Builder_Build(&response->body, arena);
      if (response->error.size)
        result.error = DN_Str8_FromStr8(arena, response->error);

      // NOTE: Deallocate the memory used in the request
      DN_Arena_PopTo(&request_ptr->arena, request_ptr->start_response_arena_pos);
      request_ptr->response.body = DN_Str8Builder_FromArena(&request_ptr->arena);

      // NOTE: Decrement the global completion tracking semaphore (this is so that if you waited on
      // the net object's semaphore, you don't get a phantom wakeup because this function already
      // consumed it).
      DN_OSSemaphoreWaitResult net_wait_result = DN_OS_SemaphoreWait(&net->completion_sem, 0 /*timeout_ms*/);
      DN_AssertF(net_wait_result == DN_OSSemaphoreWaitResult_Success, "Wait result was: %zu", DN_CAST(DN_USize)net_wait_result);

      // NOTE: Remove the request from the done list
      for (DN_OS_MutexScope(&net->free_or_done_mutex)) {
        bool is_in_done_list = false;
        for (DN_DLList_ForEach(it, net->done_list)) {
          if (it == request_ptr) {
            is_in_done_list = true;
            break;
          }
        }
        DN_Assert(is_in_done_list);
        DN_DLList_Detach(request_ptr);
      }

      // NOTE: For websocket requests, notify the NET thread we've read data from it and it can go
      // back to polling the socket for more data
      if (request_ptr->type == DN_NET2RequestType_WS && request_ptr->response.status != DN_NET2RequestStatus_Error) {
        DN_NET2RingEvent event = {};
        event.type             = DN_NET2RingEventType_ReceivedWSReceipt;
        event.request          = request;
        for (DN_OS_MutexScope(&net->ring_mutex))
          DN_Ring_WriteStruct(&net->ring, &event);
        curl_multi_wakeup(net->curlm);
      }
    }
  }

  return result;
}

static void DN_NET2_DeinitRequest(DN_NET2Core *net, DN_NET2Request *request)
{
  DN_NET2RingEvent event = {};
  event.type             = DN_NET2RingEventType_DeinitRequest;
  event.request          = *request;
  *request               = {};
  for (DN_OS_MutexScope(&net->ring_mutex))
    DN_Ring_WriteStruct(&net->ring, &event);
}

static DN_USize DN_NET2_HTTPCallback_(char *payload, DN_USize size, DN_USize count, void *user_data)
{
  auto    *request      = DN_CAST(DN_NET2RequestInternal *) user_data;
  DN_USize result       = 0;
  DN_USize payload_size = size * count;
  if (DN_Str8Builder_AppendBytesCopy(&request->response.body, payload, payload_size))
    result = payload_size;
  return result;
}

static int32_t DN_NET2_ThreadEntryPoint_(DN_OSThread *thread)
{
  DN_NET2Core *net = DN_CAST(DN_NET2Core *) thread->user_context;
  DN_OS_ThreadSetName(DN_Str8_Init(net->curl_thread.name.data, net->curl_thread.name.size));

  for (;;) {
    DN_OSTLSTMem tmem = DN_OS_TLSPushTMem(nullptr);
    for (bool dequeue_ring = true; dequeue_ring;) {
      // NOTE: Dequeue user request
      DN_NET2RingEvent event = {};
      for (DN_OS_MutexScope(&net->ring_mutex)) {
        if (DN_Ring_HasData(&net->ring, sizeof(event)))
            DN_Ring_Read(&net->ring, &event, sizeof(event));
      }

      switch (event.type) {
        case DN_NET2RingEventType_Nil: dequeue_ring = false; break;

        case DN_NET2RingEventType_DoHTTP: {
          DN_NET2RequestInternal *request = DN_CAST(DN_NET2RequestInternal *)event.request.handle;
          DN_Assert(request->response.status != DN_NET2RequestStatus_Error);
          switch (request->type) {
            case DN_NET2RequestType_Nil: {
              DN_NET2_MarkRequestDone_(net, request);
              DN_InvalidCodePath;
            } break;

            case DN_NET2RequestType_HTTP: {
              DN_Assert(request->response.status == DN_NET2RequestStatus_Nil);
              DN_NET2CurlConn *conn      = DN_CAST(DN_NET2CurlConn *) request->context;
              CURLMcode        multi_add = curl_multi_add_handle(net->curlm, conn->curl);
              DN_Assert(multi_add == CURLM_OK);
              DN_Assert(request->next == nullptr);
              DN_Assert(request->prev == nullptr);
              DN_DLList_Append(net->http_list, request);
            } break;

            case DN_NET2RequestType_WS: {
              DN_Assert(request->response.status == DN_NET2RequestStatus_HTTPReceived ||
                        request->response.status == DN_NET2RequestStatus_WSReceived ||
                        request->response.status == DN_NET2RequestStatus_Nil);

              DN_Assert(request->next == nullptr);
              DN_Assert(request->prev == nullptr);
              if (request->response.status == DN_NET2RequestStatus_Nil) {
                DN_NET2CurlConn *conn      = DN_CAST(DN_NET2CurlConn *) request->context;
                CURLMcode        multi_add = curl_multi_add_handle(net->curlm, conn->curl);
                DN_Assert(multi_add == CURLM_OK);
                DN_DLList_Append(net->http_list, request); // Open the WS connection
              } else {
                DN_DLList_Append(net->ws_list, request); // Ready to send data through the WS connection
              }
            } break;
          }
        } break;

        case DN_NET2RingEventType_SendWS: {
          DN_Str8 payload = {};
          for (DN_OS_MutexScope(&net->ring_mutex)) {
            DN_Assert(DN_Ring_HasData(&net->ring, event.send_ws_payload_size));
            payload = DN_Str8_Alloc(tmem.arena, event.send_ws_payload_size, DN_ZeroMem_No);
            DN_Ring_Read(&net->ring, payload.data, payload.size);
          }

          DN_U32 curlws_flag = 0;
          switch (event.send_ws_type) {
            case DN_NET2WSType_Nil:    DN_InvalidCodePath; break;
            case DN_NET2WSType_Text:   curlws_flag = CURLWS_TEXT; break;
            case DN_NET2WSType_Binary: curlws_flag = CURLWS_BINARY; break;
            case DN_NET2WSType_Close:  curlws_flag = CURLWS_CLOSE; break;
            case DN_NET2WSType_Ping:   curlws_flag = CURLWS_PING; break;
            case DN_NET2WSType_Pong:   curlws_flag = CURLWS_PONG; break;
          }

          DN_NET2RequestInternal *request = DN_CAST(DN_NET2RequestInternal *) event.request.handle;
          DN_Assert(request->type == DN_NET2RequestType_WS);
          DN_Assert(request->response.status == DN_NET2RequestStatus_HTTPReceived || request->response.status == DN_NET2RequestStatus_WSReceived);

          DN_NET2CurlConn        *conn        = DN_CAST(DN_NET2CurlConn *) request->context;
          DN_USize                sent        = 0;
          CURLcode                send_result = curl_ws_send(conn->curl, payload.data, payload.size, &sent, 0, curlws_flag);
          DN_AssertF(send_result == CURLE_OK, "Failed to send: %s", curl_easy_strerror(send_result));
          DN_AssertF(sent == payload.size, "Failed to send all bytes (%zu vs %zu)", sent, payload.size);
        } break;

        case DN_NET2RingEventType_ReceivedWSReceipt: {
          DN_NET2RequestInternal *request = DN_CAST(DN_NET2RequestInternal *) event.request.handle;
          DN_Assert(request->type == DN_NET2RequestType_WS);
          DN_Assert(request->response.status == DN_NET2RequestStatus_WSReceived ||
                    request->response.status == DN_NET2RequestStatus_HTTPReceived);
          DN_Assert(request->next == nullptr);
          DN_Assert(request->prev == nullptr);
          DN_DLList_Append(net->ws_list, request);
        } break;

        case DN_NET2RingEventType_DeinitRequest: {
          if (event.request.handle != 0) {
            DN_NET2RequestInternal *request = DN_CAST(DN_NET2RequestInternal *) event.request.handle;
            request->response               = {};

            DN_Arena_Clear(&request->arena);
            DN_OS_SemaphoreDeinit(&request->completion_sem);

            DN_NET2CurlConn *conn = DN_CAST(DN_NET2CurlConn *) request->context;
            curl_multi_remove_handle(net->curlm, conn->curl);
            curl_easy_reset(conn->curl);
            curl_slist_free_all(conn->curl_slist);

            for (DN_OS_MutexScope(&net->free_or_done_mutex))
              DN_DLList_Append(net->free_list, request);
          }
        } break;
      }
    }

    // NOTE: Pump handles
    int       running_handles = 0;
    CURLMcode perform_result  = curl_multi_perform(net->curlm, &running_handles);
    if (perform_result != CURLM_OK)
      DN_InvalidCodePath;

    // NOTE: Check pump result
    for (;;) {
      int      msgs_in_queue = 0;
      CURLMsg *msg           = curl_multi_info_read(net->curlm, &msgs_in_queue);
      if (msg) {
        // NOTE: Get request handle
        DN_NET2RequestInternal *request = nullptr;
        curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, DN_CAST(void **) & request);
        DN_Assert(request);

        DN_NET2CurlConn *conn = DN_CAST(DN_NET2CurlConn *)request->context;
        DN_Assert(conn->curl == msg->easy_handle);

        if (msg->data.result == CURLE_OK) {
          // NOTE: Get HTTP response code
          CURLcode get_result = curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &request->response.http_status);
          if (get_result == CURLE_OK) {
            request->response.status = DN_NET2RequestStatus_HTTPReceived;
          } else {
            request->response.error  = DN_Str8_FromF(&request->arena, "Failed to get HTTP response status (CURL %d): %s", msg->data.result, curl_easy_strerror(get_result));
            request->response.status = DN_NET2RequestStatus_Error;
          }
        } else {
          request->response.status = DN_NET2RequestStatus_Error;
          request->response.error  = DN_Str8_FromF(&request->arena,
                                                  "Net request to '%.*s' failed (CURL %d): %s",
                                                  DN_STR_FMT(request->url),
                                                  msg->data.result,
                                                  curl_easy_strerror(msg->data.result));
        }

        if (request->type == DN_NET2RequestType_HTTP || request->response.status == DN_NET2RequestStatus_Error) {
          // NOTE: Remove the request from the multi handle if we're a HTTP request
          // because it typically terminates the connection. In websockets the
          // connection remains in the multi-handle to allow you to send and
          // receive WS data from it.
          //
          // If there's an error (either websocket or HTTP) we will also remove the
          // connection from the multi handle as it failed. One a connection has
          // failed, curl will not poll that connection so there's no point keeping
          // it attached to the multi handle.
          curl_multi_remove_handle(net->curlm, msg->easy_handle);
        }

        DN_NET2_MarkRequestDone_(net, request);
      }

      if (msgs_in_queue == 0)
        break;
    }

    // NOTE: Check websockets
    DN_I32 sleep_time_ms = DN_DLList_HasItems(net->ws_list) ? 100 : INT32_MAX;
    for (DN_DLList_ForEach(request, net->ws_list)) {
      DN_Assert(request->response.status == DN_NET2RequestStatus_HTTPReceived ||
                request->response.status == DN_NET2RequestStatus_WSReceived);

      CURLcode             receive_result        = CURLE_OK;
      const curl_ws_frame *meta                  = nullptr;
      size_t               bytes_read_this_frame = {};

      DN_NET2CurlConn *conn = DN_CAST(DN_NET2CurlConn *) request->context;
      for (;;) {
        // NOTE: Determine WS payload size received
        DN_USize bytes_read = 0;
        receive_result      = curl_ws_recv(conn->curl, nullptr, 0, &bytes_read, &meta);
        if (receive_result != CURLE_OK || meta->bytesleft == 0)
          break;

        // NOTE: Allocate and read
        DN_Str8 buffer         = DN_Str8_Alloc(&request->arena, meta->bytesleft, DN_ZeroMem_No);
        receive_result         = curl_ws_recv(conn->curl, buffer.data, buffer.size, &buffer.size, &meta);
        bytes_read_this_frame += buffer.size;
        DN_Str8Builder_AppendRef(&request->response.body, buffer);

        if (meta->flags & CURLWS_TEXT)
          request->response.ws_type = DN_NET2WSType_Text;

        if (meta->flags & CURLWS_BINARY)
          request->response.ws_type = DN_NET2WSType_Binary;

        if (meta->flags & CURLWS_PING)
          request->response.ws_type = DN_NET2WSType_Ping;

        if (meta->flags & CURLWS_PONG)
          request->response.ws_type = DN_NET2WSType_Pong;

        if (meta->flags & CURLWS_CLOSE)
          request->response.ws_type = DN_NET2WSType_Close;

        request->response.ws_has_more = meta->flags & CURLWS_CONT || meta->bytesleft > 0;
        if (request->response.ws_has_more) {
          if (meta->flags & CURLWS_CONT) {
            bool is_text_or_binary = request->response.ws_type == DN_NET2WSType_Binary ||
                                     request->response.ws_type == DN_NET2WSType_Text;
            DN_Assert(is_text_or_binary);
          }
        }

        if (receive_result != CURLE_OK || bytes_read_this_frame == meta->len)
          break;
      }

      // NOTE: We read all the possible bytes that CURL has received for this
      // socket, but, there are more bytes left that we will receive on subsequent
      // calls. We will continue to the next request and return back to this one
      // when PumpRequests is called again where hopefully that data has arrived.
      if (request->response.ws_has_more || receive_result == CURLE_AGAIN) {
        if (receive_result == CURLE_AGAIN)
            sleep_time_ms = 100;
        continue;
      }

      request->response.status = DN_NET2RequestStatus_WSReceived;
      if (receive_result != CURLE_OK) {
        request->response.status = DN_NET2RequestStatus_Error;
        request->response.error  = DN_Str8_FromF(&request->arena,
                                                "Websocket failed to receive data for '%.*s' (CURL %d): %s",
                                                DN_STR_FMT(request->url),
                                                receive_result,
                                                curl_easy_strerror(receive_result));
      }

      DN_NET2RequestInternal *request_copy = request;
      request                              = request->prev;
      DN_NET2_MarkRequestDone_(net, request_copy);
    }

    curl_multi_poll(net->curlm, nullptr, 0, sleep_time_ms, nullptr);
  }
  return 0;
}

static void DN_NET2_Init(DN_NET2Core *net, char *ring_base, DN_USize ring_size, char *base, DN_U64 base_size)
{
  net->base               = base;
  net->base_size          = base_size;
  net->arena              = DN_Arena_FromBuffer(net->base, net->base_size, DN_ArenaFlags_Nil);
  net->ring.base          = ring_base;
  net->ring.size          = ring_size;
  net->ring_mutex         = DN_OS_MutexInit();
  net->completion_sem     = DN_OS_SemaphoreInit(0);
  net->free_or_done_mutex = DN_OS_MutexInit();
  net->curlm              = DN_CAST(CURLM *)curl_multi_init();
  DN_DLList_InitArena(net->free_list, DN_NET2RequestInternal, &net->arena);
  DN_DLList_InitArena(net->http_list, DN_NET2RequestInternal, &net->arena);
  DN_DLList_InitArena(net->ws_list, DN_NET2RequestInternal, &net->arena);
  DN_DLList_InitArena(net->done_list, DN_NET2RequestInternal, &net->arena);

  DN_IStr8_AppendF(&net->curl_thread.name, "NET (CURL)");
  DN_OS_ThreadInit(&net->curl_thread, DN_NET2_ThreadEntryPoint_, net);
}

static void DN_NET2_SetupCurlRequest_(DN_NET2RequestInternal *request)
{
  DN_NET2CurlConn *conn = DN_CAST(DN_NET2CurlConn *) request->context;
  CURL            *curl = conn->curl;
  curl_easy_setopt(curl, CURLOPT_PRIVATE, request);

  // NOTE: Perform request and read all response headers before handing
  // control back to app.
  curl_easy_setopt(curl, CURLOPT_URL, request->url.data);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

  // NOTE: Setup response handler
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, DN_NET2_HTTPCallback_);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, request);

  // NOTE: Assign HTTP headers
  for (DN_ForItSize(it, DN_Str8, request->args.headers, request->args.headers_size))
    conn->curl_slist = curl_slist_append(conn->curl_slist, it.data->data);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, conn->curl_slist);

  // NOTE: Setup handle for protocol /////////////////////////////////////////////////////////
  switch (request->type) {
    case DN_NET2RequestType_Nil: DN_InvalidCodePath; break;

    case DN_NET2RequestType_WS: {
      curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 2L);
    } break;

    case DN_NET2RequestType_HTTP: {
      request->method    = DN_Str8_TrimWhitespaceAround(request->method);
      DN_Str8 const GET  = DN_STR8("GET");
      DN_Str8 const POST = DN_STR8("POST");

      if (DN_Str8_EqInsensitive(request->method, GET)) {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
      } else if (DN_Str8_EqInsensitive(request->method, POST)) {
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        if (request->args.payload.size > DN_Gigabytes(2))
          curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, request->args.payload.size);
        else
          curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, request->args.payload.size);
        curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, request->args.payload.data);
      } else {
        DN_InvalidCodePathF("Unimplemented");
      }
    } break;
  }

  // NOTE: Handle basic auth
  if (request->args.flags & DN_NET2DoHTTPFlags_BasicAuth) {
    if (DN_Str8_HasData(request->args.username) && DN_Str8_HasData(request->args.password)) {
      DN_Assert(request->args.username.data[request->args.username.size] == 0);
      DN_Assert(request->args.password.data[request->args.password.size] == 0);
      curl_easy_setopt(curl, CURLOPT_USERNAME, request->args.username.data);
      curl_easy_setopt(curl, CURLOPT_PASSWORD, request->args.password.data);
    }
  }
}

static DN_NET2Request DN_NET2_DoRequest_(DN_NET2Core *net, DN_Str8 url, DN_Str8 method, DN_NET2DoHTTPArgs const *args, DN_NET2RequestType type)
{
  DN_Assert(net);

  // NOTE: Allocate request
  DN_NET2RequestInternal *request = nullptr;
  for (DN_OS_MutexScope(&net->free_or_done_mutex))
    DN_DLList_Dequeue(net->free_list, request);

  if (!request) {
    request               = DN_Arena_New(&net->arena, DN_NET2RequestInternal, DN_ZeroMem_Yes);
    DN_NET2CurlConn *conn = new (request->context) DN_NET2CurlConn;
    conn->curl            = DN_CAST(CURL *)curl_easy_init();
  }

  // NOTE: Setup request
  DN_NET2Request result = {};
  if (request) {
    result.handle = DN_CAST(DN_U64) request;
    if (!request->arena.curr)
      request->arena = DN_Arena_FromVMem(DN_Megabytes(1), DN_Kilobytes(1), DN_ArenaFlags_Nil);

    request->type   = type;
    request->gen    = DN_Max(request->gen + 1, 1);
    request->url    = DN_Str8_FromStr8(&request->arena, url);
    request->method = DN_Str8_FromStr8(&request->arena, method);

    if (args) {
      request->args.flags    = args->flags;
      request->args.username = DN_Str8_FromStr8(&request->arena, args->username);
      request->args.password = DN_Str8_FromStr8(&request->arena, args->password);
      if (type == DN_NET2RequestType_HTTP)
        request->args.payload = DN_Str8_FromStr8(&request->arena, args->payload);

      request->args.headers = DN_Arena_NewArray(&request->arena, DN_Str8, args->headers_size, DN_ZeroMem_No);
      DN_Assert(request->args.headers);
      if (request->args.headers) {
        for (DN_ForItSize(it, DN_Str8, args->headers, args->headers_size))
          request->args.headers[it.index] = DN_Str8_FromStr8(&request->arena, *it.data);
        request->args.headers_size = args->headers_size;
      }
    }

    request->response.body            = DN_Str8Builder_FromArena(&request->arena);
    request->completion_sem           = DN_OS_SemaphoreInit(0);
    request->start_response_arena_pos = DN_Arena_Pos(&request->arena);

    DN_NET2_SetupCurlRequest_(request);

    // NOTE: Submit request to the networking thread's queue
    DN_NET2RingEvent event = {};
    event.type             = DN_NET2RingEventType_DoHTTP;
    event.request          = result;
    for (DN_OS_MutexScope(&net->ring_mutex))
      DN_Ring_WriteStruct(&net->ring, &event);

    curl_multi_wakeup(net->curlm);
  }

  return result;
}

static DN_NET2Request DN_NET2_DoHTTP(DN_NET2Core *net, DN_Str8 url, DN_Str8 method, DN_NET2DoHTTPArgs const *args)
{
  DN_NET2Request result = DN_NET2_DoRequest_(net, url, method, args, DN_NET2RequestType_HTTP);
  return result;
}

static DN_NET2Request DN_NET2_OpenWS(DN_NET2Core *net, DN_Str8 url, DN_NET2DoHTTPArgs const *args)
{
  DN_NET2Request result = DN_NET2_DoRequest_(net, url, DN_STR8(""), args, DN_NET2RequestType_WS);
  return result;
}

static void DN_NET2_SendWS(DN_NET2Core *net, DN_NET2Request request, DN_Str8 payload, DN_NET2WSType type)
{
  DN_Assert(type != DN_NET2WSType_Nil);
  DN_NET2RingEvent event     = {};
  event.type                 = DN_NET2RingEventType_SendWS;
  event.request              = request;
  event.send_ws_payload_size = payload.size;
  event.send_ws_type         = type;

  for (DN_OS_MutexScope(&net->ring_mutex)) {
    DN_Assert(DN_Ring_HasSpace(&net->ring, payload.size));
    DN_Ring_WriteStruct(&net->ring, &event);
    DN_Ring_Write(&net->ring, payload.data, payload.size);
  }
  curl_multi_wakeup(net->curlm);
}

