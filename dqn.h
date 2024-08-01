#pragma once
#define DQN_H

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
//   dqn.h -- Personal standard library -- MIT License -- git.doylet.dev/dqn
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
// Compile dqn.cpp or include it into one of your translation units.
//
// Additionally, this library supports including/excluding specific sections
// of the library by using #define on the name of the section. These names are
// documented in the section table of contents at the #define column, for
// example:
//
//     #define DQN_ONLY_VARRAY
//     #define DQN_ONLY_WIN
//
// Compiles the library with all optional APIs turned off except virtual arrays
// and the Win32 helpers. Alternatively:
//
//     #define DQN_NO_VARRAY
//     #define DQN_NO_WIN
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
// Instead, documentation is laid out in dqn_docs.cpp in alphabetical order
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
//     #define DQN_MEMCPY(dest, src, count)   memcpy(dest, src, value)
//     #define DQN_MEMSET(dest, value, count) memset(dest, value, count)
//     #define DQN_MEMCMP(lhs, rhs, count)    memcpy(lhs, rhs, count)
//     #define DQN_MEMMOVE(dest, src, count)  memmove(dest, src, count)
//     #define DQN_SQRTF(val)                 sqrtf(val)
//     #define DQN_SINF(val)                  sinf(val)
//     #define DQN_COSF(val)                  cosf(val)
//     #define DQN_TANF(val)                  tanf(val)
//
// - Redefine 'DQN_API' to change the prefix of all declared functions in the library
//
//     #define DQN_API
//
// - Define 'DQN_STATIC_API' to apply 'static' to all function definitions and
//   disable external linkage to other translation units by redefining 'DQN_API' to
//   'static'.
//
//     #define DQN_STATIC_API
//
// - Turn all assertion macros to no-ops except for hard asserts (which are
//   always enabled and represent unrecoverable errors in the library).
//
//     #define DQN_NO_ASSERT
//
// - Augment DQN_CHECK(expr) macro's behaviour. By default when the check fails a
//   debug break is emitted. If this macro is defined, the check will not trigger
//   a debug break.
//
//     #define DQN_NO_CHECK_BREAK
//
// - Define this macro to enable memory leak tracking on arena's that are
//   configured to track allocations.
//
//   Allocations are stored in a global hash-table and their respective stack
//   traces for the allocation location. Memory leaks can be dumped at the end
//   of the program or some epoch by calling Dqn_Debug_DumpLeaks()
//
//     #define DQN_LEAK_TRACKING
//
// - Define this to revert to the family of printf functions from <stdio.h>
//   instead of using stb_sprintf in this library. stb_sprintf is 5-6x faster
//   than printf with a smaller binary footprint and has deterministic behaviour
//   across all supported platforms.
//
//     #define DQN_USE_STD_PRINTF
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
//     #define DQN_NO_WIN32_MIN_HEADER
//
// - Define this to stop this library from defining STB_SPRINTF_IMPLEMENTATION.
//   Useful if another library uses and includes "stb_sprintf.h"
//
//     #define DQN_STB_SPRINTF_HEADER_ONLY
//
// - Override the default break into the active debugger function. By default
//   we use __debugbreak() on Windows and raise(SIGTRAP) on other platforms.
//
//     #define DQN_DEBUG_BREAK
//
// - Define this macro to 1 to enable poisoning of memory from arenas when ASAN
//   `-fsanitize=address` is enabled. Enabling this will detect memory overwrite
//   by padding allocated before and after with poisoned memory which will raise
//   a use-after-poison in ASAN on read/write. This is a no-op if the library is
//   not compiled with ASAN.
//
//     #define DQN_ASAN_POISON 1
//
// - Define this macro 1 to enable sanity checks for manually poisoned memory in
//   this library when ASAN `-fsanitize=address` is enabled. These sanity checks
//   ensure that memory from arenas are correctly un/poisoned when pointers are
//   allocated and returned to the memory arena's. This is a no-op if we are not
//   compiled with ASAN or `DQN_ASAN_POISON` is not set to `1`.
//
//     #define DQN_ASAN_VET_POISON 1
//
// - Define this macro to the size of the guard memory reserved before and after
//   allocations made that are poisoned to protect against out-of-bounds memory
//   accesses. By default the library sets the guard to 128 bytes.
//
//     #define DQN_ASAN_POISON_GUARD_SIZE 128
//
// - Enable 'Dqn_CGen' a parser that can emit run-time type information and
//   allow arbitrary querying of data definitions expressed in Excel-like tables
//   using text files encoded in Dion-System's Metadesk grammar.
//
//   This option automatically includes 'dqn_cpp_file.h' to assist with code
//   generation and Metadesk's 'md.h' and its implementation library.
//
//     #define DQN_WITH_CGEN
//
//   Optionally define 'DQN_NO_METADESK' to disable the inclusion of Metadesk
//   in the library. This might be useful if you are including the librarin in
//   your  project yourself. This library must still be defined and visible
//   before this header.
//
// - Enable 'Dqn_JSON' a json parser. This option requires Sheredom's 'json.h'
//   to be included prior to this file.
//
//     #define DQN_WITH_JSON
//
//   Optionally define 'DQN_NO_SHEREDOM_JSON' to prevent Sheredom's 'json.h'
//   library from being included. This might be useful if you are including the
//   library in your project yourself. The library must still be defined and
//   visible before this header.
//
// - Enable compilation of unit tests with the library.
//
//     #define DQN_WITH_UNIT_TESTS
//
// - Increase the capacity of the job queue, default is 128.
//
//     #define DQN_JOB_QUEUE_SPMC_SIZE 128
*/

