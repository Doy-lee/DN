#define DN_BASE_CPP

#include "../dn_base_inc.h"

DN_API bool DN_MemEq(void const *lhs, DN_USize lhs_size, void const *rhs, DN_USize rhs_size)
{
  bool result = lhs_size == rhs_size && DN_Memcmp(lhs, rhs, rhs_size) == 0;
  return result;
}

#if !defined(DN_PLATFORM_ARM64) && !defined(DN_PLATFORM_EMSCRIPTEN)
  #define DN_SUPPORTS_CPU_ID
#endif

#if defined(DN_SUPPORTS_CPU_ID) && (defined(DN_COMPILER_GCC) || defined(DN_COMPILER_CLANG))
  #include <cpuid.h>
#endif

static DN_CPUFeatureDecl g_dn_cpu_feature_decl[DN_CPUFeature_Count];

DN_API DN_U64 DN_AtomicSetValue64(DN_U64 volatile *target, DN_U64 value)
{
#if defined(DN_COMPILER_MSVC) || defined(DN_COMPILER_CLANG_CL)
  __int64 result;
  do {
    result = *target;
  } while (DN_AtomicCompareExchange64(target, value, result) != result);
  return DN_Cast(DN_U64) result;
#elif defined(DN_COMPILER_GCC) || defined(DN_COMPILER_CLANG)
  DN_U64 result = __sync_lock_test_and_set(target, value);
  return result;
#else
  #error Unsupported compiler
#endif
}

DN_API DN_U32 DN_AtomicSetValue32(DN_U32 volatile *target, DN_U32 value)
{
#if defined(DN_COMPILER_MSVC) || defined(DN_COMPILER_CLANG_CL)
  long result;
  do {
    result = *target;
  } while (DN_AtomicCompareExchange32(target, value, result) != result);
  return result;
#elif defined(DN_COMPILER_GCC) || defined(DN_COMPILER_CLANG)
  long result = __sync_lock_test_and_set(target, value);
  return result;
#else
  #error Unsupported compiler
#endif
}

DN_API DN_CPUIDResult DN_CPUID(DN_CPUIDArgs args)
{
  DN_CPUIDResult result = {};
#if defined(DN_SUPPORTS_CPU_ID)
  __cpuidex(result.values, args.eax, args.ecx);
#endif
  return result;
}

DN_API DN_USize DN_CPUHasFeatureArray(DN_CPUReport const *report, DN_CPUFeatureQuery *features, DN_USize features_size)
{
  DN_USize       result = 0;
  DN_USize const BITS   = sizeof(report->features[0]) * 8;
  for (DN_ForIndexU(feature_index, features_size)) {
    DN_CPUFeatureQuery *query       = features + feature_index;
    DN_USize            chunk_index = query->feature / BITS;
    DN_USize            chunk_bit   = query->feature % BITS;
    DN_U64              chunk       = report->features[chunk_index];
    query->available                = chunk & (1ULL << chunk_bit);
    result += DN_Cast(int) query->available;
  }

  return result;
}

DN_API bool DN_CPUHasFeature(DN_CPUReport const *report, DN_CPUFeature feature)
{
  DN_CPUFeatureQuery query = {};
  query.feature            = feature;
  bool result              = DN_CPUHasFeatureArray(report, &query, 1) == 1;
  return result;
}

DN_API bool DN_CPUHasAllFeatures(DN_CPUReport const *report, DN_CPUFeature const *features, DN_USize features_size)
{
  bool result = true;
  for (DN_USize index = 0; result && index < features_size; index++)
    result &= DN_CPUHasFeature(report, features[index]);
  return result;
}

DN_API void DN_CPUSetFeature(DN_CPUReport *report, DN_CPUFeature feature)
{
  DN_Assert(feature < DN_CPUFeature_Count);
  DN_USize const BITS        = sizeof(report->features[0]) * 8;
  DN_USize       chunk_index = feature / BITS;
  DN_USize       chunk_bit   = feature % BITS;
  report->features[chunk_index] |= (1ULL << chunk_bit);
}

DN_API DN_CPUReport DN_CPUGetReport()
{
  DN_CPUReport   result                 = {};
#if defined(DN_SUPPORTS_CPU_ID)
  DN_CPUIDResult fn_0000_[500]          = {};
  DN_CPUIDResult fn_8000_[500]          = {};
  int const      EXTENDED_FUNC_BASE_EAX = 0x8000'0000;
  int const      REGISTER_SIZE          = sizeof(fn_0000_[0].reg.eax);

  // NOTE: Query standard/extended numbers ///////////////////////////////////////////////////////
  {
    DN_CPUIDArgs args = {};

    // NOTE: Query standard function (e.g. eax = 0x0)         for function count + cpu vendor
    args        = {};
    fn_0000_[0] = DN_CPUID(args);

    // NOTE: Query extended function (e.g. eax = 0x8000'0000) for function count + cpu vendor
    args        = {};
    args.eax    = DN_Cast(int) EXTENDED_FUNC_BASE_EAX;
    fn_8000_[0] = DN_CPUID(args);
  }

  // NOTE: Extract function count ////////////////////////////////////////////////////////////////
  int const STANDARD_FUNC_MAX_EAX = fn_0000_[0x0000].reg.eax;
  int const EXTENDED_FUNC_MAX_EAX = fn_8000_[0x0000].reg.eax;

  // NOTE: Enumerate all CPUID results for the known function counts /////////////////////////////
  {
    DN_AssertF((STANDARD_FUNC_MAX_EAX + 1) <= DN_ArrayCountI(fn_0000_),
               "Max standard count is %d",
               STANDARD_FUNC_MAX_EAX + 1);
    DN_AssertF((DN_Cast(DN_ISize) EXTENDED_FUNC_MAX_EAX - EXTENDED_FUNC_BASE_EAX + 1) <= DN_ArrayCountI(fn_8000_),
               "Max extended count is %zu",
               DN_Cast(DN_ISize) EXTENDED_FUNC_MAX_EAX - EXTENDED_FUNC_BASE_EAX + 1);

    for (int eax = 1; eax <= STANDARD_FUNC_MAX_EAX; eax++) {
      DN_CPUIDArgs args = {};
      args.eax          = eax;
      fn_0000_[eax]     = DN_CPUID(args);
    }

    for (int eax = EXTENDED_FUNC_BASE_EAX + 1, index = 1; eax <= EXTENDED_FUNC_MAX_EAX; eax++, index++) {
      DN_CPUIDArgs args = {};
      args.eax          = eax;
      fn_8000_[index]   = DN_CPUID(args);
    }
  }

  // NOTE: Query CPU vendor //////////////////////////////////////////////////////////////////////
  {
    DN_Memcpy(result.vendor + 0, &fn_8000_[0x0000].reg.ebx, REGISTER_SIZE);
    DN_Memcpy(result.vendor + 4, &fn_8000_[0x0000].reg.edx, REGISTER_SIZE);
    DN_Memcpy(result.vendor + 8, &fn_8000_[0x0000].reg.ecx, REGISTER_SIZE);
  }

  // NOTE: Query CPU brand ///////////////////////////////////////////////////////////////////////
  if (EXTENDED_FUNC_MAX_EAX >= (EXTENDED_FUNC_BASE_EAX + 4)) {
    DN_Memcpy(result.brand + 0, &fn_8000_[0x0002].reg.eax, REGISTER_SIZE);
    DN_Memcpy(result.brand + 4, &fn_8000_[0x0002].reg.ebx, REGISTER_SIZE);
    DN_Memcpy(result.brand + 8, &fn_8000_[0x0002].reg.ecx, REGISTER_SIZE);
    DN_Memcpy(result.brand + 12, &fn_8000_[0x0002].reg.edx, REGISTER_SIZE);

    DN_Memcpy(result.brand + 16, &fn_8000_[0x0003].reg.eax, REGISTER_SIZE);
    DN_Memcpy(result.brand + 20, &fn_8000_[0x0003].reg.ebx, REGISTER_SIZE);
    DN_Memcpy(result.brand + 24, &fn_8000_[0x0003].reg.ecx, REGISTER_SIZE);
    DN_Memcpy(result.brand + 28, &fn_8000_[0x0003].reg.edx, REGISTER_SIZE);

    DN_Memcpy(result.brand + 32, &fn_8000_[0x0004].reg.eax, REGISTER_SIZE);
    DN_Memcpy(result.brand + 36, &fn_8000_[0x0004].reg.ebx, REGISTER_SIZE);
    DN_Memcpy(result.brand + 40, &fn_8000_[0x0004].reg.ecx, REGISTER_SIZE);
    DN_Memcpy(result.brand + 44, &fn_8000_[0x0004].reg.edx, REGISTER_SIZE);

    DN_Assert(result.brand[sizeof(result.brand) - 1] == 0);
  }

  // NOTE: Query CPU features //////////////////////////////////////////////////////////////////
  for (DN_USize ext_index = 0; ext_index < DN_CPUFeature_Count; ext_index++) {
    bool available = false;

    // NOTE: Mask bits taken from various manuals
    //   - AMD64 Architecture Programmer's Manual, Volumes 1-5
    //   - https://en.wikipedia.org/wiki/CPUID#Calling_CPUID
    switch (DN_Cast(DN_CPUFeature)           ext_index) {
      case DN_CPUFeature_3DNow:              available = (fn_8000_[0x0001].reg.edx & (1 << 31)); break;
      case DN_CPUFeature_3DNowExt:           available = (fn_8000_[0x0001].reg.edx & (1 << 30)); break;
      case DN_CPUFeature_ABM:                available = (fn_8000_[0x0001].reg.ecx & (1 << 5)); break;
      case DN_CPUFeature_AES:                available = (fn_0000_[0x0001].reg.ecx & (1 << 25)); break;
      case DN_CPUFeature_AVX:                available = (fn_0000_[0x0001].reg.ecx & (1 << 28)); break;
      case DN_CPUFeature_AVX2:               available = (fn_0000_[0x0007].reg.ebx & (1 << 0)); break;
      case DN_CPUFeature_AVX512F:            available = (fn_0000_[0x0007].reg.ebx & (1 << 16)); break;
      case DN_CPUFeature_AVX512DQ:           available = (fn_0000_[0x0007].reg.ebx & (1 << 17)); break;
      case DN_CPUFeature_AVX512IFMA:         available = (fn_0000_[0x0007].reg.ebx & (1 << 21)); break;
      case DN_CPUFeature_AVX512PF:           available = (fn_0000_[0x0007].reg.ebx & (1 << 26)); break;
      case DN_CPUFeature_AVX512ER:           available = (fn_0000_[0x0007].reg.ebx & (1 << 27)); break;
      case DN_CPUFeature_AVX512CD:           available = (fn_0000_[0x0007].reg.ebx & (1 << 28)); break;
      case DN_CPUFeature_AVX512BW:           available = (fn_0000_[0x0007].reg.ebx & (1 << 30)); break;
      case DN_CPUFeature_AVX512VL:           available = (fn_0000_[0x0007].reg.ebx & (1 << 31)); break;
      case DN_CPUFeature_AVX512VBMI:         available = (fn_0000_[0x0007].reg.ecx & (1 << 1)); break;
      case DN_CPUFeature_AVX512VBMI2:        available = (fn_0000_[0x0007].reg.ecx & (1 << 6)); break;
      case DN_CPUFeature_AVX512VNNI:         available = (fn_0000_[0x0007].reg.ecx & (1 << 11)); break;
      case DN_CPUFeature_AVX512BITALG:       available = (fn_0000_[0x0007].reg.ecx & (1 << 12)); break;
      case DN_CPUFeature_AVX512VPOPCNTDQ:    available = (fn_0000_[0x0007].reg.ecx & (1 << 14)); break;
      case DN_CPUFeature_AVX5124VNNIW:       available = (fn_0000_[0x0007].reg.edx & (1 << 2)); break;
      case DN_CPUFeature_AVX5124FMAPS:       available = (fn_0000_[0x0007].reg.edx & (1 << 3)); break;
      case DN_CPUFeature_AVX512VP2INTERSECT: available = (fn_0000_[0x0007].reg.edx & (1 << 8)); break;
      case DN_CPUFeature_AVX512FP16:         available = (fn_0000_[0x0007].reg.edx & (1 << 23)); break;
      case DN_CPUFeature_CLZERO:             available = (fn_8000_[0x0008].reg.ebx & (1 << 0)); break;
      case DN_CPUFeature_CMPXCHG8B:          available = (fn_0000_[0x0001].reg.edx & (1 << 8)); break;
      case DN_CPUFeature_CMPXCHG16B:         available = (fn_0000_[0x0001].reg.ecx & (1 << 13)); break;
      case DN_CPUFeature_F16C:               available = (fn_0000_[0x0001].reg.ecx & (1 << 29)); break;
      case DN_CPUFeature_FMA:                available = (fn_0000_[0x0001].reg.ecx & (1 << 12)); break;
      case DN_CPUFeature_FMA4:               available = (fn_8000_[0x0001].reg.ecx & (1 << 16)); break;
      case DN_CPUFeature_FP128:              available = (fn_8000_[0x001A].reg.eax & (1 << 0)); break;
      case DN_CPUFeature_FP256:              available = (fn_8000_[0x001A].reg.eax & (1 << 2)); break;
      case DN_CPUFeature_FPU:                available = (fn_0000_[0x0001].reg.edx & (1 << 0)); break;
      case DN_CPUFeature_MMX:                available = (fn_0000_[0x0001].reg.edx & (1 << 23)); break;
      case DN_CPUFeature_MONITOR:            available = (fn_0000_[0x0001].reg.ecx & (1 << 3)); break;
      case DN_CPUFeature_MOVBE:              available = (fn_0000_[0x0001].reg.ecx & (1 << 22)); break;
      case DN_CPUFeature_MOVU:               available = (fn_8000_[0x001A].reg.eax & (1 << 1)); break;
      case DN_CPUFeature_MmxExt:             available = (fn_8000_[0x0001].reg.edx & (1 << 22)); break;
      case DN_CPUFeature_PCLMULQDQ:          available = (fn_0000_[0x0001].reg.ecx & (1 << 1)); break;
      case DN_CPUFeature_POPCNT:             available = (fn_0000_[0x0001].reg.ecx & (1 << 23)); break;
      case DN_CPUFeature_RDRAND:             available = (fn_0000_[0x0001].reg.ecx & (1 << 30)); break;
      case DN_CPUFeature_RDSEED:             available = (fn_0000_[0x0007].reg.ebx & (1 << 18)); break;
      case DN_CPUFeature_RDTSCP:             available = (fn_8000_[0x0001].reg.edx & (1 << 27)); break;
      case DN_CPUFeature_SHA:                available = (fn_0000_[0x0007].reg.ebx & (1 << 29)); break;
      case DN_CPUFeature_SSE:                available = (fn_0000_[0x0001].reg.edx & (1 << 25)); break;
      case DN_CPUFeature_SSE2:               available = (fn_0000_[0x0001].reg.edx & (1 << 26)); break;
      case DN_CPUFeature_SSE3:               available = (fn_0000_[0x0001].reg.ecx & (1 << 0)); break;
      case DN_CPUFeature_SSE41:              available = (fn_0000_[0x0001].reg.ecx & (1 << 19)); break;
      case DN_CPUFeature_SSE42:              available = (fn_0000_[0x0001].reg.ecx & (1 << 20)); break;
      case DN_CPUFeature_SSE4A:              available = (fn_8000_[0x0001].reg.ecx & (1 << 6)); break;
      case DN_CPUFeature_SSSE3:              available = (fn_0000_[0x0001].reg.ecx & (1 << 9)); break;
      case DN_CPUFeature_TSC:                available = (fn_0000_[0x0001].reg.edx & (1 << 4)); break;
      case DN_CPUFeature_TscInvariant:       available = (fn_8000_[0x0007].reg.edx & (1 << 8)); break;
      case DN_CPUFeature_VAES:               available = (fn_0000_[0x0007].reg.ecx & (1 << 9)); break;
      case DN_CPUFeature_VPCMULQDQ:          available = (fn_0000_[0x0007].reg.ecx & (1 << 10)); break;
      case DN_CPUFeature_Count:              DN_InvalidCodePath; break;
    }

    if (available)
      DN_CPUSetFeature(&result, DN_Cast(DN_CPUFeature) ext_index);
  }
#endif // DN_SUPPORTS_CPU_ID
  return result;
}

