#include <emscripten.h>
#include <emscripten/fetch.h>
#include <emscripten/websocket.h>

#include "dn_net2.h"
#include "dn_net_emscripten.h"

struct DN_NET2EmcWSEvent
{
  DN_NET2ResponseState state;
  DN_Str8              payload;
  DN_NET2EmcWSEvent   *next;
};

struct DN_NET2EmcCore
{
  DN_Pool pool;
};

struct DN_NET2EmcRequest
{
  int                socket;
  DN_NET2EmcWSEvent *first_event;
  DN_NET2EmcWSEvent *last_event;
};

DN_NET2Interface DN_NET2_EmcInterface()
{
  DN_NET2Interface result      = {};
  result.init                  = DN_NET2_EmcInit;
  result.do_http               = DN_NET2_EmcDoHTTP;
  result.do_ws                 = DN_NET2_EmcDoWS;
  result.do_ws_send            = DN_NET2_EmcDoWSSend;
  result.wait_for_response     = DN_NET2_EmcWaitForResponse;
  result.wait_for_any_response = DN_NET2_EmcWaitForAnyResponse;
  return result;
}

static DN_NET2EmcWSEvent *DN_NET2_EmcAllocWSEvent_(DN_NET2RequestInternal *request)
{
  // NOTE: Allocate the event and attach to the request
  DN_NET2EmcRequest *emc_request = DN_Cast(DN_NET2EmcRequest *) request->context[1];
  DN_NET2EmcWSEvent *result      = DN_ArenaNew(&request->arena, DN_NET2EmcWSEvent, DN_ZMem_Yes);
  if (result) {
    if (!emc_request->first_event)
      emc_request->first_event = result;
    if (emc_request->last_event)
      emc_request->last_event->next = result;
    emc_request->last_event = result;
  }
  return result;
}

static void DN_NET2_EmcOnRequestDone_(DN_NET2Core *net, DN_NET2RequestInternal *request)
{
  // NOTE: This may be call multiple times if we get multiple responses when we yield to the javascript event loop
  if (!request->next) {
    request->next  = net->done_list;
    net->done_list = request;
  }
  DN_OS_SemaphoreIncrement(&net->completion_sem, 1);
  DN_OS_SemaphoreIncrement(&request->completion_sem, 1);
}

// TODO: Need to enqueue the results since they can accumulate when you yield to the javascript event loop
static bool DN_NET2_EmcWSOnOpen(int eventType, EmscriptenWebSocketOpenEvent const *event, void *user_data)
{
  DN_NET2RequestInternal *request   = DN_Cast(DN_NET2RequestInternal *) user_data;
  DN_NET2Core            *net       = DN_Cast(DN_NET2Core *) request->context[0];
  DN_NET2EmcCore         *emc       = DN_Cast(DN_NET2EmcCore *) net->context;
  DN_NET2EmcWSEvent      *net_event = DN_NET2_EmcAllocWSEvent_(request);
  net_event->state                  = DN_NET2ResponseState_WSOpen;
  DN_NET2_EmcOnRequestDone_(net, request);
  return true;
}

static bool DN_NET2_EmcWSOnMessage(int eventType, const EmscriptenWebSocketMessageEvent *event, void *user_data)
{
  DN_NET2RequestInternal *request   = DN_Cast(DN_NET2RequestInternal *) user_data;
  DN_NET2Core            *net       = DN_Cast(DN_NET2Core *) request->context[0];
  DN_NET2EmcWSEvent      *net_event = DN_NET2_EmcAllocWSEvent_(request);
  net_event->state                   = event->isText ? DN_NET2ResponseState_WSText : DN_NET2ResponseState_WSBinary;
  if (event->numBytes > 0) {
    DN_NET2EmcCore *emc = DN_Cast(DN_NET2EmcCore *) net->context;
    net_event->payload  = DN_Str8FromPtrPool(&emc->pool, event->data, event->numBytes);
  }
  DN_NET2_EmcOnRequestDone_(net, request);
  return true;
}

