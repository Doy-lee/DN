#define DN_NET_CURL_CPP

#include "../dn_base_inc.h"
#include "../dn_os_inc.h"

DN_NETRequest *DN_NET_RequestFromHandle(DN_NETRequestHandle handle)
{
  DN_NETRequest *ptr    = DN_Cast(DN_NETRequest *) handle.handle;
  DN_NETRequest *result = nullptr;
  if (ptr && ptr->gen == handle.gen)
    result = ptr;
  return result;
}

DN_NETRequestHandle DN_NET_HandleFromRequest(DN_NETRequest *request)
{
  DN_NETRequestHandle result = {};
  if (request) {
    result.handle = DN_Cast(DN_UPtr) request;
    result.gen    = request->gen;
  }
  return result;
}

void DN_NET_EndFinishedRequest_(DN_NETRequest *request)
{
  // NOTE: Deallocate the memory used in the request and reset the string builder
  DN_ArenaPopTo(&request->arena, request->start_response_arena_pos);
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

DN_NETRequestHandle DN_NET_SetupRequest_(DN_NETRequest *request, DN_Str8 url, DN_Str8 method, DN_NETDoHTTPArgs const *args, DN_NETRequestType type)
{
  // NOTE: Setup request
  DN_Assert(request);
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

    request->completion_sem           = DN_OS_SemaphoreInit(0);
    request->start_response_arena_pos = DN_ArenaPos(&request->arena);
  }

  DN_NETRequestHandle result = DN_NET_HandleFromRequest(request);
  request->response.request  = result;
  return result;
}
