#if !defined(DN_NET_CURL_H)
#define DN_NET_CURL_H

#include "dn_net.h"

struct DN_NETCurlCore
{
  // NOTE: Shared w/ user and networking thread
  DN_Ring        ring;
  DN_OSMutex     ring_mutex;
  bool           kill_thread;

  DN_OSMutex     list_mutex;          // Lock for request, response, deinit, free list
  DN_NETRequest *request_list;        // Current requests submitted by the user thread awaiting to move into the thread request list
  DN_NETRequest *response_list;       // Finished requests that are to be deqeued by the user via wait for response
  DN_NETRequest *deinit_list;         // Requests that are finished and are awaiting to be de-initialised by the CURL thread
  DN_NETRequest *free_list;           // Request pool that new requests will use before allocating

  // NOTE: Networking thread only
  DN_NETRequest *thread_request_list; // Current requests being executed by the CURL thread.
                                      // This list is exclusively owned by the CURL thread so no locking is needed
  DN_OSThread    thread;
  void          *thread_curlm;
};

#define             DN_NET_CurlCoreFromNet(net)  ((net) ? (DN_Cast(DN_NETCurlCore *)(net)->context) : nullptr);
DN_NETInterface     DN_NET_CurlInterface         ();
void                DN_NET_CurlInit              (DN_NETCore *net, char *base, DN_U64 base_size);
void                DN_NET_CurlDeinit            (DN_NETCore *net);
DN_NETRequestHandle DN_NET_CurlDoHTTP            (DN_NETCore *net, DN_Str8 url, DN_Str8 method, DN_NETDoHTTPArgs const *args);
DN_NETRequestHandle DN_NET_CurlDoWSArgs          (DN_NETCore *net, DN_Str8 url, DN_NETDoHTTPArgs const *args);
DN_NETRequestHandle DN_NET_CurlDoWS              (DN_NETCore *net, DN_Str8 url);
void                DN_NET_CurlDoWSSend          (DN_NETRequestHandle handle, DN_Str8 payload, DN_NETWSSend send);
DN_NETResponse      DN_NET_CurlWaitForResponse   (DN_NETRequestHandle handle, DN_Arena *arena, DN_U32 timeout_ms);
DN_NETResponse      DN_NET_CurlWaitForAnyResponse(DN_NETCore *net, DN_Arena *arena, DN_U32 timeout_ms);

#endif // !defined(DN_NET_CURL_H)
