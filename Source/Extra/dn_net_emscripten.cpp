#if !defined(__EMSCRIPTEN__)
  #error "This file can only be compiled with Emscripten"
#endif

#include <emscripten.h>
#include <emscripten/fetch.h>
#include <emscripten/websocket.h>

#if defined(_CLANGD)
  #include "dn_net.h"
  #include "dn_net_emscripten.h"
#endif

struct DN_NETEmcWSEvent
{
  DN_NETResponseState state;
  DN_Str8             payload;
  DN_NETEmcWSEvent   *next;
};

struct DN_NETEmcCore
{
  DN_Pool        pool;
  DN_NETRequest *response_list; // Responses received that are to be deqeued via wait for response
  DN_NETRequest *free_list;     // Request pool that new requests will use before allocating
};

struct DN_NETEmcRequest
{
  int               socket;
  DN_NETEmcWSEvent *first_event;
  DN_NETEmcWSEvent *last_event;
};

DN_NETInterface DN_NET_EmcInterface()
{
  DN_NETInterface result      = {};
  result.init                  = DN_NET_EmcInit;
  result.deinit                = DN_NET_EmcDeinit;
  result.do_http               = DN_NET_EmcDoHTTP;
  result.do_ws                 = DN_NET_EmcDoWS;
  result.do_ws_send            = DN_NET_EmcDoWSSend;
  result.wait_for_response     = DN_NET_EmcWaitForResponse;
  result.wait_for_any_response = DN_NET_EmcWaitForAnyResponse;
  return result;
}

static DN_NETEmcWSEvent *DN_NET_EmcAllocWSEvent_(DN_NETRequest *request)
{
  // NOTE: Allocate the event and attach to the request
  DN_NETEmcRequest *emc_request = DN_Cast(DN_NETEmcRequest *) request->context[1];
  DN_NETEmcWSEvent *result      = DN_ArenaNew(&request->arena, DN_NETEmcWSEvent, DN_ZMem_Yes);
  DN_Assert(result);
  if (result) {
    if (!emc_request->first_event)
      emc_request->first_event = result;
    if (emc_request->last_event)
      emc_request->last_event->next = result;
    emc_request->last_event = result;
  }
  return result;
}

static void DN_NET_EmcOnRequestDone_(DN_NETCore *net, DN_NETRequest *request)
{
  // NOTE: This may be call multiple times on the same request if we get multiple responses when we
  // yield to the javascript event loop, e.g. the application received multiple WS payloads before
  // it waited and consequently consumed the response from the payload.
  //
  // So if the next pointer is already set, then it should be that the request is already enqueued.
  DN_NETEmcCore *emc = DN_Cast(DN_NETEmcCore *) net->context;
  if (!request->next && !request->prev && request != emc->response_list) {
    request->prev = nullptr;
    request->next = emc->response_list;
    if (emc->response_list)
      emc->response_list->prev = request;
    emc->response_list = request;
  }
  DN_OS_SemaphoreIncrement(&net->completion_sem, 1);
  DN_OS_SemaphoreIncrement(&request->completion_sem, 1);
}

static bool DN_NET_EmcWSOnOpen(int eventType, EmscriptenWebSocketOpenEvent const *event, void *user_data)
{
  DN_NETRequest    *req       = DN_Cast(DN_NETRequest *) user_data;
  DN_NETCore       *net       = DN_Cast(DN_NETCore *) req->context[0];
  DN_NETEmcWSEvent *net_event = DN_NET_EmcAllocWSEvent_(req);
  net_event->state            = DN_NETResponseState_WSOpen;
  DN_NET_EmcOnRequestDone_(net, req);
  return true;
}

static bool DN_NET_EmcWSOnMessage(int eventType, const EmscriptenWebSocketMessageEvent *event, void *user_data)
{
  DN_NETRequest    *req       = DN_Cast(DN_NETRequest *) user_data;
  DN_NETCore       *net       = DN_Cast(DN_NETCore *) req->context[0];
  DN_NETEmcWSEvent *net_event = DN_NET_EmcAllocWSEvent_(req);
  net_event->state            = event->isText ? DN_NETResponseState_WSText : DN_NETResponseState_WSBinary;
  if (event->numBytes > 0)
    net_event->payload = DN_Str8FromPtrArena(&req->arena, event->data, event->numBytes);
  DN_NET_EmcOnRequestDone_(net, req);
  return true;
}

