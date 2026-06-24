#if !defined(DN_H)
#define DN_H

// NOTE: DN

//  NOTE: Getting Started
//    - Copy the entire DN folder to your project and include `dn.h` alongside your headers and
//      include `dn.cpp` in one of your implementation files that has `dn.h` included in its scope,
//      an example. There are #defines to selectively enable features of the library (see the
//      configuration section for more info), example:
/*
          #define DN_WITH_OS 1 // Enable OS features (like virtual mem, TLS, file system, threads)
          #include "dn.h"

          #define DN_CPP_WITH_TESTS 1
          #include "dn.cpp"

          int main()
          {
            DN_Core core = {};
            DN_Init(&core, DN_InitFlags_Nil, DN_TCInitArgsDefault());
            return 0;
          }
*/
//    - `DN/Standalone` contains utilities that are self-contained and can be used without `dn.h` in
//      a similar manner, typically a single header and single implementation file.

//  NOTE: Configuration

//    NOTE: OS layer
//      Enable the operating system layer which enables thread context, file system, threads, e.t.c:
//
//        #define DN_WITH_OS 1

//    NOTE: Networking layer
//      Enable the networking layer (pre-requisite that the OS layer is enabled) that allows sending
//      HTTP/Websocket requests either using CURL or Emscripten's networking APIs.
//
//        #define DN_WITH_NET            1
//        #define DN_WITH_NET_CURL       1
//        #define DN_WITH_NET_EMSCRIPTEN 1
//
//      For CURL, ensure that you the target application links to a libcurl and that the
//      #include <curl/curl.h> is visible before this translation unit.
//
//      For Emscripten ensure that this translation unit is compiled with `emcc` to have it enabled.

//    NOTE: Platform Target
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

//    NOTE: Static functions
//      All public functions in the DN library are prefixed with the macro '#define DN_API'. By
//      default 'DN_API' is not defined to anything. Define
//
//        DN_STATIC_API
//
//      To replace all the functions prefixed with DN_API to be prefixed with 'static' ensuring that
//      the functions in the library do not export an entry into the linking table.
//      translation units.

//    NOTE: Disabling the in-built <Windows.h> (if #define DN_WITH_OS 1)
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

//    NOTE: Freestanding
//      The base layer can be used without an OS implementation by defining DN_FREESTANDING like:
//
//        #define DN_FREESTANDING
//
//      This means functionality that relies on the OS like printing, memory allocation, stack traces
//      and so forth are disabled.

//    NOTE: ASAN Arena Poisoning
//      When compiled with address sanitizer (.e.g -fsanitize=address) you can optionally enable
//      memory region poisoning on the inbuilt arena's to catch in certain scenarios, use-after-free
//
//        #define DN_ASAN_POISON 1
//
//      Since arenas manage their own block of memory it does not automatically benefit from ASAN's
//      memory markup that ASAN does and so it is implemented manually by using the ASAN user-level
//      poisoning APIs. Similarly, since the arena recycles its own memory rather than release back
//      to the OS, poisoning is not as effective for arenas but every little bit helps.

//    NOTE: Scrub Uninitialised Memory
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

//    NOTE: Arena temp memory use-after-free (UAF) tooling
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
//      NOTE: UAF Tracing
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

//    NOTE: Paranoia Level
//      Set the `DN_PARANOIA_LEVEL` to an integer value to enable various validation layers and
//      error checking mechanisms in the codebase and primitives exposed by the library. Defaults to
//      paranoia level 0 in release builds and level 1 for debug.
//
//          #define DN_PARANOIA_LEVEL 1
//
//      Each level activates the following debug mechanisms. Note that any of the following #defines
//      enabled by a paranoia level can be overridden by defining the preprocessor definition before
//      the inclusion of this file.
//
//        Level 0
//          `DN_Assert` calls are compiled out
//
//          `DN_Verify` calls logs an error and continues
//
//          `DN_VerifyWarning` calls logs a warning and continues
//
//        Level 1
//          `DN_Assert` calls are compiled in
//
//          `DN_Verify` calls a debug trap rather than just logging and continuing
//
//          `DN_Verify` calls dump a stack trace when triggered
//
//          `DN_ASAN_POISON` is set. When an arena allocates memory unallocated bytes from the
//          memory owned by the arena are manually poisoned using ASAN. A fault will be triggered if
//          the memory is written to (UAF e.g. use-after-free). Address sanitizer must be enabled or
//          otherwise this is a no-op. This incurs a performance penalty on-top of the overhead of
//          running ASAN on your binary as recycling memory calls into ASAN to poison the region.
//
//          `DN_ARENA_TEMP_MEM_UAF_GUARD` is set. When an arena uses temporary memory it will record
//          the active temporary memory region and compare them when allocating to ensure that
//          memory is allocated in the active region otherwise a UAF fault is triggered. This has a
//          small runtime performance penalty.
//
//          `DN_SCRUB_UNINIT_MEM_BYTE` is set to `0xCD`. When memory is cleared in an arena or a
//          pool backed by an arena upon deallocation if the `DN_ZMem_Yes` flag is passed then the
//          bytes are scrubbed to this byte to make UAF more salient.
//
//        Level 2
//          `DN_ARENA_TEMP_MEM_UAF_TRACE_ON_BY_DEFAULT` is set. When an arena uses temporary memory
//          regions that region's a stack trace of the call site is recorded. This is very expensive
//          but when a temporary memory region is used after it has been deallocated, a full stack
//          trace diagnostic is available of where the various regions where created and freed.
//
//    NOTE: Str8 AVX512F variants
//      We have some AVX512 string functions that can be enabled by defining the following
//
//        #define DN_STR8_AVX512F 1

#if defined(_CLANGD)
  #define DN_WITH_OS 1
  #define DN_STR8_AVX512F 1
  #define DN_PARANOIA_LEVEL 1
#endif

// NOTE: Compiler identification
// NOTE: Warning! Order is important here, clang-cl on Windows defines _MSC_VER
#if defined(_MSC_VER)
  #if defined(__clang__)
    #define DN_COMPILER_CLANG_CL
    #define DN_COMPILER_CLANG
  #else
    #define DN_COMPILER_MSVC
  #endif
#elif defined(__clang__)
  #define DN_COMPILER_CLANG
#elif defined(__GNUC__)
  #define DN_COMPILER_GCC
#endif

// NOTE: __has_feature
// NOTE: MSVC for example does not support the feature detection macro for instance so we compile it
// out
#if defined(__has_feature)
  #define DN_HAS_FEATURE(expr) __has_feature(expr)
#else
  #define DN_HAS_FEATURE(expr) 0
#endif

// NOTE: __has_builtin
// NOTE: MSVC for example does not support the feature detection macro for instance so we compile it out
#if defined(__has_builtin)
  #define DN_HAS_BUILTIN(expr) __has_builtin(expr)
#else
  #define DN_HAS_BUILTIN(expr) 0
#endif

// NOTE: Warning suppression macros
#if defined(DN_COMPILER_MSVC)
  #define DN_MSVC_WARNING_PUSH         __pragma(warning(push))
  #define DN_MSVC_WARNING_DISABLE(...) __pragma(warning(disable :##__VA_ARGS__))
  #define DN_MSVC_WARNING_ENABLE(...)  __pragma(warning(default :##__VA_ARGS__))
  #define DN_MSVC_WARNING_POP          __pragma(warning(pop))
#else
  #define DN_MSVC_WARNING_PUSH
  #define DN_MSVC_WARNING_ENABLE(...)
  #define DN_MSVC_WARNING_DISABLE(...)
  #define DN_MSVC_WARNING_POP
#endif

#if defined(DN_COMPILER_CLANG) || defined(DN_COMPILER_GCC) || defined(DN_COMPILER_CLANG_CL)
  #define DN_GCC_WARNING_PUSH                _Pragma("GCC diagnostic push")
  #define DN_GCC_WARNING_DISABLE_HELPER_0(x) #x
  #define DN_GCC_WARNING_DISABLE_HELPER_1(y) DN_GCC_WARNING_DISABLE_HELPER_0(GCC diagnostic ignored #y)
  #define DN_GCC_WARNING_DISABLE(warning)    _Pragma(DN_GCC_WARNING_DISABLE_HELPER_1(warning))
  #define DN_GCC_WARNING_POP                 _Pragma("GCC diagnostic pop")
#else
  #define DN_GCC_WARNING_PUSH
  #define DN_GCC_WARNING_DISABLE(...)
  #define DN_GCC_WARNING_POP
#endif

// NOTE: Host OS identification
#if defined(_WIN32)
  #define DN_OS_WIN32
#elif defined(__gnu_linux__) || defined(__linux__)
  #define DN_OS_UNIX
#endif

// NOTE: Platform identification
#if !defined(DN_PLATFORM_EMSCRIPTEN) && \
    !defined(DN_PLATFORM_POSIX) &&      \
    !defined(DN_PLATFORM_WIN32)
  #if defined(__aarch64__) || defined(_M_ARM64)
    #define DN_PLATFORM_ARM64
  #elif defined(__EMSCRIPTEN__)
    #define DN_PLATFORM_EMSCRIPTEN
  #elif defined(DN_OS_WIN32)
    #define DN_PLATFORM_WIN32
  #else
    #define DN_PLATFORM_POSIX
  #endif
#endif

// NOTE: Windows crap
#if defined(DN_COMPILER_MSVC) || defined(DN_COMPILER_CLANG_CL)
  #if defined(_CRT_SECURE_NO_WARNINGS)
    #define DN_CRT_SECURE_NO_WARNINGS_PREVIOUSLY_DEFINED
  #else
    #define _CRT_SECURE_NO_WARNINGS
  #endif
#endif

// NOTE: Force Inline
#if defined(DN_COMPILER_MSVC) || defined(DN_COMPILER_CLANG_CL)
  #define DN_FORCE_INLINE __forceinline
#else
  #define DN_FORCE_INLINE inline __attribute__((always_inline))
#endif

// NOTE: Function/Variable Annotations
#if defined(DN_STATIC_API)
  #define DN_API static
#else
  #define DN_API
#endif

// NOTE: C/CPP Literals
// Declare struct literals that work in both C and C++ because the syntax is different between
// languages.
#if 0
  struct Foo { int a; }
  struct Foo foo = DN_LITERAL(Foo){32}; // Works on both C and C++
#endif

#if defined(__cplusplus)
  #define DN_Literal(T) T
#else
  #define DN_Literal(T) (T)
#endif

// NOTE: Thread Locals
#if defined(__cplusplus)
  #define DN_THREAD_LOCAL thread_local
#else
  #define DN_THREAD_LOCAL _Thread_local
#endif

// NOTE: C variadic argument annotations
// TODO: Other compilers
#if defined(DN_COMPILER_MSVC)
  #define DN_FMT_ATTRIB _Printf_format_string_
#else
  #define DN_FMT_ATTRIB
#endif

// NOTE: Type Cast
#define DN_Cast(val) (val)

// NOTE: Zero initialisation macro
#if defined(__cplusplus)
  #define DN_ZeroInit {}
#else
  #define DN_ZeroInit {0}
#endif

// NOTE: Macros
#define DN_Stringify(x) #x
#define DN_TokenCombine2(x, y) x ## y
#define DN_TokenCombine(x, y) DN_TokenCombine2(x, y)

// NOTE: Error Checking/Validating
// Asserts are useful to verify invariants in the codebase, but there's sometimes the ambiguous
// question of what should be asserted, what happens when we should have triggered an assert
// in a release build (where they are canonically turned off), what alternative mechanisms should we
// use for error checking that should be visible to non-developers.
//
// The following is an excerpt from Tom Forsyth's assertion article which he references Chris
// Hargrove's guidelines on how asserts show be used. It is quite reasonable and we model our
// primitives after based on those concepts:
//
//   Logging, asserts and unit tests (https://tomforsyth1000.github.io/blog.wiki.html
//
//   Assert: Immediately fatal, and not ignorable. Fundamental assumption by an engineer has been
//   disproven and needs immediate handling. Requires discipline on the part of the engineer to not
//   add them in situations that are actually non-fatal (rule of thumb being that if a crash would
//   be almost certain to happen anyway due to the same condition, then you’re no worse off making
//   an assert).
//
//   Errors: Probably fatal soon, but not necessarily immediately. Basically a marker for “you are
//   now in a f*cked state, you might limp along a bit, but assume nothing”. Game continues, but an
//   ugly red number gets displayed onscreen for how many of these have been encountered (so when
//   people send you screenshots of bugs you can then point to the red error count and blame
//   accordingly). Savegames are disabled from this point so as not to make the error effectively
//   permanent; you should also deliberately violate a few other TCRs as soon as an error is
//   encountered in order to ensure that all parties up and down the publisher/developer chain are
//   aware of how bad things are. Errors are technically “ignorable” but everyone knows that it
//   might only buy you a little bit of borrowed time; these are only a small step away from the
//   immediately-blocking nature of an assert, but sometimes that small step can have a big impact
//   on productivity.
//
//   Warnings: Used for “you did something bad, but we caught it so it’s fine (the game state is
//   still okay), however it might not be fine in the future so if you want to save yourself some
//   headache you should fix this sooner rather than later”. Great for content problems. Also
//   displayed onscreen as a yellow number (near the red error number). You can keep these around
//   for a while and triage them when their utility is called into question.
//
//   Crumbs: The meta-category for a large number of “verbose” informational breadcrumb categories
//   that must be explicitly enabled so you don’t clutter everything up and obscure stuff that
//   matters. Note that the occurrance of certain Errors should automatically enable relevant
//   categories of crumbs so that more detailed information about the aforementioned f*cked state
//   will be provided during the limp-along timeframe.
//
// In the excerpt, their domain (games programming) prioritises continuity over immediate failure
// on warning and error as this allows non-developer clientele to continue using the application
// despite error laden states. This is useful in general as not all failures are critical to the
// use case that the end user is dealing with.
//
// We model `Errors` and `Warnings` as `DN_Verify` and `DN_VerifyWarning` respectively. The verify
// variants check the expression to test, log and a message and allow the developer to branch on the
// result and "recover" where appropriate. Verify checks are never compiled out. We have traditional
// `Asserts` as `DN_Assert` which can be compiled out.
//
// The article also defines what it calls a paranoia level. We `#define DN_PARANOIA_LEVEL <Integer>`
// to customise the validation layers of the codebase. See DN_PARANOIA_LEVEL in the customisation
// section for more information.
//
// In summary use each of the primitives in these situation:
//
//   `DN_Assert`: Fatal and immediately needs attention and can be compiled out
//
//   `DN_Verify`: Fatal or eventually fatal but not necessarily immediately, program is or will
//   degenerate into an incorrect state. Is always compiled in and is visible in non-debug
//   environments.
//
//   `DN_VerifyWarning`: Something bad happened, but we caught it and recovered from it. Program
//   state remains consistent. It is always compiled in and is visible in non-debug environments.

#if !defined(DN_PARANOIA_LEVEL)
  #if defined(NDEBUG)
    #define DN_PARANOIA_LEVEL 0
  #else
    #define DN_PARANOIA_LEVEL 1
  #endif
#endif

#if DN_HAS_FEATURE(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
  #include <sanitizer/asan_interface.h>
#endif

#define DN_ASAN_POISON_ALIGNMENT 8
#if !defined(DN_ASAN_VET_POISON)
  #define DN_ASAN_VET_POISON 0
#endif

#if !defined(DN_ASAN_POISON)
  #if DN_PARANOIA_LEVEL >= 1
    #define DN_ASAN_POISON 1
  #else
    #define DN_ASAN_POISON 0
  #endif
#endif

#if !defined(DN_ASAN_POISON_GUARD_SIZE)
  #define DN_ASAN_POISON_GUARD_SIZE 128
#endif

#if !defined(DN_ARENA_TEMP_MEM_UAF_GUARD)
  #if DN_PARANOIA_LEVEL >= 1
    #define DN_ARENA_TEMP_MEM_UAF_GUARD 1
  #else
    #define DN_ARENA_TEMP_MEM_UAF_GUARD 0
  #endif
#endif

#if !defined(DN_ARENA_TEMP_MEM_UAF_TRACE_ON_BY_DEFAULT)
  #if DN_PARANOIA_LEVEL >= 2
    #define DN_ARENA_TEMP_MEM_UAF_TRACE_ON_BY_DEFAULT 1
  #else
    #define DN_ARENA_TEMP_MEM_UAF_TRACE_ON_BY_DEFAULT 0
  #endif
#endif

#if !defined(DN_SCRUB_UNINIT_MEM_BYTE)
  #if DN_PARANOIA_LEVEL >= 1
    #define DN_SCRUB_UNINIT_MEM_BYTE 0xCD
  #else
    #define DN_SCRUB_UNINIT_MEM_BYTE 0x00
  #endif
#endif

