#define DN_NET_CURL_CPP

#if defined(_CLANGD)
  #define DN_H_WITH_OS 1
  #include "../dn.h"
  #include "dn_net.h"
  #include "dn_net_curl.h"
#endif

struct DN_NETCurlRequest
{
  void              *handle;
  struct curl_slist *slist;
  char               error[CURL_ERROR_SIZE];
  bool               ws_has_more;
  DN_Str8Builder     str8_builder;
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
  DN_NETRequestHandle     request;
  DN_USize                ws_send_size;
  DN_NETWSSend            ws_send;
};

static DN_NETCurlRequest *DN_NET_CurlRequestFromRequest_(DN_NETRequest *req)
{
  DN_NETCurlRequest *result = req ? DN_Cast(DN_NETCurlRequest *) req->context[0] : 0;
  return result;
}

static DN_NETCore *DN_NET_CurlNetFromRequest(DN_NETRequest *req)
{
  DN_NETCore *result = req ? DN_Cast(DN_NETCore *) req->context[1] : 0;
  return result;
}

static bool DN_NET_CurlRequestIsInList(DN_NETRequest const *first, DN_NETRequest const *find)
{
  bool result = false;
  for (DN_NETRequest const *it = first; !result && it; it = it->next)
    result = find == it;
  return result;
}

static void DN_NET_CurlMarkRequestDone_(DN_NETCore *net, DN_NETRequest *request)
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
  DN_NETRequest     *req          = DN_Cast(DN_NETRequest *) user_data;
  DN_NETCurlRequest *curl_req     = DN_NET_CurlRequestFromRequest_(req);
  DN_USize           result       = 0;
  DN_USize           payload_size = size * count;
  if (DN_Str8BuilderAppendBytesCopy(&curl_req->str8_builder, payload, payload_size))
    result = payload_size;
  return result;
}