// NOTE: DN_TicketMutex ////////////////////////////////////////////////////////////////////////////
DN_API void DN_TicketMutex_Begin(DN_TicketMutex *mutex)
{
  unsigned int ticket = DN_AtomicAddU32(&mutex->ticket, 1);
  DN_TicketMutex_BeginTicket(mutex, ticket);
}

DN_API void DN_TicketMutex_End(DN_TicketMutex *mutex)
{
  DN_AtomicAddU32(&mutex->serving, 1);
}

DN_API DN_UInt DN_TicketMutex_MakeTicket(DN_TicketMutex *mutex)
{
  DN_UInt result = DN_AtomicAddU32(&mutex->ticket, 1);
  return result;
}

DN_API void DN_TicketMutex_BeginTicket(DN_TicketMutex const *mutex, DN_UInt ticket)
{
  DN_AssertF(mutex->serving <= ticket,
             "Mutex skipped ticket? Was ticket generated by the correct mutex via MakeTicket? ticket = %u, "
             "mutex->serving = %u",
             ticket,
             mutex->serving);
  while (ticket != mutex->serving) {
    // NOTE: Use spinlock intrinsic
    _mm_pause();
  }
}

DN_API bool DN_TicketMutex_CanLock(DN_TicketMutex const *mutex, DN_UInt ticket)
{
  bool result = (ticket == mutex->serving);
  return result;
}

#if defined(DN_COMPILER_MSVC) || defined(DN_COMPILER_CLANG_CL)
  #if !defined(DN_CRT_SECURE_NO_WARNINGS_PREVIOUSLY_DEFINED)
    #undef _CRT_SECURE_NO_WARNINGS
  #endif
#endif

// NOTE: DN_Bit ////////////////////////////////////////////////////////////////////////////////////
DN_API void DN_BitUnsetInplace(DN_USize *flags, DN_USize bitfield)
{
  *flags = (*flags & ~bitfield);
}

DN_API void DN_BitSetInplace(DN_USize *flags, DN_USize bitfield)
{
  *flags = (*flags | bitfield);
}

DN_API bool DN_BitIsSet(DN_USize bits, DN_USize bits_to_set)
{
  auto result = DN_Cast(bool)((bits & bits_to_set) == bits_to_set);
  return result;
}

DN_API bool DN_BitIsNotSet(DN_USize bits, DN_USize bits_to_check)
{
  auto result = !DN_BitIsSet(bits, bits_to_check);
  return result;
}

// NOTE: DN_Safe ///////////////////////////////////////////////////////////////////////////////////
DN_API DN_I64 DN_SafeAddI64(int64_t a, int64_t b)
{
  DN_I64 result = DN_CheckF(a <= INT64_MAX - b, "a=%zd, b=%zd", a, b) ? (a + b) : INT64_MAX;
  return result;
}

DN_API DN_I64 DN_SafeMulI64(int64_t a, int64_t b)
{
  DN_I64 result = DN_CheckF(a <= INT64_MAX / b, "a=%zd, b=%zd", a, b) ? (a * b) : INT64_MAX;
  return result;
}

DN_API DN_U64 DN_SafeAddU64(DN_U64 a, DN_U64 b)
{
  DN_U64 result = DN_CheckF(a <= UINT64_MAX - b, "a=%zu, b=%zu", a, b) ? (a + b) : UINT64_MAX;
  return result;
}

DN_API DN_U64 DN_SafeSubU64(DN_U64 a, DN_U64 b)
{
  DN_U64 result = DN_CheckF(a >= b, "a=%zu, b=%zu", a, b) ? (a - b) : 0;
  return result;
}

DN_API DN_U64 DN_SafeMulU64(DN_U64 a, DN_U64 b)
{
  DN_U64 result = DN_CheckF(a <= UINT64_MAX / b, "a=%zu, b=%zu", a, b) ? (a * b) : UINT64_MAX;
  return result;
}

DN_API DN_U32 DN_SafeSubU32(DN_U32 a, DN_U32 b)
{
  DN_U32 result = DN_CheckF(a >= b, "a=%u, b=%u", a, b) ? (a - b) : 0;
  return result;
}

// NOTE: DN_SaturateCastUSizeToI* ////////////////////////////////////////////////////////////
// INT*_MAX literals will be promoted to the type of uintmax_t as uintmax_t is
// the highest possible rank (unsigned > signed).
DN_API int DN_SaturateCastUSizeToInt(DN_USize val)
{
  int result = DN_Check(DN_Cast(uintmax_t) val <= INT_MAX) ? DN_Cast(int) val : INT_MAX;
  return result;
}

DN_API int8_t DN_SaturateCastUSizeToI8(DN_USize val)
{
  int8_t result = DN_Check(DN_Cast(uintmax_t) val <= INT8_MAX) ? DN_Cast(int8_t) val : INT8_MAX;
  return result;
}

DN_API DN_I16 DN_SaturateCastUSizeToI16(DN_USize val)
{
  DN_I16 result = DN_Check(DN_Cast(uintmax_t) val <= INT16_MAX) ? DN_Cast(DN_I16) val : INT16_MAX;
  return result;
}

DN_API DN_I32 DN_SaturateCastUSizeToI32(DN_USize val)
{
  DN_I32 result = DN_Check(DN_Cast(uintmax_t) val <= INT32_MAX) ? DN_Cast(DN_I32) val : INT32_MAX;
  return result;
}

DN_API int64_t DN_SaturateCastUSizeToI64(DN_USize val)
{
  int64_t result = DN_Check(DN_Cast(uintmax_t) val <= INT64_MAX) ? DN_Cast(int64_t) val : INT64_MAX;
  return result;
}

// NOTE: DN_SaturateCastUSizeToU* ////////////////////////////////////////////////////////////
// Both operands are unsigned and the lowest rank operand will be promoted to
// match the highest rank operand.
DN_API DN_U8 DN_SaturateCastUSizeToU8(DN_USize val)
{
  DN_U8 result = DN_Check(val <= UINT8_MAX) ? DN_Cast(DN_U8) val : UINT8_MAX;
  return result;
}

DN_API DN_U16 DN_SaturateCastUSizeToU16(DN_USize val)
{
  DN_U16 result = DN_Check(val <= UINT16_MAX) ? DN_Cast(DN_U16) val : UINT16_MAX;
  return result;
}

DN_API DN_U32 DN_SaturateCastUSizeToU32(DN_USize val)
{
  DN_U32 result = DN_Check(val <= UINT32_MAX) ? DN_Cast(DN_U32) val : UINT32_MAX;
  return result;
}

DN_API DN_U64 DN_SaturateCastUSizeToU64(DN_USize val)
{
  DN_U64 result = DN_Check(DN_Cast(DN_U64) val <= UINT64_MAX) ? DN_Cast(DN_U64) val : UINT64_MAX;
  return result;
}

// NOTE: DN_SaturateCastU64To* ///////////////////////////////////////////////////////////////
DN_API int DN_SaturateCastU64ToInt(DN_U64 val)
{
  int result = DN_Check(val <= INT_MAX) ? DN_Cast(int) val : INT_MAX;
  return result;
}

DN_API int8_t DN_SaturateCastU64ToI8(DN_U64 val)
{
  int8_t result = DN_Check(val <= INT8_MAX) ? DN_Cast(int8_t) val : INT8_MAX;
  return result;
}

DN_API DN_I16 DN_SaturateCastU64ToI16(DN_U64 val)
{
  DN_I16 result = DN_Check(val <= INT16_MAX) ? DN_Cast(DN_I16) val : INT16_MAX;
  return result;
}

DN_API DN_I32 DN_SaturateCastU64ToI32(DN_U64 val)
{
  DN_I32 result = DN_Check(val <= INT32_MAX) ? DN_Cast(DN_I32) val : INT32_MAX;
  return result;
}

DN_API int64_t DN_SaturateCastU64ToI64(DN_U64 val)
{
  int64_t result = DN_Check(val <= INT64_MAX) ? DN_Cast(int64_t) val : INT64_MAX;
  return result;
}

// Both operands are unsigned and the lowest rank operand will be promoted to
// match the highest rank operand.
DN_API unsigned int DN_SaturateCastU64ToUInt(DN_U64 val)
{
  unsigned int result = DN_Check(val <= UINT8_MAX) ? DN_Cast(unsigned int) val : UINT_MAX;
  return result;
}

DN_API DN_U8 DN_SaturateCastU64ToU8(DN_U64 val)
{
  DN_U8 result = DN_Check(val <= UINT8_MAX) ? DN_Cast(DN_U8) val : UINT8_MAX;
  return result;
}

DN_API DN_U16 DN_SaturateCastU64ToU16(DN_U64 val)
{
  DN_U16 result = DN_Check(val <= UINT16_MAX) ? DN_Cast(DN_U16) val : UINT16_MAX;
  return result;
}

DN_API DN_U32 DN_SaturateCastU64ToU32(DN_U64 val)
{
  DN_U32 result = DN_Check(val <= UINT32_MAX) ? DN_Cast(DN_U32) val : UINT32_MAX;
  return result;
}

// NOTE: DN_SaturateCastISizeToI* ////////////////////////////////////////////////////////////
// Both operands are signed so the lowest rank operand will be promoted to
// match the highest rank operand.
DN_API int DN_SaturateCastISizeToInt(DN_ISize val)
{
  DN_Assert(val >= INT_MIN && val <= INT_MAX);
  int result = DN_Cast(int) DN_Clamp(val, INT_MIN, INT_MAX);
  return result;
}

DN_API int8_t DN_SaturateCastISizeToI8(DN_ISize val)
{
  DN_Assert(val >= INT8_MIN && val <= INT8_MAX);
  int8_t result = DN_Cast(int8_t) DN_Clamp(val, INT8_MIN, INT8_MAX);
  return result;
}

DN_API DN_I16 DN_SaturateCastISizeToI16(DN_ISize val)
{
  DN_Assert(val >= INT16_MIN && val <= INT16_MAX);
  DN_I16 result = DN_Cast(DN_I16) DN_Clamp(val, INT16_MIN, INT16_MAX);
  return result;
}

DN_API DN_I32 DN_SaturateCastISizeToI32(DN_ISize val)
{
  DN_Assert(val >= INT32_MIN && val <= INT32_MAX);
  DN_I32 result = DN_Cast(DN_I32) DN_Clamp(val, INT32_MIN, INT32_MAX);
  return result;
}

DN_API int64_t DN_SaturateCastISizeToI64(DN_ISize val)
{
  DN_Assert(DN_Cast(int64_t) val >= INT64_MIN && DN_Cast(int64_t) val <= INT64_MAX);
  int64_t result = DN_Cast(int64_t) DN_Clamp(DN_Cast(int64_t) val, INT64_MIN, INT64_MAX);
  return result;
}

// NOTE: DN_SaturateCastISizeToU* ////////////////////////////////////////////////////////////
// If the value is a negative integer, we clamp to 0. Otherwise, we know that
// the value is >=0, we can upcast safely to bounds check against the maximum
// allowed value.
DN_API unsigned int DN_SaturateCastISizeToUInt(DN_ISize val)
{
  unsigned int result = 0;
  if (DN_Check(val >= DN_Cast(DN_ISize) 0)) {
    if (DN_Check(DN_Cast(uintmax_t) val <= UINT_MAX))
      result = DN_Cast(unsigned int) val;
    else
      result = UINT_MAX;
  }
  return result;
}

DN_API DN_U8 DN_SaturateCastISizeToU8(DN_ISize val)
{
  DN_U8 result = 0;
  if (DN_Check(val >= DN_Cast(DN_ISize) 0)) {
    if (DN_Check(DN_Cast(uintmax_t) val <= UINT8_MAX))
      result = DN_Cast(DN_U8) val;
    else
      result = UINT8_MAX;
  }
  return result;
}

DN_API DN_U16 DN_SaturateCastISizeToU16(DN_ISize val)
{
  DN_U16 result = 0;
  if (DN_Check(val >= DN_Cast(DN_ISize) 0)) {
    if (DN_Check(DN_Cast(uintmax_t) val <= UINT16_MAX))
      result = DN_Cast(DN_U16) val;
    else
      result = UINT16_MAX;
  }
  return result;
}

DN_API DN_U32 DN_SaturateCastISizeToU32(DN_ISize val)
{
  DN_U32 result = 0;
  if (DN_Check(val >= DN_Cast(DN_ISize) 0)) {
    if (DN_Check(DN_Cast(uintmax_t) val <= UINT32_MAX))
      result = DN_Cast(DN_U32) val;
    else
      result = UINT32_MAX;
  }
  return result;
}

DN_API DN_U64 DN_SaturateCastISizeToU64(DN_ISize val)
{
  DN_U64 result = 0;
  if (DN_Check(val >= DN_Cast(DN_ISize) 0)) {
    if (DN_Check(DN_Cast(uintmax_t) val <= UINT64_MAX))
      result = DN_Cast(DN_U64) val;
    else
      result = UINT64_MAX;
  }
  return result;
}

// NOTE: DN_SaturateCastI64To* ///////////////////////////////////////////////////////////////
// Both operands are signed so the lowest rank operand will be promoted to
// match the highest rank operand.
DN_API DN_ISize DN_SaturateCastI64ToISize(int64_t val)
{
  DN_Check(val >= DN_ISIZE_MIN && val <= DN_ISIZE_MAX);
  DN_ISize result = DN_Cast(int64_t) DN_Clamp(val, DN_ISIZE_MIN, DN_ISIZE_MAX);
  return result;
}

DN_API int8_t DN_SaturateCastI64ToI8(int64_t val)
{
  DN_Check(val >= INT8_MIN && val <= INT8_MAX);
  int8_t result = DN_Cast(int8_t) DN_Clamp(val, INT8_MIN, INT8_MAX);
  return result;
}

DN_API DN_I16 DN_SaturateCastI64ToI16(int64_t val)
{
  DN_Check(val >= INT16_MIN && val <= INT16_MAX);
  DN_I16 result = DN_Cast(DN_I16) DN_Clamp(val, INT16_MIN, INT16_MAX);
  return result;
}

DN_API DN_I32 DN_SaturateCastI64ToI32(int64_t val)
{
  DN_Check(val >= INT32_MIN && val <= INT32_MAX);
  DN_I32 result = DN_Cast(DN_I32) DN_Clamp(val, INT32_MIN, INT32_MAX);
  return result;
}

DN_API unsigned int DN_SaturateCastI64ToUInt(int64_t val)
{
  unsigned int result = 0;
  if (DN_Check(val >= DN_Cast(int64_t) 0)) {
    if (DN_Check(DN_Cast(uintmax_t) val <= UINT_MAX))
      result = DN_Cast(unsigned int) val;
    else
      result = UINT_MAX;
  }
  return result;
}

DN_API DN_ISize DN_SaturateCastI64ToUSize(int64_t val)
{
  DN_USize result = 0;
  if (DN_Check(val >= DN_Cast(int64_t) 0)) {
    if (DN_Check(DN_Cast(uintmax_t) val <= DN_USIZE_MAX))
      result = DN_Cast(DN_USize) val;
    else
      result = DN_USIZE_MAX;
  }
  return result;
}

DN_API DN_U8 DN_SaturateCastI64ToU8(int64_t val)
{
  DN_U8 result = 0;
  if (DN_Check(val >= DN_Cast(int64_t) 0)) {
    if (DN_Check(DN_Cast(uintmax_t) val <= UINT8_MAX))
      result = DN_Cast(DN_U8) val;
    else
      result = UINT8_MAX;
  }
  return result;
}

DN_API DN_U16 DN_SaturateCastI64ToU16(int64_t val)
{
  DN_U16 result = 0;
  if (DN_Check(val >= DN_Cast(int64_t) 0)) {
    if (DN_Check(DN_Cast(uintmax_t) val <= UINT16_MAX))
      result = DN_Cast(DN_U16) val;
    else
      result = UINT16_MAX;
  }
  return result;
}

DN_API DN_U32 DN_SaturateCastI64ToU32(int64_t val)
{
  DN_U32 result = 0;
  if (DN_Check(val >= DN_Cast(int64_t) 0)) {
    if (DN_Check(DN_Cast(uintmax_t) val <= UINT32_MAX))
      result = DN_Cast(DN_U32) val;
    else
      result = UINT32_MAX;
  }
  return result;
}

