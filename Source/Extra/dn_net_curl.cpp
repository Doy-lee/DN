#include "dn_net.h"
#include "dn_net_curl.h"

struct DN_NETCurlConn
{
  void              *curl;
  struct curl_slist *curl_slist;
  char               error[CURL_ERROR_SIZE];
};

enum DN_NETCurlRingEventType
{
  DN_NETCurlRingEventType_Nil,
  DN_NETCurlRingEventType_DoRequest,
  DN_NETCurlRingEventType_SendWS,
  DN_NETCurlRingEventType_ReceivedWSReceipt,
  DN_NETCurlRingEventType_DeinitRequest,
};

struct DN_NETCurlRingEvent
{
  DN_NETCurlRingEventType type;
  DN_NETRequest           request;
  DN_USize                ws_send_size;
  DN_NETWSSend            ws_send;
};

struct DN_NETCurlCore
{
  // NOTE: Shared w/ user and networking thread
  DN_Ring                ring;
  DN_OSMutex             ring_mutex;
  bool                   kill_thread;

  DN_OSMutex             list_mutex;          // Lock for request, response, deinit, free list
  DN_NETRequestInternal *request_list;        // Current requests submitted by the user thread awaiting to move into the thread request list
  DN_NETRequestInternal *thread_request_list; // Current requests being executed by the CURL thread.
                                              // This list is exclusively owned by the CURL thread so no locking is needed
  DN_NETRequestInternal *response_list;       // Finished requests that are to be deqeued by the user via wait for response
  DN_NETRequestInternal *deinit_list;         // Requests that are finished and are awaiting to be de-initialised by the CURL thread
  DN_NETRequestInternal *free_list;           // Request pool that new requests will use before allocating

  // NOTE: Networking thread only
  DN_OSThread            thread;
  void                  *thread_curlm;
};

static bool DN_NET_CurlRequestIsInList(DN_NETRequestInternal const *first, DN_NETRequestInternal const *find)
{
  bool result = false;
  for (DN_NETRequestInternal const *it = first; !result && it; it = it->next)
    result = find == it;
  return result;
}

static void DN_NET_CurlMarkRequestDone_(DN_NETCore *net, DN_NETRequestInternal *request)
{
  DN_Assert(request);
  DN_Assert(net);
  // NOTE: The done list in CURL is also used as a place to put websocket requests after removing it
  // from the 'ws_list'. By doing this we are stopping the CURL thread from receiving more data on
  // the socket as that thread ticks the list of 'ws_list' sockets for data.
  //
  // Once the caller waited and has received the data from the websocket, the request is put back
  // into the 'ws_list' which then lets the CURL thread start receiving more data for that socket.
  //
  // Since CURL uses a background thread, we do this behind a mutex
  DN_NETCurlCore *curl = DN_Cast(DN_NETCurlCore *)net->context;
  for (DN_OS_MutexScope(&curl->list_mutex)) {
    DN_Assert(DN_NET_CurlRequestIsInList(curl->thread_request_list, request));
    DN_DoublyLLDetach(curl->thread_request_list, request);
    DN_DoublyLLAppend(curl->response_list, request);
  }
  DN_OS_SemaphoreIncrement(&net->completion_sem, 1);
  DN_OS_SemaphoreIncrement(&request->completion_sem, 1);
}

static DN_USize DN_NET_CurlHTTPCallback_(char *payload, DN_USize size, DN_USize count, void *user_data)
{
  auto    *request      = DN_Cast(DN_NETRequestInternal *) user_data;
  DN_USize result       = 0;
  DN_USize payload_size = size * count;
  if (DN_Str8BuilderAppendBytesCopy(&request->response.body, payload, payload_size))
    result = payload_size;
  return result;
}

