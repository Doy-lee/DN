#if !defined(DN_NET2_H)
#define DN_NET2_H

#include "../dn_base_inc.h"
#include "../dn_os_inc.h"

enum DN_NET2RequestType
{
  DN_NET2RequestType_Nil,
  DN_NET2RequestType_HTTP,
  DN_NET2RequestType_WS,
};

enum DN_NET2ResponseState
{
  DN_NET2ResponseState_Nil,
  DN_NET2ResponseState_Error,
  DN_NET2ResponseState_HTTP,
  DN_NET2ResponseState_WSOpen,
  DN_NET2ResponseState_WSText,
  DN_NET2ResponseState_WSBinary,
  DN_NET2ResponseState_WSClose,
  DN_NET2ResponseState_WSPing,
  DN_NET2ResponseState_WSPong,
};

enum DN_NET2WSSend
{
  DN_NET2WSSend_Text,
  DN_NET2WSSend_Binary,
  DN_NET2WSSend_Close,
  DN_NET2WSSend_Ping,
  DN_NET2WSSend_Pong,
};

enum DN_NET2DoHTTPFlags
{
  DN_NET2DoHTTPFlags_Nil       = 0,
  DN_NET2DoHTTPFlags_BasicAuth = 1 << 0,
};

struct DN_NET2DoHTTPArgs
{
  // NOTE: WS and HTTP args
  DN_NET2DoHTTPFlags  flags;
  DN_Str8             username;
  DN_Str8             password;
  DN_Str8            *headers;
  DN_U16              headers_size;

  // NOTE: HTTP args only
  DN_Str8             payload;
};

struct DN_NET2Request
{
  DN_UPtr handle;
  DN_U64  gen;
};

struct DN_NET2ResponseInternal
{
  DN_NET2ResponseState state;
  bool                 ws_has_more;
  DN_Str8Builder       body;
  DN_U32               http_status;
  DN_Str8              error_str8;
};

struct DN_NET2Response
{
  // NOTE: Common to WS and HTTP responses
  DN_NET2ResponseState state;
  DN_NET2Request       request;
  DN_Str8              error_str8;
  DN_Str8              body;

  // NOTE: HTTP responses only
  DN_U32               http_status;
};

struct DN_NET2RequestInternal
{
  // NOTE: Initialised in user thread, then read-only and shared to networking thread until reset
  DN_Arena                arena;
  DN_USize                start_response_arena_pos;
  DN_NET2RequestType      type;
  DN_U64                  gen;
  DN_Str8                 url;
  DN_Str8                 method;
  DN_OSSemaphore          completion_sem;
  DN_NET2DoHTTPArgs       args;
  DN_NET2ResponseInternal response;
  DN_NET2RequestInternal *next;
  DN_NET2RequestInternal *prev;
  DN_U64                  context[2];
};

struct DN_NET2Core
{
  char                   *base;
  DN_U64                  base_size;
  DN_Arena                arena;
  DN_OSSemaphore          completion_sem;
  DN_NET2RequestInternal *done_list;
  DN_NET2RequestInternal *free_list;
  void                   *context;
};

typedef void            (DN_NET2InitFunc)              (DN_NET2Core *net, char *base, DN_U64 base_size);
typedef DN_NET2Request  (DN_NET2DoHTTPFunc)            (DN_NET2Core *net, DN_Str8 url, DN_Str8 method, DN_NET2DoHTTPArgs const *args);
typedef DN_NET2Request  (DN_NET2DoWSFunc)              (DN_NET2Core *net, DN_Str8 url);
typedef void            (DN_NET2DoWSSendFunc)          (DN_NET2Request request, DN_Str8 data, DN_NET2WSSend send);
typedef DN_NET2Response (DN_NET2WaitForResponseFunc)   (DN_NET2Request request, DN_Arena *arena, DN_U32 timeout_ms);
typedef DN_NET2Response (DN_NET2WaitForAnyResponseFunc)(DN_NET2Core *net, DN_Arena *arena, DN_U32 timeout_ms);

struct DN_NET2Interface
{
  DN_NET2InitFunc*               init;
  DN_NET2DoHTTPFunc*             do_http;
  DN_NET2DoWSFunc*               do_ws;
  DN_NET2DoWSSendFunc*           do_ws_send;
  DN_NET2WaitForResponseFunc*    wait_for_response;
  DN_NET2WaitForAnyResponseFunc* wait_for_any_response;
};

// NOTE: Internal functions for different networking implementations to use
void            DN_NET2_BaseInit_                       (DN_NET2Core *net, char *base, DN_U64 base_size);
DN_NET2Request  DN_NET2_SetupRequest_                   (DN_NET2RequestInternal *request, DN_Str8 url, DN_Str8 method, DN_NET2DoHTTPArgs const *args, DN_NET2RequestType type);
DN_NET2Response DN_NET2_MakeResponseFromFinishedRequest_(DN_NET2Request request, DN_Arena *arena);
void            DN_NET2_EndFinishedRequest_             (DN_NET2Core *net, DN_NET2RequestInternal *request);

#endif // DN_NET2_H
