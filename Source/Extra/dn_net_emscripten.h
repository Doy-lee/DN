#if !defined(DN_NET_EMSCRIPTEN_H)
#define DN_NET_EMSCRIPTEN_H

#include "dn_net2.h"

DN_NET2Interface DN_NET2_EmcInterface();
void             DN_NET2_EmcInit              (DN_NET2Core *net, char *base, DN_U64 base_size);
DN_NET2Request   DN_NET2_EmcDoHTTP            (DN_NET2Core *net, DN_Str8 url, DN_Str8 method, DN_NET2DoHTTPArgs const *args);
DN_NET2Request   DN_NET2_EmcDoWS              (DN_NET2Core *net, DN_Str8 url);
void             DN_NET2_EmcDoWSSend          (DN_NET2Request request, DN_Str8 data, DN_NET2WSSend send);
DN_NET2Response  DN_NET2_EmcWaitForResponse   (DN_NET2Request request, DN_Arena *arena, DN_U32 timeout_ms);
DN_NET2Response  DN_NET2_EmcWaitForAnyResponse(DN_NET2Core *net, DN_Arena *arena, DN_U32 timeout_ms);

#endif // DN_NET_EMSCRIPTEN_H
