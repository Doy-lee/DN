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
// [$INTR] Intrinsics      -- Platform agnostic functions for CPU instructions (e.g. atomics, cpuid, ...)
// [$CALL] Dqn_CallSite    -- Source code location/tracing
// [$TMUT] Dqn_TicketMutex -- Userland mutex via spinlocking atomics
// [$PRIN] Dqn_Print       -- Console printing
// [$LLOG] Dqn_Log         -- Console logging macros
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

// NOTE: [$MACR] Macros ////////////////////////////////////////////////////////////////////////////
#define DQN_STRINGIFY(x) #x
#define DQN_TOKEN_COMBINE2(x, y) x ## y
#define DQN_TOKEN_COMBINE(x, y) DQN_TOKEN_COMBINE2(x, y)

// NOTE: Warning! Order is important here, clang-cl on Windows defines _MSC_VER
#if defined(_MSC_VER)
    #if defined(__clang__)
        #define DQN_COMPILER_CLANG_CL
        #define DQN_COMPILER_CLANG
    #else
        #define DQN_COMPILER_MSVC
    #endif
#elif defined(__clang__)
    #define DQN_COMPILER_CLANG
#elif defined(__GNUC__)
    #define DQN_COMPILER_GCC
#endif

// NOTE: Declare struct literals that work in both C and C++ because the syntax 
// is different between languages.
#if 0
    struct Foo { int a; }
    struct Foo foo = DQN_LITERAL(Foo){32}; // Works on both C and C++
#endif

#if defined(__cplusplus)
    #define DQN_LITERAL(T) T
#else
    #define DQN_LITERAL(T) (T)
#endif

#if defined(__cplusplus)
    #define DQN_THREAD_LOCAL thread_local
#else
    #define DQN_THREAD_LOCAL _Thread_local
#endif

#if defined(_WIN32)
    #define DQN_OS_WIN32
#elif defined(__gnu_linux__) || defined(__linux__)
    #define DQN_OS_UNIX
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <inttypes.h> // PRIu64...

#if !defined(DQN_OS_WIN32)
#include <stdlib.h> // exit()
#endif

#if !defined(DQN_PLATFORM_EMSCRIPTEN) && \
    !defined(DQN_PLATFORM_POSIX)      && \
    !defined(DQN_PLATFORM_WIN32)
    #if defined(__aarch64__) || defined(_M_ARM64)
        #define DQN_PLATFORM_ARM64
    #elif defined(__EMSCRIPTEN__)
        #define DQN_PLATFORM_EMSCRIPTEN
    #elif defined(DQN_OS_WIN32)
        #define DQN_PLATFORM_WIN32
    #else
        #define DQN_PLATFORM_POSIX
    #endif
#endif

#if defined(DQN_COMPILER_MSVC) || defined(DQN_COMPILER_CLANG_CL)
    #if defined(_CRT_SECURE_NO_WARNINGS)
        #define DQN_CRT_SECURE_NO_WARNINGS_PREVIOUSLY_DEFINED
    #else
        #define _CRT_SECURE_NO_WARNINGS
    #endif
#endif

#if defined(DQN_COMPILER_MSVC)
    #define DQN_FMT_ATTRIB _Printf_format_string_
    #define DQN_MSVC_WARNING_PUSH __pragma(warning(push))
    #define DQN_MSVC_WARNING_DISABLE(...) __pragma(warning(disable: ##__VA_ARGS__))
    #define DQN_MSVC_WARNING_POP __pragma(warning(pop))
#else
    #define DQN_FMT_ATTRIB
    #define DQN_MSVC_WARNING_PUSH
    #define DQN_MSVC_WARNING_DISABLE(...)
    #define DQN_MSVC_WARNING_POP
#endif

#if defined(DQN_COMPILER_CLANG) || defined(DQN_COMPILER_GCC) || defined(DQN_COMPILER_CLANG_CL)
    #define DQN_GCC_WARNING_PUSH _Pragma("GCC diagnostic push")
    #define DQN_GCC_WARNING_DISABLE_HELPER_0(x) #x
    #define DQN_GCC_WARNING_DISABLE_HELPER_1(y) DQN_GCC_WARNING_DISABLE_HELPER_0(GCC diagnostic ignored #y)
    #define DQN_GCC_WARNING_DISABLE(warning) _Pragma(DQN_GCC_WARNING_DISABLE_HELPER_1(warning))
    #define DQN_GCC_WARNING_POP _Pragma("GCC diagnostic pop")
#else
    #define DQN_GCC_WARNING_PUSH
    #define DQN_GCC_WARNING_DISABLE(...)
    #define DQN_GCC_WARNING_POP
#endif

// NOTE: MSVC does not support the feature detection macro for instance so we
// compile it out
#if defined(__has_feature)
    #define DQN_HAS_FEATURE(expr) __has_feature(expr)
#else
    #define DQN_HAS_FEATURE(expr) 0
#endif

#define DQN_FOR_UINDEX(index, size) for (Dqn_usize index = 0; index < size; index++)
#define DQN_FOR_IINDEX(index, size) for (Dqn_isize index = 0; index < size; index++)

#define Dqn_AlignUpPowerOfTwo(value, pot) (((uintptr_t)(value) + ((uintptr_t)(pot) - 1)) & ~((uintptr_t)(pot) - 1))
#define Dqn_AlignDownPowerOfTwo(value, pot) ((uintptr_t)(value) & ~((uintptr_t)(pot) - 1))
#define Dqn_IsPowerOfTwo(value) ((((uintptr_t)(value)) & (((uintptr_t)(value)) - 1)) == 0)
#define Dqn_IsPowerOfTwoAligned(value, pot) ((((uintptr_t)value) & (((uintptr_t)pot) - 1)) == 0)

