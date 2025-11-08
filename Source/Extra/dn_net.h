#if !defined(DN_NET_H)
#define DN_NET_H

#include "../dn_base_inc.h"
#include "../dn_os_inc.h"

enum DN_NETRequestType
{
  DN_NETRequestType_Nil,
  DN_NETRequestType_HTTP,
  DN_NETRequestType_WS,
};

enum DN_NETResponseState
{
  DN_NETResponseState_Nil,
  DN_NETResponseState_Error,
  DN_NETResponseState_HTTP,
  DN_NETResponseState_WSOpen,
  DN_NETResponseState_WSText,
  DN_NETResponseState_WSBinary,
  DN_NETResponseState_WSClose,
  DN_NETResponseState_WSPing,
  DN_NETResponseState_WSPong,
};

enum DN_NETWSSend
{
  DN_NETWSSend_Text,
  DN_NETWSSend_Binary,
  DN_NETWSSend_Close,
  DN_NETWSSend_Ping,
  DN_NETWSSend_Pong,
};

enum DN_NETDoHTTPFlags
{
  DN_NETDoHTTPFlags_Nil       = 0,
  DN_NETDoHTTPFlags_BasicAuth = 1 << 0,
};

struct DN_NETDoHTTPArgs
{
  // NOTE: WS and HTTP args
  DN_NETDoHTTPFlags  flags;
  DN_Str8            username;
  DN_Str8            password;
  DN_Str8           *headers;
  DN_U16             headers_size;

  // NOTE: HTTP args only
  DN_Str8             payload;
};

struct DN_NETRequest
{
  DN_UPtr handle;
  DN_U64  gen;
};

struct DN_NETResponseInternal
{
  DN_NETResponseState state;
  bool                ws_has_more;
  DN_Str8Builder      body;
  DN_U32              http_status;
  DN_Str8             error_str8;
};

struct DN_NETResponse
{
  // NOTE: Common to WS and HTTP responses
  DN_NETResponseState state;
  DN_NETRequest       request;
  DN_Str8             error_str8;
  DN_Str8             body;

  // NOTE: HTTP responses only
  DN_U32              http_status;
};

struct DN_NETRequestInternal
{
  // NOTE: Initialised in user thread, then read-only and shared to networking thread until reset
  DN_Arena               arena;
  DN_USize               start_response_arena_pos;
  DN_NETRequestType      type;
  DN_U64                 gen;
  DN_Str8                url;
  DN_Str8                method;
  DN_OSSemaphore         completion_sem;
  DN_NETDoHTTPArgs       args;
  DN_NETResponseInternal response;
  DN_NETRequestInternal *next;
  DN_NETRequestInternal *prev;
  DN_U64                 context[2];
};

struct DN_NETCore
{
  char                  *base;
  DN_U64                 base_size;
  DN_Arena               arena;
  DN_OSSemaphore         completion_sem;
  void                  *context;
};

typedef void           (DN_NETInitFunc)              (DN_NETCore *net, char *base, DN_U64 base_size);
typedef void           (DN_NETDeinitFunc)            (DN_NETCore *net);
typedef DN_NETRequest  (DN_NETDoHTTPFunc)            (DN_NETCore *net, DN_Str8 url, DN_Str8 method, DN_NETDoHTTPArgs const *args);
typedef DN_NETRequest  (DN_NETDoWSFunc)              (DN_NETCore *net, DN_Str8 url);
typedef void           (DN_NETDoWSSendFunc)          (DN_NETRequest request, DN_Str8 data, DN_NETWSSend send);
typedef DN_NETResponse (DN_NETWaitForResponseFunc)   (DN_NETRequest request, DN_Arena *arena, DN_U32 timeout_ms);
typedef DN_NETResponse (DN_NETWaitForAnyResponseFunc)(DN_NETCore *net, DN_Arena *arena, DN_U32 timeout_ms);

struct DN_NETInterface
{
  DN_NETInitFunc*               init;
  DN_NETDeinitFunc*             deinit;
  DN_NETDoHTTPFunc*             do_http;
  DN_NETDoWSFunc*               do_ws;
  DN_NETDoWSSendFunc*           do_ws_send;
  DN_NETWaitForResponseFunc*    wait_for_response;
  DN_NETWaitForAnyResponseFunc* wait_for_any_response;
};

DN_NETRequestInternal *DN_NET_RequestFromHandle               (DN_NETRequest request);
DN_NETRequest          DN_NET_HandleFromRequest               (DN_NETRequestInternal *request);

// NOTE: Internal functions for different networking implementations to use
void                   DN_NET_BaseInit_                       (DN_NETCore *net, char *base, DN_U64 base_size);
DN_NETRequest          DN_NET_SetupRequest_                   (DN_NETRequestInternal *request, DN_Str8 url, DN_Str8 method, DN_NETDoHTTPArgs const *args, DN_NETRequestType type);
DN_NETResponse         DN_NET_MakeResponseFromFinishedRequest_(DN_NETRequest request, DN_Arena *arena);
void                   DN_NET_EndFinishedRequest_             (DN_NETRequestInternal *request);

#endif // DN_NET_H