static int32_t DN_NET_CurlThreadEntryPoint_(DN_OSThread *thread)
{
  DN_NETCore     *net  = DN_Cast(DN_NETCore *) thread->user_context;
  DN_NETCurlCore *curl = DN_Cast(DN_NETCurlCore *) net->context;
  DN_OS_ThreadSetName(DN_Str8FromPtr(curl->thread.name.data, curl->thread.name.size));

  while (!curl->kill_thread) {
    DN_OSTLSTMem tmem = DN_OS_TLSPushTMem(nullptr);

    // NOTE: Handle events sitting in the ring queue
    for (bool dequeue_ring = true; dequeue_ring;) {
      DN_NETCurlRingEvent event = {};
      for (DN_OS_MutexScope(&curl->ring_mutex)) {
        if (DN_Ring_HasData(&curl->ring, sizeof(event)))
            DN_Ring_Read(&curl->ring, &event, sizeof(event));
      }

      switch (event.type) {
        case DN_NETCurlRingEventType_Nil: dequeue_ring = false; break;

        case DN_NETCurlRingEventType_DoRequest: {
          DN_NETRequestInternal *request = DN_Cast(DN_NETRequestInternal *)event.request.handle;
          DN_Assert(request->response.state == DN_NETResponseState_Nil);
          DN_Assert(request->type           != DN_NETRequestType_Nil);

          // NOTE: Attach it to the CURL thread's request list 
          for (DN_OS_MutexScope(&curl->list_mutex)) {
            DN_Assert(DN_NET_CurlRequestIsInList(curl->request_list, request));
            DN_DoublyLLDetach(curl->request_list, request);
          }
          DN_DoublyLLAppend(curl->thread_request_list, request);

          // NOTE: Add the connection to CURLM and start ticking it once we finish handling all the
          // ring events
          DN_NETCurlConn *conn      = DN_Cast(DN_NETCurlConn *) request->context[0];
          CURLMcode       multi_add = curl_multi_add_handle(curl->thread_curlm, conn->curl);
          DN_Assert(multi_add == CURLM_OK);
        } break;

        case DN_NETCurlRingEventType_SendWS: {
          DN_Str8 payload = {};
          for (DN_OS_MutexScope(&curl->ring_mutex)) {
            DN_Assert(DN_Ring_HasData(&curl->ring, event.ws_send_size));
            payload = DN_Str8FromArena(tmem.arena, event.ws_send_size, DN_ZMem_No);
            DN_Ring_Read(&curl->ring, payload.data, payload.size);
          }

          DN_U32 curlws_flag = 0;
          switch (event.ws_send) {
            case DN_NETWSSend_Text:   curlws_flag = CURLWS_TEXT; break;
            case DN_NETWSSend_Binary: curlws_flag = CURLWS_BINARY; break;
            case DN_NETWSSend_Close:  curlws_flag = CURLWS_CLOSE; break;
            case DN_NETWSSend_Ping:   curlws_flag = CURLWS_PING; break;
            case DN_NETWSSend_Pong:   curlws_flag = CURLWS_PONG; break;
          }

          DN_NETRequestInternal *request = DN_Cast(DN_NETRequestInternal *) event.request.handle;
          DN_Assert(request->type           == DN_NETRequestType_WS);
          DN_Assert(request->response.state == DN_NETResponseState_WSOpen);
          DN_Assert(DN_NET_CurlRequestIsInList(curl->thread_request_list, request));

          DN_NETCurlConn *conn        = DN_Cast(DN_NETCurlConn *) request->context[0];
          DN_USize        sent        = 0;
          CURLcode        send_result = curl_ws_send(conn->curl, payload.data, payload.size, &sent, 0, curlws_flag);
          DN_AssertF(send_result == CURLE_OK, "Failed to send: %s", curl_easy_strerror(send_result));
          DN_AssertF(sent == payload.size, "Failed to send all bytes (%zu vs %zu)", sent, payload.size);
        } break;

        case DN_NETCurlRingEventType_ReceivedWSReceipt: {
          DN_NETRequestInternal *request = DN_Cast(DN_NETRequestInternal *) event.request.handle;
          DN_Assert(request->type == DN_NETRequestType_WS);
          DN_Assert(request->response.state >= DN_NETResponseState_WSOpen && request->response.state <= DN_NETResponseState_WSPong);
          request->response.state = DN_NETResponseState_WSOpen;

          for (DN_OS_MutexScope(&curl->list_mutex)) {
            DN_Assert(DN_NET_CurlRequestIsInList(curl->request_list, request));
            DN_DoublyLLDetach(curl->request_list, request);
          }
          DN_DoublyLLAppend(curl->thread_request_list, request);
        } break;

        case DN_NETCurlRingEventType_DeinitRequest: {
          DN_Assert(event.request.handle != 0);
          DN_NETRequestInternal *request = DN_Cast(DN_NETRequestInternal *) event.request.handle;

          // NOTE: Release resources
          DN_ArenaClear(&request->arena);
          DN_OS_SemaphoreDeinit(&request->completion_sem);

          DN_NETCurlConn *conn = DN_Cast(DN_NETCurlConn *) request->context[0];
          curl_multi_remove_handle(curl->thread_curlm, conn->curl);
          curl_easy_reset(conn->curl);
          curl_slist_free_all(conn->curl_slist);

          // NOTE: Zero the struct preserving just the data we need to retain
          DN_NETRequestInternal resetter = {};
          resetter.arena                 = request->arena;
          resetter.gen                   = request->gen;
          DN_Memcpy(resetter.context, request->context, sizeof(resetter.context));
          *request = resetter;

          // NOTE: Add it to the free list
          for (DN_OS_MutexScope(&curl->list_mutex)) {
            DN_Assert(DN_NET_CurlRequestIsInList(curl->deinit_list, request));
            DN_DoublyLLDetach(curl->deinit_list, request);
            DN_DoublyLLAppend(curl->free_list, request);
          }
        } break;
      }
    }

    // NOTE: Pump handles
    int       running_handles = 0;
    CURLMcode perform_result  = curl_multi_perform(curl->thread_curlm, &running_handles);
    if (perform_result != CURLM_OK)
      DN_InvalidCodePath;

    // NOTE: Check pump result
    for (;;) {
      int      msgs_in_queue = 0;
      CURLMsg *msg           = curl_multi_info_read(curl->thread_curlm, &msgs_in_queue);
      if (msg) {
        // NOTE: Get request handle
        DN_NETRequestInternal *request = nullptr;
        curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, DN_Cast(void **) & request);
        DN_Assert(request);
        DN_Assert(DN_NET_CurlRequestIsInList(curl->thread_request_list, request));

        DN_NETCurlConn *conn = DN_Cast(DN_NETCurlConn *)request->context[0];
        DN_Assert(conn->curl == msg->easy_handle);

        if (msg->data.result == CURLE_OK) {
          // NOTE: Get HTTP response code
          CURLcode get_result = curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &request->response.http_status);
          if (get_result == CURLE_OK) {
            if (request->type == DN_NETRequestType_HTTP) {
              request->response.state = DN_NETResponseState_HTTP;
            } else {
              DN_Assert(request->type == DN_NETRequestType_WS);
              request->response.state = DN_NETResponseState_WSOpen;
            }
          } else {
            request->response.error_str8 = DN_Str8FromFmtArena(&request->arena, "Failed to get HTTP response status (CURL %d): %s", msg->data.result, curl_easy_strerror(get_result));
            request->response.state     = DN_NETResponseState_Error;
          }
        } else {
          DN_USize curl_extended_error_size = DN_CStr8Size(conn->error);
          request->response.state      = DN_NETResponseState_Error;
          request->response.error_str8 = DN_Str8FromFmtArena(&request->arena,
                                                             "HTTP request '%.*s' failed (CURL %d): %s%s%s%s",
                                                             DN_Str8PrintFmt(request->url),
                                                             msg->data.result,
                                                             curl_easy_strerror(msg->data.result),
                                                             curl_extended_error_size ? " (" : "",
                                                             curl_extended_error_size ? conn->error : "",
                                                             curl_extended_error_size ? ")" : "");
        }

        if (request->type == DN_NETRequestType_HTTP || request->response.state == DN_NETResponseState_Error) {
          // NOTE: Remove the request from the multi handle if we're a HTTP request
          // because it typically terminates the connection. In websockets the
          // connection remains in the multi-handle to allow you to send and
          // receive WS data from it.
          //
          // If there's an error (either websocket or HTTP) we will also remove the
          // connection from the multi handle as it failed. One a connection has
          // failed, curl will not poll that connection so there's no point keeping
          // it attached to the multi handle.
          curl_multi_remove_handle(curl->thread_curlm, msg->easy_handle);
        }

        DN_NET_CurlMarkRequestDone_(net, request);
      }

      if (msgs_in_queue == 0)
        break;
    }

    // NOTE: Check websockets
    DN_USize ws_count = 0;
    for (DN_NETRequestInternal *request = curl->thread_request_list; request; request = request->next) {
      DN_Assert(request->type == DN_NETRequestType_WS || request->type == DN_NETRequestType_HTTP);
      if (request->type != DN_NETRequestType_WS || !(request->response.state >= DN_NETResponseState_WSOpen && request->response.state <= DN_NETResponseState_WSPong))
        continue;
      ws_count++;
      const curl_ws_frame *meta           = nullptr;
      DN_NETCurlConn      *conn           = DN_Cast(DN_NETCurlConn *) request->context[0];
      CURLcode             receive_result = CURLE_OK;
      while (receive_result == CURLE_OK) {
        // NOTE: Determine WS payload size received. Note that since we pass in a null pointer CURL
        // will set meta->len to 0 and say that there's meta->bytesleft in the next chunk.
        DN_USize bytes_read = 0;
        receive_result      = curl_ws_recv(conn->curl, nullptr, 0, &bytes_read, &meta);
        if (receive_result != CURLE_OK)
          continue;
        DN_Assert(meta->len == 0);

        if (meta->flags & CURLWS_TEXT)
          request->response.state = DN_NETResponseState_WSText;

        if (meta->flags & CURLWS_BINARY)
          request->response.state = DN_NETResponseState_WSBinary;

        if (meta->flags & CURLWS_PING)
          request->response.state = DN_NETResponseState_WSPing;

        if (meta->flags & CURLWS_PONG)
          request->response.state = DN_NETResponseState_WSPong;

        if (meta->flags & CURLWS_CLOSE)
          request->response.state = DN_NETResponseState_WSClose;

        request->response.ws_has_more = meta->flags & CURLWS_CONT;
        if (request->response.ws_has_more) {
          bool is_text_or_binary = request->response.state == DN_NETResponseState_WSText ||
                                   request->response.state == DN_NETResponseState_WSBinary;
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
      }

      // NOTE: curl_ws_recv returns CURLE_GOT_NOTHING if the associated connection is closed.
      if (receive_result == CURLE_GOT_NOTHING)
        request->response.ws_has_more = false;

      // NOTE: We read all the possible bytes that CURL has received for this message, but, there are
      // more bytes left that we will receive on subsequent calls. We will continue to the next
      // request and return back to this one when PumpRequests is called again where hopefully that
      // data has arrived.
      if (request->response.ws_has_more)
        continue;

      // For CURLE_AGAIN
      //
      //   > Instead of blocking, the function returns CURLE_AGAIN. The correct behavior is then to
      //   > wait for the socket to signal readability before calling this function again.
      //
      // In which case we continue ticking the other sockets and eventually exit once all ticked.
      // Right after this we wait on the CURLM instance which will wake us up again when there's
      // data to be read.
      //
      // if we received data, e.g. state was set to Text, Binary ... e.t.c we bypass this and
      // report it to the user first. When the user waits for the response, they consume the data
      // and then that will reinsert it into request list for CURL to read from the socket again.
      bool received_data = (request->response.state >= DN_NETResponseState_WSText && request->response.state <= DN_NETResponseState_WSPong);
      if (receive_result == CURLE_AGAIN && !received_data)
        continue;

      if (!received_data) {
        if (receive_result == CURLE_GOT_NOTHING) {
          request->response.state = DN_NETResponseState_WSClose;
        } else if (receive_result != CURLE_OK) {
          DN_USize curl_extended_error_size = DN_CStr8Size(conn->error);
          request->response.state           = DN_NETResponseState_Error;
          request->response.error_str8      = DN_Str8FromFmtArena(&request->arena,
                                                             "Websocket receive '%.*s' failed (CURL %d): %s%s%s%s",
                                                             DN_Str8PrintFmt(request->url),
                                                             receive_result,
                                                             curl_easy_strerror(receive_result),
                                                             curl_extended_error_size ? " (" : "",
                                                             curl_extended_error_size ? conn->error : "",
                                                             curl_extended_error_size ? ")" : "");
        }
      }

      DN_NETRequestInternal *request_copy = request;
      request                             = request->prev;
      DN_NET_CurlMarkRequestDone_(net, request_copy);
      if (!request)
        break;
    }

    DN_I32 sleep_time_ms = ws_count > 0 ? 16 : INT32_MAX;
    curl_multi_poll(curl->thread_curlm, nullptr, 0, sleep_time_ms, nullptr);
  }

  return 0;
}