static bool DN_NET2_EmcWSOnError(int eventType, EmscriptenWebSocketErrorEvent const *event, void *user_data)
{
  DN_NET2RequestInternal *request   = DN_Cast(DN_NET2RequestInternal *) user_data;
  DN_NET2Core            *net       = DN_Cast(DN_NET2Core *) request->context[0];
  DN_NET2EmcCore         *emc       = DN_Cast(DN_NET2EmcCore *) net->context;
  DN_NET2EmcWSEvent      *net_event = DN_NET2_EmcAllocWSEvent_(request);
  net_event->state                   = DN_NET2ResponseState_Error;
  DN_NET2_EmcOnRequestDone_(net, request);
  return true;
}

static bool DN_NET2_EmcWSOnClose(int eventType, EmscriptenWebSocketCloseEvent const *event, void *user_data)
{
  DN_NET2RequestInternal *request   = DN_Cast(DN_NET2RequestInternal *) user_data;
  DN_NET2Core            *net       = DN_Cast(DN_NET2Core *) request->context[0];
  DN_NET2EmcCore         *emc       = DN_Cast(DN_NET2EmcCore *) net->context;
  DN_NET2EmcWSEvent      *net_event = DN_NET2_EmcAllocWSEvent_(request);
  net_event->state                  = DN_NET2ResponseState_WSClose;
  net_event->payload                = DN_Str8FromFmtPool(&emc->pool, "Websocket closed '%.*s': (%u) %s (was %s close)", DN_Str8PrintFmt(request->url), event->code, event->reason, event->wasClean ? "clean" : "unclean");
  DN_NET2_EmcOnRequestDone_(net, request);
  return true;
}

static void DN_NET2_EmcHTTPSuccessCallback(emscripten_fetch_t *fetch)
{
  DN_NET2RequestInternal *request = DN_Cast(DN_NET2RequestInternal *) fetch->userData;
  DN_NET2Core            *net     = DN_Cast(DN_NET2Core *) request->context[0];
  request->response.http_status   = fetch->status;
  request->response.state         = DN_NET2ResponseState_HTTP;
  DN_Str8BuilderAppendCopy(&request->response.body, DN_Str8FromPtr(fetch->data, fetch->numBytes - 1));
  DN_NET2_EmcOnRequestDone_(net, request);
}

static void DN_NET2_EmcHTTPFailCallback(emscripten_fetch_t *fetch)
{
  DN_NET2RequestInternal *request = DN_Cast(DN_NET2RequestInternal *) fetch->userData;
  DN_NET2Core            *net     = DN_Cast(DN_NET2Core *) request->context[0];

  request->response.http_status = fetch->status;
  request->response.state       = DN_NET2ResponseState_Error;
  DN_NET2_EmcOnRequestDone_(net, request);
}

static void DN_NET2_EmcHTTPProgressCallback(emscripten_fetch_t *fetch)
{
}

void DN_NET2_EmcInit(DN_NET2Core *net, char *base, DN_U64 base_size)
{
  DN_NET2_BaseInit_(net, base, base_size);
  DN_NET2EmcCore *emc = DN_ArenaNew(&net->arena, DN_NET2EmcCore, DN_ZMem_Yes);
  emc->pool           = DN_PoolFromArena(&net->arena, 0);
  net->context        = emc;
}