#define DN_AssertRaw(expr) do { if (!(expr)) DN_DebugBreak; } while (0)
#define DN_AssertAlwaysCallSiteF(expr, call_site, fmt, ...)                                                                                                   \
  do {                                                                                                                                                        \
    if (!(expr)) {                                                                                                                                            \
      DN_Str8         trace_    = DN_Str8FromStackTraceNowHeap(128 /*limit*/, 3 /*skip*/);                                                                    \
      DN_LogTypeParam log_type_ = DN_LogTypeParamFromType(DN_LogType_Error);                                                                                  \
      DN_LogPrintF(log_type_, call_site, DN_LogFlags_Nil, "Assertion triggered [" #expr "]. " fmt "\nTrace:\n%.*s", ## __VA_ARGS__, DN_Str8PrintFmt(trace_)); \
      DN_DebugBreak;                                                                                                                                          \
    }                                                                                                                                                         \
  } while (0)

#define DN_AssertAlwaysF(expr, fmt, ...)              DN_AssertAlwaysCallSiteF(expr, (DN_CallSiteNow), fmt, ##__VA_ARGS__)
#define DN_AssertAlways(expr)                         DN_AssertAlwaysF(expr, "")
#define DN_AssertInvalidCodePathF(fmt, ...)           DN_AssertAlwaysF(0, fmt, ##__VA_ARGS__)
#define DN_AssertInvalidCodePath                      DN_AssertInvalidCodePathF("Invalid code path")
#if DN_PARANOIA_LEVEL >= 1
#define DN_AssertCallSiteF(expr, call_site, fmt, ...) DN_AssertAlwaysCallSiteF(expr, call_site, fmt, ## __VA_ARGS__)
#define DN_AssertF(expr, fmt, ...)                    DN_AssertCallSiteF(expr, (DN_CallSiteNow), fmt, ## __VA_ARGS__)
#define DN_Assert(expr)                               DN_AssertAlways(expr)
#else
#define DN_AssertCallSiteF(expr, call_site, fmt, ...) (void)(expr); (void)call_site
#define DN_AssertF(expr, fmt, ...)                    (void)(expr)
#define DN_Assert(expr)                               (void)(expr)
#endif
#define DN_VerifyF(expr, fmt, ...)                    DN_VerifyArgsF(DN_VerifyType_Nil, expr, (DN_CallSiteNow), DN_Str8Lit(#expr), fmt, ##__VA_ARGS__)
#define DN_VerifyWarningF(expr, fmt, ...)             DN_VerifyArgsF(DN_VerifyType_Warning, expr, (DN_CallSiteNow), DN_Str8Lit(#expr), fmt, ##__VA_ARGS__)
#define DN_Verify(expr)                               DN_VerifyF(expr, 0)
#define DN_VerifyWarning(expr)                        DN_VerifyWarningF(expr, 0)

#define DN_StaticAssert(expr)                                                     \
  DN_GCC_WARNING_PUSH                                                             \
  DN_GCC_WARNING_DISABLE(-Wunused-local-typedefs)                                 \
  typedef char DN_TokenCombine(static_assert_dummy__, __LINE__)[(expr) ? 1 : -1]; \
  DN_GCC_WARNING_POP

#if defined(__aarch64__) || defined(_M_X64) || defined(__x86_64__) || defined(__x86_64)
  #define DN_64_BIT
#else
  #define DN_32_BIT
#endif

#include <stdarg.h> // va_list
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <inttypes.h> // PRIu64...

#if !defined(DN_OS_WIN32)
#include <stdlib.h> // exit()
#endif

#define DN_ForIndexU(index, count)               DN_USize index = 0; index < count; index++
#define DN_ForIndexI(index, count)               DN_ISize index = 0; index < count; index++
#define DN_ForItSize(it, T, array, count)        struct { DN_USize index; T *data; } it = {0,           &(array)[0]};         it.index < (count);               it.index++, it.data = (array)         + it.index
#define DN_ForItSizeReverse(it, T, array, count) struct { DN_USize index; T *data; } it = {(count) - 1, &(array)[count - 1]}; it.index < (count);               it.index--, it.data = (array)         + it.index
#define DN_ForIt(it, T, array)                   struct { DN_USize index; T *data; } it = {0,           &(array)->data[0]};   it.index < (array)->count;        it.index++, it.data = ((array)->data) + it.index
#define DN_ForLinkedListIt(it, T, list)          struct { DN_USize index; T *data; } it = {0,           list};                it.data;                          it.index++, it.data = ((it).data->next)
#define DN_ForItCArray(it, T, array)             struct { DN_USize index; T *data; } it = {0,           &(array)[0]};         it.index < DN_ArrayCountU(array); it.index++, it.data = (array)         + it.index

#define DN_AlignUpPowerOfTwo(value, pot)        (((uintptr_t)(value) + ((uintptr_t)(pot) - 1)) & ~((uintptr_t)(pot) - 1))
#define DN_AlignDownPowerOfTwo(value, pot)      ((uintptr_t)(value) & ~((uintptr_t)(pot) - 1))
#define DN_IsPowerOfTwo(value)                  ((((uintptr_t)(value)) & (((uintptr_t)(value)) - 1)) == 0)
#define DN_IsPowerOfTwoAligned(value, pot)      ((((uintptr_t)value) & (((uintptr_t)pot) - 1)) == 0)

// NOTE: String.h Dependencies
#if !defined(DN_Memcpy) || !defined(DN_Memset) || !defined(DN_Memcmp) || !defined(DN_Memmove)
  #include <string.h>
  #if !defined(DN_Memcpy)
    #define DN_Memcpy(dest, src, count) memcpy((dest), (src), (count))
  #endif
  #if !defined(DN_Memset)
    #define DN_Memset(dest, value, count) memset((dest), (value), (count))
  #endif
  #if !defined(DN_Memcmp)
    #define DN_Memcmp(lhs, rhs, count) memcmp((lhs), (rhs), (count))
  #endif
  #if !defined(DN_Memmove)
    #define DN_Memmove(dest, src, count) memmove((dest), (src), (count))
  #endif
#endif

// NOTE: Math.h Dependencies
#if !defined(DN_SqrtF32) || !defined(DN_SinF32) || !defined(DN_CosF32) || !defined(DN_TanF32)
  #include <math.h>
  #if !defined(DN_SqrtF32)
    #define DN_SqrtF32(val) sqrtf(val)
  #endif
  #if !defined(DN_SinF32)
    #define DN_SinF32(val) sinf(val)
  #endif
  #if !defined(DN_CosF32)
    #define DN_CosF32(val) cosf(val)
  #endif
  #if !defined(DN_TanF32)
    #define DN_TanF32(val) tanf(val)
  #endif
  #if !defined(DN_PowF32)
    #define DN_PowF32(val, exp) powf(val, exp)
  #endif
#endif

// NOTE: Math
#define DN_PiF32 3.14159265359f

#define DN_DegreesToRadsF32(degrees) ((degrees) * (DN_PiF32 / 180.0f))
#define DN_RadsToDegreesF32(radians) ((radians) * (180.f * DN_PiF32))

#define DN_Abs(val) (((val) < 0) ? (-(val)) : (val))
#define DN_Max(a, b) (((a) > (b)) ? (a) : (b))
#define DN_Min(a, b) (((a) < (b)) ? (a) : (b))
#define DN_Clamp(val, lo, hi) DN_Max(DN_Min(val, hi), lo)
#define DN_Squared(val) ((val) * (val))

#define DN_Swap(a, b) \
  do {                \
    auto temp = a;    \
    a         = b;    \
    b         = temp; \
  } while (0)

// NOTE: Size
#define DN_SizeOfI(val)       DN_Cast(ptrdiff_t)sizeof(val)
#define DN_ArrayCountU(array) (sizeof(array)/(sizeof((array)[0])))
#define DN_ArrayCountI(array) (DN_ISize)DN_ArrayCountU(array)
#define DN_CharCountU(string) (sizeof(string) - 1)

// NOTE: SI Byte
#define DN_Bytes(val)     ((DN_U64)val)
#define DN_Kilobytes(val) ((DN_U64)1024 * DN_Bytes(val))
#define DN_Megabytes(val) ((DN_U64)1024 * DN_Kilobytes(val))
#define DN_Gigabytes(val) ((DN_U64)1024 * DN_Megabytes(val))

// NOTE: Time
#define DN_MsFromSec(val)    ((val) * 1000ULL)
#define DN_SecFromMins(val)  ((val) * 60ULL)
#define DN_SecFromHours(val) (DN_SecFromMins(val)  * 60ULL)
#define DN_SecFromDays(val)  (DN_SecFromHours(val) * 24ULL)
#define DN_SecFromWeeks(val) (DN_SecFromDays(val)  *  7ULL)
#define DN_SecFromYears(val) (DN_SecFromWeeks(val) * 52ULL)

// NOTE: Debug Break
#if !defined(DN_DebugBreak)
  #if defined(NDEBUG)
    #define DN_DebugBreak
  #else
    #if defined(DN_COMPILER_MSVC) || defined(DN_COMPILER_CLANG_CL)
      #define DN_DebugBreak __debugbreak()
    #elif DN_HAS_BUILTIN(__builtin_debugtrap)
      #define DN_DebugBreak __builtin_debugtrap()
    #elif DN_HAS_BUILTIN(__builtin_trap) || defined(DN_COMPILER_GCC)
      #define DN_DebugBreak __builtin_trap()
    #else
      #include <signal.h>
      #if defined(SIGTRAP)
        #define DN_DebugBreak raise(SIGTRAP)
      #else
        #define DN_DebugBreak raise(SIGABRT)
      #endif
    #endif
  #endif
#endif

// NOTE: Helper macros to declare an array data structure for a given `Type`
#define DN_DArrayStructDecl(Type) \
  struct Type##Array             \
  {                              \
    Type*    data;               \
    DN_USize count;              \
    DN_USize max;                \
  }

#define DN_FixedArrayStructDecl(Type, capacity) \
  struct Type##x##capacity##Array \
  {                               \
    Type     data[capacity];      \
    DN_USize count;               \
    DN_USize max;                 \
  }

// NOTE: Types
typedef intptr_t     DN_ISize;
typedef uintptr_t    DN_USize;

typedef int8_t       DN_I8;
typedef int16_t      DN_I16;
typedef int32_t      DN_I32;
typedef int64_t      DN_I64;

typedef uint8_t      DN_U8;
typedef uint16_t     DN_U16;
typedef uint32_t     DN_U32;
typedef uint64_t     DN_U64;

typedef uintptr_t    DN_UPtr;
typedef float        DN_F32;
typedef double       DN_F64;
typedef unsigned int DN_UInt;
typedef DN_I32       DN_B32;

#define DN_F32_MAX   3.402823466e+38F
#define DN_F32_MIN   1.175494351e-38F
#define DN_F64_MAX   1.7976931348623158e+308
#define DN_F64_MIN   2.2250738585072014e-308
#define DN_USIZE_MAX UINTPTR_MAX
#define DN_ISIZE_MAX INTPTR_MAX
#define DN_ISIZE_MIN INTPTR_MIN

// NOTE: Intrinsics
// NOTE: DN_AtomicAdd/Exchange return the previous value store in the target
#if defined(DN_COMPILER_MSVC) || defined(DN_COMPILER_CLANG_CL)
  #include <intrin.h>
  #define DN_AtomicCompareExchange64(dest, desired_val, prev_val) _InterlockedCompareExchange64((__int64 volatile *)dest, desired_val, prev_val)
  #define DN_AtomicCompareExchange32(dest, desired_val, prev_val) _InterlockedCompareExchange((long volatile *)dest, desired_val, prev_val)

  #define DN_AtomicLoadU64(target)                                *(target)
  #define DN_AtomicLoadU32(target)                                *(target)
  #define DN_AtomicAddU32(target, value)                          _InterlockedExchangeAdd((long volatile *)target, value)
  #define DN_AtomicAddU64(target, value)                          _InterlockedExchangeAdd64((__int64 volatile *)target, value)
  #define DN_AtomicSubU32(target, value)                          DN_AtomicAddU32(DN_Cast(long volatile *) target, (long)-value)
  #define DN_AtomicSubU64(target, value)                          DN_AtomicAddU64(target, (DN_U64) - value)

  #define DN_CountLeadingZerosU64(value)                           __lzcnt64(value)
  #define DN_CountLeadingZerosU32(value)                           __lzcnt(value)
  #define DN_CPUGetTSC()                                           __rdtsc()
  #define DN_CompilerReadBarrierAndCPUReadFence                    _ReadBarrier();  _mm_lfence()
  #define DN_CompilerWriteBarrierAndCPUWriteFence                  _WriteBarrier(); _mm_sfence()
#elif defined(DN_COMPILER_GCC) || defined(DN_COMPILER_CLANG)
  #if defined(__ANDROID__)
  #elif defined(DN_PLATFORM_EMSCRIPTEN)
    #if !defined(__wasm_simd128__)
      #error DN_Base requires -msse2 to be passed to Emscripten
    #endif
    #include <emmintrin.h>
  #else
    #include <x86intrin.h>
  #endif

  #define DN_AtomicLoadU64(target)                                __atomic_load_n(x, __ATOMIC_SEQ_CST)
  #define DN_AtomicLoadU32(target)                                __atomic_load_n(x, __ATOMIC_SEQ_CST)
  #define DN_AtomicAddU32(target, value)                          __atomic_fetch_add(target, value, __ATOMIC_ACQ_REL)
  #define DN_AtomicAddU64(target, value)                          __atomic_fetch_add(target, value, __ATOMIC_ACQ_REL)
  #define DN_AtomicSubU32(target, value)                          __atomic_fetch_sub(target, value, __ATOMIC_ACQ_REL)
  #define DN_AtomicSubU64(target, value)                          __atomic_fetch_sub(target, value, __ATOMIC_ACQ_REL)

  #define DN_CountLeadingZerosU64(value)                           __builtin_clzll(value)
  #define DN_CountLeadingZerosU32(value)                           __builtin_clzl(value)

  #if defined(DN_COMPILER_GCC)
    #define DN_CPUGetTSC() __rdtsc()
  #else
    #define DN_CPUGetTSC() __builtin_readcyclecounter()
  #endif

  #if defined(DN_PLATFORM_EMSCRIPTEN)
    #define DN_CompilerReadBarrierAndCPUReadFence
    #define DN_CompilerWriteBarrierAndCPUWriteFence
  #else
    #define DN_CompilerReadBarrierAndCPUReadFence   asm volatile("lfence" ::: "memory")
    #define DN_CompilerWriteBarrierAndCPUWriteFence asm volatile("sfence" ::: "memory")
  #endif
#else
  #error "Compiler not supported"
#endif

#if defined(DN_64_BIT)
  #define DN_CountLeadingZerosUSize(value) DN_CountLeadingZerosU64(value)
#else
  #define DN_CountLeadingZerosUSize(value) DN_CountLeadingZerosU32(value)
#endif

enum DN_VerifyType
{
  DN_VerifyType_Nil,
  DN_VerifyType_Warning,
};

enum DN_ZMem
{
  DN_ZMem_No,  // Memory can be handed out without zero-ing it out
  DN_ZMem_Yes, // Memory should be zero-ed out before giving to the callee
};

struct DN_Str8
{
  char    *data; // The bytes of the string
  DN_USize size; // The number of bytes in the string
};

struct DN_Str8Slice
{
  DN_Str8 *data;
  DN_USize count;
};

struct DN_Str8x16   { char data[16];  DN_USize size; };
struct DN_Str8x32   { char data[32];  DN_USize size; };
struct DN_Str8x64   { char data[64];  DN_USize size; };
struct DN_Str8x128  { char data[128]; DN_USize size; };
struct DN_Str8x256  { char data[256]; DN_USize size; };
struct DN_Str8x512  { char data[512]; DN_USize size; };
struct DN_Str8x1024 { char data[1024]; DN_USize size; };

struct DN_Str16 // A pointer and length style string that holds slices to UTF16 bytes.
{
  wchar_t *data; // The UTF16 bytes of the string
  DN_USize size; // The number of characters in the string
};

struct DN_Str16Slice
{
  DN_Str16 *data;
  DN_USize count;
};

struct DN_CPURegisters
{
  int eax;
  int ebx;
  int ecx;
  int edx;
};

union DN_CPUIDResult
{
  DN_CPURegisters reg;
  int             values[4];
};

struct DN_CPUIDArgs { int eax; int ecx; };

#define DN_CPU_FEAT_XMACRO               \
  DN_CPU_FEAT_XENTRY(3DNow)              \
  DN_CPU_FEAT_XENTRY(3DNowExt)           \
  DN_CPU_FEAT_XENTRY(ABM)                \
  DN_CPU_FEAT_XENTRY(AES)                \
  DN_CPU_FEAT_XENTRY(AVX)                \
  DN_CPU_FEAT_XENTRY(AVX2)               \
  DN_CPU_FEAT_XENTRY(AVX512F)            \
  DN_CPU_FEAT_XENTRY(AVX512DQ)           \
  DN_CPU_FEAT_XENTRY(AVX512IFMA)         \
  DN_CPU_FEAT_XENTRY(AVX512PF)           \
  DN_CPU_FEAT_XENTRY(AVX512ER)           \
  DN_CPU_FEAT_XENTRY(AVX512CD)           \
  DN_CPU_FEAT_XENTRY(AVX512BW)           \
  DN_CPU_FEAT_XENTRY(AVX512VL)           \
  DN_CPU_FEAT_XENTRY(AVX512VBMI)         \
  DN_CPU_FEAT_XENTRY(AVX512VBMI2)        \
  DN_CPU_FEAT_XENTRY(AVX512VNNI)         \
  DN_CPU_FEAT_XENTRY(AVX512BITALG)       \
  DN_CPU_FEAT_XENTRY(AVX512VPOPCNTDQ)    \
  DN_CPU_FEAT_XENTRY(AVX5124VNNIW)       \
  DN_CPU_FEAT_XENTRY(AVX5124FMAPS)       \
  DN_CPU_FEAT_XENTRY(AVX512VP2INTERSECT) \
  DN_CPU_FEAT_XENTRY(AVX512FP16)         \
  DN_CPU_FEAT_XENTRY(CLZERO)             \
  DN_CPU_FEAT_XENTRY(CMPXCHG8B)          \
  DN_CPU_FEAT_XENTRY(CMPXCHG16B)         \
  DN_CPU_FEAT_XENTRY(F16C)               \
  DN_CPU_FEAT_XENTRY(FMA)                \
  DN_CPU_FEAT_XENTRY(FMA4)               \
  DN_CPU_FEAT_XENTRY(FP128)              \
  DN_CPU_FEAT_XENTRY(FP256)              \
  DN_CPU_FEAT_XENTRY(FPU)                \
  DN_CPU_FEAT_XENTRY(MMX)                \
  DN_CPU_FEAT_XENTRY(MONITOR)            \
  DN_CPU_FEAT_XENTRY(MOVBE)              \
  DN_CPU_FEAT_XENTRY(MOVU)               \
  DN_CPU_FEAT_XENTRY(MmxExt)             \
  DN_CPU_FEAT_XENTRY(PCLMULQDQ)          \
  DN_CPU_FEAT_XENTRY(POPCNT)             \
  DN_CPU_FEAT_XENTRY(RDRAND)             \
  DN_CPU_FEAT_XENTRY(RDSEED)             \
  DN_CPU_FEAT_XENTRY(RDTSCP)             \
  DN_CPU_FEAT_XENTRY(SHA)                \
  DN_CPU_FEAT_XENTRY(SSE)                \
  DN_CPU_FEAT_XENTRY(SSE2)               \
  DN_CPU_FEAT_XENTRY(SSE3)               \
  DN_CPU_FEAT_XENTRY(SSE41)              \
  DN_CPU_FEAT_XENTRY(SSE42)              \
  DN_CPU_FEAT_XENTRY(SSE4A)              \
  DN_CPU_FEAT_XENTRY(SSSE3)              \
  DN_CPU_FEAT_XENTRY(TSC)                \
  DN_CPU_FEAT_XENTRY(TscInvariant)       \
  DN_CPU_FEAT_XENTRY(VAES)               \
  DN_CPU_FEAT_XENTRY(VPCMULQDQ)

enum DN_CPUFeature
{
  #define DN_CPU_FEAT_XENTRY(label) DN_CPUFeature_##label,
  DN_CPU_FEAT_XMACRO
  #undef DN_CPU_FEAT_XENTRY
  DN_CPUFeature_Count,
};

struct DN_CPUFeatureDecl
{
  DN_CPUFeature value;
  DN_Str8       label;
};

struct DN_CPUFeatureQuery
{
  DN_CPUFeature feature;
  bool          available;
};

struct DN_CPUReport
{
  char   vendor[4 /*bytes*/ * 3 /*EDX, ECX, EBX*/ + 1 /*null*/];
  char   brand[48];
  DN_U64 features[(DN_CPUFeature_Count / (sizeof(DN_U64) * 8)) + 1];
};

struct DN_TicketMutex
{
  unsigned int volatile ticket;  // The next ticket to give out to the thread taking the mutex
  unsigned int volatile serving; // The ticket ID to block the mutex on until it is returned
};


struct DN_Hex32    { char data[32 + 1];  DN_USize size; };
struct DN_Hex64    { char data[64 + 1];  DN_USize size; };
struct DN_Hex128   { char data[128 + 1]; DN_USize size; };

struct DN_HexU64
{
  char  data[(sizeof(DN_U64) * 2) + 1 /*null-terminator*/];
  DN_U8 size;
};

enum DN_HexFromU64Type
{
  DN_HexFromU64Type_Nil,
  DN_HexFromU64Type_Uppercase,
};

enum DN_TrimLeadingZero
{
  DN_TrimLeadingZero_No,
  DN_TrimLeadingZero_Yes,
};

struct DN_U8x16 { DN_U8 data[16]; };
struct DN_U8x32 { DN_U8 data[32]; };
struct DN_U8x64 { DN_U8 data[64]; };

DN_MSVC_WARNING_PUSH
DN_MSVC_WARNING_DISABLE(4201) // warning C4201: nonstandard extension used: nameless struct/union
union DN_V2USize
{
  struct { DN_USize x,     y; };
  struct { DN_USize w,     h; };
  struct { DN_USize min,   max; };
  struct { DN_USize begin, end; };
  DN_USize data[2];
};

union DN_V2U64
{
  struct { DN_U64 x,     y; };
  struct { DN_U64 w,     h; };
  struct { DN_U64 min,   max; };
  struct { DN_U64 begin, end; };
  DN_U64 data[2];
};
DN_MSVC_WARNING_POP

struct DN_CallSite
{
  DN_Str8 file;
  DN_Str8 function;
  DN_U32  line;
};

#define DN_CallSiteNow DN_Literal(DN_CallSite){DN_Str8Lit(__FILE__), DN_Str8Lit(__func__), __LINE__ }

#if defined(__cplusplus)
template <typename Procedure>
struct DN_Defer
{
  Procedure proc;
  DN_Defer(Procedure p) : proc(p) {}
  ~DN_Defer() { proc(); }
};

struct DN_DeferHelper
{
  template <typename Lambda>
  DN_Defer<Lambda> operator+(Lambda lambda) { return DN_Defer<Lambda>(lambda); };
};

#define DN_UniqueName(prefix) DN_TokenCombine(prefix, __LINE__)
#define DN_DEFER              const auto DN_UniqueName(defer_lambda_) = DN_DeferHelper() + [&]()
#endif // defined(__cplusplus)

#define DN_DeferLoop(begin, end)            \
  bool DN_UniqueName(once) = (begin, true); \
  DN_UniqueName(once);                      \
  end, DN_UniqueName(once) = false

struct DN_U64FromResult
{
  bool   success;
  DN_U64 value;
};

struct DN_USizeFromResult
{
  bool     success;
  DN_USize value;
};

struct DN_I64FromResult
{
  bool   success;
  DN_I64 value;
};

struct DN_U8x32FromResult
{
  bool   success;
  DN_U8x32 value;
};

struct DN_StackTraceFrame
{
  DN_U64  address;
  DN_U64  line_number;
  DN_Str8 file_name;
  DN_Str8 function_name;
};

struct DN_StackTraceFrameSlice
{
  DN_StackTraceFrame *data;
  DN_USize            count;
};

struct DN_StackTraceRawFrame
{
  void  *process;
  DN_U64 base_addr;
};

struct DN_StackTrace
{
  void   *process;   // [Internal] Windows handle to the process
  DN_U64 *base_addr; // The addresses of the functions in the stack trace
  DN_U16  size;      // The number of `base_addr`'s stored from the walk
};

struct DN_StackTraceIterator
{
  DN_StackTraceRawFrame raw_frame;
  DN_U16                index;
};

enum DN_MemCommit
{
  DN_MemCommit_No,
  DN_MemCommit_Yes,
};

typedef DN_U32 DN_MemPage;
enum DN_MemPage_
{
  // Exception on read/write with a page. This flag overrides the read/write
  // access.
  DN_MemPage_NoAccess = 1 << 0,

  DN_MemPage_Read = 1 << 1, // Only read permitted on the page.

  // Only write permitted on the page. On Windows this is not supported and
  // will be promoted to read+write permissions.
  DN_MemPage_Write = 1 << 2,

  DN_MemPage_ReadWrite = DN_MemPage_Read | DN_MemPage_Write,

  // Modifier used in conjunction with previous flags. Raises exception on
  // first access to the page, then, the underlying protection flags are
  // active. This is supported on Windows, on other OS's using this flag will
  // set the OS equivalent of DN_MemPage_NoAccess.
  // This flag must only be used in DN_Mem_Protect
  DN_MemPage_Guard = 1 << 3,

  // If leak tracing is enabled, this flag will allow the allocation recorded
  // from the reserve call to be leaked, e.g. not printed when leaks are
  // dumped to the console.
  DN_MemPage_AllocRecordLeakPermitted = 1 << 4,

  // If leak tracing is enabled this flag will prevent any allocation record
  // from being created in the allocation table at all. If this flag is
  // enabled, 'OSMemPage_AllocRecordLeakPermitted' has no effect since the
  // record will never be created.
  DN_MemPage_NoAllocRecordEntry = 1 << 5,

  // [INTERNAL] Do not use. All flags together do not constitute a correct
  // configuration of pages.
  DN_MemPage_All = DN_MemPage_NoAccess |
                     DN_MemPage_ReadWrite |
                     DN_MemPage_Guard |
                     DN_MemPage_AllocRecordLeakPermitted |
                     DN_MemPage_NoAllocRecordEntry,
};

#if !defined(DN_ARENA_RESERVE_SIZE)
  #define DN_ARENA_RESERVE_SIZE DN_Megabytes(64)
#endif

#if !defined(DN_ARENA_COMMIT_SIZE)
  #define DN_ARENA_COMMIT_SIZE DN_Kilobytes(64)
#endif

enum DN_MemFuncsType
{
  DN_MemFuncsType_Nil,
  DN_MemFuncsType_Heap,
  DN_MemFuncsType_Virtual,
};

typedef void *(DN_MemHeapAllocFunc)(DN_USize size);
typedef void  (DN_MemHeapDeallocFunc)(void *ptr);
typedef void *(DN_MemVirtualReserveFunc)(DN_USize size, DN_MemCommit commit, DN_MemPage page_flags);
typedef bool  (DN_MemVirtualCommitFunc)(void *ptr, DN_USize size, DN_U32 page_flags);
typedef void  (DN_MemVirtualReleaseFunc)(void *ptr, DN_USize size);
struct DN_MemFuncs
{
  DN_MemFuncsType        type;
  DN_MemHeapAllocFunc   *heap_alloc;
  DN_MemHeapDeallocFunc *heap_dealloc;

  DN_U32                     virtual_page_size;
  DN_MemVirtualReserveFunc  *virtual_reserve;
  DN_MemVirtualCommitFunc   *virtual_commit;
  DN_MemVirtualReleaseFunc  *virtual_release;
};

struct DN_MemBlock
{
  DN_MemBlock* prev;
  DN_U64       used;
  DN_U64       commit;
  DN_U64       reserve;
  DN_U64       reserve_sum;
};

struct DN_MemListInfo
{
  DN_U64 used;
  DN_U64 commit;
  DN_U64 reserve;
  DN_U64 blocks;
};

struct DN_MemStats
{
  DN_MemListInfo info;
  DN_MemListInfo hwm;
};

typedef DN_U32 DN_MemFlags;
enum DN_MemFlags_
{
  DN_MemFlags_Nil             = 0,
  DN_MemFlags_NoGrow          = 1 << 0,
  DN_MemFlags_NoPoison        = 1 << 1,
  DN_MemFlags_NoAllocTrack    = 1 << 2,
  DN_MemFlags_AllocCanLeak    = 1 << 3,
  DN_MemFlags_SimAlloc        = 1 << 4,

  // NOTE: Records stack traces of temp memory regions on construction to provide more diagnostics
  // when UAF violation occurs in the use of a region (e.g. nested regions A and B, with A
  // allocating whilst B is active would result in A's memory being wiped at the end of B). Tracing
  // has a heavy performance penalty as each scratch/temp memory region triggers and stores the
  // stack trace.
  //
  // Ignored if UAF guard is disabled at the preprocessor level
  // (e.g.: #define DN_ARENA_TEMP_MEM_UAF_GUARD 0)
  DN_MemFlags_TempMemUAFTrace        = 1 << 5,

  // NOTE: Forcibly disables TempMemUAFTrace for the arena irrespective of global settings. Globally
  // UAF tracing can be enabled across all arenas via the preprocessor which turns the tracing
  // feature (e.g.: #define DN_ARENA_TEMP_MEM_UAF_TRACE_ON_BY_DEFAULT 1) into an opt-out situation
  // where arenas have to specify this flag, specifically to not be traced.
  //
  // If both TempMemUAFTrace, TempMemUAFTraceDisable and or the global preprocessor flag is set
  // disabling takes precedence, always if it is set.
  DN_MemFlags_TempMemUAFTraceDisable = 1 << 6,

  // NOTE: Internal flags. Do not use
  DN_MemFlags_UserBuffer   = 1 << 7,
  DN_MemFlags_MemFuncs     = 1 << 8,
};

struct DN_MemList
{
  DN_MemBlock* curr;
  DN_MemFlags  flags;
  DN_MemFuncs  funcs;
  DN_MemStats  stats;
  DN_Str8      label;

  #if DN_ARENA_TEMP_MEM_UAF_GUARD
  DN_U32                 uaf_guard_next_id;
  DN_U32                 uaf_guard_active_id;
  struct DN_MemListTemp* uaf_guard_active_temp_mem;
  #endif
};

struct DN_MemListTemp
{
  DN_MemList*   mem;
  DN_U64        used_sum;
  #if DN_ARENA_TEMP_MEM_UAF_GUARD
  DN_StackTrace trace;
  #endif
};

enum DN_AllocatorType
{
  DN_AllocatorType_MemList,
  DN_AllocatorType_Arena,
  DN_AllocatorType_Pool,
};

struct DN_Allocator
{
  DN_AllocatorType type;
  void*            context;
};

enum DN_ArenaReset
{
  DN_ArenaReset_No,
  DN_ArenaReset_Yes,
};

typedef DN_U32 DN_ArenaFlags;
enum DN_ArenaFlags_
{
  DN_ArenaFlags_Nil = 0,
  DN_ArenaFlags_OwnsMemList = 1 << 0,
};

struct DN_Arena
{
  DN_ArenaFlags   flags;
  DN_MemList*     mem;
  #if DN_ARENA_TEMP_MEM_UAF_GUARD
  DN_U32          uaf_guard_id;
  DN_MemListTemp* uaf_guard_temp_mem;
  DN_U32          uaf_guard_prev_id;
  DN_MemListTemp* uaf_guard_prev_temp_mem;
  bool            uaf_guard_is_being_checked;
  #else
  DN_MemListTemp  temp_mem;
  #endif
};

DN_USize const DN_ARENA_HEADER_SIZE = DN_AlignUpPowerOfTwo(sizeof(DN_Arena), 64);

#if !defined(DN_POOL_DEFAULT_ALIGN)
  #define DN_POOL_DEFAULT_ALIGN 16
#endif

struct DN_PoolSlot
{
  void        *data;
  DN_PoolSlot *next;
};

enum DN_PoolSlotSize
{
  DN_PoolSlotSize_32B,
  DN_PoolSlotSize_64B,
  DN_PoolSlotSize_128B,
  DN_PoolSlotSize_256B,
  DN_PoolSlotSize_512B,
  DN_PoolSlotSize_1KiB,
  DN_PoolSlotSize_2KiB,
  DN_PoolSlotSize_4KiB,
  DN_PoolSlotSize_8KiB,
  DN_PoolSlotSize_16KiB,
  DN_PoolSlotSize_32KiB,
  DN_PoolSlotSize_64KiB,
  DN_PoolSlotSize_128KiB,
  DN_PoolSlotSize_256KiB,
  DN_PoolSlotSize_512KiB,
  DN_PoolSlotSize_1MiB,
  DN_PoolSlotSize_2MiB,
  DN_PoolSlotSize_4MiB,
  DN_PoolSlotSize_8MiB,
  DN_PoolSlotSize_16MiB,
  DN_PoolSlotSize_32MiB,
  DN_PoolSlotSize_64MiB,
  DN_PoolSlotSize_128MiB,
  DN_PoolSlotSize_256MiB,
  DN_PoolSlotSize_512MiB,
  DN_PoolSlotSize_1GiB,
  DN_PoolSlotSize_2GiB,
  DN_PoolSlotSize_4GiB,
  DN_PoolSlotSize_8GiB,
  DN_PoolSlotSize_16GiB,
  DN_PoolSlotSize_32GiB,
  DN_PoolSlotSize_Count,
};

struct DN_Pool
{
  DN_Arena    *arena;
  DN_PoolSlot *slots[DN_PoolSlotSize_Count];
  DN_U8        align;
};

struct DN_UTF8DecodeResult
{
  bool    success;
  DN_Str8 remaining;
  DN_U32  codepoint;
};

struct DN_UTF8DecodeIterator
{
  bool     init;
  bool     success;
  DN_Str8  remaining;
  DN_USize codepoint_index;
  DN_U32   codepoint;
};

typedef DN_U32 DN_CodepointCountFlags;
enum DN_CodepointCountFlags_
{
  DN_CodepointCountFlags_Nil          = 0,
  DN_CodepointCountFlags_SkipANSICode = 1 << 0,
};

struct DN_NibbleFromU8Result
{
  char nibble0;
  char nibble1;
};

enum DN_Str8EqCase
{
  DN_Str8EqCase_Sensitive,
  DN_Str8EqCase_Insensitive,
};

enum DN_Str8IsAllType
{
  DN_Str8IsAllType_Digits,
  DN_Str8IsAllType_Hex,
};

struct DN_Str8BSplitResult
{
  // If there are multiple strings passed to split against, this is the index into that array of
  // which the string was split on. If no array was passed this is always 0.
  DN_USize input_index;
  DN_Str8  lhs;
  DN_Str8  rhs;
};

struct DN_Str8FindResult
{
  bool     found;                        // True if string was found. If false, the subsequent fields below are not set.
  DN_USize index;                        // Index in the buffer where the found string starts
  DN_Str8  match;                        // Matching string in the buffer that was searched
  DN_Str8  match_to_end_of_buffer;       // Substring containing the found string to the end of the buffer
  DN_Str8  after_match_to_end_of_buffer; // Substring starting after the found string to the end of the buffer
  DN_Str8  start_to_before_match;        // Substring from the start of the buffer up until the found string, not including it
};

typedef DN_USize DN_Str8FindFlag;
enum DN_Str8FindFlag_
{
  DN_Str8FindFlag_Digit      = 1 << 0, // 0-9
  DN_Str8FindFlag_Whitespace = 1 << 1, // '\r', '\t', '\n', ' '
  DN_Str8FindFlag_Alphabet   = 1 << 2, // A-Z, a-z
  DN_Str8FindFlag_Plus       = 1 << 3, // +
  DN_Str8FindFlag_Minus      = 1 << 4, // -
  DN_Str8FindFlag_AlphaNum   = DN_Str8FindFlag_Alphabet | DN_Str8FindFlag_Digit,
};

typedef DN_USize DN_Str8SplitFlags;
enum DN_Str8SplitFlags_
{
  DN_Str8SplitFlags_Nil                 = 0,
  DN_Str8SplitFlags_ExcludeEmptyStrings = 1 << 0,
  DN_Str8SplitFlags_HandleQuotedStrings = 1 << 1,
};

struct DN_Str8TruncResult
{
  bool     truncated;
  DN_Str8  str8;
  DN_USize size_req; // Not including null-terminator
};

struct DN_Str8SplitResult
{
  DN_Str8 *data;
  DN_USize count;
};

enum DN_Str8LineBreakMode
{
  DN_Str8LineBreakMode_AtWord,  // Add delimiter to string at ' ' and '\n' boundaries
  DN_Str8LineBreakMode_AtWidth, // Add delimiter to string at width intervals
};

typedef DN_USize DN_Str8TableFlags;
enum DN_Str8TableFlags_
{
  DN_Str8TableFlags_None      = 0,
  DN_Str8TableFlags_HasHeader = 1 << 0,
  DN_Str8TableFlags_RowLines  = 1 << 1,
};

struct DN_Str8Link
{
  DN_Str8      string; // The string
  DN_Str8Link *next;   // The next string in the linked list
  DN_Str8Link *prev;   // The prev string in the linked list
};

struct DN_Str8Builder
{
  DN_Arena*    arena;       // Allocator to use to back the string list
  DN_Str8Link* head;        // First string in the linked list of strings
  DN_Str8Link* tail;        // Last string in the linked list of strings
  DN_USize     string_size; // The size in bytes necessary to construct the current string
  DN_USize     count;       // The number of links in the linked list of strings
};

enum DN_Str8BuilderAdd
{
  DN_Str8BuilderAdd_Append,
  DN_Str8BuilderAdd_Prepend,
};

typedef DN_U32 DN_AgeUnit;
enum DN_AgeUnit_
{
  DN_AgeUnit_Ms            = 1 << 0,
  DN_AgeUnit_Sec           = 1 << 1,
  DN_AgeUnit_Min           = 1 << 2,
  DN_AgeUnit_Hr            = 1 << 3,
  DN_AgeUnit_Day           = 1 << 4,
  DN_AgeUnit_Week          = 1 << 5,
  DN_AgeUnit_Year          = 1 << 6,
  DN_AgeUnit_FractionalSec = 1 << 7,
  DN_AgeUnit_HMS           = DN_AgeUnit_Sec | DN_AgeUnit_Min | DN_AgeUnit_Hr,
  DN_AgeUnit_All           = DN_AgeUnit_Ms | DN_AgeUnit_HMS | DN_AgeUnit_Day | DN_AgeUnit_Week | DN_AgeUnit_Year,
};

enum DN_ByteType
{
  DN_ByteType_B,
  DN_ByteType_KiB,
  DN_ByteType_MiB,
  DN_ByteType_GiB,
  DN_ByteType_TiB,
  DN_ByteType_Count,
  DN_ByteType_Auto,
};

struct DN_ByteCount
{
  DN_ByteType type;
  DN_Str8     suffix; // "KiB", "MiB", "GiB" .. e.t.c
  DN_F64      bytes;
};

struct DN_Date
{
  DN_U8  day;
  DN_U8  month;
  DN_U16 year;
  DN_U8  hour;
  DN_U8  minutes;
  DN_U8  seconds;
  DN_U16 milliseconds;
};

struct DN_FmtAppendResult
{
  DN_USize size_req;
  DN_Str8  str8;
  bool     truncated;
};

struct DN_ProfilerAnchor
{
  // Inclusive refers to the time spent to complete the function call
  // including all children functions.
  //
  // Exclusive refers to the time spent in the function, not including any
  // time spent in children functions that we call that are also being
  // profiled. If we recursively call into ourselves, the time we spent in
  // our function is accumulated.
  DN_U64  tsc_inclusive;
  DN_U64  tsc_exclusive;
  DN_U16  hit_count;
  DN_Str8 name;
};

struct DN_ProfilerZone
{
  struct DN_Profiler *profiler;
  DN_U16 anchor_index;
  DN_U64 begin_tsc;
  DN_U16 parent_zone;
  DN_U64 elapsed_tsc_at_zone_start;
};

struct DN_ProfilerAnchorArray
{
  DN_ProfilerAnchor *data;
  DN_USize           count;
};

typedef DN_U64 (DN_ProfilerTSCNowFunc)();
struct DN_Profiler
{
  DN_USize               frame_index;
  DN_ProfilerAnchor     *anchors;
  DN_USize               anchors_count;
  DN_USize               anchors_per_frame;
  DN_U16                 parent_zone;
  bool                   paused;
  DN_ProfilerTSCNowFunc *tsc_now;
  DN_U64                 tsc_frequency;
  DN_ProfilerZone        frame_zone;
  DN_F64                 frame_avg_tsc;
};

typedef bool (DN_QSortCompareFunc)(void const *a, void const *b, void *user_context);
enum DN_ErrSinkMode
{
  DN_ErrSinkMode_Nil,                  // Default behaviour to accumulate errors into the sink
  DN_ErrSinkMode_DebugBreakOnErrorLog, // Debug break (int3) when error is encountered and the sink is ended by the 'end and log' functions.
  DN_ErrSinkMode_ExitOnError,          // When an error is encountered, exit the program with the error code of the error that was caught.
};

struct DN_ErrSinkMsg
{
  DN_I32         error_code;
  DN_Str8        msg;
  DN_CallSite    call_site;
  DN_ErrSinkMsg *next;
  DN_ErrSinkMsg *prev;
};

struct DN_ErrSinkNode
{
  DN_CallSite    call_site;    // Call site that the node was created
  DN_ErrSinkMode mode;         // Controls how the sink behaves when an error is registered onto the sink.
  DN_ErrSinkMsg *msg_sentinel; // List of error messages accumulated for the current scope
  DN_U64         arena_pos;    // Position to reset the arena when the scope is ended
};

struct DN_ErrSink
{
  DN_Arena*      arena;      // Dedicated allocator from the thread's local storage
  DN_ErrSinkNode stack[128]; // Each entry contains errors accumulated between a [begin, end] region of the active sink.
  DN_USize       stack_size;
};

struct DN_TCScratch
{
  DN_Arena arena;
  DN_B32   destructed;
};

#if defined(__cplusplus)
struct DN_TCScratchCpp
{
  DN_TCScratchCpp(DN_Arena **conflicts, DN_USize count);
  ~DN_TCScratchCpp();
  DN_TCScratch data;
};
#endif

struct DN_TCInitArgs
{
  DN_U64 main_reserve;
  DN_U64 main_commit;
  DN_U64 temp_reserve;
  DN_U64 temp_commit;
  DN_U64 temp_count;
  DN_U64 err_sink_reserve;
  DN_U64 err_sink_commit;
};

struct DN_TCCore // (T)hread (C)ontext sitting in thread-local storage
{
  DN_Str8x64  name;
  DN_U64      thread_id;
  DN_CallSite call_site;
  char        lane_opaque[sizeof(DN_U64) * 4];
  void*       user_context;

  DN_MemList  main_arena_mem_;
  DN_MemList  temp_arena_mems_[4];
  DN_MemList  err_sink_arena_mem_;

  DN_Arena    main_arena_;
  DN_Arena    temp_arenas_[4];
  DN_Arena    err_sink_arena_;

  DN_Arena*   main_arena;
  DN_Pool     main_pool;
  DN_Arena*   temp_arenas[4];
  DN_USize    temp_arenas_count;

  DN_ErrSink  err_sink;

  DN_Arena*   frame_arena;
};

enum DN_TCDeinitArenas
{
  DN_TCDeinitArenas_No,
  DN_TCDeinitArenas_Yes,
};

struct DN_PCG32       { DN_U64 state; };
struct DN_MurmurHash3 { DN_U64 e[2]; };

enum DN_LogType
{
  DN_LogType_Debug,
  DN_LogType_Info,
  DN_LogType_Warning,
  DN_LogType_Error,
  DN_LogType_Count,
};

enum DN_LogBold
{
  DN_LogBold_No,
  DN_LogBold_Yes,
};

struct DN_LogStyle
{
  DN_LogBold bold;
  bool       colour;
  DN_U8      r, g, b;
};

struct DN_LogTypeParam
{
  bool    is_u32_enum;
  DN_U32  u32;
  DN_Str8 str8;
};

enum DN_ANSIColourMode
{
  DN_ANSIColourMode_Fg,
  DN_ANSIColourMode_Bg,
};

struct DN_LogDate
{
  DN_U16 year;
  DN_U8  month;
  DN_U8  day;

  DN_U8 hour;
  DN_U8 minute;
  DN_U8 second;
};

struct DN_LogPrefixSize
{
  DN_USize size;
  DN_USize padding;
};

typedef DN_U32 DN_LogFlags;
enum DN_LogFlags_
{
  DN_LogFlags_Nil       = 0,
  DN_LogFlags_NoNewLine = 1 << 0,
  DN_LogFlags_NoPrefix  = 1 << 1,
};

typedef void DN_LogPrintFunc(DN_LogTypeParam type, void *user_data, DN_CallSite call_site, DN_LogFlags flags, DN_FMT_ATTRIB char const *fmt, va_list args);

DN_MSVC_WARNING_PUSH
DN_MSVC_WARNING_DISABLE(4201) // warning C4201: nonstandard extension used: nameless struct/union
union DN_V2I32
{
  struct { DN_I32 x, y; };
  struct { DN_I32 w, h; };
  DN_I32 data[2];
};

union DN_V2U16
{
  struct { DN_U16 x, y; };
  struct { DN_U16 w, h; };
  DN_U16 data[2];
};

union DN_V2U32
{
  struct { DN_U32 x,   y; };
  struct { DN_U32 w,   h; };
  struct { DN_U32 min, max; };
  DN_U32 data[2];
};

union DN_V2F32
{
  struct { DN_F32 x, y; };
  struct { DN_F32 w, h; };
  DN_F32 data[2];
};

struct DN_2V2F32
{
  DN_V2F32 min;
  DN_V2F32 max;
};

struct DN_V2F32Array
{
  DN_V2F32 *data;
  DN_USize  count;
  DN_USize  max;
};

union DN_V3F32
{
  struct { DN_F32 x, y, z; };
  struct { DN_F32 r, g, b; };
  DN_V2F32  xy;
  DN_F32    data[3];
};

union DN_V4F32
{
  struct { DN_F32 x, y, z, w; };
  struct { DN_F32 r, g, b, a; };
  DN_V3F32  rgb;
  DN_V3F32  xyz;
  DN_F32    data[4];
};

struct DN_V4F32Array
{
  DN_V4F32* data;
  DN_USize  count;
  DN_USize  max;
};
DN_MSVC_WARNING_POP

struct DN_M4
{
  DN_F32 columns[4][4]; // Column major matrix
};

union DN_M2x3
{
  DN_F32 e[6];
  DN_F32 row[2][3];
};

struct DN_M2x3XForm
{
  DN_M2x3 forward;
  DN_M2x3 inverse;
};

enum DN_M2x3ProjOrigin
{
  DN_M2x3ProjOrigin_TopLeft,
  DN_M2x3ProjOrigin_Center,
};

struct DN_Rect
{
  DN_V2F32 pos, size;
};

enum DN_RectCutClip
{
  DN_RectCutClip_No,
  DN_RectCutClip_Yes,
};

enum DN_RectCutSide
{
  DN_RectCutSide_Left,
  DN_RectCutSide_Right,
  DN_RectCutSide_Top,
  DN_RectCutSide_Bottom,
};

struct DN_RectCut
{
  DN_Rect*       rect;
  DN_RectCutSide side;
};

struct DN_RaycastV2
{
  bool   hit; // True if there was an intersection, false if the lines are parallel
  DN_F32 t_a; // Distance along `dir_a` that the intersection occurred, e.g. `origin_a + (dir_a * t_a)`
  DN_F32 t_b; // Distance along `dir_b` that the intersection occurred, e.g. `origin_b + (dir_b * t_b)`
};

struct DN_Ring
{
  DN_U64 size;
  char  *base;
  DN_U64 write_pos;
  DN_U64 read_pos;
};

enum DN_ArrayErase
{
  DN_ArrayErase_Unstable,
  DN_ArrayErase_Stable,
};

enum DN_ArrayAdd
{
  DN_ArrayAdd_Append,
  DN_ArrayAdd_Prepend,
};

struct DN_ArrayEraseResult
{
  // The next index your for-index should be set to such that you can continue
  // to iterate the remainder of the array, e.g:
  //
  // for (DN_USize index = 0; index < array.size; index++) {
  //     if (erase)
  //          index = DN_FArray_EraseRange(&array, index, -3, DN_ArrayErase_Unstable);
  // }
  DN_USize it_index;
  DN_USize items_erased; // The number of items erased
};

struct DN_ArrayFindResult
{
  bool     success;
  DN_USize index;
  void    *value;
};
typedef bool (DN_ArrayFindEqFunc)(void const *lhs, void const *find);

enum DN_DSMapKeyType
{
                                     // Key    | Key Hash         | Map Index
  DN_DSMapKeyType_Invalid,
  DN_DSMapKeyType_U64,               // U64    | Hash(U64)        | Hash(U64)        % map_size
  DN_DSMapKeyType_U64NoHash,         // U64    | U64              | U64              % map_size
  DN_DSMapKeyType_Buffer,            // Buffer | Hash(buffer)     | Hash(buffer)     % map_size
  DN_DSMapKeyType_BufferAsU64NoHash, // Buffer | U64(buffer[0:4]) | U64(buffer[0:4]) % map_size
};

struct DN_DSMapKey
{
  DN_DSMapKeyType  type;
  DN_U32           hash; // Hash to lookup in the map. If it equals, we check that the original key payload matches
  void const      *buffer_data;
  DN_U32           buffer_size;
  DN_U64           u64;
  bool             no_copy_buffer;
};

template <typename T>
struct DN_DSMapSlot
{
  DN_DSMapKey key;   // Hash table lookup key
  T           value; // Hash table value
};

typedef DN_U32 DN_DSMapFlags;
enum DN_DSMapFlags_
{
  DN_DSMapFlags_Nil                   = 0,
  DN_DSMapFlags_DontFreeArenaOnResize = 1 << 0,
};

using DN_DSMapHashFunction = DN_U32(DN_DSMapKey key, DN_U32 seed);
template <typename T> struct DN_DSMap
{
  DN_U32               *hash_to_slot;  // Mapping from hash to a index in the slots array
  DN_DSMapSlot<T>      *slots;         // Values of the array stored contiguously, non-sorted order
  DN_U32                size;          // Total capacity of the map and is a power of two
  DN_U32                occupied;      // Number of slots used in the hash table
  DN_Arena             *arena;         // Backing arena for the hash table
  DN_Pool               pool;          // Allocator for keys that are variable-sized buffers
  DN_U32                initial_size;  // Initial map size, map cannot shrink on erase below this size
  DN_DSMapHashFunction *hash_function; // Custom hashing function to use if field is set
  DN_U32                hash_seed;     // Seed for the hashing function, when 0, DN_DS_MAP_DEFAULT_HASH_SEED is used
  DN_DSMapFlags         flags;
};

template <typename T> struct DN_DSMapResult
{
  bool             found;
  DN_DSMapSlot<T> *slot;
  T               *value;
};

enum DN_LeakAllocFlag
{
  DN_LeakAllocFlag_Freed         = 1 << 0,
  DN_LeakAllocFlag_LeakPermitted = 1 << 1,
};

struct DN_LeakAlloc
{
  void     *ptr;               // 8  Pointer to the allocation being tracked
  DN_USize  size;              // 16 Size of the allocation
  DN_USize  freed_size;        // 24 Store the size of the allocation when it is freed
  DN_Str8   stack_trace;       // 40 Stack trace at the point of allocation
  DN_Str8   freed_stack_trace; // 56 Stack trace of where the allocation was freed
  DN_U16    flags;             // 72 Bit flags from `DN_LeakAllocFlag`
};

// NOTE: We aim to keep the allocation record as light as possible as memory tracking can get
// expensive. Enforce that there is no unexpected padding.
DN_StaticAssert(sizeof(DN_LeakAlloc) == 64 || sizeof(DN_LeakAlloc) == 32); // NOTE: 64 bit vs 32 bit pointers respectively

struct DN_LeakTracker
{
  DN_DSMap<DN_LeakAlloc> alloc_table;
  DN_TicketMutex         alloc_table_mutex;
  DN_MemList             alloc_table_mem;
  DN_Arena               alloc_table_arena;
  DN_U64                 alloc_table_bytes_allocated_for_stack_traces;
};

typedef DN_USize DN_InitFlags;
enum DN_InitFlags_
{
  DN_InitFlags_Nil            = 0,

  // NOTE: Query the OS for information and enable functionality that requires the OS (like virtual
  // memory APIs, mutexes, secure RNG) as well as per-thread persistent and scratch allocators.
  DN_InitFlags_OS             = (1 << 0),
  DN_InitFlags_LeakTracker    = (1 << 1) | DN_InitFlags_OS,
  DN_InitFlags_LogLibFeatures = (1 << 2),
  DN_InitFlags_LogCPUFeatures = (1 << 3) | DN_InitFlags_OS,
  DN_InitFlags_LogAllFeatures = DN_InitFlags_LogLibFeatures | DN_InitFlags_LogCPUFeatures,
};

#if !defined(DN_STB_SPRINTF_HEADER_ONLY)
  #define STB_SPRINTF_IMPLEMENTATION
  #define STB_SPRINTF_STATIC
#endif

DN_MSVC_WARNING_PUSH
DN_MSVC_WARNING_DISABLE(4505) // Unused function warning
DN_GCC_WARNING_PUSH
DN_GCC_WARNING_DISABLE(-Wunused-function)
// NOTE: This depends on DN_Str8 because we've customised the library
#include "External/stb_sprintf.h"
DN_GCC_WARNING_POP
DN_MSVC_WARNING_POP

#if DN_WITH_OS
#include <new> // operator new

#if !defined(DN_OS_WIN32) || defined(DN_OS_WIN32_USE_PTHREADS)
  #include <pthread.h>
  #include <semaphore.h>
#endif

#if defined(DN_PLATFORM_POSIX) || defined(DN_PLATFORM_EMSCRIPTEN)
  #include <errno.h>      // errno
  #include <fcntl.h>      // O_RDONLY ... etc
  #include <sys/ioctl.h>  // ioctl
  #include <sys/mman.h>   // mmap
  #include <sys/random.h> // getrandom
  #include <sys/stat.h>   // stat
  #include <sys/types.h>  // pid_t
  #include <sys/wait.h>   // waitpid
  #include <time.h>       // clock_gettime, nanosleep
  #include <unistd.h>     // access, gettid, write

  #if !defined(DN_PLATFORM_EMSCRIPTEN)
    #include <linux/fs.h>     // FICLONE
    #include <sys/sendfile.h> // sendfile
  #endif
#endif

extern DN_CPUFeatureDecl g_dn_cpu_feature_decl[DN_CPUFeature_Count];
struct DN_OSTimer /// Record time between two time-points using the OS's performance counter.
{
  DN_U64 start;
  DN_U64 end;
};

// NOTE: DN_OSFile
enum DN_OSPathInfoType
{
  DN_OSPathInfoType_Unknown,
  DN_OSPathInfoType_Directory,
  DN_OSPathInfoType_File,
};

struct DN_OSPathInfo
{
  bool              exists;
  DN_OSPathInfoType type;
  DN_U64            create_time_in_s;
  DN_U64            last_write_time_in_s;
  DN_U64            last_access_time_in_s;
  DN_U64            size;
};

struct DN_OSDirIterator
{
  void   *handle;
  DN_Str8 file_name;
  char    buffer[512];
};

// NOTE: R/W Stream API
struct DN_OSFileRead
{
  bool     success;
  DN_USize bytes_read;
};

struct DN_OSFile
{
  bool  error;
  void *handle;
};

enum DN_OSFileOpen
{
  DN_OSFileOpen_CreateAlways, // Create file if it does not exist, otherwise, zero out the file and open
  DN_OSFileOpen_OpenIfExist,  // Open file at path only if it exists
  DN_OSFileOpen_OpenAlways,   // Open file at path, create file if it does not exist
};

typedef DN_U32 DN_OSFileAccess;

enum DN_OSFileAccess_
{
  DN_OSFileAccess_Read       = 1 << 0,
  DN_OSFileAccess_Write      = 1 << 1,
  DN_OSFileAccess_Execute    = 1 << 2,
  DN_OSFileAccess_AppendOnly = 1 << 3, // This flag cannot be combined with any other access mode
  DN_OSFileAccess_ReadWrite  = DN_OSFileAccess_Read | DN_OSFileAccess_Write,
  DN_OSFileAccess_All        = DN_OSFileAccess_ReadWrite | DN_OSFileAccess_Execute | DN_OSFileAccess_AppendOnly,
};

// NOTE: DN_OSPath
#if !defined(DN_OSPathSeperator)
  #if defined(DN_OS_WIN32)
    #define DN_OSPathSeperator "\\"
  #else
    #define DN_OSPathSeperator "/"
  #endif
  #define DN_OSPathSeperatorString DN_Str8Lit(DN_OSPathSeperator)
#endif

struct DN_OSPathLink
{
  DN_Str8        string;
  DN_OSPathLink *next;
  DN_OSPathLink *prev;
};

struct DN_OSPath
{
  bool           has_prefix_path_separator;
  DN_OSPathLink *head;
  DN_OSPathLink *tail;
  DN_USize       string_size;
  DN_U16         links_size;
};

// NOTE: DN_OSExec
typedef DN_U32 DN_OSExecFlags;

enum DN_OSExecFlags_
{
  DN_OSExecFlags_Nil                 = 0,
  DN_OSExecFlags_SaveStdout          = 1 << 0,
  DN_OSExecFlags_SaveStderr          = 1 << 1,
  DN_OSExecFlags_SaveOutput          = DN_OSExecFlags_SaveStdout | DN_OSExecFlags_SaveStderr,
  DN_OSExecFlags_MergeStderrToStdout = 1 << 2 | DN_OSExecFlags_SaveOutput,
};

struct DN_OSExecAsyncHandle
{
  DN_OSExecFlags exec_flags;
  DN_U32       os_error_code;
  DN_U32       exit_code;
  void          *process;
  void          *stdout_read;
  void          *stdout_write;
  void          *stderr_read;
  void          *stderr_write;
};

struct DN_OSExecResult
{
  bool    finished;
  DN_Str8 stdout_text;
  DN_Str8 stderr_text;
  DN_U32  os_error_code;
  DN_U32  exit_code;
};

struct DN_OSExecArgs
{
  DN_OSExecFlags flags;
  DN_Str8        working_dir;
  DN_Str8Slice   environment;
};

// NOTE: DN_OSSemaphore
DN_U32 const DN_OS_SEMAPHORE_INFINITE_TIMEOUT = UINT32_MAX;

struct DN_OSSemaphore
{
  DN_U64 handle;
};

struct DN_OSBarrier
{
  DN_U64 handle;
};

enum DN_OSSemaphoreWaitResult
{
  DN_OSSemaphoreWaitResult_Failed,
  DN_OSSemaphoreWaitResult_Success,
  DN_OSSemaphoreWaitResult_Timeout,
};

struct DN_OSMutex
{
  DN_U64 handle;
};

struct DN_OSConditionVariable
{
  DN_U64 handle;
};

// NOTE: DN_OSThread
typedef DN_I32(DN_OSThreadFunc)(struct DN_OSThread *);
struct DN_OSThreadLane
{
  DN_USize     index;
  DN_USize     count;
  DN_OSBarrier barrier;
  void*        shared_mem;
};

struct DN_OSThreadLaneway
{
  DN_OSThread* threads;
  DN_USize     threads_count;
  DN_UPtr*     shared_mem;
  DN_OSBarrier barrier;
};

struct DN_OSThread
{
  DN_Str8x64       name;
  DN_TCCore        context;
  DN_OSThreadLane  lane;
  bool             is_lane_set;
  void            *handle;
  DN_U64           thread_id;
  void            *user_context;
  DN_OSThreadFunc *func;
  DN_OSSemaphore   init_semaphore;
  DN_TCInitArgs    tc_init_args;
};

struct DN_OSCore
{
  DN_CPUReport           cpu_report;

  // NOTE: Logging
  bool                   log_to_file;              // Output logs to file as well as standard out
  DN_OSFile              log_file;                 // TODO(dn): Hmmm, how should we do this... ?
  DN_TicketMutex         log_file_mutex;           // Is locked when instantiating the log_file for the first time
  bool                   log_no_colour;            // Disable colours in the logging output
  DN_TicketMutex         log_mutex;

  // NOTE: OS
  DN_U32                 logical_processor_count;
  DN_U32                 page_size;
  DN_U32                 alloc_granularity;

  // NOTE: Memory
  // Total OS mem allocs in lifetime of program (e.g. malloc, VirtualAlloc, HeapAlloc ...). This
  // only includes allocations routed through the library such as the growing nature of arenas or
  // using the memory allocation routines in the library like DN_OS_MemCommit and so forth.
  DN_U64                 vmem_allocs_total;
  DN_U64                 vmem_allocs_frame;        // Total OS virtual memory allocs since the last 'DN_Core_FrameBegin' was invoked
  DN_U64                 mem_allocs_total;
  DN_U64                 mem_allocs_frame;         // Total OS heap allocs since the last 'DN_Core_FrameBegin' was invoked

  DN_MemList             mem;
  DN_Arena               arena;
  void*                  platform_context;
};

struct DN_OSDiskSpace
{
  bool   success;
  DN_U64 avail;
  DN_U64 size;
};

enum DN_OSAsyncPriority
{
  DN_OSAsyncPriority_Low,
  DN_OSAsyncPriority_High,
  DN_OSAsyncPriority_Count,
};

struct DN_OSAsyncCore
{
  DN_OSMutex             ring_mutex;
  DN_OSConditionVariable ring_write_cv;
  DN_OSSemaphore         worker_sem;
  DN_Ring                ring;
  DN_OSThread           *threads;
  DN_U32                 thread_count;
  DN_U32                 busy_threads;
  DN_U32                 join_threads;
};

struct DN_OSAsyncWorkArgs
{
  DN_OSThread *thread;
  void        *input;
};

typedef void(DN_OSAsyncWorkFunc)(DN_OSAsyncWorkArgs work_args);

struct DN_OSAsyncWork
{
  DN_OSAsyncWorkFunc *func;
  void             *input;
  void             *output;
};

struct DN_OSAsyncTask
{
  bool           queued;
  DN_OSAsyncWork   work;
  DN_OSSemaphore completion_sem;
};
#endif // #if DN_WITH_OS

struct DN_Core
{
  DN_InitFlags     init_flags;
  DN_TCCore        main_tc;
  DN_USize         mem_allocs_frame;
  DN_LeakTracker   leak;

  DN_LogType       log_level_to_show_from;
  DN_LogPrintFunc* print_func;
  void*            print_func_context;

  bool             os_init;
  #if DN_WITH_OS
  DN_OSCore        os;
  #endif
};

// NOTE: Library initialisation. This must be called before using the library once to setup TLS and
// query the OS for information (such as page size) for tuning allocations and so forth. The caller
// should pass in a zero-initialised `DN_Core` that should persist for program lifetime.
//
// A reference to the core passed in is kept which can be queried with `DN_Get` and may be used
// internally by the library. If you have an application that has the concept of frames, you may
// optionally call `DN_BeginFrame` which resets some metrics that are counted for example it tracks
// the number of memory allocations for the current frame and that counter can be reset.
DN_API void                     DN_Init                                                (DN_Core *dn, DN_InitFlags flags, DN_TCInitArgs args);
DN_API void                     DN_Set                                                 (DN_Core *dn);
DN_API DN_Core*                 DN_Get                                                 ();
DN_API void                     DN_BeginFrame                                          ();

DN_API bool                     DN_VerifyArgsF                                         (DN_VerifyType type, bool expr, DN_CallSite call_site, DN_Str8 expr_str8, char const *fmt, ...);
DN_API bool                     DN_VerifyArgs                                          (DN_VerifyType type, bool expr, DN_CallSite call_site, DN_Str8 expr_str8);

#define                         DN_SPrintF(...)                                        STB_SPRINTF_DECORATE(sprintf)(__VA_ARGS__)
#define                         DN_SNPrintF(...)                                       STB_SPRINTF_DECORATE(snprintf)(__VA_ARGS__)
#define                         DN_VSPrintF(...)                                       STB_SPRINTF_DECORATE(vsprintf)(__VA_ARGS__)
#define                         DN_VSNPrintF(...)                                      STB_SPRINTF_DECORATE(vsnprintf)(__VA_ARGS__)

DN_API bool                     DN_MemStartsWith                                       (void const *lhs, DN_USize lhs_size, void const *rhs, DN_USize rhs_size);
DN_API bool                     DN_MemEq                                               (void const *lhs, DN_USize lhs_size, void const *rhs, DN_USize rhs_size);
DN_API bool                     DN_MemEqUnsafe                                         (void const *lhs, void const *rhs, DN_USize size);
#if defined(__cplusplus)
template <typename T> T*        DN_MemCopyObjT                                         (T *dest, T const *src, DN_USize count);
#define                         DN_MemCopyObj(dest, src, count)                        DN_MemCopyObjT(dest, src, count)
#else
#define                         DN_MemCopyObj(dest, src, count)                        DN_Memcpy(dest, src, sizeof(*src) * count)
#endif

DN_API DN_U64                   DN_AtomicSetValue64                                    (DN_U64 volatile *target, DN_U64 value);
DN_API DN_U32                   DN_AtomicSetValue32                                    (DN_U32 volatile *target, DN_U32 value);
DN_API DN_USize                 DN_AlignUpPowerOfTwoUSize                              (DN_USize val);
DN_API DN_U64                   DN_AlignUpPowerOfTwoU64                                (DN_U64 val);
DN_API DN_U32                   DN_AlignUpPowerOfTwoU32                                (DN_U32 val);

DN_API void                     DN_ByteSwapU64Ptr                                      (DN_U8* dest, DN_U64 src);
#define                         DN_ByteSwap64(val)                                     ( \
                                                                                        ((((DN_U64)(val) >> 56) & 0xFF) <<  0) | \
                                                                                        ((((DN_U64)(val) >> 48) & 0xFF) <<  8) | \
                                                                                        ((((DN_U64)(val) >> 40) & 0xFF) << 16) | \
                                                                                        ((((DN_U64)(val) >> 32) & 0xFF) << 24) | \
                                                                                        ((((DN_U64)(val) >> 24) & 0xFF) << 32) | \
                                                                                        ((((DN_U64)(val) >> 16) & 0xFF) << 40) | \
                                                                                        ((((DN_U64)(val) >> 8)  & 0xFF) << 48) | \
                                                                                        ((((DN_U64)(val) >> 0)  & 0xFF) << 56)   \
                                                                                       )
