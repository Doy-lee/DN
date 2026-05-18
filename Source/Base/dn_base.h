#if !defined(DN_BASE_H)
#define DN_BASE_H

#if defined(_CLANGD)
  #include "../dn.h"
#endif

// NOTE: Compiler identification
// Warning! Order is important here, clang-cl on Windows defines _MSC_VER
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
// MSVC for example does not support the feature detection macro for instance so we compile it out
#if defined(__has_feature)
  #define DN_HAS_FEATURE(expr) __has_feature(expr)
#else
  #define DN_HAS_FEATURE(expr) 0
#endif

// NOTE: __has_builtin
// MSVC for example does not support the feature detection macro for instance so we compile it out
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

// NOTE: Address sanitizer
#if !defined(DN_ASAN_POISON)
  #define DN_ASAN_POISON 0
#endif

#if !defined(DN_ASAN_VET_POISON)
  #define DN_ASAN_VET_POISON 0
#endif

#define DN_ASAN_POISON_ALIGNMENT 8

#if !defined(DN_ASAN_POISON_GUARD_SIZE)
  #define DN_ASAN_POISON_GUARD_SIZE 128
#endif

#if DN_HAS_FEATURE(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
  #include <sanitizer/asan_interface.h>
#endif

// NOTE: Memory
#if !defined(DN_ARENA_TEMP_MEM_UAF_GUARD)
  #define DN_ARENA_TEMP_MEM_UAF_GUARD 0
#endif

#if !defined(DN_ARENA_TEMP_MEM_UAF_TRACE_ON_BY_DEFAULT)
  #define DN_ARENA_TEMP_MEM_UAF_TRACE_ON_BY_DEFAULT 0
#endif

#if !defined(DN_SCRUB_UNINIT_MEM_BYTE)
  #define DN_SCRUB_UNINIT_MEM_BYTE 0
#endif

// NOTE: Macros
#define DN_Stringify(x) #x
#define DN_TokenCombine2(x, y) x ## y
#define DN_TokenCombine(x, y) DN_TokenCombine2(x, y)

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

#define DN_ForIndexU(index, size)               DN_USize index = 0; index < size; index++
#define DN_ForIndexI(index, size)               DN_ISize index = 0; index < size; index++
#define DN_ForItSize(it, T, array, size)        struct { DN_USize index; T *data; } it = {0,          &(array)[0]};        it.index < (size);                it.index++, it.data = (array)         + it.index
#define DN_ForItSizeReverse(it, T, array, size) struct { DN_USize index; T *data; } it = {(size) - 1, &(array)[size - 1]}; it.index < (size);                it.index--, it.data = (array)         + it.index
#define DN_ForIt(it, T, array)                  struct { DN_USize index; T *data; } it = {0,          &(array)->data[0]};  it.index < (array)->size;         it.index++, it.data = ((array)->data) + it.index
#define DN_ForLinkedListIt(it, T, list)         struct { DN_USize index; T *data; } it = {0,          list};               it.data;                          it.index++, it.data = ((it).data->next)
#define DN_ForItCArray(it, T, array)            struct { DN_USize index; T *data; } it = {0,          &(array)[0]};        it.index < DN_ArrayCountU(array); it.index++, it.data = (array)         + it.index

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

struct DN_HexU64Str8
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

#define DN_CALL_SITE DN_CallSite { DN_Str8Lit(__FILE__), DN_Str8Lit(__func__), __LINE__ }

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

struct DN_StackTraceWalkResult
{
  void   *process;   // [Internal] Windows handle to the process
  DN_U64 *base_addr; // The addresses of the functions in the stack trace
  DN_U16  size;      // The number of `base_addr`'s stored from the walk
};

struct DN_StackTraceWalkResultIterator
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
  DN_MemList* mem;
  DN_U64      used_sum;
  #if DN_ARENA_TEMP_MEM_UAF_GUARD
  DN_StackTraceWalkResult trace;
  #endif
};

enum DN_AllocatorType
{
  DN_AllocatorType_Arena,
  DN_AllocatorType_Pool,
};

struct DN_Allocator
{
  DN_AllocatorType type;
  void*            context;
};

struct DN_ArenaStatsStr8x64
{
  DN_Str8x64 info;
  DN_Str8x64 hwm;
};

enum DN_ArenaReset
{
  DN_ArenaReset_No,
  DN_ArenaReset_Yes,
};