DN_NETInterface DN_NET_CurlInterface()
{
  DN_NETInterface result      = {};
  result.init                  = DN_NET_CurlInit;
  result.deinit                = DN_NET_CurlDeinit;
  result.do_http               = DN_NET_CurlDoHTTP;
  result.do_ws                 = DN_NET_CurlDoWS;
  result.do_ws_send            = DN_NET_CurlDoWSSend;
  result.wait_for_response     = DN_NET_CurlWaitForResponse;
  result.wait_for_any_response = DN_NET_CurlWaitForAnyResponse;
  return result;
}

void DN_NET_CurlInit(DN_NETCore *net, char *base, DN_U64 base_size)
{
  DN_NET_BaseInit_(net, base, base_size);
  DN_NETCurlCore *curl       = DN_ArenaNew(&net->arena, DN_NETCurlCore, DN_ZMem_Yes);
  net->context               = curl;

  DN_USize arena_bytes_avail = (net->arena.curr->reserve - net->arena.curr->used);
  curl->ring.size            = arena_bytes_avail / 2;
  curl->ring.base            = DN_Cast(char *) DN_ArenaAlloc(&net->arena, curl->ring.size, /*align*/ 1, DN_ZMem_Yes);
  DN_Assert(curl->ring.base);

  curl->ring_mutex   = DN_OS_MutexInit();
  curl->list_mutex   = DN_OS_MutexInit();
  curl->thread_curlm = DN_Cast(CURLM *) curl_multi_init();

  DN_FmtAppend(curl->thread.name.data, &curl->thread.name.size, sizeof(curl->thread.name.data), "NET (CURL)");
  DN_OS_ThreadInit(&curl->thread, DN_NET_CurlThreadEntryPoint_, net);
}

