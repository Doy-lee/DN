#if !defined(DN_NET2_CURL_H)
#define DN_NET2_CURL_H

#include "../dn_base_inc.h"
#include "../dn_os_inc.h"

enum DN_NET2RequestType
{
  DN_NET2RequestType_Nil,
  DN_NET2RequestType_HTTP,
  DN_NET2RequestType_WS,
};

enum DN_NET2RequestStatus
{
  DN_NET2RequestStatus_Nil,
  DN_NET2RequestStatus_Error,
  DN_NET2RequestStatus_HTTPReceived,
  DN_NET2RequestStatus_WSReceived,
};

enum DN_NET2WSType
{
  DN_NET2WSType_Nil,
  DN_NET2WSType_Text,
  DN_NET2WSType_Binary,
  DN_NET2WSType_Close,
  DN_NET2WSType_Ping,
  DN_NET2WSType_Pong,
};

struct DN_NET2CurlConn
{
  void *curl;
  struct curl_slist *curl_slist;
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

struct DN_NET2ResponseInternal
{
  DN_NET2RequestStatus  status;
  DN_NET2WSType         ws_type;
  bool                  ws_has_more;
  DN_Str8Builder        body;
  DN_U32                http_status;
  DN_Str8               error;
};

struct DN_NET2Request
{
  DN_U64 handle;
  DN_U64 gen;
};

struct DN_NET2Response
{
  DN_NET2Request request;
  bool           success;
  DN_NET2WSType  ws_type;
  DN_U32         http_status;
  DN_Str8        error;
  DN_Str8        body;
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

  // NOTE: Networking thread only
  char                    context[sizeof(void *) * 2];
};

enum DN_NET2RingEventType
{
  DN_NET2RingEventType_Nil,
  DN_NET2RingEventType_DoHTTP,
  DN_NET2RingEventType_SendWS,
  DN_NET2RingEventType_ReceivedWSReceipt,
  DN_NET2RingEventType_DeinitRequest,
};

struct DN_NET2RingEvent
{
  DN_NET2RingEventType type;
  DN_NET2Request       request;
  DN_USize             send_ws_payload_size;
  DN_NET2WSType        send_ws_type;
};

struct DN_NET2Core
{
  // NOTE: User thread only
  char                   *base; // Backing memory for arena
  DN_U64                  base_size;
  DN_Arena                arena; // Allocates DN_NET2Request for appending to the user submission queue

  // NOTE: Shared w/ user and networking thread
  DN_Ring                 ring;
  DN_OSMutex              ring_mutex;

  DN_OSSemaphore          completion_sem;
  DN_OSMutex              free_or_done_mutex; // Lock for free list and done list
  DN_NET2RequestInternal *done_list;
  DN_NET2RequestInternal *free_list;

  DN_OSThread             curl_thread;

  // NOTE: Networking thread only
  void                   *curlm;
  DN_NET2RequestInternal *http_list;
  DN_NET2RequestInternal *ws_list;
};

static DN_NET2Response DN_NET2_WaitForAnyResponse(DN_NET2Core *net, DN_Arena *arena, DN_U32 timeout_ms);
static DN_NET2Response DN_NET2_WaitForResponse   (DN_NET2Core *net, DN_NET2Request request, DN_Arena *arena, DN_U32 timeout_ms);
static void            DN_NET2_DeinitRequest     (DN_NET2Core *net, DN_NET2Request *request);
static void            DN_NET2_Init              (DN_NET2Core *net, char *ring_base, DN_USize ring_size, char *base, DN_U64 base_size);
static DN_NET2Request  DN_NET2_DoHTTP            (DN_NET2Core *net, DN_Str8 url, DN_Str8 method, DN_NET2DoHTTPArgs const *args);
static DN_NET2Request  DN_NET2_OpenWS            (DN_NET2Core *net, DN_Str8 url, DN_NET2DoHTTPArgs const *args);
static void            DN_NET2_SendWS            (DN_NET2Core *net, DN_NET2Request request, DN_Str8 payload, DN_NET2WSType type);

#endif // DN_NET2_CURL_H
