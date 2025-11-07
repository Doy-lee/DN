#include "dn_net2.h"
#include "dn_net_curl.h"

struct DN_NET2CurlConn
{
  void              *curl;
  struct curl_slist *curl_slist;
  char               error[CURL_ERROR_SIZE];
};

enum DN_NET2CurlRingEventType
{
  DN_NET2CurlRingEventType_Nil,
  DN_NET2CurlRingEventType_DoRequest,
  DN_NET2CurlRingEventType_SendWS,
  DN_NET2CurlRingEventType_ReceivedWSReceipt,
  DN_NET2CurlRingEventType_DeinitRequest,
};

struct DN_NET2CurlRingEvent
{
  DN_NET2CurlRingEventType type;
  DN_NET2Request           request;
  DN_USize                 ws_send_size;
  DN_NET2WSSend            ws_send;
};

struct DN_NET2CurlCore
{
  // NOTE: Shared w/ user and networking thread
  DN_Ring                 ring;
  DN_OSMutex              ring_mutex;

  DN_OSMutex              free_or_done_mutex; // Lock for free list and done list
  DN_OSThread             thread;

  // NOTE: Networking thread only
  void                   *curlm;
  DN_NET2RequestInternal *http_list;
  DN_NET2RequestInternal *ws_list;
};

DN_NET2Interface DN_NET2_CurlInterface()
{
  DN_NET2Interface result      = {};
  result.init                  = DN_NET2_CurlInit;
  result.do_http               = DN_NET2_CurlDoHTTP;
  result.do_ws                 = DN_NET2_CurlDoWS;
  result.do_ws_send            = DN_NET2_CurlDoWSSend;
  result.wait_for_response     = DN_NET2_CurlWaitForResponse;
  result.wait_for_any_response = DN_NET2_CurlWaitForAnyResponse;
  return result;
}

static void DN_NET2_CurlMarkRequestDone_(DN_NET2Core *net, DN_NET2RequestInternal *request)
{
  // NOTE: The done list in CURL is also used as a place to put websocket requests after removing it
  // from the 'ws_list'. By doing this we are stopping the CURL thread from receiving more data on
  // the socket as that thread ticks the list of 'ws_list' sockets for data.
  //
  // Once the caller waited and has received the data from the websocket, the request is put back
  // into the 'ws_list' which then lets the CURL thread start receiving more data for that socket.
  //
  // Since CURL uses a background thread, we do this behind a mutex
  DN_NET2CurlCore *curl = DN_Cast(DN_NET2CurlCore *)net->context;
  for (DN_OS_MutexScope(&curl->free_or_done_mutex))
    DN_DLList_Append(net->done_list, request);
  DN_OS_SemaphoreIncrement(&net->completion_sem, 1);
  DN_OS_SemaphoreIncrement(&request->completion_sem, 1);
}

static DN_USize DN_NET2_CurlHTTPCallback_(char *payload, DN_USize size, DN_USize count, void *user_data)
{
  auto    *request      = DN_Cast(DN_NET2RequestInternal *) user_data;
  DN_USize result       = 0;
  DN_USize payload_size = size * count;
  if (DN_Str8BuilderAppendBytesCopy(&request->response.body, payload, payload_size))
    result = payload_size;
  return result;
}