void DN_NET_CurlDeinit(DN_NETCore *net)
{
  DN_NETCurlCore *curl = DN_Cast(DN_NETCurlCore *) net->context;
  curl->kill_thread    = true;
  curl_multi_wakeup(curl->thread_curlm);
  DN_OS_ThreadDeinit(&curl->thread);
}

static DN_NETRequest DN_NET_CurlDoRequest_(DN_NETCore *net, DN_Str8 url, DN_Str8 method, DN_NETDoHTTPArgs const *args, DN_NETRequestType type)
{
  // NOTE: Allocate the request
  DN_NETCurlCore        *curl_core = DN_Cast(DN_NETCurlCore *) net->context;
  DN_NETRequestInternal *request   = nullptr;
  DN_NETRequest          result    = {};
  {
    // NOTE: The free list is modified by both the calling thread and the CURLM thread (which ticks
    // all the requests in the background for us)
    for (DN_OS_MutexScope(&curl_core->list_mutex)) {
      request = curl_core->free_list;
      DN_DoublyLLDetach(curl_core->free_list, request);
    }

    // NOTE None in the free list so allocate one
    if (!request) {
      DN_U64 arena_pos     = DN_ArenaPos(&net->arena);
      request              = DN_ArenaNew(&net->arena, DN_NETRequestInternal, DN_ZMem_Yes);
      DN_NETCurlConn *conn = DN_ArenaNew(&net->arena, DN_NETCurlConn, DN_ZMem_Yes);
      if (!request || !conn) {
        DN_ArenaPopTo(&net->arena, arena_pos);
        return result;
      }

      conn->curl          = DN_Cast(CURL *) curl_easy_init();
      request->context[0] = DN_Cast(DN_UPtr) conn;
    }
  }

  // NOTE: Setup the request
  result              = DN_NET_SetupRequest_(request, url, method, args, type);
  request->context[1] = DN_Cast(DN_UPtr) net;

  // NOTE: Setup the request for curl
  {
    DN_NETCurlConn *conn = DN_Cast(DN_NETCurlConn *) request->context[0];
    CURL            *curl = conn->curl;
    curl_easy_setopt(curl, CURLOPT_PRIVATE, request);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, conn->error);

    // NOTE: Perform request and read all response headers before handing
    // control back to app.
    curl_easy_setopt(curl, CURLOPT_URL, request->url.data);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

    // NOTE: Setup response handler
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, DN_NET_CurlHTTPCallback_);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, request);

    // NOTE: Assign HTTP headers
    for (DN_ForItSize(it, DN_Str8, request->args.headers, request->args.headers_size))
      conn->curl_slist = curl_slist_append(conn->curl_slist, it.data->data);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, conn->curl_slist);

    // NOTE: Setup handle for protocol
    switch (request->type) {
      case DN_NETRequestType_Nil: DN_InvalidCodePath; break;

      case DN_NETRequestType_WS: {
        curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 2L);
      } break;

      case DN_NETRequestType_HTTP: {
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
    if (request->args.flags & DN_NETDoHTTPFlags_BasicAuth) {
      if (request->args.username.size && request->args.password.size) {
        DN_Assert(request->args.username.data[request->args.username.size] == 0);
        DN_Assert(request->args.password.data[request->args.password.size] == 0);
        curl_easy_setopt(curl, CURLOPT_USERNAME, request->args.username.data);
        curl_easy_setopt(curl, CURLOPT_PASSWORD, request->args.password.data);
      }
    }
  }

  // NOTE: Dispatch the request to the CURL thread
  {
    // NOTE: Immediately add the request to the request list so it happens "atomically" in the
    // calling thread. If the calling thread deinitialises this layer before the CURL thread can be
    // pre-empted, we can lose track of this request.
    for (DN_OS_MutexScope(&curl_core->list_mutex))
      DN_DoublyLLAppend(curl_core->request_list, request);

    // NOTE: Enqueue request to go into CURL's ring queue. The CURL thread will sleep and wait for
    // bytes to come in for the request and then dump the response into the done list to be consumed
    // via wait for response
    DN_NETCurlRingEvent event = {};
    event.type                = DN_NETCurlRingEventType_DoRequest;
    event.request             = result;
    for (DN_OS_MutexScope(&curl_core->ring_mutex))
      DN_Ring_WriteStruct(&curl_core->ring, &event);

    curl_multi_wakeup(curl_core->thread_curlm);
  }

  return result;
}