#if defined(DQN_ONLY_VARRAY)       || \
    defined(DQN_ONLY_SARRAY)       || \
    defined(DQN_ONLY_FARRAY)       || \
    defined(DQN_ONLY_DSMAP)        || \
    defined(DQN_ONLY_LIST)         || \
    defined(DQN_ONLY_FSTR8)        || \
    defined(DQN_ONLY_FS)           || \
    defined(DQN_ONLY_WINNET)       || \
    defined(DQN_ONLY_WIN)          || \
    defined(DQN_ONLY_SEMAPHORE)    || \
    defined(DQN_ONLY_THREAD)       || \
    defined(DQN_ONLY_V2)           || \
    defined(DQN_ONLY_V3)           || \
    defined(DQN_ONLY_V4)           || \
    defined(DQN_ONLY_M4)           || \
    defined(DQN_ONLY_RECT)         || \
    defined(DQN_ONLY_JSON_BUILDER) || \
    defined(DQN_ONLY_BIN)          || \
    defined(DQN_ONLY_PROFILER)

    #if !defined(DQN_ONLY_VARRAY)
    #define DQN_NO_VARRAY
    #endif
    #if !defined(DQN_ONLY_FARRAY)
    #define DQN_NO_FARRAY
    #endif
    #if !defined(DQN_ONLY_SARRAY)
    #define DQN_NO_SARRAY
    #endif
    #if !defined(DQN_ONLY_DSMAP)
    #define DQN_NO_DSMAP
    #endif
    #if !defined(DQN_ONLY_LIST)
    #define DQN_NO_LIST
    #endif
    #if !defined(DQN_ONLY_FSTR8)
    #define DQN_NO_FSTR8
    #endif
    #if !defined(DQN_ONLY_FS)
    #define DQN_NO_FS
    #endif
    #if !defined(DQN_ONLY_WINNET)
    #define DQN_NO_WINNET
    #endif
    #if !defined(DQN_ONLY_WIN)
    #define DQN_NO_WIN
    #endif
    #if !defined(DQN_ONLY_SEMAPHORE)
    #define DQN_NO_SEMAPHORE
    #endif
    #if !defined(DQN_ONLY_THREAD)
    #define DQN_NO_THREAD
    #endif
    #if !defined(DQN_ONLY_V2)
    #define DQN_NO_V2
    #endif
    #if !defined(DQN_ONLY_V3)
    #define DQN_NO_V3
    #endif
    #if !defined(DQN_ONLY_V4)
    #define DQN_NO_V4
    #endif
    #if !defined(DQN_ONLY_M4)
    #define DQN_NO_M4
    #endif
    #if !defined(DQN_ONLY_RECT)
    #define DQN_NO_RECT
    #endif
    #if !defined(DQN_ONLY_JSON_BUILDER)
    #define DQN_NO_JSON_BUILDER
    #endif
    #if !defined(DQN_ONLY_BIN)
    #define DQN_NO_BIN
    #endif
    #if !defined(DQN_ONLY_PROFILER)
    #define DQN_NO_PROFILER
    #endif
#endif

#if defined(DQN_WITH_CGEN)
    #if !defined(DQN_NO_METADESK)
        #if !defined(_CRT_SECURE_NO_WARNINGS)
            #define _CRT_SECURE_NO_WARNINGS
            #define DQN_UNDO_CRT_SECURE_NO_WARNINGS
        #endif
        #define MD_DEFAULT_SPRINTF 0
        #define MD_IMPL_Vsnprintf DQN_VSNPRINTF
        #include "External/metadesk/md.h"
        #if defined(DQN_UNDO_CRT_SECURE_NO_WARNINGS)
            #undef _CRT_SECURE_NO_WARNINGS
        #endif
    #endif

    // Metadesk includes Windows.h
    #define DQN_NO_WIN32_MIN_HEADER
#endif

#include "dqn_base.h"
#include "dqn_external.h"
#if defined(DQN_PLATFORM_WIN32)
#include "dqn_win32.h"
#endif
#include "dqn_allocator.h"
#include "dqn_tls.h"
#include "dqn_debug.h"
#include "dqn_string.h"
#include "dqn_containers.h"
#if defined(DQN_PLATFORM_EMSCRIPTEN) || defined(DQN_PLATFORM_POSIX) || defined(DQN_PLATFORM_ARM64)
#elif defined(DQN_PLATFORM_WIN32)
    #include "dqn_os_win32.h"
#else
    #error Please define a platform e.g. 'DQN_PLATFORM_WIN32' to enable the correct implementation for platform APIs
#endif
#include "dqn_os.h"
#include "dqn_math.h"
#include "dqn_hash.h"
#include "dqn_helpers.h"
#include "dqn_type_info.h"

#if defined(DQN_WITH_CGEN)
    #include "Standalone/dqn_cpp_file.h"
    #include "dqn_cgen.h"
#endif

#if defined(DQN_WITH_JSON)
    #if !defined(DQN_NO_SHEREDOM_JSON)
        #include "External/json.h"
    #endif
    #include "dqn_json.h"
#endif
