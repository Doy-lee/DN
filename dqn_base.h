/*
///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  $$$$$$$\
//  $$  __$$\
//  $$ |  $$ | $$$$$$\   $$$$$$$\  $$$$$$\
//  $$$$$$$\ | \____$$\ $$  _____|$$  __$$\
//  $$  __$$\  $$$$$$$ |\$$$$$$\  $$$$$$$$ |
//  $$ |  $$ |$$  __$$ | \____$$\ $$   ____|
//  $$$$$$$  |\$$$$$$$ |$$$$$$$  |\$$$$$$$\
//  \_______/  \_______|\_______/  \_______|
//
//  dqn_base.h -- Base primitives for the library
//
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// [$MACR] Macros          -- General macros
// [$TYPE] Types           -- Basic types and typedefs
// [$SDLL] DN_SentinelDLL -- Doubly linked list w/ sentinel macros
// [$INTR] Intrinsics      -- Platform agnostic functions for CPU instructions (e.g. atomics, cpuid, ...)
// [$CALL] DN_CallSite    -- Source code location/tracing
// [$TMUT] DN_TicketMutex -- Userland mutex via spinlocking atomics
// [$PRIN] DN_Print       -- Console printing
// [$LLOG] DN_Log         -- Console logging macros
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

// NOTE: [$MACR] Macros ////////////////////////////////////////////////////////////////////////////
#define DN_STRINGIFY(x) #x
#define DN_TOKEN_COMBINE2(x, y) x ## y
#define DN_TOKEN_COMBINE(x, y) DN_TOKEN_COMBINE2(x, y)

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

// NOTE: Declare struct literals that work in both C and C++ because the syntax 
// is different between languages.
#if 0
    struct Foo { int a; }
    struct Foo foo = DN_LITERAL(Foo){32}; // Works on both C and C++
#endif

#if defined(__cplusplus)
    #define DN_LITERAL(T) T
#else
    #define DN_LITERAL(T) (T)
#endif

#if defined(__cplusplus)
    #define DN_THREAD_LOCAL thread_local
#else
    #define DN_THREAD_LOCAL _Thread_local
#endif

#if defined(_WIN32)
    #define DN_OS_WIN32
#elif defined(__gnu_linux__) || defined(__linux__)
    #define DN_OS_UNIX
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <inttypes.h> // PRIu64...

#if !defined(DN_OS_WIN32)
#include <stdlib.h> // exit()
#endif

#if !defined(DN_PLATFORM_EMSCRIPTEN) && \
    !defined(DN_PLATFORM_POSIX)      && \
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

#if defined(DN_COMPILER_MSVC) || defined(DN_COMPILER_CLANG_CL)
    #if defined(_CRT_SECURE_NO_WARNINGS)
        #define DN_CRT_SECURE_NO_WARNINGS_PREVIOUSLY_DEFINED
    #else
        #define _CRT_SECURE_NO_WARNINGS
    #endif
#endif

#if defined(DN_COMPILER_MSVC)
    #define DN_FMT_ATTRIB _Printf_format_string_
    #define DN_MSVC_WARNING_PUSH __pragma(warning(push))
    #define DN_MSVC_WARNING_DISABLE(...) __pragma(warning(disable: ##__VA_ARGS__))
    #define DN_MSVC_WARNING_POP __pragma(warning(pop))
#else
    #define DN_FMT_ATTRIB
    #define DN_MSVC_WARNING_PUSH
    #define DN_MSVC_WARNING_DISABLE(...)
    #define DN_MSVC_WARNING_POP
#endif

#if defined(DN_COMPILER_CLANG) || defined(DN_COMPILER_GCC) || defined(DN_COMPILER_CLANG_CL)
    #define DN_GCC_WARNING_PUSH _Pragma("GCC diagnostic push")
    #define DN_GCC_WARNING_DISABLE_HELPER_0(x) #x
    #define DN_GCC_WARNING_DISABLE_HELPER_1(y) DN_GCC_WARNING_DISABLE_HELPER_0(GCC diagnostic ignored #y)
    #define DN_GCC_WARNING_DISABLE(warning) _Pragma(DN_GCC_WARNING_DISABLE_HELPER_1(warning))
    #define DN_GCC_WARNING_POP _Pragma("GCC diagnostic pop")
#else
    #define DN_GCC_WARNING_PUSH
    #define DN_GCC_WARNING_DISABLE(...)
    #define DN_GCC_WARNING_POP
#endif

// NOTE: MSVC does not support the feature detection macro for instance so we
// compile it out
#if defined(__has_feature)
    #define DN_HAS_FEATURE(expr) __has_feature(expr)
#else
    #define DN_HAS_FEATURE(expr) 0
#endif

#define DN_FOR_UINDEX(index, size) for (DN_USize index = 0; index < size; index++)
#define DN_FOR_IINDEX(index, size) for (DN_isize index = 0; index < size; index++)

#define DN_ForItSize(it, T, array, size) for (struct { USize index; T *data; } it = {0, &(array)[0]};       it.index < (size);                it.index++, it.data++)
#define DN_ForIt(it, T, array)           for (struct { USize index; T *data; } it = {0, &(array)->data[0]}; it.index < (array)->size;         it.index++, it.data++)
#define DN_ForItCArray(it, T, array)     for (struct { USize index; T *data; } it = {0, &(array)[0]};       it.index < DN_ARRAY_UCOUNT(array); it.index++, it.data++)

#define DN_AlignUpPowerOfTwo(value, pot) (((uintptr_t)(value) + ((uintptr_t)(pot) - 1)) & ~((uintptr_t)(pot) - 1))
#define DN_AlignDownPowerOfTwo(value, pot) ((uintptr_t)(value) & ~((uintptr_t)(pot) - 1))
#define DN_IsPowerOfTwo(value) ((((uintptr_t)(value)) & (((uintptr_t)(value)) - 1)) == 0)
#define DN_IsPowerOfTwoAligned(value, pot) ((((uintptr_t)value) & (((uintptr_t)pot) - 1)) == 0)

// NOTE: String.h Dependencies /////////////////////////////////////////////////////////////////////
#if !defined(DN_MEMCPY) || !defined(DN_MEMSET) || !defined(DN_MEMCMP) || !defined(DN_MEMMOVE)
    #include <string.h>
    #if !defined(DN_MEMCPY)
        #define DN_MEMCPY(dest, src, count) memcpy((dest), (src), (count))
    #endif
    #if !defined(DN_MEMSET)
        #define DN_MEMSET(dest, value, count) memset((dest), (value), (count))
    #endif
    #if !defined(DN_MEMCMP)
        #define DN_MEMCMP(lhs, rhs, count) memcmp((lhs), (rhs), (count))
    #endif
    #if !defined(DN_MEMMOVE)
        #define DN_MEMMOVE(dest, src, count) memmove((dest), (src), (count))
    #endif
#endif

// NOTE: Math.h Dependencies ///////////////////////////////////////////////////////////////////////
#if !defined(DN_SQRTF) || !defined(DN_SINF) || !defined(DN_COSF) || !defined(DN_TANF)
    #include <math.h>
    #if !defined(DN_SQRTF)
        #define DN_SQRTF(val) sqrtf(val)
    #endif
    #if !defined(DN_SINF)
        #define DN_SINF(val) sinf(val)
    #endif
    #if !defined(DN_COSF)
        #define DN_COSF(val) cosf(val)
    #endif
    #if !defined(DN_TANF)
        #define DN_TANF(val) tanf(val)
    #endif
#endif

// NOTE: Math //////////////////////////////////////////////////////////////////////////////////////
#define DN_PI 3.14159265359f

#define DN_DEGREE_TO_RADIAN(degrees) ((degrees) * (DN_PI / 180.0f))
#define DN_RADIAN_TO_DEGREE(radians) ((radians) * (180.f * DN_PI))

#define DN_ABS(val) (((val) < 0) ? (-(val)) : (val))
#define DN_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define DN_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define DN_CLAMP(val, lo, hi) DN_MAX(DN_MIN(val, hi), lo)
#define DN_SQUARED(val) ((val) * (val))

#define DN_SWAP(a, b)   \
    do                   \
    {                    \
        auto temp = a;   \
        a        = b;    \
        b        = temp; \
    } while (0)

// NOTE: Function/Variable Annotations /////////////////////////////////////////////////////////////
#if defined(DN_STATIC_API)
    #define DN_API static
#else
    #define DN_API
#endif

#define DN_LOCAL_PERSIST static
#define DN_FILE_SCOPE static
#define DN_CAST(val) (val)

#if defined(DN_COMPILER_MSVC) || defined(DN_COMPILER_CLANG_CL)
    #define DN_FORCE_INLINE __forceinline
#else
    #define DN_FORCE_INLINE inline __attribute__((always_inline))
#endif

// NOTE: Size //////////////////////////////////////////////////////////////////////////////////////
#define DN_ISIZEOF(val) DN_CAST(ptrdiff_t)sizeof(val)
#define DN_ARRAY_UCOUNT(array) (sizeof(array)/(sizeof((array)[0])))
#define DN_ARRAY_ICOUNT(array) (DN_ISize)DN_ARRAY_UCOUNT(array)
#define DN_CHAR_COUNT(string) (sizeof(string) - 1)

// NOTE: SI Byte ///////////////////////////////////////////////////////////////////////////////////
#define DN_BYTES(val)     ((uint64_t)val)
#define DN_KILOBYTES(val) ((uint64_t)1024 * DN_BYTES(val))
#define DN_MEGABYTES(val) ((uint64_t)1024 * DN_KILOBYTES(val))
#define DN_GIGABYTES(val) ((uint64_t)1024 * DN_MEGABYTES(val))

// NOTE: Time //////////////////////////////////////////////////////////////////////////////////////
#define DN_SECONDS_TO_MS(val) ((val) * 1000)
#define DN_MINS_TO_S(val)     ((val) * 60ULL)
#define DN_HOURS_TO_S(val)    (DN_MINS_TO_S(val)  * 60ULL)
#define DN_DAYS_TO_S(val)     (DN_HOURS_TO_S(val) * 24ULL)
#define DN_WEEKS_TO_S(val)    (DN_DAYS_TO_S(val)  *  7ULL)
#define DN_YEARS_TO_S(val)    (DN_WEEKS_TO_S(val) * 52ULL)

#if defined(__has_builtin)
    #define DN_HAS_BUILTIN(expr) __has_builtin(expr)
#else
    #define DN_HAS_BUILTIN(expr) 0
#endif

// NOTE: Debug Break ///////////////////////////////////////////////////////////////////////////////
#if !defined(DN_DEBUG_BREAK)
    #if defined(NDEBUG)
        #define DN_DEBUG_BREAK
    #else
        #if defined(DN_COMPILER_MSVC) || defined(DN_COMPILER_CLANG_CL)
            #define DN_DEBUG_BREAK __debugbreak()
        #elif DN_HAS_BUILTIN(__builtin_debugtrap)
            #define DN_DEBUG_BREAK __builtin_debugtrap()
        #elif DN_HAS_BUILTIN(__builtin_trap) || defined(DN_COMPILER_GCC)
            #define DN_DEBUG_BREAK __builtin_trap()
        #else
            #include <signal.h>
            #if defined(SIGTRAP)
                #define DN_DEBUG_BREAK raise(SIGTRAP)
            #else
                #define DN_DEBUG_BREAK raise(SIGABRT)
            #endif
        #endif
    #endif
#endif

// NOTE: Assert Macros /////////////////////////////////////////////////////////////////////////////
#define DN_HARD_ASSERT(expr) DN_HARD_ASSERTF(expr, "")
#define DN_HARD_ASSERTF(expr, fmt, ...)                                                   \
    do {                                                                                   \
        if (!(expr)) {                                                                     \
            DN_Str8 stack_trace_ = DN_StackTrace_WalkStr8CRT(128 /*limit*/, 2 /*skip*/); \
            DN_Log_ErrorF("Hard assertion [" #expr "], stack trace was:\n\n%.*s\n\n" fmt, \
                           DN_STR_FMT(stack_trace_),                                      \
                           ##__VA_ARGS__);                                                 \
            DN_DEBUG_BREAK;                                                               \
        }                                                                                  \
    } while (0)