// NOTE: String.h Dependencies /////////////////////////////////////////////////////////////////////
#if !defined(DQN_MEMCPY) || !defined(DQN_MEMSET) || !defined(DQN_MEMCMP) || !defined(DQN_MEMMOVE)
    #include <string.h>
    #if !defined(DQN_MEMCPY)
        #define DQN_MEMCPY(dest, src, count) memcpy((dest), (src), (count))
    #endif
    #if !defined(DQN_MEMSET)
        #define DQN_MEMSET(dest, value, count) memset((dest), (value), (count))
    #endif
    #if !defined(DQN_MEMCMP)
        #define DQN_MEMCMP(lhs, rhs, count) memcmp((lhs), (rhs), (count))
    #endif
    #if !defined(DQN_MEMMOVE)
        #define DQN_MEMMOVE(dest, src, count) memmove((dest), (src), (count))
    #endif
#endif

// NOTE: Math.h Dependencies ///////////////////////////////////////////////////////////////////////
#if !defined(DQN_SQRTF) || !defined(DQN_SINF) || !defined(DQN_COSF) || !defined(DQN_TANF)
    #include <math.h>
    #if !defined(DQN_SQRTF)
        #define DQN_SQRTF(val) sqrtf(val)
    #endif
    #if !defined(DQN_SINF)
        #define DQN_SINF(val) sinf(val)
    #endif
    #if !defined(DQN_COSF)
        #define DQN_COSF(val) cosf(val)
    #endif
    #if !defined(DQN_TANF)
        #define DQN_TANF(val) tanf(val)
    #endif
#endif

// NOTE: Math //////////////////////////////////////////////////////////////////////////////////////
#define DQN_PI 3.14159265359f

#define DQN_DEGREE_TO_RADIAN(degrees) ((degrees) * (DQN_PI / 180.0f))
#define DQN_RADIAN_TO_DEGREE(radians) ((radians) * (180.f * DQN_PI))

#define DQN_ABS(val) (((val) < 0) ? (-(val)) : (val))
#define DQN_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define DQN_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define DQN_CLAMP(val, lo, hi) DQN_MAX(DQN_MIN(val, hi), lo)
#define DQN_SQUARED(val) ((val) * (val))

#define DQN_SWAP(a, b)   \
    do                   \
    {                    \
        auto temp = a;   \
        a        = b;    \
        b        = temp; \
    } while (0)

// NOTE: Function/Variable Annotations /////////////////////////////////////////////////////////////
#if defined(DQN_STATIC_API)
    #define DQN_API static
#else
    #define DQN_API
#endif

#define DQN_LOCAL_PERSIST static
#define DQN_FILE_SCOPE static
#define DQN_CAST(val) (val)

#if defined(DQN_COMPILER_MSVC) || defined(DQN_COMPILER_CLANG_CL)
    #define DQN_FORCE_INLINE __forceinline
#else
    #define DQN_FORCE_INLINE inline __attribute__((always_inline))
#endif

// NOTE: Size //////////////////////////////////////////////////////////////////////////////////////
#define DQN_ISIZEOF(val) DQN_CAST(ptrdiff_t)sizeof(val)
#define DQN_ARRAY_UCOUNT(array) (sizeof(array)/(sizeof((array)[0])))
#define DQN_ARRAY_ICOUNT(array) (Dqn_isize)DQN_ARRAY_UCOUNT(array)
#define DQN_CHAR_COUNT(string) (sizeof(string) - 1)

// NOTE: SI Byte ///////////////////////////////////////////////////////////////////////////////////
#define DQN_BYTES(val)     ((uint64_t)val)
#define DQN_KILOBYTES(val) ((uint64_t)1024 * DQN_BYTES(val))
#define DQN_MEGABYTES(val) ((uint64_t)1024 * DQN_KILOBYTES(val))
#define DQN_GIGABYTES(val) ((uint64_t)1024 * DQN_MEGABYTES(val))

// NOTE: Time //////////////////////////////////////////////////////////////////////////////////////
#define DQN_SECONDS_TO_MS(val) ((val) * 1000)
#define DQN_MINS_TO_S(val)     ((val) * 60ULL)
#define DQN_HOURS_TO_S(val)    (DQN_MINS_TO_S(val)  * 60ULL)
#define DQN_DAYS_TO_S(val)     (DQN_HOURS_TO_S(val) * 24ULL)
#define DQN_WEEKS_TO_S(val)    (DQN_DAYS_TO_S(val)  *  7ULL)
#define DQN_YEARS_TO_S(val)    (DQN_WEEKS_TO_S(val) * 52ULL)

#if defined(__has_builtin)
    #define DQN_HAS_BUILTIN(expr) __has_builtin(expr)
#else
    #define DQN_HAS_BUILTIN(expr) 0
#endif

// NOTE: Debug Break ///////////////////////////////////////////////////////////////////////////////
#if !defined(DQN_DEBUG_BREAK)
    #if defined(NDEBUG)
        #define DQN_DEBUG_BREAK
    #else
        #if defined(DQN_COMPILER_MSVC) || defined(DQN_COMPILER_CLANG_CL)
            #define DQN_DEBUG_BREAK __debugbreak()
        #elif DQN_HAS_BUILTIN(__builtin_debugtrap)
            #define DQN_DEBUG_BREAK __builtin_debugtrap()
        #elif DQN_HAS_BUILTIN(__builtin_trap) || defined(DQN_COMPILER_GCC)
            #define DQN_DEBUG_BREAK __builtin_trap()
        #else
            #include <signal.h>
            #if defined(SIGTRAP)
                #define DQN_DEBUG_BREAK raise(SIGTRAP)
            #else
                #define DQN_DEBUG_BREAK raise(SIGABRT)
            #endif
        #endif
    #endif
#endif

