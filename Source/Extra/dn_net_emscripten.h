#if !defined(DN_NET_EMSCRIPTEN_H)
#define DN_NET_EMSCRIPTEN_H

#include "dn_net.h"

DN_NETInterface     DN_NET_EmcInterface();
void                DN_NET_EmcInit              (DN_NETCore *net, char *base, DN_U64 base_size);
void                DN_NET_EmcDeinit            (DN_NETCore *net);
DN_NETRequestHandle DN_NET_EmcDoHTTP            (DN_NETCore *net, DN_Str8 url, DN_Str8 method, DN_NETDoHTTPArgs const *args);
DN_NETRequestHandle DN_NET_EmcDoWS              (DN_NETCore *net, DN_Str8 url);
void                DN_NET_EmcDoWSSend          (DN_NETRequestHandle handle, DN_Str8 data, DN_NETWSSend send);
DN_NETResponse      DN_NET_EmcWaitForResponse   (DN_NETRequestHandle handle, DN_Arena *arena, DN_U32 timeout_ms);
DN_NETResponse      DN_NET_EmcWaitForAnyResponse(DN_NETCore *net, DN_Arena *arena, DN_U32 timeout_ms);

#endif // DN_NET_EMSCRIPTEN_H