static bool DN_NET_EmcWSOnError(int eventType, EmscriptenWebSocketErrorEvent const *event, void *user_data)
{
  DN_NETRequest    *req       = DN_Cast(DN_NETRequest *) user_data;
  DN_NETCore       *net       = DN_Cast(DN_NETCore *) req->context[0];
  DN_NETEmcWSEvent *net_event = DN_NET_EmcAllocWSEvent_(req);
  net_event->state            = DN_NETResponseState_Error;
  DN_NET_EmcOnRequestDone_(net, req);
  return true;
}

static bool DN_NET_EmcWSOnClose(int eventType, EmscriptenWebSocketCloseEvent const *event, void *user_data)
{
  DN_NETRequest    *req       = DN_Cast(DN_NETRequest *) user_data;
  DN_NETCore       *net       = DN_Cast(DN_NETCore *) req->context[0];
  DN_NETEmcWSEvent *net_event = DN_NET_EmcAllocWSEvent_(req);
  net_event->state            = DN_NETResponseState_WSClose;
  net_event->payload          = DN_Str8FromFmtArena(&req->arena, "Websocket closed '%.*s': (%u) %s (was %s close)", DN_Str8PrintFmt(req->url), event->code, event->reason, event->wasClean ? "clean" : "unclean");
  DN_NET_EmcOnRequestDone_(net, req);
  return true;
}

static void DN_NET_EmcHTTPSuccessCallback(emscripten_fetch_t *fetch)
{
  DN_NETRequest *req        = DN_Cast(DN_NETRequest *) fetch->userData;
  DN_NETCore    *net        = DN_Cast(DN_NETCore *) req->context[0];
  req->response.http_status = fetch->status;
  req->response.state       = DN_NETResponseState_HTTP;
  req->response.body        = DN_Str8FromStr8Arena(&req->arena, DN_Str8FromPtr(fetch->data, fetch->numBytes - 1));
  DN_NET_EmcOnRequestDone_(net, req);
}

static void DN_NET_EmcHTTPFailCallback(emscripten_fetch_t *fetch)
{
  DN_NETRequest *req        = DN_Cast(DN_NETRequest *) fetch->userData;
  DN_NETCore    *net        = DN_Cast(DN_NETCore *) req->context[0];
  req->response.http_status = fetch->status;
  req->response.state       = DN_NETResponseState_Error;
  DN_NET_EmcOnRequestDone_(net, req);
}

static void DN_NET_EmcHTTPProgressCallback(emscripten_fetch_t *fetch)
{
}

void DN_NET_EmcInit(DN_NETCore *net, char *base, DN_U64 base_size)
{
  DN_NET_BaseInit_(net, base, base_size);
  DN_NETEmcCore *emc = DN_ArenaNew(&net->arena, DN_NETEmcCore, DN_ZMem_Yes);
  emc->pool          = DN_PoolFromArena(&net->arena, 0);
  net->context       = emc;
}

void DN_NET_EmcDeinit(DN_NETCore *net)
{
  (void)net;
  // TODO: Track all the request handles and clean it up
}

static DN_NETRequest *DN_NET_EmcAllocRequest_(DN_NETCore *net)
{
  // NOTE: Allocate request
  DN_NETEmcCore *emc = DN_Cast(DN_NETEmcCore *) net->context;
  DN_NETRequest *result = emc->free_list;
  if (result) {
    emc->free_list = emc->free_list->next;
    result->next      = nullptr;
    DN_Assert(result->prev == nullptr);
    if (emc->free_list) {
      DN_Assert(emc->free_list->prev == nullptr);
    }
  } else {
    // NOTE: Setup the request's arena here. WASM doesn't have the concept of virtual memory
    // so we use malloc to initialise it.
    result = DN_ArenaNew(&net->arena, DN_NETRequest, DN_ZMem_Yes);
    if (result)
      result->arena = DN_ArenaFromHeap(DN_Megabytes(1), DN_ArenaFlags_Nil);
  }

  // NOTE: Setup some emscripten specific data into our request context
  if (result) {
    result->context[0] = DN_Cast(DN_UPtr) net;
  }

  return result;
}