DN_API DN_U64 DN_SaturateCastI64ToU64(int64_t val)
{
  DN_U64 result = 0;
  if (DN_Check(val >= DN_Cast(int64_t) 0)) {
    if (DN_Check(DN_Cast(uintmax_t) val <= UINT64_MAX))
      result = DN_Cast(DN_U64) val;
    else
      result = UINT64_MAX;
  }
  return result;
}

DN_API int8_t DN_SaturateCastIntToI8(int val)
{
  DN_Check(val >= INT8_MIN && val <= INT8_MAX);
  int8_t result = DN_Cast(int8_t) DN_Clamp(val, INT8_MIN, INT8_MAX);
  return result;
}

DN_API DN_I16 DN_SaturateCastIntToI16(int val)
{
  DN_Check(val >= INT16_MIN && val <= INT16_MAX);
  DN_I16 result = DN_Cast(DN_I16) DN_Clamp(val, INT16_MIN, INT16_MAX);
  return result;
}

DN_API DN_U8 DN_SaturateCastIntToU8(int val)
{
  DN_U8 result = 0;
  if (DN_Check(val >= DN_Cast(DN_ISize) 0)) {
    if (DN_Check(DN_Cast(uintmax_t) val <= UINT8_MAX))
      result = DN_Cast(DN_U8) val;
    else
      result = UINT8_MAX;
  }
  return result;
}

DN_API DN_U16 DN_SaturateCastIntToU16(int val)
{
  DN_U16 result = 0;
  if (DN_Check(val >= DN_Cast(DN_ISize) 0)) {
    if (DN_Check(DN_Cast(uintmax_t) val <= UINT16_MAX))
      result = DN_Cast(DN_U16) val;
    else
      result = UINT16_MAX;
  }
  return result;
}

DN_API DN_U32 DN_SaturateCastIntToU32(int val)
{
  static_assert(sizeof(val) <= sizeof(DN_U32), "Sanity check to allow simplifying of casting");
  DN_U32 result = 0;
  if (DN_Check(val >= 0))
    result = DN_Cast(DN_U32) val;
  return result;
}

DN_API DN_U64 DN_SaturateCastIntToU64(int val)
{
  static_assert(sizeof(val) <= sizeof(DN_U64), "Sanity check to allow simplifying of casting");
  DN_U64 result = 0;
  if (DN_Check(val >= 0))
    result = DN_Cast(DN_U64) val;
  return result;
}

// NOTE: DN_Asan ////////////////////////////////////////////////////////////////////////// ////////
static_assert(DN_IsPowerOfTwoAligned(DN_ASAN_POISON_GUARD_SIZE, DN_ASAN_POISON_ALIGNMENT),
              "ASAN poison guard size must be a power-of-two and aligned to ASAN's alignment"
              "requirement (8 bytes)");

DN_API void DN_ASanPoisonMemoryRegion(void const volatile *ptr, DN_USize size)
{
  if (!ptr || !size)
    return;

#if DN_HAS_FEATURE(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
  DN_AssertF(DN_IsPowerOfTwoAligned(ptr, 8),
             "Poisoning requires the pointer to be aligned on an 8 byte boundary");

  __asan_poison_memory_region(ptr, size);
  if (DN_ASAN_VET_POISON) {
    DN_HardAssert(__asan_address_is_poisoned(ptr));
    DN_HardAssert(__asan_address_is_poisoned((char *)ptr + (size - 1)));
  }
#else
  (void)ptr;
  (void)size;
#endif
}

DN_API void DN_ASanUnpoisonMemoryRegion(void const volatile *ptr, DN_USize size)
{
  if (!ptr || !size)
    return;

#if DN_HAS_FEATURE(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
  __asan_unpoison_memory_region(ptr, size);
  if (DN_ASAN_VET_POISON)
    DN_HardAssert(__asan_region_is_poisoned((void *)ptr, size) == 0);
#else
  (void)ptr;
  (void)size;
#endif
}

DN_API DN_F32 DN_EpsilonClampF32(DN_F32 value, DN_F32 target, DN_F32 epsilon)
{
  DN_F32 delta  = DN_Abs(target - value);
  DN_F32 result = (delta < epsilon) ? target : value;
  return result;
}

static DN_ArenaBlock *DN_ArenaBlockFromMemFuncs_(DN_U64 reserve, DN_U64 commit, bool track_alloc, bool alloc_can_leak, DN_ArenaMemFuncs mem_funcs)
{
  DN_ArenaBlock *result = nullptr;
  switch (mem_funcs.type) {
    case DN_ArenaMemFuncType_Nil:
      break;

    case DN_ArenaMemFuncType_Basic: {
      DN_AssertF(reserve > DN_ARENA_HEADER_SIZE, "%I64u > %I64u", reserve, DN_ARENA_HEADER_SIZE);
      result = DN_Cast(DN_ArenaBlock *) mem_funcs.basic_alloc(reserve);
      if (!result)
        return result;

      result->used    = DN_ARENA_HEADER_SIZE;
      result->commit  = reserve;
      result->reserve = reserve;
    } break;

    case DN_ArenaMemFuncType_VMem: {
      DN_AssertF(mem_funcs.vmem_page_size, "Page size must be set to a non-zero, power of two value");
      DN_Assert(DN_IsPowerOfTwo(mem_funcs.vmem_page_size));

      DN_USize const page_size    = mem_funcs.vmem_page_size;
      DN_U64         real_reserve = reserve ? reserve : DN_ARENA_RESERVE_SIZE;
      DN_U64         real_commit  = commit ? commit   : DN_ARENA_COMMIT_SIZE;
      real_reserve                = DN_AlignUpPowerOfTwo(real_reserve, page_size);
      real_commit                 = DN_Min(DN_AlignUpPowerOfTwo(real_commit, page_size), real_reserve);
      DN_AssertF(DN_ARENA_HEADER_SIZE < real_commit && real_commit <= real_reserve, "%I64u < %I64u <= %I64u", DN_ARENA_HEADER_SIZE, real_commit, real_reserve);

      DN_MemCommit mem_commit = real_reserve == real_commit ? DN_MemCommit_Yes : DN_MemCommit_No;
      result                    = DN_Cast(DN_ArenaBlock *) mem_funcs.vmem_reserve(real_reserve, mem_commit, DN_MemPage_ReadWrite);
      if (!result)
        return result;

      if (mem_commit == DN_MemCommit_No && !mem_funcs.vmem_commit(result, real_commit, DN_MemPage_ReadWrite)) {
        mem_funcs.vmem_release(result, real_reserve);
        return result;
      }

      result->used    = DN_ARENA_HEADER_SIZE;
      result->commit  = real_commit;
      result->reserve = real_reserve;
    } break;
  }

  if (track_alloc && result)
    DN_LeakTrackAlloc(dn->leak, result, result->reserve, alloc_can_leak);

  return result;
}

static DN_ArenaBlock *DN_ArenaBlockFlagsFromMemFuncs_(DN_U64 reserve, DN_U64 commit, DN_ArenaFlags flags, DN_ArenaMemFuncs mem_funcs)
{
  bool           track_alloc    = (flags & DN_ArenaFlags_NoAllocTrack) == 0;
  bool           alloc_can_leak = flags & DN_ArenaFlags_AllocCanLeak;
  DN_ArenaBlock *result         = DN_ArenaBlockFromMemFuncs_(reserve, commit, track_alloc, alloc_can_leak, mem_funcs);
  if (result && ((flags & DN_ArenaFlags_NoPoison) == 0))
    DN_ASanPoisonMemoryRegion(DN_Cast(char *) result + DN_ARENA_HEADER_SIZE, result->commit - DN_ARENA_HEADER_SIZE);
  return result;
}

static void DN_ArenaUpdateStatsOnNewBlock_(DN_Arena *arena, DN_ArenaBlock const *block)
{
  DN_Assert(arena);
  if (block) {
    arena->stats.info.used    += block->used;
    arena->stats.info.commit  += block->commit;
    arena->stats.info.reserve += block->reserve;
    arena->stats.info.blocks  += 1;

    arena->stats.hwm.used    = DN_Max(arena->stats.hwm.used,    arena->stats.info.used);
    arena->stats.hwm.commit  = DN_Max(arena->stats.hwm.commit,  arena->stats.info.commit);
    arena->stats.hwm.reserve = DN_Max(arena->stats.hwm.reserve, arena->stats.info.reserve);
    arena->stats.hwm.blocks  = DN_Max(arena->stats.hwm.blocks,  arena->stats.info.blocks);
  }
}

DN_API DN_Arena DN_ArenaFromBuffer(void *buffer, DN_USize size, DN_ArenaFlags flags)
{
  DN_Assert(buffer);
  DN_AssertF(DN_ARENA_HEADER_SIZE < size, "Buffer (%zu bytes) too small, need atleast %zu bytes to store arena metadata", size, DN_ARENA_HEADER_SIZE);
  DN_AssertF(DN_IsPowerOfTwo(size), "Buffer (%zu bytes) must be a power-of-two", size);

  // NOTE: Init block
  DN_ArenaBlock *block = DN_Cast(DN_ArenaBlock *) buffer;
  block->commit        = size;
  block->reserve       = size;
  block->used          = DN_ARENA_HEADER_SIZE;
  if (block && ((flags & DN_ArenaFlags_NoPoison) == 0))
    DN_ASanPoisonMemoryRegion(DN_Cast(char *) block + DN_ARENA_HEADER_SIZE, block->commit - DN_ARENA_HEADER_SIZE);

  DN_Arena result = {};
  result.flags    = flags | DN_ArenaFlags_NoGrow | DN_ArenaFlags_NoAllocTrack | DN_ArenaFlags_AllocCanLeak | DN_ArenaFlags_UserBuffer;
  result.curr     = block;
  DN_ArenaUpdateStatsOnNewBlock_(&result, result.curr);
  return result;
}

DN_API DN_Arena DN_ArenaFromMemFuncs(DN_U64 reserve, DN_U64 commit, DN_ArenaFlags flags, DN_ArenaMemFuncs mem_funcs)
{
  DN_Arena result   = {};
  result.flags      = flags;
  result.mem_funcs  = mem_funcs;
  result.flags     |= DN_ArenaFlags_MemFuncs;
  result.curr       = DN_ArenaBlockFlagsFromMemFuncs_(reserve, commit, flags, mem_funcs);
  DN_ArenaUpdateStatsOnNewBlock_(&result, result.curr);
  return result;
}

static void DN_ArenaBlockDeinit_(DN_Arena const *arena, DN_ArenaBlock *block)
{
  DN_USize release_size = block->reserve;
  if (DN_BitIsNotSet(arena->flags, DN_ArenaFlags_NoAllocTrack))
    DN_LeakTrackDealloc(&g_dn_->leak, block);
  DN_ASanUnpoisonMemoryRegion(block, block->commit);
  if (arena->flags & DN_ArenaFlags_MemFuncs) {
    if (arena->mem_funcs.type == DN_ArenaMemFuncType_Basic)
      arena->mem_funcs.basic_dealloc(block);
    else
      arena->mem_funcs.vmem_release(block, release_size);
  }
}

DN_API void DN_ArenaDeinit(DN_Arena *arena)
{
  for (DN_ArenaBlock *block = arena ? arena->curr : nullptr; block;) {
    DN_ArenaBlock *block_to_free = block;
    block                        = block->prev;
    DN_ArenaBlockDeinit_(arena, block_to_free);
  }
  if (arena)
    *arena = {};
}

DN_API bool DN_ArenaCommitTo(DN_Arena *arena, DN_U64 pos)
{
  if (!arena || !arena->curr)
    return false;

  DN_ArenaBlock *curr = arena->curr;
  if (pos <= curr->commit)
    return true;

  DN_U64 real_pos = pos;
  if (!DN_Check(pos <= curr->reserve))
    real_pos = curr->reserve;

  DN_Assert(arena->mem_funcs.vmem_page_size);
  DN_USize end_commit  = DN_AlignUpPowerOfTwo(real_pos, arena->mem_funcs.vmem_page_size);
  DN_USize commit_size = end_commit - curr->commit;
  char    *commit_ptr  = DN_Cast(char *) curr + curr->commit;
  if (!arena->mem_funcs.vmem_commit(commit_ptr, commit_size, DN_MemPage_ReadWrite))
    return false;

  bool poison = DN_ASAN_POISON && ((arena->flags & DN_ArenaFlags_NoPoison) == 0);
  if (poison)
    DN_ASanPoisonMemoryRegion(commit_ptr, commit_size);

  curr->commit = end_commit;
  return true;
}

DN_API bool DN_ArenaCommit(DN_Arena *arena, DN_U64 size)
{
  if (!arena || !arena->curr)
    return false;
  DN_U64 pos    = DN_Min(arena->curr->reserve, arena->curr->commit + size);
  bool   result = DN_ArenaCommitTo(arena, pos);
  return result;
}

DN_API bool DN_ArenaGrow(DN_Arena *arena, DN_U64 reserve, DN_U64 commit)
{
  if (arena->flags & (DN_ArenaFlags_NoGrow | DN_ArenaFlags_UserBuffer))
    return false;

  bool result = false;
  DN_ArenaBlock *new_block = DN_ArenaBlockFlagsFromMemFuncs_(reserve, commit, arena->flags, arena->mem_funcs);
  if (new_block) {
    result                      = true;
    new_block->prev             = arena->curr;
    arena->curr                 = new_block;
    new_block->reserve_sum      = new_block->prev->reserve_sum + new_block->prev->reserve;
    DN_ArenaUpdateStatsOnNewBlock_(arena, arena->curr);
  }
  return result;
}

DN_API void *DN_ArenaAlloc(DN_Arena *arena, DN_U64 size, uint8_t align, DN_ZMem z_mem)
{
  if (!arena)
    return nullptr;

  if (!arena->curr) {
    arena->curr = DN_ArenaBlockFlagsFromMemFuncs_(DN_ARENA_RESERVE_SIZE, DN_ARENA_COMMIT_SIZE, arena->flags, arena->mem_funcs);
    DN_ArenaUpdateStatsOnNewBlock_(arena, arena->curr);
  }

  if (!arena->curr)
    return nullptr;

  try_alloc_again:
  DN_ArenaBlock *curr       = arena->curr;
  bool           poison     = DN_ASAN_POISON && ((arena->flags & DN_ArenaFlags_NoPoison) == 0);
  uint8_t        real_align = poison ? DN_Max(align, DN_ASAN_POISON_ALIGNMENT) : align;
  DN_U64         offset_pos = DN_AlignUpPowerOfTwo(curr->used, real_align) + (poison ? DN_ASAN_POISON_GUARD_SIZE : 0);
  DN_U64         end_pos    = offset_pos + size;
  DN_U64         alloc_size = end_pos - curr->used;

  if (end_pos > curr->reserve) {
    if (arena->flags & (DN_ArenaFlags_NoGrow | DN_ArenaFlags_UserBuffer))
      return nullptr;
    DN_USize new_reserve = DN_Max(DN_ARENA_HEADER_SIZE + alloc_size, DN_ARENA_RESERVE_SIZE);
    DN_USize new_commit  = DN_Max(DN_ARENA_HEADER_SIZE + alloc_size, DN_ARENA_COMMIT_SIZE);
    if (!DN_ArenaGrow(arena, new_reserve, new_commit))
      return nullptr;
    goto try_alloc_again;
  }

  DN_USize prev_arena_commit = curr->commit;
  if (end_pos > curr->commit) {
    DN_Assert(arena->mem_funcs.vmem_page_size);
    DN_Assert(arena->mem_funcs.type == DN_ArenaMemFuncType_VMem);
    DN_Assert((arena->flags & DN_ArenaFlags_UserBuffer) == 0);
    DN_USize end_commit  = DN_AlignUpPowerOfTwo(end_pos, arena->mem_funcs.vmem_page_size);
    DN_USize commit_size = end_commit - curr->commit;
    char    *commit_ptr  = DN_Cast(char *) curr + curr->commit;
    if (!arena->mem_funcs.vmem_commit(commit_ptr, commit_size, DN_MemPage_ReadWrite))
      return nullptr;
    if (poison)
      DN_ASanPoisonMemoryRegion(commit_ptr, commit_size);
    curr->commit              = end_commit;
    arena->stats.info.commit += commit_size;
    arena->stats.hwm.commit   = DN_Max(arena->stats.hwm.commit, arena->stats.info.commit);
  }

  void *result            = DN_Cast(char *) curr + offset_pos;
  curr->used             += alloc_size;
  arena->stats.info.used += alloc_size;
  arena->stats.hwm.used   = DN_Max(arena->stats.hwm.used, arena->stats.info.used);
  DN_ASanUnpoisonMemoryRegion(result, size);

  if (z_mem == DN_ZMem_Yes) {
    DN_USize reused_bytes = DN_Min(prev_arena_commit - offset_pos, size);
    DN_Memset(result, 0, reused_bytes);
  }

  DN_Assert(arena->stats.hwm.used >= arena->stats.info.used);
  DN_Assert(arena->stats.hwm.commit >= arena->stats.info.commit);
  DN_Assert(arena->stats.hwm.reserve >= arena->stats.info.reserve);
  DN_Assert(arena->stats.hwm.blocks >= arena->stats.info.blocks);
  return result;
}

