#if !defined(DN_BASE_INC_H)
#define DN_BASE_INC_H

// NOTE: DN configuration
//  All the following configuration options are optional. If omitted, DN has default behaviours to
//  handle the various options.
//
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

#include "Base/dn_base_compiler.h"
#include "Base/dn_base.h"
#include "Base/dn_base_os.h"
#include "Base/dn_base_assert.h"
#include "Base/dn_base_mem.h"
#include "Base/dn_base_log.h"
#include "Base/dn_base_string.h"
#include "Base/dn_base_containers.h"
#include "Base/dn_base_convert.h"

#endif // !defined(DN_BASE_INC_H)
