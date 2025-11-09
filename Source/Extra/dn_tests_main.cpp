#if defined(DN_UNIT_TESTS_WITH_CURL)
  #define DN_NO_WINDOWS_H_REPLACEMENT_HEADER
#endif

#include "../dn_base_inc.h"
#include "../dn_os_inc.h"
#include "../dn_inc.h"

#include "../dn_base_inc.cpp"
#include "../dn_os_inc.cpp"
#include "../dn_inc.cpp"

#include "../Extra/dn_math.h"
#include "../Extra/dn_helpers.h"

#include "../Extra/dn_math.cpp"
#include "../Extra/dn_helpers.cpp"

#include "../Extra/dn_net.h"
#include "../Extra/dn_net.cpp"

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
  DN_Core core = {};
  DN_Init(&core, DN_InitFlags_LogAllFeatures, nullptr);
  DN_Tests_RunSuite(DN_TestsPrint_Yes);
  return 0;
}
DN_MSVC_WARNING_POP
