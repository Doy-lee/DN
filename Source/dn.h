#if !defined(DN_H)
#define DN_H

// NOTE: DN configuration
//  Enabling DN modules
//    By including this mega header 'dn.h' you must define the following symbols to 0 or 1 in order
//    to include the module's implementation into the library as follows
/*
    #define DN_H_WITH_OS      1
    #define DN_H_WITH_CORE    1
    #define DN_H_WITH_MATH    1
    #define DN_H_WITH_HELPERS 1
    #define DN_H_WITH_ASYNC   1
    #define DN_H_WITH_NET     1
    #include "dn.h"

    #define DN_CPP_WITH_TESTS 1
    #define DN_CPP_WITH_DEMO 1
    #include "dn.cpp"
*/
//  Platform Target
//    Define one of the following directives to configure this library to compile for that platform.
//    By default, the library will auto-detect the current host platform and select that as the
//    target platform.
//
//      DN_PLATFORM_EMSCRIPTEN
//      DN_PLATFORM_POSIX
//      DN_PLATFORM_WIN32
//
//    For example
//
//      #define DN_PLATFORM_WIN32
//
//    Will ensure that <Windows.h> is included and the OS layer is implemented using Win32
//    primitives.
//
//  Static functions
//    All public functions in the DN library are prefixed with the macro '#define DN_API'. By
//    default 'DN_API' is not defined to anything. Define
//
//      DN_STATIC_API
//
//    To replace all the prefixed with 'static' ensuring that the functions in the library do not
//    export an entry into the linking table and thereby optimise compilation times as the linker
//    will not try to resolve symbols from other translation units from the the unit including the
//    DN library.
//
//  Using the in-built replacement header for <Windows.h> (requires dn_os_inc.h)
//    If you are building DN for the Windows platform, <Windows.h> is a large legacy header that
//    applications have to link to use Windows syscalls. By default DN library uses a replacement
//    header for all the Windows functions that it uses in the OS layer removing the need to include
//    <Windows.h> to improve compilation times. This can be disabled by defining:
//
//      DN_NO_WINDOWS_H_REPLACEMENT_HEADER
//
//    To instead use <Windows.h>. DN automatically detects if <Windows.h> is included in an earlier
//    translation unit and will automatically disable the in-built replacement header in which case
//    this does not need to be defined.
//
//  Freestanding
//    The base layer can be used without an OS implementation by defining DN_FREESTANDING like:
//
//      #define DN_FREESTANDING
//
//    This means functionality that relies on the OS like printing, memory allocation, stack traces
//    and so forth are disabled.

#include "Base/dn_base.h"
#include "Base/dn_base_assert.h"
#include "Base/dn_base_containers.h"
#include "Base/dn_base_leak.h"

#if DN_H_WITH_OS
#if defined(DN_PLATFORM_WIN32)
  #include "OS/dn_os_windows.h"
  #include "OS/dn_os_w32.h"
#elif defined(DN_PLATFORM_POSIX) || defined(DN_PLATFORM_EMSCRIPTEN)
  #include "OS/dn_os_posix.h"
#else
  #error Please define a platform e.g. 'DN_PLATFORM_WIN32' to enable the correct implementation for platform APIs
#endif

#include "OS/dn_os.h"
#endif

#if DN_H_WITH_CORE
#include "dn_core.h"
#endif

#if DN_H_WITH_MATH
#include "Extra/dn_math.h"
#endif

#if DN_H_WITH_HELPERS
#include "Extra/dn_helpers.h"
#endif

#if DN_H_WITH_ASYNC
#include "Extra/dn_async.h"
#endif

#if DN_H_WITH_NET
#include "Extra/dn_net.h"
#endif
#endif // !defined(DN_H)
