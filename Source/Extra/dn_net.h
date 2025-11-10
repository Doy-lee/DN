#if !defined(DN_NET_H)
#define DN_NET_H

#if defined(_CLANGD)
  #include "../dn_base_inc.h"
  #include "../dn_os_inc.h"
#endif

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

struct DN_NETRequestHandle
{
  DN_UPtr handle;
  DN_U64  gen;
};

struct DN_NETResponse
{
  // NOTE: Common to WS and HTTP responses
  DN_NETResponseState state;
  DN_NETRequestHandle request;
  DN_Str8             error_str8;
  DN_Str8             body;

  // NOTE: HTTP responses only
  DN_U32              http_status;
};

struct DN_NETRequest
{
  DN_Arena          arena;
  DN_USize          start_response_arena_pos;
  DN_NETRequestType type;
  DN_U64            gen;
  DN_Str8           url;
  DN_Str8           method;
  DN_OSSemaphore    completion_sem;
  DN_NETDoHTTPArgs  args;
  DN_NETResponse    response;
  DN_NETRequest    *next;
  DN_NETRequest    *prev;
  DN_U64            context[2];
};

struct DN_NETCore
{
  char          *base;
  DN_U64         base_size;
  DN_Arena       arena;
  DN_OSSemaphore completion_sem;
  void          *context;
};

typedef void               (DN_NETInitFunc)              (DN_NETCore *net, char *base, DN_U64 base_size);
typedef void               (DN_NETDeinitFunc)            (DN_NETCore *net);
typedef DN_NETRequestHandle(DN_NETDoHTTPFunc)            (DN_NETCore *net, DN_Str8 url, DN_Str8 method, DN_NETDoHTTPArgs const *args);
typedef DN_NETRequestHandle(DN_NETDoWSFunc)              (DN_NETCore *net, DN_Str8 url);
typedef void               (DN_NETDoWSSendFunc)          (DN_NETRequestHandle handle, DN_Str8 data, DN_NETWSSend send);
typedef DN_NETResponse     (DN_NETWaitForResponseFunc)   (DN_NETRequestHandle handle, DN_Arena *arena, DN_U32 timeout_ms);
typedef DN_NETResponse     (DN_NETWaitForAnyResponseFunc)(DN_NETCore *net, DN_Arena *arena, DN_U32 timeout_ms);

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

DN_Str8             DN_NET_Str8FromResponseState(DN_NETResponseState state);
DN_NETRequest *     DN_NET_RequestFromHandle    (DN_NETRequestHandle handle);
DN_NETRequestHandle DN_NET_HandleFromRequest    (DN_NETRequest *request);

// NOTE: Internal functions for different networking implementations to use
void                DN_NET_BaseInit_            (DN_NETCore *net, char *base, DN_U64 base_size);
DN_NETRequestHandle DN_NET_SetupRequest_        (DN_NETRequest *request, DN_Str8 url, DN_Str8 method, DN_NETDoHTTPArgs const *args, DN_NETRequestType type);
void                DN_NET_EndFinishedRequest_  (DN_NETRequest *request);

#endif // DN_NET_H
