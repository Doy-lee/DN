#if !defined(DN_BASE_H)
#define DN_BASE_H

#include "../dn_base_inc.h"

// NOTE: Macros ////////////////////////////////////////////////////////////////////////////////////
#define DN_Stringify(x) #x
#define DN_TokenCombine2(x, y) x ## y
#define DN_TokenCombine(x, y) DN_TokenCombine2(x, y)

#include <stdarg.h> // va_list
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <inttypes.h> // PRIu64...

#if !defined(DN_OS_WIN32)
#include <stdlib.h> // exit()
#endif

#define DN_ForIndexU(index, size)        DN_USize index = 0; index < size; index++
#define DN_ForIndexI(index, size)        DN_ISize index = 0; index < size; index++
#define DN_ForItSize(it, T, array, size) struct { DN_USize index; T *data; } it = {0, &(array)[0]};       it.index < (size);                it.index++, it.data = (array)         + it.index
#define DN_ForIt(it, T, array)           struct { DN_USize index; T *data; } it = {0, &(array)->data[0]}; it.index < (array)->size;         it.index++, it.data = ((array)->data) + it.index
#define DN_ForItCArray(it, T, array)     struct { DN_USize index; T *data; } it = {0, &(array)[0]};       it.index < DN_ArrayCountU(array); it.index++, it.data = (array)         + it.index

#define DN_AlignUpPowerOfTwo(value, pot)   (((uintptr_t)(value) + ((uintptr_t)(pot) - 1)) & ~((uintptr_t)(pot) - 1))
#define DN_AlignDownPowerOfTwo(value, pot) ((uintptr_t)(value) & ~((uintptr_t)(pot) - 1))
#define DN_IsPowerOfTwo(value)             ((((uintptr_t)(value)) & (((uintptr_t)(value)) - 1)) == 0)
#define DN_IsPowerOfTwoAligned(value, pot) ((((uintptr_t)value) & (((uintptr_t)pot) - 1)) == 0)

// NOTE: String.h Dependencies /////////////////////////////////////////////////////////////////////
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

// NOTE: Math.h Dependencies ///////////////////////////////////////////////////////////////////////
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
#endif

// NOTE: Math //////////////////////////////////////////////////////////////////////////////////////
#define DN_PiF32 3.14159265359f

#define DN_DegreesToRadians(degrees) ((degrees) * (DN_PI / 180.0f))
#define DN_RadiansToDegrees(radians) ((radians) * (180.f * DN_PI))

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

// NOTE: Size //////////////////////////////////////////////////////////////////////////////////////
#define DN_SizeOfI(val)       DN_CAST(ptrdiff_t)sizeof(val)
#define DN_ArrayCountU(array) (sizeof(array)/(sizeof((array)[0])))
#define DN_ArrayCountI(array) (DN_ISize)DN_ArrayCountU(array)
#define DN_CharCountU(string) (sizeof(string) - 1)

// NOTE: SI Byte ///////////////////////////////////////////////////////////////////////////////////
#define DN_Bytes(val)     ((DN_U64)val)
#define DN_Kilobytes(val) ((DN_U64)1024 * DN_Bytes(val))
#define DN_Megabytes(val) ((DN_U64)1024 * DN_Kilobytes(val))
#define DN_Gigabytes(val) ((DN_U64)1024 * DN_Megabytes(val))

// NOTE: Time //////////////////////////////////////////////////////////////////////////////////////
#define DN_SecondsToMs(val) ((val) * 1000)
#define DN_MinutesToSec(val)  ((val) * 60ULL)
#define DN_HoursToSec(val)    (DN_MinutesToSec(val)  * 60ULL)
#define DN_DaysToSec(val)     (DN_HoursToSec(val)    * 24ULL)
#define DN_WeeksToSec(val)    (DN_DaysToSec(val)     *  7ULL)
#define DN_YearsToSec(val)    (DN_WeeksToSec(val)    * 52ULL)