DN_API void *DN_ArenaAllocContiguous(DN_Arena *arena, DN_U64 size, uint8_t align, DN_ZMem z_mem)
{
  DN_ArenaFlags prev_flags  = arena->flags;
  arena->flags             |= (DN_ArenaFlags_NoGrow | DN_ArenaFlags_NoPoison);
  void *memory              = DN_ArenaAlloc(arena, size, align, z_mem);
  arena->flags              = prev_flags;
  return memory;
}

DN_API void *DN_ArenaCopy(DN_Arena *arena, void const *data, DN_U64 size, uint8_t align)
{
  if (!arena || !data || size == 0)
    return nullptr;
  void *result = DN_ArenaAlloc(arena, size, align, DN_ZMem_No);
  if (result)
    DN_Memcpy(result, data, size);
  return result;
}

DN_API void DN_ArenaPopTo(DN_Arena *arena, DN_U64 init_used)
{
  if (!arena || !arena->curr)
    return;
  DN_U64         used = DN_Max(DN_ARENA_HEADER_SIZE, init_used);
  DN_ArenaBlock *curr = arena->curr;
  while (curr->reserve_sum >= used) {
    DN_ArenaBlock *block_to_free = curr;
    arena->stats.info.used    -= block_to_free->used;
    arena->stats.info.commit  -= block_to_free->commit;
    arena->stats.info.reserve -= block_to_free->reserve;
    arena->stats.info.blocks  -= 1;
    if (arena->flags & DN_ArenaFlags_UserBuffer)
      break;
    curr = curr->prev;
    DN_ArenaBlockDeinit_(arena, block_to_free);
  }

  arena->stats.info.used -= curr->used;
  arena->curr          = curr;
  curr->used           = used - curr->reserve_sum;
  char    *poison_ptr  = (char *)curr + DN_AlignUpPowerOfTwo(curr->used, DN_ASAN_POISON_ALIGNMENT);
  DN_USize poison_size = ((char *)curr + curr->commit) - poison_ptr;
  DN_ASanPoisonMemoryRegion(poison_ptr, poison_size);
  arena->stats.info.used += curr->used;
}

DN_API void DN_ArenaPop(DN_Arena *arena, DN_U64 amount)
{
  DN_ArenaBlock *curr     = arena->curr;
  DN_USize       used_sum = curr->reserve_sum + curr->used;
  if (!DN_Check(amount <= used_sum))
    amount = used_sum;
  DN_USize pop_to = used_sum - amount;
  DN_ArenaPopTo(arena, pop_to);
}

DN_API DN_U64 DN_ArenaPos(DN_Arena const *arena)
{
  DN_U64 result = (arena && arena->curr) ? arena->curr->reserve_sum + arena->curr->used : 0;
  return result;
}

DN_API void DN_ArenaClear(DN_Arena *arena)
{
  DN_ArenaPopTo(arena, 0);
}

DN_API bool DN_ArenaOwnsPtr(DN_Arena const *arena, void *ptr)
{
  bool      result   = false;
  uintptr_t uint_ptr = DN_Cast(uintptr_t) ptr;
  for (DN_ArenaBlock const *block = arena ? arena->curr : nullptr; !result && block; block = block->prev) {
    uintptr_t begin = DN_Cast(uintptr_t) block + DN_ARENA_HEADER_SIZE;
    uintptr_t end   = begin + block->reserve;
    result          = uint_ptr >= begin && uint_ptr <= end;
  }
  return result;
}

DN_API DN_Str8x64 DN_ArenaInfoStr8x64(DN_ArenaInfo info)
{
  DN_Str8x64 result  = {};
  DN_Str8x32 used    = DN_ByteCountStr8x32(info.used);
  DN_Str8x32 commit  = DN_ByteCountStr8x32(info.commit);
  DN_Str8x32 reserve = DN_ByteCountStr8x32(info.reserve);
  result             = DN_Str8x64FromFmt("Blks/Used/Comm/Resv (%u/%.*s/%.*s/%.*s)", DN_Cast(DN_U32)info.blocks, DN_Str8PrintFmt(used), DN_Str8PrintFmt(commit), DN_Str8PrintFmt(reserve));
  return result;
}

DN_API DN_ArenaStats DN_ArenaSumStatsArray(DN_ArenaStats const *array, DN_USize size)
{
  DN_ArenaStats result = {};
  for (DN_ForItSize(it, DN_ArenaStats const, array, size)) {
    DN_ArenaStats stats = *it.data;
    result.info.used += stats.info.used;
    result.info.commit += stats.info.commit;
    result.info.reserve += stats.info.reserve;
    result.info.blocks += stats.info.blocks;

    result.hwm.used      = DN_Max(result.hwm.used, result.info.used);
    result.hwm.commit    = DN_Max(result.hwm.commit, result.info.commit);
    result.hwm.reserve   = DN_Max(result.hwm.reserve, result.info.reserve);
    result.hwm.blocks    = DN_Max(result.hwm.blocks, result.info.blocks);
  }
  return result;
}

DN_API DN_ArenaStats DN_ArenaSumStats(DN_ArenaStats lhs, DN_ArenaStats rhs)
{
  DN_ArenaStats array[] = {lhs, rhs};
  DN_ArenaStats result  = DN_ArenaSumStatsArray(array, DN_ArrayCountU(array));
  return result;
}

DN_API DN_ArenaStats DN_ArenaSumArenaArrayToStats(DN_Arena const *array, DN_USize size)
{
  DN_ArenaStats result = {};
  for (DN_USize index = 0; index < size; index++) {
    DN_Arena const *arena = array + index;
    result                = DN_ArenaSumStats(result, arena->stats);
  }
  return result;
}

DN_API DN_ArenaTempMem DN_ArenaTempMemBegin(DN_Arena *arena)
{
  DN_ArenaTempMem result = {};
  if (arena) {
    DN_ArenaBlock *curr = arena->curr;
    result              = {arena, curr ? curr->reserve_sum + curr->used : 0};
  }
  return result;
};

DN_API void DN_ArenaTempMemEnd(DN_ArenaTempMem mem)
{
  DN_ArenaPopTo(mem.arena, mem.used_sum);
};

DN_ArenaTempMemScope::DN_ArenaTempMemScope(DN_Arena *arena)
{
  mem = DN_ArenaTempMemBegin(arena);
}

DN_ArenaTempMemScope::~DN_ArenaTempMemScope()
{
  DN_ArenaTempMemEnd(mem);
}

// NOTE: DN_Pool ///////////////////////////////////////////////////////////////////////////////////
DN_API DN_Pool DN_PoolFromArena(DN_Arena *arena, uint8_t align)
{
  DN_Pool result = {};
  if (arena) {
    result.arena = arena;
    result.align = align ? align : DN_POOL_DEFAULT_ALIGN;
  }
  return result;
}

DN_API bool DN_PoolIsValid(DN_Pool const *pool)
{
  bool result = pool && pool->arena && pool->align;
  return result;
}

DN_API void *DN_PoolAlloc(DN_Pool *pool, DN_USize size)
{
  void *result = nullptr;
  if (!DN_PoolIsValid(pool))
    return result;

  DN_USize const required_size       = sizeof(DN_PoolSlot) + pool->align + size;
  DN_USize const size_to_slot_offset = 5; // __lzcnt64(32) e.g. DN_PoolSlotSize_32B
  DN_USize       slot_index          = 0;
  if (required_size > 32) {
    // NOTE: Round up if not PoT as the low bits are set.
    DN_USize dist_to_next_msb = DN_CountLeadingZerosUSize(required_size) + 1;
    dist_to_next_msb -= DN_Cast(DN_USize)(!DN_IsPowerOfTwo(required_size));

    DN_USize const register_size = sizeof(DN_USize) * 8;
    DN_AssertF(register_size >= (dist_to_next_msb - size_to_slot_offset), "lhs=%zu, rhs=%zu", register_size, (dist_to_next_msb - size_to_slot_offset));
    slot_index = register_size - dist_to_next_msb - size_to_slot_offset;
  }

  if (!DN_CheckF(slot_index < DN_PoolSlotSize_Count, "Chunk pool does not support the requested allocation size"))
    return result;

  DN_USize slot_size_in_bytes = 1ULL << (slot_index + size_to_slot_offset);
  DN_AssertF(required_size <= (slot_size_in_bytes << 0), "slot_index=%zu, lhs=%zu, rhs=%zu", slot_index, required_size, (slot_size_in_bytes << 0));
  DN_AssertF(required_size >= (slot_size_in_bytes >> 1), "slot_index=%zu, lhs=%zu, rhs=%zu", slot_index, required_size, (slot_size_in_bytes >> 1));

  DN_PoolSlot *slot = nullptr;
  if (pool->slots[slot_index]) {
    slot                    = pool->slots[slot_index];
    pool->slots[slot_index] = slot->next;
    DN_Memset(slot->data, 0, size);
    DN_Assert(DN_IsPowerOfTwoAligned(slot->data, pool->align));
  } else {
    void *bytes = DN_ArenaAlloc(pool->arena, slot_size_in_bytes, alignof(DN_PoolSlot), DN_ZMem_Yes);
    slot        = DN_Cast(DN_PoolSlot *) bytes;

    // NOTE: The raw pointer is round up to the next 'pool->align'-ed
    // address ensuring at least 1 byte of padding between the raw pointer
    // and the pointer given to the user and that the user pointer is
    // aligned to the pool's alignment.
    //
    // This allows us to smuggle 1 byte behind the user pointer that has
    // the offset to the original pointer.
    slot->data = DN_Cast(void *) DN_AlignDownPowerOfTwo(DN_Cast(uintptr_t) slot + sizeof(DN_PoolSlot) + pool->align, pool->align);

    uintptr_t offset_to_original_ptr = DN_Cast(uintptr_t) slot->data - DN_Cast(uintptr_t) bytes;
    DN_Assert(slot->data > bytes);
    DN_Assert(offset_to_original_ptr <= sizeof(DN_PoolSlot) + pool->align);

    // NOTE: Store the offset to the original pointer behind the user's
    // pointer.
    char *offset_to_original_storage = DN_Cast(char *) slot->data - 1;
    DN_Memcpy(offset_to_original_storage, &offset_to_original_ptr, 1);
  }

  // NOTE: Smuggle the slot type in the next pointer so that we know, when the
  // pointer gets returned which free list to return the pointer to.
  result     = slot->data;
  slot->next = DN_Cast(DN_PoolSlot *) slot_index;
  return result;
}

DN_API void DN_PoolDealloc(DN_Pool *pool, void *ptr)
{
  if (!DN_PoolIsValid(pool) || !ptr)
    return;

  DN_Assert(DN_ArenaOwnsPtr(pool->arena, ptr));

  char const *one_byte_behind_ptr    = DN_Cast(char *) ptr - 1;
  DN_USize    offset_to_original_ptr = 0;
  DN_Memcpy(&offset_to_original_ptr, one_byte_behind_ptr, 1);
  DN_Assert(offset_to_original_ptr <= sizeof(DN_PoolSlot) + pool->align);

  char           *original_ptr = DN_Cast(char *) ptr - offset_to_original_ptr;
  DN_PoolSlot    *slot         = DN_Cast(DN_PoolSlot *) original_ptr;
  DN_PoolSlotSize slot_index   = DN_Cast(DN_PoolSlotSize)(DN_Cast(uintptr_t) slot->next);
  DN_Assert(slot_index < DN_PoolSlotSize_Count);

  slot->next              = pool->slots[slot_index];
  pool->slots[slot_index] = slot;
}

DN_API void *DN_PoolCopy(DN_Pool *pool, void const *data, DN_U64 size, uint8_t align)
{
  if (!pool || !data || size == 0)
    return nullptr;

  // TODO: Hmm should align be part of the alloc interface in general? I'm not going to worry
  // about this until we crash because of misalignment.
  DN_Assert(pool->align >= align);

  void *result = DN_PoolAlloc(pool, size);
  if (result)
    DN_Memcpy(result, data, size);
  return result;
}