struct DN_Arena
{
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

enum DN_ByteCountType
{
  DN_ByteCountType_B,
  DN_ByteCountType_KiB,
  DN_ByteCountType_MiB,
  DN_ByteCountType_GiB,
  DN_ByteCountType_TiB,
  DN_ByteCountType_Count,
  DN_ByteCountType_Auto,
};

struct DN_ByteCountResult
{
  DN_ByteCountType type;
  DN_Str8          suffix; // "KiB", "MiB", "GiB" .. e.t.c
  DN_F64           bytes;
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
  DN_U64 err_sink_reserve;
  DN_U64 err_sink_commit;
};

struct DN_TCCore // (T)hread (C)ontext sitting in thread-local storage
{
  DN_Str8x64  name;
  DN_U64      thread_id;
  DN_CallSite call_site;
  char        lane_opaque[sizeof(DN_U64) * 4];

  DN_MemList  main_arena_mem_;
  DN_MemList  temp_a_arena_mem_;
  DN_MemList  temp_b_arena_mem_;
  DN_MemList  err_sink_arena_mem_;

  DN_Arena    main_arena_;
  DN_Arena    temp_a_arena_;
  DN_Arena    temp_b_arena_;
  DN_Arena    err_sink_arena_;

  DN_Arena*   main_arena;
  DN_Pool     main_pool;
  DN_Arena*   temp_a_arena;
  DN_Arena*   temp_b_arena;

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

#if !defined(DN_STB_SPRINTF_HEADER_ONLY)
  #define STB_SPRINTF_IMPLEMENTATION
  #define STB_SPRINTF_STATIC
#endif

DN_MSVC_WARNING_PUSH
DN_MSVC_WARNING_DISABLE(4505) // Unused function warning
DN_GCC_WARNING_PUSH
DN_GCC_WARNING_DISABLE(-Wunused-function)
#include "../External/stb_sprintf.h"
DN_GCC_WARNING_POP
DN_MSVC_WARNING_POP

#define                         DN_SPrintF(...)                                        STB_SPRINTF_DECORATE(sprintf)(__VA_ARGS__)
#define                         DN_SNPrintF(...)                                       STB_SPRINTF_DECORATE(snprintf)(__VA_ARGS__)
#define                         DN_VSPrintF(...)                                       STB_SPRINTF_DECORATE(vsprintf)(__VA_ARGS__)
#define                         DN_VSNPrintF(...)                                      STB_SPRINTF_DECORATE(vsnprintf)(__VA_ARGS__)

DN_API bool                     DN_MemStartsWith                                       (void const *lhs, DN_USize lhs_size, void const *rhs, DN_USize rhs_size);
DN_API bool                     DN_MemEq                                               (void const *lhs, DN_USize lhs_size, void const *rhs, DN_USize rhs_size);
DN_API bool                     DN_MemEqUnsafe                                         (void const *lhs, void const *rhs, DN_USize size);

DN_API DN_U64                   DN_AtomicSetValue64                                    (DN_U64 volatile *target, DN_U64 value);
DN_API DN_U32                   DN_AtomicSetValue32                                    (DN_U32 volatile *target, DN_U32 value);
DN_API DN_USize                 DN_AlignUpPowerOfTwoUSize                              (DN_USize val);
DN_API DN_U64                   DN_AlignUpPowerOfTwoU64                                (DN_U64 val);
DN_API DN_U32                   DN_AlignUpPowerOfTwoU32                                (DN_U32 val);

DN_API void                     DN_ByteSwapU64Ptr                                      (DN_U8* dest, DN_U64 src);
#define                         DN_ByteSwap64(val) ( \
                                                    (((((DN_U64)(val)) >> 56) & 0xFF) <<  0) | \
                                                    (((((DN_U64)(val)) >> 48) & 0xFF) <<  8) | \
                                                    (((((DN_U64)(val)) >> 40) & 0xFF) << 16) | \
                                                    (((((DN_U64)(val)) >> 32) & 0xFF) << 24) | \
                                                    (((((DN_U64)(val)) >> 24) & 0xFF) << 32) | \
                                                    (((((DN_U64)(val)) >> 16) & 0xFF) << 40) | \
                                                    (((((DN_U64)(val)) >> 8)  & 0xFF) << 48) | \
                                                    (((((DN_U64)(val)) >> 0)  & 0xFF) << 56)   \
                                                   )
#define                         DN_ByteSwap32(val) ( \
                                                    (((((DN_U32)(val)) >> 24) & 0xFF) << 0)  | \
                                                    (((((DN_U32)(val)) >> 16) & 0xFF) << 8)  | \
                                                    (((((DN_U32)(val)) >> 8)  & 0xFF) << 16) | \
                                                    (((((DN_U32)(val)) >> 0)  & 0xFF) << 24)   \
                                                   )
