#if !defined(DN_BASE_H)
#define DN_BASE_H

#include "../dn_base_inc.h"

// NOTE: Macros
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
#define DN_ForLinkedListIt(it, T, list)  struct { DN_USize index; T *data; } it = {0, list};              it.data;                          it.index++, it.data = ((it).data->next)
#define DN_ForItCArray(it, T, array)     struct { DN_USize index; T *data; } it = {0, &(array)[0]};       it.index < DN_ArrayCountU(array); it.index++, it.data = (array)         + it.index

#define DN_AlignUpPowerOfTwo(value, pot)   (((uintptr_t)(value) + ((uintptr_t)(pot) - 1)) & ~((uintptr_t)(pot) - 1))
#define DN_AlignDownPowerOfTwo(value, pot) ((uintptr_t)(value) & ~((uintptr_t)(pot) - 1))
#define DN_IsPowerOfTwo(value)             ((((uintptr_t)(value)) & (((uintptr_t)(value)) - 1)) == 0)
#define DN_IsPowerOfTwoAligned(value, pot) ((((uintptr_t)value) & (((uintptr_t)pot) - 1)) == 0)

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
#endif

// NOTE: Math
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

// NOTE: Byte swaps
#define DN_ByteSwap64(u64) (((((u64) >> 56) & 0xFF) <<  0) | \
                              ((((u64) >> 48) & 0xFF) <<  8) | \
                              ((((u64) >> 40) & 0xFF) << 16) | \
                              ((((u64) >> 32) & 0xFF) << 24) | \
                              ((((u64) >> 24) & 0xFF) << 32) | \
                              ((((u64) >> 16) & 0xFF) << 40) | \
                              ((((u64) >> 8)  & 0xFF) << 48) | \
                              ((((u64) >> 0)  & 0xFF) << 56))

// NOTE: Types
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

struct DN_Str8x32  { char data[32];  DN_USize size; };
struct DN_Str8x64  { char data[64];  DN_USize size; };
struct DN_Str8x128 { char data[128]; DN_USize size; };
struct DN_Str8x256 { char data[256]; DN_USize size; };

struct DN_Hex32    { char data[32];  };
struct DN_Hex64    { char data[64];  };
struct DN_Hex128   { char data[128]; };

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

struct DN_Str16 // A pointer and length style string that holds slices to UTF16 bytes.
{
  wchar_t *data; // The UTF16 bytes of the string
  DN_USize size; // The number of characters in the string
};

struct DN_U8x16 { DN_U8 data[16]; };
struct DN_U8x32 { DN_U8 data[32]; };
struct DN_U8x64 { DN_U8 data[64]; };

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

struct DN_CallSite
{
  DN_Str8 file;
  DN_Str8 function;
  DN_U32  line;
};

#define DN_CALL_SITE DN_CallSite { DN_Str8Lit(__FILE__), DN_Str8Lit(__func__), __LINE__ }

// NOTE: Defer Macro
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

struct DN_U64FromResult
{
  bool   success;
  DN_U64 value;
};

struct DN_I64FromResult
{
  bool   success;
  DN_I64 value;
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

enum DN_Allocator
{
  DN_Allocator_Arena,
  DN_Allocator_Pool,
};

struct DN_ArenaBlock
{
  DN_ArenaBlock *prev;
  DN_U64         used;
  DN_U64         commit;
  DN_U64         reserve;
  DN_U64         reserve_sum;
};

typedef DN_U32 DN_ArenaFlags;
enum DN_ArenaFlags_
{
  DN_ArenaFlags_Nil          = 0,
  DN_ArenaFlags_NoGrow       = 1 << 0,
  DN_ArenaFlags_NoPoison     = 1 << 1,
  DN_ArenaFlags_NoAllocTrack = 1 << 2,
  DN_ArenaFlags_AllocCanLeak = 1 << 3,

  // NOTE: Internal flags. Do not use
  DN_ArenaFlags_UserBuffer   = 1 << 4,
  DN_ArenaFlags_MemFuncs     = 1 << 5,
};

struct DN_ArenaInfo
{
  DN_U64 used;
  DN_U64 commit;
  DN_U64 reserve;
  DN_U64 blocks;
};

struct DN_ArenaStats
{
  DN_ArenaInfo info;
  DN_ArenaInfo hwm;
};

enum DN_ArenaMemFuncType
{
  DN_ArenaMemFuncType_Nil,
  DN_ArenaMemFuncType_Basic,
  DN_ArenaMemFuncType_VMem,
};

typedef void *(DN_ArenaMemBasicAllocFunc)(DN_USize size);
typedef void  (DN_ArenaMemBasicDeallocFunc)(void *ptr);
typedef void *(DN_ArenaMemVMemReserveFunc)(DN_USize size, DN_MemCommit commit, DN_MemPage page_flags);
typedef bool  (DN_ArenaMemVMemCommitFunc)(void *ptr, DN_USize size, DN_U32 page_flags);
typedef void  (DN_ArenaMemVMemReleaseFunc)(void *ptr, DN_USize size);
struct DN_ArenaMemFuncs
{
  DN_ArenaMemFuncType          type;
  DN_ArenaMemBasicAllocFunc   *basic_alloc;
  DN_ArenaMemBasicDeallocFunc *basic_dealloc;