#define                         DN_ByteSwap32(val)                                     ( \
                                                                                        ((((DN_U32)(val) >> 24) & 0xFF) << 0)  | \
                                                                                        ((((DN_U32)(val) >> 16) & 0xFF) << 8)  | \
                                                                                        ((((DN_U32)(val) >> 8)  & 0xFF) << 16) | \
                                                                                        ((((DN_U32)(val) >> 0)  & 0xFF) << 24)   \
                                                                                       )
#define                         DN_ByteSwap24(val)                                     ( \
                                                                                        ((((DN_U32)(val) >> 16) & 0xFF) << 0)  | \
                                                                                        ((((DN_U32)(val) >> 8)  & 0xFF) << 8)  | \
                                                                                        ((((DN_U32)(val) >> 0)  & 0xFF) << 16)   \
                                                                                       )
#define                         DN_ByteSwap16(val)                                     ( \
                                                                                        ((((DN_U16)(val) >> 8)  & 0xFF) << 0) | \
                                                                                        ((((DN_U16)(val) >> 0)  & 0xFF) << 8)   \
                                                                                       )
#if defined(DN_64_BIT)
  #define                       DN_ByteSwapUSize(val)                                  DN_ByteSwap64(val)
#else
  #define                       DN_ByteSwapUSize(val)                                  DN_ByteSwap32(val)
#endif

DN_API DN_CPUIDResult           DN_CPUID                                               (DN_CPUIDArgs args);
DN_API DN_USize                 DN_CPUHasFeatureArray                                  (DN_CPUReport const *report, DN_CPUFeatureQuery *features, DN_USize features_size);
DN_API bool                     DN_CPUHasFeature                                       (DN_CPUReport const *report, DN_CPUFeature feature);
DN_API bool                     DN_CPUHasAllFeatures                                   (DN_CPUReport const *report, DN_CPUFeature const *features, DN_USize features_size);
DN_API void                     DN_CPUSetFeature                                       (DN_CPUReport *report, DN_CPUFeature feature);
DN_API DN_CPUReport             DN_CPUGetReport                                        ();

DN_API void                     DN_TicketMutex_Begin                                   (DN_TicketMutex *mutex);
DN_API void                     DN_TicketMutex_End                                     (DN_TicketMutex *mutex);
DN_API DN_UInt                  DN_TicketMutex_MakeTicket                              (DN_TicketMutex *mutex);
DN_API void                     DN_TicketMutex_BeginTicket                             (DN_TicketMutex const *mutex, DN_UInt ticket);
DN_API bool                     DN_TicketMutex_CanLock                                 (DN_TicketMutex const *mutex, DN_UInt ticket);

DN_API void                     DN_BitUnsetInplace                                     (DN_USize *flags, DN_USize bitfield);
DN_API void                     DN_BitSetInplace                                       (DN_USize *flags, DN_USize bitfield);
DN_API bool                     DN_BitIsSet                                            (DN_USize bits, DN_USize bits_to_set);
DN_API bool                     DN_BitIsNotSet                                         (DN_USize bits, DN_USize bits_to_check);
DN_API bool                     DN_BitIsAny                                            (DN_USize bits, DN_USize bits_to_check);
#define                         DN_BitClearNextLSB(value)                              (value) & ((value) - 1)

DN_API DN_I64                   DN_SafeAddI64                                          (DN_I64 a,  DN_I64 b);
DN_API DN_I64                   DN_SafeMulI64                                          (DN_I64 a,  DN_I64 b);

DN_API DN_U64                   DN_SafeAddU64                                          (DN_U64 a, DN_U64 b);
DN_API DN_U64                   DN_SafeMulU64                                          (DN_U64 a, DN_U64 b);

DN_API DN_U64                   DN_SafeSubU64                                          (DN_U64 a, DN_U64 b);
DN_API DN_U32                   DN_SafeSubU32                                          (DN_U32 a, DN_U32 b);

DN_API int                      DN_SaturateCastUSizeToInt                              (DN_USize val);
DN_API DN_I8                    DN_SaturateCastUSizeToI8                               (DN_USize val);
DN_API DN_I16                   DN_SaturateCastUSizeToI16                              (DN_USize val);
DN_API DN_I32                   DN_SaturateCastUSizeToI32                              (DN_USize val);
DN_API DN_I64                   DN_SaturateCastUSizeToI64                              (DN_USize val);

DN_API int                      DN_SaturateCastU64ToInt                                (DN_U64 val);
DN_API DN_I8                    DN_SaturateCastU8ToI8                                  (DN_U64 val);
DN_API DN_I16                   DN_SaturateCastU16ToI16                                (DN_U64 val);
DN_API DN_I32                   DN_SaturateCastU32ToI32                                (DN_U64 val);
DN_API DN_I64                   DN_SaturateCastU64ToI64                                (DN_U64 val);
DN_API DN_UInt                  DN_SaturateCastU64ToUInt                               (DN_U64 val);
DN_API DN_U8                    DN_SaturateCastU64ToU8                                 (DN_U64 val);
DN_API DN_U16                   DN_SaturateCastU64ToU16                                (DN_U64 val);
DN_API DN_U32                   DN_SaturateCastU64ToU32                                (DN_U64 val);

DN_API DN_U8                    DN_SaturateCastUSizeToU8                               (DN_USize val);
DN_API DN_U16                   DN_SaturateCastUSizeToU16                              (DN_USize val);
DN_API DN_U32                   DN_SaturateCastUSizeToU32                              (DN_USize val);
DN_API DN_U64                   DN_SaturateCastUSizeToU64                              (DN_USize val);

DN_API int                      DN_SaturateCastISizeToInt                              (DN_ISize val);
DN_API DN_I8                    DN_SaturateCastISizeToI8                               (DN_ISize val);
DN_API DN_I16                   DN_SaturateCastISizeToI16                              (DN_ISize val);
DN_API DN_I32                   DN_SaturateCastISizeToI32                              (DN_ISize val);
DN_API DN_I64                   DN_SaturateCastISizeToI64                              (DN_ISize val);

DN_API DN_UInt                  DN_SaturateCastISizeToUInt                             (DN_ISize val);
DN_API DN_U8                    DN_SaturateCastISizeToU8                               (DN_ISize val);
DN_API DN_U16                   DN_SaturateCastISizeToU16                              (DN_ISize val);
DN_API DN_U32                   DN_SaturateCastISizeToU32                              (DN_ISize val);
DN_API DN_U64                   DN_SaturateCastISizeToU64                              (DN_ISize val);

DN_API DN_ISize                 DN_SaturateCastI64ToISize                              (DN_I64 val);
DN_API DN_I8                    DN_SaturateCastI64ToI8                                 (DN_I64 val);
DN_API DN_I16                   DN_SaturateCastI64ToI16                                (DN_I64 val);
DN_API DN_I32                   DN_SaturateCastI64ToI32                                (DN_I64 val);

DN_API DN_UInt                  DN_SaturateCastI64ToUInt                               (DN_I64 val);
DN_API DN_USize                 DN_SaturateCastI64ToUSize                              (DN_I64 val);
DN_API DN_U8                    DN_SaturateCastI64ToU8                                 (DN_I64 val);
DN_API DN_U16                   DN_SaturateCastI64ToU16                                (DN_I64 val);
DN_API DN_U32                   DN_SaturateCastI64ToU32                                (DN_I64 val);
DN_API DN_U64                   DN_SaturateCastI64ToU64                                (DN_I64 val);

DN_API DN_I8                    DN_SaturateCastIntToI8                                 (int val);
DN_API DN_I16                   DN_SaturateCastIntToI16                                (int val);
DN_API DN_U8                    DN_SaturateCastIntToU8                                 (int val);
DN_API DN_U16                   DN_SaturateCastIntToU16                                (int val);
DN_API DN_U32                   DN_SaturateCastIntToU32                                (int val);
DN_API DN_U64                   DN_SaturateCastIntToU64                                (int val);

DN_API void                     DN_ASanPoisonMemoryRegion                              (void const volatile *ptr, DN_USize size);
DN_API void                     DN_ASanUnpoisonMemoryRegion                            (void const volatile *ptr, DN_USize size);

DN_API DN_F32                   DN_EpsilonClampF32                                     (DN_F32 value, DN_F32 target, DN_F32 epsilon);

DN_API DN_MemStats              DN_MemStatsSum                                         (DN_MemStats lhs, DN_MemStats rhs);
DN_API DN_MemStats              DN_MemStatsSumArray                                    (DN_MemStats const *array, DN_USize size);

// NOTE: MemList
// Overview
//   `MemList` is an implementation of a classical `Arena` (e.g. bump allocator, can dynamically
//   grow, frees by bumping pointer back, sub-divides a block of memory). The term `Arena` is
//   reserved as a thin-layer over the functionality here to provide some use-after-free protection.
//   See `Arena` for more info.
DN_API DN_MemList               DN_MemListFromBuffer                                   (void *buffer, DN_USize size, DN_MemFlags flags);
DN_API DN_MemList               DN_MemListFromMemFuncs                                 (DN_U64 reserve, DN_U64 commit, DN_MemFlags flags, DN_MemFuncs mem_funcs);
DN_API void                     DN_MemListDeinit                                       (DN_MemList *mem);
DN_API bool                     DN_MemListCommit                                       (DN_MemList *mem, DN_U64 size);
DN_API bool                     DN_MemListCommitTo                                     (DN_MemList *mem, DN_U64 pos);
DN_API bool                     DN_MemListGrow                                         (DN_MemList *mem, DN_U64 reserve, DN_U64 commit);
DN_API void *                   DN_MemListAlloc                                        (DN_MemList *mem, DN_U64 size, uint8_t align, DN_ZMem zmem);
DN_API void *                   DN_MemListAllocContiguous                              (DN_MemList *mem, DN_U64 size, uint8_t align, DN_ZMem zmem);
DN_API void *                   DN_MemListCopy                                         (DN_MemList *mem, void const *data, DN_U64 size, uint8_t align);
DN_API void                     DN_MemListPopTo                                        (DN_MemList *mem, DN_U64 init_used);
DN_API void                     DN_MemListPop                                          (DN_MemList *mem, DN_U64 amount);
DN_API DN_U64                   DN_MemListPos                                          (DN_MemList const *mem);
DN_API void                     DN_MemListClear                                        (DN_MemList *mem);
DN_API bool                     DN_MemListOwnsPtr                                      (DN_MemList const *mem, void *ptr);
DN_API DN_Str8x64               DN_MemListInfoStr8x64                                  (DN_MemListInfo info);
DN_API DN_MemListTemp           DN_MemListTempBegin                                    (DN_MemList *mem);
DN_API void                     DN_MemListTempEnd                                      (DN_MemListTemp mem);
#define                         DN_MemListNew(arena, T, zmem)                          (T *)DN_MemListAlloc(arena, sizeof(T), alignof(T), zmem)
#define                         DN_MemListNewZ(arena, T)                               (T *)DN_MemListAlloc(arena, sizeof(T), alignof(T), DN_ZMem_Yes)
#define                         DN_MemListNewContiguous(arena, T, zmem)                (T *)DN_MemListAllocContiguous(arena, sizeof(T), alignof(T), zmem)
#define                         DN_MemListNewContiguousZ(arena, T)                     (T *)DN_MemListAllocContiguous(arena, sizeof(T), alignof(T), DN_ZMem_Yes)
#define                         DN_MemListNewArray(arena, T, count, zmem)              (T *)DN_MemListAlloc(arena, sizeof(T) * (count), alignof(T), zmem)
#define                         DN_MemListNewArrayZ(arena, T, count)                   (T *)DN_MemListAlloc(arena, sizeof(T) * (count), alignof(T), DN_ZMem_Yes)
#define                         DN_MemListNewArrayNoZ(arena, T, count)                 (T *)DN_MemListAlloc(arena, sizeof(T) * (count), alignof(T), DN_ZMem_No)
#define                         DN_MemListNewCopy(arena, T, src)                       (T *)DN_MemListCopy(arena, (src), sizeof(T),            alignof(T))
#define                         DN_MemListNewArrayCopy(arena, T, src, count)           (T *)DN_MemListCopy(arena, (src), sizeof(T)  * (count), alignof(T))

// NOTE: Arena
// Overview
//   `Arena`'s in this codebase are thin-layers over `MemList` but additionally provide
//   use-after-free (UAF) protection when using temporary memory regions (e.g. thread context
//   scratch `TCScratch` or `Temp[Begin|End]` family of functions).
//
//   These arenas associate themselves with the temporary memory region they begin in if it is
//   constructed using the `Temp[Begin|End]` family of functions (TCScratch implicitly call these
//   for you before handing you the arena). If you attempt to allocate from a different arena bound
//   with a different temporary memory region than the active one an assertion is triggered. This
//   protection is gated by the presence of the preprocessor definition
//   `#define DN_ARENA_TEMP_MEM_UAF_GUARD 1`.
//
//   Without the preprocessor definition UAF protection is compiled out (e.g. no-op). UAF protection
//   is also not enabled if you use `ArenaFromMemList` which simply sets up a plain arena that
//   forwards all calls into the `MemList` API.
//
//   To get UAF protection, all allocations _must_ go through the `Arena` API, using the `MemList`
//   field directly in the `Arena` will bypass these checks and lead to unusual behaviour. If you
//   want to forgo any of this infrastructure store and use the `MemList` directly in your codebase.
//
// UAF Example
/*
   DN_Arena arena = DN_ArenaFromHeap(DN_Megabytes(1), DN_MemFlags_Nil);
   DN_Arena temp  = DN_ArenaTempBeginFromArena(&arena);
   {
     // NOTE: You can also `TempBegin` with `&temp`, either is valid. They both have pointers to
     // the same underlying memory block owned by `arena`.
     DN_Arena nested_temp = DN_ArenaTempBeginFromArena(&arena);

     // NOTE: This allocation triggers the UAF guard and asserts! An allocation into `temp`'s memory
     // region would be reset when we end `nested_temp`'s memory region since they are spawned from
     // the same underlying memory block sitting in `arena`.
     //
     // But the intent here is that the caller is resetting `nested_temp`'s allocations and not
     // `temp` hence the UAF protection triggers.
     DN_U64 *u64          = DN_ArenaNewZ(&temp, DN_U64);

     DN_ArenaTempEnd(&nested_temp);
   }
   DN_ArenaTempEnd(&temp);
   DN_ArenaDeinit(&arena); // Frees the memory
 */
DN_API DN_Arena                 DN_ArenaFromMemList                                    (DN_MemList *mem);
DN_API DN_Arena                 DN_ArenaTempBeginFromMemList                           (DN_MemList *mem);
DN_API DN_Arena                 DN_ArenaTempBeginFromArena                             (DN_Arena *arena);
DN_API void                     DN_ArenaTempEnd                                        (DN_Arena *arena, DN_ArenaReset reset);
DN_API void*                    DN_ArenaAlloc                                          (DN_Arena *arena, DN_U64 size, uint8_t align, DN_ZMem z_mem);
DN_API void*                    DN_ArenaAllocContiguous                                (DN_Arena *arena, DN_U64 size, uint8_t align, DN_ZMem z_arena);
DN_API void*                    DN_ArenaCopy                                           (DN_Arena *arena, void const *data, DN_U64 size, uint8_t align);
DN_API void                     DN_ArenaDeinit                                         (DN_Arena *arena);

#define                         DN_ArenaNew(arena, T, zmem)                            (T *)DN_ArenaAlloc(arena, sizeof(T), alignof(T), zmem)
#define                         DN_ArenaNewZ(arena, T)                                 (T *)DN_ArenaAlloc(arena, sizeof(T), alignof(T), DN_ZMem_Yes)
#define                         DN_ArenaNewContiguous(arena, T, zmem)                  (T *)DN_ArenaAllocContiguous(arena, sizeof(T), alignof(T), zmem)
#define                         DN_ArenaNewContiguousZ(arena, T)                       (T *)DN_ArenaAllocContiguous(arena, sizeof(T), alignof(T), DN_ZMem_Yes)
#define                         DN_ArenaNewArray(arena, T, count, zmem)                (T *)DN_ArenaAlloc(arena, sizeof(T) * (count), alignof(T), zmem)
#define                         DN_ArenaNewArrayZ(arena, T, count)                     (T *)DN_ArenaAlloc(arena, sizeof(T) * (count), alignof(T), DN_ZMem_Yes)
#define                         DN_ArenaNewArrayNoZ(arena, T, count)                   (T *)DN_ArenaAlloc(arena, sizeof(T) * (count), alignof(T), DN_ZMem_No)
#define                         DN_ArenaNewCopy(arena, T, src)                         (T *)DN_ArenaCopy(arena, (src), sizeof(T),            alignof(T))
#define                         DN_ArenaNewArrayCopy(arena, T, src, count)             (T *)DN_ArenaCopy(arena, (src), sizeof(T)  * (count), alignof(T))

DN_API DN_Pool                  DN_PoolFromArena                                       (DN_Arena *arena, DN_U8 align);
DN_API bool                     DN_PoolIsValid                                         (DN_Pool const *pool);
DN_API void *                   DN_PoolAlloc                                           (DN_Pool *pool, DN_USize size);
DN_API void                     DN_PoolDealloc                                         (DN_Pool *pool, void *ptr);
DN_API void *                   DN_PoolCopy                                            (DN_Pool *pool, void const *data, DN_U64 size, uint8_t align);
#define                         DN_PoolNew(pool, T)                                    (T *)DN_PoolAlloc(pool, sizeof(T))
#define                         DN_PoolNewArray(pool, T, count)                        (T *)DN_PoolAlloc(pool, count * sizeof(T))
#define                         DN_PoolNewCopy(pool, T, src)                           (T *)DN_PoolCopy (pool, (src), sizeof(T),            alignof(T))
#define                         DN_PoolNewArrayCopy(pool, T, src, count)               (T *)DN_PoolCopy (pool, (src), sizeof(T)  * (count), alignof(T))

DN_API DN_ErrSink*              DN_ErrSinkBegin_                                       (DN_ErrSink *err, DN_ErrSinkMode mode, DN_CallSite call_site);
#define                         DN_ErrSinkBegin(err, mode)                             DN_ErrSinkBegin_(err, mode, DN_CallSiteNow)
#define                         DN_ErrSinkBeginDefault(err)                            DN_ErrSinkBegin(err, DN_ErrSinkMode_Nil)
DN_API bool                     DN_ErrSinkHasError                                     (DN_ErrSink *err);
DN_API DN_ErrSinkMsg*           DN_ErrSinkEnd                                          (DN_Arena *arena, DN_ErrSink *err);
DN_API DN_Str8                  DN_ErrSinkEndStr8                                      (DN_Arena *arena, DN_ErrSink *err);
DN_API void                     DN_ErrSinkEndIgnore                                    (DN_ErrSink *err);
DN_API bool                     DN_ErrSinkEndLogError_                                 (DN_ErrSink *err, DN_CallSite call_site, DN_Str8 msg);
#define                         DN_ErrSinkEndLogError(err, err_msg)                    DN_ErrSinkEndLogError_(err, DN_CallSiteNow, err_msg)
DN_API bool                     DN_ErrSinkEndLogErrorFV_                               (DN_ErrSink *err, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, va_list args);
#define                         DN_ErrSinkEndLogErrorFV(err, fmt, args)                DN_ErrSinkEndLogErrorFV_(err, DN_CallSiteNow, fmt, args)
DN_API bool                     DN_ErrSinkEndLogErrorF_                                (DN_ErrSink *err, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, ...);
#define                         DN_ErrSinkEndLogErrorF(err, fmt, ...)                  DN_ErrSinkEndLogErrorF_(err, DN_CallSiteNow, fmt, ##__VA_ARGS__)
DN_API void                     DN_ErrSinkEndExitIfErrorF_                             (DN_ErrSink *err, DN_CallSite call_site, DN_U32 exit_val, DN_FMT_ATTRIB char const *fmt, ...);
#define                         DN_ErrSinkEndExitIfErrorF(err, exit_val, fmt, ...)     DN_ErrSinkEndExitIfErrorF_(err, DN_CallSiteNow, exit_val, fmt, ##__VA_ARGS__)
DN_API void                     DN_ErrSinkEndExitIfErrorFV_                            (DN_ErrSink *err, DN_CallSite call_site, DN_U32 exit_val, DN_FMT_ATTRIB char const *fmt, va_list args);
#define                         DN_ErrSinkEndExitIfErrorFV(err, exit_val, fmt, args)   DN_ErrSinkEndExitIfErrorFV_(err, DN_CallSiteNow, exit_val, fmt, args)
DN_API void                     DN_ErrSinkAppendFV_                                    (DN_ErrSink *err, DN_U32 error_code, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, va_list args);
#define                         DN_ErrSinkAppendFV(error, error_code, fmt, args)       DN_ErrSinkAppendFV_(error, error_code, DN_CallSiteNow, fmt, args)
DN_API void                     DN_ErrSinkAppendF_                                     (DN_ErrSink *err, DN_U32 error_code, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, ...);
#define                         DN_ErrSinkAppendF(error, error_code, fmt, ...)         DN_ErrSinkAppendF_(error, error_code, DN_CallSiteNow, fmt, ##__VA_ARGS__)