// NOTE: Debug Break ///////////////////////////////////////////////////////////////////////////////
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

// NOTE: Byte swaps ////////////////////////////////////////////////////////////////////////////////
#define DN_ByteSwap64(u64) (((((u64) >> 56) & 0xFF) <<  0) | \
                              ((((u64) >> 48) & 0xFF) <<  8) | \
                              ((((u64) >> 40) & 0xFF) << 16) | \
                              ((((u64) >> 32) & 0xFF) << 24) | \
                              ((((u64) >> 24) & 0xFF) << 32) | \
                              ((((u64) >> 16) & 0xFF) << 40) | \
                              ((((u64) >> 8)  & 0xFF) << 48) | \
                              ((((u64) >> 0)  & 0xFF) << 56))

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

#define DN_UniqueName(prefix) DN_TokenCombine(prefix, __LINE__)
#define DN_DEFER              const auto DN_UniqueName(defer_lambda_) = DN_DeferHelper() + [&]()
#endif // defined(__cplusplus)

#define DN_DeferLoop(begin, end)            \
  bool DN_UniqueName(once) = (begin, true); \
  DN_UniqueName(once);                      \
  end, DN_UniqueName(once) = false

// NOTE: Types /////////////////////////////////////////////////////////////////////////////////////
typedef intptr_t  DN_ISize;
typedef uintptr_t DN_USize;

typedef int8_t  DN_I8;
typedef int16_t DN_I16;
typedef int32_t DN_I32;
typedef int64_t DN_I64;

typedef uint8_t  DN_U8;
typedef uint16_t DN_U16;
typedef uint32_t DN_U32;
typedef uint64_t DN_U64;

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

enum DN_ZeroMem
{
  DN_ZeroMem_No,  // Memory can be handed out without zero-ing it out
  DN_ZeroMem_Yes, // Memory should be zero-ed out before giving to the callee
};

struct DN_Str8
{
  char    *data; // The bytes of the string
  DN_USize size; // The number of bytes in the string

  char const *begin() const { return data; }
  char const *end()   const { return data + size; }
  char       *begin()       { return data; }
  char       *end()         { return data + size; }
};

struct DN_Str16 // A pointer and length style string that holds slices to UTF16 bytes.
{
  wchar_t *data; // The UTF16 bytes of the string
  DN_USize size; // The number of characters in the string

  #if defined(__cplusplus)
  wchar_t const *begin() const { return data; } // Const begin iterator for range-for loops
  wchar_t const *end() const { return data + size; } // Const end iterator for range-for loops
  wchar_t *begin() { return data; } // Begin iterator for range-for loops
  wchar_t *end() { return data + size; } // End iterator for range-for loops
  #endif
};

template <typename T>
struct DN_Slice // A pointer and length container of data
{
  T       *data;
  DN_USize size;

  T *begin() { return data; }
  T *end() { return data + size; }
  T const *begin() const { return data; }
  T const *end() const { return data + size; }
};

// NOTE: DN_CallSite ///////////////////////////////////////////////////////////////////////////////
struct DN_CallSite
{
  DN_Str8 file;
  DN_Str8 function;
  DN_U32  line;
};
#define DN_CALL_SITE DN_CallSite { DN_STR8(__FILE__), DN_STR8(__func__), __LINE__ }