DN_NETRequestHandle DN_NET_EmcDoHTTP(DN_NETCore *net, DN_Str8 url, DN_Str8 method, DN_NETDoHTTPArgs const *args)
{
  DN_NETRequest      *req    = DN_NET_EmcAllocRequest_(net);
  DN_NETRequestHandle result = DN_NET_SetupRequest_(req, url, method, args, DN_NETRequestType_HTTP);

  // NOTE: Setup the HTTP request via Emscripten
  emscripten_fetch_attr_t fetch_attribs = {};
  {
    DN_Assert(req->args.payload.data[req->args.payload.size] == 0);
    DN_Assert(req->url.data[req->url.size] == 0);

    // NOTE: Setup request for emscripten
    emscripten_fetch_attr_init(&fetch_attribs);

    fetch_attribs.requestData     = req->args.payload.data;
    fetch_attribs.requestDataSize = req->args.payload.size;
    DN_Assert(req->method.size < DN_ArrayCountU(fetch_attribs.requestMethod));
    DN_Memcpy(fetch_attribs.requestMethod, req->method.data, req->method.size);
    fetch_attribs.requestMethod[req->method.size] = 0;

    // NOTE: Assign HTTP headers
    if (req->args.headers_size) {
      char **headers = DN_ArenaNewArray(&req->arena, char *, req->args.headers_size + 1, DN_ZMem_Yes);
      for (DN_ForItSize(it, DN_Str8, req->args.headers, req->args.headers_size)) {
        DN_Assert(it.data->data[it.data->size] == 0);
        headers[it.index] = it.data->data;
      }
      fetch_attribs.requestHeaders = headers;
    }

    // NOTE: Handle basic auth
    if (req->args.flags & DN_NETDoHTTPFlags_BasicAuth) {
      if (req->args.username.size && req->args.password.size) {
        DN_Assert(req->args.username.data[req->args.username.size] == 0);
        DN_Assert(req->args.password.data[req->args.password.size] == 0);
        fetch_attribs.withCredentials = true;
        fetch_attribs.userName        = req->args.username.data;
        fetch_attribs.password        = req->args.password.data;
      }
    }

    // NOTE: It would be nice to use EMSCRIPTEN_FETCH_STREAM_DATA however
    // emscripten has this note on the current version I'm using that this is
    // only supported in Firefox so this is a no-go.
    //
    //   > If passed, the intermediate streamed bytes will be passed in to the
    //   > onprogress() handler. If not specified, the onprogress() handler will still
    //   > be called, but without data bytes.  Note: Firefox only as it depends on
    //   > 'moz-chunked-arraybuffer'.
    fetch_attribs.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    fetch_attribs.onsuccess  = DN_NET_EmcHTTPSuccessCallback;
    fetch_attribs.onerror    = DN_NET_EmcHTTPFailCallback;
    fetch_attribs.onprogress = DN_NET_EmcHTTPProgressCallback;
    fetch_attribs.userData   = req;
  }

  // NOTE: Update the pop to position for the request
  req->start_response_arena_pos = DN_ArenaPos(&req->arena);

  // NOTE: Dispatch the asynchronous fetch
  emscripten_fetch(&fetch_attribs, req->url.data);
  return result;
}

DN_NETRequestHandle DN_NET_EmcDoWS(DN_NETCore *net, DN_Str8 url)
{
  DN_Assert(emscripten_websocket_is_supported());
  DN_NETRequest      *req    = DN_NET_EmcAllocRequest_(net);
  DN_NETRequestHandle result = DN_NET_SetupRequest_(req, url, /*method=*/DN_Str8Lit(""), /*args=*/nullptr, DN_NETRequestType_WS);
  if (!req)
    return result;

  // NOTE: Setup some emscripten specific data into our request context
  req->context[1]               = DN_Cast(DN_UPtr) DN_ArenaNew(&req->arena, DN_NETEmcRequest, DN_ZMem_Yes);
  req->start_response_arena_pos = DN_ArenaPos(&req->arena);

  // NOTE: Create the websocket request and dispatch it via emscripten
  EmscriptenWebSocketCreateAttributes attr;
  emscripten_websocket_init_create_attributes(&attr);
  attr.url = req->url.data;

  DN_NETEmcRequest *emc_request = DN_Cast(DN_NETEmcRequest *) req->context[1];
  emc_request->socket           = emscripten_websocket_new(&attr);
  DN_Assert(emc_request->socket > 0);
  emscripten_websocket_set_onopen_callback(emc_request->socket, /*userData=*/req, DN_NET_EmcWSOnOpen);
  emscripten_websocket_set_onmessage_callback(emc_request->socket, /*userData=*/req, DN_NET_EmcWSOnMessage);
  emscripten_websocket_set_onerror_callback(emc_request->socket, /*userData=*/req, DN_NET_EmcWSOnError);
  emscripten_websocket_set_onclose_callback(emc_request->socket, /*userData=*/req, DN_NET_EmcWSOnClose);

  return result;
}