static int32_t DN_NET2_CurlThreadEntryPoint_(DN_OSThread *thread)
{
  DN_NET2Core     *net  = DN_Cast(DN_NET2Core *) thread->user_context;
  DN_NET2CurlCore *curl = DN_Cast(DN_NET2CurlCore *) net->context;
  DN_OS_ThreadSetName(DN_Str8FromPtr(curl->thread.name.data, curl->thread.name.size));

  for (;;) {
    DN_OSTLSTMem tmem = DN_OS_TLSPushTMem(nullptr);
    for (bool dequeue_ring = true; dequeue_ring;) {
      // NOTE: Dequeue user request
      DN_NET2CurlRingEvent event = {};
      for (DN_OS_MutexScope(&curl->ring_mutex)) {
        if (DN_Ring_HasData(&curl->ring, sizeof(event)))
            DN_Ring_Read(&curl->ring, &event, sizeof(event));
      }

      switch (event.type) {
        case DN_NET2CurlRingEventType_Nil: dequeue_ring = false; break;

        case DN_NET2CurlRingEventType_DoRequest: {
          DN_NET2RequestInternal *request = DN_Cast(DN_NET2RequestInternal *)event.request.handle;
          DN_Assert(request->response.state != DN_NET2ResponseState_Error);
          switch (request->type) {
            case DN_NET2RequestType_Nil: {
              DN_NET2_CurlMarkRequestDone_(net, request);
              DN_InvalidCodePath;
            } break;

            case DN_NET2RequestType_HTTP: {
              DN_Assert(request->response.state == DN_NET2ResponseState_Nil);
              DN_NET2CurlConn *conn      = DN_Cast(DN_NET2CurlConn *) request->context[0];
              CURLMcode        multi_add = curl_multi_add_handle(curl->curlm, conn->curl);
              DN_Assert(multi_add == CURLM_OK);
              DN_Assert(request->next == nullptr);
              DN_Assert(request->prev == nullptr);
              DN_DLList_Append(curl->http_list, request);
            } break;

            case DN_NET2RequestType_WS: {
              DN_Assert(request->response.state == DN_NET2ResponseState_Nil);
              DN_Assert(request->next == nullptr);
              DN_Assert(request->prev == nullptr);
              if (request->response.state == DN_NET2ResponseState_Nil) {
                DN_NET2CurlConn *conn      = DN_Cast(DN_NET2CurlConn *) request->context[0];
                CURLMcode        multi_add = curl_multi_add_handle(curl->curlm, conn->curl);
                DN_Assert(multi_add == CURLM_OK);
                DN_DLList_Append(curl->http_list, request); // Open the WS connection
              } else {
                DN_DLList_Append(curl->ws_list, request); // Ready to send data through the WS connection
              }
            } break;
          }
        } break;

        case DN_NET2CurlRingEventType_SendWS: {
          DN_Str8 payload = {};
          for (DN_OS_MutexScope(&curl->ring_mutex)) {
            DN_Assert(DN_Ring_HasData(&curl->ring, event.ws_send_size));
            payload = DN_Str8FromArena(tmem.arena, event.ws_send_size, DN_ZMem_No);
            DN_Ring_Read(&curl->ring, payload.data, payload.size);
          }

          DN_U32 curlws_flag = 0;
          switch (event.ws_send) {
            case DN_NET2WSSend_Text:   curlws_flag = CURLWS_TEXT; break;
            case DN_NET2WSSend_Binary: curlws_flag = CURLWS_BINARY; break;
            case DN_NET2WSSend_Close:  curlws_flag = CURLWS_CLOSE; break;
            case DN_NET2WSSend_Ping:   curlws_flag = CURLWS_PING; break;
            case DN_NET2WSSend_Pong:   curlws_flag = CURLWS_PONG; break;
          }

          DN_NET2RequestInternal *request = DN_Cast(DN_NET2RequestInternal *) event.request.handle;
          DN_Assert(request->type == DN_NET2RequestType_WS);
          DN_Assert(request->response.state == DN_NET2ResponseState_WSOpen);

          DN_NET2CurlConn *conn        = DN_Cast(DN_NET2CurlConn *) request->context[0];
          DN_USize         sent        = 0;
          CURLcode         send_result = curl_ws_send(conn->curl, payload.data, payload.size, &sent, 0, curlws_flag);
          DN_AssertF(send_result == CURLE_OK, "Failed to send: %s", curl_easy_strerror(send_result));
          DN_AssertF(sent == payload.size, "Failed to send all bytes (%zu vs %zu)", sent, payload.size);
        } break;

        case DN_NET2CurlRingEventType_ReceivedWSReceipt: {
          DN_NET2RequestInternal *request = DN_Cast(DN_NET2RequestInternal *) event.request.handle;
          DN_Assert(request->type == DN_NET2RequestType_WS);
          DN_Assert(request->response.state >= DN_NET2ResponseState_WSOpen && request->response.state <= DN_NET2ResponseState_WSPong);
          DN_Assert(request->next == nullptr);
          DN_Assert(request->prev == nullptr);
          request->response.state = DN_NET2ResponseState_WSOpen;
          DN_DLList_Append(curl->ws_list, request);
        } break;

        case DN_NET2CurlRingEventType_DeinitRequest: {
          if (event.request.handle != 0) {
            DN_NET2RequestInternal *request  = DN_Cast(DN_NET2RequestInternal *) event.request.handle;

            // NOTE: Release resources
            DN_ArenaClear(&request->arena);
            DN_OS_SemaphoreDeinit(&request->completion_sem);

            DN_NET2CurlConn *conn = DN_Cast(DN_NET2CurlConn *) request->context[0];
            curl_multi_remove_handle(curl->curlm, conn->curl);
            curl_easy_reset(conn->curl);
            curl_slist_free_all(conn->curl_slist);

            // NOTE: Zero the struct preserving just the data we need to retain
            DN_NET2RequestInternal resetter = {};
            resetter.arena                  = request->arena;
            resetter.gen                    = request->gen;
            DN_Memcpy(resetter.context, request->context, sizeof(resetter.context));
            *request                        = resetter;

            for (DN_OS_MutexScope(&curl->free_or_done_mutex))
              DN_DLList_Append(net->free_list, request);
          }
        } break;
      }
    }

    // NOTE: Pump handles
    int       running_handles = 0;
    CURLMcode perform_result  = curl_multi_perform(curl->curlm, &running_handles);
    if (perform_result != CURLM_OK)
      DN_InvalidCodePath;

    // NOTE: Check pump result
    for (;;) {
      int      msgs_in_queue = 0;
      CURLMsg *msg           = curl_multi_info_read(curl->curlm, &msgs_in_queue);
      if (msg) {
        // NOTE: Get request handle
        DN_NET2RequestInternal *request = nullptr;
        curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, DN_Cast(void **) & request);
        DN_Assert(request);

        DN_NET2CurlConn *conn = DN_Cast(DN_NET2CurlConn *)request->context[0];
        DN_Assert(conn->curl == msg->easy_handle);

        if (msg->data.result == CURLE_OK) {
          // NOTE: Get HTTP response code
          CURLcode get_result = curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &request->response.http_status);
          if (get_result == CURLE_OK) {
            if (request->type == DN_NET2RequestType_HTTP) {
              request->response.state = DN_NET2ResponseState_HTTP;
            } else {
              DN_Assert(request->type == DN_NET2RequestType_WS);
              request->response.state = DN_NET2ResponseState_WSOpen;
            }
          } else {
            request->response.error_str8 = DN_Str8FromFmtArena(&request->arena, "Failed to get HTTP response status (CURL %d): %s", msg->data.result, curl_easy_strerror(get_result));
            request->response.state     = DN_NET2ResponseState_Error;
          }
        } else {
          DN_USize curl_extended_error_size = DN_CStr8Size(conn->error);
          request->response.state      = DN_NET2ResponseState_Error;
          request->response.error_str8 = DN_Str8FromFmtArena(&request->arena,
                                                             "HTTP request '%.*s' failed (CURL %d): %s%s%s%s",
                                                             DN_Str8PrintFmt(request->url),
                                                             msg->data.result,
                                                             curl_easy_strerror(msg->data.result),
                                                             curl_extended_error_size ? " (" : "",
                                                             curl_extended_error_size ? conn->error : "",
                                                             curl_extended_error_size ? ")" : "");
        }

        if (request->type == DN_NET2RequestType_HTTP || request->response.state == DN_NET2ResponseState_Error) {
          // NOTE: Remove the request from the multi handle if we're a HTTP request
          // because it typically terminates the connection. In websockets the
          // connection remains in the multi-handle to allow you to send and
          // receive WS data from it.
          //
          // If there's an error (either websocket or HTTP) we will also remove the
          // connection from the multi handle as it failed. One a connection has
          // failed, curl will not poll that connection so there's no point keeping
          // it attached to the multi handle.
          curl_multi_remove_handle(curl->curlm, msg->easy_handle);
        }

        DN_NET2_CurlMarkRequestDone_(net, request);
      }

      if (msgs_in_queue == 0)
        break;
    }

    // NOTE: Check websockets
    DN_I32 sleep_time_ms = DN_DLList_HasItems(curl->ws_list) ? 100 : INT32_MAX;
    for (DN_DLList_ForEach(request, curl->ws_list)) {
      DN_Assert(request->type == DN_NET2RequestType_WS);
      DN_Assert(request->response.state == DN_NET2ResponseState_WSOpen);
      CURLcode             receive_result = CURLE_OK;
      const curl_ws_frame *meta           = nullptr;
      DN_NET2CurlConn *conn = DN_Cast(DN_NET2CurlConn *) request->context[0];
      for (;;) {
        // NOTE: Determine WS payload size received. Note that since we pass in a null pointer CURL
        // will set meta->len to 0 and say that there's meta->bytesleft in the next chunk.
        DN_USize bytes_read = 0;
        receive_result      = curl_ws_recv(conn->curl, nullptr, 0, &bytes_read, &meta);
        if (receive_result != CURLE_OK)
          break;
        DN_Assert(meta->len == 0);

        if (meta->flags & CURLWS_TEXT)
          request->response.state = DN_NET2ResponseState_WSText;

        if (meta->flags & CURLWS_BINARY)
          request->response.state = DN_NET2ResponseState_WSBinary;

        if (meta->flags & CURLWS_PING)
          request->response.state = DN_NET2ResponseState_WSPing;

        if (meta->flags & CURLWS_PONG)
          request->response.state = DN_NET2ResponseState_WSPong;

        if (meta->flags & CURLWS_CLOSE)
          request->response.state = DN_NET2ResponseState_WSClose;

        request->response.ws_has_more = meta->flags & CURLWS_CONT;
        if (request->response.ws_has_more) {
          bool is_text_or_binary = request->response.state == DN_NET2ResponseState_WSText ||
                                   request->response.state == DN_NET2ResponseState_WSBinary;
          DN_Assert(is_text_or_binary);
        }

        // NOTE: Allocate and read (we use meta->bytesleft as per comment from initial recv)
        if (meta->bytesleft) {
          DN_Str8 buffer  = DN_Str8FromArena(&request->arena, meta->bytesleft, DN_ZMem_No);
          DN_Assert(buffer.size == DN_Cast(DN_USize)meta->bytesleft);
          receive_result  = curl_ws_recv(conn->curl, buffer.data, buffer.size, &buffer.size, &meta);
          DN_Assert(buffer.size == DN_Cast(DN_USize)meta->len);
          DN_Str8BuilderAppendRef(&request->response.body, buffer);
        }

        // NOTE: There are more bytes coming if meta->bytesleft is set, (e.g. the next chunk. We
        // just read the current chunk).
        //
        //   > If this is not a complete fragment, the bytesleft field informs about how many
        //     additional bytes are expected to arrive before this fragment is complete.
        request->response.ws_has_more |= meta && meta->bytesleft > 0;
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

      if (receive_result != CURLE_OK) {
        DN_USize curl_extended_error_size = DN_CStr8Size(conn->error);
        request->response.state           = DN_NET2ResponseState_Error;
        request->response.error_str8      = DN_Str8FromFmtArena(&request->arena,
                                                           "Websocket receive '%.*s' failed (CURL %d): %s%s%s%s",
                                                           DN_Str8PrintFmt(request->url),
                                                           receive_result,
                                                           curl_easy_strerror(receive_result),
                                                           curl_extended_error_size ? " (" : "",
                                                           curl_extended_error_size ? conn->error : "",
                                                           curl_extended_error_size ? ")" : "");
      }

      DN_NET2RequestInternal *request_copy = request;
      request                              = request->prev;
      DN_NET2_CurlMarkRequestDone_(net, request_copy);
    }

    curl_multi_poll(curl->curlm, nullptr, 0, sleep_time_ms, nullptr);
  }
  return 0;
}