#define                         DN_ByteSwap24(val) ( \
                                                    (((((DN_U32)(val)) >> 16) & 0xFF) << 0)  | \
                                                    (((((DN_U32)(val)) >> 8)  & 0xFF) << 8)  | \
                                                    (((((DN_U32)(val)) >> 0)  & 0xFF) << 16)   \
                                                   )
#define                         DN_ByteSwap16(val) ( \
                                                    (((((DN_U16)(val)) >> 8)  & 0xFF) << 0) | \
                                                    (((((DN_U16)(val)) >> 0)  & 0xFF) << 8)   \
                                                   )
#if defined(DN_64_BIT)
  #define                       DN_ByteSwapUSize(val) DN_ByteSwap64(val)
#else
  #define                       DN_ByteSwapUSize(val) DN_ByteSwap32(val)
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
DN_API DN_ISize                 DN_SaturateCastI64ToUSize                              (DN_I64 val);
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

DN_API void                     DN_ArenaUAFCheck                                       (DN_Arena *arena);
#define                         DN_ArenaDeref(arena_view)                              (DN_ArenaUAFCheck(arena_view), (arena_view)->arena)
DN_API DN_Arena                 DN_ArenaFromMemList                                    (DN_MemList *mem);
DN_API DN_Arena                 DN_ArenaTempBeginFromMemList                           (DN_MemList *mem);
DN_API DN_Arena                 DN_ArenaTempBeginFromArena                             (DN_Arena *arena);
DN_API void                     DN_ArenaTempEnd                                        (DN_Arena *arena, DN_ArenaReset reset);
DN_API void*                    DN_ArenaAlloc                                          (DN_Arena *arena, DN_U64 size, uint8_t align, DN_ZMem z_mem);
DN_API void*                    DN_ArenaAllocContiguous                                (DN_Arena *arena, DN_U64 size, uint8_t align, DN_ZMem z_arena);
DN_API void*                    DN_ArenaCopy                                           (DN_Arena *arena, void const *data, DN_U64 size, uint8_t align);

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
#define                         DN_ErrSinkBegin(err, mode)                             DN_ErrSinkBegin_(err, mode, DN_CALL_SITE)
#define                         DN_ErrSinkBeginDefault(err)                            DN_ErrSinkBegin(err, DN_ErrSinkMode_Nil)
DN_API bool                     DN_ErrSinkHasError                                     (DN_ErrSink *err);
DN_API DN_ErrSinkMsg*           DN_ErrSinkEnd                                          (DN_Arena *arena, DN_ErrSink *err);
DN_API DN_Str8                  DN_ErrSinkEndStr8                                      (DN_Arena *arena, DN_ErrSink *err);
DN_API void                     DN_ErrSinkEndIgnore                                    (DN_ErrSink *err);
DN_API bool                     DN_ErrSinkEndLogError_                                 (DN_ErrSink *err, DN_CallSite call_site, DN_Str8 msg);
#define                         DN_ErrSinkEndLogError(err, err_msg)                    DN_ErrSinkEndLogError_(err, DN_CALL_SITE, err_msg)
DN_API bool                     DN_ErrSinkEndLogErrorFV_                               (DN_ErrSink *err, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, va_list args);
#define                         DN_ErrSinkEndLogErrorFV(err, fmt, args)                DN_ErrSinkEndLogErrorFV_(err, DN_CALL_SITE, fmt, args)
DN_API bool                     DN_ErrSinkEndLogErrorF_                                (DN_ErrSink *err, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, ...);
#define                         DN_ErrSinkEndLogErrorF(err, fmt, ...)                  DN_ErrSinkEndLogErrorF_(err, DN_CALL_SITE, fmt, ##__VA_ARGS__)
DN_API void                     DN_ErrSinkEndExitIfErrorF_                             (DN_ErrSink *err, DN_CallSite call_site, DN_U32 exit_val, DN_FMT_ATTRIB char const *fmt, ...);
#define                         DN_ErrSinkEndExitIfErrorF(err, exit_val, fmt, ...)     DN_ErrSinkEndExitIfErrorF_(err, DN_CALL_SITE, exit_val, fmt, ##__VA_ARGS__)
DN_API void                     DN_ErrSinkEndExitIfErrorFV_                            (DN_ErrSink *err, DN_CallSite call_site, DN_U32 exit_val, DN_FMT_ATTRIB char const *fmt, va_list args);
#define                         DN_ErrSinkEndExitIfErrorFV(err, exit_val, fmt, args)   DN_ErrSinkEndExitIfErrorFV_(err, DN_CALL_SITE, exit_val, fmt, args)
DN_API void                     DN_ErrSinkAppendFV_                                    (DN_ErrSink *err, DN_U32 error_code, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, va_list args);
#define                         DN_ErrSinkAppendFV(error, error_code, fmt, args)       DN_ErrSinkAppendFV_(error, error_code, DN_CALL_SITE, fmt, args)
DN_API void                     DN_ErrSinkAppendF_                                     (DN_ErrSink *err, DN_U32 error_code, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, ...);
#define                         DN_ErrSinkAppendF(error, error_code, fmt, ...)         DN_ErrSinkAppendF_(error, error_code, DN_CALL_SITE, fmt, ##__VA_ARGS__)