static int32_t DN_NET_CurlThreadEntryPoint_(DN_OSThread *thread)
{
  DN_NETCore     *net  = DN_Cast(DN_NETCore *) thread->user_context;
  DN_NETCurlCore *curl = DN_Cast(DN_NETCurlCore *) net->context;
  DN_OS_ThreadSetNameFmt("%.*s", DN_Str8PrintFmt(curl->thread.name));

  while (!curl->kill_thread) {
    DN_TCScratch tmem = DN_TCScratchBegin(nullptr, 0);

    // NOTE: Handle events sitting in the ring queue
    for (bool dequeue_ring = true; dequeue_ring;) {
      DN_NETCurlRingEvent event = {};
      for (DN_OS_MutexScope(&curl->ring_mutex)) {
        if (DN_RingHasData(&curl->ring, sizeof(event)))
            DN_RingRead(&curl->ring, &event, sizeof(event));
      }

      DN_NETRequest     *req      = DN_NET_RequestFromHandle(event.request);
      DN_NETCurlRequest *curl_req = DN_NET_CurlRequestFromRequest_(req);
      switch (event.type) {
        case DN_NETCurlRingEventType_Nil: dequeue_ring = false; break;

        case DN_NETCurlRingEventType_DoRequest: {
          DN_Assert(req->response.state == DN_NETResponseState_Nil);
          DN_Assert(req->type           != DN_NETRequestType_Nil);

          // NOTE: Attach it to the CURL thread's request list 
          for (DN_OS_MutexScope(&curl->list_mutex)) {
            DN_Assert(DN_NET_CurlRequestIsInList(curl->request_list, req));
            DN_DoublyLLDetach(curl->request_list, req);
          }
          DN_DoublyLLAppend(curl->thread_request_list, req);

          // NOTE: Add the connection to CURLM and start ticking it once we finish handling all the
          // ring events
          CURLMcode multi_add = curl_multi_add_handle(curl->thread_curlm, curl_req->handle);
          DN_Assert(multi_add == CURLM_OK);
        } break;

        case DN_NETCurlRingEventType_SendWS: {
          DN_Str8 payload = {};
          for (DN_OS_MutexScope(&curl->ring_mutex)) {
            DN_Assert(DN_RingHasData(&curl->ring, event.ws_send_size));
            payload = DN_Str8AllocArena(event.ws_send_size, DN_ZMem_No, &tmem.arena);
            DN_RingRead(&curl->ring, payload.data, payload.size);
          }

          DN_U32 curlws_flag = 0;
          switch (event.ws_send) {
            case DN_NETWSSend_Text:   curlws_flag = CURLWS_TEXT; break;
            case DN_NETWSSend_Binary: curlws_flag = CURLWS_BINARY; break;
            case DN_NETWSSend_Close:  curlws_flag = CURLWS_CLOSE; break;
            case DN_NETWSSend_Ping:   curlws_flag = CURLWS_PING; break;
            case DN_NETWSSend_Pong:   curlws_flag = CURLWS_PONG; break;
          }

          DN_Assert(req->type           == DN_NETRequestType_WS);
          DN_Assert(req->response.state >= DN_NETResponseState_WSOpen && req->response.state <= DN_NETResponseState_WSPong);

          DN_USize sent           = 0;
          CURLcode send_result    = curl_ws_send(curl_req->handle, payload.data, payload.size, &sent, 0, curlws_flag);
          DN_AssertF(send_result == CURLE_OK, "Failed to send: %s", curl_easy_strerror(send_result));
          DN_AssertF(sent        == payload.size, "Failed to send all bytes (%zu vs %zu)", sent, payload.size);
        } break;

        case DN_NETCurlRingEventType_ReceivedWSReceipt: {
          DN_Assert(req->type == DN_NETRequestType_WS);
          DN_Assert(req->response.state >= DN_NETResponseState_WSOpen && req->response.state <= DN_NETResponseState_WSPong);
          req->response.state = DN_NETResponseState_WSOpen;

          for (DN_OS_MutexScope(&curl->list_mutex)) {
            DN_Assert(DN_NET_CurlRequestIsInList(curl->request_list, req));
            DN_DoublyLLDetach(curl->request_list, req);
          }
          DN_DoublyLLAppend(curl->thread_request_list, req);
        } break;

        case DN_NETCurlRingEventType_DeinitRequest: {
          DN_Assert(event.request.handle != 0);
          DN_NETRequest *request = DN_Cast(DN_NETRequest *) event.request.handle;

          // NOTE: Release resources
          DN_ArenaTempEnd(&request->arena, DN_ArenaReset_Yes);
          DN_OS_SemaphoreDeinit(&request->completion_sem);

          curl_multi_remove_handle(curl->thread_curlm, curl_req->handle);
          curl_easy_reset(curl_req->handle);
          curl_slist_free_all(curl_req->slist);

          // NOTE: Zero the struct preserving just the data we need to retain
          DN_NETRequest resetter = {};
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
        DN_NETRequest *req = nullptr;
        curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, DN_Cast(void **) & req);
        DN_Assert(req);
        DN_Assert(DN_NET_CurlRequestIsInList(curl->thread_request_list, req));

        DN_NETCurlRequest *curl_req = DN_NET_CurlRequestFromRequest_(req);
        DN_Assert(curl_req->handle == msg->easy_handle);

        if (msg->data.result == CURLE_OK) {
          // NOTE: Get HTTP response code
          CURLcode get_result = curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &req->response.http_status);
          if (get_result == CURLE_OK) {
            if (req->type == DN_NETRequestType_HTTP) {
              req->response.state = DN_NETResponseState_HTTP;
            } else {
              DN_Assert(req->type == DN_NETRequestType_WS);
              req->response.state = DN_NETResponseState_WSOpen;
            }
          } else {
            req->response.error_str8 = DN_Str8FromFmtArena(&req->arena, "Failed to get HTTP response status (CURL %d): %s", msg->data.result, curl_easy_strerror(get_result));
            req->response.state      = DN_NETResponseState_Error;
          }
        } else {
          DN_USize curl_extended_error_size = DN_CStr8Size(curl_req->error);
          req->response.state               = DN_NETResponseState_Error;
          req->response.error_str8          = DN_Str8FromFmtArena(&req->arena,
                                                         "HTTP request '%.*s' failed (CURL %d): %s%s%s%s",
                                                         DN_Str8PrintFmt(req->url),
                                                         msg->data.result,
                                                         curl_easy_strerror(msg->data.result),
                                                         curl_extended_error_size ? " (" : "",
                                                         curl_extended_error_size ? curl_req->error : "",
                                                         curl_extended_error_size ? ")" : "");
        }

        if (req->type == DN_NETRequestType_HTTP || req->response.state == DN_NETResponseState_Error) {
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

        DN_NET_CurlMarkRequestDone_(net, req);
      }

      if (msgs_in_queue == 0)
        break;
    }

    // NOTE: Check websockets
    DN_USize ws_count = 0;
    for (DN_NETRequest *req = curl->thread_request_list; req; req = req->next) {
      DN_Assert(req->type == DN_NETRequestType_WS || req->type == DN_NETRequestType_HTTP);
      if (req->type != DN_NETRequestType_WS || !(req->response.state >= DN_NETResponseState_WSOpen && req->response.state <= DN_NETResponseState_WSPong))
        continue;
      ws_count++;
      const curl_ws_frame *meta           = nullptr;
      DN_NETCurlRequest   *curl_req       = DN_NET_CurlRequestFromRequest_(req);
      CURLcode             receive_result = CURLE_OK;
      while (receive_result == CURLE_OK) {
        // NOTE: Determine WS payload size received. Note that since we pass in a null pointer CURL
        // will set meta->len to 0 and say that there's meta->bytesleft in the next chunk.
        DN_USize bytes_read = 0;
        receive_result      = curl_ws_recv(curl_req->handle, nullptr, 0, &bytes_read, &meta);
        if (receive_result != CURLE_OK)
          continue;
        DN_Assert(meta->len == 0);

        if (meta->flags & CURLWS_TEXT)
          req->response.state = DN_NETResponseState_WSText;

        if (meta->flags & CURLWS_BINARY)
          req->response.state = DN_NETResponseState_WSBinary;

        if (meta->flags & CURLWS_PING)
          req->response.state = DN_NETResponseState_WSPing;

        if (meta->flags & CURLWS_PONG)
          req->response.state = DN_NETResponseState_WSPong;

        if (meta->flags & CURLWS_CLOSE)
          req->response.state = DN_NETResponseState_WSClose;

        curl_req->ws_has_more = meta->flags & CURLWS_CONT;
        if (curl_req->ws_has_more) {
          bool is_text_or_binary = req->response.state == DN_NETResponseState_WSText ||
                                   req->response.state == DN_NETResponseState_WSBinary;
          DN_Assert(is_text_or_binary);
        }

        // NOTE: Allocate and read (we use meta->bytesleft as per comment from initial recv)
        if (meta->bytesleft) {
          DN_Str8 buffer  = DN_Str8AllocArena(meta->bytesleft, DN_ZMem_No, &req->arena);
          DN_Assert(buffer.size == DN_Cast(DN_USize)meta->bytesleft);
          receive_result  = curl_ws_recv(curl_req->handle, buffer.data, buffer.size, &buffer.size, &meta);
          DN_Assert(buffer.size == DN_Cast(DN_USize)meta->len);
          DN_Str8BuilderAppendRef(&curl_req->str8_builder, buffer);
        }

        // NOTE: There are more bytes coming if meta->bytesleft is set, (e.g. the next chunk. We
        // just read the current chunk).
        //
        //   > If this is not a complete fragment, the bytesleft field informs about how many
        //     additional bytes are expected to arrive before this fragment is complete.
        curl_req->ws_has_more |= meta && meta->bytesleft > 0;
        if (!curl_req->ws_has_more)
          break;
      }

      // NOTE: curl_ws_recv returns CURLE_GOT_NOTHING if the associated connection is closed.
      if (receive_result == CURLE_GOT_NOTHING)
        curl_req->ws_has_more = false;

      // NOTE: We read all the possible bytes that CURL has received for this message, but, there are
      // more bytes left that we will receive on subsequent calls. We will continue to the next
      // request and return back to this one when PumpRequests is called again where hopefully that
      // data has arrived.
      if (curl_req->ws_has_more)
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
      bool received_data = (req->response.state >= DN_NETResponseState_WSText && req->response.state <= DN_NETResponseState_WSPong);
      if (receive_result == CURLE_AGAIN && !received_data)
        continue;

      if (!received_data) {
        if (receive_result == CURLE_GOT_NOTHING) {
          req->response.state = DN_NETResponseState_WSClose;
        } else if (receive_result != CURLE_OK) {
          DN_USize curl_extended_error_size = DN_CStr8Size(curl_req->error);
          req->response.state           = DN_NETResponseState_Error;
          req->response.error_str8      = DN_Str8FromFmtArena(&req->arena,
                                                                  "Websocket receive '%.*s' failed (CURL %d): %s%s%s%s",
                                                                  DN_Str8PrintFmt(req->url),
                                                                  receive_result,
                                                                  curl_easy_strerror(receive_result),
                                                                  curl_extended_error_size ? " (" : "",
                                                                  curl_extended_error_size ? curl_req->error : "",
                                                                  curl_extended_error_size ? ")" : "");
        }
      }

      DN_NETRequest *request_copy = req;
      req                             = req->prev;
      DN_NET_CurlMarkRequestDone_(net, request_copy);
      if (!req)
        break;
    }

    DN_I32 sleep_time_ms = ws_count > 0 ? 16 : INT32_MAX;
    curl_multi_poll(curl->thread_curlm, nullptr, 0, sleep_time_ms, nullptr);
    DN_TCScratchEnd(&tmem);
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
  DN_NETCurlCore *curl = DN_ArenaNew(&net->arena, DN_NETCurlCore, DN_ZMem_Yes);
  net->context         = curl;
  net->api             = DN_NET_CurlInterface();

  DN_USize arena_bytes_avail = (net->arena.mem->curr->reserve - net->arena.mem->curr->used);
  curl->ring.size            = arena_bytes_avail / 2;
  curl->ring.base            = DN_Cast(char *) DN_ArenaAlloc(&net->arena, curl->ring.size, /*align*/ 1, DN_ZMem_Yes);
  DN_Assert(curl->ring.base);

  curl->ring_mutex   = DN_OS_MutexInit();
  curl->list_mutex   = DN_OS_MutexInit();
  curl->thread_curlm = DN_Cast(CURLM *) curl_multi_init();

  DN_FmtAppend(curl->thread.name.data, &curl->thread.name.size, sizeof(curl->thread.name.data), "NET (CURL)");
  DN_OS_ThreadInit(&curl->thread, DN_NET_CurlThreadEntryPoint_, nullptr, net);
}

void DN_NET_CurlDeinit(DN_NETCore *net)
{
  DN_NETCurlCore *curl = DN_Cast(DN_NETCurlCore *) net->context;
  curl->kill_thread    = true;
  curl_multi_wakeup(curl->thread_curlm);
  DN_OS_ThreadJoin(&curl->thread, DN_TCDeinitArenas_Yes);
}

static DN_NETRequestHandle DN_NET_CurlDoRequest_(DN_NETCore *net, DN_Str8 url, DN_Str8 method, DN_NETDoHTTPArgs const *args, DN_NETRequestType type)
{
  // NOTE: Allocate the request
  DN_NETCurlCore     *curl_core = DN_Cast(DN_NETCurlCore *) net->context;
  DN_NETRequest      *req       = nullptr;
  DN_NETRequestHandle result    = {};
  {
    // NOTE: The free list is modified by both the calling thread and the CURLM thread (which ticks
    // all the requests in the background for us)
    for (DN_OS_MutexScope(&curl_core->list_mutex)) {
      req = curl_core->free_list;
      DN_DoublyLLDetach(curl_core->free_list, req);
    }

    // NOTE None in the free list so allocate one
    if (!req) {
      DN_U64 arena_pos            = DN_MemListPos(net->arena.mem);
      req                         = DN_ArenaNew(&net->arena, DN_NETRequest, DN_ZMem_Yes);
      DN_NETCurlRequest *curl_req = DN_ArenaNew(&net->arena, DN_NETCurlRequest, DN_ZMem_Yes);
      if (!req || !curl_req) {
        DN_MemListPopTo(net->arena.mem, arena_pos);
        return result;
      }

      curl_req->handle = DN_Cast(CURL *) curl_easy_init();
      req->context[0]  = DN_Cast(DN_UPtr) curl_req;
    }
  }

  // NOTE: Setup the request
  DN_NETCurlRequest *curl_req = DN_NET_CurlRequestFromRequest_(req);
  {
    result                 = DN_NET_SetupRequest_(req, url, method, args, type);
    req->response.request  = result;
    req->context[1]        = DN_Cast(DN_UPtr) net;
    curl_req->str8_builder = DN_Str8BuilderFromArena(&req->start_response_arena);
  }

  // NOTE: Setup the request for curl API
  {
    CURL *curl = curl_req->handle;
    curl_easy_setopt(curl, CURLOPT_PRIVATE, req);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_req->error);

    // NOTE: Perform request and read all response headers before handing
    // control back to app.
    curl_easy_setopt(curl, CURLOPT_URL, req->url.data);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

    // NOTE: Setup response handler
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, DN_NET_CurlHTTPCallback_);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, req);

    // NOTE: Assign HTTP headers
    for (DN_ForItSize(it, DN_Str8, req->args.headers, req->args.headers_size))
      curl_req->slist = curl_slist_append(curl_req->slist, it.data->data);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_req->slist);

    // NOTE: Setup handle for protocol
    switch (req->type) {
      case DN_NETRequestType_Nil: DN_InvalidCodePath; break;

      case DN_NETRequestType_WS: {
        curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 2L);
      } break;

      case DN_NETRequestType_HTTP: {
        DN_Str8 const GET  = DN_Str8Lit("GET");
        DN_Str8 const POST = DN_Str8Lit("POST");

        if (DN_Str8EqInsensitive(req->method, GET)) {
          curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
        } else if (DN_Str8EqInsensitive(req->method, POST)) {
          curl_easy_setopt(curl, CURLOPT_POST, 1);
          if (req->args.payload.size > DN_Gigabytes(2))
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, req->args.payload.size);
          else
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, req->args.payload.size);
          curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, req->args.payload.data);
        } else {
          DN_InvalidCodePathF("Unimplemented");
        }
      } break;
    }

    // NOTE: Handle basic auth
    if (req->args.flags & DN_NETDoHTTPFlags_BasicAuth) {
      if (req->args.username.size && req->args.password.size) {
        DN_Assert(req->args.username.data[req->args.username.size] == 0);
        DN_Assert(req->args.password.data[req->args.password.size] == 0);
        curl_easy_setopt(curl, CURLOPT_USERNAME, req->args.username.data);
        curl_easy_setopt(curl, CURLOPT_PASSWORD, req->args.password.data);
      }
    }
  }

  // NOTE: Dispatch the request to the CURL thread
  {
    // NOTE: Immediately add the request to the request list so it happens "atomically" in the
    // calling thread. If the calling thread deinitialises this layer before the CURL thread can be
    // pre-empted, we can lose track of this request.
    for (DN_OS_MutexScope(&curl_core->list_mutex))
      DN_DoublyLLAppend(curl_core->request_list, req);

    // NOTE: Enqueue request to go into CURL's ring queue. The CURL thread will sleep and wait for
    // bytes to come in for the request and then dump the response into the done list to be consumed
    // via wait for response
    DN_NETCurlRingEvent event = {};
    event.type                = DN_NETCurlRingEventType_DoRequest;
    event.request             = result;
    for (DN_OS_MutexScope(&curl_core->ring_mutex))
      DN_RingWriteStruct(&curl_core->ring, &event);

    curl_multi_wakeup(curl_core->thread_curlm);
  }

  return result;
}