void DN_NET2_CurlInit(DN_NET2Core *net, char *base, DN_U64 base_size)
{
  DN_NET2_BaseInit_(net, base, base_size);
  DN_NET2CurlCore *curl      = DN_ArenaNew(&net->arena, DN_NET2CurlCore, DN_ZMem_Yes);
  net->context               = curl;

  DN_USize arena_bytes_avail = (net->arena.curr->reserve - net->arena.curr->used);
  curl->ring.size            = arena_bytes_avail / 2;
  curl->ring.base            = DN_Cast(char *) DN_ArenaAlloc(&net->arena, curl->ring.size, /*align*/ 1, DN_ZMem_Yes);
  DN_Assert(curl->ring.base);

  curl->ring_mutex         = DN_OS_MutexInit();
  curl->free_or_done_mutex = DN_OS_MutexInit();
  curl->curlm              = DN_Cast(CURLM *) curl_multi_init();

  // TODO: Free list should be initialised in the base
  DN_DLList_InitArena(net->free_list,  DN_NET2RequestInternal, &net->arena);
  DN_DLList_InitArena(net->done_list,  DN_NET2RequestInternal, &net->arena);
  DN_DLList_InitArena(curl->http_list, DN_NET2RequestInternal, &net->arena);
  DN_DLList_InitArena(curl->ws_list,   DN_NET2RequestInternal, &net->arena);

  DN_FmtAppend(curl->thread.name.data, &curl->thread.name.size, sizeof(curl->thread.name.data), "NET (CURL)");
  DN_OS_ThreadInit(&curl->thread, DN_NET2_CurlThreadEntryPoint_, net);
}

