#if !defined(DN_H)
#define DN_H

// NOTE: DN
//  Getting Started
//    Include this mega header `dn.h` and define the following symbols to `1` to conditionally
//    enable the interfaces for those features. Additionally in the same or different translation
//    unit, include `dn.cpp` with the same symbols defined to enable the implementation of these
//    features.
//
//    See the configuration section for more information on other symbols that can be defined.
//
//    The following is a single translation unit example:
/*
    #define DN_H_WITH_OS      1
    #define DN_H_WITH_CORE    1
    #define DN_H_WITH_HELPERS 1
    #define DN_H_WITH_ASYNC   1
    #define DN_H_WITH_NET     1
    #include "dn.h"

    #define DN_CPP_WITH_TESTS 1
    #define DN_CPP_WITH_DEMO 1
    #include "dn.cpp"
*/
//    Then initialise the library at runtime by calling DN_Init(...). The library is laid out as:
//
//    - The base layer (dn_base.h) which provides primitives that do not require a host operating
//      system (e.g. freestanding) such as string manipulation, compiler intrinsics and containers.
//      This layer is unconditionallly always available by compiling with this library.
//
//    - The OS layer (dn_os.h) which provides primitives that use the OS such as file IO, threading
//      synchronisation, memory allocation. This layer is OPTIONAL.
//
//    - Extra layer provides helper utilities that are opt-in. These layers are OPTIONAL.
//
//  Configuration
//    Platform Target
//      Define one of the following directives to configure this library to compile for that
//      platform. By default, the library will auto-detect the current host platform and select that
//      as the target platform.
//
//        DN_PLATFORM_EMSCRIPTEN
//        DN_PLATFORM_POSIX
//        DN_PLATFORM_WIN32
//
//      For example
//
//        #define DN_PLATFORM_WIN32
//
//      Will ensure that <Windows.h> is included and the OS layer is implemented using Win32
//      primitives.
//
//    Static functions
//      All public functions in the DN library are prefixed with the macro '#define DN_API'. By
//      default 'DN_API' is not defined to anything. Define
//
//        DN_STATIC_API
//
//      To replace all the functions prefixed with DN_API to be prefixed with 'static' ensuring that
//      the functions in the library do not export an entry into the linking table.
//      translation units.
//
//    Disabling the in-built <Windows.h> (if #define DN_H_WITH_OS 1)
//      If you are building DN for the Windows platform, <Windows.h> is a large legacy header that
//      applications have to include to use Windows APIs. By default this library uses a replacement
//      header for all the Windows functions that it uses in the OS layer removing the need to
//      include <Windows.h> to improve compilation times. This mini header will conflict with
//      <Windows.h> if it needs to be included in your project. The mini header can be disabled by
//      defining:
//
//        DN_NO_WINDOWS_H_REPLACEMENT_HEADER
//
//      To instead use <Windows.h>. DN automatically detects if <Windows.h> is included in an earlier
//      translation unit and will automatically disable the in-built replacement header in which case
//      this does not need to be defined.
//
//    Freestanding
//      The base layer can be used without an OS implementation by defining DN_FREESTANDING like:
//
//        #define DN_FREESTANDING
//
//      This means functionality that relies on the OS like printing, memory allocation, stack traces
//      and so forth are disabled.
//
//    ASAN Arena Poisoning
//      When compiled with address sanitizer (.e.g -fsanitize=address) you can optionally enable
//      memory region poisoning on the inbuilt arena's to catch in certain scenarios, use-after-free
//
//        #define DN_ASAN_POISON 1
//
//      Since arenas manage their own block of memory it does not automatically benefit from ASAN's
//      memory markup that ASAN does and so it is implemented manually by using the ASAN user-level
//      poisoning APIs. Similarly, since the arena recycles its own memory rather than release back
//      to the OS, poisoning is not as effective for arenas but every little bit helps.
//
//    Scrub Uninitialised Memory
//      If this macro is defined, temp memory that is returned to an arena, or allocations freed by
//      a pool are scrubbed to this specified byte, in absence of this bytes returned to the
//      allocators are left as-is or memset to 0. For example to scrub bytes to 0xCD (MSVC's
//      pattern) define as follows:
//
//        #define DN_SCRUB_UNINIT_MEM_BYTE 0xCD
//
//      Due to the recycling of memory in arenas and pool, similarly to ASAN poisoning this reduces
//      the window in which a use-after-free can be detected using this guard, however every little
//      bit helps.
//
//    Arena temp memory use-after-free (UAF) tooling
//      UAF Guard
//        Set the following preprocessor value to 1 to enable UAF protection when using
//        scratch/temporary memory functionality. Defaults to off, or 0 if not specified
//
//          #define DN_ARENA_TEMP_MEM_UAF_GUARD 1
//
//        This enables arenas to markup itself with the active memory region and subsequent
//        allocations check if the allocation belongs to the same or different region. Different
//        regions cause a UAF violation.
//
//        More detailed diagnostics can be enabled by setting the flag DN_ArenaFlags_TempMemUAFGuard
//        on the affected arenas. Note that this incurs a performance penalty as each memory region
//        will store a stacktrace of its creation. It's recommended in development builds to always
//        run with temp-memory guarding, if a violation occurs, then enable tracing on the arena to
//        pinpoint the issue.
//
//        Enabling memory guard incurs additional memory requirements from the arena's backing
//        memory block and additional book-keeping fields on each arena and their temp memory
//        instances.
//
//      UAF Tracing
//        Set the following preprocessor value to 1 to enable tracing when the UAF guard triggers.
//        Defaults to off, or 0 if not specified.
//
//          #define DN_ARENA_TEMP_MEM_UAF_TRACE_ON_BY_DEFAULT 1
//
//        This opts in all arenas to tracing functionality by default globally. Arenas can opt out
//        by setting the flag DN_ArenaFlags_TempMemUAFTraceDisable on the arena. The disable flag
//        takes precedence over all other settings including the global preprocessor macro and the
//        enablement flag (DN_ArenaFlags_TempMemUAFTrace).
//
//        Tracing incurs an additional much heavier performance penalty than the UAF guard due to
//        the stacktrace that is stored per region to report to the user when a UAF guard violation
//        occurs.

#include "Base/dn_base.h"
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

typedef DN_USize DN_InitFlags;
enum DN_InitFlags_
{
  DN_InitFlags_Nil            = 0,
  DN_InitFlags_OS             = (1 << 0),
  DN_InitFlags_ThreadContext  = (1 << 1) | DN_InitFlags_OS,
  DN_InitFlags_LeakTracker    = (1 << 2) | DN_InitFlags_OS,
  DN_InitFlags_LogLibFeatures = (1 << 3),
  DN_InitFlags_LogCPUFeatures = (1 << 4) | DN_InitFlags_OS,
  DN_InitFlags_LogAllFeatures = DN_InitFlags_LogLibFeatures | DN_InitFlags_LogCPUFeatures,
};

struct DN_Core
{
  DN_InitFlags     init_flags;
  DN_TCCore        main_tc;
  DN_USize         mem_allocs_frame;
  DN_LeakTracker   leak;

  DN_U32           log_level_to_show_from;
  DN_LogPrintFunc* print_func;
  void*            print_func_context;

  bool             os_init;
  #if defined(DN_OS_H)
  DN_OSCore        os;
  #endif
};

DN_API void     DN_Init      (DN_Core *dn, DN_InitFlags flags, DN_TCInitArgs args);
DN_API void     DN_Set       (DN_Core *dn);
DN_API DN_Core *DN_Get       ();
DN_API void     DN_BeginFrame();

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