DN_NET2Request DN_NET2_EmcDoHTTP(DN_NET2Core *net, DN_Str8 url, DN_Str8 method, DN_NET2DoHTTPArgs const *args)
{
  // NOTE: Allocate request
  DN_NET2RequestInternal *request = net->free_list;
  if (request) {
    net->free_list = net->free_list->next;
    request->next  = nullptr;
  } else {
    request = DN_ArenaNew(&net->arena, DN_NET2RequestInternal, DN_ZMem_Yes);
  }

  DN_NET2Request result = DN_NET2_SetupRequest_(request, url, method, args, DN_NET2RequestType_HTTP);

  // NOTE: Setup some emscripten specific data into our request context
  request->context[0] = DN_Cast(DN_UPtr) net;

  // NOTE: Setup the HTTP request via Emscripten
  emscripten_fetch_attr_t fetch_attribs = {};
  {
    DN_Assert(request->args.payload.data[request->args.payload.size] == 0);
    DN_Assert(request->url.data[request->url.size] == 0);

    // NOTE: Setup request for emscripten
    emscripten_fetch_attr_init(&fetch_attribs);

    fetch_attribs.requestData     = request->args.payload.data;
    fetch_attribs.requestDataSize = request->args.payload.size;
    DN_Assert(request->method.size < DN_ArrayCountU(fetch_attribs.requestMethod));
    DN_Memcpy(fetch_attribs.requestMethod, request->method.data, request->method.size);
    fetch_attribs.requestMethod[request->method.size] = 0;

    // NOTE: Assign HTTP headers
    if (request->args.headers_size) {
      char **headers = DN_ArenaNewArray(&request->arena, char *, request->args.headers_size + 1, DN_ZMem_Yes);
      for (DN_ForItSize(it, DN_Str8, request->args.headers, request->args.headers_size)) {
        DN_Assert(it.data->data[it.data->size] == 0);
        headers[it.index] = it.data->data;
      }
      fetch_attribs.requestHeaders = headers;
    }

    // NOTE: Handle basic auth
    if (request->args.flags & DN_NET2DoHTTPFlags_BasicAuth) {
      if (request->args.username.size && request->args.password.size) {
        DN_Assert(request->args.username.data[request->args.username.size] == 0);
        DN_Assert(request->args.password.data[request->args.password.size] == 0);
        fetch_attribs.withCredentials = true;
        fetch_attribs.userName        = request->args.username.data;
        fetch_attribs.password        = request->args.password.data;
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
    fetch_attribs.onsuccess  = DN_NET2_EmcHTTPSuccessCallback;
    fetch_attribs.onerror    = DN_NET2_EmcHTTPFailCallback;
    fetch_attribs.onprogress = DN_NET2_EmcHTTPProgressCallback;
    fetch_attribs.userData   = request;
  }

  // NOTE: Update the pop to position for the request
  request->start_response_arena_pos = DN_ArenaPos(&request->arena);

  // NOTE: Dispatch the asynchronous fetch
  emscripten_fetch(&fetch_attribs, request->url.data);
  return result;
}

DN_NET2Request DN_NET2_EmcDoWS(DN_NET2Core *net, DN_Str8 url)
{
  DN_Assert(emscripten_websocket_is_supported());

  // NOTE: Allocate request
  DN_NET2RequestInternal *request = net->free_list;
  if (request) {
    net->free_list = net->free_list->next;
    request->next  = nullptr;
  } else {
    request = DN_ArenaNew(&net->arena, DN_NET2RequestInternal, DN_ZMem_Yes);
  }

  DN_NET2Request result = DN_NET2_SetupRequest_(request, url, /*method=*/ DN_Str8Lit(""), /*args=*/nullptr, DN_NET2RequestType_WS);
  if (!request)
    return result;

  // NOTE: Setup some emscripten specific data into our request context
  DN_NET2EmcCore *emc               = DN_Cast(DN_NET2EmcCore *) net->context;
  request->context[0]               = DN_Cast(DN_UPtr) net;
  request->context[1]               = DN_Cast(DN_UPtr) DN_ArenaNew(&request->arena, DN_NET2EmcRequest, DN_ZMem_Yes);
  request->start_response_arena_pos = DN_ArenaPos(&request->arena);

  // NOTE: Create the websocket request and dispatch it via emscripten
  EmscriptenWebSocketCreateAttributes attr;
  emscripten_websocket_init_create_attributes(&attr);
  attr.url = request->url.data;

  DN_NET2EmcRequest *emc_request = DN_Cast(DN_NET2EmcRequest *) request->context[1];
  emc_request->socket            = emscripten_websocket_new(&attr);
  DN_Assert(emc_request->socket > 0);
  emscripten_websocket_set_onopen_callback(emc_request->socket,    /*userData=*/ request, DN_NET2_EmcWSOnOpen);
  emscripten_websocket_set_onmessage_callback(emc_request->socket, /*userData=*/ request, DN_NET2_EmcWSOnMessage);
  emscripten_websocket_set_onerror_callback(emc_request->socket,   /*userData=*/ request, DN_NET2_EmcWSOnError);
  emscripten_websocket_set_onclose_callback(emc_request->socket,   /*userData=*/ request, DN_NET2_EmcWSOnClose);

  return result;
}

void DN_NET2_EmcDoWSSend(DN_NET2Request request, DN_Str8 data, DN_NET2WSSend send)
{
  DN_AssertF(send == DN_NET2WSSend_Binary || send == DN_NET2WSSend_Text || send == DN_NET2WSSend_Close,
             "Unimplemented, Emscripten only supports some of the available operations");

  bool                    result      = false;
  DN_NET2RequestInternal *request_ptr = DN_Cast(DN_NET2RequestInternal *) request.handle;
  if (request_ptr && request_ptr->gen == request.gen) {
    DN_Assert(request_ptr->type == DN_NET2RequestType_WS);
    DN_NET2EmcRequest *emc_request = DN_Cast(DN_NET2EmcRequest *) request_ptr->context[1];
    switch (send) {
      default: DN_InvalidCodePath; break;
      case DN_NET2WSSend_Text: {
        DN_U64  pos                  = DN_ArenaPos(&request_ptr->arena);
        DN_Str8 data_null_terminated = DN_Str8FromStr8Arena(&request_ptr->arena, data);
        result                       = emscripten_websocket_send_utf8_text(emc_request->socket, data_null_terminated.data);
        DN_ArenaPopTo(&request_ptr->arena, pos);
      } break;

      case DN_NET2WSSend_Binary: {
        result = emscripten_websocket_send_binary(emc_request->socket, data.data, data.size);
      } break;

      case DN_NET2WSSend_Close: {
        result = emscripten_websocket_close(emc_request->socket, 0, nullptr);
      } break;
    }
  }
  // TODO: Handle result
}

static DN_NET2Response DN_NET2_EmcHandleFinishedRequest_(DN_NET2Core *net, DN_NET2EmcCore *emc, DN_NET2Request request, DN_NET2RequestInternal *request_ptr, DN_Arena *arena)
{
  DN_NET2Response result      = {};
  bool            end_request = true;
  if (request_ptr->type == DN_NET2RequestType_HTTP) {
    result = DN_NET2_MakeResponseFromFinishedRequest_(request, arena);
  } else {
    // NOTE: Get emscripten contexts
    DN_NET2EmcRequest *emc_request = DN_Cast(DN_NET2EmcRequest *) request_ptr->context[1];
    DN_NET2EmcWSEvent *emc_event   = emc_request->first_event;
    emc_request->first_event       = emc_event->next; // Advance the list pointer
    DN_Assert(emc_event);
    DN_Assert((emc_event->state >= DN_NET2ResponseState_WSOpen && emc_event->state <= DN_NET2ResponseState_WSPong) ||
              emc_event->state == DN_NET2ResponseState_Error);

    // NOTE: Build the result
    result.state   = emc_event->state;
    result.request = request;
    result.body    = DN_Str8FromStr8Arena(arena, emc_event->payload);


    // NOTE: Free the payload which is stored in the Emscripten pool
    if (emc_event->payload.size) {
      DN_PoolDealloc(&emc->pool, emc_event->payload.data);
      emc_event->payload = {};
    }

    if (result.state != DN_NET2ResponseState_WSClose)
      end_request = false;
  }

  // NOTE: Deallocate the memory used in the request and reset the string builder
  DN_ArenaPopTo(&request_ptr->arena, 0);
  request_ptr->response.body = DN_Str8BuilderFromArena(&request_ptr->arena);

  if (end_request) {
    DN_NET2_EndFinishedRequest_(net, request_ptr);
    DN_NET2EmcCore    *emc         = DN_Cast(DN_NET2EmcCore *) net->context;
    DN_NET2EmcRequest *emc_request = DN_Cast(DN_NET2EmcRequest *) request_ptr->context[1];
    emscripten_websocket_delete(emc_request->socket);
    request_ptr->next = net->free_list;
    net->free_list    = request_ptr;
  }

  return result;
}

static DN_OSSemaphoreWaitResult DN_NET2_EmcSemaphoreWait_(DN_OSSemaphore *sem, DN_U32 timeout_ms)
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

DN_NET2Response DN_NET2_EmcWaitForResponse(DN_NET2Request request, DN_Arena *arena, DN_U32 timeout_ms)
{
  DN_NET2Response         result      = {};
  DN_NET2RequestInternal *request_ptr = DN_Cast(DN_NET2RequestInternal *) request.handle;
  if (request_ptr && request_ptr->gen == request.gen) {
    DN_NET2Core    *net = DN_Cast(DN_NET2Core *) request_ptr->context[0];
    DN_NET2EmcCore *emc = DN_Cast(DN_NET2EmcCore *) net->context;
    DN_Assert(emc);
    DN_OSSemaphoreWaitResult wait = DN_NET2_EmcSemaphoreWait_(&request_ptr->completion_sem, timeout_ms);
    if (wait != DN_OSSemaphoreWaitResult_Success)
      return result;

    // NOTE: Remove request from the done list
    request_ptr->next = nullptr;
    net->done_list    = net->done_list->next;
    result            = DN_NET2_EmcHandleFinishedRequest_(net, emc, request, request_ptr, arena);

    // NOTE: Decrement the global 'request done' completion semaphore since the user consumed the
    // request individually.
    DN_OSSemaphoreWaitResult net_wait_result = DN_OS_SemaphoreWait(&net->completion_sem, 0 /*timeout_ms*/);
    DN_AssertF(net_wait_result == DN_OSSemaphoreWaitResult_Success, "Wait result was: %zu", DN_Cast(DN_USize) net_wait_result);
  }
  return result;
}

DN_NET2Response DN_NET2_EmcWaitForAnyResponse(DN_NET2Core *net, DN_Arena *arena, DN_U32 timeout_ms)
{
  DN_NET2EmcCore *emc = DN_Cast(DN_NET2EmcCore *) net->context;
  DN_Assert(emc);

  DN_NET2Response          result = {};
  DN_OSSemaphoreWaitResult wait   = DN_NET2_EmcSemaphoreWait_(&net->completion_sem, timeout_ms);
  if (wait != DN_OSSemaphoreWaitResult_Success)
      return result;

  // NOTE: Dequeue the request that is done from the done list
  DN_AssertF(net->done_list,
             "This should be set otherwise we bumped the completion sem without queueing into the "
             "done list or we forgot to wait on the global semaphore after a request finished");
  DN_NET2RequestInternal *request_ptr = net->done_list;
  DN_Assert(request_ptr == net->done_list);
  request_ptr->next = nullptr;
  net->done_list    = net->done_list->next;

  // NOTE: Decrement the request's completion semaphore since the user consumed the global semaphore
  DN_OSSemaphoreWaitResult net_wait_result = DN_OS_SemaphoreWait(&request_ptr->completion_sem, 0 /*timeout_ms*/);
  DN_AssertF(net_wait_result == DN_OSSemaphoreWaitResult_Success, "Wait result was: %zu", DN_Cast(DN_USize) net_wait_result);

  DN_NET2Request request = {};
  request.handle         = DN_Cast(DN_UPtr) request_ptr;
  request.gen            = request_ptr->gen;
  result                 = DN_NET2_EmcHandleFinishedRequest_(net, emc, request, request_ptr, arena);

  return result;
}