static DN_NET2Request DN_NET2_CurlDoRequest_(DN_NET2Core *net, DN_Str8 url, DN_Str8 method, DN_NET2DoHTTPArgs const *args, DN_NET2RequestType type)
{
  // NOTE: Allocate the request
  DN_NET2CurlCore        *curl_core = DN_Cast(DN_NET2CurlCore *) net->context;
  DN_NET2RequestInternal *request   = nullptr;
  DN_NET2Request          result    = {};
  {
    // NOTE: The free list is modified by both the calling thread and the CURLM thread (which ticks
    // all the requests in the background for us)
    for (DN_OS_MutexScope(&curl_core->free_or_done_mutex))
      DN_DLList_Dequeue(net->free_list, request);

    // NOTE None if the free list so allocate one
    if (!request) {
      DN_U64 arena_pos      = DN_ArenaPos(&net->arena);
      request               = DN_ArenaNew(&net->arena, DN_NET2RequestInternal, DN_ZMem_Yes);
      DN_NET2CurlConn *conn = DN_ArenaNew(&net->arena, DN_NET2CurlConn, DN_ZMem_Yes);
      if (!request || !conn) {
        DN_ArenaPopTo(&net->arena, arena_pos);
        return result;
      }

      conn->curl          = DN_Cast(CURL *) curl_easy_init();
      request->context[0] = DN_Cast(DN_UPtr) conn;
    }
  }

  // NOTE: Setup the request
  result              = DN_NET2_SetupRequest_(request, url, method, args, type);
  request->context[1] = DN_Cast(DN_UPtr) net;

  // NOTE: Setup the request for curl
  {
    DN_NET2CurlConn *conn = DN_Cast(DN_NET2CurlConn *) request->context[0];
    CURL            *curl = conn->curl;
    curl_easy_setopt(curl, CURLOPT_PRIVATE, request);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, conn->error);

    // NOTE: Perform request and read all response headers before handing
    // control back to app.
    curl_easy_setopt(curl, CURLOPT_URL, request->url.data);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

    // NOTE: Setup response handler
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, DN_NET2_CurlHTTPCallback_);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, request);

    // NOTE: Assign HTTP headers
    for (DN_ForItSize(it, DN_Str8, request->args.headers, request->args.headers_size))
      conn->curl_slist = curl_slist_append(conn->curl_slist, it.data->data);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, conn->curl_slist);

    // NOTE: Setup handle for protocol
    switch (request->type) {
      case DN_NET2RequestType_Nil: DN_InvalidCodePath; break;

      case DN_NET2RequestType_WS: {
        curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 2L);
      } break;

      case DN_NET2RequestType_HTTP: {
        DN_Str8 const GET  = DN_Str8Lit("GET");
        DN_Str8 const POST = DN_Str8Lit("POST");

        if (DN_Str8EqInsensitive(request->method, GET)) {
          curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
        } else if (DN_Str8EqInsensitive(request->method, POST)) {
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
      if (request->args.username.size && request->args.password.size) {
        DN_Assert(request->args.username.data[request->args.username.size] == 0);
        DN_Assert(request->args.password.data[request->args.password.size] == 0);
        curl_easy_setopt(curl, CURLOPT_USERNAME, request->args.username.data);
        curl_easy_setopt(curl, CURLOPT_PASSWORD, request->args.password.data);
      }
    }
  }

  // NOTE: Dispatch the request to the CURL thread. It will append to the CURLM
  // instance and tick it in the background for us
  {
    DN_NET2CurlRingEvent event = {};
    event.type                 = DN_NET2CurlRingEventType_DoRequest;
    event.request              = result;
    for (DN_OS_MutexScope(&curl_core->ring_mutex))
      DN_Ring_WriteStruct(&curl_core->ring, &event);
    curl_multi_wakeup(curl_core->curlm);
  }

  return result;
}

