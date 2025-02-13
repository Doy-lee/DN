#pragma once
#define DN_H

/*
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   $$$$$$$\   $$$$$$\  $$\   $$\
//   $$  __$$\ $$  __$$\ $$$\  $$ |
//   $$ |  $$ |$$ /  $$ |$$$$\ $$ |
//   $$ |  $$ |$$ |  $$ |$$ $$\$$ |
//   $$ |  $$ |$$ |  $$ |$$ \$$$$ |
//   $$ |  $$ |$$ $$\$$ |$$ |\$$$ |
//   $$$$$$$  |\$$$$$$ / $$ | \$$ |
//   \_______/  \___$$$\ \__|  \__|
//                  \___|
//
//   dqn.h -- Personal standard library -- MIT License -- git.doylet.dev/dn
//   ASCII -- BigMoney-NW by Nathan Bloomfild
//
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// This library is a single-header file-esque library with inspiration taken
// from STB libraries for ease of integration and use. It defines a bunch of
// primitives and standard library functions that are missing and or more
// appropriate for development in modern day computing (e.g. allocator
// first-class APIs, a 64bit MMU and in general non-pessimized APIs that aren't
// constrained by the language specification and operate closer to the OS).
//
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//    $$$$$$\  $$$$$$$$\ $$$$$$$$\ $$$$$$$$\ $$$$$$\ $$\   $$\  $$$$$$\
//   $$  __$$\ $$  _____|\__$$  __|\__$$  __|\_$$  _|$$$\  $$ |$$  __$$\
//   $$ /  \__|$$ |         $$ |      $$ |     $$ |  $$$$\ $$ |$$ /  \__|
//   $$ |$$$$\ $$$$$\       $$ |      $$ |     $$ |  $$ $$\$$ |$$ |$$$$\
//   $$ |\_$$ |$$  __|      $$ |      $$ |     $$ |  $$ \$$$$ |$$ |\_$$ |
//   $$ |  $$ |$$ |         $$ |      $$ |     $$ |  $$ |\$$$ |$$ |  $$ |
//   \$$$$$$  |$$$$$$$$\    $$ |      $$ |   $$$$$$\ $$ | \$$ |\$$$$$$  |
//    \______/ \________|   \__|      \__|   \______|\__|  \__| \______/
//
//   Getting started -- Compiling with the library and library documentation
//
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// -- Compiling --
//
// Compile dn.cpp or include it into one of your translation units.
//
// Additionally, this library supports including/excluding specific sections
// of the library by using #define on the name of the section. These names are
// documented in the section table of contents at the #define column, for
// example:
//
//     #define DN_ONLY_VARRAY
//     #define DN_ONLY_WIN
//
// Compiles the library with all optional APIs turned off except virtual arrays
// and the Win32 helpers. Alternatively:
//
//     #define DN_NO_VARRAY
//     #define DN_NO_WIN
//
// Compiles the library with all optional APIs turned on except the previously
// mentioned APIs.
//
// -- Library documentation --
//
// The header files in this library are intentionally extremely minimal and
// concise as to provide a dense reference of the APIs available without
// drowning out the library interface with code comments like many other
// documentation systems.
//
// Instead, documentation is laid out in dn_docs.cpp in alphabetical order
// which provides self-contained examples in one contiguous top-down block of
// source code with comments.
//
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//    $$$$$$\  $$$$$$$\ $$$$$$$$\ $$$$$$\  $$$$$$\  $$\   $$\  $$$$$$\
//   $$  __$$\ $$  __$$\\__$$  __|\_$$  _|$$  __$$\ $$$\  $$ |$$  __$$\
//   $$ /  $$ |$$ |  $$ |  $$ |     $$ |  $$ /  $$ |$$$$\ $$ |$$ /  \__|
//   $$ |  $$ |$$$$$$$  |  $$ |     $$ |  $$ |  $$ |$$ $$\$$ |\$$$$$$\
//   $$ |  $$ |$$  ____/   $$ |     $$ |  $$ |  $$ |$$ \$$$$ | \____$$\
//   $$ |  $$ |$$ |        $$ |     $$ |  $$ |  $$ |$$ |\$$$ |$$\   $$ |
//    $$$$$$  |$$ |        $$ |   $$$$$$\  $$$$$$  |$$ | \$$ |\$$$$$$  |
//    \______/ \__|        \__|   \______| \______/ \__|  \__| \______/
//
//   Options -- Compile time build customisation
//
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// - Override these routines from the CRT by redefining them. By default we wrap
//   the CRT functions from <strings.h> and <math.h>, e.g:
//
//     #define DN_MEMCPY(dest, src, count)   memcpy(dest, src, value)
//     #define DN_MEMSET(dest, value, count) memset(dest, value, count)
//     #define DN_MEMCMP(lhs, rhs, count)    memcpy(lhs, rhs, count)
//     #define DN_MEMMOVE(dest, src, count)  memmove(dest, src, count)
//     #define DN_SQRTF(val)                 sqrtf(val)
//     #define DN_SINF(val)                  sinf(val)
//     #define DN_COSF(val)                  cosf(val)
//     #define DN_TANF(val)                  tanf(val)
//
// - Redefine 'DN_API' to change the prefix of all declared functions in the library
//
//     #define DN_API
//
// - Define 'DN_STATIC_API' to apply 'static' to all function definitions and
//   disable external linkage to other translation units by redefining 'DN_API' to
//   'static'.
//
//     #define DN_STATIC_API
//
// - Turn all assertion macros to no-ops except for hard asserts (which are
//   always enabled and represent unrecoverable errors in the library).
//
//     #define DN_NO_ASSERT
//
// - Augment DN_CHECK(expr) macro's behaviour. By default when the check fails a
//   debug break is emitted. If this macro is defined, the check will not trigger
//   a debug break.
//
//     #define DN_NO_CHECK_BREAK
//
// - Define this macro to enable memory leak tracking on arena's that are
//   configured to track allocations.
//
//   Allocations are stored in a global hash-table and their respective stack
//   traces for the allocation location. Memory leaks can be dumped at the end
//   of the program or some epoch by calling DN_Debug_DumpLeaks()
//
//     #define DN_LEAK_TRACKING
//
// - Define this to revert to the family of printf functions from <stdio.h>
//   instead of using stb_sprintf in this library. stb_sprintf is 5-6x faster
//   than printf with a smaller binary footprint and has deterministic behaviour
//   across all supported platforms.
//
//     #define DN_USE_STD_PRINTF
//
//   However, if you are compiling with ASAN on MSVC, MSVC's implementation of
//   __declspec(no_sanitize_address) is unable to suppress warnings in some
//   individual functions in stb's implementation causing ASAN to trigger. This
//   library will error on compilation if it detects this is the case and is
//   being compiled with STB sprintf.
//
// - Define this to stop this library from defining a minimal subset of Win32
//   prototypes and definitions in this file. You should use this macro if you
//   intend to #include <Windows.h> yourself to avoid symbol conflicts with
//   the redefined declarations in this library.
//
//     #define DN_NO_WIN32_MIN_HEADER
//
// - Define this to stop this library from defining STB_SPRINTF_IMPLEMENTATION.
//   Useful if another library uses and includes "stb_sprintf.h"
//
//     #define DN_STB_SPRINTF_HEADER_ONLY
//
// - Override the default break into the active debugger function. By default
//   we use __debugbreak() on Windows and raise(SIGTRAP) on other platforms.
//
//     #define DN_DEBUG_BREAK
//
// - Define this macro to 1 to enable poisoning of memory from arenas when ASAN
//   `-fsanitize=address` is enabled. Enabling this will detect memory overwrite
//   by padding allocated before and after with poisoned memory which will raise
//   a use-after-poison in ASAN on read/write. This is a no-op if the library is
//   not compiled with ASAN.
//
//     #define DN_ASAN_POISON 1
//
// - Define this macro 1 to enable sanity checks for manually poisoned memory in
//   this library when ASAN `-fsanitize=address` is enabled. These sanity checks
//   ensure that memory from arenas are correctly un/poisoned when pointers are
//   allocated and returned to the memory arena's. This is a no-op if we are not
//   compiled with ASAN or `DN_ASAN_POISON` is not set to `1`.
//
//     #define DN_ASAN_VET_POISON 1
//
// - Define this macro to the size of the guard memory reserved before and after
//   allocations made that are poisoned to protect against out-of-bounds memory
//   accesses. By default the library sets the guard to 128 bytes.
//
//     #define DN_ASAN_POISON_GUARD_SIZE 128
//
// - Enable 'DN_CGen' a parser that can emit run-time type information and
//   allow arbitrary querying of data definitions expressed in Excel-like tables
//   using text files encoded in Dion-System's Metadesk grammar.
//
//   This option automatically includes 'dn_cpp_file.h' to assist with code
//   generation and Metadesk's 'md.h' and its implementation library.
//
//     #define DN_WITH_CGEN
//
//   Optionally define 'DN_NO_METADESK' to disable the inclusion of Metadesk
//   in the library. This might be useful if you are including the librarin in
//   your  project yourself. This library must still be defined and visible
//   before this header.
//
// - Enable 'DN_JSON' a json parser. This option requires Sheredom's 'json.h'
//   to be included prior to this file.
//
//     #define DN_WITH_JSON
//
//   Optionally define 'DN_NO_SHEREDOM_JSON' to prevent Sheredom's 'json.h'
//   library from being included. This might be useful if you are including the
//   library in your project yourself. The library must still be defined and
//   visible before this header.
//
// - Enable compilation of unit tests with the library.
//
//     #define DN_WITH_UNIT_TESTS
//
// - Increase the capacity of the job queue, default is 128.
//
//     #define DN_JOB_QUEUE_SPMC_SIZE 128
*/

