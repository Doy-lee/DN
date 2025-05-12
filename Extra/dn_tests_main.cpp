#include "../dn_base_inc.h"
#include "../dn_os_inc.h"
#include "../dn_core_inc.h"

#include "../dn_base_inc.cpp"
#include "../dn_os_inc.cpp"
#include "../dn_core_inc.cpp"

#include "../Extra/dn_math.h"
#include "../Extra/dn_helpers.h"

#include "../Extra/dn_math.cpp"
#include "../Extra/dn_helpers.cpp"

#define DN_UT_IMPLEMENTATION
#include "../Standalone/dn_utest.h"
#include "../Extra/dn_tests.cpp"

DN_MSVC_WARNING_PUSH
DN_MSVC_WARNING_DISABLE(6262) // Function uses '29804' bytes of stack.  Consider moving some data to heap.
int main(int, char**)
{
  DN_Core core = {};
  DN_OSCore os = {};

  DN_OS_Init(&os, nullptr);
  DN_Core_Init(&core, DN_CoreOnInit_LogAllFeatures);
  DN_Tests_RunSuite(DN_TestsPrint_Yes);
  return 0;
}
DN_MSVC_WARNING_POP