DN_NET2Request DN_NET2_CurlDoHTTP(DN_NET2Core *net, DN_Str8 url, DN_Str8 method, DN_NET2DoHTTPArgs const *args)
{
  DN_NET2Request result = DN_NET2_CurlDoRequest_(net, url, method, args, DN_NET2RequestType_HTTP);
  return result;
}

DN_NET2Request DN_NET2_CurlDoWSArgs(DN_NET2Core *net, DN_Str8 url, DN_NET2DoHTTPArgs const *args)
{
  DN_NET2Request result = DN_NET2_CurlDoRequest_(net, url, DN_Str8Lit(""), args, DN_NET2RequestType_WS);
  return result;
}

DN_NET2Request DN_NET2_CurlDoWS(DN_NET2Core *net, DN_Str8 url)
{
  DN_NET2Request result = DN_NET2_CurlDoWSArgs(net, url, nullptr);
  return result;
}

void DN_NET2_CurlDoWSSend(DN_NET2Request request, DN_Str8 payload, DN_NET2WSSend send)
{
  DN_NET2RequestInternal *request_ptr = DN_Cast(DN_NET2RequestInternal *) request.handle;
  if (!request_ptr || request_ptr->gen != request.gen)
    return;

  DN_NET2Core     *net  = DN_Cast(DN_NET2Core *) request_ptr->context[1];
  DN_NET2CurlCore *curl = DN_Cast(DN_NET2CurlCore *) net->context;
  DN_Assert(curl);

  DN_NET2CurlRingEvent event = {};
  event.type                 = DN_NET2CurlRingEventType_SendWS;
  event.request              = request;
  event.ws_send_size         = payload.size;
  event.ws_send              = send;

  for (DN_OS_MutexScope(&curl->ring_mutex)) {
    DN_Assert(DN_Ring_HasSpace(&curl->ring, payload.size));
    DN_Ring_WriteStruct(&curl->ring, &event);
    DN_Ring_Write(&curl->ring, payload.data, payload.size);
  }
  curl_multi_wakeup(curl->curlm);
}