DN_NETRequest DN_NET_CurlDoHTTP(DN_NETCore *net, DN_Str8 url, DN_Str8 method, DN_NETDoHTTPArgs const *args)
{
  DN_NETRequest result = DN_NET_CurlDoRequest_(net, url, method, args, DN_NETRequestType_HTTP);
  return result;
}

DN_NETRequest DN_NET_CurlDoWSArgs(DN_NETCore *net, DN_Str8 url, DN_NETDoHTTPArgs const *args)
{
  DN_NETRequest result = DN_NET_CurlDoRequest_(net, url, DN_Str8Lit(""), args, DN_NETRequestType_WS);
  return result;
}

DN_NETRequest DN_NET_CurlDoWS(DN_NETCore *net, DN_Str8 url)
{
  DN_NETRequest result = DN_NET_CurlDoWSArgs(net, url, nullptr);
  return result;
}

void DN_NET_CurlDoWSSend(DN_NETRequest request, DN_Str8 payload, DN_NETWSSend send)
{
  DN_NETRequestInternal *request_ptr = DN_NET_RequestFromHandle(request);
  if (!request_ptr)
    return;

  DN_NETCore     *net  = DN_Cast(DN_NETCore *) request_ptr->context[1];
  DN_NETCurlCore *curl = DN_Cast(DN_NETCurlCore *) net->context;
  DN_Assert(curl);

  DN_NETCurlRingEvent event = {};
  event.type                 = DN_NETCurlRingEventType_SendWS;
  event.request              = request;
  event.ws_send_size         = payload.size;
  event.ws_send              = send;

  for (DN_OS_MutexScope(&curl->ring_mutex)) {
    DN_Assert(DN_Ring_HasSpace(&curl->ring, payload.size));
    DN_Ring_WriteStruct(&curl->ring, &event);
    DN_Ring_Write(&curl->ring, payload.data, payload.size);
  }
  curl_multi_wakeup(curl->thread_curlm);
}

