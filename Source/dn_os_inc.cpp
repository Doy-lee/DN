#define DN_OS_INC_CPP

#include "OS/dn_os_tls.cpp"
#include "OS/dn_os.cpp"
#include "OS/dn_os_allocator.cpp"
#include "OS/dn_os_containers.cpp"
#include "OS/dn_os_print.cpp"
#include "OS/dn_os_string.cpp"

#if defined(DN_PLATFORM_POSIX) || defined(DN_PLATFORM_EMSCRIPTEN)
  #include "OS/dn_os_posix.cpp"
#elif defined(DN_PLATFORM_WIN32)
  #include "OS/dn_os_w32.cpp"
#else
  #error Please define a platform e.g. 'DN_PLATFORM_WIN32' to enable the correct implementation for platform APIs
#endif

#include "OS/dn_os_stacktrace.cpp"