// NOTE: Intrinsics ////////////////////////////////////////////////////////////////////////////////
// NOTE: DN_AtomicAdd/Exchange return the previous value store in the target
#if defined(DN_COMPILER_MSVC) || defined(DN_COMPILER_CLANG_CL)
  #include <intrin.h>
  #define DN_AtomicCompareExchange64(dest, desired_val, prev_val) _InterlockedCompareExchange64((__int64 volatile *)dest, desired_val, prev_val)
  #define DN_AtomicCompareExchange32(dest, desired_val, prev_val) _InterlockedCompareExchange((long volatile *)dest, desired_val, prev_val)

  #define DN_AtomicLoadU64(target)                                *(target)
  #define DN_AtomicLoadU32(target)                                *(target)
  #define DN_AtomicAddU32(target, value)                          _InterlockedExchangeAdd((long volatile *)target, value)
  #define DN_AtomicAddU64(target, value)                          _InterlockedExchangeAdd64((__int64 volatile *)target, value)
  #define DN_AtomicSubU32(target, value)                          DN_AtomicAddU32(DN_CAST(long volatile *) target, (long)-value)
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
    #define DN_CPUTSC() __rdtsc()
  #else
    #define DN_CPUTSC() __builtin_readcyclecounter()
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

#if defined(__aarch64__) || defined(_M_X64) || defined(__x86_64__) || defined(__x86_64)
  #define DN_CountLeadingZerosUSize(value) DN_CountLeadingZerosU64(value)
#else
  #define DN_CountLeadingZerosUSize(value) DN_CountLeadingZerosU32(value)
#endif

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

DN_API DN_U64          DN_AtomicSetValue64       (DN_U64 volatile *target, DN_U64 value);
DN_API DN_U32          DN_AtomicSetValue32       (DN_U32 volatile *target, DN_U32 value);

DN_API DN_CPUIDResult  DN_CPUID                  (DN_CPUIDArgs args);
DN_API DN_USize        DN_CPUHasFeatureArray     (DN_CPUReport const *report, DN_CPUFeatureQuery *features, DN_USize features_size);
DN_API bool            DN_CPUHasFeature          (DN_CPUReport const *report, DN_CPUFeature feature);
DN_API bool            DN_CPUHasAllFeatures      (DN_CPUReport const *report, DN_CPUFeature const *features, DN_USize features_size);
DN_API void            DN_CPUSetFeature          (DN_CPUReport *report, DN_CPUFeature feature);
DN_API DN_CPUReport    DN_CPUGetReport           ();

DN_API void            DN_TicketMutex_Begin       (DN_TicketMutex *mutex);
DN_API void            DN_TicketMutex_End         (DN_TicketMutex *mutex);
DN_API DN_UInt         DN_TicketMutex_MakeTicket  (DN_TicketMutex *mutex);
DN_API void            DN_TicketMutex_BeginTicket (DN_TicketMutex const *mutex, DN_UInt ticket);
DN_API bool            DN_TicketMutex_CanLock     (DN_TicketMutex const *mutex, DN_UInt ticket);

DN_API void            DN_BitUnsetInplace         (DN_USize *flags, DN_USize bitfield);
DN_API void            DN_BitSetInplace           (DN_USize *flags, DN_USize bitfield);
DN_API bool            DN_BitIsSet                (DN_USize bits, DN_USize bits_to_set);
DN_API bool            DN_BitIsNotSet             (DN_USize bits, DN_USize bits_to_check);
#define                DN_BitClearNextLSB(value)  (value) & ((value) - 1)

DN_API DN_I64          DN_SafeAddI64              (DN_I64 a,  DN_I64 b);
DN_API DN_I64          DN_SafeMulI64              (DN_I64 a,  DN_I64 b);

DN_API DN_U64          DN_SafeAddU64              (DN_U64 a, DN_U64 b);
DN_API DN_U64          DN_SafeMulU64              (DN_U64 a, DN_U64 b);

DN_API DN_U64          DN_SafeSubU64              (DN_U64 a, DN_U64 b);
DN_API DN_U32          DN_SafeSubU32              (DN_U32 a, DN_U32 b);

DN_API int             DN_SaturateCastUSizeToInt  (DN_USize val);
DN_API DN_I8           DN_SaturateCastUSizeToI8   (DN_USize val);
DN_API DN_I16          DN_SaturateCastUSizeToI16  (DN_USize val);
DN_API DN_I32          DN_SaturateCastUSizeToI32  (DN_USize val);
DN_API DN_I64          DN_SaturateCastUSizeToI64  (DN_USize val);