DN_API DN_TCInitArgs            DN_TCInitArgsDefault                                   ();
DN_API void                     DN_TCInit                                              (DN_TCCore *tc, DN_U64 thread_id, DN_Arena *main_arena, DN_Arena *temp_arenas, DN_USize temp_arenas_count, DN_Arena *err_sink_arena);
DN_API void                     DN_TCInitFromMemFuncs                                  (DN_TCCore *tc, DN_U64 thread_id, DN_TCInitArgs args, DN_MemFuncs mem_funcs);
DN_API void                     DN_TCDeinit                                            (DN_TCCore *tc, DN_TCDeinitArenas deinit_arenas);
DN_API void                     DN_TCEquip                                             (DN_TCCore *tc);
DN_API DN_TCCore*               DN_TCGet                                               ();
DN_API DN_Arena*                DN_TCMainArena                                         ();
DN_API DN_Pool*                 DN_TCMainPool                                          ();
DN_API DN_Arena                 DN_TCTempArenaFromAllocator                            (DN_Allocator *conflicts, DN_USize count);
DN_API DN_Arena                 DN_TCTempArenaFromArena                                (DN_Arena **conflicts, DN_USize count);
DN_API DN_TCScratch             DN_TCScratchBeginAllocator                             (DN_Allocator *conflicts, DN_USize count);
DN_API DN_TCScratch             DN_TCScratchBeginArena                                 (DN_Arena **conflicts, DN_USize count);
DN_API void                     DN_TCScratchEnd                                        (DN_TCScratch *scratch);
DN_API void                     DN_TCSetFrameArena                                     (DN_Arena *arena);
DN_API DN_Arena*                DN_TCFrameArena                                        ();
DN_API DN_ErrSink*              DN_TCErrSink                                           ();
#define                         DN_TCErrSinkBegin(mode)                                DN_ErrSinkBegin(DN_TCErrSink(), mode)
#define                         DN_TCErrSinkBeginDefault()                             DN_ErrSinkBeginDefault(DN_TCErrSink())

DN_API bool                     DN_CharIsAlphabet                                      (char ch);
DN_API bool                     DN_CharIsDigit                                         (char ch);
DN_API bool                     DN_CharIsAlphaNum                                      (char ch);
DN_API bool                     DN_CharIsWhitespace                                    (char ch);
DN_API bool                     DN_CharIsHex                                           (char ch);
DN_API char                     DN_CharToLower                                         (char ch);
DN_API char                     DN_CharToUpper                                         (char ch);

DN_API DN_U64FromResult         DN_U64FromStr8                                         (DN_Str8 string, char separator);
DN_API DN_U64FromResult         DN_U64FromPtr                                          (void const *data, DN_USize size, char separator);
DN_API DN_U64                   DN_U64FromPtrUnsafe                                    (void const *data, DN_USize size, char separator);
DN_API DN_U64FromResult         DN_U64FromHexPtr                                       (void const *hex, DN_USize hex_count);
DN_API DN_U64                   DN_U64FromHexPtrUnsafe                                 (void const *hex, DN_USize hex_count);
DN_API DN_U64FromResult         DN_U64FromHexStr8                                      (DN_Str8 hex);
DN_API DN_U64                   DN_U64FromHexStr8Unsafe                                (DN_Str8 hex);
DN_API DN_U64                   DN_U64FromU8x32HiBEUnsafe                              (DN_U8x32 const *val); // Get U64 stored in big-endian at the high bytes [24:32)
DN_API DN_U64FromResult         DN_U64FromU8x32HiBE                                    (DN_U8x32 const *val); // Checks [0:24) bytes aren't set before getting the U64
DN_API DN_USize                 DN_USizeFromU8x32HiBEUnsafe                            (DN_U8x32 const *val); // Get USize stored in big-endian at the high bytes [32 - sizeof USize:32)
DN_API DN_USizeFromResult       DN_USizeFromU8x32HiBE                                  (DN_U8x32 const *val); // Checks [0:sizeof USize) bytes aren't set before getting the U64
DN_API DN_I64FromResult         DN_I64FromStr8                                         (DN_Str8 string, char separator);
DN_API DN_I64FromResult         DN_I64FromPtr                                          (void const *data, DN_USize size, char separator);
DN_API DN_I64                   DN_I64FromPtrUnsafe                                    (void const *data, DN_USize size, char separator);

DN_API bool                     DN_U8x32Eq                                             (DN_U8x32 const *lhs, DN_U8x32 const *rhs);
DN_API DN_U8x32                 DN_U8x32FromBytesLeftPadZ                              (DN_U8 const *ptr, DN_USize count);
DN_API DN_U8x32                 DN_U8x32FromHexUnsafe                                  (DN_Str8 hex_32b);
DN_API DN_U8x32FromResult       DN_U8x32FromHex                                        (DN_Str8 hex_32b);
DN_API DN_U8x32FromResult       DN_U8x32FromDecimalStr8                                (DN_Str8 decimal); // Write decimal string (e.g. "12345") as big-endian 256-bit value

DN_API DN_Allocator             DN_AllocatorFromMemList                                (DN_MemList *mem);
DN_API DN_Allocator             DN_AllocatorFromArena                                  (DN_Arena *arena);
DN_API DN_Allocator             DN_AllocatorFromPool                                   (DN_Pool *pool);
DN_API void*                    DN_AllocatorAlloc                                      (DN_Allocator allocator, DN_USize size, DN_U8 align, DN_ZMem z_mem);