DN_API void                     DN_TCInit                                              (DN_TCCore *tc, DN_U64 thread_id, DN_Arena *main_arena, DN_Arena *temp_a_arena, DN_Arena *temp_b_arena, DN_Arena *err_sink_arena);
DN_API void                     DN_TCInitFromMemFuncs                                  (DN_TCCore *tc, DN_U64 thread_id, DN_TCInitArgs *args, DN_MemFuncs mem_funcs);
DN_API void                     DN_TCDeinit                                            (DN_TCCore *tc, DN_TCDeinitArenas deinit_arenas);
DN_API void                     DN_TCEquip                                             (DN_TCCore *tc);
DN_API DN_TCCore*               DN_TCGet                                               ();
DN_API DN_Arena*                DN_TCMainArena                                         ();
DN_API DN_Pool*                 DN_TCMainPool                                          ();
DN_API DN_Arena                 DN_TCTempArena                                         (DN_Arena **conflicts, DN_USize count);
DN_API DN_TCScratch             DN_TCScratchBegin                                      (DN_Arena **conflicts, DN_USize count);
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
DN_API DN_Str8                  DN_Str8AllocArena                                      (DN_USize size, DN_ZMem z_mem, DN_Arena *arena);
DN_API DN_Str8                  DN_Str8AllocPool                                       (DN_USize size, DN_Pool *pool);
DN_API DN_Str8                  DN_Str8FromCStr8                                       (char const *src);
DN_API DN_Str8                  DN_Str8FromCStr8Arena                                  (char const *src, DN_Arena *arena);
DN_API DN_Str8                  DN_Str8FromPtrArena                                    (void const *data, DN_USize size, DN_Arena *arena);
DN_API DN_Str8                  DN_Str8FromPtrPool                                     (void const *data, DN_USize size, DN_Pool *pool);
DN_API DN_Str8                  DN_Str8FromStr8Arena                                   (DN_Str8 string, DN_Arena *arena);
DN_API DN_Str8                  DN_Str8FromStr8Pool                                    (DN_Str8 string, DN_Pool *pool);
DN_API DN_Str8                  DN_Str8FromFmtVArena                                   (DN_Arena *arena, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API DN_Str8                  DN_Str8FromFmtArena                                    (DN_Arena *arena, DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_Str8                  DN_Str8FromFmtVPool                                    (DN_Pool *pool, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API DN_Str8                  DN_Str8FromFmtPool                                     (DN_Pool *pool, DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_Str8                  DN_Str8FromByteCountType                               (DN_ByteCountType type);
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
DN_API DN_Str8                  DN_Str8PadNewLines                                     (DN_Str8 string, DN_Str8 pad_string, DN_Arena *arena);
DN_API DN_Str8                  DN_Str8LineBreakStr8                                   (DN_Str8 src, DN_USize desired_width, DN_Arena *arena);
DN_API DN_Str8                  DN_Str8Table                                           (DN_Str8 const* rows, DN_USize num_rows, DN_USize num_cols, DN_Str8TableFlags flags, DN_Arena *arena);

DN_API DN_Str8                  DN_Str8SliceRender                                     (DN_Str8Slice array, DN_Str8 separator, DN_Arena *arena);
DN_API DN_Str8                  DN_Str8RenderSpaceSep                                  (DN_Str8Slice array, DN_Arena *arena);

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
DN_API DN_Str8                  DN_Str8BuilderBuild                                    (DN_Str8Builder const *builder, DN_Arena *arena);
DN_API DN_Str8                  DN_Str8BuilderBuildDelimited                           (DN_Str8Builder const *builder, DN_Str8 delimiter, DN_Arena *arena);
DN_API DN_Str8Slice             DN_Str8BuilderBuildSlice                               (DN_Str8Builder const *builder, DN_Arena *arena);

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

DN_API DN_HexU64Str8            DN_HexFromU64                                          (DN_U64 value, DN_HexFromU64Type type);
DN_API DN_USize                 DN_HexFromPtrBytes                                     (void const *bytes, DN_USize bytes_count, void *hex, DN_USize hex_count, DN_TrimLeadingZero trim_leading_z);
DN_API DN_Str8                  DN_HexFromPtrBytesArena                                (void const *bytes, DN_USize bytes_count, DN_Arena *arena, DN_TrimLeadingZero trim_leading_z);
DN_API DN_USize                 DN_HexFromStr8Bytes                                    (void const *bytes, DN_USize bytes_count, void *hex, DN_USize hex_count, DN_TrimLeadingZero trim_leading_z);
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

DN_API DN_ByteCountResult       DN_ByteCountFromType                                   (DN_U64 bytes, DN_ByteCountType type);
#define                         DN_ByteCount(bytes)                                    DN_ByteCountFromType(bytes, DN_ByteCountType_Auto)
DN_API DN_Str8x32               DN_ByteCountStr8x32FromType                            (DN_U64 bytes, DN_ByteCountType type);
#define                         DN_ByteCountStr8x32(bytes)                             DN_ByteCountStr8x32FromType(bytes, DN_ByteCountType_Auto)

#define DN_ProfilerZoneLoop(prof, name, index)                                                                            \
  DN_ProfilerZone DN_UniqueName(zone_) = DN_ProfilerBeginZone(prof, DN_Str8Lit(name), index), DN_UniqueName(dummy_) = {}; \
  DN_UniqueName(dummy_).begin_tsc == 0;                                                                                   \
  DN_ProfilerEndZone(prof, DN_UniqueName(zone_)), DN_UniqueName(dummy_).begin_tsc = 1

#define                         DN_ProfilerZoneLoopAuto(prof, name)                    DN_ProfilerZoneLoop(prof, name, __COUNTER__ + 1)
DN_API DN_Profiler              DN_ProfilerInit                                        (DN_ProfilerAnchor *anchors, DN_USize count, DN_USize anchors_per_frame, DN_ProfilerTSCNowFunc *tsc_now, DN_U64 tsc_frequency);
DN_API DN_ProfilerZone          DN_ProfilerBeginZone                                   (DN_Profiler *profiler, DN_Str8 name, DN_U16 anchor_index);
#define                         DN_ProfilerBeginZoneAuto(prof, name)                   DN_ProfilerBeginZone(prof, DN_Str8Lit(name), __COUNTER__ + 1)
DN_API void                     DN_ProfilerEndZone                                     (DN_Profiler *profiler, DN_ProfilerZone zone);
DN_API DN_USize                 DN_ProfilerFrameCount                                  (DN_Profiler const *profiler);
DN_API DN_ProfilerAnchorArray   DN_ProfilerFrameAnchorsFromIndex                       (DN_Profiler *profiler, DN_USize frame_index);
DN_API DN_ProfilerAnchorArray   DN_ProfilerFrameAnchors                                (DN_Profiler *profiler);
DN_API void                     DN_ProfilerNewFrame                                    (DN_Profiler *profiler);
DN_API void                     DN_ProfilerDump                                        (DN_Profiler *profiler);
DN_API DN_F64                   DN_ProfilerSecFromTSC                                  (DN_Profiler *profiler, DN_U64 duration_tsc);
DN_API DN_F64                   DN_ProfilerMsFromTSC                                   (DN_Profiler *profiler, DN_U64 duration_tsc);

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

DN_API DN_LogPrefixSize         DN_LogMakePrefix                                       (DN_LogStyle style, DN_LogTypeParam type, DN_CallSite call_site, DN_LogDate date, char *dest, DN_USize dest_size);
DN_API void                     DN_LogSetPrintFunc                                     (DN_LogPrintFunc *print_func, void *user_data);
DN_API void                     DN_LogPrint                                            (DN_LogTypeParam type, DN_CallSite call_site, DN_LogFlags flags, DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_LogTypeParam          DN_LogTypeParamFromType                                (DN_LogType type);
#define                         DN_LogF(type, fmt, ...)                                DN_LogPrint(DN_LogTypeParamFromType(type), DN_CALL_SITE, DN_LogFlags_Nil, fmt, ##__VA_ARGS__)

#define                         DN_LogDebugF(fmt, ...)                                 DN_LogPrint(DN_LogTypeParamFromType(DN_LogType_Debug),   DN_CALL_SITE, DN_LogFlags_Nil, fmt, ##__VA_ARGS__)
#define                         DN_LogInfoF(fmt, ...)                                  DN_LogPrint(DN_LogTypeParamFromType(DN_LogType_Info),    DN_CALL_SITE, DN_LogFlags_Nil, fmt, ##__VA_ARGS__)
#define                         DN_LogWarningF(fmt, ...)                               DN_LogPrint(DN_LogTypeParamFromType(DN_LogType_Warning), DN_CALL_SITE, DN_LogFlags_Nil, fmt, ##__VA_ARGS__)
#define                         DN_LogErrorF(fmt, ...)                                 DN_LogPrint(DN_LogTypeParamFromType(DN_LogType_Error),   DN_CALL_SITE, DN_LogFlags_Nil, fmt, ##__VA_ARGS__)

#define                         DN_LogFlagF(type, flags, fmt, ...)                     DN_LogPrint(DN_LogTypeParamFromType(type),               DN_CALL_SITE, flags, fmt, ##__VA_ARGS__)
#define                         DN_LogFlagDebugF(flags, fmt, ...)                      DN_LogPrint(DN_LogTypeParamFromType(DN_LogType_Debug),   DN_CALL_SITE, flags, fmt, ##__VA_ARGS__)
#define                         DN_LogFlagInfoF(flags, fmt, ...)                       DN_LogPrint(DN_LogTypeParamFromType(DN_LogType_Info),    DN_CALL_SITE, flags, fmt, ##__VA_ARGS__)
#define                         DN_LogFlagWarningF(flags, fmt, ...)                    DN_LogPrint(DN_LogTypeParamFromType(DN_LogType_Warning), DN_CALL_SITE, flags, fmt, ##__VA_ARGS__)
#define                         DN_LogFlagErrorF(flags, fmt, ...)                      DN_LogPrint(DN_LogTypeParamFromType(DN_LogType_Error),   DN_CALL_SITE, flags, fmt, ##__VA_ARGS__)


// NOTE: OS primitives that the OS layer can provide for the base layer but is optional.
#if defined(DN_FREESTANDING)
#define                        DN_StackTraceWalk(...)
#define                        DN_StackTraceWalkResultIterate(...)
#define                        DN_StackTraceWalkResultToStr8(...) DN_Str8Lit("N/A")
#define                        DN_StackTraceWalkStr8(...) DN_Str8Lit("N/A")
#define                        DN_StackTraceWalkStr8FromHeap(...) DN_Str8Lit("N/A")
#define                        DN_StackTraceGetFrames(...)
#define                        DN_StackTraceRawFrameToFrame(...)
#define                        DN_StackTracePrint(...)
#define                        DN_StackTraceReloadSymbols(...)
#else
DN_API DN_StackTraceWalkResult DN_StackTraceWalk                                       (DN_Arena *arena, DN_U16 limit);
DN_API bool                    DN_StackTraceWalkResultIterate                          (DN_StackTraceWalkResultIterator *it, DN_StackTraceWalkResult const *walk);
DN_API DN_Str8                 DN_StackTraceWalkResultToStr8                           (DN_Arena *arena, DN_StackTraceWalkResult const *walk, DN_U16 skip);
DN_API DN_Str8                 DN_StackTraceWalkStr8                                   (DN_Arena *arena, DN_U16 limit, DN_U16 skip);
DN_API DN_Str8                 DN_StackTraceWalkStr8FromHeap                           (DN_U16 limit, DN_U16 skip);
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

DN_API bool                    operator==                                              (DN_M2x3 const &lhs, DN_M2x3 const &rhs);
DN_API bool                    operator!=                                              (DN_M2x3 const &lhs, DN_M2x3 const &rhs);
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

DN_API bool                    operator==                                              (const DN_Rect& lhs, const DN_Rect& rhs);
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
#endif // !defined(DN_BASE_H)
