#if DN_WITH_NET_CURL
  #define DN_NO_WINDOWS_H_REPLACEMENT_HEADER
#endif

#define DN_PARANOIA_LEVEL 1
#define DN_WITH_OS        1
#define DN_WITH_NET       1
#define DN_WITH_TESTS     1
#include "dn.h"

#if DN_WITH_NET_CURL
  #define CURL_STATICLIB
  #include <curl/curl.h>
#endif

#if defined(DN_PLATFORM_EMSCRIPTEN)
  #define DN_WITH_NET_EMSCRIPTEN 1
#endif
#include "dn.cpp"

#define DN_SHA3_WITH_TESTS
#define DN_SHA3_IMPLEMENTATION
#include "Standalone/dn_sha3.h"

DN_MSVC_WARNING_PUSH
DN_MSVC_WARNING_DISABLE(6262) // Function uses '29804' bytes of stack.  Consider moving some data to heap.
int main(int, char**)
{
  DN_Core dn = {};
  DN_Init(&dn, DN_InitFlags_LogAllFeatures | DN_InitFlags_OS, DN_TCInitArgsDefault());

  DN_Arena*   arena     = DN_TCMainArena();
  DN_TestCore dn_test   = DN_TestSuite(arena);
  DN_TestCore sha3_test = DN_SHA3_TestSuite(arena);
  DN_Str8     dn_str8   = DN_Str8FromTestCore(&dn_test, arena, DN_Str8FromTestCoreFlags_Colour);
  DN_Str8     sha3_str8 = DN_Str8FromTestCore(&sha3_test, arena, DN_Str8FromTestCoreFlags_Colour);
  DN_OS_PrintOutF("%.*s\n%.*s", DN_Str8PrintFmt(dn_str8), DN_Str8PrintFmt(sha3_str8));
  return 0;
}
DN_MSVC_WARNING_POP