static DN_NET2Response DN_NET2_CurlHandleFinishedRequest_(DN_NET2Core *net, DN_NET2CurlCore *curl, DN_NET2Request request, DN_NET2RequestInternal *request_ptr, DN_Arena *arena)
{
  // NOTE: Process the response
  DN_NET2Response result = DN_NET2_MakeResponseFromFinishedRequest_(request, arena);
  DN_NET2_EndFinishedRequest_(net, request_ptr);

  // NOTE: For websocket requests, notify the CURL thread we've read data from it and it can go
  // back to polling the socket for more data. Over on the CURL thread when it receives the event
  // it will append the request onto the 'ws_list' which it iterates to poll for data.
  //
  // The request was _just_ sitting in the done list (see above) so it was not being polled but
  // it will be polled after this step.
  //
  // TODO: How do we unsticky the error? Right now we just deallocate/close the request entirely and move on
  bool end_the_request = true;
  if (request_ptr->type == DN_NET2RequestType_WS &&
      request_ptr->response.state != DN_NET2ResponseState_Error &&
      request_ptr->response.state != DN_NET2ResponseState_WSClose) {
    DN_NET2CurlRingEvent event = {};
    event.type                 = DN_NET2CurlRingEventType_ReceivedWSReceipt;
    event.request              = request;
    for (DN_OS_MutexScope(&curl->ring_mutex))
      DN_Ring_WriteStruct(&curl->ring, &event);
    curl_multi_wakeup(curl->curlm);
    end_the_request = false;
  }

  if (end_the_request) {
    // NOTE: This _has_ to be sent to our CURL thread because we need to remove the CURL handle from
    // the CURLM instance and the CURL thread uses the CURLM instance (e.g. not thread safe to do
    // here)
    DN_NET2CurlRingEvent event = {};
    event.type                 = DN_NET2CurlRingEventType_DeinitRequest;
    event.request              = request;
    for (DN_OS_MutexScope(&curl->ring_mutex))
      DN_Ring_WriteStruct(&curl->ring, &event);
  }

  return result;
}