void DN_NET_EmcDoWSSend(DN_NETRequestHandle handle, DN_Str8 data, DN_NETWSSend send)
{
  DN_AssertF(send == DN_NETWSSend_Binary || send == DN_NETWSSend_Text || send == DN_NETWSSend_Close,
             "Unimplemented, Emscripten only supports some of the available operations");

  int                    result      = 0;
  DN_NETRequest *request_ptr = DN_Cast(DN_NETRequest *) handle.handle;
  if (request_ptr && request_ptr->gen == handle.gen) {
    DN_Assert(request_ptr->type == DN_NETRequestType_WS);
    DN_NETEmcRequest *emc_request = DN_Cast(DN_NETEmcRequest *) request_ptr->context[1];
    switch (send) {
      default: DN_InvalidCodePath; break;
      case DN_NETWSSend_Text: {
        DN_U64  pos                  = DN_ArenaPos(&request_ptr->arena);
        DN_Str8 data_null_terminated = DN_Str8FromStr8Arena(&request_ptr->arena, data);
        result                       = emscripten_websocket_send_utf8_text(emc_request->socket, data_null_terminated.data);
        DN_ArenaPopTo(&request_ptr->arena, pos);
      } break;

      case DN_NETWSSend_Binary: {
        result = emscripten_websocket_send_binary(emc_request->socket, data.data, data.size);
      } break;

      case DN_NETWSSend_Close: {
        result = emscripten_websocket_close(emc_request->socket, 0, nullptr);
      } break;
    }
  }
  // TODO: Handle result, the header file doesn't really elucidate what this result value is
  (void)result;
}

static DN_NETResponse DN_NET_EmcHandleFinishedRequest_(DN_NETCore *net, DN_NETEmcCore *emc, DN_NETRequestHandle handle, DN_NETRequest *request, DN_Arena *arena)
{
  // NOTE: Generate the response, copy out the strings into the user given memory
  DN_NETEmcRequest *emc_request     = DN_Cast(DN_NETEmcRequest *) request->context[1];
  DN_NETResponse    result          = request->response;
  bool              end_request     = true;
  bool              dequeue_request = true;
  if (request->type == DN_NETRequestType_HTTP) {
    result.body = DN_Str8FromStr8Arena(arena, result.body);
  } else {
    // NOTE: Get emscripten contexts
    DN_NETEmcWSEvent *emc_event = emc_request->first_event;
    DN_Assert(emc_event);

    DN_AssertF((emc_event->state >= DN_NETResponseState_WSOpen && emc_event->state <= DN_NETResponseState_WSPong) || emc_event->state == DN_NETResponseState_Error,
               "emc_event=%p", emc_event);

    // NOTE: Build the result
    result.state   = emc_event->state;
    result.request = handle;
    result.body    = DN_Str8FromStr8Arena(arena, emc_event->payload);

    // NOTE: Advance the event list
    {
      if (emc_request->first_event == emc_request->last_event) {
        emc_request->last_event = emc_request->last_event->next;
        DN_Assert(emc_request->first_event->next == emc_request->last_event);
      }
      emc_request->first_event = emc_event->next;

      // NOTE: If there's still an event on the request then we do not dequeue the request from the
      // response list. The user can still "wait" for a response to read more data from it.
      if (emc_request->first_event)
        dequeue_request = false;
    }

    if (result.state != DN_NETResponseState_WSClose)
      end_request = false;
  }

  // NOTE: Remove request from the response list which is doubly-linked
  if (dequeue_request) {
    if (request->prev) {
      DN_AssertF(request->prev->next == request, "next=%p vs request=%p", request->prev->next, request);
      request->prev->next = request->next;
    }

    if (request->next) {
      DN_AssertF(request->next->prev == request, "prev=%p vs request=%p", request->next->prev, request);
      request->next->prev = request->prev;
    }

    if (request == emc->response_list)
      emc->response_list = emc->response_list->next;

    request->prev = nullptr;
    request->next = nullptr;
    DN_Assert(emc_request->first_event == nullptr);
    DN_Assert(emc_request->last_event == nullptr);

    // NOTE: Deallocate the memory used in the request and reset the string builder (as all
    // payload(s) have been read from the request).
    if (end_request)
        DN_ArenaPopTo(&request->arena, 0);
    else
        DN_ArenaPopTo(&request->arena, request->start_response_arena_pos);
  }

  if (end_request) {
    DN_NET_EndFinishedRequest_(request);
    emscripten_websocket_delete(emc_request->socket);
    emc_request->socket = 0;

    DN_NETEmcCore *emc = DN_Cast(DN_NETEmcCore *) net->context;
    request->next      = emc->free_list;
    request->prev      = nullptr;
    emc->free_list     = request;
  }

  return result;
}