DN_NETRequestHandle DN_NET_CurlDoHTTP(DN_NETCore *net, DN_Str8 url, DN_Str8 method, DN_NETDoHTTPArgs const *args)
{
  DN_NETRequestHandle result = DN_NET_CurlDoRequest_(net, url, method, args, DN_NETRequestType_HTTP);
  return result;
}

DN_NETRequestHandle DN_NET_CurlDoWSArgs(DN_NETCore *net, DN_Str8 url, DN_NETDoHTTPArgs const *args)
{
  DN_NETRequestHandle result = DN_NET_CurlDoRequest_(net, url, DN_Str8Lit(""), args, DN_NETRequestType_WS);
  return result;
}

DN_NETRequestHandle DN_NET_CurlDoWS(DN_NETCore *net, DN_Str8 url)
{
  DN_NETRequestHandle result = DN_NET_CurlDoWSArgs(net, url, nullptr);
  return result;
}

void DN_NET_CurlDoWSSend(DN_NETRequestHandle handle, DN_Str8 payload, DN_NETWSSend send)
{
  DN_NETRequest *req = DN_NET_RequestFromHandle(handle);
  if (!req)
    return;

  DN_NETCore     *net  = DN_NET_CurlNetFromRequest(req);
  DN_NETCurlCore *curl = DN_Cast(DN_NETCurlCore *) net->context;
  DN_Assert(curl);

  DN_NETCurlRingEvent event = {};
  event.type                 = DN_NETCurlRingEventType_SendWS;
  event.request              = handle;
  event.ws_send_size         = payload.size;
  event.ws_send              = send;

  for (DN_OS_MutexScope(&curl->ring_mutex)) {
    DN_Assert(DN_RingHasSpace(&curl->ring, payload.size));
    DN_RingWriteStruct(&curl->ring, &event);
    DN_RingWrite(&curl->ring, payload.data, payload.size);
  }
  curl_multi_wakeup(curl->thread_curlm);
}

