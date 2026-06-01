#if defined(DN_UNIT_TESTS_WITH_CURL)
  #define DN_NO_WINDOWS_H_REPLACEMENT_HEADER
#endif

#define DN_ARENA_TEMP_MEM_UAF_GUARD               1
#define DN_ARENA_TEMP_MEM_UAF_TRACE_ON_BY_DEFAULT 0
#define DN_H_WITH_OS                              1
#define DN_H_WITH_CORE                            1
#define DN_H_WITH_HASH                            1
#define DN_H_WITH_HELPERS                         1
#define DN_H_WITH_ASYNC                           1
#define DN_H_WITH_NET                             1
#include "../dn.h"
#include "../dn.cpp"

#if defined(DN_UNIT_TESTS_WITH_CURL)
  #define CURL_STATICLIB
  #include <curl/curl.h>
  #include "../Extra/dn_net_curl.h"
  #include "../Extra/dn_net_curl.cpp"
#endif

#if defined(DN_PLATFORM_EMSCRIPTEN)
  #include <emscripten/emscripten.h>
  #include <emscripten/fetch.h>
  #include "../Extra/dn_net_emscripten.h"
  #include "../Extra/dn_net_emscripten.cpp"
#endif

#define DN_UT_IMPLEMENTATION
#include "../Standalone/dn_utest.h"
#include "../Extra/dn_tests.cpp"

DN_MSVC_WARNING_PUSH
DN_MSVC_WARNING_DISABLE(6262) // Function uses '29804' bytes of stack.  Consider moving some data to heap.
int main(int, char**)
{
  DN_Core       dn      = {};
  DN_Init(&dn, DN_InitFlags_LogAllFeatures | DN_InitFlags_OS | DN_InitFlags_ThreadContext, DN_TCInitArgsDefault());
  DN_TST_RunSuite(DN_TSTPrint_Yes);
  return 0;
}
DN_MSVC_WARNING_POP