DN_NET2Response DN_NET2_CurlWaitForResponse(DN_NET2Request request, DN_Arena *arena, DN_U32 timeout_ms)
{
  DN_NET2Response         result      = {};
  DN_NET2RequestInternal *request_ptr = DN_Cast(DN_NET2RequestInternal *) request.handle;
  if (!request_ptr || request_ptr->gen != request.gen)
    return result;

  DN_NET2Core     *net  = DN_Cast(DN_NET2Core *) request_ptr->context[1];
  DN_NET2CurlCore *curl = DN_Cast(DN_NET2CurlCore *) net->context;
  DN_Assert(curl);

  DN_OSSemaphoreWaitResult wait = DN_OS_SemaphoreWait(&request_ptr->completion_sem, timeout_ms);
  if (wait != DN_OSSemaphoreWaitResult_Success)
    return result;

  // NOTE: Remove the request from the done list. Note it _has_ to be in the done list, anything
  // sitting in the done list is guaranteed to not be modified by the CURL thread.
  for (DN_OS_MutexScope(&curl->free_or_done_mutex)) {
    bool is_in_done_list = false;
    for (DN_DLList_ForEach(it, net->done_list)) {
      if (it == request_ptr) {
        is_in_done_list = true;
        break;
      }
    }
    DN_AssertF(is_in_done_list, "Any request that has signalled completion should be inserted into the done list");
    DN_DLList_Detach(request_ptr);
  }

  // NOTE: Finish handling the response
  result = DN_NET2_CurlHandleFinishedRequest_(net, curl, request, request_ptr, arena);

  // NOTE: Decrement the global 'request done' completion semaphore since the user consumed the
  // request individually.
  DN_OSSemaphoreWaitResult net_wait_result = DN_OS_SemaphoreWait(&net->completion_sem, 0 /*timeout_ms*/);
  DN_AssertF(net_wait_result == DN_OSSemaphoreWaitResult_Success, "Wait result was: %zu", DN_Cast(DN_USize) net_wait_result);

  return result;
}

DN_NET2Response DN_NET2_CurlWaitForAnyResponse(DN_NET2Core *net, DN_Arena *arena, DN_U32 timeout_ms)
{
  DN_NET2CurlCore *curl = DN_Cast(DN_NET2CurlCore *) net->context;
  DN_Assert(curl);

  DN_NET2Response          result = {};
  DN_OSSemaphoreWaitResult wait   = DN_OS_SemaphoreWait(&net->completion_sem, timeout_ms);
  if (wait != DN_OSSemaphoreWaitResult_Success)
      return result;

  // NOTE: Dequeue the request that is done from the done list
  DN_NET2RequestInternal *request_ptr = nullptr;
  for (DN_OS_MutexScope(&curl->free_or_done_mutex))
    DN_DLList_Dequeue(net->done_list, request_ptr);
  DN_Assert(request_ptr);

  // NOTE: Decrement the request's completion semaphore since the user consumed the global semaphore
  DN_OSSemaphoreWaitResult net_wait_result = DN_OS_SemaphoreWait(&request_ptr->completion_sem, 0 /*timeout_ms*/);
  DN_AssertF(net_wait_result == DN_OSSemaphoreWaitResult_Success, "Wait result was: %zu", DN_Cast(DN_USize) net_wait_result);

  // NOTE: Finish handling the response
  DN_NET2Request request = {};
  request.handle         = DN_Cast(DN_UPtr) request_ptr;
  request.gen            = request_ptr->gen;
  result                 = DN_NET2_CurlHandleFinishedRequest_(net, curl, request, request_ptr, arena);

  return result;
}