// NOTE: Assert Macros /////////////////////////////////////////////////////////////////////////////
#define DQN_HARD_ASSERT(expr) DQN_HARD_ASSERTF(expr, "")
#define DQN_HARD_ASSERTF(expr, fmt, ...)                                                   \
    do {                                                                                   \
        if (!(expr)) {                                                                     \
            Dqn_Str8 stack_trace_ = Dqn_StackTrace_WalkStr8CRT(128 /*limit*/, 2 /*skip*/); \
            Dqn_Log_ErrorF("Hard assertion [" #expr "], stack trace was:\n\n%.*s\n\n" fmt, \
                           DQN_STR_FMT(stack_trace_),                                      \
                           ##__VA_ARGS__);                                                 \
            DQN_DEBUG_BREAK;                                                               \
        }                                                                                  \
    } while (0)

#if defined(DQN_NO_ASSERT)
    #define DQN_ASSERTF(...)
    #define DQN_ASSERT(...)
#else
    #define DQN_ASSERT(expr) DQN_ASSERTF((expr), "")
    #define DQN_ASSERTF(expr, fmt, ...)                                                        \
        do {                                                                                   \
            if (!(expr)) {                                                                     \
                Dqn_Str8 stack_trace_ = Dqn_StackTrace_WalkStr8CRT(128 /*limit*/, 2 /*skip*/); \
                Dqn_Log_ErrorF("Assertion [" #expr "], stack trace was:\n\n%.*s\n\n" fmt,      \
                               DQN_STR_FMT(stack_trace_),                                      \
                               ##__VA_ARGS__);                                                 \
                DQN_DEBUG_BREAK;                                                               \
            }                                                                                  \
        } while (0)
#endif

#define DQN_INVALID_CODE_PATHF(fmt, ...) DQN_HARD_ASSERTF(0, fmt, ##__VA_ARGS__)
#define DQN_INVALID_CODE_PATH            DQN_INVALID_CODE_PATHF("Invalid code path triggered")

// NOTE: Check macro ///////////////////////////////////////////////////////////////////////////////
#define DQN_CHECK(expr) DQN_CHECKF(expr, "")

#if defined(DQN_NO_CHECK_BREAK)
    #define DQN_CHECKF(expr, fmt, ...) \
        ((expr) ? true : (Dqn_Log_TypeFCallSite(Dqn_LogType_Warning, DQN_CALL_SITE, fmt, ## __VA_ARGS__), false))
#else
    #define DQN_CHECKF(expr, fmt, ...) \
        ((expr) ? true : (Dqn_Log_TypeFCallSite(Dqn_LogType_Error, DQN_CALL_SITE, fmt, ## __VA_ARGS__), Dqn_StackTrace_Print(128 /*limit*/), DQN_DEBUG_BREAK, false))
#endif

// NOTE: Zero initialisation macro /////////////////////////////////////////////////////////////////
#if defined(__cplusplus)
    #define DQN_ZERO_INIT {}
#else
    #define DQN_ZERO_INIT {0}
#endif

// NOTE: Defer Macro ///////////////////////////////////////////////////////////////////////////////
#if defined(__cplusplus)
template <typename Procedure>
struct Dqn_Defer
{
    Procedure proc;
    Dqn_Defer(Procedure p) : proc(p) {}
    ~Dqn_Defer() { proc(); }
};

struct Dqn_DeferHelper
{
    template <typename Lambda>
    Dqn_Defer<Lambda> operator+(Lambda lambda) { return Dqn_Defer<Lambda>(lambda); };
};

#define DQN_UNIQUE_NAME(prefix) DQN_TOKEN_COMBINE(prefix, __LINE__)
#define DQN_DEFER const auto DQN_UNIQUE_NAME(defer_lambda_) = Dqn_DeferHelper() + [&]()
#endif // defined(__cplusplus)

#define DQN_DEFER_LOOP(begin, end)                   \
    for (bool DQN_UNIQUE_NAME(once) = (begin, true); \
         DQN_UNIQUE_NAME(once);                      \
         end, DQN_UNIQUE_NAME(once) = false)

// NOTE: [$TYPE] Types /////////////////////////////////////////////////////////////////////////////
typedef intptr_t     Dqn_isize;
typedef uintptr_t    Dqn_usize;
typedef intptr_t     Dqn_isize;
typedef float        Dqn_f32;
typedef double       Dqn_f64;
typedef unsigned int Dqn_uint;
typedef int32_t      Dqn_b32;

#define DQN_F32_MAX   3.402823466e+38F
#define DQN_F32_MIN   1.175494351e-38F
#define DQN_F64_MAX   1.7976931348623158e+308
#define DQN_F64_MIN   2.2250738585072014e-308
#define DQN_USIZE_MAX UINTPTR_MAX
#define DQN_ISIZE_MAX INTPTR_MAX
#define DQN_ISIZE_MIN INTPTR_MIN

enum Dqn_ZeroMem
{
    Dqn_ZeroMem_No,  // Memory can be handed out without zero-ing it out
    Dqn_ZeroMem_Yes, // Memory should be zero-ed out before giving to the callee
};

struct Dqn_Str8
{
    char      *data; // The bytes of the string
    Dqn_usize  size; // The number of bytes in the string

    char const *begin() const { return data; }
    char const *end  () const { return data + size; }
    char       *begin()       { return data; }
    char       *end  ()       { return data + size; }
};

template <typename T> struct Dqn_Slice // A pointer and length container of data
{
    T         *data;
    Dqn_usize  size;

    T       *begin()       { return data; }
    T       *end  ()       { return data + size; }
    T const *begin() const { return data; }
    T const *end  () const { return data + size; }
};

// NOTE: [$CALL] Dqn_CallSite //////////////////////////////////////////////////////////////////////
struct Dqn_CallSite
{
    Dqn_Str8 file;
    Dqn_Str8 function;
    uint32_t line;
};
#define DQN_CALL_SITE Dqn_CallSite{DQN_STR8(__FILE__), DQN_STR8(__func__), __LINE__}

// NOTE: [$ERRS] Dqn_ErrorSink /////////////////////////////////////////////////////////////////////
enum Dqn_ErrorSinkMode
{
    Dqn_ErrorSinkMode_Nil,
    Dqn_ErrorSinkMode_ExitOnError,
};

struct Dqn_ErrorSinkNode
{
    Dqn_ErrorSinkMode  mode;
    bool               error;
    int32_t            error_code;
    Dqn_Str8           msg;
    Dqn_CallSite       call_site;
    Dqn_ErrorSinkNode *next;
    uint64_t           arena_pos;
};

struct Dqn_ErrorSink
{
    struct Dqn_Arena  *arena;
    Dqn_ErrorSinkNode *stack;
};

// NOTE: [$INTR] Intrinsics ////////////////////////////////////////////////////////////////////////
// NOTE: Dqn_Atomic_Add/Exchange return the previous value store in the target
#if defined(DQN_COMPILER_MSVC) || defined(DQN_COMPILER_CLANG_CL)
    #include <intrin.h>
    #define Dqn_Atomic_CompareExchange64(dest, desired_val, prev_val) _InterlockedCompareExchange64((__int64 volatile *)dest, desired_val, prev_val)
    #define Dqn_Atomic_CompareExchange32(dest, desired_val, prev_val) _InterlockedCompareExchange((long volatile *)dest, desired_val, prev_val)
    #define Dqn_Atomic_AddU32(target, value)                          _InterlockedExchangeAdd((long volatile *)target, value)
    #define Dqn_Atomic_AddU64(target, value)                          _InterlockedExchangeAdd64((__int64 volatile *)target, value)
    #define Dqn_Atomic_SubU32(target, value)                          Dqn_Atomic_AddU32(DQN_CAST(long volatile *)target, (long)-value)
    #define Dqn_Atomic_SubU64(target, value)                          Dqn_Atomic_AddU64(target, (uint64_t)-value)

    #define Dqn_CountLeadingZerosU64(value)                           __lzcnt64(value)

    #define Dqn_CPU_TSC()                                             __rdtsc()
    #define Dqn_CompilerReadBarrierAndCPUReadFence                    _ReadBarrier(); _mm_lfence()
    #define Dqn_CompilerWriteBarrierAndCPUWriteFence                  _WriteBarrier(); _mm_sfence()
#elif defined(DQN_COMPILER_GCC) || defined(DQN_COMPILER_CLANG)
    #if defined(__ANDROID__)
    #elif defined(DQN_PLATFORM_EMSCRIPTEN)
        #include <emmintrin.h>
    #else
        #include <x86intrin.h>
    #endif

    #define Dqn_Atomic_AddU32(target, value) __atomic_fetch_add(target, value, __ATOMIC_ACQ_REL)
    #define Dqn_Atomic_AddU64(target, value) __atomic_fetch_add(target, value, __ATOMIC_ACQ_REL)
    #define Dqn_Atomic_SubU32(target, value) __atomic_fetch_sub(target, value, __ATOMIC_ACQ_REL)
    #define Dqn_Atomic_SubU64(target, value) __atomic_fetch_sub(target, value, __ATOMIC_ACQ_REL)

    #define Dqn_CountLeadingZerosU64(value)                           __builtin_clzll(value)
    #if defined(DQN_COMPILER_GCC)
        #define Dqn_CPU_TSC() __rdtsc()
    #else
        #define Dqn_CPU_TSC() __builtin_readcyclecounter()
    #endif
    #if defined(DQN_PLATFORM_EMSCRIPTEN)
        #define Dqn_CompilerReadBarrierAndCPUReadFence
        #define Dqn_CompilerWriteBarrierAndCPUWriteFence
    #else
        #define Dqn_CompilerReadBarrierAndCPUReadFence   asm volatile("lfence" ::: "memory")
        #define Dqn_CompilerWriteBarrierAndCPUWriteFence asm volatile("sfence" ::: "memory")
    #endif
#else
    #error "Compiler not supported"
#endif

#if !defined(DQN_PLATFORM_ARM64)
struct Dqn_CPURegisters
{
    int eax;
    int ebx;
    int ecx;
    int edx;
};

union Dqn_CPUIDResult
{
    Dqn_CPURegisters reg;
    int              values[4];
};

struct Dqn_CPUIDArgs
{
    int eax;
    int ecx;
};

#define DQN_CPU_FEAT_XMACRO                 \
    DQN_CPU_FEAT_XENTRY(3DNow)              \
    DQN_CPU_FEAT_XENTRY(3DNowExt)           \
    DQN_CPU_FEAT_XENTRY(ABM)                \
    DQN_CPU_FEAT_XENTRY(AES)                \
    DQN_CPU_FEAT_XENTRY(AVX)                \
    DQN_CPU_FEAT_XENTRY(AVX2)               \
    DQN_CPU_FEAT_XENTRY(AVX512F)            \
    DQN_CPU_FEAT_XENTRY(AVX512DQ)           \
    DQN_CPU_FEAT_XENTRY(AVX512IFMA)         \
    DQN_CPU_FEAT_XENTRY(AVX512PF)           \
    DQN_CPU_FEAT_XENTRY(AVX512ER)           \
    DQN_CPU_FEAT_XENTRY(AVX512CD)           \
    DQN_CPU_FEAT_XENTRY(AVX512BW)           \
    DQN_CPU_FEAT_XENTRY(AVX512VL)           \
    DQN_CPU_FEAT_XENTRY(AVX512VBMI)         \
    DQN_CPU_FEAT_XENTRY(AVX512VBMI2)        \
    DQN_CPU_FEAT_XENTRY(AVX512VNNI)         \
    DQN_CPU_FEAT_XENTRY(AVX512BITALG)       \
    DQN_CPU_FEAT_XENTRY(AVX512VPOPCNTDQ)    \
    DQN_CPU_FEAT_XENTRY(AVX5124VNNIW)       \
    DQN_CPU_FEAT_XENTRY(AVX5124FMAPS)       \
    DQN_CPU_FEAT_XENTRY(AVX512VP2INTERSECT) \
    DQN_CPU_FEAT_XENTRY(AVX512FP16)         \
    DQN_CPU_FEAT_XENTRY(CLZERO)             \
    DQN_CPU_FEAT_XENTRY(CMPXCHG8B)          \
    DQN_CPU_FEAT_XENTRY(CMPXCHG16B)         \
    DQN_CPU_FEAT_XENTRY(F16C)               \
    DQN_CPU_FEAT_XENTRY(FMA)                \
    DQN_CPU_FEAT_XENTRY(FMA4)               \
    DQN_CPU_FEAT_XENTRY(FP128)              \
    DQN_CPU_FEAT_XENTRY(FP256)              \
    DQN_CPU_FEAT_XENTRY(FPU)                \
    DQN_CPU_FEAT_XENTRY(MMX)                \
    DQN_CPU_FEAT_XENTRY(MONITOR)            \
    DQN_CPU_FEAT_XENTRY(MOVBE)              \
    DQN_CPU_FEAT_XENTRY(MOVU)               \
    DQN_CPU_FEAT_XENTRY(MmxExt)             \
    DQN_CPU_FEAT_XENTRY(PCLMULQDQ)          \
    DQN_CPU_FEAT_XENTRY(POPCNT)             \
    DQN_CPU_FEAT_XENTRY(RDRAND)             \
    DQN_CPU_FEAT_XENTRY(RDSEED)             \
    DQN_CPU_FEAT_XENTRY(RDTSCP)             \
    DQN_CPU_FEAT_XENTRY(SHA)                \
    DQN_CPU_FEAT_XENTRY(SSE)                \
    DQN_CPU_FEAT_XENTRY(SSE2)               \
    DQN_CPU_FEAT_XENTRY(SSE3)               \
    DQN_CPU_FEAT_XENTRY(SSE41)              \
    DQN_CPU_FEAT_XENTRY(SSE42)              \
    DQN_CPU_FEAT_XENTRY(SSE4A)              \
    DQN_CPU_FEAT_XENTRY(SSSE3)              \
    DQN_CPU_FEAT_XENTRY(TSC)                \
    DQN_CPU_FEAT_XENTRY(TscInvariant)       \
    DQN_CPU_FEAT_XENTRY(VAES)               \
    DQN_CPU_FEAT_XENTRY(VPCMULQDQ)

enum Dqn_CPUFeature
{
    #define DQN_CPU_FEAT_XENTRY(label) Dqn_CPUFeature_##label,
    DQN_CPU_FEAT_XMACRO
    #undef DQN_CPU_FEAT_XENTRY
    Dqn_CPUFeature_Count,
};

struct Dqn_CPUFeatureDecl
{
    Dqn_CPUFeature value;
    Dqn_Str8       label;
};

struct Dqn_CPUFeatureQuery
{
    Dqn_CPUFeature feature;
    bool           available;
};

struct Dqn_CPUReport
{
    char     vendor  [4 /*bytes*/ * 3 /*EDX, ECX, EBX*/ + 1 /*null*/];
    char     brand   [48];
    uint64_t features[(Dqn_CPUFeature_Count / (sizeof(uint64_t) * 8)) + 1];
};

extern Dqn_CPUFeatureDecl g_dqn_cpu_feature_decl[Dqn_CPUFeature_Count];
#endif // DQN_PLATFORM_ARM64

// NOTE: [$TMUT] Dqn_TicketMutex ///////////////////////////////////////////////////////////////////
struct Dqn_TicketMutex
{
    unsigned int volatile ticket;  // The next ticket to give out to the thread taking the mutex
    unsigned int volatile serving; // The ticket ID to block the mutex on until it is returned
};

// NOTE: [$MUTX] Dqn_OSMutex ///////////////////////////////////////////////////////////////////////
struct Dqn_OSMutex
{
    #if defined(DQN_OS_WIN32) && !defined(DQN_OS_WIN32_USE_PTHREADS)
    char                win32_handle[48];
    #else
    pthread_mutex_t     posix_handle;
    pthread_mutexattr_t posix_attribs;
    #endif
};

// NOTE: [$PRIN] Dqn_Print /////////////////////////////////////////////////////////////////////////
enum Dqn_PrintStd
{
    Dqn_PrintStd_Out,
    Dqn_PrintStd_Err,
};

enum Dqn_PrintBold
{
    Dqn_PrintBold_No,
    Dqn_PrintBold_Yes,
};

struct Dqn_PrintStyle
{
    Dqn_PrintBold bold;
    bool          colour;
    uint8_t       r, g, b;
};

enum Dqn_PrintESCColour
{
    Dqn_PrintESCColour_Fg,
    Dqn_PrintESCColour_Bg,
};


// NOTE: [$LLOG] Dqn_Log ///////////////////////////////////////////////////////////////////////////
enum Dqn_LogType
{
    Dqn_LogType_Debug,
    Dqn_LogType_Info,
    Dqn_LogType_Warning,
    Dqn_LogType_Error,
    Dqn_LogType_Count,
};

typedef void Dqn_LogProc(Dqn_Str8 type,
                         int log_type,
                         void *user_data,
                         Dqn_CallSite call_site,
                         DQN_FMT_ATTRIB char const *fmt,
                         va_list va);

// NOTE: [$INTR] Intrinsics ////////////////////////////////////////////////////////////////////////
DQN_FORCE_INLINE uint64_t           Dqn_Atomic_SetValue64                       (uint64_t volatile *target, uint64_t value);
DQN_FORCE_INLINE long               Dqn_Atomic_SetValue32                       (long volatile *target, long value);
#if !defined(DQN_PLATFORM_ARM64)
DQN_API          Dqn_CPUIDResult    Dqn_CPU_ID                                  (Dqn_CPUIDArgs args);
DQN_API          Dqn_usize          Dqn_CPU_HasFeatureArray                     (Dqn_CPUReport const *report, Dqn_CPUFeatureQuery *features, Dqn_usize features_size);
DQN_API          bool               Dqn_CPU_HasFeature                          (Dqn_CPUReport const *report, Dqn_CPUFeature feature);
DQN_API          bool               Dqn_CPU_HasAllFeatures                      (Dqn_CPUReport const *report, Dqn_CPUFeature const *features, Dqn_usize features_size);
template <Dqn_usize N> bool         Dqn_CPU_HasAllFeaturesCArray                (Dqn_CPUReport const *report, Dqn_CPUFeature const (&features)[N]);
DQN_API          void               Dqn_CPU_SetFeature                          (Dqn_CPUReport *report, Dqn_CPUFeature feature);
DQN_API          Dqn_CPUReport      Dqn_CPU_Report                              ();
#endif

// NOTE: [$TMUT] Dqn_TicketMutex ///////////////////////////////////////////////////////////////////
DQN_API          void               Dqn_TicketMutex_Begin                       (Dqn_TicketMutex *mutex);
DQN_API          void               Dqn_TicketMutex_End                         (Dqn_TicketMutex *mutex);
DQN_API          Dqn_uint           Dqn_TicketMutex_MakeTicket                  (Dqn_TicketMutex *mutex);
DQN_API          void               Dqn_TicketMutex_BeginTicket                 (Dqn_TicketMutex const *mutex, Dqn_uint ticket);
DQN_API          bool               Dqn_TicketMutex_CanLock                     (Dqn_TicketMutex const *mutex, Dqn_uint ticket);

// NOTE: [$PRIN] Dqn_Print /////////////////////////////////////////////////////////////////////////
// NOTE: Print Style ///////////////////////////////////////////////////////////////////////////////
DQN_API          Dqn_PrintStyle     Dqn_Print_StyleColour                       (uint8_t r, uint8_t g, uint8_t b, Dqn_PrintBold bold);
DQN_API          Dqn_PrintStyle     Dqn_Print_StyleColourU32                    (uint32_t rgb, Dqn_PrintBold bold);
DQN_API          Dqn_PrintStyle     Dqn_Print_StyleBold                         ();

// NOTE: Print Macros //////////////////////////////////////////////////////////////////////////////
#define                             Dqn_Print(string)                           Dqn_Print_Std(Dqn_PrintStd_Out, string)
#define                             Dqn_Print_F(fmt, ...)                       Dqn_Print_StdF(Dqn_PrintStd_Out, fmt, ## __VA_ARGS__)
#define                             Dqn_Print_FV(fmt, args)                     Dqn_Print_StdFV(Dqn_PrintStd_Out, fmt, args)

#define                             Dqn_Print_Style(style, string)              Dqn_Print_StdStyle(Dqn_PrintStd_Out, style, string)
#define                             Dqn_Print_FStyle(style, fmt, ...)           Dqn_Print_StdFStyle(Dqn_PrintStd_Out, style, fmt, ## __VA_ARGS__)
#define                             Dqn_Print_FVStyle(style, fmt, args, ...)    Dqn_Print_StdFVStyle(Dqn_PrintStd_Out, style, fmt, args)

#define                             Dqn_Print_Ln(string)                        Dqn_Print_StdLn(Dqn_PrintStd_Out, string)
#define                             Dqn_Print_LnF(fmt, ...)                     Dqn_Print_StdLnF(Dqn_PrintStd_Out, fmt, ## __VA_ARGS__)
#define                             Dqn_Print_LnFV(fmt, args)                   Dqn_Print_StdLnFV(Dqn_PrintStd_Out, fmt, args)

#define                             Dqn_Print_LnStyle(style, string)            Dqn_Print_StdLnStyle(Dqn_PrintStd_Out, style, string);
#define                             Dqn_Print_LnFStyle(style, fmt, ...)         Dqn_Print_StdLnFStyle(Dqn_PrintStd_Out, style, fmt, ## __VA_ARGS__);
#define                             Dqn_Print_LnFVStyle(style, fmt, args)       Dqn_Print_StdLnFVStyle(Dqn_PrintStd_Out, style, fmt, args);

#define                             Dqn_Print_Err(string)                       Dqn_Print_Std(Dqn_PrintStd_Err, string)
#define                             Dqn_Print_ErrF(fmt, ...)                    Dqn_Print_StdF(Dqn_PrintStd_Err, fmt, ## __VA_ARGS__)
#define                             Dqn_Print_ErrFV(fmt, args)                  Dqn_Print_StdFV(Dqn_PrintStd_Err, fmt, args)

#define                             Dqn_Print_ErrStyle(style, string)           Dqn_Print_StdStyle(Dqn_PrintStd_Err, style, string)
#define                             Dqn_Print_ErrFStyle(style, fmt, ...)        Dqn_Print_StdFStyle(Dqn_PrintStd_Err, style, fmt, ## __VA_ARGS__)
#define                             Dqn_Print_ErrFVStyle(style, fmt, args, ...) Dqn_Print_StdFVStyle(Dqn_PrintStd_Err, style, fmt, args)

#define                             Dqn_Print_ErrLn(string)                     Dqn_Print_StdLn(Dqn_PrintStd_Err, string)
#define                             Dqn_Print_ErrLnF(fmt, ...)                  Dqn_Print_StdLnF(Dqn_PrintStd_Err, fmt, ## __VA_ARGS__)
#define                             Dqn_Print_ErrLnFV(fmt, args)                Dqn_Print_StdLnFV(Dqn_PrintStd_Err, fmt, args)

#define                             Dqn_Print_ErrLnStyle(style, string)         Dqn_Print_StdLnStyle(Dqn_PrintStd_Err, style, string);
#define                             Dqn_Print_ErrLnFStyle(style, fmt, ...)      Dqn_Print_StdLnFStyle(Dqn_PrintStd_Err, style, fmt, ## __VA_ARGS__);
#define                             Dqn_Print_ErrLnFVStyle(style, fmt, args)    Dqn_Print_StdLnFVStyle(Dqn_PrintStd_Err, style, fmt, args);
// NOTE: Print /////////////////////////////////////////////////////////////////////////////////////
DQN_API          void               Dqn_Print_Std                               (Dqn_PrintStd std_handle, Dqn_Str8 string);
DQN_API          void               Dqn_Print_StdF                              (Dqn_PrintStd std_handle, DQN_FMT_ATTRIB char const *fmt, ...);
DQN_API          void               Dqn_Print_StdFV                             (Dqn_PrintStd std_handle, DQN_FMT_ATTRIB char const *fmt, va_list args);

DQN_API          void               Dqn_Print_StdStyle                          (Dqn_PrintStd std_handle, Dqn_PrintStyle style, Dqn_Str8 string);
DQN_API          void               Dqn_Print_StdFStyle                         (Dqn_PrintStd std_handle, Dqn_PrintStyle style, DQN_FMT_ATTRIB char const *fmt, ...);
DQN_API          void               Dqn_Print_StdFVStyle                        (Dqn_PrintStd std_handle, Dqn_PrintStyle style, DQN_FMT_ATTRIB char const *fmt, va_list args);

DQN_API          void               Dqn_Print_StdLn                             (Dqn_PrintStd std_handle, Dqn_Str8 string);
DQN_API          void               Dqn_Print_StdLnF                            (Dqn_PrintStd std_handle, DQN_FMT_ATTRIB char const *fmt, ...);
DQN_API          void               Dqn_Print_StdLnFV                           (Dqn_PrintStd std_handle, DQN_FMT_ATTRIB char const *fmt, va_list args);

DQN_API          void               Dqn_Print_StdLnStyle                        (Dqn_PrintStd std_handle, Dqn_PrintStyle style, Dqn_Str8 string);
DQN_API          void               Dqn_Print_StdLnFStyle                       (Dqn_PrintStd std_handle, Dqn_PrintStyle style, DQN_FMT_ATTRIB char const *fmt, ...);
DQN_API          void               Dqn_Print_StdLnFVStyle                      (Dqn_PrintStd std_handle, Dqn_PrintStyle style, DQN_FMT_ATTRIB char const *fmt, va_list args);

// NOTE: ANSI Formatting Codes /////////////////////////////////////////////////////////////////////
DQN_API          Dqn_Str8           Dqn_Print_ESCColourStr8                     (Dqn_PrintESCColour colour, uint8_t r, uint8_t g, uint8_t b);
DQN_API          Dqn_Str8           Dqn_Print_ESCColourU32Str8                  (Dqn_PrintESCColour colour, uint32_t value);

#define                             Dqn_Print_ESCColourFgStr8(r, g, b)          Dqn_Print_ESCColourStr8(Dqn_PrintESCColour_Fg, r, g, b)
#define                             Dqn_Print_ESCColourBgStr8(r, g, b)          Dqn_Print_ESCColourStr8(Dqn_PrintESCColour_Bg, r, g, b)
#define                             Dqn_Print_ESCColourFg(r, g, b)              Dqn_Print_ESCColourStr8(Dqn_PrintESCColour_Fg, r, g, b).data
#define                             Dqn_Print_ESCColourBg(r, g, b)              Dqn_Print_ESCColourStr8(Dqn_PrintESCColour_Bg, r, g, b).data

#define                             Dqn_Print_ESCColourFgU32Str8(value)         Dqn_Print_ESCColourU32Str8(Dqn_PrintESCColour_Fg, value)
#define                             Dqn_Print_ESCColourBgU32Str8(value)         Dqn_Print_ESCColourU32Str8(Dqn_PrintESCColour_Bg, value)
#define                             Dqn_Print_ESCColourFgU32(value)             Dqn_Print_ESCColourU32Str8(Dqn_PrintESCColour_Fg, value).data
#define                             Dqn_Print_ESCColourBgU32(value)             Dqn_Print_ESCColourU32Str8(Dqn_PrintESCColour_Bg, value).data

#define                             Dqn_Print_ESCReset                          "\x1b[0m"
#define                             Dqn_Print_ESCBold                           "\x1b[1m"
#define                             Dqn_Print_ESCResetStr8                      DQN_STR8(Dqn_Print_ESCReset)
#define                             Dqn_Print_ESCBoldStr8                       DQN_STR8(Dqn_Print_ESCBold)
// NOTE: [$LLOG] Dqn_Log ///////////////////////////////////////////////////////////////////////////
#define                             Dqn_LogTypeColourU32_Info                   0x00'87'ff'ff // Blue
#define                             Dqn_LogTypeColourU32_Warning                0xff'ff'00'ff // Yellow
#define                             Dqn_LogTypeColourU32_Error                  0xff'00'00'ff // Red

#define                             Dqn_Log_DebugF(fmt, ...)                    Dqn_Log_TypeFCallSite (Dqn_LogType_Debug, DQN_CALL_SITE, fmt, ## __VA_ARGS__)
#define                             Dqn_Log_InfoF(fmt, ...)                     Dqn_Log_TypeFCallSite (Dqn_LogType_Info, DQN_CALL_SITE, fmt, ## __VA_ARGS__)
#define                             Dqn_Log_WarningF(fmt, ...)                  Dqn_Log_TypeFCallSite (Dqn_LogType_Warning, DQN_CALL_SITE, fmt, ## __VA_ARGS__)
#define                             Dqn_Log_ErrorF(fmt, ...)                    Dqn_Log_TypeFCallSite (Dqn_LogType_Error, DQN_CALL_SITE, fmt, ## __VA_ARGS__)
#define                             Dqn_Log_DebugFV(fmt, args)                  Dqn_Log_TypeFVCallSite(Dqn_LogType_Debug, DQN_CALL_SITE, fmt, args)
#define                             Dqn_Log_InfoFV(fmt, args)                   Dqn_Log_TypeFVCallSite(Dqn_LogType_Info, DQN_CALL_SITE, fmt, args)
#define                             Dqn_Log_WarningFV(fmt, args)                Dqn_Log_TypeFVCallSite(Dqn_LogType_Warning, DQN_CALL_SITE, fmt, args)
#define                             Dqn_Log_ErrorFV(fmt, args)                  Dqn_Log_TypeFVCallSite(Dqn_LogType_Error, DQN_CALL_SITE, fmt, args)
#define                             Dqn_Log_TypeFV(type, fmt, args)             Dqn_Log_TypeFVCallSite(type, DQN_CALL_SITE, fmt, args)
#define                             Dqn_Log_TypeF(type, fmt, ...)               Dqn_Log_TypeFCallSite (type, DQN_CALL_SITE, fmt, ## __VA_ARGS__)
#define                             Dqn_Log_FV(type, fmt, args)                 Dqn_Log_FVCallSite    (type, DQN_CALL_SITE, fmt, args)
#define                             Dqn_Log_F(type, fmt, ...)                   Dqn_Log_FCallSite     (type, DQN_CALL_SITE, fmt, ## __VA_ARGS__)

DQN_API          Dqn_Str8           Dqn_Log_MakeStr8                            (struct Dqn_Arena *arena, bool colour, Dqn_Str8 type, int log_type, Dqn_CallSite call_site, DQN_FMT_ATTRIB char const *fmt, va_list args);
DQN_API          void               Dqn_Log_TypeFVCallSite                      (Dqn_LogType type, Dqn_CallSite call_site, DQN_FMT_ATTRIB char const *fmt, va_list va);
DQN_API          void               Dqn_Log_TypeFCallSite                       (Dqn_LogType type, Dqn_CallSite call_site, DQN_FMT_ATTRIB char const *fmt, ...);
DQN_API          void               Dqn_Log_FVCallSite                          (Dqn_Str8 type, Dqn_CallSite call_site, DQN_FMT_ATTRIB char const *fmt, va_list va);
DQN_API          void               Dqn_Log_FCallSite                           (Dqn_Str8 type, Dqn_CallSite call_site, DQN_FMT_ATTRIB char const *fmt, ...);

// NOTE: [$ERRS] Dqn_ErrorSink /////////////////////////////////////////////////////////////////////
DQN_API          Dqn_ErrorSink *    Dqn_ErrorSink_Begin                         (Dqn_ErrorSinkMode mode);
DQN_API          bool               Dqn_ErrorSink_HasError                      (Dqn_ErrorSink *error);
DQN_API          Dqn_ErrorSinkNode  Dqn_ErrorSink_End                           (Dqn_Arena *arena, Dqn_ErrorSink *error);
DQN_API          void               Dqn_ErrorSink_EndAndIgnore                  (Dqn_ErrorSink *error);
DQN_API          bool               Dqn_ErrorSink_EndAndLogError                (Dqn_ErrorSink *error, Dqn_Str8 error_msg);
DQN_API          bool               Dqn_ErrorSink_EndAndLogErrorFV              (Dqn_ErrorSink *error, DQN_FMT_ATTRIB char const *fmt, va_list args);
DQN_API          bool               Dqn_ErrorSink_EndAndLogErrorF               (Dqn_ErrorSink *error, DQN_FMT_ATTRIB char const *fmt, ...);
DQN_API          void               Dqn_ErrorSink_EndAndExitIfErrorF            (Dqn_ErrorSink *error, uint32_t exit_code, DQN_FMT_ATTRIB char const *fmt, ...);
DQN_API          void               Dqn_ErrorSink_EndAndExitIfErrorFV           (Dqn_ErrorSink *error, uint32_t exit_code, DQN_FMT_ATTRIB char const *fmt, va_list args);

#define                             Dqn_ErrorSink_MakeFV(error, error_code, fmt, args) do { Dqn_TLS_SaveCallSite; Dqn_ErrorSink_MakeFV_(error, error_code, fmt, args); } while (0)
#define                             Dqn_ErrorSink_MakeF(error, error_code, fmt, ...)   do { Dqn_TLS_SaveCallSite; Dqn_ErrorSink_MakeF_(error, error_code, fmt, ## __VA_ARGS__); } while (0)
DQN_API          void               Dqn_ErrorSink_MakeFV_                       (Dqn_ErrorSink *error, uint32_t error_code, DQN_FMT_ATTRIB char const *fmt, va_list args);
DQN_API          void               Dqn_ErrorSink_MakeF_                        (Dqn_ErrorSink *error, uint32_t error_code, DQN_FMT_ATTRIB char const *fmt, ...);

// NOTE: [$INTR] Intrinsics ////////////////////////////////////////////////////////////////////////
DQN_FORCE_INLINE uint64_t Dqn_Atomic_SetValue64(uint64_t volatile *target, uint64_t value)
{
    #if defined(DQN_COMPILER_MSVC) || defined(DQN_COMPILER_CLANG_CL)
    __int64 result;
    do {
        result = *target;
    } while (Dqn_Atomic_CompareExchange64(target, value, result) != result);
    return DQN_CAST(uint64_t)result;
    #elif defined(DQN_COMPILER_GCC) || defined(DQN_COMPILER_CLANG)
    uint64_t result = __sync_lock_test_and_set(target, value);
    return result;
    #else
    #error Unsupported compiler
    #endif
}

DQN_FORCE_INLINE long Dqn_Atomic_SetValue32(long volatile *target, long value)
{
    #if defined(DQN_COMPILER_MSVC) || defined(DQN_COMPILER_CLANG_CL)
    long result;
    do {
        result = *target;
    } while (Dqn_Atomic_CompareExchange32(target, value, result) != result);
    return result;
    #elif defined(DQN_COMPILER_GCC) || defined(DQN_COMPILER_CLANG)
    long result = __sync_lock_test_and_set(target, value);
    return result;
    #else
    #error Unsupported compiler
    #endif
}

template <Dqn_usize N> bool Dqn_CPU_HasAllFeaturesCArray(Dqn_CPUReport const *report, Dqn_CPUFeature const (&features)[N])
{
    bool result = Dqn_CPU_HasAllFeatures(report, features, N);
    return result;
}
