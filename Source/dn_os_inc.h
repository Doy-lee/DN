#if !defined(DN_OS_INC_H)
#define DN_OS_INC_H

#if defined(DN_PLATFORM_WIN32)
  #include "OS/dn_os_windows.h"
  #include "OS/dn_os_w32.h"
#elif defined(DN_PLATFORM_POSIX) || defined(DN_PLATFORM_EMSCRIPTEN)
  #include "OS/dn_os_posix.h"
#else
  #error Please define a platform e.g. 'DN_PLATFORM_WIN32' to enable the correct implementation for platform APIs
#endif

#include "OS/dn_os_tls.h"
#include "OS/dn_os.h"
#include "OS/dn_os_allocator.h"
#include "OS/dn_os_containers.h"
#include "OS/dn_os_print.h"
#include "OS/dn_os_string.h"

#endif // DN_OS_INC_H