static DN_NETResponse DN_NET_CurlHandleFinishedRequest_(DN_NETCurlCore *curl, DN_NETRequest *req, DN_Arena *arena)
{
  // NOTE: Generate the response, copy out the strings into the user given memory
  DN_NETResponse result = req->response;
  {
    DN_NETCurlRequest *curl_req = DN_NET_CurlRequestFromRequest_(req);
    result.body                 = DN_Str8BuilderBuild(&curl_req->str8_builder, arena);
    if (result.error_str8.size)
      result.error_str8 = DN_Str8FromStr8Arena(result.error_str8, arena);
    curl_req->str8_builder = {}; // Clear it out, reinitialised on subsequent do request call
  }
  DN_NET_EndFinishedRequest_(req);

  bool continue_ws_request = false;
  if (req->type == DN_NETRequestType_WS &&
      req->response.state != DN_NETResponseState_Error &&
      req->response.state != DN_NETResponseState_WSClose) {
    continue_ws_request = true;
  }

  // NOTE: Put the request into the requisite list
  for (DN_OS_MutexScope(&curl->list_mutex)) {
    // NOTE: Dequeue the request, it _must_ have been in the response list at this point for it to
    // have ben waitable in the first place.
    DN_AssertF(DN_NET_CurlRequestIsInList(curl->response_list, req),
               "A completed response should only signal the completion semaphore when it's in the response list");
    DN_DoublyLLDetach(curl->response_list, req);

    // NOTE: A websocket that is continuing to get data should go back into the request list because
    // there's more data to be received. All other requests need to go into the deinit list (so that
    // we keep track of it in the time inbetween it takes for the CURL thread to be scheduled and
    // release the CURL handle from CURLM and release resources e.t.c.)
    if (continue_ws_request)
      DN_DoublyLLAppend(curl->request_list, req);
    else
      DN_DoublyLLAppend(curl->deinit_list, req);
  }

  // NOTE: Submit the post-request event to the CURL thread
  DN_NETCurlRingEvent event = {};
  event.request             = DN_NET_HandleFromRequest(req);
  if (continue_ws_request) {
    event.type = DN_NETCurlRingEventType_ReceivedWSReceipt;
  } else {
    // NOTE: Deinit _has_ to be sent to the CURL thread because we need to remove the CURL handle
    // from the CURLM instance and the CURL thread uses the CURLM instance (e.g. CURLM is not thread
    // safe)
    event.type = DN_NETCurlRingEventType_DeinitRequest;
  }

  for (DN_OS_MutexScope(&curl->ring_mutex))
    DN_RingWriteStruct(&curl->ring, &event);
  curl_multi_wakeup(curl->thread_curlm);

  return result;
}

