#if !defined(DN_NET_CURL_H)
#define DN_NET_CURL_H

#include "dn_net.h"

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