#if defined(DN_ONLY_VARRAY)       || \
    defined(DN_ONLY_SARRAY)       || \
    defined(DN_ONLY_FARRAY)       || \
    defined(DN_ONLY_DSMAP)        || \
    defined(DN_ONLY_LIST)         || \
    defined(DN_ONLY_FSTR8)        || \
    defined(DN_ONLY_FS)           || \
    defined(DN_ONLY_WINNET)       || \
    defined(DN_ONLY_WIN)          || \
    defined(DN_ONLY_SEMAPHORE)    || \
    defined(DN_ONLY_THREAD)       || \
    defined(DN_ONLY_V2)           || \
    defined(DN_ONLY_V3)           || \
    defined(DN_ONLY_V4)           || \
    defined(DN_ONLY_M4)           || \
    defined(DN_ONLY_RECT)         || \
    defined(DN_ONLY_JSON_BUILDER) || \
    defined(DN_ONLY_BIN)          || \
    defined(DN_ONLY_PROFILER)

    #if !defined(DN_ONLY_VARRAY)
    #define DN_NO_VARRAY
    #endif
    #if !defined(DN_ONLY_FARRAY)
    #define DN_NO_FARRAY
    #endif
    #if !defined(DN_ONLY_SARRAY)
    #define DN_NO_SARRAY
    #endif
    #if !defined(DN_ONLY_DSMAP)
    #define DN_NO_DSMAP
    #endif
    #if !defined(DN_ONLY_LIST)
    #define DN_NO_LIST
    #endif
    #if !defined(DN_ONLY_FSTR8)
    #define DN_NO_FSTR8
    #endif
    #if !defined(DN_ONLY_FS)
    #define DN_NO_FS
    #endif
    #if !defined(DN_ONLY_WINNET)
    #define DN_NO_WINNET
    #endif
    #if !defined(DN_ONLY_WIN)
    #define DN_NO_WIN
    #endif
    #if !defined(DN_ONLY_SEMAPHORE)
    #define DN_NO_SEMAPHORE
    #endif
    #if !defined(DN_ONLY_THREAD)
    #define DN_NO_THREAD
    #endif
    #if !defined(DN_ONLY_V2)
    #define DN_NO_V2
    #endif
    #if !defined(DN_ONLY_V3)
    #define DN_NO_V3
    #endif
    #if !defined(DN_ONLY_V4)
    #define DN_NO_V4
    #endif
    #if !defined(DN_ONLY_M4)
    #define DN_NO_M4
    #endif
    #if !defined(DN_ONLY_RECT)
    #define DN_NO_RECT
    #endif
    #if !defined(DN_ONLY_JSON_BUILDER)
    #define DN_NO_JSON_BUILDER
    #endif
    #if !defined(DN_ONLY_BIN)
    #define DN_NO_BIN
    #endif
    #if !defined(DN_ONLY_PROFILER)
    #define DN_NO_PROFILER
    #endif