static DN_NETResponse DN_NET_CurlHandleFinishedRequest_(DN_NETCurlCore *curl, DN_NETRequest request, DN_NETRequestInternal *request_ptr, DN_Arena *arena)
{
  // NOTE: Process the response
  DN_NETResponse result = DN_NET_MakeResponseFromFinishedRequest_(request, arena);
  DN_NET_EndFinishedRequest_(request_ptr);

  bool continue_ws_request = false;
  if (request_ptr->type == DN_NETRequestType_WS &&
      request_ptr->response.state != DN_NETResponseState_Error &&
      request_ptr->response.state != DN_NETResponseState_WSClose) {
    continue_ws_request = true;
  }

  // NOTE: Put the request into the requisite list
  for (DN_OS_MutexScope(&curl->list_mutex)) {
    // NOTE: Dequeue the request, it _must_ have been in the response list at this point for it to
    // have ben waitable in the first place.
    DN_AssertF(DN_NET_CurlRequestIsInList(curl->response_list, request_ptr),
               "A completed response should only signal the completion semaphore when it's in the response list");
    DN_DoublyLLDetach(curl->response_list, request_ptr);

    // NOTE: A websocket that is continuing to get data should go back into the request list because
    // there's more data to be received. All other requests need to go into the deinit list (so that
    // we keep track of it in the time inbetween it takes for the CURL thread to be scheduled and
    // release the CURL handle from CURLM and release resources e.t.c.)
    if (continue_ws_request)
      DN_DoublyLLAppend(curl->request_list, request_ptr);
    else
      DN_DoublyLLAppend(curl->deinit_list, request_ptr);
  }

  // NOTE: Submit the post-request event to the CURL thread
  DN_NETCurlRingEvent event = {};
  event.request             = request;
  if (continue_ws_request) {
    event.type = DN_NETCurlRingEventType_ReceivedWSReceipt;
  } else {
    // NOTE: Deinit _has_ to be sent to the CURL thread because we need to remove the CURL handle
    // from the CURLM instance and the CURL thread uses the CURLM instance (e.g. CURLM is not thread
    // safe)
    event.type = DN_NETCurlRingEventType_DeinitRequest;
  }

  for (DN_OS_MutexScope(&curl->ring_mutex))
    DN_Ring_WriteStruct(&curl->ring, &event);
  curl_multi_wakeup(curl->thread_curlm);

  return result;
}

