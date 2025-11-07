#if !defined(DN_NET_CURL_H)
#define DN_NET_CURL_H

#include "dn_net2.h"

DN_NET2Interface DN_NET2_CurlInterface();
void             DN_NET2_CurlInit              (DN_NET2Core *net, char *base, DN_U64 base_size);
DN_NET2Request   DN_NET2_CurlDoHTTP            (DN_NET2Core *net, DN_Str8 url, DN_Str8 method, DN_NET2DoHTTPArgs const *args);
DN_NET2Request   DN_NET2_CurlDoWSArgs          (DN_NET2Core *net, DN_Str8 url, DN_NET2DoHTTPArgs const *args);
DN_NET2Request   DN_NET2_CurlDoWS              (DN_NET2Core *net, DN_Str8 url);
void             DN_NET2_CurlDoWSSend          (DN_NET2Request request, DN_Str8 payload, DN_NET2WSSend send);
DN_NET2Response  DN_NET2_CurlWaitForResponse   (DN_NET2Request request, DN_Arena *arena, DN_U32 timeout_ms);
DN_NET2Response  DN_NET2_CurlWaitForAnyResponse(DN_NET2Core *net, DN_Arena *arena, DN_U32 timeout_ms);

#endif // !defined(DN_NET_CURL_H)