DN_API int             DN_SaturateCastU64ToInt    (DN_U64 val);
DN_API DN_I8           DN_SaturateCastU8ToI8      (DN_U64 val);
DN_API DN_I16          DN_SaturateCastU16ToI16    (DN_U64 val);
DN_API DN_I32          DN_SaturateCastU32ToI32    (DN_U64 val);
DN_API DN_I64          DN_SaturateCastU64ToI64    (DN_U64 val);
DN_API DN_UInt         DN_SaturateCastU64ToUInt   (DN_U64 val);
DN_API DN_U8           DN_SaturateCastU64ToU8     (DN_U64 val);
DN_API DN_U16          DN_SaturateCastU64ToU16    (DN_U64 val);
DN_API DN_U32          DN_SaturateCastU64ToU32    (DN_U64 val);

DN_API DN_U8           DN_SaturateCastUSizeToU8   (DN_USize val);
DN_API DN_U16          DN_SaturateCastUSizeToU16  (DN_USize val);
DN_API DN_U32          DN_SaturateCastUSizeToU32  (DN_USize val);
DN_API DN_U64          DN_SaturateCastUSizeToU64  (DN_USize val);

DN_API int             DN_SaturateCastISizeToInt  (DN_ISize val);
DN_API DN_I8           DN_SaturateCastISizeToI8   (DN_ISize val);
DN_API DN_I16          DN_SaturateCastISizeToI16  (DN_ISize val);
DN_API DN_I32          DN_SaturateCastISizeToI32  (DN_ISize val);
DN_API DN_I64          DN_SaturateCastISizeToI64  (DN_ISize val);

DN_API DN_UInt         DN_SaturateCastISizeToUInt (DN_ISize val);
DN_API DN_U8           DN_SaturateCastISizeToU8   (DN_ISize val);
DN_API DN_U16          DN_SaturateCastISizeToU16  (DN_ISize val);
DN_API DN_U32          DN_SaturateCastISizeToU32  (DN_ISize val);
DN_API DN_U64          DN_SaturateCastISizeToU64  (DN_ISize val);

DN_API DN_ISize        DN_SaturateCastI64ToISize  (DN_I64 val);
DN_API DN_I8           DN_SaturateCastI64ToI8     (DN_I64 val);
DN_API DN_I16          DN_SaturateCastI64ToI16    (DN_I64 val);
DN_API DN_I32          DN_SaturateCastI64ToI32    (DN_I64 val);

DN_API DN_UInt         DN_SaturateCastI64ToUInt   (DN_I64 val);
DN_API DN_ISize        DN_SaturateCastI64ToUSize  (DN_I64 val);
DN_API DN_U8           DN_SaturateCastI64ToU8     (DN_I64 val);
DN_API DN_U16          DN_SaturateCastI64ToU16    (DN_I64 val);
DN_API DN_U32          DN_SaturateCastI64ToU32    (DN_I64 val);
DN_API DN_U64          DN_SaturateCastI64ToU64    (DN_I64 val);

DN_API DN_I8           DN_SaturateCastIntToI8     (int val);
DN_API DN_I16          DN_SaturateCastIntToI16    (int val);
DN_API DN_U8           DN_SaturateCastIntToU8     (int val);
DN_API DN_U16          DN_SaturateCastIntToU16    (int val);
DN_API DN_U32          DN_SaturateCastIntToU32    (int val);
DN_API DN_U64          DN_SaturateCastIntToU64    (int val);

DN_API void            DN_ASanPoisonMemoryRegion  (void const volatile *ptr, DN_USize size);
DN_API void            DN_ASanUnpoisonMemoryRegion(void const volatile *ptr, DN_USize size);
#endif // !defined(DN_BASE_H)