#if defined(DN_NO_ASSERT)
    #define DN_ASSERT(...)
    #define DN_ASSERT_ONCE(...)
    #define DN_ASSERTF(...)
    #define DN_ASSERTF_ONCE(...)
#else
    #define DN_ASSERT(expr) DN_ASSERTF((expr), "")
    #define DN_ASSERT_ONCE(expr) DN_ASSERTF_ONCE((expr), "")

    #define DN_ASSERTF(expr, fmt, ...)                                                        \
        do {                                                                                   \
            if (!(expr)) {                                                                     \
                DN_Str8 stack_trace_ = DN_StackTrace_WalkStr8CRT(128 /*limit*/, 2 /*skip*/); \
                DN_Log_ErrorF("Assertion [" #expr "], stack trace was:\n\n%.*s\n\n" fmt,      \
                               DN_STR_FMT(stack_trace_),                                      \
                               ##__VA_ARGS__);                                                 \
                DN_DEBUG_BREAK;                                                               \
            }                                                                                  \
        } while (0)

    #define DN_ASSERTF_ONCE(expr, fmt, ...)                                                   \
        do {                                                                                   \
            static bool once = true;                                                           \
            if (!(expr) && once) {                                                             \
                DN_Str8 stack_trace_ = DN_StackTrace_WalkStr8CRT(128 /*limit*/, 2 /*skip*/); \
                DN_Log_ErrorF("Assertion [" #expr "], stack trace was:\n\n%.*s\n\n" fmt,      \
                               DN_STR_FMT(stack_trace_),                                      \
                               ##__VA_ARGS__);                                                 \
                once = false;                                                                  \
                DN_DEBUG_BREAK;                                                               \
            }                                                                                  \
        } while (0)
#endif

#define DN_INVALID_CODE_PATHF(fmt, ...) DN_HARD_ASSERTF(0, fmt, ##__VA_ARGS__)
#define DN_INVALID_CODE_PATH            DN_INVALID_CODE_PATHF("Invalid code path triggered")

// NOTE: Check macro ///////////////////////////////////////////////////////////////////////////////
#define DN_CHECK(expr) DN_CHECKF(expr, "")

#if defined(DN_NO_CHECK_BREAK)
    #define DN_CHECKF(expr, fmt, ...) \
        ((expr) ? true : (DN_Log_TypeFCallSite(DN_LogType_Warning, DN_CALL_SITE, fmt, ## __VA_ARGS__), false))
#else
    #define DN_CHECKF(expr, fmt, ...) \
        ((expr) ? true : (DN_Log_TypeFCallSite(DN_LogType_Error, DN_CALL_SITE, fmt, ## __VA_ARGS__), DN_StackTrace_Print(128 /*limit*/), DN_DEBUG_BREAK, false))
#endif

// NOTE: Zero initialisation macro /////////////////////////////////////////////////////////////////
#if defined(__cplusplus)
    #define DN_ZERO_INIT {}
#else
    #define DN_ZERO_INIT {0}
#endif

// NOTE: Defer Macro ///////////////////////////////////////////////////////////////////////////////
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

#define DN_UNIQUE_NAME(prefix) DN_TOKEN_COMBINE(prefix, __LINE__)
#define DN_DEFER const auto DN_UNIQUE_NAME(defer_lambda_) = DN_DeferHelper() + [&]()
#endif // defined(__cplusplus)

#define DN_DEFER_LOOP(begin, end)                   \
    for (bool DN_UNIQUE_NAME(once) = (begin, true); \
         DN_UNIQUE_NAME(once);                      \
         end, DN_UNIQUE_NAME(once) = false)

// NOTE: [$TYPE] Types /////////////////////////////////////////////////////////////////////////////
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

typedef float        DN_F32;
typedef double       DN_F64;
typedef unsigned int DN_UInt;
typedef int32_t      DN_B32;

#define DN_F32_MAX   3.402823466e+38F
#define DN_F32_MIN   1.175494351e-38F
#define DN_F64_MAX   1.7976931348623158e+308
#define DN_F64_MIN   2.2250738585072014e-308
#define DN_USIZE_MAX UINTPTR_MAX
#define DN_ISIZE_MAX INTPTR_MAX
#define DN_ISIZE_MIN INTPTR_MIN

enum DN_ZeroMem
{
    DN_ZeroMem_No,  // Memory can be handed out without zero-ing it out
    DN_ZeroMem_Yes, // Memory should be zero-ed out before giving to the callee
};

struct DN_Str8
{
    char      *data; // The bytes of the string
    DN_USize  size; // The number of bytes in the string

    char const *begin() const { return data; }
    char const *end  () const { return data + size; }
    char       *begin()       { return data; }
    char       *end  ()       { return data + size; }
};

template <typename T> struct DN_Slice // A pointer and length container of data
{
    T         *data;
    DN_USize  size;

    T       *begin()       { return data; }
    T       *end  ()       { return data + size; }
    T const *begin() const { return data; }
    T const *end  () const { return data + size; }
};

// NOTE: [$CALL] DN_CallSite //////////////////////////////////////////////////////////////////////
struct DN_CallSite
{
    DN_Str8 file;
    DN_Str8 function;
    uint32_t line;
};
#define DN_CALL_SITE DN_CallSite{DN_STR8(__FILE__), DN_STR8(__func__), __LINE__}

// NOTE: [$ErrS] DN_ErrSink /////////////////////////////////////////////////////////////////////
enum DN_ErrSinkMode
{
    // Default behaviour to accumulate errors into the sink
    DN_ErrSinkMode_Nil,

    // Break into the debugger (int3) when error is encountered and the sink is
    // ended by the 'end and log' functions.
    DN_ErrSinkMode_DebugBreakOnEndAndLog,

    // When an error is encountered, exit the program with the error code of the
    // error that was caught.
    DN_ErrSinkMode_ExitOnError,
};

struct DN_ErrSinkMsg
{
    int32_t         error_code;
    DN_Str8        msg;
    DN_CallSite    call_site;
    DN_ErrSinkMsg *next;
    DN_ErrSinkMsg *prev;
};

struct DN_ErrSinkNode
{
    DN_ErrSinkMode  mode;         // Controls how the sink behaves when an error is registered onto the sink.
    DN_ErrSinkMsg  *msg_sentinel; // List of error messages accumulated for the current scope
    DN_ErrSinkNode *next;         // Next error scope
    uint64_t         arena_pos;    // Position to reset the arena when the scope is ended
};

struct DN_ErrSink
{
    // Arena solely for handling errors take from the thread's TLS
    struct DN_Arena *arena;

    // Each entry in the stack represents the errors accumulated in the scope
    // between a begin and end scope of the stink. The base sink is stored in
    // the thread's TLS.
    //
    // The stack has the latest error scope at the front.
    DN_ErrSinkNode  *stack;

    size_t debug_open_close_counter;
};

// NOTE: [$INTR] Intrinsics ////////////////////////////////////////////////////////////////////////
// NOTE: DN_Atomic_Add/Exchange return the previous value store in the target
#if defined(DN_COMPILER_MSVC) || defined(DN_COMPILER_CLANG_CL)
    #include <intrin.h>
    #define DN_Atomic_CompareExchange64(dest, desired_val, prev_val) _InterlockedCompareExchange64((__int64 volatile *)dest, desired_val, prev_val)
    #define DN_Atomic_CompareExchange32(dest, desired_val, prev_val) _InterlockedCompareExchange((long volatile *)dest, desired_val, prev_val)
    #define DN_Atomic_AddU32(target, value)                          _InterlockedExchangeAdd((long volatile *)target, value)
    #define DN_Atomic_AddU64(target, value)                          _InterlockedExchangeAdd64((__int64 volatile *)target, value)
    #define DN_Atomic_SubU32(target, value)                          DN_Atomic_AddU32(DN_CAST(long volatile *)target, (long)-value)
    #define DN_Atomic_SubU64(target, value)                          DN_Atomic_AddU64(target, (uint64_t)-value)

    #define DN_CountLeadingZerosU64(value)                           __lzcnt64(value)

    #define DN_CPU_TSC()                                             __rdtsc()
    #define DN_CompilerReadBarrierAndCPUReadFence                    _ReadBarrier(); _mm_lfence()
    #define DN_CompilerWriteBarrierAndCPUWriteFence                  _WriteBarrier(); _mm_sfence()
#elif defined(DN_COMPILER_GCC) || defined(DN_COMPILER_CLANG)
    #if defined(__ANDROID__)
    #elif defined(DN_PLATFORM_EMSCRIPTEN)
        #include <emmintrin.h>
    #else
        #include <x86intrin.h>
    #endif

    #define DN_Atomic_AddU32(target, value) __atomic_fetch_add(target, value, __ATOMIC_ACQ_REL)
    #define DN_Atomic_AddU64(target, value) __atomic_fetch_add(target, value, __ATOMIC_ACQ_REL)
    #define DN_Atomic_SubU32(target, value) __atomic_fetch_sub(target, value, __ATOMIC_ACQ_REL)
    #define DN_Atomic_SubU64(target, value) __atomic_fetch_sub(target, value, __ATOMIC_ACQ_REL)

    #define DN_CountLeadingZerosU64(value)                           __builtin_clzll(value)
    #if defined(DN_COMPILER_GCC)
        #define DN_CPU_TSC() __rdtsc()
    #else
        #define DN_CPU_TSC() __builtin_readcyclecounter()
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

#if !defined(DN_PLATFORM_ARM64)
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
    int              values[4];
};

struct DN_CPUIDArgs
{
    int eax;
    int ecx;
};

#define DN_CPU_FEAT_XMACRO                 \
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
    bool           available;
};

struct DN_CPUReport
{
    char     vendor  [4 /*bytes*/ * 3 /*EDX, ECX, EBX*/ + 1 /*null*/];
    char     brand   [48];
    uint64_t features[(DN_CPUFeature_Count / (sizeof(uint64_t) * 8)) + 1];
};

extern DN_CPUFeatureDecl g_dn_cpu_feature_decl[DN_CPUFeature_Count];
#endif // DN_PLATFORM_ARM64

// NOTE: [$TMUT] DN_TicketMutex ///////////////////////////////////////////////////////////////////
struct DN_TicketMutex
{
    unsigned int volatile ticket;  // The next ticket to give out to the thread taking the mutex
    unsigned int volatile serving; // The ticket ID to block the mutex on until it is returned
};

// NOTE: [$MUTX] DN_OSMutex ///////////////////////////////////////////////////////////////////////
struct DN_OSMutex
{
    #if defined(DN_OS_WIN32) && !defined(DN_OS_WIN32_USE_PTHREADS)
    char                win32_handle[48];
    #else
    pthread_mutex_t     posix_handle;
    pthread_mutexattr_t posix_attribs;
    #endif
};

// NOTE: [$PRIN] DN_Print /////////////////////////////////////////////////////////////////////////
enum DN_PrintStd
{
    DN_PrintStd_Out,
    DN_PrintStd_Err,
};

enum DN_PrintBold
{
    DN_PrintBold_No,
    DN_PrintBold_Yes,
};

struct DN_PrintStyle
{
    DN_PrintBold bold;
    bool          colour;
    uint8_t       r, g, b;
};

enum DN_PrintESCColour
{
    DN_PrintESCColour_Fg,
    DN_PrintESCColour_Bg,
};


// NOTE: [$LLOG] DN_Log ///////////////////////////////////////////////////////////////////////////
enum DN_LogType
{
    DN_LogType_Debug,
    DN_LogType_Info,
    DN_LogType_Warning,
    DN_LogType_Error,
    DN_LogType_Count,
};

typedef void DN_LogProc(DN_Str8 type,
                         int log_type,
                         void *user_data,
                         DN_CallSite call_site,
                         DN_FMT_ATTRIB char const *fmt,
                         va_list va);

// NOTE: [$INTR] Intrinsics ////////////////////////////////////////////////////////////////////////
DN_FORCE_INLINE       uint64_t       DN_Atomic_SetValue64                       (uint64_t volatile *target, uint64_t value);
DN_FORCE_INLINE       long           DN_Atomic_SetValue32                       (long volatile *target, long value);
#if !defined(DN_PLATFORM_ARM64)
DN_API                DN_CPUIDResult DN_CPU_ID                                  (DN_CPUIDArgs args);
DN_API                DN_USize       DN_CPU_HasFeatureArray                     (DN_CPUReport const *report, DN_CPUFeatureQuery *features, DN_USize features_size);
DN_API                bool           DN_CPU_HasFeature                          (DN_CPUReport const *report, DN_CPUFeature feature);
DN_API                bool           DN_CPU_HasAllFeatures                      (DN_CPUReport const *report, DN_CPUFeature const *features, DN_USize features_size);
template <DN_USize N> bool           DN_CPU_HasAllFeaturesCArray                (DN_CPUReport const *report, DN_CPUFeature const (&features)[N]);
DN_API                void           DN_CPU_SetFeature                          (DN_CPUReport *report, DN_CPUFeature feature);
DN_API                DN_CPUReport   DN_CPU_Report                              ();
#endif

// NOTE: [$TMUT] DN_TicketMutex ///////////////////////////////////////////////////////////////////
DN_API          void                 DN_TicketMutex_Begin                       (DN_TicketMutex *mutex);
DN_API          void                 DN_TicketMutex_End                         (DN_TicketMutex *mutex);
DN_API          DN_UInt              DN_TicketMutex_MakeTicket                  (DN_TicketMutex *mutex);
DN_API          void                 DN_TicketMutex_BeginTicket                 (DN_TicketMutex const *mutex, DN_UInt ticket);
DN_API          bool                 DN_TicketMutex_CanLock                     (DN_TicketMutex const *mutex, DN_UInt ticket);

// NOTE: [$PRIN] DN_Print /////////////////////////////////////////////////////////////////////////
// NOTE: Print Style ///////////////////////////////////////////////////////////////////////////////
DN_API          DN_PrintStyle        DN_Print_StyleColour                       (uint8_t r, uint8_t g, uint8_t b, DN_PrintBold bold);
DN_API          DN_PrintStyle        DN_Print_StyleColourU32                    (uint32_t rgb, DN_PrintBold bold);
DN_API          DN_PrintStyle        DN_Print_StyleBold                         ();

// NOTE: Print Macros //////////////////////////////////////////////////////////////////////////////
#define                              DN_Print(string)                           DN_Print_Std(DN_PrintStd_Out, string)
#define                              DN_Print_F(fmt, ...)                       DN_Print_StdF(DN_PrintStd_Out, fmt, ## __VA_ARGS__)
#define                              DN_Print_FV(fmt, args)                     DN_Print_StdFV(DN_PrintStd_Out, fmt, args)

#define                              DN_Print_Style(style, string)              DN_Print_StdStyle(DN_PrintStd_Out, style, string)
#define                              DN_Print_FStyle(style, fmt, ...)           DN_Print_StdFStyle(DN_PrintStd_Out, style, fmt, ## __VA_ARGS__)
#define                              DN_Print_FVStyle(style, fmt, args, ...)    DN_Print_StdFVStyle(DN_PrintStd_Out, style, fmt, args)

#define                              DN_Print_Ln(string)                        DN_Print_StdLn(DN_PrintStd_Out, string)
#define                              DN_Print_LnF(fmt, ...)                     DN_Print_StdLnF(DN_PrintStd_Out, fmt, ## __VA_ARGS__)
#define                              DN_Print_LnFV(fmt, args)                   DN_Print_StdLnFV(DN_PrintStd_Out, fmt, args)

#define                              DN_Print_LnStyle(style, string)            DN_Print_StdLnStyle(DN_PrintStd_Out, style, string);
#define                              DN_Print_LnFStyle(style, fmt, ...)         DN_Print_StdLnFStyle(DN_PrintStd_Out, style, fmt, ## __VA_ARGS__);
#define                              DN_Print_LnFVStyle(style, fmt, args)       DN_Print_StdLnFVStyle(DN_PrintStd_Out, style, fmt, args);

#define                              DN_Print_Err(string)                       DN_Print_Std(DN_PrintStd_Err, string)
#define                              DN_Print_ErrF(fmt, ...)                    DN_Print_StdF(DN_PrintStd_Err, fmt, ## __VA_ARGS__)
#define                              DN_Print_ErrFV(fmt, args)                  DN_Print_StdFV(DN_PrintStd_Err, fmt, args)

#define                              DN_Print_ErrStyle(style, string)           DN_Print_StdStyle(DN_PrintStd_Err, style, string)
#define                              DN_Print_ErrFStyle(style, fmt, ...)        DN_Print_StdFStyle(DN_PrintStd_Err, style, fmt, ## __VA_ARGS__)
#define                              DN_Print_ErrFVStyle(style, fmt, args, ...) DN_Print_StdFVStyle(DN_PrintStd_Err, style, fmt, args)

#define                              DN_Print_ErrLn(string)                     DN_Print_StdLn(DN_PrintStd_Err, string)
#define                              DN_Print_ErrLnF(fmt, ...)                  DN_Print_StdLnF(DN_PrintStd_Err, fmt, ## __VA_ARGS__)
#define                              DN_Print_ErrLnFV(fmt, args)                DN_Print_StdLnFV(DN_PrintStd_Err, fmt, args)

#define                              DN_Print_ErrLnStyle(style, string)         DN_Print_StdLnStyle(DN_PrintStd_Err, style, string);
#define                              DN_Print_ErrLnFStyle(style, fmt, ...)      DN_Print_StdLnFStyle(DN_PrintStd_Err, style, fmt, ## __VA_ARGS__);
#define                              DN_Print_ErrLnFVStyle(style, fmt, args)    DN_Print_StdLnFVStyle(DN_PrintStd_Err, style, fmt, args);
// NOTE: Print /////////////////////////////////////////////////////////////////////////////////////
DN_API          void                 DN_Print_Std                               (DN_PrintStd std_handle, DN_Str8 string);
DN_API          void                 DN_Print_StdF                              (DN_PrintStd std_handle, DN_FMT_ATTRIB char const *fmt, ...);
DN_API          void                 DN_Print_StdFV                             (DN_PrintStd std_handle, DN_FMT_ATTRIB char const *fmt, va_list args);

DN_API          void                 DN_Print_StdStyle                          (DN_PrintStd std_handle, DN_PrintStyle style, DN_Str8 string);
DN_API          void                 DN_Print_StdFStyle                         (DN_PrintStd std_handle, DN_PrintStyle style, DN_FMT_ATTRIB char const *fmt, ...);
DN_API          void                 DN_Print_StdFVStyle                        (DN_PrintStd std_handle, DN_PrintStyle style, DN_FMT_ATTRIB char const *fmt, va_list args);

DN_API          void                 DN_Print_StdLn                             (DN_PrintStd std_handle, DN_Str8 string);
DN_API          void                 DN_Print_StdLnF                            (DN_PrintStd std_handle, DN_FMT_ATTRIB char const *fmt, ...);
DN_API          void                 DN_Print_StdLnFV                           (DN_PrintStd std_handle, DN_FMT_ATTRIB char const *fmt, va_list args);

DN_API          void                 DN_Print_StdLnStyle                        (DN_PrintStd std_handle, DN_PrintStyle style, DN_Str8 string);
DN_API          void                 DN_Print_StdLnFStyle                       (DN_PrintStd std_handle, DN_PrintStyle style, DN_FMT_ATTRIB char const *fmt, ...);
DN_API          void                 DN_Print_StdLnFVStyle                      (DN_PrintStd std_handle, DN_PrintStyle style, DN_FMT_ATTRIB char const *fmt, va_list args);

// NOTE: ANSI Formatting Codes /////////////////////////////////////////////////////////////////////
DN_API          DN_Str8              DN_Print_ESCColourStr8                     (DN_PrintESCColour colour, uint8_t r, uint8_t g, uint8_t b);
DN_API          DN_Str8              DN_Print_ESCColourU32Str8                  (DN_PrintESCColour colour, uint32_t value);

#define                              DN_Print_ESCColourFgStr8(r, g, b)          DN_Print_ESCColourStr8(DN_PrintESCColour_Fg, r, g, b)
#define                              DN_Print_ESCColourBgStr8(r, g, b)          DN_Print_ESCColourStr8(DN_PrintESCColour_Bg, r, g, b)
#define                              DN_Print_ESCColourFg(r, g, b)              DN_Print_ESCColourStr8(DN_PrintESCColour_Fg, r, g, b).data
#define                              DN_Print_ESCColourBg(r, g, b)              DN_Print_ESCColourStr8(DN_PrintESCColour_Bg, r, g, b).data

#define                              DN_Print_ESCColourFgU32Str8(value)         DN_Print_ESCColourU32Str8(DN_PrintESCColour_Fg, value)
#define                              DN_Print_ESCColourBgU32Str8(value)         DN_Print_ESCColourU32Str8(DN_PrintESCColour_Bg, value)
#define                              DN_Print_ESCColourFgU32(value)             DN_Print_ESCColourU32Str8(DN_PrintESCColour_Fg, value).data
#define                              DN_Print_ESCColourBgU32(value)             DN_Print_ESCColourU32Str8(DN_PrintESCColour_Bg, value).data

#define                              DN_Print_ESCReset                          "\x1b[0m"
#define                              DN_Print_ESCBold                           "\x1b[1m"
#define                              DN_Print_ESCResetStr8                      DN_STR8(DN_Print_ESCReset)
#define                              DN_Print_ESCBoldStr8                       DN_STR8(DN_Print_ESCBold)
// NOTE: [$LLOG] DN_Log ///////////////////////////////////////////////////////////////////////////
#define                              DN_LogTypeColourU32_Info                   0x00'87'ff'ff // Blue
#define                              DN_LogTypeColourU32_Warning                0xff'ff'00'ff // Yellow
#define                              DN_LogTypeColourU32_Error                  0xff'00'00'ff // Red

#define                              DN_Log_DebugF(fmt, ...)                    DN_Log_TypeFCallSite (DN_LogType_Debug, DN_CALL_SITE, fmt, ## __VA_ARGS__)
#define                              DN_Log_InfoF(fmt, ...)                     DN_Log_TypeFCallSite (DN_LogType_Info, DN_CALL_SITE, fmt, ## __VA_ARGS__)
#define                              DN_Log_WarningF(fmt, ...)                  DN_Log_TypeFCallSite (DN_LogType_Warning, DN_CALL_SITE, fmt, ## __VA_ARGS__)
#define                              DN_Log_ErrorF(fmt, ...)                    DN_Log_TypeFCallSite (DN_LogType_Error, DN_CALL_SITE, fmt, ## __VA_ARGS__)
#define                              DN_Log_DebugFV(fmt, args)                  DN_Log_TypeFVCallSite(DN_LogType_Debug, DN_CALL_SITE, fmt, args)
#define                              DN_Log_InfoFV(fmt, args)                   DN_Log_TypeFVCallSite(DN_LogType_Info, DN_CALL_SITE, fmt, args)
#define                              DN_Log_WarningFV(fmt, args)                DN_Log_TypeFVCallSite(DN_LogType_Warning, DN_CALL_SITE, fmt, args)
#define                              DN_Log_ErrorFV(fmt, args)                  DN_Log_TypeFVCallSite(DN_LogType_Error, DN_CALL_SITE, fmt, args)
#define                              DN_Log_TypeFV(type, fmt, args)             DN_Log_TypeFVCallSite(type, DN_CALL_SITE, fmt, args)
#define                              DN_Log_TypeF(type, fmt, ...)               DN_Log_TypeFCallSite (type, DN_CALL_SITE, fmt, ## __VA_ARGS__)
#define                              DN_Log_FV(type, fmt, args)                 DN_Log_FVCallSite    (type, DN_CALL_SITE, fmt, args)
#define                              DN_Log_F(type, fmt, ...)                   DN_Log_FCallSite     (type, DN_CALL_SITE, fmt, ## __VA_ARGS__)

DN_API          DN_Str8              DN_Log_MakeStr8                            (struct DN_Arena *arena, bool colour, DN_Str8 type, int log_type, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API          void                 DN_Log_TypeFVCallSite                      (DN_LogType type, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, va_list va);
DN_API          void                 DN_Log_TypeFCallSite                       (DN_LogType type, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, ...);
DN_API          void                 DN_Log_FVCallSite                          (DN_Str8 type, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, va_list va);
DN_API          void                 DN_Log_FCallSite                           (DN_Str8 type, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, ...);

// NOTE: [$ERRS] DN_ErrSink /////////////////////////////////////////////////////////////////////
DN_API          DN_ErrSink *         DN_ErrSink_Begin                           (DN_ErrSinkMode mode);
#define                              DN_ErrSink_BeginDefault()                  DN_ErrSink_Begin(DN_ErrSinkMode_Nil)
DN_API          bool                 DN_ErrSink_HasError                        (DN_ErrSink *err);
DN_API          DN_ErrSinkMsg*       DN_ErrSink_End                             (DN_Arena *arena, DN_ErrSink *err);
DN_API          DN_Str8              DN_ErrSink_EndStr8                         (DN_Arena *arena, DN_ErrSink *err);
DN_API          void                 DN_ErrSink_EndAndIgnore                    (DN_ErrSink *err);

#define                              DN_ErrSink_EndAndLogError(err, err_msg)                  DN_ErrSink_EndAndLogError_     (err, DN_CALL_SITE, err_msg)
#define                              DN_ErrSink_EndAndLogErrorFV(err, fmt, args)              DN_ErrSink_EndAndLogErrorFV_   (err, DN_CALL_SITE, fmt, args)
#define                              DN_ErrSink_EndAndLogErrorF(err, fmt, ...)                DN_ErrSink_EndAndLogErrorF_    (err, DN_CALL_SITE, fmt, ##__VA_ARGS__)
#define                              DN_ErrSink_EndAndExitIfErrorFV(err, exit_val, fmt, args) DN_ErrSink_EndAndExitIfErrorFV_(err, DN_CALL_SITE, exit_val, fmt, args)
#define                              DN_ErrSink_EndAndExitIfErrorF(err, exit_val, fmt, ...)   DN_ErrSink_EndAndExitIfErrorF_ (err, DN_CALL_SITE, exit_val, fmt, ##__VA_ARGS__)

DN_API          bool                 DN_ErrSink_EndAndLogError_                 (DN_ErrSink *err, DN_CallSite call_site, DN_Str8 msg);
DN_API          bool                 DN_ErrSink_EndAndLogErrorFV_               (DN_ErrSink *err, DN_CallSite call_site,                    DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API          bool                 DN_ErrSink_EndAndLogErrorF_                (DN_ErrSink *err, DN_CallSite call_site,                    DN_FMT_ATTRIB char const *fmt, ...);
DN_API          void                 DN_ErrSink_EndAndExitIfErrorF_             (DN_ErrSink *err, DN_CallSite call_site, uint32_t exit_val, DN_FMT_ATTRIB char const *fmt, ...);
DN_API          void                 DN_ErrSink_EndAndExitIfErrorFV_            (DN_ErrSink *err, DN_CallSite call_site, uint32_t exit_val, DN_FMT_ATTRIB char const *fmt, va_list args);

#define                              DN_ErrSink_AppendFV(error, error_code, fmt, args) do { DN_TLS_SaveCallSite; DN_ErrSink_AppendFV_(error, error_code, fmt, args); } while (0)
#define                              DN_ErrSink_AppendF(error, error_code, fmt, ...)   do { DN_TLS_SaveCallSite; DN_ErrSink_AppendF_(error, error_code, fmt, ## __VA_ARGS__); } while (0)
DN_API          void                 DN_ErrSink_AppendFV_                       (DN_ErrSink *err, uint32_t error_code, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API          void                 DN_ErrSink_AppendF_                        (DN_ErrSink *err, uint32_t error_code, DN_FMT_ATTRIB char const *fmt, ...);

// NOTE: [$SDLL] DN_SentinelDLL ///////////////////////////////////////////////////////////////////
#define DN_SentinelDLL_Init(list) \
    (list)->next = (list)->prev = (list)

#define DN_SentinelDLL_InitArena(list, T, arena)                \
    do {                                                         \
        (list)       = DN_Arena_New(arena, T, DN_ZeroMem_Yes); \
        DN_SentinelDLL_Init(list);                              \
    } while (0)

#define DN_SentinelDLL_InitPool(list, T, pool) \
    do {                                        \
        (list)       = DN_Pool_New(pool, T);   \
        DN_SentinelDLL_Init(list);             \
    } while (0)

#define DN_SentinelDLL_Detach(item)           \
    do {                                       \
        if (item) {                            \
            (item)->prev->next = (item)->next; \
            (item)->next->prev = (item)->prev; \
            (item)->next       = nullptr;      \
            (item)->prev       = nullptr;      \
        }                                      \
    } while (0)

#define DN_SentinelDLL_Dequeue(list, dest_ptr) \
    if (DN_SentinelDLL_HasItems(list)) {       \
        dest_ptr = (list)->next;                \
        DN_SentinelDLL_Detach(dest_ptr);       \
    }

#define DN_SentinelDLL_Append(list, item)     \
    do {                                       \
        if (item) {                            \
            if ((item)->next)                  \
                DN_SentinelDLL_Detach(item);  \
            (item)->next       = (list)->next; \
            (item)->prev       = (list);       \
            (item)->next->prev = (item);       \
            (item)->prev->next = (item);       \
        }                                      \
    } while (0)

#define DN_SentinelDLL_Prepend(list, item)    \
    do {                                       \
        if (item) {                            \
            if ((item)->next)                  \
                DN_SentinelDLL_Detach(item);  \
            (item)->next       = (list);       \
            (item)->prev       = (list)->prev; \
            (item)->next->prev = (item);       \
            (item)->prev->next = (item);       \
        }                                      \
    } while (0)

#define DN_SentinelDLL_IsEmpty(list) \
    (!(list) || ((list) == (list)->next))

#define DN_SentinelDLL_IsInit(list) \
    ((list)->next && (list)->prev)

#define DN_SentinelDLL_HasItems(list) \
    ((list) && ((list) != (list)->next))

#define DN_SentinelDLL_ForEach(it_name, list) \
    auto *it_name = (list)->next; (it_name) != (list); (it_name) = (it_name)->next


// NOTE: [$INTR] Intrinsics ////////////////////////////////////////////////////////////////////////
DN_FORCE_INLINE uint64_t DN_Atomic_SetValue64(uint64_t volatile *target, uint64_t value)
{
    #if defined(DN_COMPILER_MSVC) || defined(DN_COMPILER_CLANG_CL)
    __int64 result;
    do {
        result = *target;
    } while (DN_Atomic_CompareExchange64(target, value, result) != result);
    return DN_CAST(uint64_t)result;
    #elif defined(DN_COMPILER_GCC) || defined(DN_COMPILER_CLANG)
    uint64_t result = __sync_lock_test_and_set(target, value);
    return result;
    #else
    #error Unsupported compiler
    #endif
}

DN_FORCE_INLINE long DN_Atomic_SetValue32(long volatile *target, long value)
{
    #if defined(DN_COMPILER_MSVC) || defined(DN_COMPILER_CLANG_CL)
    long result;
    do {
        result = *target;
    } while (DN_Atomic_CompareExchange32(target, value, result) != result);
    return result;
    #elif defined(DN_COMPILER_GCC) || defined(DN_COMPILER_CLANG)
    long result = __sync_lock_test_and_set(target, value);
    return result;
    #else
    #error Unsupported compiler
    #endif
}

template <DN_USize N> bool DN_CPU_HasAllFeaturesCArray(DN_CPUReport const *report, DN_CPUFeature const (&features)[N])
{
    bool result = DN_CPU_HasAllFeatures(report, features, N);
    return result;
}

extern struct DN_Core *g_dn_core;