static DN_OSSemaphoreWaitResult DN_NET_EmcSemaphoreWait_(DN_OSSemaphore *sem, DN_U32 timeout_ms)
{
  // NOTE: In emscripten you can't just block on the semaphore with 'timeout_ms' because it needs
  // to yield to the javascript's event loop otherwise the fetching step cannot progress. Instead
  // we use a timeout of 0 to just immediately check if the semaphore has been signalled, if not,
  // then we yield to the event loop by calling sleep.
  //
  // Once yielded, fetch will execute and eventually in the callback it will signal the semaphore
  // where it'll return and we can break out of the simulated "timeout".
  DN_OSSemaphoreWaitResult result               = {};
  DN_U32                   timeout_remaining_ms = timeout_ms;
  DN_F64                   begin_ms             = emscripten_get_now();
  for (;;) {
    result = DN_OS_SemaphoreWait(sem, 0);
    if (result == DN_OSSemaphoreWaitResult_Success)
      break;
    if (timeout_remaining_ms <= 0)
      break;

    emscripten_sleep(100 /*ms*/);
    DN_F64   end_ms      = emscripten_get_now();
    DN_USize duration_ms = DN_Cast(DN_USize)(end_ms - begin_ms);
    timeout_remaining_ms = timeout_remaining_ms >= duration_ms ? timeout_remaining_ms - duration_ms : 0;
    begin_ms             = end_ms;
  }
  return result;
}

DN_NETResponse DN_NET_EmcWaitForResponse(DN_NETRequestHandle handle, DN_Arena *arena, DN_U32 timeout_ms)
{
  DN_NETResponse result      = {};
  DN_NETRequest *request_ptr = DN_Cast(DN_NETRequest *) handle.handle;
  if (request_ptr && request_ptr->gen == handle.gen) {
    DN_NETCore    *net = DN_Cast(DN_NETCore *) request_ptr->context[0];
    DN_NETEmcCore *emc = DN_Cast(DN_NETEmcCore *) net->context;
    DN_Assert(emc);
    DN_OSSemaphoreWaitResult wait = DN_NET_EmcSemaphoreWait_(&request_ptr->completion_sem, timeout_ms);
    if (wait != DN_OSSemaphoreWaitResult_Success)
      return result;

    result = DN_NET_EmcHandleFinishedRequest_(net, emc, handle, request_ptr, arena);

    // NOTE: Decrement the global 'request done' completion semaphore since the user consumed the
    // request individually.
    DN_OSSemaphoreWaitResult net_wait_result = DN_OS_SemaphoreWait(&net->completion_sem, 0 /*timeout_ms*/);
    DN_AssertF(net_wait_result == DN_OSSemaphoreWaitResult_Success, "Wait result was: %zu", DN_Cast(DN_USize) net_wait_result);
  }
  return result;
}

DN_NETResponse DN_NET_EmcWaitForAnyResponse(DN_NETCore *net, DN_Arena *arena, DN_U32 timeout_ms)
{
  DN_NETEmcCore *emc = DN_Cast(DN_NETEmcCore *) net->context;
  DN_Assert(emc);

  DN_NETResponse          result = {};
  DN_OSSemaphoreWaitResult wait   = DN_NET_EmcSemaphoreWait_(&net->completion_sem, timeout_ms);
  if (wait != DN_OSSemaphoreWaitResult_Success)
      return result;

  DN_AssertF(emc->response_list,
             "This should be set otherwise we bumped the completion sem without queueing into the "
             "done list or we forgot to wait on the global semaphore after a request finished");

  // NOTE: Decrement the request's completion semaphore since the user consumed the global semaphore
  DN_NETRequest           *request_ptr     = emc->response_list;
  DN_OSSemaphoreWaitResult net_wait_result = DN_OS_SemaphoreWait(&request_ptr->completion_sem, 0 /*timeout_ms*/);
  DN_AssertF(net_wait_result == DN_OSSemaphoreWaitResult_Success, "Wait result was: %zu", DN_Cast(DN_USize) net_wait_result);

  DN_NETRequestHandle request = {};
  request.handle              = DN_Cast(DN_UPtr) request_ptr;
  request.gen                 = request_ptr->gen;
  result                      = DN_NET_EmcHandleFinishedRequest_(net, emc, request, request_ptr, arena);

  return result;
}