DN_NETResponse DN_NET_CurlWaitForResponse(DN_NETRequestHandle handle, DN_Arena *arena, DN_U32 timeout_ms)
{
  DN_NETResponse result = {};
  DN_NETRequest *req    = DN_NET_RequestFromHandle(handle);
  if (!req)
    return result;

  DN_NETCore     *net  = DN_Cast(DN_NETCore *) req->context[1];
  DN_NETCurlCore *curl = DN_Cast(DN_NETCurlCore *) net->context;
  DN_Assert(curl);

  DN_OSSemaphoreWaitResult wait = DN_OS_SemaphoreWait(&req->completion_sem, timeout_ms);
  if (wait != DN_OSSemaphoreWaitResult_Success)
    return result;

  // NOTE: Decrement the global 'request done' completion semaphore since the user consumed the
  // request individually.
  DN_OSSemaphoreWaitResult net_wait_result = DN_OS_SemaphoreWait(&net->completion_sem, 0 /*timeout_ms*/);
  DN_AssertF(net_wait_result == DN_OSSemaphoreWaitResult_Success, "Wait result was: %zu", DN_Cast(DN_USize) net_wait_result);

  // NOTE: Finish handling the response
  result = DN_NET_CurlHandleFinishedRequest_(curl, req, arena);
  return result;
}

