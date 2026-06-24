#if DN_WITH_NET_CURL
  #define DN_NO_WINDOWS_H_REPLACEMENT_HEADER
#endif

#define DN_ARENA_TEMP_MEM_UAF_GUARD               1
#define DN_ARENA_TEMP_MEM_UAF_TRACE_ON_BY_DEFAULT 0
#define DN_WITH_OS                                1
#define DN_WITH_NET                               1
#if defined(DN_PLATFORM_EMSCRIPTEN)
  #define DN_WITH_NET_EMSCRIPTEN 1
#endif
#include "../dn.h"

#if DN_WITH_NET_CURL
  #define CURL_STATICLIB
  #include <curl/curl.h>
#endif
#include "../dn.cpp"

#define DN_UT_IMPLEMENTATION
#include "../Standalone/dn_utest.h"
#include "../Extra/dn_tests.cpp"

DN_MSVC_WARNING_PUSH
DN_MSVC_WARNING_DISABLE(6262) // Function uses '29804' bytes of stack.  Consider moving some data to heap.
int main(int, char**)
{
  DN_Core dn = {};
  DN_Init(&dn, DN_InitFlags_LogAllFeatures | DN_InitFlags_OS, DN_TCInitArgsDefault());
  DN_TST_RunSuite(DN_TSTPrint_Yes);
  return 0;
}
DN_MSVC_WARNING_POP