DN_NETResponse DN_NET_CurlWaitForResponse(DN_NETRequest request, DN_Arena *arena, DN_U32 timeout_ms)
{
  DN_NETResponse         result      = {};
  DN_NETRequestInternal *request_ptr = DN_NET_RequestFromHandle(request);
  if (!request_ptr)
    return result;

  DN_NETCore     *net  = DN_Cast(DN_NETCore *) request_ptr->context[1];
  DN_NETCurlCore *curl = DN_Cast(DN_NETCurlCore *) net->context;
  DN_Assert(curl);

  DN_OSSemaphoreWaitResult wait = DN_OS_SemaphoreWait(&request_ptr->completion_sem, timeout_ms);
  if (wait != DN_OSSemaphoreWaitResult_Success)
    return result;

  // NOTE: Decrement the global 'request done' completion semaphore since the user consumed the
  // request individually.
  DN_OSSemaphoreWaitResult net_wait_result = DN_OS_SemaphoreWait(&net->completion_sem, 0 /*timeout_ms*/);
  DN_AssertF(net_wait_result == DN_OSSemaphoreWaitResult_Success, "Wait result was: %zu", DN_Cast(DN_USize) net_wait_result);

  // NOTE: Finish handling the response
  result = DN_NET_CurlHandleFinishedRequest_(curl, request, request_ptr, arena);
  return result;
}

DN_NETResponse DN_NET_CurlWaitForAnyResponse(DN_NETCore *net, DN_Arena *arena, DN_U32 timeout_ms)
{
  DN_NETCurlCore *curl = DN_Cast(DN_NETCurlCore *) net->context;
  DN_Assert(curl);

  DN_NETResponse           result = {};
  DN_OSSemaphoreWaitResult wait   = DN_OS_SemaphoreWait(&net->completion_sem, timeout_ms);
  if (wait != DN_OSSemaphoreWaitResult_Success)
    return result;

  // NOTE: Just grab the handle, handle finished request will dequeue for us
  DN_NETRequest request = {};
  for (DN_OS_MutexScope(&curl->list_mutex)) {
    DN_Assert(curl->response_list);
    request = DN_NET_HandleFromRequest(curl->response_list);
  }

  // NOTE: Decrement the request's completion semaphore since the user consumed the global semaphore
  DN_NETRequestInternal   *request_ptr     = DN_NET_RequestFromHandle(request);
  DN_OSSemaphoreWaitResult net_wait_result = DN_OS_SemaphoreWait(&request_ptr->completion_sem, 0 /*timeout_ms*/);
  DN_AssertF(net_wait_result == DN_OSSemaphoreWaitResult_Success, "Wait result was: %zu", DN_Cast(DN_USize) net_wait_result);

  // NOTE: Finish handling the response
  result = DN_NET_CurlHandleFinishedRequest_(curl, request, request_ptr, arena);
  return result;
}
