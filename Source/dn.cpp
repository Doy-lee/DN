#include "Base/dn_base.cpp"
#include "Base/dn_base_containers.cpp"
#include "Base/dn_base_leak.cpp"

#if DN_H_WITH_OS
#include "OS/dn_os.cpp"
#if defined(DN_PLATFORM_POSIX) || defined(DN_PLATFORM_EMSCRIPTEN)
  #include "OS/dn_os_posix.cpp"
#elif defined(DN_PLATFORM_WIN32)
  #include "OS/dn_os_w32.cpp"
#else
  #error Please define a platform e.g. 'DN_PLATFORM_WIN32' to enable the correct implementation for platform APIs
#endif
#endif

#if DN_H_WITH_CORE
#include "dn_core.cpp"
#endif

#if DN_H_WITH_MATH
#include "Extra/dn_math.cpp"
#endif

#if DN_H_WITH_HELPERS
#include "Extra/dn_helpers.cpp"
#endif

#if DN_H_WITH_ASYNC
#include "Extra/dn_async.cpp"
#endif

#if DN_H_WITH_NET
#include "Extra/dn_net.cpp"
#endif

#if DN_CPP_WITH_TESTS
#include "Extra/dn_tests.cpp"
#endif

#if DN_CPP_WITH_DEMO
#include "Extra/dn_demo.cpp"
#endif