DN_API bool DN_CharIsAlphabet(char ch)
{
  bool result = (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
  return result;
}

DN_API bool DN_CharIsDigit(char ch)
{
  bool result = (ch >= '0' && ch <= '9');
  return result;
}

DN_API bool DN_CharIsAlphaNum(char ch)
{
  bool result = DN_CharIsAlphabet(ch) || DN_CharIsDigit(ch);
  return result;
}

DN_API bool DN_CharIsWhitespace(char ch)
{
  bool result = (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r');
  return result;
}

DN_API bool DN_CharIsHex(char ch)
{
  bool result = ((ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F') || (ch >= '0' && ch <= '9'));
  return result;
}

DN_API char DN_CharToLower(char ch)
{
  char result = ch;
  if (result >= 'A' && result <= 'Z')
    result += 'a' - 'A';
  return result;
}

DN_API char DN_CharToUpper(char ch)
{
  char result = ch;
  if (result >= 'a' && result <= 'z')
    result -= 'a' - 'A';
  return result;
}

DN_API DN_U64FromResult DN_U64FromStr8(DN_Str8 string, char separator)
{
  // NOTE: Argument check
  DN_U64FromResult result = {};
  if (string.size == 0) {
    result.success = true;
    return result;
  }

  // NOTE: Sanitize input/output
  DN_Str8 trim_string = DN_Str8TrimWhitespaceAround(string);
  if (trim_string.size == 0) {
    result.success = true;
    return result;
  }

  // NOTE: Handle prefix '+'
  DN_USize start_index = 0;
  if (!DN_CharIsDigit(trim_string.data[0])) {
    if (trim_string.data[0] != '+')
      return result;
    start_index++;
  }

  // NOTE: Convert the string number to the binary number
  for (DN_USize index = start_index; index < trim_string.size; index++) {
    char ch = trim_string.data[index];
    if (index) {
      if (separator != 0 && ch == separator)
        continue;
    }

    if (!DN_CharIsDigit(ch))
      return result;

    result.value   = DN_SafeMulU64(result.value, 10);
    uint64_t digit = ch - '0';
    result.value   = DN_SafeAddU64(result.value, digit);
  }

  result.success = true;
  return result;
}

DN_API DN_U64FromResult DN_U64FromPtr(void const *data, DN_USize size, char separator)
{
  DN_Str8          str8   = DN_Str8FromPtr((char *)data, size);
  DN_U64FromResult result = DN_U64FromStr8(str8, separator);
  return result;
}

DN_API DN_U64 DN_U64FromPtrUnsafe(void const *data, DN_USize size, char separator)
{
  DN_U64FromResult from   = DN_U64FromPtr(data, size, separator);
  DN_U64           result = from.value;
  DN_Assert(from.success);
  return result;
}

DN_API DN_U64FromResult DN_U64FromHexPtr(void const *hex, DN_USize hex_count)
{
  char *hex_ptr = DN_Cast(char *) hex;
  if (hex_count >= 2 && hex_ptr[0] == '0' && (hex_ptr[1] == 'x' || hex_ptr[1] == 'X')) {
      hex_ptr += 2;
      hex_count -= 2;
  }

  DN_U64FromResult result        = {};
  DN_USize         max_hex_count = sizeof(DN_U64) * 2;
  DN_USize         count         = DN_Min(max_hex_count, hex_count);
  DN_Assert(hex_count <= max_hex_count);
  for (DN_USize index = 0; index < count; index++) {
    char  ch  = hex_ptr[index];
    DN_U8 val = DN_U8FromHexNibble(ch);
    if (val == 0xFF)
      return result;
    result.value = (result.value << 4) | val;
  }
  result.success = true;
  return result;
}

DN_API DN_U64 DN_U64FromHexPtrUnsafe(void const *hex, DN_USize hex_count)
{
  DN_U64FromResult from   = DN_U64FromHexPtr(hex, hex_count);
  DN_U64           result = from.value;
  DN_Assert(from.success);
  return result;
}

DN_API DN_U64FromResult DN_U64FromHexStr8(DN_Str8 hex)
{
  DN_U64FromResult result = DN_U64FromHexPtr(hex.data, hex.size);
  return result;
}

DN_API DN_U64 DN_U64FromHexStr8Unsafe(DN_Str8 hex)
{
  DN_U64 result = DN_U64FromHexPtrUnsafe(hex.data, hex.size);
  return result;
}

DN_API DN_I64FromResult DN_I64FromStr8(DN_Str8 string, char separator)
{
  // NOTE: Argument check
  DN_I64FromResult result = {};
  if (string.size == 0) {
    result.success = true;
    return result;
  }

  // NOTE: Sanitize input/output
  DN_Str8 trim_string = DN_Str8TrimWhitespaceAround(string);
  if (trim_string.size == 0) {
    result.success = true;
    return result;
  }

  bool     negative    = false;
  DN_USize start_index = 0;
  if (!DN_CharIsDigit(trim_string.data[0])) {
    negative = (trim_string.data[start_index] == '-');
    if (!negative && trim_string.data[0] != '+')
      return result;
    start_index++;
  }

  // NOTE: Convert the string number to the binary number
  for (DN_USize index = start_index; index < trim_string.size; index++) {
    char ch = trim_string.data[index];
    if (index) {
      if (separator != 0 && ch == separator)
        continue;
    }

    if (!DN_CharIsDigit(ch))
      return result;

    result.value   = DN_SafeMulU64(result.value, 10);
    uint64_t digit = ch - '0';
    result.value   = DN_SafeAddU64(result.value, digit);
  }

  if (negative)
    result.value *= -1;

  result.success = true;
  return result;
}

DN_API DN_I64FromResult DN_I64FromPtr(void const *data, DN_USize size, char separator)
{
  DN_Str8          str8   = DN_Str8FromPtr((char *)data, size);
  DN_I64FromResult result = DN_I64FromStr8(str8, separator);
  return result;
}

DN_API DN_I64 DN_I64FromPtrUnsafe(void const *data, DN_USize size, char separator)
{
  DN_I64FromResult from   = DN_I64FromPtr(data, size, separator);
  DN_I64           result = from.value;
  DN_Assert(from.success);
  return result;
}

DN_API DN_FmtAppendResult DN_FmtVAppend(char *buf, DN_USize *buf_size, DN_USize buf_max, char const *fmt, va_list args)
{
  DN_FmtAppendResult result  = {};
  result.size_req            = DN_VSNPrintF(buf + *buf_size, DN_Cast(int)(buf_max - *buf_size), fmt, args);
  *buf_size                 += result.size_req;
  if (*buf_size >= (buf_max - 1))
    *buf_size = buf_max - 1;
  DN_Assert(*buf_size <= (buf_max - 1));
  result.str8      = DN_Str8FromPtr(buf, *buf_size);
  result.truncated = result.str8.size != result.size_req;
  return result;
}

DN_API DN_FmtAppendResult DN_FmtAppend(char *buf, DN_USize *buf_size, DN_USize buf_max, char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_FmtAppendResult result = DN_FmtVAppend(buf, buf_size, buf_max - (*buf_size), fmt, args);
  va_end(args);
  return result;
}

DN_API DN_FmtAppendResult DN_FmtAppendTruncate(char *buf, DN_USize *buf_size, DN_USize buf_max, DN_Str8 truncator, char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_FmtAppendResult result = DN_FmtVAppend(buf, buf_size, buf_max, fmt, args);
  if (result.truncated)
    DN_Memcpy(result.str8.data + result.str8.size - truncator.size, truncator.data, truncator.size);
  va_end(args);
  return result;
}

DN_API DN_USize DN_FmtSize(DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_USize result = DN_VSNPrintF(nullptr, 0, fmt, args);
  va_end(args);
  return result;
}

DN_API DN_USize DN_FmtVSize(DN_FMT_ATTRIB char const *fmt, va_list args)
{
  va_list args_copy;
  va_copy(args_copy, args);
  DN_USize result = DN_VSNPrintF(nullptr, 0, fmt, args_copy);
  va_end(args_copy);
  return result;
}

DN_API DN_USize DN_CStr8Size(char const *src)
{
  DN_USize result = 0;
  for (; src && src[0] != 0; src++, result++)
    ;
  return result;
}

DN_API DN_USize DN_CStr16Size(wchar_t const *src)
{
  DN_USize result = 0;
  for (; src && src[0] != 0; src++, result++)
    ;
  return result;
}

DN_API bool DN_Str16Eq(DN_Str16 lhs, DN_Str16 rhs)
{
  if (lhs.size != rhs.size)
      return false;
  bool result = (DN_Memcmp(lhs.data, rhs.data, lhs.size) == 0);
  return result;
}

DN_API DN_Str8 DN_Str8FromCStr8(char const *src)
{
  DN_USize size   = DN_CStr8Size(src);
  DN_Str8  result = DN_Str8FromPtr(src, size);
  return result;
}

DN_API DN_Str8 DN_Str8FromArena(DN_Arena *arena, DN_USize size, DN_ZMem z_mem)
{
  DN_Str8 result = {};
  result.data    = DN_ArenaNewArray(arena, char, size + 1, z_mem);
  if (result.data)
    result.size = size;
  result.data[result.size] = 0;
  return result;
}

DN_API DN_Str8 DN_Str8FromPool(DN_Pool *pool, DN_USize size)
{
  DN_Str8 result = {};
  result.data    = DN_PoolNewArray(pool, char, size + 1);
  if (result.data)
    result.size = size;
  result.data[result.size] = 0;
  return result;
}

DN_API DN_Str8 DN_Str8FromPtrArena(DN_Arena *arena, void const *data, DN_USize size)
{
  DN_Str8 result = DN_Str8FromArena(arena, size, DN_ZMem_No);
  if (result.size)
    DN_Memcpy(result.data, data, size);
  return result;
}

DN_API DN_Str8 DN_Str8FromPtrPool(DN_Pool *pool, void const *data, DN_USize size)
{
  DN_Str8 result = DN_Str8FromPool(pool, size);
  if (result.size)
    DN_Memcpy(result.data, data, size);
  return result;
}

DN_API DN_Str8 DN_Str8FromStr8Arena(DN_Arena *arena, DN_Str8 string)
{
  DN_Str8 result = {};
  result.data    = DN_Cast(char *) DN_ArenaAlloc(arena, string.size + 1, alignof(char), DN_ZMem_No);
  if (result.data) {
    DN_Memcpy(result.data, string.data, string.size);
    result.data[string.size] = 0;
    result.size              = string.size;
  }
  return result;
}

DN_API DN_Str8 DN_Str8FromStr8Pool(DN_Pool *pool, DN_Str8 string)
{
  DN_Str8 result = {};
  result.data    = DN_Cast(char *) DN_PoolAlloc(pool, string.size + 1);
  if (result.data) {
    DN_Memcpy(result.data, string.data, string.size);
    result.data[string.size] = 0;
    result.size              = string.size;
  }
  return result;
}

DN_API DN_Str8 DN_Str8FromFmtArena(DN_Arena *arena, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  DN_Str8 result = DN_Str8FromFmtVArena(arena, fmt, va);
  va_end(va);
  return result;
}

DN_API DN_Str8 DN_Str8FromFmtVArena(DN_Arena *arena, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_USize size   = DN_FmtVSize(fmt, args);
  DN_Str8  result = DN_Str8FromArena(arena, size, DN_ZMem_No);
  if (result.data) {
    DN_USize written = 0;
    DN_FmtVAppend(result.data, &written, result.size + 1, fmt, args);
    DN_Assert(written == result.size);
  }
  return result;
}

DN_API DN_Str8 DN_Str8FromFmtPool(DN_Pool *pool, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_USize size   = DN_FmtVSize(fmt, args);
  DN_Str8  result = DN_Str8FromPool(pool, size);
  if (result.data) {
    DN_USize written = 0;
    DN_FmtVAppend(result.data, &written, result.size + 1, fmt, args);
    DN_Assert(written == result.size);
  }
  va_end(args);
  return result;
}

DN_API DN_Str8x32 DN_Str8x32FromFmt(DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8x32 result = {};
  DN_FmtVAppend(result.data, &result.size, sizeof(result.data), fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8x64 DN_Str8x64FromFmt(DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8x64 result = {};
  DN_FmtVAppend(result.data, &result.size, sizeof(result.data), fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8x128 DN_Str8x128FromFmt(DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8x128 result = {};
  DN_FmtVAppend(result.data, &result.size, sizeof(result.data), fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8x256 DN_Str8x256FromFmt(DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8x256 result = {};
  DN_FmtVAppend(result.data, &result.size, sizeof(result.data), fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8x32 DN_Str8x32FromU64(DN_U64 val, char separator)
{
  DN_Str8x32 result     = {};
  DN_Str8x32 temp       = DN_Str8x32FromFmt("%" PRIu64, val);
  DN_USize   temp_index = 0;

  // NOTE: Write the digits the first, up to [0, 2] digits that do not need a thousandth separator
  DN_USize   range_without_separator = temp.size % 3;
  for (; temp_index < range_without_separator; temp_index++)
    result.data[result.size++] = temp.data[temp_index];

  // NOTE: Write the subsequent digits and every 3rd digit, add the seperator
  DN_USize   digit_counter = 0;
  for (; temp_index < temp.size; temp_index++, digit_counter++) {
    if (separator && temp_index && (digit_counter % 3 == 0))
      result.data[result.size++] = separator;
    result.data[result.size++] = temp.data[temp_index];
  }
  return result;
}


DN_API bool DN_Str8IsAll(DN_Str8 string, DN_Str8IsAllType is_all)
{
  bool result = string.size;
  if (!result)
    return result;

  switch (is_all) {
    case DN_Str8IsAllType_Digits: {
      for (DN_USize index = 0; result && index < string.size; index++)
        result = string.data[index] >= '0' && string.data[index] <= '9';
    } break;

    case DN_Str8IsAllType_Hex: {
      DN_Str8 trimmed = DN_Str8TrimPrefix(string, DN_Str8Lit("0x"), DN_Str8EqCase_Insensitive);
      for (DN_USize index = 0; result && index < trimmed.size; index++) {
        char ch = trimmed.data[index];
        result  = (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
      }
    } break;
  }

  return result;
}

DN_API char *DN_Str8End(DN_Str8 string)
{
  char *result = string.data + string.size;
  return result;
}

DN_API DN_Str8 DN_Str8Slice(DN_Str8 string, DN_USize offset, DN_USize size)
{
  DN_Str8 result = DN_Str8FromPtr(string.data, 0);
  if (string.size == 0)
    return result;

  DN_USize capped_offset = DN_Min(offset, string.size);
  DN_USize max_size      = string.size - capped_offset;
  DN_USize capped_size   = DN_Min(size, max_size);
  result                 = DN_Str8FromPtr(string.data + capped_offset, capped_size);
  return result;
}

DN_API DN_Str8 DN_Str8Advance(DN_Str8 string, DN_USize amount)
{
  DN_Str8 result = DN_Str8Slice(string, amount, DN_USIZE_MAX);
  return result;
}

DN_API DN_Str8 DN_Str8NextLine(DN_Str8 string)
{
  DN_Str8 result = DN_Str8BSplit(string, DN_Str8Lit("\n")).rhs;
  return result;
}

DN_API DN_Str8BSplitResult DN_Str8BSplitArray(DN_Str8 string, DN_Str8 const *find, DN_USize find_size)
{
  DN_Str8BSplitResult result = {};
  if (string.size == 0 || !find || find_size == 0)
    return result;

  result.lhs = string;
  for (size_t index = 0; !result.rhs.data && index < string.size; index++) {
    for (DN_USize find_index = 0; find_index < find_size; find_index++) {
      DN_Str8 find_item    = find[find_index];
      DN_Str8 string_slice = DN_Str8Slice(string, index, find_item.size);
      if (DN_Str8Eq(string_slice, find_item)) {
        result.lhs.size = index;
        result.rhs.data = string_slice.data + find_item.size;
        result.rhs.size = string.size - (index + find_item.size);
        break;
      }
    }
  }

  return result;
}

DN_API DN_Str8BSplitResult DN_Str8BSplit(DN_Str8 string, DN_Str8 find)
{
  DN_Str8BSplitResult result = DN_Str8BSplitArray(string, &find, 1);
  return result;
}

DN_API DN_Str8BSplitResult DN_Str8BSplitLastArray(DN_Str8 string, DN_Str8 const *find, DN_USize find_size)
{
  DN_Str8BSplitResult result = {};
  if (string.size == 0 || !find || find_size == 0)
    return result;

  result.lhs = string;
  for (size_t index = string.size - 1; !result.rhs.data && index < string.size; index--) {
    for (DN_USize find_index = 0; find_index < find_size; find_index++) {
      DN_Str8 find_item    = find[find_index];
      DN_Str8 string_slice = DN_Str8Slice(string, index, find_item.size);
      if (DN_Str8Eq(string_slice, find_item)) {
        result.lhs.size = index;
        result.rhs.data = string_slice.data + find_item.size;
        result.rhs.size = string.size - (index + find_item.size);
        break;
      }
    }
  }

  return result;
}

DN_API DN_Str8BSplitResult DN_Str8BSplitLast(DN_Str8 string, DN_Str8 find)
{
  DN_Str8BSplitResult result = DN_Str8BSplitLastArray(string, &find, 1);
  return result;
}

DN_API DN_USize DN_Str8Split(DN_Str8 string, DN_Str8 delimiter, DN_Str8 *splits, DN_USize splits_count, DN_Str8SplitIncludeEmptyStrings mode)
{
  DN_USize result = 0; // The number of splits in the actual string.
  if (string.size == 0 || delimiter.size == 0 || delimiter.size <= 0)
    return result;

  DN_Str8BSplitResult split = {};
  DN_Str8                  first = string;
  do {
    split = DN_Str8BSplit(first, delimiter);
    if (split.lhs.size || mode == DN_Str8SplitIncludeEmptyStrings_Yes) {
      if (splits && result < splits_count)
        splits[result] = split.lhs;
      result++;
    }
    first = split.rhs;
  } while (first.size);

  return result;
}

DN_API DN_Str8SplitResult DN_Str8SplitArena(DN_Arena *arena, DN_Str8 string, DN_Str8 delimiter, DN_Str8SplitIncludeEmptyStrings mode)
{
  DN_Str8SplitResult result = {};
  DN_USize           count  = DN_Str8Split(string, delimiter, /*splits*/ nullptr, /*count*/ 0, mode);
  result.data               = DN_ArenaNewArray(arena, DN_Str8, count, DN_ZMem_No);
  if (result.data) {
    result.count = DN_Str8Split(string, delimiter, result.data, count, mode);
    DN_Assert(count == result.count);
  }
  return result;
}

DN_API DN_Str8FindResult DN_Str8FindStr8Array(DN_Str8 string, DN_Str8 const *find, DN_USize find_size, DN_Str8EqCase eq_case)
{
  DN_Str8FindResult result = {};
  for (DN_USize index = 0; !result.found && index < string.size; index++) {
    for (DN_USize find_index = 0; find_index < find_size; find_index++) {
      DN_Str8 find_item    = find[find_index];
      DN_Str8 string_slice = DN_Str8Slice(string, index, find_item.size);
      if (DN_Str8Eq(string_slice, find_item, eq_case)) {
        result.found                        = true;
        result.index                        = index;
        result.start_to_before_match        = DN_Str8FromPtr(string.data, index);
        result.match                        = DN_Str8FromPtr(string.data + index, find_item.size);
        result.match_to_end_of_buffer       = DN_Str8FromPtr(result.match.data, string.size - index);
        result.after_match_to_end_of_buffer = DN_Str8Advance(result.match_to_end_of_buffer, find_item.size);
        break;
      }
    }
  }
  return result;
}

DN_API DN_Str8FindResult DN_Str8FindStr8(DN_Str8 string, DN_Str8 find, DN_Str8EqCase eq_case)
{
  DN_Str8FindResult result = DN_Str8FindStr8Array(string, &find, 1, eq_case);
  return result;
}

DN_API DN_Str8FindResult DN_Str8Find(DN_Str8 string, uint32_t flags)
{
  DN_Str8FindResult result = {};
  for (size_t index = 0; !result.found && index < string.size; index++) {
    result.found |= ((flags & DN_Str8FindFlag_Digit) && DN_CharIsDigit(string.data[index]));
    result.found |= ((flags & DN_Str8FindFlag_Alphabet) && DN_CharIsAlphabet(string.data[index]));
    result.found |= ((flags & DN_Str8FindFlag_Whitespace) && DN_CharIsWhitespace(string.data[index]));
    result.found |= ((flags & DN_Str8FindFlag_Plus) && string.data[index] == '+');
    result.found |= ((flags & DN_Str8FindFlag_Minus) && string.data[index] == '-');
    if (result.found) {
      result.index                        = index;
      result.match                        = DN_Str8FromPtr(string.data + index, 1);
      result.match_to_end_of_buffer       = DN_Str8FromPtr(result.match.data, string.size - index);
      result.after_match_to_end_of_buffer = DN_Str8Advance(result.match_to_end_of_buffer, 1);
    }
  }
  return result;
}

DN_API DN_Str8 DN_Str8Segment(DN_Arena *arena, DN_Str8 src, DN_USize segment_size, char segment_char)
{
  if (!segment_size || src.size == 0) {
    DN_Str8 result = DN_Str8FromStr8Arena(arena, src);
    return result;
  }

  DN_USize segments = src.size / segment_size;
  if (src.size % segment_size == 0)
    segments--;

  DN_USize segment_counter = 0;
  DN_Str8  result          = DN_Str8FromArena(arena, src.size + segments, DN_ZMem_Yes);
  DN_USize write_index     = 0;
  for (DN_ForIndexU(src_index, src.size)) {
    result.data[write_index++] = src.data[src_index];
    if ((src_index + 1) % segment_size == 0 && segment_counter < segments) {
      result.data[write_index++] = segment_char;
      segment_counter++;
    }
    DN_AssertF(write_index <= result.size, "result.size=%zu, write_index=%zu", result.size, write_index);
  }

  DN_AssertF(write_index == result.size, "result.size=%zu, write_index=%zu", result.size, write_index);
  return result;
}

DN_API DN_Str8 DN_Str8ReverseSegment(DN_Arena *arena, DN_Str8 src, DN_USize segment_size, char segment_char)
{
  if (!segment_size || src.size == 0) {
    DN_Str8 result = DN_Str8FromStr8Arena(arena, src);
    return result;
  }

  DN_USize segments = src.size / segment_size;
  if (src.size % segment_size == 0)
    segments--;

  DN_USize write_counter   = 0;
  DN_USize segment_counter = 0;
  DN_Str8  result          = DN_Str8FromArena(arena, src.size + segments, DN_ZMem_Yes);
  DN_USize write_index     = result.size - 1;

  DN_MSVC_WARNING_PUSH
  DN_MSVC_WARNING_DISABLE(6293) // NOTE: Ill-defined loop
  for (size_t src_index = src.size - 1; src_index < src.size; src_index--) {
    DN_MSVC_WARNING_POP
    result.data[write_index--] = src.data[src_index];
    if (++write_counter % segment_size == 0 && segment_counter < segments) {
      result.data[write_index--] = segment_char;
      segment_counter++;
    }
  }

  DN_Assert(write_index == SIZE_MAX);
  return result;
}

DN_API bool DN_Str8Eq(DN_Str8 lhs, DN_Str8 rhs, DN_Str8EqCase eq_case)
{
  if (lhs.size != rhs.size)
    return false;
  bool result = true;
  switch (eq_case) {
    case DN_Str8EqCase_Sensitive: {
      result = (DN_Memcmp(lhs.data, rhs.data, lhs.size) == 0);
    } break;

    case DN_Str8EqCase_Insensitive: {
      for (DN_USize index = 0; index < lhs.size && result; index++)
        result = (DN_CharToLower(lhs.data[index]) == DN_CharToLower(rhs.data[index]));
    } break;
  }
  return result;
}

DN_API bool DN_Str8EqInsensitive(DN_Str8 lhs, DN_Str8 rhs)
{
  bool result = DN_Str8Eq(lhs, rhs, DN_Str8EqCase_Insensitive);
  return result;
}

DN_API bool DN_Str8StartsWith(DN_Str8 string, DN_Str8 prefix, DN_Str8EqCase eq_case)
{
  DN_Str8 substring = {string.data, DN_Min(prefix.size, string.size)};
  bool    result    = DN_Str8Eq(substring, prefix, eq_case);
  return result;
}

DN_API bool DN_Str8StartsWithInsensitive(DN_Str8 string, DN_Str8 prefix)
{
  bool result = DN_Str8StartsWith(string, prefix, DN_Str8EqCase_Insensitive);
  return result;
}

DN_API bool DN_Str8EndsWith(DN_Str8 string, DN_Str8 suffix, DN_Str8EqCase eq_case)
{
  DN_Str8 substring = {string.data + string.size - suffix.size, DN_Min(string.size, suffix.size)};
  bool    result    = DN_Str8Eq(substring, suffix, eq_case);
  return result;
}

DN_API bool DN_Str8EndsWithInsensitive(DN_Str8 string, DN_Str8 suffix)
{
  bool result = DN_Str8EndsWith(string, suffix, DN_Str8EqCase_Insensitive);
  return result;
}

DN_API bool DN_Str8HasChar(DN_Str8 string, char ch)
{
  bool result = false;
  for (DN_USize index = 0; !result && index < string.size; index++)
    result = string.data[index] == ch;
  return result;
}

DN_API DN_Str8 DN_Str8TrimPrefix(DN_Str8 string, DN_Str8 prefix, DN_Str8EqCase eq_case)
{
  DN_Str8 result = string;
  if (DN_Str8StartsWith(string, prefix, eq_case)) {
    result.data += prefix.size;
    result.size -= prefix.size;
  }
  return result;
}

DN_API DN_Str8 DN_Str8TrimHexPrefix(DN_Str8 string)
{
  DN_Str8 result = DN_Str8TrimPrefix(string, DN_Str8Lit("0x"), DN_Str8EqCase_Insensitive);
  return result;
}

DN_API DN_Str8 DN_Str8TrimSuffix(DN_Str8 string, DN_Str8 suffix, DN_Str8EqCase eq_case)
{
  DN_Str8 result = string;
  if (DN_Str8EndsWith(string, suffix, eq_case))
    result.size -= suffix.size;
  return result;
}

DN_API DN_Str8 DN_Str8TrimAround(DN_Str8 string, DN_Str8 trim_string)
{
  DN_Str8 result = DN_Str8TrimPrefix(string, trim_string);
  result         = DN_Str8TrimSuffix(result, trim_string);
  return result;
}

DN_API DN_Str8 DN_Str8TrimHeadWhitespace(DN_Str8 string)
{
  DN_Str8 result = string;
  if (string.size == 0)
    return result;

  char const *start = string.data;
  char const *end   = string.data + string.size;
  while (start < end && DN_CharIsWhitespace(start[0]))
    start++;

  result = DN_Str8FromPtr(start, end - start);
  return result;
}

DN_API DN_Str8 DN_Str8TrimTailWhitespace(DN_Str8 string)
{
  DN_Str8 result = string;
  if (string.size == 0)
    return result;

  char const *start = string.data;
  char const *end   = string.data + string.size;
  while (end > start && DN_CharIsWhitespace(end[-1]))
    end--;

  result = DN_Str8FromPtr(start, end - start);
  return result;
}

DN_API DN_Str8 DN_Str8TrimWhitespaceAround(DN_Str8 string)
{
  DN_Str8 result = DN_Str8TrimHeadWhitespace(string);
  result         = DN_Str8TrimTailWhitespace(result);
  return result;
}

DN_API DN_Str8 DN_Str8TrimByteOrderMark(DN_Str8 string)
{
  DN_Str8 result = string;
  if (result.size == 0)
    return result;

  // TODO(dn): This is little endian
  DN_Str8 UTF8_BOM     = DN_Str8Lit("\xEF\xBB\xBF");
  DN_Str8 UTF16_BOM_BE = DN_Str8Lit("\xEF\xFF");
  DN_Str8 UTF16_BOM_LE = DN_Str8Lit("\xFF\xEF");
  DN_Str8 UTF32_BOM_BE = DN_Str8Lit("\x00\x00\xFE\xFF");
  DN_Str8 UTF32_BOM_LE = DN_Str8Lit("\xFF\xFE\x00\x00");

  result = DN_Str8TrimPrefix(result, UTF8_BOM, DN_Str8EqCase_Sensitive);
  result = DN_Str8TrimPrefix(result, UTF16_BOM_BE, DN_Str8EqCase_Sensitive);
  result = DN_Str8TrimPrefix(result, UTF16_BOM_LE, DN_Str8EqCase_Sensitive);
  result = DN_Str8TrimPrefix(result, UTF32_BOM_BE, DN_Str8EqCase_Sensitive);
  result = DN_Str8TrimPrefix(result, UTF32_BOM_LE, DN_Str8EqCase_Sensitive);
  return result;
}

DN_API DN_Str8 DN_Str8FileNameFromPath(DN_Str8 path)
{
  DN_Str8             separators[] = {DN_Str8Lit("/"), DN_Str8Lit("\\")};
  DN_Str8BSplitResult split        = DN_Str8BSplitLastArray(path, separators, DN_ArrayCountU(separators));
  DN_Str8             result       = split.rhs.size ? split.rhs : split.lhs;
  return result;
}

DN_API DN_Str8 DN_Str8FileNameNoExtension(DN_Str8 path)
{
  DN_Str8 file_name = DN_Str8FileNameFromPath(path);
  DN_Str8 result    = DN_Str8FilePathNoExtension(file_name);
  return result;
}

DN_API DN_Str8 DN_Str8FilePathNoExtension(DN_Str8 path)
{
  DN_Str8BSplitResult split  = DN_Str8BSplitLast(path, DN_Str8Lit("."));
  DN_Str8                  result = split.lhs;
  return result;
}

DN_API DN_Str8 DN_Str8FileExtension(DN_Str8 path)
{
  DN_Str8BSplitResult split  = DN_Str8BSplitLast(path, DN_Str8Lit("."));
  DN_Str8                  result = split.rhs;
  return result;
}

DN_API DN_Str8 DN_Str8FileDirectoryFromPath(DN_Str8 path)
{
  DN_Str8                  separators[] = {DN_Str8Lit("/"), DN_Str8Lit("\\")};
  DN_Str8BSplitResult split        = DN_Str8BSplitLastArray(path, separators, DN_ArrayCountU(separators));
  DN_Str8                  result       = split.lhs;
  return result;
}

DN_API DN_Str8 DN_Str8AppendF(DN_Arena *arena, DN_Str8 string, char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8 result = DN_Str8AppendFV(arena, string, fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8 DN_Str8AppendFV(DN_Arena *arena, DN_Str8 string, char const *fmt, va_list args)
{
  // TODO: Calculate size and write into one buffer instead of 2 appends
  DN_Str8 append = DN_Str8FromFmtVArena(arena, fmt, args);
  DN_Str8 result = DN_Str8FromArena(arena, string.size + append.size, DN_ZMem_No);
  DN_Memcpy(result.data, string.data, string.size);
  DN_Memcpy(result.data + string.size, append.data, append.size);
  return result;
}

DN_API DN_Str8 DN_Str8FillF(DN_Arena *arena, DN_USize count, char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8 result = DN_Str8FillFV(arena, count, fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8 DN_Str8FillFV(DN_Arena *arena, DN_USize count, char const *fmt, va_list args)
{
  DN_Str8 fill = DN_Str8FromFmtVArena(arena, fmt, args);
  DN_Str8 result = DN_Str8FromArena(arena, count * fill.size, DN_ZMem_No);
  for (DN_USize index = 0; index < count; index++) {
    void *dest = result.data + (index * fill.size);
    DN_Memcpy(dest, fill.data, fill.size);
  }
  return result;
}

DN_API void DN_Str8Remove(DN_Str8 *string, DN_USize offset, DN_USize size)
{
  if (!string || string->size)
    return;

  char    *end           = string->data + string->size;
  char    *dest          = DN_Min(string->data + offset, end);
  char    *src           = DN_Min(string->data + offset + size, end);
  DN_USize bytes_to_move = end - src;
  DN_Memmove(dest, src, bytes_to_move);
  string->size -= bytes_to_move;
}

DN_API DN_Str8TruncateResult DN_Str8TruncateMiddle(DN_Arena *arena, DN_Str8 str8, DN_U32 side_size, DN_Str8 truncator)
{
  DN_Str8TruncateResult result = {};
  if (str8.size <= (side_size * 2)) {
    result.str8 = DN_Str8FromStr8Arena(arena, str8);
    return result;
  }

  DN_Str8 head     = DN_Str8Slice(str8, 0, side_size);
  DN_Str8 tail     = DN_Str8Slice(str8, str8.size - side_size, side_size);
  DN_MSVC_WARNING_PUSH
  DN_MSVC_WARNING_DISABLE(6284) // Object passed as _Param_(3) when a string is required in call to 'DN_Str8FromFmtArena' Actual type: 'struct DN_Str8'
  result.str8      = DN_Str8FromFmtArena(arena, "%S%S%S", head, truncator, tail);
  DN_MSVC_WARNING_POP
  result.truncated = true;
  return result;
}

DN_API DN_Str8 DN_Str8Lower(DN_Arena *arena, DN_Str8 string)
{
  DN_Str8 result = DN_Str8FromStr8Arena(arena, string);
  for (DN_ForIndexU(index, result.size))
    result.data[index] = DN_CharToLower(result.data[index]);
  return result;
}

DN_API DN_Str8 DN_Str8Upper(DN_Arena *arena, DN_Str8 string)
{
  DN_Str8 result = DN_Str8FromStr8Arena(arena, string);
  for (DN_ForIndexU(index, result.size))
    result.data[index] = DN_CharToUpper(result.data[index]);
  return result;
}

DN_API DN_Str8Builder DN_Str8BuilderFromArena(DN_Arena *arena)
{
  DN_Str8Builder result = {};
  result.arena          = arena;
  return result;
}

DN_API DN_Str8Builder DN_Str8BuilderFromStr8PtrRef(DN_Arena *arena, DN_Str8 const *strings, DN_USize size)
{
  DN_Str8Builder result = DN_Str8BuilderFromArena(arena);
  DN_Str8BuilderAppendArrayRef(&result, strings, size);
  return result;
}

DN_API DN_Str8Builder DN_Str8BuilderFromStr8PtrCopy(DN_Arena *arena, DN_Str8 const *strings, DN_USize size)
{
  DN_Str8Builder result = DN_Str8BuilderFromArena(arena);
  DN_Str8BuilderAppendArrayCopy(&result, strings, size);
  return result;
}

DN_API DN_Str8Builder DN_Str8BuilderFromBuilder(DN_Arena *arena, DN_Str8Builder const *builder)
{
  DN_Str8Builder result = DN_Str8BuilderFromArena(arena);
  DN_Str8BuilderAppendBuilderCopy(&result, builder);
  return result;
}

DN_API bool DN_Str8BuilderAddArrayRef(DN_Str8Builder *builder, DN_Str8 const *strings, DN_USize size, DN_Str8BuilderAdd add)
{
  if (!builder)
    return false;

  if (!strings || size <= 0)
    return true;

  DN_Str8Link *links = DN_ArenaNewArray(builder->arena, DN_Str8Link, size, DN_ZMem_No);
  if (!links)
    return false;

  if (add == DN_Str8BuilderAdd_Append) {
    for (DN_ForIndexU(index, size)) {
      DN_Str8      string = strings[index];
      DN_Str8Link *link   = links + index;

      link->string = string;
      link->next   = NULL;

      if (builder->head)
        builder->tail->next = link;
      else
        builder->head = link;

      builder->tail = link;
      builder->count++;
      builder->string_size += string.size;
    }
  } else {
    DN_Assert(add == DN_Str8BuilderAdd_Prepend);
    DN_MSVC_WARNING_PUSH
    DN_MSVC_WARNING_DISABLE(6293) // NOTE: Ill-defined loop
    for (DN_USize index = size - 1; index < size; index--) {
      DN_MSVC_WARNING_POP
      DN_Str8      string = strings[index];
      DN_Str8Link *link   = links + index;
      link->string        = string;
      link->next          = builder->head;
      builder->head       = link;
      if (!builder->tail)
        builder->tail = link;
      builder->count++;
      builder->string_size += string.size;
    }
  }
  return true;
}

DN_API bool DN_Str8BuilderAddArrayCopy(DN_Str8Builder *builder, DN_Str8 const *strings, DN_USize size, DN_Str8BuilderAdd add)
{
  if (!builder)
    return false;

  if (!strings || size <= 0)
    return true;

  DN_ArenaTempMem tmp_mem      = DN_ArenaTempMemBegin(builder->arena);
  bool            result       = true;
  DN_Str8        *strings_copy = DN_ArenaNewArray(builder->arena, DN_Str8, size, DN_ZMem_No);
  for (DN_ForIndexU(index, size)) {
    strings_copy[index] = DN_Str8FromStr8Arena(builder->arena, strings[index]);
    if (strings_copy[index].size != strings[index].size) {
      result = false;
      break;
    }
  }

  if (result)
    result = DN_Str8BuilderAddArrayRef(builder, strings_copy, size, add);

  if (!result)
    DN_ArenaTempMemEnd(tmp_mem);

  return result;
}

DN_API bool DN_Str8BuilderAddFV(DN_Str8Builder *builder, DN_Str8BuilderAdd add, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_Str8         string   = DN_Str8FromFmtVArena(builder->arena, fmt, args);
  DN_ArenaTempMem temp_mem = DN_ArenaTempMemBegin(builder->arena);
  bool            result   = DN_Str8BuilderAddArrayRef(builder, &string, 1, add);
  if (!result)
    DN_ArenaTempMemEnd(temp_mem);
  return result;
}

DN_API bool DN_Str8BuilderAppendRef(DN_Str8Builder *builder, DN_Str8 string)
{
  bool result = DN_Str8BuilderAddArrayRef(builder, &string, 1, DN_Str8BuilderAdd_Append);
  return result;
}

DN_API bool DN_Str8BuilderAppendCopy(DN_Str8Builder *builder, DN_Str8 string)
{
  bool result = DN_Str8BuilderAddArrayCopy(builder, &string, 1, DN_Str8BuilderAdd_Append);
  return result;
}

DN_API bool DN_Str8BuilderAppendF(DN_Str8Builder *builder, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  bool result = DN_Str8BuilderAppendFV(builder, fmt, args);
  va_end(args);
  return result;
}

DN_API bool DN_Str8BuilderAppendBytesRef(DN_Str8Builder *builder, void const *ptr, DN_USize size)
{
  DN_Str8 input  = DN_Str8FromPtr(ptr, size);
  bool    result = DN_Str8BuilderAppendRef(builder, input);
  return result;
}

DN_API bool DN_Str8BuilderAppendBytesCopy(DN_Str8Builder *builder, void const *ptr, DN_USize size)
{
  DN_Str8 input  = DN_Str8FromPtr(ptr, size);
  bool    result = DN_Str8BuilderAppendCopy(builder, input);
  return result;
}

static bool DN_Str8BuilderAppendBuilder_(DN_Str8Builder *dest, DN_Str8Builder const *src, bool copy)
{
  if (!dest)
    return false;
  if (!src)
    return true;

  DN_ArenaTempMemBegin(dest->arena);
  DN_Str8Link *links = DN_ArenaNewArray(dest->arena, DN_Str8Link, src->count, DN_ZMem_No);
  if (!links)
    return false;

  DN_Str8Link *first      = nullptr;
  DN_Str8Link *last       = nullptr;
  DN_USize     link_index = 0;
  bool         result     = true;
  for (DN_Str8Link const *it = src->head; it; it = it->next) {
    DN_Str8Link *link = links + link_index++;
    link->next        = nullptr;
    link->string      = it->string;

    if (copy) {
      link->string = DN_Str8FromStr8Arena(dest->arena, it->string);
      if (link->string.size != it->string.size) {
        result = false;
        break;
      }
    }

    if (last)
      last->next = link;
    else
      first = link;
    last = link;
  }

  if (result) {
    if (dest->head)
      dest->tail->next = first;
    else
      dest->head = first;
    dest->tail = last;
    dest->count += src->count;
    dest->string_size += src->string_size;
  }
  return true;
}

DN_API bool DN_Str8BuilderAppendBuilderRef(DN_Str8Builder *dest, DN_Str8Builder const *src)
{
  bool result = DN_Str8BuilderAppendBuilder_(dest, src, false);
  return result;
}

DN_API bool DN_Str8BuilderAppendBuilderCopy(DN_Str8Builder *dest, DN_Str8Builder const *src)
{
  bool result = DN_Str8BuilderAppendBuilder_(dest, src, true);
  return result;
}

DN_API bool DN_Str8BuilderPrependRef(DN_Str8Builder *builder, DN_Str8 string)
{
  bool result = DN_Str8BuilderAddArrayRef(builder, &string, 1, DN_Str8BuilderAdd_Prepend);
  return result;
}

DN_API bool DN_Str8BuilderPrependCopy(DN_Str8Builder *builder, DN_Str8 string)
{
  bool result = DN_Str8BuilderAddArrayCopy(builder, &string, 1, DN_Str8BuilderAdd_Prepend);
  return result;
}

DN_API bool DN_Str8BuilderPrependF(DN_Str8Builder *builder, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  bool result = DN_Str8BuilderPrependFV(builder, fmt, args);
  va_end(args);
  return result;
}

DN_API bool DN_Str8BuilderErase(DN_Str8Builder *builder, DN_Str8 string)
{
  for (DN_Str8Link **it = &builder->head; *it; it = &((*it)->next)) {
    if (DN_Str8Eq((*it)->string, string)) {
      *it = (*it)->next;
      builder->string_size -= string.size;
      builder->count -= 1;
      return true;
    }
  }
  return false;
}

DN_API DN_Str8 DN_Str8BuilderBuild(DN_Str8Builder const *builder, DN_Arena *arena)
{
  DN_Str8 result = DN_Str8BuilderBuildDelimited(builder, DN_Str8Lit(""), arena);
  return result;
}

DN_API DN_Str8 DN_Str8BuilderBuildDelimited(DN_Str8Builder const *builder, DN_Str8 delimiter, DN_Arena *arena)
{
  DN_Str8 result = DN_ZeroInit;
  if (!builder || builder->string_size <= 0 || builder->count <= 0)
    return result;

  DN_USize size_for_delimiter = delimiter.size ? ((builder->count - 1) * delimiter.size) : 0;
  result.data                 = DN_ArenaNewArray(arena,
                                  char,
                                  builder->string_size + size_for_delimiter + 1 /*null terminator*/,
                                  DN_ZMem_No);
  if (!result.data)
    return result;

  for (DN_Str8Link *link = builder->head; link; link = link->next) {
    DN_Memcpy(result.data + result.size, link->string.data, link->string.size);
    result.size += link->string.size;
    if (link->next && delimiter.size) {
      DN_Memcpy(result.data + result.size, delimiter.data, delimiter.size);
      result.size += delimiter.size;
    }
  }

  result.data[result.size] = 0;
  DN_Assert(result.size == builder->string_size + size_for_delimiter);
  return result;
}

DN_API DN_Slice<DN_Str8> DN_Str8BuilderBuildSlice(DN_Str8Builder const *builder, DN_Arena *arena)
{
  DN_Slice<DN_Str8> result = DN_ZeroInit;
  if (!builder || builder->string_size <= 0 || builder->count <= 0)
    return result;

  result = DN_Slice_Alloc<DN_Str8>(arena, builder->count, DN_ZMem_No);
  if (!result.data)
    return result;

  DN_USize slice_index = 0;
  for (DN_Str8Link *link = builder->head; link; link = link->next)
    result.data[slice_index++] = DN_Str8FromStr8Arena(arena, link->string);

  DN_Assert(slice_index == builder->count);
  return result;
}

// NOTE: DN_Char ///////////////////////////////////////////////////////////////////////////////////
// NOTE: DN_UTF ////////////////////////////////////////////////////////////////////////////////////
DN_API int DN_UTF8_EncodeCodepoint(uint8_t utf8[4], uint32_t codepoint)
{
  // NOTE: Table from https://www.reedbeta.com/blog/programmers-intro-to-unicode/
  // ----------------------------------------+----------------------------+--------------------+
  // UTF-8 (binary)                          | Code point (binary)        | Range              |
  // ----------------------------------------+----------------------------+--------------------+
  // 0xxx'xxxx                               |                   xxx'xxxx | U+0000  - U+007F   |
  // 110x'xxxx 10yy'yyyy                     |              xxx'xxyy'yyyy | U+0080  - U+07FF   |
  // 1110'xxxx 10yy'yyyy 10zz'zzzz           |        xxxx'yyyy'yyzz'zzzz | U+0800  - U+FFFF   |
  // 1111'0xxx 10yy'yyyy 10zz'zzzz 10ww'wwww | x'xxyy'yyyy'zzzz'zzww'wwww | U+10000 - U+10FFFF |
  // ----------------------------------------+----------------------------+--------------------+

  if (codepoint <= 0b0111'1111) {
    utf8[0] = DN_Cast(uint8_t) codepoint;
    return 1;
  }

  if (codepoint <= 0b0111'1111'1111) {
    utf8[0] = (0b1100'0000 | ((codepoint >> 6) & 0b01'1111)); // x
    utf8[1] = (0b1000'0000 | ((codepoint >> 0) & 0b11'1111)); // y
    return 2;
  }

  if (codepoint <= 0b1111'1111'1111'1111) {
    utf8[0] = (0b1110'0000 | ((codepoint >> 12) & 0b00'1111)); // x
    utf8[1] = (0b1000'0000 | ((codepoint >> 6) & 0b11'1111));  // y
    utf8[2] = (0b1000'0000 | ((codepoint >> 0) & 0b11'1111));  // z
    return 3;
  }

  if (codepoint <= 0b1'1111'1111'1111'1111'1111) {
    utf8[0] = (0b1111'0000 | ((codepoint >> 18) & 0b00'0111)); // x
    utf8[1] = (0b1000'0000 | ((codepoint >> 12) & 0b11'1111)); // y
    utf8[2] = (0b1000'0000 | ((codepoint >> 6) & 0b11'1111));  // z
    utf8[3] = (0b1000'0000 | ((codepoint >> 0) & 0b11'1111));  // w
    return 4;
  }

  return 0;
}

DN_API int DN_UTF16_EncodeCodepoint(uint16_t utf16[2], uint32_t codepoint)
{
  // NOTE: Table from https://www.reedbeta.com/blog/programmers-intro-to-unicode/
  // ----------------------------------------+------------------------------------+------------------+
  // UTF-16 (binary)                         | Code point (binary)                | Range            |
  // ----------------------------------------+------------------------------------+------------------+
  // xxxx'xxxx'xxxx'xxxx                     | xxxx'xxxx'xxxx'xxxx                | U+0000???U+FFFF    |
  // 1101'10xx'xxxx'xxxx 1101'11yy'yyyy'yyyy | xxxx'xxxx'xxyy'yyyy'yyyy + 0x10000 | U+10000???U+10FFFF |
  // ----------------------------------------+------------------------------------+------------------+

  if (codepoint <= 0b1111'1111'1111'1111) {
    utf16[0] = DN_Cast(uint16_t) codepoint;
    return 1;
  }

  if (codepoint <= 0b1111'1111'1111'1111'1111) {
    uint32_t surrogate_codepoint = codepoint + 0x10000;
    utf16[0]                     = 0b1101'1000'0000'0000 | ((surrogate_codepoint >> 10) & 0b11'1111'1111); // x
    utf16[1]                     = 0b1101'1100'0000'0000 | ((surrogate_codepoint >> 0) & 0b11'1111'1111);  // y
    return 2;
  }

  return 0;
}


DN_API DN_U8 DN_U8FromHexNibble(char hex)
{
  bool digit  = hex >= '0' && hex <= '9';
  bool upper  = hex >= 'A' && hex <= 'F';
  bool lower  = hex >= 'a' && hex <= 'f';
  DN_U8  result = 0xFF;
  if (digit)
    result = hex - '0';
  if (upper)
    result = hex - 'A' + 10;
  if (lower)
    result = hex - 'a' + 10;
  return result;
}

DN_API DN_NibbleFromU8Result DN_NibbleFromU8(DN_U8 u8)
{
  static char const    *table  = "0123456789abcdef";
  DN_U8                 lhs    = (u8 >> 0) & 0xF;
  DN_U8                 rhs    = (u8 >> 4) & 0xF;
  DN_NibbleFromU8Result result = {};
  result.nibble0               = table[rhs];
  result.nibble1               = table[lhs];
  return result;
}

DN_API DN_USize DN_BytesFromHexPtr(void const *hex, DN_USize hex_count, void *bytes, DN_USize bytes_count)
{
  DN_USize     result = 0;
  DN_U8 const *hex_u8 = DN_Cast(DN_U8 const *) hex;
  if (hex_count >= 2 && hex_u8[0] == '0' && (hex_u8[1] == 'x' || hex_u8[1] == 'X')) {
    hex_u8    += 2;
    hex_count -= 2;
  }

  if (hex_count > (bytes_count * 2))
    return result;

  DN_U8       *ptr    = DN_Cast(DN_U8 *)bytes;
  for (DN_USize index = 0; index < hex_count; index += 2) {
    DN_U8 nibble0 = DN_U8FromHexNibble(hex_u8[index + 0]);
    DN_U8 nibble1 = DN_U8FromHexNibble(hex_u8[index + 1]);
    if (nibble0 == 0xFF || nibble1 == 0xFF)
      return result;
    *ptr++ = nibble0 << 4 | nibble1 << 0;
    result++;
  }
  return result;
}

DN_API DN_Str8 DN_BytesFromHexPtrArena(void const *hex, DN_USize hex_size, DN_Arena *arena)
{
  DN_Assert(hex_size % 2 == 0);
  DN_Str8 result = {};
  result.data    = DN_ArenaNewArray(arena, char, hex_size / 2, DN_ZMem_No);
  if (result.data)
    result.size = DN_BytesFromHexPtr(hex, hex_size, result.data, hex_size / 2);
  return result;
}

DN_API DN_USize DN_BytesFromHexStr8(DN_Str8 hex, void *dest, DN_USize dest_count)
{
  DN_USize result = DN_BytesFromHexPtr(hex.data, hex.size, dest, dest_count);
  return result;
}

DN_API DN_Str8 DN_BytesFromHexStr8Arena(DN_Str8 hex, DN_Arena *arena)
{
  DN_Str8 result = DN_BytesFromHexPtrArena(hex.data, hex.size, arena);
  return result;
}

DN_API DN_U8x16 DN_BytesFromHex32Ptr(void const *hex, DN_USize hex_count)
{
  DN_U8x16 result = {};
  DN_Assert(hex_count / 2 == sizeof result.data);
  DN_USize bytes_written = DN_BytesFromHexPtr(hex, hex_count, result.data, sizeof result);
  DN_Assert(bytes_written == sizeof result.data);
  return result;
}

DN_API DN_U8x32 DN_BytesFromHex64Ptr(void const *hex, DN_USize hex_count)
{
  DN_U8x32 result = {};
  DN_Assert(hex_count / 2 == sizeof result.data);
  DN_USize bytes_written = DN_BytesFromHexPtr(hex, hex_count, result.data, sizeof result);
  DN_Assert(bytes_written == sizeof result.data);
  return result;
}

DN_API DN_HexU64Str8 DN_HexFromU64(DN_U64 value, DN_HexFromU64Type type)
{
  DN_HexU64Str8 result = {};
  DN_HexFromBytesPtr(&value, sizeof(value), result.data, sizeof(result.data));
  if (type == DN_HexFromU64Type_Uppercase) {
    for (DN_USize index = 0; index < result.size; index++)
      result.data[index] = DN_CharToUpper(result.data[index]);
  }
  return result;
}

DN_API DN_USize DN_HexFromBytesPtr(void const *bytes, DN_USize bytes_count, void *hex, DN_USize hex_count)
{
  DN_USize result = 0;
  if ((bytes_count * 2) != hex_count)
    return result;
  DN_U8 const *src_u8 = DN_Cast(DN_U8 const *)bytes;
  DN_U8       *ptr    = DN_Cast(DN_U8 *)hex;
  for (DN_USize index = 0; index < bytes_count; index++) {
    DN_NibbleFromU8Result to_nibbles  = DN_NibbleFromU8(src_u8[index]);
    *ptr++                            = to_nibbles.nibble0;
    *ptr++                            = to_nibbles.nibble1;
    result                           += 2;
  }
  return result;
}

DN_API DN_Str8 DN_HexFromBytesPtrArena(void const *bytes, DN_USize bytes_count, DN_Arena *arena)
{
  DN_Str8 result = {};
  result.data    = DN_ArenaNewArray(arena, char, bytes_count * 2, DN_ZMem_No);
  if (result.data)
    result.size = DN_HexFromBytesPtr(bytes, bytes_count, result.data, bytes_count * 2);
  return result;
}

DN_API DN_Hex32 DN_HexFromBytes16Ptr(void const *bytes, DN_USize bytes_count)
{
  DN_Hex32 result = {};
  DN_Assert(bytes_count * 2 == sizeof result.data);
  DN_USize hex_written = DN_HexFromBytesPtr(bytes, bytes_count, result.data, sizeof result.data);
  DN_Assert(hex_written == sizeof result.data);
  return result;
}

DN_API DN_Hex64 DN_HexFromBytes32Ptr(void const *bytes, DN_USize bytes_count)
{
  DN_Hex64 result = {};
  DN_Assert(bytes_count * 2 == sizeof result.data);
  DN_USize hex_written = DN_HexFromBytesPtr(bytes, bytes_count, result.data, sizeof result.data);
  DN_Assert(hex_written == sizeof result.data);
  return result;
}

DN_API DN_Hex128 DN_HexFromBytes64Ptr(void const *bytes, DN_USize bytes_count)
{
  DN_Hex128 result = {};
  DN_Assert(bytes_count * 2 == sizeof result.data);
  DN_USize hex_written = DN_HexFromBytesPtr(bytes, bytes_count, result.data, sizeof result.data);
  DN_Assert(hex_written == sizeof result.data);
  return result;
}

DN_API DN_Str8x128 DN_AgeStr8FromMsU64(DN_U64 duration_ms, DN_AgeUnit units)
{
  DN_Str8x128 result    = {};
  DN_U64      remainder_ms = duration_ms;
  if (units & DN_AgeUnit_FractionalSec) {
    units |= DN_AgeUnit_Sec;
    units &= ~DN_AgeUnit_Ms;
  }

  if (units & DN_AgeUnit_Year) {
    DN_USize   value_usize  = remainder_ms / (DN_SecFromYears(1) * 1000);
    remainder_ms           -= DN_SecFromYears(value_usize) * 1000;
    if (value_usize)
      DN_FmtAppend(result.data, &result.size, sizeof(result.data), "%s%zuyr", result.size ? " " : "", value_usize);
  }

  if (units & DN_AgeUnit_Week) {
    DN_USize value_usize    = remainder_ms / (DN_SecFromWeeks(1) * 1000);
    remainder_ms           -= DN_SecFromWeeks(value_usize) * 1000;
    if (value_usize)
      DN_FmtAppend(result.data, &result.size, sizeof(result.data), "%s%zuw", result.size ? " " : "", value_usize);
  }

  if (units & DN_AgeUnit_Day) {
    DN_USize value_usize    = remainder_ms / (DN_SecFromDays(1) * 1000);
    remainder_ms           -= DN_SecFromDays(value_usize) * 1000;
    if (value_usize)
      DN_FmtAppend(result.data, &result.size, sizeof(result.data), "%s%zud", result.size ? " " : "", value_usize);
  }

  if (units & DN_AgeUnit_Hr) {
    DN_USize value_usize    = remainder_ms / (DN_SecFromHours(1) * 1000);
    remainder_ms           -= DN_SecFromHours(value_usize) * 1000;
    if (value_usize)
      DN_FmtAppend(result.data, &result.size, sizeof(result.data), "%s%zuh", result.size ? " " : "", value_usize);
  }

  if (units & DN_AgeUnit_Min) {
    DN_USize value_usize    = remainder_ms / (DN_SecFromMins(1) * 1000);
    remainder_ms           -= DN_SecFromMins(value_usize) * 1000;
    if (value_usize)
      DN_FmtAppend(result.data, &result.size, sizeof(result.data), "%s%zum", result.size ? " " : "", value_usize);
  }

  if (units & DN_AgeUnit_Sec) {
    if (units & DN_AgeUnit_FractionalSec) {
        DN_F64 remainder_s = remainder_ms / 1000.0;
        DN_FmtAppend(result.data, &result.size, sizeof(result.data), "%s%.3fs", result.size ? " " : "", remainder_s);
        remainder_ms = 0;
    } else {
      DN_USize value_usize  = remainder_ms / 1000;
      remainder_ms         -= DN_Cast(DN_USize)(value_usize * 1000);
      if (value_usize)
        DN_FmtAppend(result.data, &result.size, sizeof(result.data), "%s%zus", result.size ? " " : "", value_usize);
    }
  }

  if (units & DN_AgeUnit_Ms) {
    DN_Assert((units & DN_AgeUnit_FractionalSec) == 0);
    DN_USize value_usize  = remainder_ms;
    remainder_ms         -= value_usize;
    if (value_usize || result.size == 0)
      DN_FmtAppend(result.data, &result.size, sizeof(result.data), "%s%zums", result.size ? " " : "", value_usize);
  }
  return result;
}

DN_API DN_Str8x128 DN_AgeStr8FromSecU64(DN_U64 duration_s, DN_AgeUnit units)
{
  DN_U64      duration_ms = duration_s * 1000;
  DN_Str8x128 result      = DN_AgeStr8FromMsU64(duration_ms, units);
  return result;
}

DN_API DN_Str8x128 DN_AgeStr8FromSecF64(DN_F64 duration_s, DN_AgeUnit units)
{
  DN_U64      duration_ms = DN_Cast(DN_U64)(duration_s * 1000.0);
  DN_Str8x128 result      = DN_AgeStr8FromMsU64(duration_ms, units);
  return result;
}

DN_API DN_Str8 DN_Str8FromByteCountType(DN_ByteCountType type)
{
  DN_Str8 result = DN_Str8Lit("");
  switch (type) {
    case DN_ByteCountType_B:     result = DN_Str8Lit("B"); break;
    case DN_ByteCountType_KiB:   result = DN_Str8Lit("KiB"); break;
    case DN_ByteCountType_MiB:   result = DN_Str8Lit("MiB"); break;
    case DN_ByteCountType_GiB:   result = DN_Str8Lit("GiB"); break;
    case DN_ByteCountType_TiB:   result = DN_Str8Lit("TiB"); break;
    case DN_ByteCountType_Count: result = DN_Str8Lit(""); break;
    case DN_ByteCountType_Auto:  result = DN_Str8Lit(""); break;
  }
  return result;
}

DN_API DN_ByteCountResult DN_ByteCountFromType(DN_U64 bytes, DN_ByteCountType type)
{
  DN_Assert(type != DN_ByteCountType_Count);
  DN_ByteCountResult result = {};
  result.bytes              = DN_Cast(DN_F64) bytes;
  if (type == DN_ByteCountType_Auto)
    for (; result.type < DN_ByteCountType_Count && result.bytes >= 1024.0; result.type = DN_Cast(DN_ByteCountType)(DN_Cast(DN_USize) result.type + 1))
      result.bytes /= 1024.0;
  else
    for (; result.type < type; result.type = DN_Cast(DN_ByteCountType)(DN_Cast(DN_USize) result.type + 1))
      result.bytes /= 1024.0;
  result.suffix = DN_Str8FromByteCountType(result.type);
  return result;
}

DN_API DN_Str8x32 DN_ByteCountStr8x32FromType(DN_U64 bytes, DN_ByteCountType type)
{
  DN_ByteCountResult byte_count = DN_ByteCountFromType(bytes, type);
  DN_Str8x32         result     = DN_Str8x32FromFmt("%.2f%.*s", byte_count.bytes, DN_Str8PrintFmt(byte_count.suffix));
  return result;
}

DN_API DN_Profiler DN_ProfilerInit(DN_ProfilerAnchor *anchors, DN_USize count, DN_USize anchors_per_frame, DN_ProfilerTSCNowFunc *tsc_now, DN_U64 tsc_frequency)
{
  DN_Profiler result       = {};
  result.anchors           = anchors;
  result.anchors_count     = count;
  result.anchors_per_frame = anchors_per_frame;
  result.tsc_now           = tsc_now;
  result.tsc_frequency     = tsc_frequency;

  DN_AssertF(result.tsc_frequency != 0,
             "You must set this to the frequency of the timestamp counter function (TSC) (e.g. how "
             "many ticks occur between timestamps). We use this to determine the duration between "
             "each zone's recorded TSC. For example if the 'tsc_now' was set to Window's "
             "QueryPerformanceCounter then 'tsc_frequency' would be set to the value of "
             "QueryPerformanceFrequency which is typically 10mhz (e.g. The duration between two "
             "consecutive TSC's is 10mhz)."
             ""
             "Hence frequency can't be zero otherwise it's a divide by 0. If you don't have a TSC "
             "function and pass in null, the profiler defaults to rdtsc() and you must measure the "
             "frequency of rdtsc yourself. The reason for this is that measuring rdtsc requires "
             "having some alternate timing mechanism to measure the duration between the TSCs "
             "provided by rdtsc and this profiler makes no assumption about what timing primitives "
             "are available other than rdtsc which is a CPU builtin available on basically all "
             "platforms or have an equivalent (e.g. __builtin_readcyclecounter)"
             ""
             "This codebase provides DN_OS_EstimateTSCPerSecond() as an example of how to that for "
             "convenience and is available if compiling with the OS layer. Some platforms like "
             "Emscripten don't support rdtsc() so you should use an alternative method like "
             "emscripten_get_now() or clock_gettime with CLOCK_MONOTONIC.");
  return result;
}

DN_API DN_USize DN_ProfilerFrameCount(DN_Profiler const *profiler)
{
  DN_USize result = profiler->anchors_count / profiler->anchors_per_frame;
  return result;
}

DN_API DN_ProfilerAnchorArray DN_ProfilerFrameAnchorsFromIndex(DN_Profiler *profiler, DN_USize frame_index)
{
  DN_ProfilerAnchorArray result        = {};
  DN_USize               anchor_offset = frame_index * profiler->anchors_per_frame;
  result.data                          = profiler->anchors + anchor_offset;
  result.count                         = profiler->anchors_per_frame;
  return result;
}

DN_API DN_ProfilerAnchorArray DN_ProfilerFrameAnchors(DN_Profiler *profiler)
{
  DN_ProfilerAnchorArray result = DN_ProfilerFrameAnchorsFromIndex(profiler, profiler->frame_index);
  return result;
}

DN_API DN_ProfilerZone DN_ProfilerBeginZone(DN_Profiler *profiler, DN_Str8 name, DN_U16 anchor_index)
{
  DN_ProfilerZone result = {};
  if (profiler->paused)
    return result;

  DN_Assert(anchor_index < profiler->anchors_per_frame);
  DN_ProfilerAnchor *anchor = DN_ProfilerFrameAnchors(profiler).data + anchor_index;
  anchor->name              = name;

  // TODO: We need per-thread-local-storage profiler so that we can use these apis
  // across threads. For now, we let them overwrite each other but this is not tenable.
  #if 0
    if (anchor->name.size && anchor->name != name)
        DN_AssertF(name == anchor->name, "Potentially overwriting a zone by accident? Anchor is '%.*s', name is '%.*s'", DN_Str8PrintFmt(anchor->name), DN_Str8PrintFmt(name));
  #endif

  result.begin_tsc                 = profiler->tsc_now ? profiler->tsc_now() : DN_CPUGetTSC();
  result.anchor_index              = anchor_index;
  result.parent_zone               = profiler->parent_zone;
  result.elapsed_tsc_at_zone_start = anchor->tsc_inclusive;
  profiler->parent_zone            = anchor_index;
  return result;
}

DN_API void DN_ProfilerEndZone(DN_Profiler *profiler, DN_ProfilerZone zone)
{
  if (profiler->paused)
    return;

  DN_Assert(zone.anchor_index < profiler->anchors_per_frame);
  DN_Assert(zone.parent_zone < profiler->anchors_per_frame);

  DN_ProfilerAnchorArray array       = DN_ProfilerFrameAnchors(profiler);
  DN_ProfilerAnchor     *anchor      = array.data + zone.anchor_index;
  DN_U64                 tsc_now     = profiler->tsc_now ? profiler->tsc_now() : DN_CPUGetTSC();
  DN_U64                 elapsed_tsc = tsc_now - zone.begin_tsc;

  anchor->hit_count++;
  anchor->tsc_inclusive             = zone.elapsed_tsc_at_zone_start + elapsed_tsc;
  anchor->tsc_exclusive            += elapsed_tsc;

  if (zone.parent_zone != zone.anchor_index) {
    DN_ProfilerAnchor *parent_anchor  = array.data + zone.parent_zone;
    parent_anchor->tsc_exclusive     -= elapsed_tsc;
  }
  profiler->parent_zone = zone.parent_zone;
}

DN_API void DN_ProfilerNewFrame(DN_Profiler *profiler)
{
  if (profiler->paused)
    return;

  // NOTE: End the frame's zone
  DN_ProfilerEndZone(profiler, profiler->frame_zone);
  DN_ProfilerAnchorArray old_frame_anchors = DN_ProfilerFrameAnchors(profiler);
  DN_ProfilerAnchor      old_frame_anchor  = old_frame_anchors.data[0];
  profiler->frame_avg_tsc                  = (profiler->frame_avg_tsc + old_frame_anchor.tsc_inclusive) / 2.f;

  // NOTE: Bump to the next frame
  DN_USize frame_count                = profiler->anchors_count / profiler->anchors_per_frame;
  profiler->frame_index               = (profiler->frame_index + 1) % frame_count;

  // NOTE: Zero out the anchors
  DN_ProfilerAnchorArray next_anchors = DN_ProfilerFrameAnchors(profiler);
  DN_Memset(next_anchors.data, 0, sizeof(*profiler->anchors) * next_anchors.count);

  // NOTE: Start the frame's zone
  profiler->frame_zone = DN_ProfilerBeginZone(profiler, DN_Str8Lit("Profiler Frame"), 0);
}

DN_API void DN_ProfilerDump(DN_Profiler *profiler)
{
  if (profiler->frame_index == 0)
    return;

  DN_USize frame_index = profiler->frame_index - 1;
  DN_Assert(profiler->frame_index < profiler->anchors_per_frame);

  DN_ProfilerAnchor *anchors = profiler->anchors + (frame_index * profiler->anchors_per_frame);
  for (DN_USize index = 1; index < profiler->anchors_per_frame; index++) {
    DN_ProfilerAnchor const *anchor = anchors + index;
    if (!anchor->hit_count)
      continue;

    DN_U64 tsc_exclusive              = anchor->tsc_exclusive;
    DN_U64 tsc_inclusive              = anchor->tsc_inclusive;
    DN_F64 tsc_exclusive_milliseconds = tsc_exclusive * 1000 / DN_Cast(DN_F64) profiler->tsc_frequency;
    if (tsc_exclusive == tsc_inclusive) {
      DN_OS_PrintOutLnF("%.*s[%u]: %.1fms", DN_Str8PrintFmt(anchor->name), anchor->hit_count, tsc_exclusive_milliseconds);
    } else {
      DN_F64 tsc_inclusive_milliseconds = tsc_inclusive * 1000 / DN_Cast(DN_F64) profiler->tsc_frequency;
      DN_OS_PrintOutLnF("%.*s[%u]: %.1f/%.1fms",
                        DN_Str8PrintFmt(anchor->name),
                        anchor->hit_count,
                        tsc_exclusive_milliseconds,
                        tsc_inclusive_milliseconds);
    }
  }
}

DN_API DN_F64 DN_ProfilerSecFromTSC(DN_Profiler *profiler, DN_U64 duration_tsc)
{
  DN_F64 result = DN_Cast(DN_F64)duration_tsc / profiler->tsc_frequency;
  return result;
}

DN_API DN_F64 DN_ProfilerMsFromTSC(DN_Profiler *profiler, DN_U64 duration_tsc)
{
  DN_F64 result = DN_Cast(DN_F64)duration_tsc / profiler->tsc_frequency * 1000.0;
  return result;
}