DN_API DN_USize                 DN_FmtVSize                                            (DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API DN_USize                 DN_FmtSize                                             (DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_FmtAppendResult       DN_FmtVAppend                                          (char *buf, DN_USize *buf_size, DN_USize buf_max, char const *fmt, va_list args);
DN_API DN_FmtAppendResult       DN_FmtAppend                                           (char *buf, DN_USize *buf_size, DN_USize buf_max, char const *fmt, ...);
DN_API DN_FmtAppendResult       DN_FmtAppendTruncate                                   (char *buf, DN_USize *buf_size, DN_USize buf_max, DN_Str8 truncator, char const *fmt, ...);
DN_API DN_USize                 DN_CStr8Size                                           (char const *src);
DN_API DN_USize                 DN_CStr16Size                                          (wchar_t const *src);

#define                         DN_Str16Lit(string)                                    DN_Str16{(wchar_t *)(string), sizeof(string)/sizeof(string[0]) - 1}
#define                         DN_Str16FromPtr(data, size)                            DN_Literal(DN_Str16){(wchar_t *)(data), (DN_USize)(size)}

#define                         DN_Str8Lit(c_str)                                      DN_Literal(DN_Str8){(char *)(c_str), sizeof(c_str) - 1}
#define                         DN_Str8PrintFmt(string)                                (int)((string).size), (string).data

#define                         DN_Str8FromPtr(data, size)                             DN_Literal(DN_Str8){(char *)(data), (DN_USize)(size)}
#define                         DN_Str8FromStruct(ptr)                                 DN_Str8FromPtr((ptr)->data, (ptr)->size)
#define                         DN_Str8FromLitArray(c_array)                           DN_Str8FromPtr(c_array, DN_ArrayCountU(c_array))
DN_API DN_Str8                  DN_Str8AllocAllocator                                  (DN_USize size, DN_ZMem z_mem, DN_Allocator allocator);
DN_API DN_Str8                  DN_Str8AllocArena                                      (DN_USize size, DN_ZMem z_mem, DN_Arena *arena);
DN_API DN_Str8                  DN_Str8AllocPool                                       (DN_USize size, DN_Pool *pool);

DN_API DN_Str8                  DN_Str8FromCStr8                                       (char const *src);
DN_API DN_Str8                  DN_Str8FromCStr8Arena                                  (char const *src, DN_Arena *arena);
DN_API DN_Str8                  DN_Str8FromPtrArena                                    (void const *data, DN_USize size, DN_Arena *arena);
DN_API DN_Str8                  DN_Str8FromPtrPool                                     (void const *data, DN_USize size, DN_Pool *pool);
DN_API DN_Str8                  DN_Str8FromStr8Allocator                               (DN_Str8 string, DN_Allocator allocator);
DN_API DN_Str8                  DN_Str8FromStr8Arena                                   (DN_Str8 string, DN_Arena *arena);
DN_API DN_Str8                  DN_Str8FromStr8Pool                                    (DN_Str8 string, DN_Pool *pool);
DN_API DN_Str8                  DN_Str8FromFmtVAllocator                               (DN_Allocator allocator, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API DN_Str8                  DN_Str8FromFmtVArena                                   (DN_Arena *arena, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API DN_Str8                  DN_Str8FromFmtAllocator                                (DN_Allocator allocator, DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_Str8                  DN_Str8FromFmtArena                                    (DN_Arena *arena, DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_Str8                  DN_Str8FromFmtVPool                                    (DN_Pool *pool, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API DN_Str8                  DN_Str8FromFmtPool                                     (DN_Pool *pool, DN_FMT_ATTRIB char const *fmt, ...);

DN_API DN_Str8x16               DN_Str8x16FromFmt                                      (DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_Str8x16               DN_Str8x16FromFmtV                                     (DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API DN_Str8x32               DN_Str8x32FromFmt                                      (DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_Str8x32               DN_Str8x32FromFmtV                                     (DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API DN_Str8x64               DN_Str8x64FromFmt                                      (DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_Str8x64               DN_Str8x64FromFmtV                                     (DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API DN_Str8x128              DN_Str8x128FromFmt                                     (DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_Str8x256              DN_Str8x256FromFmtV                                    (DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API DN_Str8x256              DN_Str8x256FromFmt                                     (DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_Str8x256              DN_Str8x256FromFmtV                                    (DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API DN_Str8x512              DN_Str8x512FromFmt                                     (DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_Str8x512              DN_Str8x512FromFmtV                                    (DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API DN_Str8x1024             DN_Str8x1024FromFmt                                    (DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_Str8x1024             DN_Str8x1024FromFmtV                                   (DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API void                     DN_Str8x16AppendFmt                                    (DN_Str8x16 *str, DN_FMT_ATTRIB char const *fmt, ...);
DN_API void                     DN_Str8x16AppendFmtV                                   (DN_Str8x16 *str, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API void                     DN_Str8x32AppendFmt                                    (DN_Str8x32 *str, DN_FMT_ATTRIB char const *fmt, ...);
DN_API void                     DN_Str8x32AppendFmtV                                   (DN_Str8x32 *str, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API void                     DN_Str8x64AppendFmt                                    (DN_Str8x64 *str, DN_FMT_ATTRIB char const *fmt, ...);
DN_API void                     DN_Str8x64AppendFmtV                                   (DN_Str8x64 *str, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API void                     DN_Str8x128AppendFmt                                   (DN_Str8x128 *str, DN_FMT_ATTRIB char const *fmt, ...);
DN_API void                     DN_Str8x128AppendFmtV                                  (DN_Str8x128 *str, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API void                     DN_Str8x256AppendFmt                                   (DN_Str8x256 *str, DN_FMT_ATTRIB char const *fmt, ...);
DN_API void                     DN_Str8x256AppendFmtV                                  (DN_Str8x256 *str, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API void                     DN_Str8x512AppendFmt                                   (DN_Str8x512 *str, DN_FMT_ATTRIB char const *fmt, ...);
DN_API void                     DN_Str8x512AppendFmtV                                  (DN_Str8x512 *str, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API void                     DN_Str8x1024AppendFmt                                  (DN_Str8x1024 *str, DN_FMT_ATTRIB char const *fmt, ...);
DN_API void                     DN_Str8x1024AppendFmtV                                 (DN_Str8x1024 *str, DN_FMT_ATTRIB char const *fmt, va_list args);

DN_API DN_Str8x32               DN_Str8x32FromU64                                      (DN_U64 val, char separator);
DN_API bool                     DN_Str8IsAll                                           (DN_Str8 string, DN_Str8IsAllType is_all);
DN_API char *                   DN_Str8End                                             (DN_Str8 string);
DN_API DN_Str8                  DN_Str8Subset                                          (DN_Str8 string, DN_USize offset, DN_USize size);
DN_API DN_Str8                  DN_Str8Advance                                         (DN_Str8 string, DN_USize amount);
DN_API DN_Str8                  DN_Str8NextLine                                        (DN_Str8 string);
DN_API DN_Str8BSplitResult      DN_Str8BSplitArray                                     (DN_Str8 string, DN_Str8 const *find, DN_USize find_size);
DN_API DN_Str8BSplitResult      DN_Str8BSplit                                          (DN_Str8 string, DN_Str8 find);
DN_API DN_Str8BSplitResult      DN_Str8BSplitLastArray                                 (DN_Str8 string, DN_Str8 const *find, DN_USize find_size);
DN_API DN_Str8BSplitResult      DN_Str8BSplitLast                                      (DN_Str8 string, DN_Str8 find);
DN_API DN_USize                 DN_Str8Split                                           (DN_Str8 string, DN_Str8 delimiter, DN_Str8 *splits, DN_USize splits_count, DN_Str8SplitFlags mode);
DN_API DN_Str8SplitResult       DN_Str8SplitArena                                      (DN_Str8 string, DN_Str8 delimiter, DN_Str8SplitFlags mode, DN_Arena *arena);
DN_API DN_Str8FindResult        DN_Str8FindStr8Array                                   (DN_Str8 string, DN_Str8 const *find, DN_USize find_size, DN_Str8EqCase eq_case);
DN_API DN_Str8FindResult        DN_Str8FindStr8                                        (DN_Str8 string, DN_Str8 find, DN_Str8EqCase eq_case);
DN_API DN_Str8FindResult        DN_Str8Find                                            (DN_Str8 string, DN_Str8FindFlag flags);
DN_API DN_Str8                  DN_Str8Segment                                         (DN_Arena *arena, DN_Str8 src, DN_USize segment_size, char segment_char);
DN_API DN_Str8                  DN_Str8ReverseSegment                                  (DN_Arena *arena, DN_Str8 src, DN_USize segment_size, char segment_char);
DN_API bool                     DN_Str8Eq                                              (DN_Str8 lhs, DN_Str8 rhs, DN_Str8EqCase eq_case = DN_Str8EqCase_Sensitive);
DN_API bool                     DN_Str8EqInsensitive                                   (DN_Str8 lhs, DN_Str8 rhs);
DN_API bool                     DN_Str8StartsWith                                      (DN_Str8 string, DN_Str8 prefix, DN_Str8EqCase eq_case = DN_Str8EqCase_Sensitive);
DN_API bool                     DN_Str8StartsWithInsensitive                           (DN_Str8 string, DN_Str8 prefix);
DN_API bool                     DN_Str8EndsWith                                        (DN_Str8 string, DN_Str8 prefix, DN_Str8EqCase eq_case = DN_Str8EqCase_Sensitive);
DN_API bool                     DN_Str8EndsWithInsensitive                             (DN_Str8 string, DN_Str8 prefix);
DN_API bool                     DN_Str8HasChar                                         (DN_Str8 string, char ch);

DN_API DN_Str8                  DN_Str8TrimPrefix                                      (DN_Str8 string, DN_Str8 prefix, DN_Str8EqCase eq_case = DN_Str8EqCase_Sensitive);
DN_API DN_Str8                  DN_Str8TrimHexPrefix                                   (DN_Str8 string);
DN_API DN_Str8                  DN_Str8TrimSuffix                                      (DN_Str8 string, DN_Str8 suffix, DN_Str8EqCase eq_case = DN_Str8EqCase_Sensitive);
DN_API DN_Str8                  DN_Str8TrimAround                                      (DN_Str8 string, DN_Str8 trim_string);
DN_API DN_Str8                  DN_Str8TrimHeadWhitespace                              (DN_Str8 string);
DN_API DN_Str8                  DN_Str8TrimTailWhitespace                              (DN_Str8 string);
DN_API DN_Str8                  DN_Str8TrimWhitespaceAround                            (DN_Str8 string);
DN_API DN_Str8                  DN_Str8TrimByteOrderMark                               (DN_Str8 string);

DN_API DN_Str8                  DN_Str8FileNameFromPath                                (DN_Str8 path);
DN_API DN_Str8                  DN_Str8FileNameNoExtension                             (DN_Str8 path);
DN_API DN_Str8                  DN_Str8FilePathNoExtension                             (DN_Str8 path);
DN_API DN_Str8                  DN_Str8FileExtension                                   (DN_Str8 path);
DN_API DN_Str8                  DN_Str8FileDirectoryFromPath                           (DN_Str8 path);
DN_API DN_Str8                  DN_Str8AppendF                                         (DN_Arena *arena, DN_Str8 string, char const *fmt, ...);
DN_API DN_Str8                  DN_Str8AppendFV                                        (DN_Arena *arena, DN_Str8 string, char const *fmt, va_list args);
DN_API DN_Str8                  DN_Str8FillF                                           (DN_Arena *arena, DN_USize count, char const *fmt, ...);
DN_API DN_Str8                  DN_Str8FillFV                                          (DN_Arena *arena, DN_USize count, char const *fmt, va_list args);
DN_API void                     DN_Str8Remove                                          (DN_Str8 *string, DN_USize offset, DN_USize size);
DN_API DN_Str8TruncResult       DN_Str8TruncMiddlePtr                                  (DN_Str8 str8, DN_USize side_size, DN_Str8 truncator, char *dest, DN_USize dest_max);
DN_API DN_Str8TruncResult       DN_Str8TruncMiddle                                     (DN_Str8 str8, DN_USize side_size, DN_Str8 truncator, DN_Arena *arena);
DN_API DN_Str8                  DN_Str8Lower                                           (DN_Str8 string, DN_Arena *arena);
DN_API DN_Str8                  DN_Str8Upper                                           (DN_Str8 string, DN_Arena *arena);
DN_API DN_Str8                  DN_Str8Replace                                         (DN_Str8 string, DN_Str8 find, DN_Str8 replace, DN_USize start_index, DN_Arena *arena, DN_Str8EqCase eq_case);
DN_API DN_Str8                  DN_Str8ReplaceSensitive                                (DN_Str8 string, DN_Str8 find, DN_Str8 replace, DN_USize start_index, DN_Arena *arena);
DN_API DN_Str8                  DN_Str8ReplaceInsensitive                              (DN_Str8 string, DN_Str8 find, DN_Str8 replace, DN_USize start_index, DN_Arena *arena);
DN_API DN_Str8                  DN_Str8PadNewLinesAllocator                            (DN_Str8 string, DN_Str8 pad_string, DN_Allocator allocator);
DN_API DN_Str8                  DN_Str8PadNewLinesArena                                (DN_Str8 string, DN_Str8 pad_string, DN_Arena *arena);
DN_API DN_Str8                  DN_Str8LineBreakAllocator                              (DN_Str8 src, DN_USize desired_width, DN_Str8 delimiter, DN_Str8LineBreakMode mode, DN_Allocator allocator);
DN_API DN_Str8                  DN_Str8LineBreakArena                                  (DN_Str8 src, DN_USize desired_width, DN_Str8 delimiter, DN_Str8LineBreakMode mode, DN_Arena *arena);
DN_API DN_Str8                  DN_Str8Table                                           (DN_Str8 const* rows, DN_USize num_rows, DN_USize num_cols, DN_Str8TableFlags flags, DN_Arena *arena);

#if DN_STR8_AVX512F
DN_API DN_Str8FindResult        DN_Str8FindStr8AVX512F                                 (DN_Str8 string, DN_Str8 find);
DN_API DN_Str8FindResult        DN_Str8FindLastStr8AVX512F                             (DN_Str8 string, DN_Str8 find);
DN_API DN_Str8BSplitResult      DN_Str8BSplitAVX512F                                   (DN_Str8 string, DN_Str8 find);
DN_API DN_Str8BSplitResult      DN_Str8BSplitLastAVX512F                               (DN_Str8 string, DN_Str8 find);
DN_API DN_USize                 DN_Str8SplitAVX512F                                    (DN_Str8 string, DN_Str8 delimiter, DN_Str8 *splits, DN_USize splits_count, DN_Str8SplitFlags flags);
DN_API DN_Str8Slice             DN_Str8SplitAllocAVX512F                               (DN_Arena *arena, DN_Str8 string, DN_Str8 delimiter, DN_Str8SplitFlags flags);
#endif

DN_API DN_Str8                  DN_Str8SliceRender                                     (DN_Str8Slice array, DN_Str8 separator, DN_Arena *arena);
DN_API DN_Str8                  DN_Str8RenderSpaceSep                                  (DN_Str8Slice array, DN_Arena *arena);
DN_API int                      DN_Str8CompareNatural                                  (DN_Str8 lhs, DN_Str8 rhs, DN_Str8EqCase eq_case);
DN_API int                      DN_Str8CompareLexicographic                            (DN_Str8 lhs, DN_Str8 rhs, DN_Str8EqCase eq_case);

DN_API bool                     DN_Str16Eq                                             (DN_Str16 lhs, DN_Str16 rhs);
DN_API DN_Str16                 DN_Str16SliceRender                                    (DN_Str16Slice array, DN_Str16 separator, DN_Arena *arena);
DN_API DN_Str16                 DN_Str16RenderSpaceSep                                 (DN_Str16Slice array, DN_Arena *arena);

DN_API DN_Str8Builder           DN_Str8BuilderFromArena                                (DN_Arena *arena);
DN_API DN_Str8Builder           DN_Str8BuilderFromStr8PtrRef                           (DN_Arena *arena, DN_Str8 const *strings, DN_USize size);
DN_API DN_Str8Builder           DN_Str8BuilderFromStr8PtrCopy                          (DN_Arena *arena, DN_Str8 const *strings, DN_USize size);
DN_API DN_Str8Builder           DN_Str8BuilderFromBuilder                              (DN_Arena *arena, DN_Str8Builder const *builder);
DN_API bool                     DN_Str8BuilderAddArrayRef                              (DN_Str8Builder *builder, DN_Str8 const *strings, DN_USize size, DN_Str8BuilderAdd add);
DN_API bool                     DN_Str8BuilderAddArrayCopy                             (DN_Str8Builder *builder, DN_Str8 const *strings, DN_USize size, DN_Str8BuilderAdd add);
DN_API bool                     DN_Str8BuilderAddFV                                    (DN_Str8Builder *builder, DN_Str8BuilderAdd add, DN_FMT_ATTRIB char const *fmt, va_list args);
#define                         DN_Str8BuilderAppendArrayRef(builder, strings, size)   DN_Str8BuilderAddArrayRef(builder, strings, size, DN_Str8BuilderAdd_Append)
#define                         DN_Str8BuilderAppendArrayCopy(builder, strings, size)  DN_Str8BuilderAddArrayCopy(builder, strings, size, DN_Str8BuilderAdd_Append)
#define                         DN_Str8BuilderAppendSliceRef(builder, slice)           DN_Str8BuilderAddArrayRef(builder, slice.data, slice.size, DN_Str8BuilderAdd_Append)
#define                         DN_Str8BuilderAppendSliceCopy(builder, slice)          DN_Str8BuilderAddArrayCopy(builder, slice.data, slice.size, DN_Str8BuilderAdd_Append)
DN_API bool                     DN_Str8BuilderAppendRef                                (DN_Str8Builder *builder, DN_Str8 string);
DN_API bool                     DN_Str8BuilderAppendCopy                               (DN_Str8Builder *builder, DN_Str8 string);
#define                         DN_Str8BuilderAppendFV(builder, fmt, args)             DN_Str8BuilderAddFV(builder, DN_Str8BuilderAdd_Append, fmt, args)
DN_API bool                     DN_Str8BuilderAppendF                                  (DN_Str8Builder *builder, DN_FMT_ATTRIB char const *fmt, ...);
DN_API bool                     DN_Str8BuilderAppendBytesRef                           (DN_Str8Builder *builder, void const *ptr, DN_USize size);
DN_API bool                     DN_Str8BuilderAppendBytesCopy                          (DN_Str8Builder *builder, void const *ptr, DN_USize size);
DN_API bool                     DN_Str8BuilderAppendBuilderRef                         (DN_Str8Builder *dest, DN_Str8Builder const *src);
DN_API bool                     DN_Str8BuilderAppendBuilderCopy                        (DN_Str8Builder *dest, DN_Str8Builder const *src);
#define                         DN_Str8BuilderPrependArrayRef(builder, strings, size)  DN_Str8BuilderAddArrayRef(builder, strings, size, DN_Str8BuilderAdd_Prepend)
#define                         DN_Str8BuilderPrependArrayCopy(builder, strings, size) DN_Str8BuilderAddArrayCopy(builder, strings, size, DN_Str8BuilderAdd_Prepend)
#define                         DN_Str8BuilderPrependSliceRef(builder, slice)          DN_Str8BuilderAddArrayRef(builder, slice.data, slice.size, DN_Str8BuilderAdd_Prepend)
#define                         DN_Str8BuilderPrependSliceCopy(builder, slice)         DN_Str8BuilderAddArrayCopy(builder, slice.data, slice.size, DN_Str8BuilderAdd_Prepend)
DN_API bool                     DN_Str8BuilderPrependRef                               (DN_Str8Builder *builder, DN_Str8 string);
DN_API bool                     DN_Str8BuilderPrependCopy                              (DN_Str8Builder *builder, DN_Str8 string);
#define                         DN_Str8BuilderPrependFV(builder, fmt, args)            DN_Str8BuilderAddFV(builder, DN_Str8BuilderAdd_Prepend, fmt, args)
DN_API bool                     DN_Str8BuilderPrependF                                 (DN_Str8Builder *builder, DN_FMT_ATTRIB char const *fmt, ...);
DN_API bool                     DN_Str8BuilderErase                                    (DN_Str8Builder *builder, DN_Str8 string);
DN_API DN_Str8                  DN_Str8FromStr8BuilderAllocator                        (DN_Str8Builder const *builder, DN_Allocator allocator);
DN_API DN_Str8                  DN_Str8FromStr8BuilderArena                            (DN_Str8Builder const *builder, DN_Arena *arena);
DN_API DN_Str8                  DN_Str8FromStr8BuilderDelimitAllocator                 (DN_Str8Builder const *builder, DN_Str8 delimiter, DN_Allocator allocator);
DN_API DN_Str8                  DN_Str8FromStr8BuilderDelimitArena                     (DN_Str8Builder const *builder, DN_Str8 delimiter, DN_Arena *arena);

DN_API int                      DN_UTF8Encode                                          (DN_U8 utf8[4], DN_U32 codepoint);
DN_API int                      DN_UTF16Encode                                         (DN_U16 utf16[2], DN_U32 codepoint);
DN_API DN_UTF8DecodeResult      DN_UTF8Decode                                          (DN_Str8 stream);
DN_API bool                     DN_UTF8DecodeIterate                                   (DN_UTF8DecodeIterator *it, DN_Str8 utf8);
DN_API DN_USize                 DN_USizeCodepointCountFromUTF8                         (DN_Str8 str, DN_CodepointCountFlags flags);

DN_API DN_U8                    DN_U8FromHexNibble                                     (char hex);
DN_API DN_NibbleFromU8Result    DN_NibbleFromU8                                        (DN_U8 u8);

DN_API DN_USize                 DN_BytesFromHex                                        (DN_Str8 hex, void *dest, DN_USize dest_count);
DN_API DN_Str8                  DN_BytesFromHexArena                                   (DN_Str8 hex, DN_Arena *arena);
DN_API DN_USize                 DN_BytesFromHexPtr                                     (char const *hex, DN_USize hex_count, void *dest, DN_USize dest_count);
DN_API DN_Str8                  DN_BytesFromHexPtrArena                                (char const *hex, DN_USize hex_count, DN_Arena *arena);
DN_API DN_Str8                  DN_BytesFromHexPtrPool                                 (char const *hex, DN_USize hex_count, DN_Pool *pool);
DN_API DN_U8x16                 DN_BytesFromHex32Ptr                                   (char const *hex, DN_USize hex_count);
DN_API DN_U8x32                 DN_BytesFromHex64Ptr                                   (char const *hex, DN_USize hex_count);

DN_API DN_HexU64                DN_HexFromU64                                          (DN_U64 value, DN_HexFromU64Type type);
DN_API DN_USize                 DN_HexFromPtrBytes                                     (void const *bytes, DN_USize bytes_count, void *hex, DN_USize hex_count, DN_TrimLeadingZero trim_leading_z);
DN_API DN_Str8                  DN_HexFromPtrBytesArena                                (void const *bytes, DN_USize bytes_count, DN_Arena *arena, DN_TrimLeadingZero trim_leading_z);
DN_API DN_USize                 DN_HexFromStr8Bytes                                    (DN_Str8 bytes, void *hex, DN_USize hex_count, DN_TrimLeadingZero trim_leading_z);
DN_API DN_Str8                  DN_HexFromStr8BytesArena                               (DN_Str8 bytes, DN_Arena *arena, DN_TrimLeadingZero trim_leading_z);
DN_API DN_Hex32                 DN_Hex32FromPtr16b                                     (void const *bytes, DN_USize bytes_count, DN_TrimLeadingZero trim_leading_z);
DN_API DN_Hex64                 DN_Hex64FromPtr32b                                     (void const *bytes, DN_USize bytes_count, DN_TrimLeadingZero trim_leading_z);
DN_API DN_Hex128                DN_Hex128FromPtr64b                                    (void const *bytes, DN_USize bytes_count, DN_TrimLeadingZero trim_leading_z);

DN_API DN_Str8x128              DN_AgeStr8FromMsU64                                    (DN_U64 duration_ms, DN_AgeUnit units);
DN_API DN_Str8x128              DN_AgeStr8FromSecU64                                   (DN_U64 duration_ms, DN_AgeUnit units);
DN_API DN_Str8x128              DN_AgeStr8FromSecF64                                   (DN_F64 sec, DN_AgeUnit units);

DN_API int                      DN_IsLeapYear                                          (int year);
DN_API bool                     DN_DateIsValid                                         (DN_Date date);
DN_API DN_Date                  DN_DateFromUnixTimeMs                                  (DN_USize unix_ts_ms);
DN_API DN_U64                   DN_UnixTimeMsFromDate                                  (DN_Date date);

DN_API DN_Str8                  DN_Str8FromByteType                                    (DN_ByteType type);
DN_API DN_ByteCount             DN_ByteCountFromU64                                    (DN_U64 byte_count, DN_ByteType type);
DN_API DN_Str8x32               DN_Str8x32FromByteCountU64                             (DN_U64 byte_count, DN_ByteType type);
#define                         DN_Str8x32FromByteCountU64Auto(bytes)                  DN_Str8x32FromByteCountU64(bytes, DN_ByteType_Auto)

// NOTE: Profiler
// Overview
//   Basic profiler that tracks the duration of marked-up regions which are denoted by an
//   opening and closing `anchor`. The profiler works in "frame" life-cycles which can by cycled by
//   calling `DN_ProfilerNewFrame`. The number of frames that the profiler will persist is chosen by
//   `anchors_per_frame`.
//
//   For example if you pass a buffer of 1024 `anchors` and `anchors_per_frame = 128` then the
//   profiler will hold onto 8 (1024 anchors / 128 anchors per frame) frames of profiling
//   information. The profiler will cycle through the 8 frames of anchors and upon reaching the end
//   begin overwriting the oldest frames worth of anchors.
//
//   Once a frame has ended the just completed buffer of anchors that was just written to can be
//   read by the caller and visualised to find the timings of each region, relative to the duration
//   of the frame until it is eventually overwritten by calling `DN_ProfilerNewFrame` once the
//   profiler has cycled through all the other frames.
//
//   The caller must upfront determine the number of anchors and frames the profiler should have and
//   pass it in. The profiler does not allocate any memory.
//
//   When profiling functions that are invoked from different call-stacks the exclusive elapsed
//   duration can exceed its inclusive duration. This is by design as the profiler makes no
//   effort to distinguish between the different call-tree that a zone may have been triggered by.
//   The profiler simpler always updates the same static anchor assigned for that zone.
//
// API
//   DN_ProfilerInit
//     You can set `tsc_now` to NULL to use the default timer mechanic which relies on
//     DN_CPUGetTSC() which essentially uses __rdtsc. `tsc_frequency` must however always be
//     provided, for, DN_CPUGetTSC() you can use `DN_OS_EstimateTSCPerSecond()` to calculate the
//     frequency of `__rdtsc`.
//   DN_ProfilerNewFrame
//     Always call `DN_ProfilerNewFrame` at-least once using the `Zone` family of functions as it
//     sets up
//   DN_ProfilerBeginZone
//     The zeroth anchor is reserved for profiling the begin and end of a profiler frame. If you are
//     manually specifying `anchor_index` (e.g. using an enum) ensure that the first anchor index
//     starts from `1`. Note that the `BeginZoneAuto` macro uses `__COUNTER__ + 1` to skip the 0th
//     zone.
//   DN_ProfilerFrameAnchors
//     Returns the current frame's (e.g. the anchors that the profiler is currently writing to)
//     worth of anchors
#define DN_ProfilerZoneLoop(prof, name, index)                                                                            \
  DN_ProfilerZone DN_UniqueName(zone_) = DN_ProfilerBeginZone(prof, DN_Str8Lit(name), index), DN_UniqueName(dummy_) = {}; \
  DN_UniqueName(dummy_).begin_tsc == 0;                                                                                   \
  DN_ProfilerEndZone(DN_UniqueName(zone_)), DN_UniqueName(dummy_).begin_tsc = 1

#define                         DN_ProfilerZoneLoopAuto(prof, name)                    DN_ProfilerZoneLoop(prof, name, __COUNTER__ + 1)
DN_API DN_Profiler              DN_ProfilerInit                                        (DN_ProfilerAnchor *anchors, DN_USize count, DN_USize anchors_per_frame, DN_ProfilerTSCNowFunc *tsc_now, DN_U64 tsc_frequency);
DN_API DN_ProfilerZone          DN_ProfilerBeginZone                                   (DN_Profiler *profiler, DN_Str8 name, DN_U16 anchor_index);
#define                         DN_ProfilerBeginZoneAuto(prof, name)                   DN_ProfilerBeginZone(prof, DN_Str8Lit(name), __COUNTER__ + 1)
DN_API void                     DN_ProfilerEndZone                                     (DN_ProfilerZone zone);
DN_API DN_USize                 DN_ProfilerFrameCount                                  (DN_Profiler const *profiler);
DN_API DN_ProfilerAnchorArray   DN_ProfilerFrameAnchorsFromIndex                       (DN_Profiler *profiler, DN_USize frame_index);
DN_API DN_ProfilerAnchorArray   DN_ProfilerFrameAnchors                                (DN_Profiler *profiler);
DN_API void                     DN_ProfilerNewFrame                                    (DN_Profiler *profiler);
DN_API DN_USize                 DN_ProfilerFmtAnchor                                   (DN_ProfilerAnchor anchor, DN_U64 tsc_frequency, char *buffer, DN_USize count);
DN_API DN_Str8                  DN_ProfilerFmtAnchorStr8                               (DN_ProfilerAnchor anchor, DN_U64 tsc_frequency, DN_Arena *arena);
DN_API void                     DN_ProfilerFmtToStdout                                 (DN_Profiler *profiler);
DN_API DN_F64                   DN_ProfilerSecFromTSC                                  (DN_Profiler *profiler, DN_U64 duration_tsc);
DN_API DN_F64                   DN_ProfilerMsFromTSC                                   (DN_Profiler *profiler, DN_U64 duration_tsc);

DN_API void                     DN_QSort                                               (void *array, DN_USize array_size, DN_USize elem_size, void *user_context, DN_QSortCompareFunc *compare);
DN_API bool                     DN_QSortCompareStr8NaturalAsc                          (void const* lhs, void const *rhs, void *user_context);
DN_API bool                     DN_QSortCompareStr8NaturalDesc                         (void const* lhs, void const *rhs, void *user_context);
DN_API bool                     DN_QSortCompareStr8LexicographicAsc                    (void const* lhs, void const *rhs, void *user_context);
DN_API bool                     DN_QSortCompareStr8LexicographicDesc                   (void const* lhs, void const *rhs, void *user_context);
DN_API bool                     DN_QSortCompareBytesLT                                 (void const* lhs, void const *rhs, void *user_context);
DN_API bool                     DN_QSortCompareBytesGT                                 (void const* lhs, void const *rhs, void *user_context);
DN_API void                     DN_QSortBytesLT                                        (void *array, DN_USize array_size, DN_USize elem_size);
DN_API void                     DN_QSortBytesGT                                        (void *array, DN_USize array_size, DN_USize elem_size);
DN_API void                     DN_QSortStr8NaturalAsc                                 (DN_Str8 *array, DN_USize array_size, DN_Str8EqCase eq_case);
DN_API void                     DN_QSortStr8NaturalDesc                                (DN_Str8 *array, DN_USize array_size, DN_Str8EqCase eq_case);
DN_API void                     DN_QSortStr8LexicographicAsc                           (DN_Str8 *array, DN_USize array_size, DN_Str8EqCase eq_case);
DN_API void                     DN_QSortStr8LexicographicDesc                          (DN_Str8 *array, DN_USize array_size, DN_Str8EqCase eq_case);

DN_API DN_PCG32                 DN_PCG32Init                                           (DN_U64 seed);
DN_API DN_U32                   DN_PCG32Next                                           (DN_PCG32 *rng);
DN_API DN_U64                   DN_PCG32Next64                                         (DN_PCG32 *rng);
DN_API DN_U32                   DN_PCG32Range                                          (DN_PCG32 *rng, DN_U32 low, DN_U32 high);
DN_API DN_F32                   DN_PCG32NextF32                                        (DN_PCG32 *rng);
DN_API DN_F64                   DN_PCG32NextF64                                        (DN_PCG32 *rng);
DN_API void                     DN_PCG32Advance                                        (DN_PCG32 *rng, DN_U64 delta);

#if !defined(DN_FNV1A32_SEED)
  #define DN_FNV1A32_SEED 2166136261U
#endif

#if !defined(DN_FNV1A64_SEED)
  #define DN_FNV1A64_SEED 14695981039346656037ULL
#endif

DN_API DN_U32                   DN_FNV1AHashU32FromBytes                               (void const *bytes, DN_USize size, DN_U32 seed);
DN_API DN_U64                   DN_FNV1AHashU64FromBytes                               (void const *bytes, DN_USize size, DN_U64 seed);

DN_API DN_U32                   DN_MurmurHash3HashU32FromBytesX86                      (void const *bytes, int len, DN_U32 seed);
DN_API DN_MurmurHash3           DN_MurmurHash3HashU128FromBytesX64                     (void const *bytes, int len, DN_U32 seed);
DN_API DN_U64                   DN_MurmurHash3HashU64FromBytesX64                      (void const *bytes, int len, DN_U32 seed);
DN_API DN_U32                   DN_MurmurHash3HashU32FromBytesX64                      (void const *bytes, int len, DN_U32 seed);

#if defined(DN_64_BIT)
  #define                       DN_MurmurHash3HashU32FromBytes(bytes, len, seed)       DN_MurmurHash3HashU32FromBytesX64(bytes, len, seed)
#else
  #define                       DN_MurmurHash3HashU32FromBytes(bytes, len, seed)       DN_MurmurHash3HashU32FromBytesX86(bytes, len, seed)
#endif

#define                         DN_ANSICodeBoldLit                                     "\x1b[1m"
#define                         DN_ANSICodeResetLit                                    "\x1b[0m"
DN_API DN_Str8x32               DN_Str8x32FromANSIColourCodeU8RGB                      (DN_ANSIColourMode mode, DN_U8 r, DN_U8 g, DN_U8 b);
DN_API DN_Str8x32               DN_Str8x32FromANSIColourCodeV3F32RGB255                (DN_ANSIColourMode mode, DN_V3F32 rgb_255);
DN_API DN_Str8x32               DN_Str8x32FromANSIColourCodeU32RGB                     (DN_ANSIColourMode mode, DN_U32 value);
DN_API DN_Str8                  DN_Str8FromStr8ANSIColourU8RGBArena                    (DN_ANSIColourMode mode, DN_Str8 str8, DN_U8 r, DN_U8 g, DN_U8 b, DN_Arena *arena);
DN_API DN_Str8                  DN_Str8FromStr8ANSIColourV3F32RGB255Arena              (DN_ANSIColourMode mode, DN_Str8 str8, DN_V3F32 rgb_255, DN_Arena *arena);
DN_API DN_Str8                  DN_Str8FromFmtANSIColourU8RGBArena                     (DN_ANSIColourMode mode, DN_U8 r, DN_U8 g, DN_U8 b, DN_Arena *arena, char const *fmt, ...);
DN_API DN_Str8                  DN_Str8FromFmtANSIColourV3F32RGB255Arena               (DN_ANSIColourMode mode, DN_V3F32 rgb_255, DN_Arena *arena, char const *fmt, ...);

// NOTE: Create log printable lines with a date prefix, severity and message. The platform
// implementation should call `SetPrintFunc` to intercept the log messages and output to the desired
// destination (log file, standard out, e.t.c.). When the library is initialised `DN_Init` to with
// OS functionality enabled, the log callback is by default set to outputting via standard out.
DN_API DN_LogPrefixSize         DN_LogMakePrefix                                       (DN_LogStyle style, DN_LogTypeParam type, DN_CallSite call_site, DN_LogDate date, char *dest, DN_USize dest_size);
DN_API void                     DN_LogSetPrintFunc                                     (DN_LogPrintFunc *print_func, void *user_data);
DN_API void                     DN_LogPrintF                                           (DN_LogTypeParam type, DN_CallSite call_site, DN_LogFlags flags, DN_FMT_ATTRIB char const *fmt, ...);
DN_API void                     DN_LogPrintFV                                          (DN_LogTypeParam type, DN_CallSite call_site, DN_LogFlags flags, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API DN_LogTypeParam          DN_LogTypeParamFromType                                (DN_LogType type);
#define                         DN_LogF(type, fmt, ...)                                DN_LogPrintF(DN_LogTypeParamFromType(type), DN_CallSiteNow, DN_LogFlags_Nil, fmt, ##__VA_ARGS__)

#define                         DN_LogDebugF(fmt, ...)                                 DN_LogPrintF(DN_LogTypeParamFromType(DN_LogType_Debug),   DN_CallSiteNow, DN_LogFlags_Nil, fmt, ##__VA_ARGS__)
#define                         DN_LogInfoF(fmt, ...)                                  DN_LogPrintF(DN_LogTypeParamFromType(DN_LogType_Info),    DN_CallSiteNow, DN_LogFlags_Nil, fmt, ##__VA_ARGS__)
#define                         DN_LogWarningF(fmt, ...)                               DN_LogPrintF(DN_LogTypeParamFromType(DN_LogType_Warning), DN_CallSiteNow, DN_LogFlags_Nil, fmt, ##__VA_ARGS__)
#define                         DN_LogErrorF(fmt, ...)                                 DN_LogPrintF(DN_LogTypeParamFromType(DN_LogType_Error),   DN_CallSiteNow, DN_LogFlags_Nil, fmt, ##__VA_ARGS__)

#define                         DN_LogFlagF(type, flags, fmt, ...)                     DN_LogPrintF(DN_LogTypeParamFromType(type),               DN_CallSiteNow, flags, fmt, ##__VA_ARGS__)
#define                         DN_LogFlagDebugF(flags, fmt, ...)                      DN_LogPrintF(DN_LogTypeParamFromType(DN_LogType_Debug),   DN_CallSiteNow, flags, fmt, ##__VA_ARGS__)
#define                         DN_LogFlagInfoF(flags, fmt, ...)                       DN_LogPrintF(DN_LogTypeParamFromType(DN_LogType_Info),    DN_CallSiteNow, flags, fmt, ##__VA_ARGS__)
#define                         DN_LogFlagWarningF(flags, fmt, ...)                    DN_LogPrintF(DN_LogTypeParamFromType(DN_LogType_Warning), DN_CallSiteNow, flags, fmt, ##__VA_ARGS__)
#define                         DN_LogFlagErrorF(flags, fmt, ...)                      DN_LogPrintF(DN_LogTypeParamFromType(DN_LogType_Error),   DN_CallSiteNow, flags, fmt, ##__VA_ARGS__)


// NOTE: OS primitives that the OS layer can provide for the base layer but is optional.
#if defined(DN_FREESTANDING)
#define                        DN_StackTraceFromArena(...) {}
#define                        DN_StackTraceFromAllocator(...) {}
#define                        DN_StackTraceIterate(...) false
#define                        DN_Str8FromStackTraceAllocator(...) DN_Str8Lit("N/A")
#define                        DN_Str8FromStackTraceArena(...) DN_Str8Lit("N/A")
#define                        DN_Str8FromStackTraceNowAllocator(...) DN_Str8Lit("N/A")
#define                        DN_Str8FromStackTraceNowArena(...) DN_Str8Lit("N/A")
#define                        DN_Str8FromStackTraceNowHeap(...) DN_Str8Lit("N/A")
#define                        DN_StackTraceGetFrames(...)
#define                        DN_StackTraceRawFrameToFrame(...)
#define                        DN_StackTracePrint(...)
#define                        DN_StackTraceReloadSymbols(...)
#else
DN_API DN_StackTrace           DN_StackTraceFromArena                                  (DN_Arena *arena, DN_U16 limit);
DN_API DN_StackTrace           DN_StackTraceFromAllocator                              (DN_Allocator allocator, DN_U16 limit);
DN_API bool                    DN_StackTraceIterate                                    (DN_StackTraceIterator *it, DN_StackTrace const *walk);
DN_API DN_Str8                 DN_Str8FromStackTraceAllocator                          (DN_Allocator allocator, DN_StackTrace const *walk, DN_U16 skip);
DN_API DN_Str8                 DN_Str8FromStackTraceArena                              (DN_Arena *arena, DN_StackTrace const *walk, DN_U16 skip);
DN_API DN_Str8                 DN_Str8FromStackTraceNowAllocator                       (DN_Arena *arena, DN_U16 limit, DN_U16 skip);
DN_API DN_Str8                 DN_Str8FromStackTraceNowArena                           (DN_Arena *arena, DN_U16 limit, DN_U16 skip);
DN_API DN_Str8                 DN_Str8FromStackTraceNowHeap                            (DN_U16 limit, DN_U16 skip);
DN_API DN_StackTraceFrameSlice DN_StackTraceGetFrames                                  (DN_Arena *arena, DN_U16 limit);
DN_API DN_StackTraceFrame      DN_StackTraceRawFrameToFrame                            (DN_Arena *arena, DN_StackTraceRawFrame raw_frame);
DN_API void                    DN_StackTracePrint                                      (DN_U16 limit);
DN_API void                    DN_StackTraceReloadSymbols                              ();
#endif

DN_API DN_F32                  DN_F32Lerp                                              (DN_F32 a, DN_F32 t, DN_F32 b);
DN_API DN_F32                  DN_F32Floor                                             (DN_F32 val);
DN_API DN_F32                  DN_F32Ceil                                              (DN_F32 val);
DN_API DN_F32                  DN_F32RoundHalfUp                                       (DN_F32 val);

#define                        DN_V2I32Zero                                            DN_Literal(DN_V2I32){{(DN_I32)(0),    (DN_I32)(0)}}
#define                        DN_V2I32One                                             DN_Literal(DN_V2I32){{(DN_I32)(1),    (DN_I32)(1)}}
#define                        DN_V2I32From1N(x)                                       DN_Literal(DN_V2I32){{(DN_I32)(x),    (DN_I32)(x)}}
#define                        DN_V2I32From2N(x, y)                                    DN_Literal(DN_V2I32){{(DN_I32)(x),    (DN_I32)(y)}}
#define                        DN_V2I32InitV2(xy)                                      DN_Literal(DN_V2I32){{(DN_I32)(xy).x, (DN_I32)(xy).y}}

DN_API bool                    operator!=                                              (DN_V2I32  lhs, DN_V2I32 rhs);
DN_API bool                    operator==                                              (DN_V2I32  lhs, DN_V2I32 rhs);
DN_API bool                    operator>=                                              (DN_V2I32  lhs, DN_V2I32 rhs);
DN_API bool                    operator<=                                              (DN_V2I32  lhs, DN_V2I32 rhs);
DN_API bool                    operator<                                               (DN_V2I32  lhs, DN_V2I32 rhs);
DN_API bool                    operator>                                               (DN_V2I32  lhs, DN_V2I32 rhs);
DN_API DN_V2I32                operator-                                               (DN_V2I32  lhs, DN_V2I32 rhs);
DN_API DN_V2I32                operator-                                               (DN_V2I32                 lhs);
DN_API DN_V2I32                operator+                                               (DN_V2I32  lhs, DN_V2I32 rhs);
DN_API DN_V2I32                operator*                                               (DN_V2I32  lhs, DN_V2I32 rhs);
DN_API DN_V2I32                operator*                                               (DN_V2I32  lhs, DN_F32   rhs);
DN_API DN_V2I32                operator*                                               (DN_V2I32  lhs, DN_I32   rhs);
DN_API DN_V2I32                operator/                                               (DN_V2I32  lhs, DN_V2I32 rhs);
DN_API DN_V2I32                operator/                                               (DN_V2I32  lhs, DN_F32   rhs);
DN_API DN_V2I32                operator/                                               (DN_V2I32  lhs, DN_I32   rhs);
DN_API DN_V2I32&               operator*=                                              (DN_V2I32& lhs, DN_V2I32 rhs);
DN_API DN_V2I32&               operator*=                                              (DN_V2I32& lhs, DN_F32   rhs);
DN_API DN_V2I32&               operator*=                                              (DN_V2I32& lhs, DN_I32   rhs);
DN_API DN_V2I32&               operator/=                                              (DN_V2I32& lhs, DN_V2I32 rhs);
DN_API DN_V2I32&               operator/=                                              (DN_V2I32& lhs, DN_F32   rhs);
DN_API DN_V2I32&               operator/=                                              (DN_V2I32& lhs, DN_I32   rhs);
DN_API DN_V2I32&               operator-=                                              (DN_V2I32& lhs, DN_V2I32 rhs);
DN_API DN_V2I32&               operator+=                                              (DN_V2I32& lhs, DN_V2I32 rhs);

DN_API DN_V2I32                DN_V2I32Min                                             (DN_V2I32 a, DN_V2I32 b);
DN_API DN_V2I32                DN_V2I32Max                                             (DN_V2I32 a, DN_V2I32 b);
DN_API DN_V2I32                DN_V2I32Abs                                             (DN_V2I32 a);

#define                        DN_V2U16Zero                                            DN_Literal(DN_V2U16){{(DN_U16)(0), (DN_U16)(0)}}
#define                        DN_V2U16One                                             DN_Literal(DN_V2U16){{(DN_U16)(1), (DN_U16)(1)}}
#define                        DN_V2U16From1N(x)                                       DN_Literal(DN_V2U16){{(DN_U16)(x), (DN_U16)(x)}}
#define                        DN_V2U16From2N(x, y)                                    DN_Literal(DN_V2U16){{(DN_U16)(x), (DN_U16)(y)}}

DN_API bool                    operator!=                                              (DN_V2U16  lhs, DN_V2U16 rhs);
DN_API bool                    operator==                                              (DN_V2U16  lhs, DN_V2U16 rhs);
DN_API bool                    operator>=                                              (DN_V2U16  lhs, DN_V2U16 rhs);
DN_API bool                    operator<=                                              (DN_V2U16  lhs, DN_V2U16 rhs);
DN_API bool                    operator<                                               (DN_V2U16  lhs, DN_V2U16 rhs);
DN_API bool                    operator>                                               (DN_V2U16  lhs, DN_V2U16 rhs);
DN_API DN_V2U16                operator-                                               (DN_V2U16  lhs, DN_V2U16 rhs);
DN_API DN_V2U16                operator+                                               (DN_V2U16  lhs, DN_V2U16 rhs);
DN_API DN_V2U16                operator*                                               (DN_V2U16  lhs, DN_V2U16 rhs);
DN_API DN_V2U16                operator*                                               (DN_V2U16  lhs, DN_F32 rhs);
DN_API DN_V2U16                operator*                                               (DN_V2U16  lhs, DN_I32 rhs);
DN_API DN_V2U16                operator/                                               (DN_V2U16  lhs, DN_V2U16 rhs);
DN_API DN_V2U16                operator/                                               (DN_V2U16  lhs, DN_F32 rhs);
DN_API DN_V2U16                operator/                                               (DN_V2U16  lhs, DN_I32 rhs);
DN_API DN_V2U16&               operator*=                                              (DN_V2U16& lhs, DN_V2U16 rhs);
DN_API DN_V2U16&               operator*=                                              (DN_V2U16& lhs, DN_F32 rhs);
DN_API DN_V2U16&               operator*=                                              (DN_V2U16& lhs, DN_I32 rhs);
DN_API DN_V2U16&               operator/=                                              (DN_V2U16& lhs, DN_V2U16 rhs);
DN_API DN_V2U16&               operator/=                                              (DN_V2U16& lhs, DN_F32 rhs);
DN_API DN_V2U16&               operator/=                                              (DN_V2U16& lhs, DN_I32 rhs);
DN_API DN_V2U16&               operator-=                                              (DN_V2U16& lhs, DN_V2U16 rhs);
DN_API DN_V2U16&               operator+=                                              (DN_V2U16& lhs, DN_V2U16 rhs);

#define                        DN_V2U32Zero                                            DN_Literal(DN_V2U32){{(DN_U32)(0), (DN_U32)(0)}}
#define                        DN_V2U32One                                             DN_Literal(DN_V2U32){{(DN_U32)(1), (DN_U32)(1)}}
#define                        DN_V2U32From1N(x)                                       DN_Literal(DN_V2U32){{(DN_U32)(x), (DN_U32)(x)}}
#define                        DN_V2U32From2N(x, y)                                    DN_Literal(DN_V2U32){{(DN_U32)(x), (DN_U32)(y)}}

#define                        DN_V2F32Zero                                            DN_Literal(DN_V2F32){{(DN_F32)(0),    (DN_F32)(0)}}
#define                        DN_V2F32One                                             DN_Literal(DN_V2F32){{(DN_F32)(1),    (DN_F32)(1)}}
#define                        DN_V2F32From1N(x)                                       DN_Literal(DN_V2F32){{(DN_F32)(x),    (DN_F32)(x)}}
#define                        DN_V2F32From2N(x, y)                                    DN_Literal(DN_V2F32){{(DN_F32)(x),    (DN_F32)(y)}}
#define                        DN_V2F32FromV2(xy)                                      DN_Literal(DN_V2F32){{(DN_F32)(xy).x, (DN_F32)(xy).y}}

DN_API DN_V2F32                DN_V2F32Lerp                                            (DN_V2F32 a, DN_F32 t, DN_V2F32 b);

DN_API bool                    operator!=                                              (DN_V2F32  lhs, DN_V2F32  rhs);
DN_API bool                    operator==                                              (DN_V2F32  lhs, DN_V2F32  rhs);
DN_API bool                    operator>=                                              (DN_V2F32  lhs, DN_V2F32  rhs);
DN_API bool                    operator<=                                              (DN_V2F32  lhs, DN_V2F32  rhs);
DN_API bool                    operator<                                               (DN_V2F32  lhs, DN_V2F32  rhs);
DN_API bool                    operator>                                               (DN_V2F32  lhs, DN_V2F32  rhs);

DN_API DN_V2F32                operator-                                               (DN_V2F32  lhs);
DN_API DN_V2F32                operator-                                               (DN_V2F32  lhs, DN_V2F32  rhs);
DN_API DN_V2F32                operator-                                               (DN_V2F32  lhs, DN_V2I32  rhs);
DN_API DN_V2F32                operator-                                               (DN_V2F32  lhs, DN_F32    rhs);
DN_API DN_V2F32                operator-                                               (DN_V2F32  lhs, DN_I32    rhs);

DN_API DN_V2F32                operator+                                               (DN_V2F32  lhs, DN_V2F32  rhs);
DN_API DN_V2F32                operator+                                               (DN_V2F32  lhs, DN_V2I32  rhs);
DN_API DN_V2F32                operator+                                               (DN_V2F32  lhs, DN_F32    rhs);
DN_API DN_V2F32                operator+                                               (DN_V2F32  lhs, DN_I32    rhs);

DN_API DN_V2F32                operator*                                               (DN_V2F32  lhs, DN_V2F32  rhs);
DN_API DN_V2F32                operator*                                               (DN_V2F32  lhs, DN_V2I32  rhs);
DN_API DN_V2F32                operator*                                               (DN_V2F32  lhs, DN_F32    rhs);
DN_API DN_V2F32                operator*                                               (DN_V2F32  lhs, DN_I32    rhs);

DN_API DN_V2F32                operator/                                               (DN_V2F32  lhs, DN_V2F32  rhs);
DN_API DN_V2F32                operator/                                               (DN_V2F32  lhs, DN_V2I32  rhs);
DN_API DN_V2F32                operator/                                               (DN_V2F32  lhs, DN_F32    rhs);
DN_API DN_V2F32                operator/                                               (DN_V2F32  lhs, DN_I32    rhs);

DN_API DN_V2F32&               operator*=                                              (DN_V2F32&  lhs, DN_V2F32 rhs);
DN_API DN_V2F32&               operator*=                                              (DN_V2F32&  lhs, DN_V2I32 rhs);
DN_API DN_V2F32&               operator*=                                              (DN_V2F32&  lhs, DN_F32   rhs);
DN_API DN_V2F32&               operator*=                                              (DN_V2F32&  lhs, DN_I32   rhs);

DN_API DN_V2F32&               operator/=                                              (DN_V2F32&  lhs, DN_V2F32 rhs);
DN_API DN_V2F32&               operator/=                                              (DN_V2F32&  lhs, DN_V2I32 rhs);
DN_API DN_V2F32&               operator/=                                              (DN_V2F32&  lhs, DN_F32   rhs);
DN_API DN_V2F32&               operator/=                                              (DN_V2F32&  lhs, DN_I32   rhs);

DN_API DN_V2F32&               operator-=                                              (DN_V2F32&  lhs, DN_V2F32 rhs);
DN_API DN_V2F32&               operator-=                                              (DN_V2F32&  lhs, DN_V2I32 rhs);
DN_API DN_V2F32&               operator-=                                              (DN_V2F32&  lhs, DN_F32   rhs);
DN_API DN_V2F32&               operator-=                                              (DN_V2F32&  lhs, DN_I32   rhs);

DN_API DN_V2F32&               operator+=                                              (DN_V2F32&  lhs, DN_V2F32 rhs);
DN_API DN_V2F32&               operator+=                                              (DN_V2F32&  lhs, DN_V2I32 rhs);
DN_API DN_V2F32&               operator+=                                              (DN_V2F32&  lhs, DN_F32   rhs);
DN_API DN_V2F32&               operator+=                                              (DN_V2F32&  lhs, DN_I32   rhs);

DN_API DN_V2F32                DN_V2F32Min                                             (DN_V2F32 a, DN_V2F32 b);
DN_API DN_V2F32                DN_V2F32Max                                             (DN_V2F32 a, DN_V2F32 b);
DN_API DN_V2F32                DN_V2F32Abs                                             (DN_V2F32 a);
DN_API DN_F32                  DN_V2F32Dot                                             (DN_V2F32 a, DN_V2F32 b);
DN_API DN_F32                  DN_V2F32LengthSq2V2                                     (DN_V2F32 lhs, DN_V2F32 rhs);
DN_API bool                    DN_V2F32LengthSqIsWithin2V2                             (DN_V2F32 lhs, DN_V2F32 rhs, DN_F32 within_amount_sq);
DN_API DN_F32                  DN_V2F32Length2V2                                       (DN_V2F32 lhs, DN_V2F32 rhs);
DN_API DN_F32                  DN_V2F32LengthSq                                        (DN_V2F32 lhs);
DN_API DN_F32                  DN_V2F32Length                                          (DN_V2F32 lhs);
DN_API DN_V2F32                DN_V2F32Normalise                                       (DN_V2F32 a);
DN_API DN_V2F32                DN_V2F32Perpendicular                                   (DN_V2F32 a);
DN_API DN_V2F32                DN_V2F32Reflect                                         (DN_V2F32 in, DN_V2F32 surface);
DN_API DN_F32                  DN_V2F32Area                                            (DN_V2F32 a);

#define                        DN_V3F32From1N(x)                                       DN_Literal(DN_V3F32){{(DN_F32)(x),    (DN_F32)(x),    (DN_F32)(x)}}
#define                        DN_V3F32From3N(x, y, z)                                 DN_Literal(DN_V3F32){{(DN_F32)(x),    (DN_F32)(y),    (DN_F32)(z)}}
#define                        DN_V3F32FromV2F32And1N(xy, z)                           DN_Literal(DN_V3F32){{(DN_F32)(xy.x), (DN_F32)(xy.y), (DN_F32)(z)}}

// NOTE: Grayscale co-efficients from:
//   https://github.com/EpicGames/UnrealEngine/blob/260bb2e1c5610b31c63a36206eedd289409c5f11/Engine/Source/Runtime/Core/Private/Math/Color.cpp#L304
DN_V3F32 static const          DN_V3F32_RGB_LUMINANCE = DN_V3F32From3N(0.3f, 0.59f, 0.11f);

DN_API bool                    operator==                                              (DN_V3F32  lhs, DN_V3F32  rhs);
DN_API bool                    operator!=                                              (DN_V3F32  lhs, DN_V3F32  rhs);
DN_API bool                    operator>=                                              (DN_V3F32  lhs, DN_V3F32  rhs);
DN_API bool                    operator<=                                              (DN_V3F32  lhs, DN_V3F32  rhs);
DN_API bool                    operator<                                               (DN_V3F32  lhs, DN_V3F32  rhs);
DN_API bool                    operator>                                               (DN_V3F32  lhs, DN_V3F32  rhs);
DN_API DN_V3F32                operator-                                               (DN_V3F32  lhs, DN_V3F32  rhs);
DN_API DN_V3F32                operator-                                               (DN_V3F32  lhs);
DN_API DN_V3F32                operator+                                               (DN_V3F32  lhs, DN_V3F32 rhs);
DN_API DN_V3F32                operator*                                               (DN_V3F32  lhs, DN_V3F32 rhs);
DN_API DN_V3F32                operator*                                               (DN_V3F32  lhs, DN_F32   rhs);
DN_API DN_V3F32                operator*                                               (DN_V3F32  lhs, DN_I32   rhs);
DN_API DN_V3F32                operator/                                               (DN_V3F32  lhs, DN_V3F32 rhs);
DN_API DN_V3F32                operator/                                               (DN_V3F32  lhs, DN_F32   rhs);
DN_API DN_V3F32                operator/                                               (DN_V3F32  lhs, DN_I32   rhs);
DN_API DN_V3F32&               operator*=                                              (DN_V3F32 &lhs, DN_V3F32 rhs);
DN_API DN_V3F32&               operator*=                                              (DN_V3F32 &lhs, DN_F32   rhs);
DN_API DN_V3F32&               operator*=                                              (DN_V3F32 &lhs, DN_I32   rhs);
DN_API DN_V3F32&               operator/=                                              (DN_V3F32 &lhs, DN_V3F32 rhs);
DN_API DN_V3F32&               operator/=                                              (DN_V3F32 &lhs, DN_F32   rhs);
DN_API DN_V3F32&               operator/=                                              (DN_V3F32 &lhs, DN_I32   rhs);
DN_API DN_V3F32&               operator-=                                              (DN_V3F32 &lhs, DN_V3F32 rhs);
DN_API DN_V3F32&               operator+=                                              (DN_V3F32 &lhs, DN_V3F32 rhs);
DN_API DN_V3F32                DN_V3F32Lerp                                            (DN_V3F32 lhs, DN_F32 t01, DN_V3F32 rhs);
DN_API DN_F32                  DN_V3F32LengthSq                                        (DN_V3F32 a);
DN_API DN_F32                  DN_V3F32Length                                          (DN_V3F32 a);
DN_API DN_V3F32                DN_V3F32Normalise                                       (DN_V3F32 a);

#define                        DN_V4F32From1N(x)                                       DN_Literal(DN_V4F32){{(DN_F32)(x), (DN_F32)(x), (DN_F32)(x), (DN_F32)(x)}}
#define                        DN_V4F32From4N(x, y, z, w)                              DN_Literal(DN_V4F32){{(DN_F32)(x), (DN_F32)(y), (DN_F32)(z), (DN_F32)(w)}}
#define                        DN_V4F32FromV3And1N(xyz, w)                             DN_Literal(DN_V4F32){{xyz.x,        xyz.y,        xyz.z,        w}}
#define                        DN_V4F32RGBA01FromRGBAU8(r, g, b, a)                    DN_Literal(DN_V4F32){{r / 255.f, g / 255.f, b / 255.f, a / 255.f}}
#define                        DN_V4F32RGBA01FromRGBU8(r, g, b)                        DN_Literal(DN_V4F32){{r / 255.f, g / 255.f, b / 255.f,       1.f}}

DN_API DN_V4F32                DN_V4F32Lerp                                            (DN_V4F32 lhs, DN_F32 t01, DN_V4F32 rhs);
DN_API bool                    DN_V4F32RGBA01IsValid                                   (DN_V4F32 rgba01);
DN_API DN_V4F32                DN_V4F32RGBA01FromRGBU32                                (DN_U32 u32);
DN_API DN_V4F32                DN_V4F32RGBA01FromRGBAU32                               (DN_U32 u32);
DN_API DN_V4F32                DN_V4F32Linear01FromSRGB01                              (DN_V4F32 rgb01);
DN_API DN_V4F32                DN_V4F32Linear01Desaturate                                      (DN_V4F32 linear01, DN_F32 t01);
DN_API DN_V4F32                DN_V4F32SRGB01FromLinear01                              (DN_V4F32 linear01);

#define                        DN_V4F32FromV4Alpha(v4, alpha)                          DN_V4F32FromV3And1N(v4.xyz, alpha)

DN_API bool                    operator==                                              (DN_V4F32  lhs, DN_V4F32  rhs);
DN_API bool                    operator!=                                              (DN_V4F32  lhs, DN_V4F32  rhs);
DN_API bool                    operator<=                                              (DN_V4F32  lhs, DN_V4F32  rhs);
DN_API bool                    operator<                                               (DN_V4F32  lhs, DN_V4F32  rhs);
DN_API bool                    operator>                                               (DN_V4F32  lhs, DN_V4F32  rhs);
DN_API DN_V4F32                operator-                                               (DN_V4F32  lhs, DN_V4F32  rhs);
DN_API DN_V4F32                operator-                                               (DN_V4F32  lhs);
DN_API DN_V4F32                operator+                                               (DN_V4F32  lhs, DN_V4F32 rhs);
DN_API DN_V4F32                operator*                                               (DN_V4F32  lhs, DN_V4F32 rhs);
DN_API DN_V4F32                operator*                                               (DN_V4F32  lhs, DN_F32   rhs);
DN_API DN_V4F32                operator*                                               (DN_V4F32  lhs, DN_I32   rhs);
DN_API DN_V4F32                operator/                                               (DN_V4F32  lhs, DN_F32   rhs);
DN_API DN_V4F32&               operator*=                                              (DN_V4F32 &lhs, DN_V4F32 rhs);
DN_API DN_V4F32&               operator*=                                              (DN_V4F32 &lhs, DN_F32   rhs);
DN_API DN_V4F32&               operator*=                                              (DN_V4F32 &lhs, DN_I32   rhs);
DN_API DN_V4F32&               operator-=                                              (DN_V4F32 &lhs, DN_V4F32 rhs);
DN_API DN_V4F32&               operator+=                                              (DN_V4F32 &lhs, DN_V4F32 rhs);
DN_API DN_F32                  DN_V4F32Dot                                             (DN_V4F32 a, DN_V4F32 b);

DN_API DN_M4                   DN_M4Identity                                           ();
DN_API DN_M4                   DN_M4ScaleF                                             (DN_F32 x, DN_F32 y, DN_F32 z);
DN_API DN_M4                   DN_M4Scale                                              (DN_V3F32 xyz);
DN_API DN_M4                   DN_M4TranslateF                                         (DN_F32 x, DN_F32 y, DN_F32 z);
DN_API DN_M4                   DN_M4Translate                                          (DN_V3F32 xyz);
DN_API DN_M4                   DN_M4Transpose                                          (DN_M4 mat);
DN_API DN_M4                   DN_M4Rotate                                             (DN_V3F32 axis, DN_F32 radians);
DN_API DN_M4                   DN_M4Orthographic                                       (DN_F32 left, DN_F32 right, DN_F32 bottom, DN_F32 top, DN_F32 z_near, DN_F32 z_far);
DN_API DN_M4                   DN_M4Perspective                                        (DN_F32 fov /*radians*/, DN_F32 aspect, DN_F32 z_near, DN_F32 z_far);
DN_API DN_M4                   DN_M4Add                                                (DN_M4 lhs, DN_M4 rhs);
DN_API DN_M4                   DN_M4Sub                                                (DN_M4 lhs, DN_M4 rhs);
DN_API DN_M4                   DN_M4Mul                                                (DN_M4 lhs, DN_M4 rhs);
DN_API DN_M4                   DN_M4Div                                                (DN_M4 lhs, DN_M4 rhs);
DN_API DN_M4                   DN_M4AddF                                               (DN_M4 lhs, DN_F32 rhs);
DN_API DN_M4                   DN_M4SubF                                               (DN_M4 lhs, DN_F32 rhs);
DN_API DN_M4                   DN_M4MulF                                               (DN_M4 lhs, DN_F32 rhs);
DN_API DN_M4                   DN_M4DivF                                               (DN_M4 lhs, DN_F32 rhs);
DN_API DN_Str8x256             DN_M4ColumnMajorString                                  (DN_M4 mat);

DN_API bool                    DN_M2x3Eq                                               (DN_M2x3 const *lhs, DN_M2x3 const *rhs);
DN_API bool                    DN_M2x3NotEq                                            (DN_M2x3 const *lhs, DN_M2x3 const *rhs);
DN_API DN_M2x3                 DN_M2x3Identity                                         ();
DN_API DN_M2x3                 DN_M2x3Translate                                        (DN_V2F32 offset);
DN_API DN_V2F32                DN_M2x3ScaleGet                                         (DN_M2x3 m2x3);
DN_API DN_M2x3                 DN_M2x3Scale                                            (DN_V2F32 scale);
DN_API DN_M2x3                 DN_M2x3Rotate                                           (DN_F32 radians);
DN_API DN_M2x3                 DN_M2x3ProjFromV2F32                                    (DN_V2F32 size, DN_M2x3ProjOrigin origin);
DN_API DN_M2x3XForm            DN_M2x3XFormFrom2M2x3                                   (DN_M2x3 forward, DN_M2x3 inverse);
DN_API DN_M2x3XForm            DN_M2x3XFormFromTRS                                     (DN_V2F32 pos, DN_V2F32 scale, DN_F32 rotate_rads, DN_V2F32 pivot_pos);
DN_API DN_M2x3XForm            DN_M2x3XFormIdentity                                    ();
DN_API DN_M2x3XForm            DN_M2x3XFormMul                                         (DN_M2x3XForm m1, DN_M2x3XForm m2);
DN_API DN_M2x3                 DN_M2x3Mul                                              (DN_M2x3 m1, DN_M2x3 m2);
DN_API DN_V2F32                DN_M2x3Mul2F32                                          (DN_M2x3 m1, DN_F32 x, DN_F32 y);
DN_API DN_V2F32                DN_M2x3MulV2F32                                         (DN_M2x3 m1, DN_V2F32 v2);
DN_API DN_Rect                 DN_M2x3MulRect                                          (DN_M2x3 m1, DN_Rect rect);

#define                        DN_RectFrom2V2(pos, size)                               DN_Literal(DN_Rect){(pos), (size)}
#define                        DN_RectFrom4N(x, y, w, h)                               DN_Literal(DN_Rect){DN_Literal(DN_V2F32){{x, y}}, DN_Literal(DN_V2F32){{w, h}}}
#define                        DN_RectZero                                             DN_RectFrom4N(0, 0, 0, 0)

DN_API DN_V2F32                DN_RectCenter                                           (DN_Rect rect);
DN_API bool                    DN_RectContainsPoint                                    (DN_Rect rect, DN_V2F32 p);
DN_API bool                    DN_RectContainsRect                                     (DN_Rect a, DN_Rect b);
DN_API DN_Rect                 DN_RectExpand                                           (DN_Rect a, DN_F32 amount);
DN_API DN_Rect                 DN_RectExpandV2                                         (DN_Rect a, DN_V2F32 amount);
DN_API bool                    DN_RectIntersects                                       (DN_Rect a, DN_Rect b);
DN_API DN_Rect                 DN_RectIntersection                                     (DN_Rect a, DN_Rect b);
DN_API DN_Rect                 DN_RectUnion                                            (DN_Rect a, DN_Rect b);
DN_API DN_2V2F32               DN_RectRange                                            (DN_Rect a);
DN_API bool                    DN_RectEq                                               (DN_Rect lhs, DN_Rect rhs);
DN_API DN_F32                  DN_RectArea                                             (DN_Rect a);
DN_API DN_V2F32                DN_RectInterpV2F32                                      (DN_Rect rect, DN_V2F32 t01);
DN_API DN_V2F32                DN_RectTopLeft                                          (DN_Rect rect);
DN_API DN_V2F32                DN_RectTopRight                                         (DN_Rect rect);
DN_API DN_V2F32                DN_RectBottomLeft                                       (DN_Rect rect);
DN_API DN_V2F32                DN_RectBottomRight                                      (DN_Rect rect);

DN_API DN_Rect                 DN_RectCutLeftClip                                      (DN_Rect *rect, DN_F32 amount, DN_RectCutClip clip);
DN_API DN_Rect                 DN_RectCutRightClip                                     (DN_Rect *rect, DN_F32 amount, DN_RectCutClip clip);
DN_API DN_Rect                 DN_RectCutTopClip                                       (DN_Rect *rect, DN_F32 amount, DN_RectCutClip clip);
DN_API DN_Rect                 DN_RectCutBottomClip                                    (DN_Rect *rect, DN_F32 amount, DN_RectCutClip clip);

#define                        DN_RectCutLeft(rect, amount)                            DN_RectCutLeftClip(rect, amount, DN_RectCutClip_Yes)
#define                        DN_RectCutRight(rect, amount)                           DN_RectCutRightClip(rect, amount, DN_RectCutClip_Yes)
#define                        DN_RectCutTop(rect, amount)                             DN_RectCutTopClip(rect, amount, DN_RectCutClip_Yes)
#define                        DN_RectCutBottom(rect, amount)                          DN_RectCutBottomClip(rect, amount, DN_RectCutClip_Yes)

#define                        DN_RectCutLeftNoClip(rect, amount)                      DN_RectCutLeftClip(rect, amount, DN_RectCutClip_No)
#define                        DN_RectCutRightNoClip(rect, amount)                     DN_RectCutRightClip(rect, amount, DN_RectCutClip_No)
#define                        DN_RectCutTopNoClip(rect, amount)                       DN_RectCutTopClip(rect, amount, DN_RectCutClip_No)
#define                        DN_RectCutBottomNoClip(rect, amount)                    DN_RectCutBottomClip(rect, amount, DN_RectCutClip_No)

DN_API DN_Rect                 DN_RectCutCut                                           (DN_RectCut rect_cut, DN_V2F32 size, DN_RectCutClip clip);
#define                        DN_RectCutInit(rect, side)                              DN_Literal(DN_RectCut){rect, side}

DN_API DN_RaycastV2            DN_RaycastLineIntersectV2                               (DN_V2F32 origin_a, DN_V2F32 dir_a, DN_V2F32 origin_b, DN_V2F32 dir_b);

// NOTE: Containers that are imlpemented using primarily macros for operating on data structures
// that are embedded into a C style struct or from a set of defined variables from the available
// scope. Keep it stupid simple, structs and functions. Minimal amount of container types with
// flexible construction leads to less duplicated container code and less template meta-programming.

// NOTE: Intrusive Singly Linked List
//   Define a struct with the members `next`:
//
//     struct MyLinkItem {
//       int         data;
//       MyLinkItem *next;
//     } my_link = {};
//
//     MyLinkItem *first_item = DN_ISinglyLLDetach(&my_link, MyLinkItem);
#define DN_ISinglyLLDetach(list) (decltype(list))DN_SinglyLLDetach((void **)&(list), (void **)&(list)->next)

// NOTE: Singly Linked List with Head and Tail pointer
/*
  struct MyLinkItem {
    int         data;
    MyLinkItem *next;
  } my_list = {};

  struct MyContainer {
    MyLinkItem *head;
    MyLinkItem *tail;
  };

  MyLinkItem  item = {};
  MyContainer container = {};
  DN_ISinglyHeadTailLLAppend(container, item);
  // ... or alternatively, DN_SinglyHeadTailLLAppend(container.head, container.tail, item);

  for (MyLinkItem *it = container.head; it; it = it->next) { }
*/
#define DN_SinglyHeadTailLLAppend(head, tail, to_append) \
  do {                                                   \
    if (!head)                                           \
      head = to_append;                                  \
    if (tail)                                            \
      tail->next = to_append;                            \
    tail = to_append;                                    \
  } while (0)
#define DN_ISinglyHeadTailLLAppend(container_ptr, to_append) DN_SinglyHeadTailLLAppend((container_ptr)->head, (container_ptr)->tail, to_append)

// NOTE: Sentinel Doubly Linked List
//   Uses a sentinel/dummy node as the list head. The sentinel points to itself when empty.
//   Define a struct with the members `next` and `prev`:
//
//     struct MyLinkItem {
//       int         data;
//       MyLinkItem *next;
//       MyLinkItem *prev;
//     } my_list = {};
//
//     DN_SentinelDoublyLLInit(&my_list);
//     DN_SentinelDoublyLLAppend(&my_list, &new_item);
//     DN_SentinelDoublyLLForEach(it, &my_list) { /* ... */ }
//
#define DN_SentinelDoublyLLInit(list)             (list)->next = (list)->prev = (list)
#define DN_SentinelDoublyLLIsSentinel(list, item) ((list) == (item))
#define DN_SentinelDoublyLLIsEmpty(list)          (!(list) || ((list) == (list)->next))
#define DN_SentinelDoublyLLIsInit(list)           ((list)->next && (list)->prev)
#define DN_SentinelDoublyLLHasItems(list)         ((list) && ((list) != (list)->next))
#define DN_SentinelDoublyLLForEach(it, list)      auto *it = (list)->next; (it) != (list); (it) = (it)->next

#define DN_SentinelDoublyLLInitArena(list, T, ptr_arena)   \
  do {                                                     \
    (list) = DN_ArenaNew(ptr_arena, T, DN_ZMem_Yes); \
    DN_SentinelDoublyLLInit(list);                         \
  } while (0)

#define DN_SentinelDoublyLLInitPool(list, T, pool) \
  do {                                             \
    (list) = DN_PoolNew(pool, T);                  \
    DN_SentinelDoublyLLInit(list);                 \
  } while (0)

#define DN_SentinelDoublyLLDetach(item)  \
  do {                                   \
    if (item) {                          \
      (item)->prev->next = (item)->next; \
      (item)->next->prev = (item)->prev; \
      (item)->next       = nullptr;      \
      (item)->prev       = nullptr;      \
    }                                    \
  } while (0)

#define DN_SentinelDoublyLLDequeue(list, dest_ptr) \
  if (DN_SentinelDoublyLLHasItems(list)) {         \
    dest_ptr = (list)->next;                       \
    DN_SentinelDoublyLLDetach(dest_ptr);           \
  }

#define DN_SentinelDoublyLLAppend(list, item) \
  do {                                        \
    if (item) {                               \
      if ((item)->next)                       \
        DN_SentinelDoublyLLDetach(item);      \
      (item)->next       = (list)->next;      \
      (item)->prev       = (list);            \
      (item)->next->prev = (item);            \
      (item)->prev->next = (item);            \
    }                                         \
  } while (0)

#define DN_SentinelDoublyLLPrepend(list, item) \
  do {                                         \
    if (item) {                                \
      if ((item)->next)                        \
        DN_SentinelDoublyLLDetach(item);       \
      (item)->next       = (list);             \
      (item)->prev       = (list)->prev;       \
      (item)->next->prev = (item);             \
      (item)->prev->next = (item);             \
    }                                          \
  } while (0)

// NOTE: Doubly Linked List
//   Define a struct with the members `next` and `prev`. This list has null pointers for head->prev
//   and tail->next.
//
//     struct MyLinkItem {
//       int         data;
//       MyLinkItem *next;
//       MyLinkItem *prev;
//     } my_link = {};
//
//     MyLinkItem first_item = {}, second_item = {};
//     DN_DoublyLLAppend(&first_item, &second_item); // first_item -> second_item
//
#define DN_DoublyLLDetach(head, ptr)     \
  do {                                   \
    if ((head) && (head) == (ptr))       \
      (head) = (head)->next;             \
    if ((ptr)) {                         \
      if ((ptr)->next)                   \
        (ptr)->next->prev = (ptr)->prev; \
      if ((ptr)->prev)                   \
        (ptr)->prev->next = (ptr)->next; \
      (ptr)->prev = (ptr)->next = 0;     \
    }                                    \
  } while (0)

#define DN_DoublyLLAppend(head, ptr)                                                \
  do {                                                                              \
    if ((ptr)) {                                                                    \
      DN_AssertF((ptr)->prev == 0 && (ptr)->next == 0, "Detach the pointer first"); \
      (ptr)->prev = (head);                                                         \
      (ptr)->next = 0;                                                              \
      if ((head)) {                                                                 \
        (ptr)->next  = (head)->next;                                                \
        (head)->next = (ptr);                                                       \
        if ((ptr)->next)                                                            \
          (ptr)->next->prev = (ptr);                                                \
      } else {                                                                      \
        (head) = (ptr);                                                             \
      }                                                                             \
    }                                                                               \
  } while (0)

#define DN_DoublyLLPrepend(head, ptr)                                               \
  do {                                                                              \
    if ((ptr)) {                                                                    \
      DN_AssertF((ptr)->prev == 0 && (ptr)->next == 0, "Detach the pointer first"); \
      (ptr)->prev = nullptr;                                                        \
      (ptr)->next = (head);                                                         \
      if ((head)) {                                                                 \
        (ptr)->prev  = (head)->prev;                                                \
        (head)->prev = (ptr);                                                       \
        if ((ptr)->prev)                                                            \
          (ptr)->prev->next = (ptr);                                                \
      } else {                                                                      \
        (head) = (ptr);                                                             \
      }                                                                             \
    }                                                                               \
  } while (0)

// NOTE: For C++ we need to cast the void* returned in these functions to the concrete type. In C,
// no cast is needed.
#if defined(__cplusplus)
  #define DN_CppDeclType(x) decltype(x)
#else
  #define DN_CppDeclType
#endif

// NOTE: Arrays
//   Data structures that have a `T *data`, `DN_USize count` and `DN_USize max` capacity that can be
//   dynamically shrunk or expanded.
//
//   API
//     ResizeFrom:   Resizes the array to `new_max` erase elements if resizing to a smaller size
//     GrowFrom:     Expands the capacity of the array if `new_max > array.max` otherwise no-op
//     GrowIfNeeded: Expands the capacity of the array if `array.size + add_count > array.max` otherwise no-op
//
//   Variants
//     PArray => Pointer (to) Array
//     LArray => Literal Array
//       Define a C array and size. (P) array macros take a pointer to the aray, its size and its max
//       capacity. The (L) array macros take the literal array and derives the max capacity
//       automatically using DN_ArrayCountU(l_array).
//
//         MyStruct buffer[TB_ASType_Count] = {};
//         DN_USize size                    = 0;
//         MyStruct *item_0                 = DN_PArrayMake(buffer, &size, DN_ArrayCountU(buffer), DN_ZMem_No);
//         MyStruct *item_1                 = DN_LArrayMake(buffer, &size, DN_ZMem_No);
//
//     IArray => Intrusive Array
//       Define a struct with the members `data`, `count` and `max`:
//
//         struct MyArray {
//           MyStruct *data;
//           DN_USize  count;
//           DN_USize  max;
//         } my_array = {};
//         DN_Arena arena = {};
//         DN_IArrayResizeFromArena(&my_array, &arena, 256);
//         MyStruct *item = DN_IArrayMake(&my_array, DN_ZMem_No);
//
#if defined(__cplusplus)
  #define DN_PArrayFind(ptr, size, ptr_find, eq_func)                          DN_TArrayFind(ptr, size, ptr_find, eq_func)
  #define DN_PArrayFindMemEq(ptr, size, ptr_find)                              DN_TArrayFindMemEq(ptr, size, ptr_find)
  #define DN_PArrayResizeFromPool(ptr, ptr_size, ptr_max, pool, new_max)       DN_TArrayResizeFromPool(&(ptr), ptr_size, ptr_max, pool, new_max)
  #define DN_PArrayResizeFromArena(ptr, ptr_size, ptr_max, arena, new_max)     DN_TArrayResizeFromArena(&(ptr), ptr_size, ptr_max, arena, new_max)
  #define DN_PArrayGrowFromPool(ptr, size, ptr_max, pool, new_max)             DN_TArrayGrowFromPool(&(ptr), size, ptr_max, pool, new_max)
  #define DN_PArrayGrowFromArena(ptr, size, ptr_max, arena, new_max)           DN_TArrayGrowFromArena(&(ptr), size, ptr_max, arena, new_max)
  #define DN_PArrayGrowIfNeededFromPool(ptr, size, ptr_max, pool, add_count)   DN_TArrayGrowIfNeededFromPool(ptr, size, ptr_max, pool, add_count)
  #define DN_PArrayGrowIfNeededFromArena(ptr, size, ptr_max, arena, add_count) DN_TArrayGrowIfNeededFromArena(ptr, size, ptr_max, arena, add_count)

  #define DN_PArrayMakeArray(ptr, ptr_size, max, count, z_mem)                 DN_TArrayMakeArray(ptr, ptr_size, max, count, z_mem)
  #define DN_PArrayMakeArrayZ(ptr, ptr_size, max, count)                       DN_TArrayMakeArray(ptr, ptr_size, max, count, DN_ZMem_Yes)
  #define DN_PArrayMakeArrayNoZ(ptr, ptr_size, max, count)                     DN_TArrayMakeArray(ptr, ptr_size, max, count, DN_ZMem_No)
  #define DN_PArrayMakeArrayAssert(ptr, ptr_size, max, count, z_mem)           DN_TArrayMakeArrayAssert(ptr, ptr_size, max, count, z_mem, DN_CallSiteNow)
  #define DN_PArrayMakeArrayAssertZ(ptr, ptr_size, max, count)                 DN_TArrayMakeArrayAssert(ptr, ptr_size, max, count, DN_ZMem_Yes, DN_CallSiteNow)
  #define DN_PArrayMakeArrayAssertNoZ(ptr, ptr_size, max, count)               DN_TArrayMakeArrayAssert(ptr, ptr_size, max, count, DN_ZMem_No, DN_CallSiteNow)

  #define DN_PArrayMake(ptr, ptr_size, max, z_mem)                             DN_TArrayMakeArray(ptr, ptr_size, max, 1, z_mem)
  #define DN_PArrayMakeZ(ptr, ptr_size, max)                                   DN_TArrayMakeArray(ptr, ptr_size, max, 1, DN_ZMem_Yes)
  #define DN_PArrayMakeNoZ(ptr, ptr_size, max)                                 DN_TArrayMakeArray(ptr, ptr_size, max, 1, DN_ZMem_No)
  #define DN_PArrayMakeAssert(ptr, ptr_size, max, z_mem)                       DN_TArrayMakeArrayAssert(ptr, ptr_size, max, 1, z_mem, DN_CallSiteNow)
  #define DN_PArrayMakeAssertZ(ptr, ptr_size, max)                             DN_TArrayMakeArrayAssert(ptr, ptr_size, max, 1, DN_ZMem_Yes, DN_CallSiteNow)
  #define DN_PArrayMakeAssertNoZ(ptr, ptr_size, max)                           DN_TArrayMakeArrayAssert(ptr, ptr_size, max, 1, DN_ZMem_No, DN_CallSiteNow)

  #define DN_PArrayAddArray(ptr, ptr_size, max, items, count, add)             DN_TArrayAddArray(ptr, ptr_size, max, items, count, add)
  #define DN_PArrayAdd(ptr, ptr_size, max, item, add)                          DN_TArrayAddArray(ptr, ptr_size, max, &item, 1, add)
  #define DN_PArrayAppendArray(ptr, ptr_size, max, items, count)               DN_TArrayAddArray(ptr, ptr_size, max, items, count, DN_ArrayAdd_Append)
  #define DN_PArrayAppend(ptr, ptr_size, max, item)                            DN_TArrayAddArray(ptr, ptr_size, max, &item, 1, DN_ArrayAdd_Append)
  #define DN_PArrayAppendAssert(ptr, ptr_size, max, item)                      DN_TArrayAddArrayAssert(ptr, ptr_size, max, &item, 1, DN_ArrayAdd_Append, DN_CallSiteNow)
  #define DN_PArrayPrependArray(ptr, ptr_size, max, items, count)              DN_TArrayAddArray(ptr, ptr_size, max, items, count, DN_ArrayAdd_Prepend)
  #define DN_PArrayPrepend(ptr, ptr_size, max, item)                           DN_TArrayAddArray(ptr, ptr_size, max, &item, 1, DN_ArrayAdd_Prepend)

  #define DN_PArrayEraseRange(ptr, ptr_size, begin_index, count, erase)        DN_TArrayEraseRange(ptr, ptr_size, begin_index, count, erase)
  #define DN_PArrayErase(ptr, ptr_size, index, erase)                          DN_TArrayEraseRange(ptr, ptr_size, index, 1, erase)
  #define DN_PArrayInsertArray(ptr, ptr_size, max, index, items, count)        DN_TArrayInsertArray(ptr, ptr_size, max, index, items, count)
  #define DN_PArrayInsert(ptr, ptr_size, max, index, item)                     DN_TArrayInsertArray(ptr, ptr_size, max, index, &item, 1)
  #define DN_PArrayPopFront(ptr, ptr_size, max, count)                         DN_TArrayPopFront(ptr, ptr_size, count)
  #define DN_PArrayPopBack(ptr, ptr_size, max, count)                          DN_TArrayPopBack(ptr, ptr_size, count)
#else
  #define DN_PArrayFind(ptr, size, ptr_find, eq_func)                          DN_ArrayFind(ptr, size, sizeof(*(ptr)), ptr_find, eq_func)
  #define DN_PArrayFindMemEq(ptr, size, ptr_find)                              DN_ArrayFindMemEq(ptr, size, sizeof(*(ptr)), ptr_find)
  #define DN_PArrayResizeFromPool(ptr, ptr_size, ptr_max, pool, new_max)       DN_ArrayResizeFromPool((void **)&(ptr), ptr_size, ptr_max, sizeof((ptr)[0]), pool, new_max)
  #define DN_PArrayResizeFromArena(ptr, ptr_size, ptr_max, arena, new_max)     DN_ArrayResizeFromArena((void **)&(ptr), ptr_size, ptr_max, sizeof((ptr)[0]), arena, new_max)
  #define DN_PArrayGrowFromPool(ptr, size, ptr_max, pool, new_max)             DN_ArrayGrowFromPool((void **)&(ptr), size, ptr_max, sizeof((ptr)[0]), pool, new_max)
  #define DN_PArrayGrowFromArena(ptr, size, ptr_max, arena, new_max)           DN_ArrayGrowFromArena((void **)&(ptr), size, ptr_max, sizeof((ptr)[0]), arena, new_max)
  #define DN_PArrayGrowIfNeededFromPool(ptr, size, ptr_max, pool, add_count)   DN_ArrayGrowIfNeededFromPool((void **)(ptr), size, ptr_max, sizeof((*ptr)[0]), pool, add_count)
  #define DN_PArrayGrowIfNeededFromArena(ptr, size, ptr_max, arena, add_count) DN_ArrayGrowIfNeededFromArena((void **)(ptr), size, ptr_max, sizeof((*ptr)[0]), arena, add_count)

  #define DN_PArrayMakeArray(ptr, ptr_size, max, count, z_mem)                 (DN_CppDeclType(&(ptr)[0]))DN_ArrayMakeArray(ptr, ptr_size, max, sizeof((ptr)[0]), count, z_mem)
  #define DN_PArrayMakeArrayZ(ptr, ptr_size, max, count)                       (DN_CppDeclType(&(ptr)[0]))DN_ArrayMakeArray(ptr, ptr_size, max, sizeof((ptr)[0]), count, DN_ZMem_Yes)
  #define DN_PArrayMakeArrayNoZ(ptr, ptr_size, max, count)                     (DN_CppDeclType(&(ptr)[0]))DN_ArrayMakeArray(ptr, ptr_size, max, sizeof((ptr)[0]), count, DN_ZMem_No)
  #define DN_PArrayMakeArrayAssert(ptr, ptr_size, max, count, z_mem)           (DN_CppDeclType(&(ptr)[0]))DN_ArrayMakeArrayAssert(ptr, ptr_size, max, sizeof((ptr)[0]), count, z_mem, DN_CallSiteNow)
  #define DN_PArrayMakeArrayAssertZ(ptr, ptr_size, max, count)                 (DN_CppDeclType(&(ptr)[0]))DN_ArrayMakeArrayAssert(ptr, ptr_size, max, sizeof((ptr)[0]), count, DN_ZMem_Yes, DN_CallSiteNow)
  #define DN_PArrayMakeArrayAssertNoZ(ptr, ptr_size, max, count)               (DN_CppDeclType(&(ptr)[0]))DN_ArrayMakeArrayAssert(ptr, ptr_size, max, sizeof((ptr)[0]), count, DN_ZMem_No, DN_CallSiteNow)

  #define DN_PArrayMake(ptr, ptr_size, max, z_mem)                             (DN_CppDeclType(&(ptr)[0]))DN_ArrayMakeArray(ptr, ptr_size, max, sizeof((ptr)[0]), 1, z_mem)
  #define DN_PArrayMakeZ(ptr, ptr_size, max)                                   (DN_CppDeclType(&(ptr)[0]))DN_ArrayMakeArray(ptr, ptr_size, max, sizeof((ptr)[0]), 1, DN_ZMem_Yes)
  #define DN_PArrayMakeNoZ(ptr, ptr_size, max)                                 (DN_CppDeclType(&(ptr)[0]))DN_ArrayMakeArray(ptr, ptr_size, max, sizeof((ptr)[0]), 1, DN_ZMem_No)
  #define DN_PArrayMakeAssert(ptr, ptr_size, max, z_mem)                       (DN_CppDeclType(&(ptr)[0]))DN_ArrayMakeArrayAssert(ptr, ptr_size, max, sizeof((ptr)[0]), 1, z_mem, DN_CallSiteNow)
  #define DN_PArrayMakeAssertZ(ptr, ptr_size, max)                             (DN_CppDeclType(&(ptr)[0]))DN_ArrayMakeArrayAssert(ptr, ptr_size, max, sizeof((ptr)[0]), 1, DN_ZMem_Yes, DN_CallSiteNow)
  #define DN_PArrayMakeAssertNoZ(ptr, ptr_size, max)                           (DN_CppDeclType(&(ptr)[0]))DN_ArrayMakeArrayAssert(ptr, ptr_size, max, sizeof((ptr)[0]), 1, DN_ZMem_No, DN_CallSiteNow)

  #define DN_PArrayAddArray(ptr, ptr_size, max, items, count, add)             (DN_CppDeclType(&(ptr)[0]))DN_ArrayAddArray(ptr, ptr_size, max, sizeof((ptr)[0]), items, count, add)
  #define DN_PArrayAdd(ptr, ptr_size, max, item, add)                          (DN_CppDeclType(&(ptr)[0]))DN_ArrayAddArray(ptr, ptr_size, max, sizeof((ptr)[0]), &item, 1, add)
  #define DN_PArrayAppendArray(ptr, ptr_size, max, items, count)               (DN_CppDeclType(&(ptr)[0]))DN_ArrayAddArray(ptr, ptr_size, max, sizeof((ptr)[0]), items, count, DN_ArrayAdd_Append)
  #define DN_PArrayAppend(ptr, ptr_size, max, item)                            (DN_CppDeclType(&(ptr)[0]))DN_ArrayAddArray(ptr, ptr_size, max, sizeof((ptr)[0]), &item, 1, DN_ArrayAdd_Append)
  #define DN_PArrayAppendAssert(ptr, ptr_size, max, item)                      (DN_CppDeclType(&(ptr)[0]))DN_ArrayAddArrayAssert(ptr, ptr_size, max, sizeof((ptr)[0]), &item, 1, DN_ArrayAdd_Append, DN_CallSiteNow)
  #define DN_PArrayPrependArray(ptr, ptr_size, max, items, count)              (DN_CppDeclType(&(ptr)[0]))DN_ArrayAddArray(ptr, ptr_size, max, sizeof((ptr)[0]), items, count, DN_ArrayAdd_Prepend)
  #define DN_PArrayPrepend(ptr, ptr_size, max, item)                           (DN_CppDeclType(&(ptr)[0]))DN_ArrayAddArray(ptr, ptr_size, max, sizeof((ptr)[0]), &item, 1, DN_ArrayAdd_Prepend)

  #define DN_PArrayEraseRange(ptr, ptr_size, begin_index, count, erase)        DN_ArrayEraseRange(ptr, ptr_size, sizeof((ptr)[0]), begin_index, count, erase)
  #define DN_PArrayErase(ptr, ptr_size, index, erase)                          DN_ArrayEraseRange(ptr, ptr_size, sizeof((ptr)[0]), index, 1, erase)
  #define DN_PArrayInsertArray(ptr, ptr_size, max, index, items, count)        (DN_CppDeclType(&(ptr)[0]))DN_ArrayInsertArray(ptr, ptr_size, max, sizeof((ptr)[0]), index, items, count)
  #define DN_PArrayInsert(ptr, ptr_size, max, index, item)                     (DN_CppDeclType(&(ptr)[0]))DN_ArrayInsertArray(ptr, ptr_size, max, sizeof((ptr)[0]), index, &item, 1)
  #define DN_PArrayPopFront(ptr, ptr_size, max, count)                         (DN_CppDeclType(&(ptr)[0]))DN_ArrayPopFront(ptr, ptr_size, sizeof((ptr)[0]), count)
  #define DN_PArrayPopBack(ptr, ptr_size, max, count)                          (DN_CppDeclType(&(ptr)[0]))DN_ArrayPopBack(ptr, ptr_size, sizeof((ptr)[0]), count)
#endif

#define                                   DN_LArrayFind(c_array, size, ptr_find, eq_func)                      DN_PArrayFind(c_array, size, ptr_find, eq_func)
#define                                   DN_LArrayFindMemEq(c_array, size, ptr_find)                          DN_PArrayFindMemEq(c_array, size, ptr_find)
#define                                   DN_LArrayResizeFromPool(c_array, size, pool, new_max)                DN_PArrayResizeFromPool(c_array, size, DN_ArrayCountU(c_array), pool, new_max)
#define                                   DN_LArrayResizeFromArena(c_array, size, arena, new_max)              DN_PArrayResizeFromArena(c_array, size, DN_ArrayCountU(c_array), arena, new_max)
#define                                   DN_LArrayGrowFromPool(c_array, size, pool, new_max)                  DN_PArrayGrowFromPool(c_array, size, DN_ArrayCountU(c_array), pool, new_max)
#define                                   DN_LArrayGrowFromArena(c_array, size, arena, new_max)                DN_PArrayGrowFromArena(c_array, size, DN_ArrayCountU(c_array), arena, new_max)
#define                                   DN_LArrayGrowIfNeededFromPool(c_array, size, pool, add_count)        DN_PArrayGrowIfNeededFromPool(c_array, size, DN_ArrayCountU(c_array), pool, add_count)
#define                                   DN_LArrayGrowIfNeededFromArena(c_array, size, arena, add_count)      DN_PArrayGrowIfNeededFromArena(c_array, size, DN_ArrayCountU(c_array), arena, add_count)

#define                                   DN_LArrayMakeArray(c_array, ptr_size, count, z_mem)                  DN_PArrayMakeArray(c_array, ptr_size, DN_ArrayCountU(c_array), count, z_mem)
#define                                   DN_LArrayMakeArrayZ(c_array, ptr_size, count)                        DN_PArrayMakeArrayZ(c_array, ptr_size, DN_ArrayCountU(c_array), count)
#define                                   DN_LArrayMakeArrayNoZ(c_array, ptr_size, count)                      DN_PArrayMakeArrayNoZ(c_array, ptr_size, DN_ArrayCountU(c_array), count)
#define                                   DN_LArrayMakeArrayAssert(c_array, ptr_size, count, z_mem)            DN_PArrayMakeArrayAssert(c_array, ptr_size, DN_ArrayCountU(c_array), count, z_mem)
#define                                   DN_LArrayMakeArrayAssertZ(c_array, ptr_size, count)                  DN_PArrayMakeArrayAssertZ(c_array, ptr_size, DN_ArrayCountU(c_array), count)
#define                                   DN_LArrayMakeArrayAssertNoZ(c_array, ptr_size, count)                DN_PArrayMakeArrayAssertNoZ(c_array, ptr_size, DN_ArrayCountU(c_array), count)

#define                                   DN_LArrayMake(c_array, ptr_size, z_mem)                              DN_PArrayMake(c_array, ptr_size, DN_ArrayCountU(c_array), z_mem)
#define                                   DN_LArrayMakeZ(c_array, ptr_size)                                    DN_PArrayMakeZ(c_array, ptr_size, DN_ArrayCountU(c_array))
#define                                   DN_LArrayMakeNoZ(c_array, ptr_size)                                  DN_PArrayMakeNoZ(c_array, ptr_size, DN_ArrayCountU(c_array))
#define                                   DN_LArrayMakeAssert(c_array, ptr_size, z_mem)                        DN_PArrayMakeAssert(c_array, ptr_size, DN_ArrayCountU(c_array), z_mem)
#define                                   DN_LArrayMakeAssertZ(c_array, ptr_size)                              DN_PArrayMakeAssertZ(c_array, ptr_size, DN_ArrayCountU(c_array))
#define                                   DN_LArrayMakeAssertNoZ(c_array, ptr_size)                            DN_PArrayMakeAssertNoZ(c_array, ptr_size, DN_ArrayCountU(c_array))

#define                                   DN_LArrayAddArray(c_array, ptr_size, items, count, add)              DN_PArrayAddArray(c_array, ptr_size, DN_ArrayCountU(c_array), items, count, add)
#define                                   DN_LArrayAdd(c_array, ptr_size, item, add)                           DN_PArrayAdd(c_array, ptr_size, DN_ArrayCountU(c_array), item, add)
#define                                   DN_LArrayAppendArray(c_array, ptr_size, items, count)                DN_PArrayAppendArray(c_array, ptr_size, DN_ArrayCountU(c_array), items, count)
#define                                   DN_LArrayAppend(c_array, ptr_size, item)                             DN_PArrayAppend(c_array, ptr_size, DN_ArrayCountU(c_array), item)
#define                                   DN_LArrayAppendAssert(c_array, ptr_size, item)                       DN_PArrayAppendAssert(c_array, ptr_size, DN_ArrayCountU(c_array), item)
#define                                   DN_LArrayPrependArray(c_array, ptr_size, items, count)               DN_PArrayPrependArray(c_array, ptr_size, DN_ArrayCountU(c_array), items, count)
#define                                   DN_LArrayPrepend(c_array, ptr_size, item)                            DN_PArrayPrepend(c_array, ptr_size, DN_ArrayCountU(c_array), item)
#define                                   DN_LArrayEraseRange(c_array, ptr_size, begin_index, count, erase)    DN_PArrayEraseRange(c_array, ptr_size, begin_index, count, erase)
#define                                   DN_LArrayErase(c_array, ptr_size, index, erase)                      DN_PArrayErase(c_array, ptr_size, index, erase)
#define                                   DN_LArrayInsertArray(c_array, ptr_size, index, items, count)         DN_PArrayInsertArray(c_array, ptr_size, DN_ArrayCountU(c_array), index, items, count)
#define                                   DN_LArrayInsert(c_array, ptr_size, index, item)                      DN_PArrayInsert(c_array, ptr_size, DN_ArrayCountU(c_array), index, item)
#define                                   DN_LArrayPopFront(c_array, ptr_size, count)                          DN_PArrayPopFront(c_array, ptr_size, DN_ArrayCountU(c_array), count)
#define                                   DN_LArrayPopBack(c_array, ptr_size, count)                           DN_PArrayPopBack(c_array, ptr_size, DN_ArrayCountU(c_array), count)

#define                                   DN_IArrayFind(ptr_array, ptr_find, eq_func)                          DN_PArrayFind((ptr_array)->data, (ptr_array)->count, ptr_find, eq_func)
#define                                   DN_IArrayFindMemEq(ptr_array, ptr_find)                              DN_PArrayFindMemEq((ptr_array)->data, (ptr_array)->count, ptr_find)
#define                                   DN_IArrayResizeFromPool(ptr_array, pool, new_max)                    DN_PArrayResizeFromPool((ptr_array)->data, &(ptr_array)->count, &(ptr_array)->max, pool, new_max)
#define                                   DN_IArrayResizeFromArena(ptr_array, arena, new_max)                  DN_PArrayResizeFromArena((ptr_array)->data, &(ptr_array)->count, &(ptr_array)->max, arena, new_max)
#define                                   DN_IArrayGrowFromPool(ptr_array, pool, new_max)                      DN_PArrayGrowFromPool((ptr_array)->data, (ptr_array)->count, &(ptr_array)->max, pool, new_max)
#define                                   DN_IArrayGrowFromArena(ptr_array, arena, new_max)                    DN_PArrayGrowFromArena((ptr_array)->data, (ptr_array)->count, &(ptr_array)->max, arena, new_max)
#define                                   DN_IArrayGrowIfNeededFromPool(ptr_array, pool, add_count)            DN_PArrayGrowIfNeededFromPool(&(ptr_array)->data, (ptr_array)->count, &(ptr_array)->max, pool, add_count)
#define                                   DN_IArrayGrowIfNeededFromArena(ptr_array, arena, add_count)          DN_PArrayGrowIfNeededFromArena(&(ptr_array)->data, (ptr_array)->count, &(ptr_array)->max, arena, add_count)

#define                                   DN_IArrayMakeArray(ptr_array, count, z_mem)                          DN_PArrayMakeArray((ptr_array)->data, &(ptr_array)->count, (ptr_array)->max, count, z_mem)
#define                                   DN_IArrayMakeArrayZ(ptr_array, count)                                DN_PArrayMakeArrayZ((ptr_array)->data, &(ptr_array)->count, (ptr_array)->max, count)
#define                                   DN_IArrayMakeArrayNoZ(ptr_array, count)                              DN_PArrayMakeArrayNoZ((ptr_array)->data, &(ptr_array)->count, (ptr_array)->max, count)
#define                                   DN_IArrayMakeArrayAssert(ptr_array, count, z_mem)                    DN_PArrayMakeArrayAssert((ptr_array)->data, &(ptr_array)->count, (ptr_array)->max, count, z_mem)
#define                                   DN_IArrayMakeArrayAssertZ(ptr_array, count)                          DN_PArrayMakeArrayAssertZ((ptr_array)->data, &(ptr_array)->count, (ptr_array)->max, count)
#define                                   DN_IArrayMakeArrayAssertNoZ(ptr_array, count)                        DN_PArrayMakeArrayAssertNoZ((ptr_array)->data, &(ptr_array)->count, (ptr_array)->max, count)

#define                                   DN_IArrayMake(ptr_array, z_mem)                                      DN_PArrayMake((ptr_array)->data, &(ptr_array)->count, (ptr_array)->max, z_mem)
#define                                   DN_IArrayMakeZ(ptr_array)                                            DN_PArrayMakeZ((ptr_array)->data, &(ptr_array)->count, (ptr_array)->max)
#define                                   DN_IArrayMakeNoZ(ptr_array)                                          DN_PArrayMakeNoZ((ptr_array)->data, &(ptr_array)->count, (ptr_array)->max)
#define                                   DN_IArrayMakeAssert(ptr_array, z_mem)                                DN_PArrayMakeAssert((ptr_array)->data, &(ptr_array)->count, (ptr_array)->max, z_mem)
#define                                   DN_IArrayMakeAssertZ(ptr_array)                                      DN_PArrayMakeAssertZ((ptr_array)->data, &(ptr_array)->count, (ptr_array)->max)
#define                                   DN_IArrayMakeAssertNoZ(ptr_array)                                    DN_PArrayMakeAssertNoZ((ptr_array)->data, &(ptr_array)->count, (ptr_array)->max)

#define                                   DN_IArrayAddArray(ptr_array, items, count, add)                      DN_PArrayAddArray((ptr_array)->data, &(ptr_array)->count, (ptr_array)->max, items, count, add)
#define                                   DN_IArrayAdd(ptr_array, item, add)                                   DN_PArrayAdd((ptr_array)->data, &(ptr_array)->count, (ptr_array)->max, item, add)
#define                                   DN_IArrayAppendArray(ptr_array, items, count)                        DN_PArrayAppendArray((ptr_array)->data, &(ptr_array)->count, (ptr_array)->max, items, count)
#define                                   DN_IArrayAppend(ptr_array, item)                                     DN_PArrayAppend((ptr_array)->data, &(ptr_array)->count, (ptr_array)->max, item)
#define                                   DN_IArrayAppendAssert(ptr_array, item)                               DN_PArrayAppendAssert((ptr_array)->data, &(ptr_array)->count, (ptr_array)->max, item)
#define                                   DN_IArrayPrependArray(ptr_array, items, count)                       DN_PArrayPrependArray((ptr_array)->data, &(ptr_array)->count, (ptr_array)->max, items, count)
#define                                   DN_IArrayPrepend(ptr_array, item)                                    DN_PArrayPrepend((ptr_array)->data, &(ptr_array)->count, (ptr_array)->max, item)
#define                                   DN_IArrayEraseRange(ptr_array, begin_index, count, erase)            DN_PArrayEraseRange((ptr_array)->data, &(ptr_array)->count, begin_index, count, erase)
#define                                   DN_IArrayErase(ptr_array, index, erase)                              DN_PArrayErase((ptr_array)->data, &(ptr_array)->count, index, erase)
#define                                   DN_IArrayInsertArray(ptr_array, index, items, count)                 DN_PArrayInsertArray((ptr_array)->data, &(ptr_array)->count, (ptr_array)->max, index, items, count)
#define                                   DN_IArrayInsert(ptr_array, index, item)                              DN_PArrayInsert((ptr_array)->data, &(ptr_array)->count, (ptr_array)->max, index, item)
#define                                   DN_IArrayPopFront(ptr_array, count)                                  DN_PArrayPopFront((ptr_array)->data, &(ptr_array)->count, (ptr_array)->max, count)
#define                                   DN_IArrayPopBack(ptr_array, count)                                   DN_PArrayPopBack((ptr_array)->data, &(ptr_array)->count, (ptr_array)->max, count)

// NOTE: Slices
//
//   Fixed size container allocated up front that have a `T *data` and `DN_USize count` elements.
//
//   API
//     AllocArena: Allocates the container with the requested `count` elements
//
#define                                   DN_ISliceAllocArena(slice_ptr, count_, zmem, arena)                  (DN_CppDeclType(&((slice_ptr)->data[0])))DN_SliceAllocArena((void **)&((slice_ptr)->data), &((slice_ptr)->count), count_, sizeof((slice_ptr)->data[0]), alignof(DN_CppDeclType((slice_ptr)->data[0])), zmem, arena)


DN_API void*                              DN_SliceAllocArena             (void **data, DN_USize *slice_size_field, DN_USize size, DN_USize elem_size, DN_U8 align, DN_ZMem zmem, DN_Arena *arena);

DN_API DN_ArrayFindResult                 DN_ArrayFind                   (void *data, DN_USize size, DN_USize elem_size, void const *find, DN_ArrayFindEqFunc *eq_func);
DN_API DN_ArrayFindResult                 DN_ArrayFindMemEq              (void *data, DN_USize size, DN_USize elem_size, void const *find);
DN_API void*                              DN_ArrayInsertArray            (void *data, DN_USize *size, DN_USize max, DN_USize elem_size, DN_USize index, void const *items, DN_USize count);
DN_API void*                              DN_ArrayPopFront               (void *data, DN_USize *size, DN_USize elem_size, DN_USize count);
DN_API void*                              DN_ArrayPopBack                (void *data, DN_USize *size, DN_USize elem_size, DN_USize count);
DN_API DN_ArrayEraseResult                DN_ArrayEraseRange             (void *data, DN_USize *size, DN_USize elem_size, DN_USize begin_index, DN_ISize count, DN_ArrayErase erase);
DN_API void*                              DN_ArrayMakeArray              (void *data, DN_USize *size, DN_USize max, DN_USize elem_size, DN_USize make_count, DN_ZMem z_mem);
DN_API void*                              DN_ArrayMakeArrayAssert        (void *data, DN_USize *size, DN_USize max, DN_USize elem_size, DN_USize make_count, DN_ZMem z_mem, DN_CallSite call_site);
DN_API void*                              DN_ArrayAddArray               (void *data, DN_USize *size, DN_USize max, DN_USize elem_size, void const *elems, DN_USize elems_count, DN_ArrayAdd add);
DN_API void*                              DN_ArrayAddArrayAssert         (void *data, DN_USize *size, DN_USize max, DN_USize elem_size, void const *elems, DN_USize elems_count, DN_ArrayAdd add, DN_CallSite call_site);
DN_API bool                               DN_ArrayResizeFromPool         (void **data, DN_USize *size, DN_USize *max, DN_USize elem_size, DN_Pool *pool, DN_USize new_max);
DN_API bool                               DN_ArrayResizeFromArena        (void **data, DN_USize *size, DN_USize *max, DN_USize elem_size, DN_Arena *arena, DN_USize new_max);
DN_API bool                               DN_ArrayGrowFromPool           (void **data, DN_USize size, DN_USize *max, DN_USize elem_size, DN_Pool *pool, DN_USize new_max);
DN_API bool                               DN_ArrayGrowFromArena          (void **data, DN_USize size, DN_USize *max, DN_USize elem_size, DN_Arena *arena, DN_USize new_max);
DN_API bool                               DN_ArrayGrowIfNeededFromPool   (void **data, DN_USize size, DN_USize *max, DN_USize elem_size, DN_Pool *pool, DN_USize add_count);
DN_API bool                               DN_ArrayGrowIfNeededFromArena  (void **data, DN_USize size, DN_USize *max, DN_USize elem_size, DN_Arena *arena, DN_USize add_count);

#if defined (__cplusplus)
template <typename T> DN_ArrayFindResult  DN_TArrayFind                  (T *data, DN_USize size, void const *find, DN_ArrayFindEqFunc *eq_func);
template <typename T> DN_ArrayFindResult  DN_TArrayFindMemEq             (T *data, DN_USize size, void const *find, DN_ArrayFindEqFunc *eq_func);
template <typename T> T*                  DN_TArrayInsertArray           (T *data, DN_USize *size, DN_USize max, DN_USize index, void const *items, DN_USize count);
template <typename T> T*                  DN_TArrayPopFront              (T *data, DN_USize *size, DN_USize count);
template <typename T> T*                  DN_TArrayPopBack               (T *data, DN_USize *size, DN_USize count);
template <typename T> DN_ArrayEraseResult DN_TArrayEraseRange            (T *data, DN_USize *size, DN_USize begin_index, DN_ISize count, DN_ArrayErase erase);
template <typename T> T*                  DN_TArrayMakeArray             (T *data, DN_USize *size, DN_USize max, DN_USize make_count, DN_ZMem z_mem);
template <typename T> T*                  DN_TArrayMakeArrayAssert       (T *data, DN_USize *size, DN_USize max, DN_USize make_count, DN_ZMem z_mem, DN_CallSite call_site);
template <typename T> T*                  DN_TArrayMakeArrayAssertZ      (T *data, DN_USize *size, DN_USize max, DN_USize make_count, DN_CallSite call_site);
template <typename T> T*                  DN_TArrayMakeArrayAssertNoZ    (T *data, DN_USize *size, DN_USize max, DN_USize make_count, DN_CallSite call_site);
template <typename T> T*                  DN_TArrayAddArray              (T *data, DN_USize *size, DN_USize max, T const *elems, DN_USize elems_count, DN_ArrayAdd add);
template <typename T> T*                  DN_TArrayAddArrayAssert        (T *data, DN_USize *size, DN_USize max, T const *elems, DN_USize elems_count, DN_ArrayAdd add, DN_CallSite call_site);
template <typename T> bool                DN_TArrayResizeFromPool        (T **data, DN_USize *size, DN_USize *max, DN_Pool *pool, DN_USize new_max);
template <typename T> bool                DN_TArrayResizeFromArena       (T **data, DN_USize *size, DN_USize *max, DN_Arena *arena, DN_USize new_max);
template <typename T> bool                DN_TArrayGrowFromPool          (T **data, DN_USize size, DN_USize *max, DN_Pool *pool, DN_USize new_max);
template <typename T> bool                DN_TArrayGrowFromArena         (T **data, DN_USize size, DN_USize *max, DN_Pool *pool, DN_USize new_max);
template <typename T> bool                DN_TArrayGrowIfNeededFromPool  (T **data, DN_USize size, DN_USize *max, DN_Pool *pool, DN_USize add_count);
template <typename T> bool                DN_TArrayGrowIfNeededFromArena (T **data, DN_USize size, DN_USize *max, DN_Arena *pool, DN_USize add_count);
#endif

DN_API void*                              DN_SinglyLLDetach              (void **link, void **next);
DN_API bool                               DN_RingHasSpace                (DN_Ring const *ring, DN_U64 size);
DN_API bool                               DN_RingHasData                 (DN_Ring const *ring, DN_U64 size);
DN_API void                               DN_RingWrite                   (DN_Ring *ring, void const *src, DN_U64 src_size);
#define                                   DN_RingWriteStruct(ring, item) DN_RingWrite((ring), (item), sizeof(*(item)))
DN_API void                               DN_RingRead                    (DN_Ring *ring, void *dest, DN_U64 dest_size);
#define                                   DN_RingReadStruct(ring, dest)  DN_RingRead((ring), (dest), sizeof(*(dest)))

// TODO: Replace with a C-style hash table
#if defined(__cplusplus)
DN_U32 const DN_DS_MAP_DEFAULT_HASH_SEED = 0x8a1ced49;
DN_U32 const DN_DS_MAP_SENTINEL_SLOT     = 0;
template <typename T> DN_DSMap<T>        DN_DSMapInit                  (DN_Arena *arena, DN_U32 size, DN_DSMapFlags flags);
template <typename T> void               DN_DSMapDeinit                (DN_DSMap<T> *map, DN_ZMem z_mem);
template <typename T> bool               DN_DSMapIsValid               (DN_DSMap<T> const *map);
template <typename T> DN_U32             DN_DSMapHash                  (DN_DSMap<T> const *map, DN_DSMapKey key);
template <typename T> DN_U32             DN_DSMapHashToSlotIndex       (DN_DSMap<T> const *map, DN_DSMapKey key);
template <typename T> DN_DSMapResult<T>  DN_DSMapFind                  (DN_DSMap<T> const *map, DN_DSMapKey key);
template <typename T> DN_DSMapResult<T>  DN_DSMapMake                  (DN_DSMap<T> *map, DN_DSMapKey key);
template <typename T> DN_DSMapResult<T>  DN_DSMapSet                   (DN_DSMap<T> *map, DN_DSMapKey key, T const &value);
template <typename T> DN_DSMapResult<T>  DN_DSMapFindKeyU64            (DN_DSMap<T> const *map, DN_U64 key);
template <typename T> DN_DSMapResult<T>  DN_DSMapMakeKeyU64            (DN_DSMap<T> *map,       DN_U64 key);
template <typename T> DN_DSMapResult<T>  DN_DSMapSetKeyU64             (DN_DSMap<T> *map,       DN_U64 key, T const &value);
template <typename T> DN_DSMapResult<T>  DN_DSMapFindKeyStr8           (DN_DSMap<T> const *map, DN_Str8 key);
template <typename T> DN_DSMapResult<T>  DN_DSMapMakeKeyStr8           (DN_DSMap<T> *map,       DN_Str8 key);
template <typename T> DN_DSMapResult<T>  DN_DSMapSetKeyStr8            (DN_DSMap<T> *map,       DN_Str8 key, T const &value);
template <typename T> bool               DN_DSMapResize                (DN_DSMap<T> *map, DN_U32 size);
template <typename T> bool               DN_DSMapErase                 (DN_DSMap<T> *map, DN_DSMapKey key);
template <typename T> bool               DN_DSMapEraseKeyU64           (DN_DSMap<T> *map, DN_U64 key);
template <typename T> bool               DN_DSMapEraseKeyStr8          (DN_DSMap<T> *map, DN_Str8 key);
template <typename T> DN_DSMapKey        DN_DSMapKeyBuffer             (DN_DSMap<T> const *map, void const *data, DN_USize size);
template <typename T> DN_DSMapKey        DN_DSMapKeyBufferAsU64NoHash  (DN_DSMap<T> const *map, void const *data, DN_USize size);
template <typename T> DN_DSMapKey        DN_DSMapKeyU64                (DN_DSMap<T> const *map, DN_U64 u64);
template <typename T> DN_DSMapKey        DN_DSMapKeyStr8               (DN_DSMap<T> const *map, DN_Str8 string);
#define                                  DN_DSMapKeyCStr8(map, string) DN_DSMapKeyBuffer(map, string, sizeof((string))/sizeof((string)[0]) - 1)
DN_API                DN_DSMapKey        DN_DSMapKeyU64NoHash          (DN_U64 u64);
DN_API                bool               DN_DSMapKeyEquals             (DN_DSMapKey lhs, DN_DSMapKey rhs);
DN_API                bool               operator==                    (DN_DSMapKey lhs, DN_DSMapKey rhs);
#endif

enum DN_BinPackMode
{
  DN_BinPackMode_Serialise,
  DN_BinPackMode_Deserialise,
};

struct DN_BinPack
{
  DN_Str8Builder writer;
  DN_Str8        read;
  DN_USize       read_index;
};

DN_API bool    DN_BinPackIsEndOfReadStream(DN_BinPack const *pack);
DN_API void    DN_BinPackUSize            (DN_BinPack *pack, DN_BinPackMode mode, DN_USize *item);
DN_API void    DN_BinPackU64              (DN_BinPack *pack, DN_BinPackMode mode, DN_U64 *item);
DN_API void    DN_BinPackU32              (DN_BinPack *pack, DN_BinPackMode mode, DN_U32 *item);
DN_API void    DN_BinPackU16              (DN_BinPack *pack, DN_BinPackMode mode, DN_U16 *item);
DN_API void    DN_BinPackU8               (DN_BinPack *pack, DN_BinPackMode mode, DN_U8 *item);
DN_API void    DN_BinPackI64              (DN_BinPack *pack, DN_BinPackMode mode, DN_I64 *item);
DN_API void    DN_BinPackI32              (DN_BinPack *pack, DN_BinPackMode mode, DN_I32 *item);
DN_API void    DN_BinPackI16              (DN_BinPack *pack, DN_BinPackMode mode, DN_I16 *item);
DN_API void    DN_BinPackI8               (DN_BinPack *pack, DN_BinPackMode mode, DN_I8 *item);
DN_API void    DN_BinPackF64              (DN_BinPack *pack, DN_BinPackMode mode, DN_F64 *item);
DN_API void    DN_BinPackF32              (DN_BinPack *pack, DN_BinPackMode mode, DN_F32 *item);
DN_API void    DN_BinPackV2               (DN_BinPack *pack, DN_BinPackMode mode, DN_V2F32 *item);
DN_API void    DN_BinPackV4               (DN_BinPack *pack, DN_BinPackMode mode, DN_V4F32 *item);
DN_API void    DN_BinPackBool             (DN_BinPack *pack, DN_BinPackMode mode, bool *item);
DN_API void    DN_BinPackStr8FromArena    (DN_BinPack *pack, DN_Arena *arena, DN_BinPackMode mode, DN_Str8 *string);
DN_API void    DN_BinPackStr8FromPool     (DN_BinPack *pack, DN_Pool *pool, DN_BinPackMode mode, DN_Str8 *string);
DN_API DN_Str8 DN_BinPackStr8FromBuffer   (DN_BinPack *pack, DN_BinPackMode mode, char *ptr, DN_USize *size, DN_USize max);
DN_API void    DN_BinPackBytesFromArena   (DN_BinPack *pack, DN_Arena *arena, DN_BinPackMode mode, void **ptr, DN_USize *size);
DN_API void    DN_BinPackBytesFromPool    (DN_BinPack *pack, DN_Pool *pool, DN_BinPackMode mode, void **ptr, DN_USize *size);
DN_API void    DN_BinPackCArray           (DN_BinPack *pack, DN_BinPackMode mode, void *ptr, DN_USize size);
DN_API void    DN_BinPackCBuffer          (DN_BinPack *pack, DN_BinPackMode mode, char *ptr, DN_USize *size, DN_USize max);
DN_API DN_Str8 DN_BinPackBuild            (DN_BinPack const *pack, DN_Arena *arena);

enum DN_CSVSerialise
{
  DN_CSVSerialise_Read,
  DN_CSVSerialise_Write,
};

struct DN_CSVTokeniser
{
  bool        bad;
  DN_Str8     string;
  char        delimiter;
  char const *it;
  bool        end_of_line;
};

struct DN_CSVPack
{
  DN_Str8Builder  write_builder;
  DN_USize        write_column;
  DN_CSVTokeniser read_tokeniser;
};

// NOTE: Data structures to create and parse CSV files, supports Python style escaped quotes (e.g.
// Using "" to escape quotes inside a quoted string).
//
// API
//  DN_CSVTokeniserNextN: Reads the next N consecutive fields from the parser. If `column_iterator`
//  is `false` then the read of the N consecutive fields does not proceed past the end of the
//  current CSV row. If `true` then it reads the next N fields even if reading would progress onto
//  the next row.
DN_API DN_CSVTokeniser DN_CSVTokeniserInit       (DN_Str8 string, char delimiter);
DN_API bool            DN_CSVTokeniserValid      (DN_CSVTokeniser *tokeniser);
DN_API bool            DN_CSVTokeniserNextRow    (DN_CSVTokeniser *tokeniser);
DN_API DN_Str8         DN_CSVTokeniserNextField  (DN_CSVTokeniser *tokeniser);
DN_API DN_Str8         DN_CSVTokeniserNextColumn (DN_CSVTokeniser *tokeniser);
DN_API void            DN_CSVTokeniserSkipLine   (DN_CSVTokeniser *tokeniser);
DN_API int             DN_CSVTokeniserNextN      (DN_CSVTokeniser *tokeniser, DN_Str8 *fields, int fields_size, bool column_iterator);
DN_API int             DN_CSVTokeniserNextColumnN(DN_CSVTokeniser *tokeniser, DN_Str8 *fields, int fields_size);
DN_API int             DN_CSVTokeniserNextFieldN (DN_CSVTokeniser *tokeniser, DN_Str8 *fields, int fields_size);
DN_API void            DN_CSVTokeniserSkipLineN  (DN_CSVTokeniser *tokeniser, int count);
DN_API void            DN_CSVPackU64             (DN_CSVPack *pack, DN_CSVSerialise serialise, DN_U64 *value);
DN_API void            DN_CSVPackI64             (DN_CSVPack *pack, DN_CSVSerialise serialise, DN_I64 *value);
DN_API void            DN_CSVPackI32             (DN_CSVPack *pack, DN_CSVSerialise serialise, DN_I32 *value);
DN_API void            DN_CSVPackI16             (DN_CSVPack *pack, DN_CSVSerialise serialise, DN_I16 *value);
DN_API void            DN_CSVPackI8              (DN_CSVPack *pack, DN_CSVSerialise serialise, DN_I8 *value);
DN_API void            DN_CSVPackU32             (DN_CSVPack *pack, DN_CSVSerialise serialise, DN_U32 *value);
DN_API void            DN_CSVPackU16             (DN_CSVPack *pack, DN_CSVSerialise serialise, DN_U16 *value);
DN_API void            DN_CSVPackBoolAsU64       (DN_CSVPack *pack, DN_CSVSerialise serialise, bool *value);
DN_API void            DN_CSVPackStr8            (DN_CSVPack *pack, DN_CSVSerialise serialise, DN_Str8 *str8, DN_Arena *arena);
DN_API void            DN_CSVPackBuffer          (DN_CSVPack *pack, DN_CSVSerialise serialise, void *dest, size_t *size);
DN_API void            DN_CSVPackBufferWithMax   (DN_CSVPack *pack, DN_CSVSerialise serialise, void *dest, size_t *size, size_t max);
DN_API bool            DN_CSVPackNewLine         (DN_CSVPack *pack, DN_CSVSerialise serialise);

// TODO: Replace with a C implementation
template <typename T>
using DN_BinarySearchLessThanProc = bool(T const &lhs, T const &rhs);

template <typename T>
bool DN_BinarySearch_DefaultLessThan(T const &lhs, T const &rhs);

enum DN_BinarySearchType
{
    // Index of the match. If no match is found, found is set to false and the
    // index is set to the index where the match should be inserted/exist, if
    // it were in the array
    DN_BinarySearchType_Match,

    // Index of the first element in the array that is `element >= find`. If no such
    // item is found or the array is empty, then, the index is set to the array
    // size and found is set to `false`.
    //
    // For example:
    //   int array[] = {0, 1, 2, 3, 4, 5};
    //   DN_BinarySearchResult result = DN_BinarySearch(array, DN_ArrayCountU(array), 4, DN_BinarySearchType_LowerBound);
    //   printf("%zu\n", result.index); // Prints index '4'

    DN_BinarySearchType_LowerBound,

    // Index of the first element in the array that is `element > find`. If no such
    // item is found or the array is empty, then, the index is set to the array
    // size and found is set to `false`.
    //
    // For example:
    //   int array[] = {0, 1, 2, 3, 4, 5};
    //   DN_BinarySearchResult result = DN_BinarySearch(array, DN_ArrayCountU(array), 4, DN_BinarySearchType_UpperBound);
    //   printf("%zu\n", result.index); // Prints index '5'

    DN_BinarySearchType_UpperBound,
};

struct DN_BinarySearchResult
{
  bool     found;
  DN_USize index;
};

template <typename T> bool                  DN_BinarySearch_DefaultLessThan(T const &lhs, T const &rhs);
template <typename T> DN_BinarySearchResult DN_BinarySearch                (T const *array, DN_USize array_size, T const &find, DN_BinarySearchType type = DN_BinarySearchType_Match, DN_BinarySearchLessThanProc<T> less_than = DN_BinarySearch_DefaultLessThan);

template <typename T>
bool DN_BinarySearch_DefaultLessThan(T const &lhs, T const &rhs)
{
    bool result = lhs < rhs;
    return result;
}

template <typename T>
DN_BinarySearchResult DN_BinarySearch(T const                         *array,
                                        DN_USize                        array_size,
                                        T const                         &find,
                                        DN_BinarySearchType             type,
                                        DN_BinarySearchLessThanProc<T>  less_than)
{
    DN_BinarySearchResult result = {};
    if (!array || array_size <= 0 || !less_than)
        return result;

    T const *end   = array + array_size;
    T const *first = array;
    T const *last  = end;
    while (first != last) {
        DN_USize count = last - first;
        T const *it     = first + (count / 2);

        bool advance_first = false;
        if (type == DN_BinarySearchType_UpperBound)
            advance_first = !less_than(find, it[0]);
        else
            advance_first = less_than(it[0], find);

        if (advance_first)
            first = it + 1;
        else
            last  = it;
    }

    switch (type) {
        case DN_BinarySearchType_Match: {
            result.found = first != end && !less_than(find, *first);
        } break;

        case DN_BinarySearchType_LowerBound: /*FALLTHRU*/
        case DN_BinarySearchType_UpperBound: {
            result.found = first != end;
        } break;
    }

    result.index = first - array;
    return result;
}

DN_API void DN_LeakTrackAlloc_  (DN_LeakTracker *leak, void *ptr, DN_USize size, bool alloc_can_leak);
DN_API void DN_LeakTrackDealloc_(DN_LeakTracker *leak, void *ptr);
DN_API void DN_LeakDump_        (DN_LeakTracker *leak);

#if defined(DN_LEAK_TRACKING)
#define     DN_LeakTrackAlloc(leak, ptr, size, alloc_can_leak) DN_LeakTrackAlloc_(leak, ptr, size, alloc_can_leak)
#define     DN_LeakTrackDealloc(leak, ptr)                     DN_LeakTrackDealloc_(leak, ptr)
#define     DN_LeakDump(leak)                                  DN_LeakDump_(leak)
#else
#define     DN_LeakTrackAlloc(leak, ptr, size, alloc_can_leak) do { (void)ptr; (void)size; (void)alloc_can_leak; } while (0)
#define     DN_LeakTrackDealloc(leak, ptr)                     do { (void)ptr;                                   } while (0)
#define     DN_LeakDump(leak)                                  do {                                              } while (0)
#endif

#if DN_WITH_OS
DN_API DN_MemFuncs               DN_MemFuncsFromType                          (DN_MemFuncsType type);
DN_API DN_MemFuncs               DN_MemFuncsDefault                           ();
DN_API DN_MemList                DN_MemListFromHeap                           (DN_U64 size, DN_MemFlags flags);
DN_API DN_MemList                DN_MemListFromVMem                           (DN_U64 reserve, DN_U64 commit, DN_MemFlags flags);
DN_API DN_Arena                  DN_ArenaFromHeap                             (DN_U64 wize, DN_MemFlags flags);
DN_API DN_Arena                  DN_ArenaFromVMem                             (DN_U64 reserve, DN_U64 commit, DN_MemFlags flags);

DN_API DN_Str8                   DN_Str8FromHeapF                             (DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_Str8                   DN_Str8FromHeap                              (DN_USize size, DN_ZMem z_mem);
DN_API DN_Str8                   DN_Str8BuilderBuildFromHeap                  (DN_Str8Builder const *builder);

DN_API void                      DN_OS_LogPrint                               (DN_LogTypeParam type, void *user_data, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API void                      DN_OS_SetLogPrintFuncToOS                    ();

DN_API void *                    DN_OS_MemReserve                             (DN_USize size, DN_MemCommit commit, DN_MemPage page_flags);
DN_API bool                      DN_OS_MemCommit                              (void *ptr, DN_USize size, DN_U32 page_flags);
DN_API void                      DN_OS_MemDecommit                            (void *ptr, DN_USize size);
DN_API void                      DN_OS_MemRelease                             (void *ptr, DN_USize size);
DN_API int                       DN_OS_MemProtect                             (void *ptr, DN_USize size, DN_U32 page_flags);

DN_API void *                    DN_OS_MemAlloc                               (DN_USize size, DN_ZMem z_mem);
DN_API void                      DN_OS_MemDealloc                             (void *ptr);

DN_API DN_Date                   DN_OS_DateLocalTimeNow                       ();
DN_API DN_Str8x32                DN_OS_DateLocalTimeStr8Now                   (char date_separator = '-', char hms_separator = ':');
DN_API DN_Str8x32                DN_OS_DateLocalTimeStr8                      (DN_Date time, char date_separator = '-', char hms_separator = ':');
DN_API DN_U64                    DN_OS_DateUnixTimeNs                         ();
#define                          DN_OS_DateUnixTimeUs()                       (DN_OS_DateUnixTimeNs() / 1000)
#define                          DN_OS_DateUnixTimeMs()                       (DN_OS_DateUnixTimeNs() / (1000 * 1000))
#define                          DN_OS_DateUnixTimeS()                        (DN_OS_DateUnixTimeNs() / (1000 * 1000 * 1000))
DN_API DN_U64                    DN_OS_DateUnixTimeSFromLocalDate             (DN_Date date);
DN_API DN_U64                    DN_OS_DateLocalUnixTimeSFromUnixTimeS        (DN_U64 unix_ts_s);

DN_API void                      DN_OS_GenBytesSecure                         (void *buffer, DN_U32 size);
DN_API bool                      DN_OS_SetEnvVar                              (DN_Str8 name, DN_Str8 value);
DN_API DN_OSDiskSpace            DN_OS_DiskSpace                              (DN_Str8 path);
DN_API DN_Str8                   DN_OS_EXEPath                                (DN_Arena *arena);
DN_API DN_Str8                   DN_OS_EXEDir                                 (DN_Arena *arena);
DN_API void                      DN_OS_SleepMs                                (DN_UInt milliseconds);

DN_API DN_U64                    DN_OS_PerfCounterNow                         ();
DN_API DN_U64                    DN_OS_PerfCounterFrequency                   ();
DN_API DN_F64                    DN_OS_PerfCounterS                           (DN_U64 begin, uint64_t end);
DN_API DN_F64                    DN_OS_PerfCounterMs                          (DN_U64 begin, uint64_t end);
DN_API DN_F64                    DN_OS_PerfCounterUs                          (DN_U64 begin, uint64_t end);
DN_API DN_F64                    DN_OS_PerfCounterNs                          (DN_U64 begin, uint64_t end);
DN_API DN_OSTimer                DN_OS_TimerBegin                             ();
DN_API void                      DN_OS_TimerEnd                               (DN_OSTimer *timer);
DN_API DN_F64                    DN_OS_TimerS                                 (DN_OSTimer timer);
DN_API DN_F64                    DN_OS_TimerMs                                (DN_OSTimer timer);
DN_API DN_F64                    DN_OS_TimerUs                                (DN_OSTimer timer);
DN_API DN_F64                    DN_OS_TimerNs                                (DN_OSTimer timer);
DN_API DN_U64                    DN_OS_EstimateTSCPerSecond                   (uint64_t duration_ms_to_gauge_tsc_frequency);

DN_API bool                      DN_OS_FileCopy                               (DN_Str8 src, DN_Str8 dest, bool overwrite, DN_ErrSink *err);
DN_API bool                      DN_OS_FileMove                               (DN_Str8 src, DN_Str8 dest, bool overwrite, DN_ErrSink *err);

DN_API DN_OSFile                 DN_OS_FileOpen                               (DN_Str8 path, DN_OSFileOpen open_mode, DN_OSFileAccess access, DN_ErrSink *err);
DN_API DN_OSFileRead             DN_OS_FileRead                               (DN_OSFile *file, void *buffer, DN_USize size, DN_ErrSink *err);
DN_API bool                      DN_OS_FileWritePtr                           (DN_OSFile *file, void const *data, DN_USize size, DN_ErrSink *err);
DN_API bool                      DN_OS_FileWrite                              (DN_OSFile *file, DN_Str8 buffer, DN_ErrSink *err);
DN_API bool                      DN_OS_FileWriteFV                            (DN_OSFile *file, DN_ErrSink *err, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API bool                      DN_OS_FileWriteF                             (DN_OSFile *file, DN_ErrSink *err, DN_FMT_ATTRIB char const *fmt, ...);
DN_API bool                      DN_OS_FileFlush                              (DN_OSFile *file, DN_ErrSink *err);
DN_API void                      DN_OS_FileClose                              (DN_OSFile *file);

DN_API DN_Str8                   DN_OS_FileReadAll                            (DN_Allocator allocator, DN_Str8 path, DN_ErrSink *err);
DN_API DN_Str8                   DN_OS_FileReadAllArena                       (DN_Arena *arena, DN_Str8 path, DN_ErrSink *err);
DN_API DN_Str8                   DN_OS_FileReadAllPool                        (DN_Pool *pool, DN_Str8 path, DN_ErrSink *err);

DN_API bool                      DN_OS_FileWriteAll                           (DN_Str8 path, DN_Str8 buffer, DN_ErrSink *err);
DN_API bool                      DN_OS_FileWriteAllFV                         (DN_Str8 path, DN_ErrSink *err, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API bool                      DN_OS_FileWriteAllF                          (DN_Str8 path, DN_ErrSink *err, DN_FMT_ATTRIB char const *fmt, ...);
DN_API bool                      DN_OS_FileWriteAllSafe                       (DN_Str8 path, DN_Str8 buffer, DN_ErrSink *err);
DN_API bool                      DN_OS_FileWriteAllSafeFV                     (DN_Str8 path, DN_ErrSink *err, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API bool                      DN_OS_FileWriteAllSafeF                      (DN_Str8 path, DN_ErrSink *err, DN_FMT_ATTRIB char const *fmt, ...);

DN_API DN_Str8                   DN_OS_Str8FromPathInfoType                   (DN_OSPathInfoType type);
DN_API DN_OSPathInfo             DN_OS_PathInfo                               (DN_Str8 path);
DN_API bool                      DN_OS_PathIsOlderThan                        (DN_Str8 file, DN_Str8 check_against);
DN_API bool                      DN_OS_PathDelete                             (DN_Str8 path);
DN_API bool                      DN_OS_PathIsFile                             (DN_Str8 path);
DN_API bool                      DN_OS_PathIsDir                              (DN_Str8 path);
DN_API bool                      DN_OS_PathMakeDir                            (DN_Str8 path);
DN_API bool                      DN_OS_PathIterateDir                         (DN_Str8 path, DN_OSDirIterator *it);

DN_API bool                      DN_OS_PathAddRef                             (DN_Arena *arena, DN_OSPath *fs_path, DN_Str8 path);
DN_API bool                      DN_OS_PathAdd                                (DN_Arena *arena, DN_OSPath *fs_path, DN_Str8 path);
DN_API bool                      DN_OS_PathAddF                               (DN_Arena *arena, DN_OSPath *fs_path, DN_FMT_ATTRIB char const *fmt, ...);
DN_API bool                      DN_OS_PathPop                                (DN_OSPath *fs_path);
DN_API DN_Str8                   DN_OS_PathBuildWithSeparator                 (DN_Arena *arena, DN_OSPath const *fs_path, DN_Str8 path_separator);
DN_API DN_Str8                   DN_OS_PathTo                                 (DN_Arena *arena, DN_Str8 path, DN_Str8 path_separtor);
DN_API DN_Str8                   DN_OS_PathToF                                (DN_Arena *arena, DN_Str8 path_separator, DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_Str8                   DN_OS_Path                                   (DN_Arena *arena, DN_Str8 path);
DN_API DN_Str8                   DN_OS_PathF                                  (DN_Arena *arena, DN_FMT_ATTRIB char const *fmt, ...);

#define                          DN_OS_PathBuildFwdSlash(allocator, fs_path)  DN_OS_PathBuildWithSeparator(allocator, fs_path, DN_Str8Lit("/"))
#define                          DN_OS_PathBuildBackSlash(allocator, fs_path) DN_OS_PathBuildWithSeparator(allocator, fs_path, DN_Str8Lit("\\"))
#define                          DN_OS_PathBuild(allocator, fs_path)          DN_OS_PathBuildWithSeparator(allocator, fs_path, DN_OSPathSeparatorString)

DN_API void                      DN_OS_Exit                                   (int32_t exit_code);
DN_API DN_OSExecResult           DN_OS_ExecPump                               (DN_OSExecAsyncHandle handle, char *stdout_buffer, size_t *stdout_size, char *stderr_buffer, size_t *stderr_size, DN_U32 timeout_ms, DN_ErrSink  *err);
DN_API DN_OSExecResult           DN_OS_ExecWait                               (DN_OSExecAsyncHandle handle, DN_Arena *arena, DN_ErrSink *err);
DN_API DN_OSExecAsyncHandle      DN_OS_ExecAsync                              (DN_Str8Slice cmd_line, DN_OSExecArgs *args, DN_ErrSink *err);
DN_API DN_OSExecResult           DN_OS_Exec                                   (DN_Str8Slice cmd_line, DN_OSExecArgs *args, DN_Arena *arena, DN_ErrSink *err);
DN_API DN_OSExecResult           DN_OS_ExecOrAbort                            (DN_Str8Slice cmd_line, DN_OSExecArgs *args, DN_Arena *arena);

DN_API DN_OSSemaphore            DN_OS_SemaphoreInit                          (DN_U32 initial_count);
DN_API void                      DN_OS_SemaphoreDeinit                        (DN_OSSemaphore *semaphore);
DN_API void                      DN_OS_SemaphoreIncrement                     (DN_OSSemaphore *semaphore, DN_U32 amount);
DN_API DN_OSSemaphoreWaitResult  DN_OS_SemaphoreWait                          (DN_OSSemaphore *semaphore, DN_U32 timeout_ms);

DN_API DN_OSBarrier              DN_OS_BarrierInit                            (DN_U32 thread_count);
DN_API void                      DN_OS_BarrierDeinit                          (DN_OSBarrier *barrier);
DN_API void                      DN_OS_BarrierWait                            (DN_OSBarrier *barrier);

DN_API DN_OSMutex                DN_OS_MutexInit                              ();
DN_API void                      DN_OS_MutexDeinit                            (DN_OSMutex *mutex);
DN_API void                      DN_OS_MutexLock                              (DN_OSMutex *mutex);
DN_API void                      DN_OS_MutexUnlock                            (DN_OSMutex *mutex);
#define                          DN_OS_MutexScope(mutex)                      DN_DeferLoop(DN_OS_MutexLock(mutex), DN_OS_MutexUnlock(mutex))

DN_API DN_OSConditionVariable    DN_OS_ConditionVariableInit                  ();
DN_API void                      DN_OS_ConditionVariableDeinit                (DN_OSConditionVariable *cv);
DN_API bool                      DN_OS_ConditionVariableWait                  (DN_OSConditionVariable *cv, DN_OSMutex *mutex, DN_U64 sleep_ms);
DN_API bool                      DN_OS_ConditionVariableWaitUntil             (DN_OSConditionVariable *cv, DN_OSMutex *mutex, DN_U64 end_ts_ms);
DN_API void                      DN_OS_ConditionVariableSignal                (DN_OSConditionVariable *cv);
DN_API void                      DN_OS_ConditionVariableBroadcast             (DN_OSConditionVariable *cv);

DN_API bool                      DN_OS_ThreadInit                             (DN_OSThread *thread, DN_OSThreadFunc *func, DN_OSThreadLane *lane, DN_TCInitArgs tc_init_args, void *user_context);
DN_API bool                      DN_OS_ThreadJoin                             (DN_OSThread *thread, DN_TCDeinitArenas deinit_arenas);
DN_API DN_U32                    DN_OS_ThreadID                               ();
DN_API void                      DN_OS_ThreadSetNameFmt                       (char const *fmt, ...);

// NOTE: Thread lanes provide an abstraction to represent the concept of programming a CPU like a
// GPU, e.g. SIMT (Single Instruction Multiple Threads). The lane terminology is popularised by Ryan
// Fleury. SIMT is formally defined as
//
//  Threads are grouped into warps/wavefronts (typically 32 or 64 threads) that execute the same
//  instruction in lockstep, but each thread operates on different data and maintains its own state
//
// The individual threads in a wavefront on the CPU side are colloquially dubbed "lanes" and a
// thread lane here contains the necessary state to facilitate this such as the current index in the
// wavefront and synchronisation primitives to coordinate the different lanes together.
//
// The idea is to write code in a single-threaded manner (linear execution) but across multiple
// threads so that the default is all execution paths are inherently multi-threaded by default. Opt
// out of parallelism instead of opt in. This optimises for the trend of core counts increasing
// whilst clock counts remain static.
//
// A laneway is a helper function to initialise the number of requested OS threads/lanes upfront and
// setup the required synchronisation primitives. It can then be dispatched all the threads which
// start executing the `entry_point` in parallel.
//
// API
//   DN_OS_ThreadLaneSync
//     A blocking call to synchronise the program-counter of all other lanes in the laneway to this
//     function call invocation (using an OS barrier). Optionally pass in the pointer to a pointer
//     `ptr_to_share` to broadcast the pointer from one lanes to the others. The lane that wishes
//     to broadcast the pointer must have a non-null pointer, all other lanes must pass in a
//     non-null pointer. A typical use case might look like:
/*
         DN_OSThreadLane *lane = DN_OS_TCThreadLane(); // Get lane from current (t)hread (c)context

         // NOTE: Allocate buffer in lane 0
         DN_U8 *buffer         = nullptr;
         if (lane->index == 0)
           buffer = DN_ArenaNewArray(DN_TCMainArena(), DN_U8, DN_Gigabytes(1), DN_ZMem_No);

         // NOTE: Lane 0 broadcasts the `buffer` pointer to lane 1..N
         DN_OS_ThreadLaneSync(lane, &buffer);

         // NOTE: We use LaneRange to divide the buffer into equal sized chunks that each lane can
         // write into without clobbering over each other.
         DN_V2USize range = DN_OS_ThreadLaneRange(lane, DN_Gigabytes(1));
         for (DN_USize index = range.begin; index < range.end; index++) { buffer[index] = index; }
*/
//     In this example, lane 0 will allocate a 1GiB buffer pass in a `buffer` to
//     DN_OS_ThreadLaneSync` that is non-null. Lanes 1->N will skip the branch (because their lanes
//     indexes are 1..N) and invoke `DN_OS_ThreadLaneSync` with a nullptr `buffer`. After the
//     blocking call is complete, lanes 0->N will now have synchronised the `buffer` pointer and all
//     lanes point to the 1GiB range allocated in lane 0's allocator.
//
//     Additionally we demonstrate `DN_OS_ThreadLaneRange` which does math behind the scenes to
//     divide the buffer up and assign each lane their own indices in the buffer that they can work
//     on in parallel without clobbering each others work.
//
//   DN_OS_ThreadLaneRange
//     Calculates the range of values the current lane in the laneway should execute. For example if
//     you have 128 items and 16 threads each lane will receive the following `DN_V2USize` range:
//       Lane 0  => [0,   8)
//       Lane 1  => [8,   16)
//       ...
//       Lane 16 => [120, 128)
DN_API DN_OSThreadLane           DN_OS_ThreadLaneInit                         (DN_USize index, DN_USize thread_count, DN_OSBarrier barrier, DN_UPtr *share_mem);
DN_API void                      DN_OS_ThreadLaneSync                         (DN_OSThreadLane *lane, void **ptr_to_share);
DN_API DN_V2USize                DN_OS_ThreadLaneRange                        (DN_OSThreadLane const *lane, DN_USize values_count);

DN_API DN_OSThreadLaneway        DN_OS_ThreadLanewayFromArgs                  (DN_OSThread* threads, DN_USize threads_count, DN_UPtr* shared_mem);
DN_API DN_OSThreadLaneway        DN_OS_ThreadLanewayFromArena                 (DN_USize threads_count, DN_Arena* arena);
DN_API void                      DN_OS_ThreadLanewayDispatch                  (DN_OSThreadLaneway *laneway, DN_OSThreadFunc *entry_point, DN_TCInitArgs tc_init_args, void *user_context);
DN_API void                      DN_OS_ThreadLanewayJoin                      (DN_OSThreadLaneway *laneway, DN_TCDeinitArenas deinit_arenas);

DN_API DN_OSThreadLane*          DN_OS_TCThreadLane                           ();
DN_API void                      DN_OS_TCThreadLaneSync                       (void **ptr_to_share);
DN_API DN_OSThreadLane           DN_OS_TCThreadLaneEquip                      (DN_OSThreadLane lane);


DN_API void                      DN_OS_AsyncInit     (DN_OSAsyncCore *async, char *base, DN_USize base_size, DN_OSThread *threads, DN_U32 threads_size);
DN_API void                      DN_OS_AsyncDeinit   (DN_OSAsyncCore *async);
DN_API bool                      DN_OS_AsyncQueueWork(DN_OSAsyncCore *async, DN_OSAsyncWorkFunc *func, void *input, DN_U64 wait_time_ms);
DN_API DN_OSAsyncTask            DN_OS_AsyncQueueTask(DN_OSAsyncCore *async, DN_OSAsyncWorkFunc *func, void *input, DN_U64 wait_time_ms);
DN_API bool                      DN_OS_AsyncWaitTask (DN_OSAsyncTask *task, DN_U32 timeout_ms);

// NOTE: DN_OSPrint
enum DN_OSPrintDest
{
  DN_OSPrintDest_Out,
  DN_OSPrintDest_Err,
};

// NOTE: Print Macros
#define DN_OS_PrintOut(string)                       DN_OS_Print(DN_OSPrintDest_Out, string)
#define DN_OS_PrintOutF(fmt, ...)                    DN_OS_PrintF(DN_OSPrintDest_Out, fmt, ##__VA_ARGS__)
#define DN_OS_PrintOutFV(fmt, args)                  DN_OS_PrintFV(DN_OSPrintDest_Out, fmt, args)

#define DN_OS_PrintOutStyle(style, string)           DN_OS_PrintStyle(DN_OSPrintDest_Out, style, string)
#define DN_OS_PrintOutFStyle(style, fmt, ...)        DN_OS_PrintFStyle(DN_OSPrintDest_Out, style, fmt, ##__VA_ARGS__)
#define DN_OS_PrintOutFVStyle(style, fmt, args, ...) DN_OS_PrintFVStyle(DN_OSPrintDest_Out, style, fmt, args)

#define DN_OS_PrintOutLn(string)                     DN_OS_PrintLn(DN_OSPrintDest_Out, string)
#define DN_OS_PrintOutLnF(fmt, ...)                  DN_OS_PrintLnF(DN_OSPrintDest_Out, fmt, ##__VA_ARGS__)
#define DN_OS_PrintOutLnFV(fmt, args)                DN_OS_PrintLnFV(DN_OSPrintDest_Out, fmt, args)

#define DN_OS_PrintOutLnStyle(style, string)         DN_OS_PrintLnStyle(DN_OSPrintDest_Out, style, string);
#define DN_OS_PrintOutLnFStyle(style, fmt, ...)      DN_OS_PrintLnFStyle(DN_OSPrintDest_Out, style, fmt, ##__VA_ARGS__)
#define DN_OS_PrintOutLnFVStyle(style, fmt, args)    DN_OS_PrintLnFVStyle(DN_OSPrintDest_Out, style, fmt, args);

#define DN_OS_PrintErr(string)                       DN_OS_Print(DN_OSPrintDest_Err, string)
#define DN_OS_PrintErrF(fmt, ...)                    DN_OS_PrintF(DN_OSPrintDest_Err, fmt, ##__VA_ARGS__)
#define DN_OS_PrintErrFV(fmt, args)                  DN_OS_PrintFV(DN_OSPrintDest_Err, fmt, args)

#define DN_OS_PrintErrStyle(style, string)           DN_OS_PrintStyle(DN_OSPrintDest_Err, style, string)
#define DN_OS_PrintErrFStyle(style, fmt, ...)        DN_OS_PrintFStyle(DN_OSPrintDest_Err, style, fmt, ##__VA_ARGS__)
#define DN_OS_PrintErrFVStyle(style, fmt, args, ...) DN_OS_PrintFVStyle(DN_OSPrintDest_Err, style, fmt, args)

#define DN_OS_PrintErrLn(string)                     DN_OS_PrintLn(DN_OSPrintDest_Err, string)
#define DN_OS_PrintErrLnF(fmt, ...)                  DN_OS_PrintLnF(DN_OSPrintDest_Err, fmt, ##__VA_ARGS__)
#define DN_OS_PrintErrLnFV(fmt, args)                DN_OS_PrintLnFV(DN_OSPrintDest_Err, fmt, args)

#define DN_OS_PrintErrLnStyle(style, string)         DN_OS_PrintLnStyle(DN_OSPrintDest_Err, style, string);
#define DN_OS_PrintErrLnFStyle(style, fmt, ...)      DN_OS_PrintLnFStyle(DN_OSPrintDest_Err, style, fmt, ##__VA_ARGS__)
#define DN_OS_PrintErrLnFVStyle(style, fmt, args)    DN_OS_PrintLnFVStyle(DN_OSPrintDest_Err, style, fmt, args);

// NOTE: Print
DN_API void DN_OS_Print                (DN_OSPrintDest dest, DN_Str8 string);
DN_API void DN_OS_PrintF               (DN_OSPrintDest dest, DN_FMT_ATTRIB char const *fmt, ...);
DN_API void DN_OS_PrintFV              (DN_OSPrintDest dest, DN_FMT_ATTRIB char const *fmt, va_list args);

DN_API void DN_OS_PrintStyle           (DN_OSPrintDest dest, DN_LogStyle style, DN_Str8 string);
DN_API void DN_OS_PrintFStyle          (DN_OSPrintDest dest, DN_LogStyle style, DN_FMT_ATTRIB char const *fmt, ...);
DN_API void DN_OS_PrintFVStyle         (DN_OSPrintDest dest, DN_LogStyle style, DN_FMT_ATTRIB char const *fmt, va_list args);

DN_API void DN_OS_PrintLn              (DN_OSPrintDest dest, DN_Str8 string);
DN_API void DN_OS_PrintLnF             (DN_OSPrintDest dest, DN_FMT_ATTRIB char const *fmt, ...);
DN_API void DN_OS_PrintLnFV            (DN_OSPrintDest dest, DN_FMT_ATTRIB char const *fmt, va_list args);

DN_API void DN_OS_PrintLnStyle         (DN_OSPrintDest dest, DN_LogStyle style, DN_Str8 string);
DN_API void DN_OS_PrintLnFStyle        (DN_OSPrintDest dest, DN_LogStyle style, DN_FMT_ATTRIB char const *fmt, ...);
DN_API void DN_OS_PrintLnFVStyle       (DN_OSPrintDest dest, DN_LogStyle style, DN_FMT_ATTRIB char const *fmt, va_list args);
#endif // DN_WITH_OS

// NOTE: Template implementations
#if defined(__cplusplus)
template <typename T> T *DN_MemCopyObjT(T *dest, T const *src, DN_USize count)
{
  T* result = dest;
  DN_Memcpy(dest, src, sizeof(T) * count);
  return result;
}

template <typename T>
DN_ArrayFindResult DN_TArrayFind(T *data, DN_USize size, void const *find, DN_ArrayFindEqFunc *eq_func)
{
  DN_ArrayFindResult result = DN_ArrayFind(data, size, sizeof(*data), find, eq_func);
  return result;
}

template <typename T>
DN_ArrayFindResult DN_TArrayFindMemEq(T *data, DN_USize size, void const *find)
{
  DN_ArrayFindResult result = DN_ArrayFindMemEq(data, size, sizeof(*data), find);
  return result;
}

template <typename T>
T *DN_TArrayInsertArray(T *data, DN_USize *size, DN_USize max, DN_USize index, T const *items, DN_USize count)
{
  T *result = DN_Cast(T *)DN_ArrayInsertArray(data, size, max, sizeof(*data), index, items, count);
  return result;
}

template <typename T>
T *DN_TArrayPopFront(T *data, DN_USize *size, DN_USize count)
{
  T *result = DN_Cast(T *)DN_ArrayPopFront(data, size, sizeof(*data), count);
  return result;
}

template <typename T>
T *DN_TArrayPopBack(T *data, DN_USize *size, DN_USize count)
{
  T *result = DN_Cast(T *)DN_ArrayPopBack(data, size, sizeof(*data), count);
  return result;
}

template <typename T>
DN_ArrayEraseResult DN_TArrayEraseRange(T *data, DN_USize *size, DN_USize begin_index, DN_ISize count, DN_ArrayErase erase)
{
  DN_ArrayEraseResult result = DN_ArrayEraseRange(data, size, sizeof(*data), begin_index, count, erase);
  return result;
}

template <typename T>
T *DN_TArrayMakeArray(T *data, DN_USize *size, DN_USize max, DN_USize make_count, DN_ZMem z_mem)
{
  T *result = DN_Cast(T *)DN_ArrayMakeArray(data, size, max, sizeof(*data), make_count, z_mem);
  return result;
}

template <typename T>
T *DN_TArrayMakeArrayAssert(T *data, DN_USize *size, DN_USize max, DN_USize make_count, DN_ZMem z_mem, DN_CallSite call_site)
{
  T *result = DN_Cast(T *)DN_ArrayMakeArrayAssert(data, size, max, sizeof(*data), make_count, z_mem, call_site);
  return result;
}

template <typename T>
T *DN_TArrayMakeArrayAssertZ(T *data, DN_USize *size, DN_USize max, DN_USize make_count, DN_CallSite call_site)
{
  T* result = DN_TArrayMakeArrayAssert(data, size, max, make_count, DN_ZMem_Yes, call_site);
  return result;
}

template <typename T>
T *DN_TArrayMakeArrayAssertNoZ(T *data, DN_USize *size, DN_USize max, DN_USize make_count, DN_CallSite call_site)
{
  T* result = DN_TArrayMakeArrayAssert(data, size, max, make_count, DN_ZMem_No, call_site);
  return result;
}

template <typename T>
T *DN_TArrayAddArray(T *data, DN_USize *size, DN_USize max, T const *elems, DN_USize elems_count, DN_ArrayAdd add)
{
  T* result = DN_Cast(T *)DN_ArrayAddArray(data, size, max, sizeof(*elems), elems, elems_count, add);
  return result;
}

template <typename T>
T *DN_TArrayAddArrayAssert(T *data, DN_USize *size, DN_USize max, T const *elems, DN_USize elems_count, DN_ArrayAdd add, DN_CallSite call_site)
{
  T* result = DN_Cast(T *)DN_ArrayAddArrayAssert(data, size, max, sizeof(*elems), elems, elems_count, add, call_site);
  return result;
}

template <typename T>
bool DN_TArrayResizeFromPool(T **data, DN_USize *size, DN_USize *max, DN_Pool *pool, DN_USize new_max)
{
  bool result = DN_ArrayResizeFromPool(DN_Cast(void **)data, size, max, sizeof(**data), pool, new_max);
  return result;
}

template <typename T>
bool DN_TArrayResizeFromArena(T **data, DN_USize *size, DN_USize *max, DN_Arena *arena, DN_USize new_max)
{
  bool result = DN_ArrayResizeFromArena(DN_Cast(void **)data, size, max, sizeof(**data), arena, new_max);
  return result;
}

template <typename T>
bool DN_TArrayGrowFromPool(T **data, DN_USize size, DN_USize *max, DN_Pool *pool, DN_USize new_max)
{
  bool result = DN_ArrayGrowFromPool(DN_Cast(void **)data, size, max, sizeof(**data), pool, new_max);
  return result;
}

template <typename T>
bool DN_TArrayGrowFromArena(T **data, DN_USize size, DN_USize *max, DN_Arena *arena, DN_USize new_max)
{
  bool result = DN_ArrayGrowFromArena(DN_Cast(void **)data, size, max, sizeof(**data), arena, new_max);
  return result;
}

template <typename T>
bool DN_TArrayGrowIfNeededFromPool(T **data, DN_USize size, DN_USize *max, DN_Pool *pool, DN_USize add_count)
{
  bool result = DN_ArrayGrowIfNeededFromPool(DN_Cast(void **)data, size, max, sizeof(**data), pool, add_count);
  return result;
}

template <typename T>
bool DN_TArrayGrowIfNeededFromArena(T **data, DN_USize size, DN_USize *max, DN_Arena *arena, DN_USize add_count)
{
  bool result = DN_ArrayGrowIfNeededFromArena(DN_Cast(void **)data, size, max, sizeof(**data), arena, add_count);
  return result;
}
#endif // defined(__cplusplus)

#if DN_WITH_NET
enum DN_NETRequestType
{
  DN_NETRequestType_Nil,
  DN_NETRequestType_HTTP,
  DN_NETRequestType_WS,
};

enum DN_NETResponseState
{
  DN_NETResponseState_Nil,
  DN_NETResponseState_Error,
  DN_NETResponseState_HTTP,
  DN_NETResponseState_WSOpen,
  DN_NETResponseState_WSText,
  DN_NETResponseState_WSBinary,
  DN_NETResponseState_WSClose,
  DN_NETResponseState_WSPing,
  DN_NETResponseState_WSPong,
};

enum DN_NETWSSend
{
  DN_NETWSSend_Text,
  DN_NETWSSend_Binary,
  DN_NETWSSend_Close,
  DN_NETWSSend_Ping,
  DN_NETWSSend_Pong,
};

enum DN_NETDoHTTPFlags
{
  DN_NETDoHTTPFlags_Nil       = 0,
  DN_NETDoHTTPFlags_BasicAuth = 1 << 0,
};

struct DN_NETDoHTTPArgs
{
  // NOTE: WS and HTTP args
  DN_NETDoHTTPFlags  flags;
  DN_Str8            username;
  DN_Str8            password;
  DN_Str8           *headers;
  DN_U16             headers_size;

  // NOTE: HTTP args only
  DN_Str8            payload;
};

struct DN_NETRequestHandle
{
  DN_UPtr handle;
  DN_U64  gen;
};

struct DN_NETResponse
{
  // NOTE: Common to WS and HTTP responses
  DN_NETRequestType   type;
  DN_NETResponseState state;
  DN_NETRequestHandle request;
  DN_Str8             error_str8;
  DN_Str8             body;

  // NOTE: HTTP responses only
  DN_U32              http_status;
};

struct DN_NETRequest
{
  DN_MemList        mem;
  DN_Arena          arena;
  DN_Arena          start_response_arena;
  DN_NETRequestType type;
  DN_U64            gen;
  DN_Str8           url;
  DN_Str8           method;
  DN_OSSemaphore    completion_sem;
  DN_NETDoHTTPArgs  args;
  DN_NETResponse    response;
  DN_NETRequest    *next;
  DN_NETRequest    *prev;
  DN_U64            context[2];
};

typedef void               (DN_NETInitFunc)              (struct DN_NETCore *net, char *base, DN_U64 base_size);
typedef void               (DN_NETDeinitFunc)            (struct DN_NETCore *net);
typedef DN_NETRequestHandle(DN_NETDoHTTPFunc)            (struct DN_NETCore *net, DN_Str8 url, DN_Str8 method, DN_NETDoHTTPArgs const *args);
typedef DN_NETRequestHandle(DN_NETDoWSFunc)              (struct DN_NETCore *net, DN_Str8 url);
typedef void               (DN_NETDoWSSendFunc)          (DN_NETRequestHandle handle, DN_Str8 data, DN_NETWSSend send);
typedef DN_NETResponse     (DN_NETWaitForResponseFunc)   (DN_NETRequestHandle handle, DN_Arena *arena, DN_U32 timeout_ms);
typedef DN_NETResponse     (DN_NETWaitForAnyResponseFunc)(struct DN_NETCore *net, DN_Arena *arena, DN_U32 timeout_ms);

struct DN_NETInterface
{
  DN_NETInitFunc*               init;
  DN_NETDeinitFunc*             deinit;
  DN_NETDoHTTPFunc*             do_http;
  DN_NETDoWSFunc*               do_ws;
  DN_NETDoWSSendFunc*           do_ws_send;
  DN_NETWaitForResponseFunc*    wait_for_response;
  DN_NETWaitForAnyResponseFunc* wait_for_any_response;
};

struct DN_NETCore
{
  char           *base;
  DN_U64          base_size;
  DN_MemList      mem;
  DN_Arena        arena;
  DN_OSSemaphore  completion_sem;
  void           *context;
  DN_NETInterface api;
};

DN_Str8             DN_NET_Str8FromResponseState     (DN_NETResponseState state);
DN_NETRequest *     DN_NET_RequestFromHandle         (DN_NETRequestHandle handle);
DN_NETRequestHandle DN_NET_HandleFromRequest         (DN_NETRequest *request);
bool                DN_NET_ResponseHasFailed         (DN_NETResponse const* resp);
DN_Str8             DN_NET_Str8DiagnosticFromResponse(DN_NETResponse const* resp, DN_Arena *arena);

// NOTE: Internal functions for different networking implementations to use
void                DN_NET_BaseInit             (DN_NETCore *net, char *base, DN_U64 base_size);
DN_NETRequestHandle DN_NET_SetupRequest         (DN_NETRequest *request, DN_Str8 url, DN_Str8 method, DN_NETDoHTTPArgs const *args, DN_NETRequestType type);
void                DN_NET_EndFinishedRequest   (DN_NETRequest *request);
#endif

#if DN_WITH_NET_CURL
#if !DN_WITH_OS || !DN_WITH_NET
  #error "NET API with CURL requires #define DN_WITH_NET 1 and #define DN_WITH_OS 1"
#endif
struct DN_NETCurlCore
{
  // NOTE: Shared w/ user and networking thread
  DN_Ring        ring;
  DN_OSMutex     ring_mutex;
  bool           kill_thread;

  DN_OSMutex     list_mutex;          // Lock for request, response, deinit, free list
  DN_NETRequest *request_list;        // Current requests submitted by the user thread awaiting to move into the thread request list
  DN_NETRequest *response_list;       // Finished requests that are to be deqeued by the user via wait for response
  DN_NETRequest *deinit_list;         // Requests that are finished and are awaiting to be de-initialised by the CURL thread
  DN_NETRequest *free_list;           // Request pool that new requests will use before allocating

  // NOTE: Networking thread only
  DN_NETRequest *thread_request_list; // Current requests being executed by the CURL thread.
                                      // This list is exclusively owned by the CURL thread so no locking is needed
  DN_OSThread    thread;
  void          *thread_curlm;
};

#define             DN_NET_CurlCoreFromNet(net)  ((net) ? (DN_Cast(DN_NETCurlCore *)(net)->context) : nullptr);
DN_NETInterface     DN_NET_CurlInterface         ();
void                DN_NET_CurlInit              (DN_NETCore *net, char *base, DN_U64 base_size);
void                DN_NET_CurlDeinit            (DN_NETCore *net);
DN_NETRequestHandle DN_NET_CurlDoHTTP            (DN_NETCore *net, DN_Str8 url, DN_Str8 method, DN_NETDoHTTPArgs const *args);
DN_NETRequestHandle DN_NET_CurlDoWSArgs          (DN_NETCore *net, DN_Str8 url, DN_NETDoHTTPArgs const *args);
DN_NETRequestHandle DN_NET_CurlDoWS              (DN_NETCore *net, DN_Str8 url);
void                DN_NET_CurlDoWSSend          (DN_NETRequestHandle handle, DN_Str8 payload, DN_NETWSSend send);
DN_NETResponse      DN_NET_CurlWaitForResponse   (DN_NETRequestHandle handle, DN_Arena *arena, DN_U32 timeout_ms);
DN_NETResponse      DN_NET_CurlWaitForAnyResponse(DN_NETCore *net, DN_Arena *arena, DN_U32 timeout_ms);
#endif // #if DN_WITH_NET_CURL

#if DN_WITH_NET_EMSCRIPTEN
#if !DN_WITH_OS || !DN_WITH_NET
  #error "NET API with Emscripten requires #define DN_WITH_NET 1 and #define DN_WITH_OS 1"
#endif
#if !defined(__EMSCRIPTEN__)
  #error "NET API with Esmcripten can only be compiled with emcc which defines __EMSCRIPTEN__"
#endif

DN_NETInterface     DN_NET_EmcInterface();
void                DN_NET_EmcInit              (DN_NETCore *net, char *base, DN_U64 base_size);
void                DN_NET_EmcDeinit            (DN_NETCore *net);
DN_NETRequestHandle DN_NET_EmcDoHTTP            (DN_NETCore *net, DN_Str8 url, DN_Str8 method, DN_NETDoHTTPArgs const *args);
DN_NETRequestHandle DN_NET_EmcDoWS              (DN_NETCore *net, DN_Str8 url);
void                DN_NET_EmcDoWSSend          (DN_NETRequestHandle handle, DN_Str8 data, DN_NETWSSend send);
DN_NETResponse      DN_NET_EmcWaitForResponse   (DN_NETRequestHandle handle, DN_Arena *arena, DN_U32 timeout_ms);
DN_NETResponse      DN_NET_EmcWaitForAnyResponse(DN_NETCore *net, DN_Arena *arena, DN_U32 timeout_ms);
#endif // #if DN_WITH_NET_EMSCRIPTEN

#if DN_WITH_OS
#if defined(DN_PLATFORM_WIN32)
  #include "OS/dn_os_windows.h"
  #include "OS/dn_os_w32.h"
#elif defined(DN_PLATFORM_POSIX) || defined(DN_PLATFORM_EMSCRIPTEN)
  #include "OS/dn_os_posix.h"
#else
  #error Please define a platform e.g. 'DN_PLATFORM_WIN32' to enable the correct implementation for platform APIs
#endif
#endif // #if DN_WITH_OS

#endif // #if !defined(DN_H)