#endif

#include "dqn_base.h"
#if defined(DN_WITH_CGEN)
    #if !defined(DN_NO_METADESK)
        #if !defined(_CRT_SECURE_NO_WARNINGS)
            #define _CRT_SECURE_NO_WARNINGS
            #define DN_UNDO_CRT_SECURE_NO_WARNINGS
        #endif

        // NOTE: Metadesk does not have the header for 'size_t'
        #if defined(DN_COMPILER_GCC)
        #include <stdint.h>
        #endif

        #define MD_DEFAULT_SPRINTF 0
        #define MD_IMPL_Vsnprintf DN_VSNPRINTF
        #include "External/metadesk/md.h"
        #if defined(DN_UNDO_CRT_SECURE_NO_WARNINGS)
            #undef _CRT_SECURE_NO_WARNINGS
        #endif
    #endif

    // Metadesk includes Windows.h
    #define DN_NO_WIN32_MIN_HEADER
#endif

#include "dqn_external.h"
#if defined(DN_PLATFORM_WIN32)
#include "dqn_win32.h"
#endif
#include "dqn_allocator.h"
#include "dqn_tls.h"
#include "dqn_debug.h"
#include "dqn_string.h"
#if defined(DN_PLATFORM_EMSCRIPTEN) || defined(DN_PLATFORM_POSIX) || defined(DN_PLATFORM_ARM64)
    #include "dqn_os_posix.h"
#elif defined(DN_PLATFORM_WIN32)
    #include "dqn_os_win32.h"
#else
    #error Please define a platform e.g. 'DN_PLATFORM_WIN32' to enable the correct implementation for platform APIs
#endif
#include "dqn_os.h"
#include "dqn_containers.h"
#include "dqn_math.h"
#include "dqn_hash.h"
#include "dqn_helpers.h"
#include "dqn_type_info.h"

#if defined(DN_WITH_CGEN)
    #include "Standalone/dqn_cpp_file.h"
    #include "dqn_cgen.h"
#endif

#if defined(DN_WITH_JSON)
    #if !defined(DN_NO_SHEREDOM_JSON)
        #include "External/json.h"
    #endif
    #include "dqn_json.h"
#endif