  DN_U32                       vmem_page_size;
  DN_ArenaMemVMemReserveFunc  *vmem_reserve;
  DN_ArenaMemVMemCommitFunc   *vmem_commit;
  DN_ArenaMemVMemReleaseFunc  *vmem_release;
};

struct DN_Arena
{
  DN_ArenaMemFuncs mem_funcs;
  DN_ArenaBlock   *curr;
  DN_ArenaStats    stats;
  DN_ArenaFlags    flags;
  DN_Str8          label;
  DN_Arena        *prev, *next;
};

struct DN_ArenaTempMem
{
  DN_Arena *arena;
  DN_U64    used_sum;
};

struct DN_ArenaTempMemScope
{
  DN_ArenaTempMemScope(DN_Arena *arena);
  ~DN_ArenaTempMemScope();
  DN_ArenaTempMem mem;
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
  DN_Str8 lhs;
  DN_Str8 rhs;
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

enum DN_Str8FindFlag
{
  DN_Str8FindFlag_Digit      = 1 << 0, // 0-9
  DN_Str8FindFlag_Whitespace = 1 << 1, // '\r', '\t', '\n', ' '
  DN_Str8FindFlag_Alphabet   = 1 << 2, // A-Z, a-z
  DN_Str8FindFlag_Plus       = 1 << 3, // +
  DN_Str8FindFlag_Minus      = 1 << 4, // -
  DN_Str8FindFlag_AlphaNum   = DN_Str8FindFlag_Alphabet | DN_Str8FindFlag_Digit,
};

enum DN_Str8SplitIncludeEmptyStrings
{
  DN_Str8SplitIncludeEmptyStrings_No,
  DN_Str8SplitIncludeEmptyStrings_Yes,
};

struct DN_Str8TruncateResult
{
  bool    truncated;
  DN_Str8 str8;
};

struct DN_Str8SplitResult
{
  DN_Str8 *data;
  DN_USize count;
};

struct DN_Str8Link
{
  DN_Str8      string; // The string
  DN_Str8Link *next;   // The next string in the linked list
  DN_Str8Link *prev;   // The prev string in the linked list
};

struct DN_Str8Builder
{
  DN_Arena    *arena;       // Allocator to use to back the string list
  DN_Str8Link *head;        // First string in the linked list of strings
  DN_Str8Link *tail;        // Last string in the linked list of strings
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

struct DN_FmtAppendResult
{
  DN_USize size_req;
  DN_Str8  str8;
  bool     truncated;
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

#define                         DN_SPrintF(...)             STB_SPRINTF_DECORATE(sprintf)(__VA_ARGS__)
#define                         DN_SNPrintF(...)            STB_SPRINTF_DECORATE(snprintf)(__VA_ARGS__)
#define                         DN_VSPrintF(...)            STB_SPRINTF_DECORATE(vsprintf)(__VA_ARGS__)
#define                         DN_VSNPrintF(...)           STB_SPRINTF_DECORATE(vsnprintf)(__VA_ARGS__)

DN_API bool                     DN_MemEq                    (void const *lhs, DN_USize lhs_size, void const *rhs, DN_USize rhs_size);

DN_API DN_U64                   DN_AtomicSetValue64         (DN_U64 volatile *target, DN_U64 value);
DN_API DN_U32                   DN_AtomicSetValue32         (DN_U32 volatile *target, DN_U32 value);

DN_API DN_CPUIDResult           DN_CPUID                    (DN_CPUIDArgs args);
DN_API DN_USize                 DN_CPUHasFeatureArray       (DN_CPUReport const *report, DN_CPUFeatureQuery *features, DN_USize features_size);
DN_API bool                     DN_CPUHasFeature            (DN_CPUReport const *report, DN_CPUFeature feature);
DN_API bool                     DN_CPUHasAllFeatures        (DN_CPUReport const *report, DN_CPUFeature const *features, DN_USize features_size);
DN_API void                     DN_CPUSetFeature            (DN_CPUReport *report, DN_CPUFeature feature);
DN_API DN_CPUReport             DN_CPUGetReport             ();

DN_API void                     DN_TicketMutex_Begin        (DN_TicketMutex *mutex);
DN_API void                     DN_TicketMutex_End          (DN_TicketMutex *mutex);
DN_API DN_UInt                  DN_TicketMutex_MakeTicket   (DN_TicketMutex *mutex);
DN_API void                     DN_TicketMutex_BeginTicket  (DN_TicketMutex const *mutex, DN_UInt ticket);
DN_API bool                     DN_TicketMutex_CanLock      (DN_TicketMutex const *mutex, DN_UInt ticket);

DN_API void                     DN_BitUnsetInplace          (DN_USize *flags, DN_USize bitfield);
DN_API void                     DN_BitSetInplace            (DN_USize *flags, DN_USize bitfield);
DN_API bool                     DN_BitIsSet                 (DN_USize bits, DN_USize bits_to_set);
DN_API bool                     DN_BitIsNotSet              (DN_USize bits, DN_USize bits_to_check);
#define                         DN_BitClearNextLSB(value)   (value) & ((value) - 1)

DN_API DN_I64                   DN_SafeAddI64               (DN_I64 a,  DN_I64 b);
DN_API DN_I64                   DN_SafeMulI64               (DN_I64 a,  DN_I64 b);

DN_API DN_U64                   DN_SafeAddU64               (DN_U64 a, DN_U64 b);
DN_API DN_U64                   DN_SafeMulU64               (DN_U64 a, DN_U64 b);

DN_API DN_U64                   DN_SafeSubU64               (DN_U64 a, DN_U64 b);
DN_API DN_U32                   DN_SafeSubU32               (DN_U32 a, DN_U32 b);

DN_API int                      DN_SaturateCastUSizeToInt   (DN_USize val);
DN_API DN_I8                    DN_SaturateCastUSizeToI8    (DN_USize val);
DN_API DN_I16                   DN_SaturateCastUSizeToI16   (DN_USize val);
DN_API DN_I32                   DN_SaturateCastUSizeToI32   (DN_USize val);
DN_API DN_I64                   DN_SaturateCastUSizeToI64   (DN_USize val);

DN_API int                      DN_SaturateCastU64ToInt     (DN_U64 val);
DN_API DN_I8                    DN_SaturateCastU8ToI8       (DN_U64 val);
DN_API DN_I16                   DN_SaturateCastU16ToI16     (DN_U64 val);
DN_API DN_I32                   DN_SaturateCastU32ToI32     (DN_U64 val);
DN_API DN_I64                   DN_SaturateCastU64ToI64     (DN_U64 val);
DN_API DN_UInt                  DN_SaturateCastU64ToUInt    (DN_U64 val);
DN_API DN_U8                    DN_SaturateCastU64ToU8      (DN_U64 val);
DN_API DN_U16                   DN_SaturateCastU64ToU16     (DN_U64 val);
DN_API DN_U32                   DN_SaturateCastU64ToU32     (DN_U64 val);

DN_API DN_U8                    DN_SaturateCastUSizeToU8    (DN_USize val);
DN_API DN_U16                   DN_SaturateCastUSizeToU16   (DN_USize val);
DN_API DN_U32                   DN_SaturateCastUSizeToU32   (DN_USize val);
DN_API DN_U64                   DN_SaturateCastUSizeToU64   (DN_USize val);

DN_API int                      DN_SaturateCastISizeToInt   (DN_ISize val);
DN_API DN_I8                    DN_SaturateCastISizeToI8    (DN_ISize val);
DN_API DN_I16                   DN_SaturateCastISizeToI16   (DN_ISize val);
DN_API DN_I32                   DN_SaturateCastISizeToI32   (DN_ISize val);
DN_API DN_I64                   DN_SaturateCastISizeToI64   (DN_ISize val);

DN_API DN_UInt                  DN_SaturateCastISizeToUInt  (DN_ISize val);
DN_API DN_U8                    DN_SaturateCastISizeToU8    (DN_ISize val);
DN_API DN_U16                   DN_SaturateCastISizeToU16   (DN_ISize val);
DN_API DN_U32                   DN_SaturateCastISizeToU32   (DN_ISize val);
DN_API DN_U64                   DN_SaturateCastISizeToU64   (DN_ISize val);

DN_API DN_ISize                 DN_SaturateCastI64ToISize   (DN_I64 val);
DN_API DN_I8                    DN_SaturateCastI64ToI8      (DN_I64 val);
DN_API DN_I16                   DN_SaturateCastI64ToI16     (DN_I64 val);
DN_API DN_I32                   DN_SaturateCastI64ToI32     (DN_I64 val);

DN_API DN_UInt                  DN_SaturateCastI64ToUInt    (DN_I64 val);
DN_API DN_ISize                 DN_SaturateCastI64ToUSize   (DN_I64 val);
DN_API DN_U8                    DN_SaturateCastI64ToU8      (DN_I64 val);
DN_API DN_U16                   DN_SaturateCastI64ToU16     (DN_I64 val);
DN_API DN_U32                   DN_SaturateCastI64ToU32     (DN_I64 val);
DN_API DN_U64                   DN_SaturateCastI64ToU64     (DN_I64 val);

DN_API DN_I8                    DN_SaturateCastIntToI8      (int val);
DN_API DN_I16                   DN_SaturateCastIntToI16     (int val);
DN_API DN_U8                    DN_SaturateCastIntToU8      (int val);
DN_API DN_U16                   DN_SaturateCastIntToU16     (int val);
DN_API DN_U32                   DN_SaturateCastIntToU32     (int val);
DN_API DN_U64                   DN_SaturateCastIntToU64     (int val);

DN_API void                     DN_ASanPoisonMemoryRegion   (void const volatile *ptr, DN_USize size);
DN_API void                     DN_ASanUnpoisonMemoryRegion (void const volatile *ptr, DN_USize size);

DN_API DN_F32                   DN_EpsilonClampF32          (DN_F32 value, DN_F32 target, DN_F32 epsilon);

DN_API DN_Arena                 DN_ArenaFromBuffer          (void *buffer, DN_USize size, DN_ArenaFlags flags);
DN_API DN_Arena                 DN_ArenaFromMemFuncs        (DN_U64 reserve, DN_U64 commit, DN_ArenaFlags flags, DN_ArenaMemFuncs mem_funcs);
DN_API void                     DN_ArenaDeinit              (DN_Arena *arena);
DN_API bool                     DN_ArenaCommit              (DN_Arena *arena, DN_U64 size);
DN_API bool                     DN_ArenaCommitTo            (DN_Arena *arena, DN_U64 pos);
DN_API bool                     DN_ArenaGrow                (DN_Arena *arena, DN_U64 reserve, DN_U64 commit);
DN_API void *                   DN_ArenaAlloc               (DN_Arena *arena, DN_U64 size, uint8_t align, DN_ZMem zmem);
DN_API void *                   DN_ArenaAllocContiguous     (DN_Arena *arena, DN_U64 size, uint8_t align, DN_ZMem zmem);
DN_API void *                   DN_ArenaCopy                (DN_Arena *arena, void const *data, DN_U64 size, uint8_t align);
DN_API void                     DN_ArenaPopTo               (DN_Arena *arena, DN_U64 init_used);
DN_API void                     DN_ArenaPop                 (DN_Arena *arena, DN_U64 amount);
DN_API DN_U64                   DN_ArenaPos                 (DN_Arena const *arena);
DN_API void                     DN_ArenaClear               (DN_Arena *arena);
DN_API bool                     DN_ArenaOwnsPtr             (DN_Arena const *arena, void *ptr);
DN_API DN_ArenaStats            DN_ArenaSumStatsArray       (DN_ArenaStats const *array, DN_USize size);
DN_API DN_ArenaStats            DN_ArenaSumStats            (DN_ArenaStats lhs, DN_ArenaStats rhs);
DN_API DN_ArenaStats            DN_ArenaSumArenaArrayToStats(DN_Arena const *array, DN_USize size);
DN_API DN_ArenaTempMem          DN_ArenaTempMemBegin        (DN_Arena *arena);
DN_API void                     DN_ArenaTempMemEnd          (DN_ArenaTempMem mem);
#define                         DN_ArenaNew(arena, T, zmem)                (T *)DN_ArenaAlloc(arena, sizeof(T), alignof(T), zmem)
#define                         DN_ArenaNewZ(arena, T)                     (T *)DN_ArenaAlloc(arena, sizeof(T), alignof(T), DN_ZMem_Yes)

#define                         DN_ArenaNewContiguous(arena, T, zmem)      (T *)DN_ArenaAllocContiguous(arena, sizeof(T), alignof(T), zmem)
#define                         DN_ArenaNewContiguousZ(arena, T)           (T *)DN_ArenaAllocContiguous(arena, sizeof(T), alignof(T), DN_ZMem_Yes)

#define                         DN_ArenaNewArray(arena, T, count, zmem)    (T *)DN_ArenaAlloc(arena, sizeof(T) * (count), alignof(T), zmem)
#define                         DN_ArenaNewArrayZ(arena, T, count)         (T *)DN_ArenaAlloc(arena, sizeof(T) * (count), alignof(T), DN_ZMem_Yes)

#define                         DN_ArenaNewCopy(arena, T, src)             (T *)DN_ArenaCopy (arena, (src),                sizeof(T),            alignof(T))
#define                         DN_ArenaNewArrayCopy(arena, T, src, count) (T *)DN_ArenaCopy (arena, (src),                sizeof(T)  * (count), alignof(T))

DN_API DN_Pool                  DN_PoolFromArena            (DN_Arena *arena, DN_U8 align);
DN_API bool                     DN_PoolIsValid              (DN_Pool const *pool);
DN_API void *                   DN_PoolAlloc                (DN_Pool *pool, DN_USize size);
DN_API void                     DN_PoolDealloc              (DN_Pool *pool, void *ptr);
DN_API void *                   DN_PoolCopy                 (DN_Pool *pool, void const *data, DN_U64 size, uint8_t align);
#define                         DN_PoolNew(pool, T)                         (T *)DN_PoolAlloc(pool, sizeof(T))
#define                         DN_PoolNewArray(pool, T, count)             (T *)DN_PoolAlloc(pool, count * sizeof(T))
#define                         DN_PoolNewCopy(pool, T, src)                (T *)DN_PoolCopy (pool, (src), sizeof(T),            alignof(T))
#define                         DN_PoolNewArrayCopy(pool, T, src, count)    (T *)DN_PoolCopy (pool, (src), sizeof(T)  * (count), alignof(T))

DN_API bool                     DN_CharIsAlphabet           (char ch);
DN_API bool                     DN_CharIsDigit              (char ch);
DN_API bool                     DN_CharIsAlphaNum           (char ch);
DN_API bool                     DN_CharIsWhitespace         (char ch);
DN_API bool                     DN_CharIsHex                (char ch);
DN_API char                     DN_CharToLower              (char ch);
DN_API char                     DN_CharToUpper              (char ch);

DN_API DN_U64FromResult         DN_U64FromStr8              (DN_Str8 string, char separator);
DN_API DN_U64FromResult         DN_U64FromPtr               (void const *data, DN_USize size, char separator);
DN_API DN_U64                   DN_U64FromPtrUnsafe         (void const *data, DN_USize size, char separator);
DN_API DN_U64FromResult         DN_U64FromHexPtr            (void const *hex, DN_USize hex_count);
DN_API DN_U64                   DN_U64FromHexPtrUnsafe      (void const *hex, DN_USize hex_count);
DN_API DN_U64FromResult         DN_U64FromHexStr8           (DN_Str8 hex);
DN_API DN_U64                   DN_U64FromHexStr8Unsafe     (DN_Str8 hex);
DN_API DN_I64FromResult         DN_I64FromStr8              (DN_Str8 string, char separator);
DN_API DN_I64FromResult         DN_I64FromPtr               (void const *data, DN_USize size, char separator);
DN_API DN_I64                   DN_I64FromPtrUnsafe         (void const *data, DN_USize size, char separator);

DN_API DN_USize                 DN_FmtVSize                 (DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API DN_USize                 DN_FmtSize                  (DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_FmtAppendResult       DN_FmtVAppend               (char *buf, DN_USize *buf_size, DN_USize buf_max, char const *fmt, va_list args);
DN_API DN_FmtAppendResult       DN_FmtAppend                (char *buf, DN_USize *buf_size, DN_USize buf_max, char const *fmt, ...);
DN_API DN_FmtAppendResult       DN_FmtAppendTruncate        (char *buf, DN_USize *buf_size, DN_USize buf_max, DN_Str8 truncator, char const *fmt, ...);
DN_API DN_USize                 DN_CStr8Size                (char const *src);
DN_API DN_USize                 DN_CStr16Size               (wchar_t const *src);

#define                         DN_Str16Lit(string)         DN_Str16{(wchar_t *)(string), sizeof(string)/sizeof(string[0]) - 1}
#define                         DN_Str8Lit(c_str)           DN_Literal(DN_Str8){(char *)(c_str), sizeof(c_str) - 1}
#define                         DN_Str8PrintFmt(string)     (int)((string).size), (string).data
#define                         DN_Str8FromPtr(data, size)  DN_Literal(DN_Str8){(char *)(data), (DN_USize)(size)}
#define                         DN_Str8FromStruct(ptr)      DN_Str8FromPtr((ptr)->data, (ptr)->size)
DN_API DN_Str8                  DN_Str8FromCStr8            (char const *src);
DN_API DN_Str8                  DN_Str8FromArena            (DN_Arena *arena, DN_USize size, DN_ZMem z_mem);
DN_API DN_Str8                  DN_Str8FromPool             (DN_Pool *pool, DN_USize size);
DN_API DN_Str8                  DN_Str8FromStr8Arena        (DN_Arena *pool, DN_Str8 string);
DN_API DN_Str8                  DN_Str8FromStr8Pool         (DN_Pool *pool, DN_Str8 string);
DN_API DN_Str8                  DN_Str8FromFmtArena         (DN_Arena *arena, DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_Str8                  DN_Str8FromFmtVArena        (DN_Arena *arena, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API DN_Str8                  DN_Str8FromFmtPool          (DN_Pool *pool, DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_Str8                  DN_Str8FromByteCountType    (DN_ByteCountType type);
DN_API DN_Str8x32               DN_Str8x32FromFmt           (DN_FMT_ATTRIB char const *fmt, ...);
DN_API bool                     DN_Str8IsAll                (DN_Str8 string, DN_Str8IsAllType is_all);
DN_API char *                   DN_Str8End                  (DN_Str8 string);
DN_API DN_Str8                  DN_Str8Slice                (DN_Str8 string, DN_USize offset, DN_USize size);
DN_API DN_Str8                  DN_Str8Advance              (DN_Str8 string, DN_USize amount);
DN_API DN_Str8                  DN_Str8NextLine             (DN_Str8 string);
DN_API DN_Str8BSplitResult      DN_Str8BSplitArray          (DN_Str8 string, DN_Str8 const *find, DN_USize find_size);
DN_API DN_Str8BSplitResult      DN_Str8BSplit               (DN_Str8 string, DN_Str8 find);
DN_API DN_Str8BSplitResult      DN_Str8BSplitLastArray      (DN_Str8 string, DN_Str8 const *find, DN_USize find_size);
DN_API DN_Str8BSplitResult      DN_Str8BSplitLast           (DN_Str8 string, DN_Str8 find);
DN_API DN_USize                 DN_Str8Split                (DN_Str8 string, DN_Str8 delimiter, DN_Str8 *splits, DN_USize splits_count, DN_Str8SplitIncludeEmptyStrings mode);
DN_API DN_Str8SplitResult       DN_Str8SplitArena           (DN_Arena *arena, DN_Str8 string, DN_Str8 delimiter, DN_Str8SplitIncludeEmptyStrings mode);
DN_API DN_Str8FindResult        DN_Str8FindStr8Array        (DN_Str8 string, DN_Str8 const *find, DN_USize find_size, DN_Str8EqCase eq_case);
DN_API DN_Str8FindResult        DN_Str8FindStr8             (DN_Str8 string, DN_Str8 find, DN_Str8EqCase eq_case);
DN_API DN_Str8FindResult        DN_Str8Find                 (DN_Str8 string, uint32_t flags);
DN_API DN_Str8                  DN_Str8Segment              (DN_Arena *arena, DN_Str8 src, DN_USize segment_size, char segment_char);
DN_API DN_Str8                  DN_Str8ReverseSegment       (DN_Arena *arena, DN_Str8 src, DN_USize segment_size, char segment_char);
DN_API bool                     DN_Str8Eq                   (DN_Str8 lhs, DN_Str8 rhs, DN_Str8EqCase eq_case = DN_Str8EqCase_Sensitive);
DN_API bool                     DN_Str8EqInsensitive        (DN_Str8 lhs, DN_Str8 rhs);
DN_API bool                     DN_Str8StartsWith           (DN_Str8 string, DN_Str8 prefix, DN_Str8EqCase eq_case = DN_Str8EqCase_Sensitive);
DN_API bool                     DN_Str8StartsWithInsensitive(DN_Str8 string, DN_Str8 prefix);
DN_API bool                     DN_Str8EndsWith             (DN_Str8 string, DN_Str8 prefix, DN_Str8EqCase eq_case = DN_Str8EqCase_Sensitive);
DN_API bool                     DN_Str8EndsWithInsensitive  (DN_Str8 string, DN_Str8 prefix);
DN_API bool                     DN_Str8HasChar              (DN_Str8 string, char ch);
DN_API DN_Str8                  DN_Str8TrimPrefix           (DN_Str8 string, DN_Str8 prefix, DN_Str8EqCase eq_case = DN_Str8EqCase_Sensitive);
DN_API DN_Str8                  DN_Str8TrimHexPrefix        (DN_Str8 string);
DN_API DN_Str8                  DN_Str8TrimSuffix           (DN_Str8 string, DN_Str8 suffix, DN_Str8EqCase eq_case = DN_Str8EqCase_Sensitive);
DN_API DN_Str8                  DN_Str8TrimAround           (DN_Str8 string, DN_Str8 trim_string);
DN_API DN_Str8                  DN_Str8TrimHeadWhitespace   (DN_Str8 string);
DN_API DN_Str8                  DN_Str8TrimTailWhitespace   (DN_Str8 string);
DN_API DN_Str8                  DN_Str8TrimWhitespaceAround (DN_Str8 string);
DN_API DN_Str8                  DN_Str8TrimByteOrderMark    (DN_Str8 string);
DN_API DN_Str8                  DN_Str8FileNameFromPath     (DN_Str8 path);
DN_API DN_Str8                  DN_Str8FileNameNoExtension  (DN_Str8 path);
DN_API DN_Str8                  DN_Str8FilePathNoExtension  (DN_Str8 path);
DN_API DN_Str8                  DN_Str8FileExtension        (DN_Str8 path);
DN_API DN_Str8                  DN_Str8FileDirectoryFromPath(DN_Str8 path);
DN_API DN_Str8                  DN_Str8AppendF              (DN_Arena *arena, DN_Str8 string, char const *fmt, ...);
DN_API DN_Str8                  DN_Str8AppendFV             (DN_Arena *arena, DN_Str8 string, char const *fmt, va_list args);
DN_API DN_Str8                  DN_Str8FillF                (DN_Arena *arena, DN_USize count, char const *fmt, ...);
DN_API DN_Str8                  DN_Str8FillFV               (DN_Arena *arena, DN_USize count, char const *fmt, va_list args);
DN_API void                     DN_Str8Remove               (DN_Str8 *string, DN_USize offset, DN_USize size);
DN_API DN_Str8TruncateResult    DN_Str8TruncateMiddle       (DN_Arena *arena, DN_Str8 str8, DN_U32 side_size, DN_Str8 truncator);
DN_API DN_Str8                  DN_Str8Lower                (DN_Arena *arena, DN_Str8 string);
DN_API DN_Str8                  DN_Str8Upper                (DN_Arena *arena, DN_Str8 string);

DN_API  DN_Str8Builder          DN_Str8BuilderFromArena               (DN_Arena *arena);
DN_API  DN_Str8Builder          DN_Str8BuilderFromStr8PtrRef          (DN_Arena *arena, DN_Str8 const *strings, DN_USize size);
DN_API  DN_Str8Builder          DN_Str8BuilderFromStr8PtrCopy         (DN_Arena *arena, DN_Str8 const *strings, DN_USize size);
DN_API  DN_Str8Builder          DN_Str8BuilderFromBuilder             (DN_Arena *arena, DN_Str8Builder const *builder);
DN_API  bool                    DN_Str8BuilderAddArrayRef             (DN_Str8Builder *builder, DN_Str8 const *strings, DN_USize size, DN_Str8BuilderAdd add);
DN_API  bool                    DN_Str8BuilderAddArrayCopy            (DN_Str8Builder *builder, DN_Str8 const *strings, DN_USize size, DN_Str8BuilderAdd add);
DN_API  bool                    DN_Str8BuilderAddFV                   (DN_Str8Builder *builder, DN_Str8BuilderAdd add, DN_FMT_ATTRIB char const *fmt, va_list args);
#define                         DN_Str8BuilderAppendArrayRef(builder, strings, size)  DN_Str8BuilderAddArrayRef(builder, strings, size, DN_Str8BuilderAdd_Append)
#define                         DN_Str8BuilderAppendArrayCopy(builder, strings, size) DN_Str8BuilderAddArrayCopy(builder, strings, size, DN_Str8BuilderAdd_Append)
#define                         DN_Str8BuilderAppendSliceRef(builder, slice)          DN_Str8BuilderAddArrayRef(builder, slice.data, slice.size, DN_Str8BuilderAdd_Append)
#define                         DN_Str8BuilderAppendSliceCopy(builder, slice)         DN_Str8BuilderAddArrayCopy(builder, slice.data, slice.size, DN_Str8BuilderAdd_Append)
DN_API  bool                    DN_Str8BuilderAppendRef               (DN_Str8Builder *builder, DN_Str8 string);
DN_API  bool                    DN_Str8BuilderAppendCopy              (DN_Str8Builder *builder, DN_Str8 string);
#define                         DN_Str8BuilderAppendFV(builder, fmt, args)            DN_Str8BuilderAddFV(builder, DN_Str8BuilderAdd_Append, fmt, args)
DN_API  bool                    DN_Str8BuilderAppendF                 (DN_Str8Builder *builder, DN_FMT_ATTRIB char const *fmt, ...);
DN_API  bool                    DN_Str8BuilderAppendBytesRef          (DN_Str8Builder *builder, void const *ptr, DN_USize size);
DN_API  bool                    DN_Str8BuilderAppendBytesCopy         (DN_Str8Builder *builder, void const *ptr, DN_USize size);
DN_API  bool                    DN_Str8BuilderAppendBuilderRef        (DN_Str8Builder *dest, DN_Str8Builder const *src);
DN_API  bool                    DN_Str8BuilderAppendBuilderCopy       (DN_Str8Builder *dest, DN_Str8Builder const *src);
#define                         DN_Str8BuilderPrependArrayRef(builder, strings, size)  DN_Str8BuilderAddArrayRef(builder, strings, size, DN_Str8BuilderAdd_Prepend)
#define                         DN_Str8BuilderPrependArrayCopy(builder, strings, size) DN_Str8BuilderAddArrayCopy(builder, strings, size, DN_Str8BuilderAdd_Prepend)
#define                         DN_Str8BuilderPrependSliceRef(builder, slice)          DN_Str8BuilderAddArrayRef(builder, slice.data, slice.size, DN_Str8BuilderAdd_Prepend)
#define                         DN_Str8BuilderPrependSliceCopy(builder, slice)         DN_Str8BuilderAddArrayCopy(builder, slice.data, slice.size, DN_Str8BuilderAdd_Prepend)
DN_API  bool                    DN_Str8BuilderPrependRef              (DN_Str8Builder *builder, DN_Str8 string);
DN_API  bool                    DN_Str8BuilderPrependCopy             (DN_Str8Builder *builder, DN_Str8 string);
#define                         DN_Str8BuilderPrependFV(builder, fmt, args)            DN_Str8BuilderAddFV(builder, DN_Str8BuilderAdd_Prepend, fmt, args)
DN_API  bool                    DN_Str8BuilderPrependF                (DN_Str8Builder *builder, DN_FMT_ATTRIB char const *fmt, ...);
DN_API  bool                    DN_Str8BuilderErase                   (DN_Str8Builder *builder, DN_Str8 string);
DN_API  DN_Str8                 DN_Str8BuilderBuild                   (DN_Str8Builder const *builder, DN_Arena *arena);
DN_API  DN_Str8                 DN_Str8BuilderBuildDelimited          (DN_Str8Builder const *builder, DN_Str8 delimiter, DN_Arena *arena);
DN_API  DN_Slice<DN_Str8>       DN_Str8BuilderBuildSlice              (DN_Str8Builder const *builder, DN_Arena *arena);

DN_API int                      DN_EncodeUTF8Codepoint                (uint8_t utf8[4], uint32_t codepoint);
DN_API int                      DN_EncodeUTF16Codepoint               (uint16_t utf16[2], uint32_t codepoint);

DN_API DN_U8                    DN_U8FromHexNibble          (char hex);
DN_API DN_NibbleFromU8Result    DN_NibbleFromU8             (DN_U8 u8);

DN_API DN_USize                 DN_BytesFromHexPtr          (void const *hex, DN_USize hex_count, void *dest, DN_USize dest_count);
DN_API DN_Str8                  DN_BytesFromHexPtrArena     (void const *hex, DN_USize hex_count, DN_Arena *arena);
DN_API DN_USize                 DN_BytesFromHexStr8         (DN_Str8 hex, void *dest, DN_USize dest_count);
DN_API DN_Str8                  DN_BytesFromHexStr8Arena    (DN_Str8 hex, DN_Arena *arena);
DN_API DN_U8x16                 DN_BytesFromHex32Ptr        (void const *hex, DN_USize hex_count);
DN_API DN_U8x32                 DN_BytesFromHex64Ptr        (void const *hex, DN_USize hex_count);

DN_API DN_HexU64Str8            DN_HexFromU64               (DN_U64 value, DN_HexFromU64Type type);
DN_API DN_USize                 DN_HexFromBytesPtr          (void const *bytes, DN_USize bytes_count, void *hex, DN_USize hex_count);
DN_API DN_Str8                  DN_HexFromBytesPtrArena     (void const *bytes, DN_USize bytes_count, DN_Arena *arena);
DN_API DN_Hex32                 DN_HexFromBytes16Ptr        (void const *bytes, DN_USize bytes_count);
DN_API DN_Hex64                 DN_HexFromBytes32Ptr        (void const *bytes, DN_USize bytes_count);
DN_API DN_Hex128                DN_HexFromBytes64Ptr        (void const *bytes, DN_USize bytes_count);

DN_API DN_Str8x128              DN_AgeStr8FromMsU64         (DN_U64 duration_ms, DN_AgeUnit units);
DN_API DN_Str8x128              DN_AgeStr8FromSecU64        (DN_U64 duration_ms, DN_AgeUnit units);
DN_API DN_Str8x128              DN_AgeStr8FromSecF64        (DN_F64 sec, DN_AgeUnit units);

DN_API DN_ByteCountResult       DN_ByteCountFromType        (DN_U64 bytes, DN_ByteCountType type);
#define                         DN_ByteCount(bytes)         DN_ByteCountFromType(bytes, DN_ByteCountType_Auto)
DN_API DN_Str8x32               DN_ByteCountStr8x32FromType (DN_U64 bytes, DN_ByteCountType type);
#define                         DN_ByteCountStr8x32(bytes)  DN_ByteCountStr8x32FromType(bytes, DN_ByteCountType_Auto)

#endif // !defined(DN_BASE_H)
