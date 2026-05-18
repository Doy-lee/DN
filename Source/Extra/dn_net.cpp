#define DN_NET_CURL_CPP

#if defined(_CLANGD)
  #define DN_H_WITH_OS 1
  #include "../dn.h"
  #include "dn_net.h"
#endif

DN_Str8 DN_NET_Str8FromResponseState(DN_NETResponseState state)
{
  DN_Str8 result = {};
  switch (state) {
      case DN_NETResponseState_Nil:      result = DN_Str8Lit("Nil");       break;
      case DN_NETResponseState_Error:    result = DN_Str8Lit("Error");     break;
      case DN_NETResponseState_HTTP:     result = DN_Str8Lit("HTTP");      break;
      case DN_NETResponseState_WSOpen:   result = DN_Str8Lit("WS Open");   break;
      case DN_NETResponseState_WSText:   result = DN_Str8Lit("WS Text");   break;
      case DN_NETResponseState_WSBinary: result = DN_Str8Lit("WS Binary"); break;
      case DN_NETResponseState_WSClose:  result = DN_Str8Lit("WS Close");  break;
      case DN_NETResponseState_WSPing:   result = DN_Str8Lit("WS Ping");   break;
      case DN_NETResponseState_WSPong:   result = DN_Str8Lit("WS Pong");   break;
  }
  return result;
}

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
  DN_ArenaTempEnd(&request->start_response_arena, DN_ArenaReset_Yes);
  // NOTE: Check that the request is completely detached
  DN_Assert(request->next == nullptr);
}

void DN_NET_BaseInit_(DN_NETCore *net, char *base, DN_U64 base_size)
{
  net->base           = base;
  net->base_size      = base_size;
  net->mem            = DN_MemListFromBuffer(net->base, net->base_size, DN_MemFlags_Nil);
  net->arena          = DN_ArenaFromMemList(&net->mem);
  net->completion_sem = DN_OS_SemaphoreInit(0);
}

DN_NETRequestHandle DN_NET_SetupRequest_(DN_NETRequest *request, DN_Str8 url, DN_Str8 method, DN_NETDoHTTPArgs const *args, DN_NETRequestType type)
{
  // NOTE: Setup request
  DN_Assert(request);
  if (request) {
    if (!request->mem.curr)
      request->mem = DN_MemListFromVMem(DN_Megabytes(1), DN_Kilobytes(1), DN_MemFlags_Nil);
    request->arena  = DN_ArenaTempBeginFromMemList(&request->mem);
    request->type   = type;
    request->gen    = DN_Max(request->gen + 1, 1);
    request->url    = DN_Str8FromStr8Arena(url, &request->arena);
    request->method = DN_Str8FromStr8Arena(DN_Str8TrimWhitespaceAround(method), &request->arena);

    if (args) {
      request->args.flags    = args->flags;
      request->args.username = DN_Str8FromStr8Arena(args->username, &request->arena);
      request->args.password = DN_Str8FromStr8Arena(args->password, &request->arena);
      if (type == DN_NETRequestType_HTTP)
        request->args.payload = DN_Str8FromStr8Arena(args->payload, &request->arena);

      request->args.headers = DN_ArenaNewArray(&request->arena, DN_Str8, args->headers_size, DN_ZMem_No);
      DN_Assert(request->args.headers);
      if (request->args.headers) {
        for (DN_ForItSize(it, DN_Str8, args->headers, args->headers_size))
          request->args.headers[it.index] = DN_Str8FromStr8Arena(*it.data, &request->arena);
        request->args.headers_size = args->headers_size;
      }
    }

    request->completion_sem       = DN_OS_SemaphoreInit(0);
    request->start_response_arena = DN_ArenaTempBeginFromArena(&request->arena);
  }

  DN_NETRequestHandle result = DN_NET_HandleFromRequest(request);
  request->response.request  = result;
  return result;
}