DN_NETResponse DN_NET_CurlWaitForAnyResponse(DN_NETCore *net, DN_Arena *arena, DN_U32 timeout_ms)
{
  DN_NETCurlCore *curl = DN_Cast(DN_NETCurlCore *) net->context;
  DN_Assert(curl);

  DN_NETResponse           result   = {};
  DN_OSSemaphoreWaitResult req_wait = DN_OS_SemaphoreWait(&net->completion_sem, timeout_ms);
  if (req_wait != DN_OSSemaphoreWaitResult_Success)
    return result;

  // NOTE: Just grab the handle, handle finished request will dequeue for us
  DN_NETRequestHandle handle = {};
  for (DN_OS_MutexScope(&curl->list_mutex)) {
    DN_Assert(curl->response_list);
    handle = DN_NET_HandleFromRequest(curl->response_list);
  }

  // NOTE: Decrement the request's completion semaphore since the user consumed the global semaphore
  DN_NETRequest           *req      = DN_NET_RequestFromHandle(handle);
  DN_OSSemaphoreWaitResult net_wait = DN_OS_SemaphoreWait(&req->completion_sem, 0 /*timeout_ms*/);
  DN_AssertF(net_wait == DN_OSSemaphoreWaitResult_Success, "Wait result was: %zu", DN_Cast(DN_USize) net_wait);

  // NOTE: Finish handling the response
  result = DN_NET_CurlHandleFinishedRequest_(curl, req, arena);
  return result;
}
