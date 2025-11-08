#define DN_NET_CURL_CPP

#include "../dn_base_inc.h"
#include "../dn_os_inc.h"

DN_NETRequestInternal *DN_NET_RequestFromHandle(DN_NETRequest request)
{
  DN_NETRequestInternal *ptr    = DN_Cast(DN_NETRequestInternal *) request.handle;
  DN_NETRequestInternal *result = nullptr;
  if (ptr && ptr->gen == request.gen)
    result = ptr;
  return result;
}

DN_NETRequest DN_NET_HandleFromRequest(DN_NETRequestInternal *request)
{
  DN_NETRequest result = {};
  if (request) {
    result.handle = DN_Cast(DN_UPtr) request;
    result.gen    = request->gen;
  }
  return result;
}

DN_NETResponse DN_NET_MakeResponseFromFinishedRequest_(DN_NETRequest request, DN_Arena *arena)
{
  DN_NETResponse result = {};
  DN_NETRequestInternal *request_ptr = DN_Cast(DN_NETRequestInternal *) request.handle;
  if (request_ptr && request_ptr->gen == request.gen) {
    DN_NETResponseInternal const *response = &request_ptr->response;

    // NOTE: Construct the response from the request
    result.request     = request;
    result.state       = response->state;
    result.http_status = response->http_status;
    result.body        = DN_Str8BuilderBuild(&response->body, arena);
    if (response->error_str8.size)
      result.error_str8 = DN_Str8FromStr8Arena(arena, response->error_str8);
  }
  return result;
}

void DN_NET_EndFinishedRequest_(DN_NETRequestInternal *request)
{
  // NOTE: Deallocate the memory used in the request and reset the string builder
  DN_ArenaPopTo(&request->arena, request->start_response_arena_pos);
  request->response.body = DN_Str8BuilderFromArena(&request->arena);

  // NOTE: Check that the request is completely detached
  DN_Assert(request->next == nullptr);
}

void DN_NET_BaseInit_(DN_NETCore *net, char *base, DN_U64 base_size)
{
  net->base           = base;
  net->base_size      = base_size;
  net->arena          = DN_ArenaFromBuffer(net->base, net->base_size, DN_ArenaFlags_Nil);
  net->completion_sem = DN_OS_SemaphoreInit(0);
}

DN_NETRequest DN_NET_SetupRequest_(DN_NETRequestInternal *request, DN_Str8 url, DN_Str8 method, DN_NETDoHTTPArgs const *args, DN_NETRequestType type)
{
  // NOTE: Setup request
  DN_Assert(request);
  DN_NETRequest result = {};
  if (request) {
    if (!request->arena.curr)
      request->arena = DN_ArenaFromVMem(DN_Megabytes(1), DN_Kilobytes(1), DN_ArenaFlags_Nil);
    request->type   = type;
    request->gen    = DN_Max(request->gen + 1, 1);
    request->url    = DN_Str8FromStr8Arena(&request->arena, url);
    request->method = DN_Str8FromStr8Arena(&request->arena, DN_Str8TrimWhitespaceAround(method));

    if (args) {
      request->args.flags    = args->flags;
      request->args.username = DN_Str8FromStr8Arena(&request->arena, args->username);
      request->args.password = DN_Str8FromStr8Arena(&request->arena, args->password);
      if (type == DN_NETRequestType_HTTP)
        request->args.payload = DN_Str8FromStr8Arena(&request->arena, args->payload);

      request->args.headers = DN_ArenaNewArray(&request->arena, DN_Str8, args->headers_size, DN_ZMem_No);
      DN_Assert(request->args.headers);
      if (request->args.headers) {
        for (DN_ForItSize(it, DN_Str8, args->headers, args->headers_size))
          request->args.headers[it.index] = DN_Str8FromStr8Arena(&request->arena, *it.data);
        request->args.headers_size = args->headers_size;
      }
    }

    request->response.body            = DN_Str8BuilderFromArena(&request->arena);
    request->completion_sem           = DN_OS_SemaphoreInit(0);
    request->start_response_arena_pos = DN_ArenaPos(&request->arena);

    result.handle = DN_Cast(DN_UPtr) request;
    result.gen    = request->gen;
  }

  return result;
}
