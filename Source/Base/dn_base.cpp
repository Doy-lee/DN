#define DN_BASE_CPP

#include "../dn_clangd.h"

// NOTE: [$INTR] Intrinsics ////////////////////////////////////////////////////////////////////////
DN_CPUFeatureDecl g_dn_cpu_feature_decl[DN_CPUFeature_Count];

#if !defined(DN_PLATFORM_ARM64) && !defined(DN_PLATFORM_EMSCRIPTEN)
  #define DN_SUPPORTS_CPU_ID
#endif

#if defined(DN_SUPPORTS_CPU_ID)
  #if defined(DN_COMPILER_GCC) || defined(DN_COMPILER_CLANG)
    #include <cpuid.h>
  #endif
#endif // defined(DN_SUPPORTS_CPU_ID)

DN_API DN_CPUIDResult DN_CPU_ID(DN_CPUIDArgs args)
{
  DN_CPUIDResult result = {};
#if defined(DN_SUPPORTS_CPU_ID)
  __cpuidex(result.values, args.eax, args.ecx);
#endif
  return result;
}

DN_API DN_USize DN_CPU_HasFeatureArray(DN_CPUReport const *report, DN_CPUFeatureQuery *features, DN_USize features_size)
{
  DN_USize       result = 0;
  DN_USize const BITS   = sizeof(report->features[0]) * 8;
  for (DN_ForIndexU(feature_index, features_size)) {
    DN_CPUFeatureQuery *query       = features + feature_index;
    DN_USize            chunk_index = query->feature / BITS;
    DN_USize            chunk_bit   = query->feature % BITS;
    DN_U64              chunk       = report->features[chunk_index];
    query->available                = chunk & (1ULL << chunk_bit);
    result += DN_CAST(int) query->available;
  }

  return result;
}

DN_API bool DN_CPU_HasFeature(DN_CPUReport const *report, DN_CPUFeature feature)
{
  DN_CPUFeatureQuery query = {};
  query.feature            = feature;
  bool result              = DN_CPU_HasFeatureArray(report, &query, 1) == 1;
  return result;
}

DN_API bool DN_CPU_HasAllFeatures(DN_CPUReport const *report, DN_CPUFeature const *features, DN_USize features_size)
{
  bool result = true;
  for (DN_USize index = 0; result && index < features_size; index++)
    result &= DN_CPU_HasFeature(report, features[index]);
  return result;
}

DN_API void DN_CPU_SetFeature(DN_CPUReport *report, DN_CPUFeature feature)
{
  DN_Assert(feature < DN_CPUFeature_Count);
  DN_USize const BITS        = sizeof(report->features[0]) * 8;
  DN_USize       chunk_index = feature / BITS;
  DN_USize       chunk_bit   = feature % BITS;
  report->features[chunk_index] |= (1ULL << chunk_bit);
}

DN_API DN_CPUReport DN_CPU_Report()
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
    fn_0000_[0] = DN_CPU_ID(args);

    // NOTE: Query extended function (e.g. eax = 0x8000'0000) for function count + cpu vendor
    args        = {};
    args.eax    = DN_CAST(int) EXTENDED_FUNC_BASE_EAX;
    fn_8000_[0] = DN_CPU_ID(args);
  }

  // NOTE: Extract function count ////////////////////////////////////////////////////////////////
  int const STANDARD_FUNC_MAX_EAX = fn_0000_[0x0000].reg.eax;
  int const EXTENDED_FUNC_MAX_EAX = fn_8000_[0x0000].reg.eax;

  // NOTE: Enumerate all CPUID results for the known function counts /////////////////////////////
  {
    DN_AssertF((STANDARD_FUNC_MAX_EAX + 1) <= DN_ArrayCountI(fn_0000_),
               "Max standard count is %d",
               STANDARD_FUNC_MAX_EAX + 1);
    DN_AssertF((DN_CAST(DN_ISize) EXTENDED_FUNC_MAX_EAX - EXTENDED_FUNC_BASE_EAX + 1) <= DN_ArrayCountI(fn_8000_),
               "Max extended count is %zu",
               DN_CAST(DN_ISize) EXTENDED_FUNC_MAX_EAX - EXTENDED_FUNC_BASE_EAX + 1);

    for (int eax = 1; eax <= STANDARD_FUNC_MAX_EAX; eax++) {
      DN_CPUIDArgs args = {};
      args.eax          = eax;
      fn_0000_[eax]     = DN_CPU_ID(args);
    }

    for (int eax = EXTENDED_FUNC_BASE_EAX + 1, index = 1; eax <= EXTENDED_FUNC_MAX_EAX; eax++, index++) {
      DN_CPUIDArgs args = {};
      args.eax          = eax;
      fn_8000_[index]   = DN_CPU_ID(args);
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
    switch (DN_CAST(DN_CPUFeature)           ext_index) {
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
      DN_CPU_SetFeature(&result, DN_CAST(DN_CPUFeature) ext_index);
  }
#endif // DN_SUPPORTS_CPU_ID
  return result;
}

// NOTE: DN_TicketMutex ////////////////////////////////////////////////////////////////////////////
DN_API void DN_TicketMutex_Begin(DN_TicketMutex *mutex)
{
  unsigned int ticket = DN_Atomic_AddU32(&mutex->ticket, 1);
  DN_TicketMutex_BeginTicket(mutex, ticket);
}

DN_API void DN_TicketMutex_End(DN_TicketMutex *mutex)
{
  DN_Atomic_AddU32(&mutex->serving, 1);
}

DN_API DN_UInt DN_TicketMutex_MakeTicket(DN_TicketMutex *mutex)
{
  DN_UInt result = DN_Atomic_AddU32(&mutex->ticket, 1);
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
DN_API void DN_Bit_UnsetInplace(DN_USize *flags, DN_USize bitfield)
{
  *flags = (*flags & ~bitfield);
}

DN_API void DN_Bit_SetInplace(DN_USize *flags, DN_USize bitfield)
{
  *flags = (*flags | bitfield);
}

DN_API bool DN_Bit_IsSet(DN_USize bits, DN_USize bits_to_set)
{
  auto result = DN_CAST(bool)((bits & bits_to_set) == bits_to_set);
  return result;
}

DN_API bool DN_Bit_IsNotSet(DN_USize bits, DN_USize bits_to_check)
{
  auto result = !DN_Bit_IsSet(bits, bits_to_check);
  return result;
}

// NOTE: DN_Safe ///////////////////////////////////////////////////////////////////////////////////
DN_API DN_I64 DN_Safe_AddI64(int64_t a, int64_t b)
{
  DN_I64 result = DN_CheckF(a <= INT64_MAX - b, "a=%zd, b=%zd", a, b) ? (a + b) : INT64_MAX;
  return result;
}

DN_API DN_I64 DN_Safe_MulI64(int64_t a, int64_t b)
{
  DN_I64 result = DN_CheckF(a <= INT64_MAX / b, "a=%zd, b=%zd", a, b) ? (a * b) : INT64_MAX;
  return result;
}

DN_API DN_U64 DN_Safe_AddU64(DN_U64 a, DN_U64 b)
{
  DN_U64 result = DN_CheckF(a <= UINT64_MAX - b, "a=%zu, b=%zu", a, b) ? (a + b) : UINT64_MAX;
  return result;
}

DN_API DN_U64 DN_Safe_SubU64(DN_U64 a, DN_U64 b)
{
  DN_U64 result = DN_CheckF(a >= b, "a=%zu, b=%zu", a, b) ? (a - b) : 0;
  return result;
}

DN_API DN_U64 DN_Safe_MulU64(DN_U64 a, DN_U64 b)
{
  DN_U64 result = DN_CheckF(a <= UINT64_MAX / b, "a=%zu, b=%zu", a, b) ? (a * b) : UINT64_MAX;
  return result;
}

DN_API DN_U32 DN_Safe_SubU32(DN_U32 a, DN_U32 b)
{
  DN_U32 result = DN_CheckF(a >= b, "a=%u, b=%u", a, b) ? (a - b) : 0;
  return result;
}

// NOTE: DN_SaturateCastUSizeToI* ////////////////////////////////////////////////////////////
// INT*_MAX literals will be promoted to the type of uintmax_t as uintmax_t is
// the highest possible rank (unsigned > signed).
DN_API int DN_SaturateCastUSizeToInt(DN_USize val)
{
  int result = DN_Check(DN_CAST(uintmax_t) val <= INT_MAX) ? DN_CAST(int) val : INT_MAX;
  return result;
}

DN_API int8_t DN_SaturateCastUSizeToI8(DN_USize val)
{
  int8_t result = DN_Check(DN_CAST(uintmax_t) val <= INT8_MAX) ? DN_CAST(int8_t) val : INT8_MAX;
  return result;
}

DN_API DN_I16 DN_SaturateCastUSizeToI16(DN_USize val)
{
  DN_I16 result = DN_Check(DN_CAST(uintmax_t) val <= INT16_MAX) ? DN_CAST(DN_I16) val : INT16_MAX;
  return result;
}

DN_API DN_I32 DN_SaturateCastUSizeToI32(DN_USize val)
{
  DN_I32 result = DN_Check(DN_CAST(uintmax_t) val <= INT32_MAX) ? DN_CAST(DN_I32) val : INT32_MAX;
  return result;
}

DN_API int64_t DN_SaturateCastUSizeToI64(DN_USize val)
{
  int64_t result = DN_Check(DN_CAST(uintmax_t) val <= INT64_MAX) ? DN_CAST(int64_t) val : INT64_MAX;
  return result;
}

// NOTE: DN_SaturateCastUSizeToU* ////////////////////////////////////////////////////////////
// Both operands are unsigned and the lowest rank operand will be promoted to
// match the highest rank operand.
DN_API DN_U8 DN_SaturateCastUSizeToU8(DN_USize val)
{
  DN_U8 result = DN_Check(val <= UINT8_MAX) ? DN_CAST(DN_U8) val : UINT8_MAX;
  return result;
}

DN_API DN_U16 DN_SaturateCastUSizeToU16(DN_USize val)
{
  DN_U16 result = DN_Check(val <= UINT16_MAX) ? DN_CAST(DN_U16) val : UINT16_MAX;
  return result;
}

DN_API DN_U32 DN_SaturateCastUSizeToU32(DN_USize val)
{
  DN_U32 result = DN_Check(val <= UINT32_MAX) ? DN_CAST(DN_U32) val : UINT32_MAX;
  return result;
}

DN_API DN_U64 DN_SaturateCastUSizeToU64(DN_USize val)
{
  DN_U64 result = DN_Check(DN_CAST(DN_U64) val <= UINT64_MAX) ? DN_CAST(DN_U64) val : UINT64_MAX;
  return result;
}

// NOTE: DN_SaturateCastU64To* ///////////////////////////////////////////////////////////////
DN_API int DN_SaturateCastU64ToInt(DN_U64 val)
{
  int result = DN_Check(val <= INT_MAX) ? DN_CAST(int) val : INT_MAX;
  return result;
}

DN_API int8_t DN_SaturateCastU64ToI8(DN_U64 val)
{
  int8_t result = DN_Check(val <= INT8_MAX) ? DN_CAST(int8_t) val : INT8_MAX;
  return result;
}

DN_API DN_I16 DN_SaturateCastU64ToI16(DN_U64 val)
{
  DN_I16 result = DN_Check(val <= INT16_MAX) ? DN_CAST(DN_I16) val : INT16_MAX;
  return result;
}

DN_API DN_I32 DN_SaturateCastU64ToI32(DN_U64 val)
{
  DN_I32 result = DN_Check(val <= INT32_MAX) ? DN_CAST(DN_I32) val : INT32_MAX;
  return result;
}

DN_API int64_t DN_SaturateCastU64ToI64(DN_U64 val)
{
  int64_t result = DN_Check(val <= INT64_MAX) ? DN_CAST(int64_t) val : INT64_MAX;
  return result;
}

// Both operands are unsigned and the lowest rank operand will be promoted to
// match the highest rank operand.
DN_API unsigned int DN_SaturateCastU64ToUInt(DN_U64 val)
{
  unsigned int result = DN_Check(val <= UINT8_MAX) ? DN_CAST(unsigned int) val : UINT_MAX;
  return result;
}

DN_API DN_U8 DN_SaturateCastU64ToU8(DN_U64 val)
{
  DN_U8 result = DN_Check(val <= UINT8_MAX) ? DN_CAST(DN_U8) val : UINT8_MAX;
  return result;
}

DN_API DN_U16 DN_SaturateCastU64ToU16(DN_U64 val)
{
  DN_U16 result = DN_Check(val <= UINT16_MAX) ? DN_CAST(DN_U16) val : UINT16_MAX;
  return result;
}

DN_API DN_U32 DN_SaturateCastU64ToU32(DN_U64 val)
{
  DN_U32 result = DN_Check(val <= UINT32_MAX) ? DN_CAST(DN_U32) val : UINT32_MAX;
  return result;
}

// NOTE: DN_SaturateCastISizeToI* ////////////////////////////////////////////////////////////
// Both operands are signed so the lowest rank operand will be promoted to
// match the highest rank operand.
DN_API int DN_SaturateCastISizeToInt(DN_ISize val)
{
  DN_Assert(val >= INT_MIN && val <= INT_MAX);
  int result = DN_CAST(int) DN_Clamp(val, INT_MIN, INT_MAX);
  return result;
}

DN_API int8_t DN_SaturateCastISizeToI8(DN_ISize val)
{
  DN_Assert(val >= INT8_MIN && val <= INT8_MAX);
  int8_t result = DN_CAST(int8_t) DN_Clamp(val, INT8_MIN, INT8_MAX);
  return result;
}

DN_API DN_I16 DN_SaturateCastISizeToI16(DN_ISize val)
{
  DN_Assert(val >= INT16_MIN && val <= INT16_MAX);
  DN_I16 result = DN_CAST(DN_I16) DN_Clamp(val, INT16_MIN, INT16_MAX);
  return result;
}

DN_API DN_I32 DN_SaturateCastISizeToI32(DN_ISize val)
{
  DN_Assert(val >= INT32_MIN && val <= INT32_MAX);
  DN_I32 result = DN_CAST(DN_I32) DN_Clamp(val, INT32_MIN, INT32_MAX);
  return result;
}

DN_API int64_t DN_SaturateCastISizeToI64(DN_ISize val)
{
  DN_Assert(DN_CAST(int64_t) val >= INT64_MIN && DN_CAST(int64_t) val <= INT64_MAX);
  int64_t result = DN_CAST(int64_t) DN_Clamp(DN_CAST(int64_t) val, INT64_MIN, INT64_MAX);
  return result;
}

// NOTE: DN_SaturateCastISizeToU* ////////////////////////////////////////////////////////////
// If the value is a negative integer, we clamp to 0. Otherwise, we know that
// the value is >=0, we can upcast safely to bounds check against the maximum
// allowed value.
DN_API unsigned int DN_SaturateCastISizeToUInt(DN_ISize val)
{
  unsigned int result = 0;
  if (DN_Check(val >= DN_CAST(DN_ISize) 0)) {
    if (DN_Check(DN_CAST(uintmax_t) val <= UINT_MAX))
      result = DN_CAST(unsigned int) val;
    else
      result = UINT_MAX;
  }
  return result;
}

DN_API DN_U8 DN_SaturateCastISizeToU8(DN_ISize val)
{
  DN_U8 result = 0;
  if (DN_Check(val >= DN_CAST(DN_ISize) 0)) {
    if (DN_Check(DN_CAST(uintmax_t) val <= UINT8_MAX))
      result = DN_CAST(DN_U8) val;
    else
      result = UINT8_MAX;
  }
  return result;
}

DN_API DN_U16 DN_SaturateCastISizeToU16(DN_ISize val)
{
  DN_U16 result = 0;
  if (DN_Check(val >= DN_CAST(DN_ISize) 0)) {
    if (DN_Check(DN_CAST(uintmax_t) val <= UINT16_MAX))
      result = DN_CAST(DN_U16) val;
    else
      result = UINT16_MAX;
  }
  return result;
}

DN_API DN_U32 DN_SaturateCastISizeToU32(DN_ISize val)
{
  DN_U32 result = 0;
  if (DN_Check(val >= DN_CAST(DN_ISize) 0)) {
    if (DN_Check(DN_CAST(uintmax_t) val <= UINT32_MAX))
      result = DN_CAST(DN_U32) val;
    else
      result = UINT32_MAX;
  }
  return result;
}

DN_API DN_U64 DN_SaturateCastISizeToU64(DN_ISize val)
{
  DN_U64 result = 0;
  if (DN_Check(val >= DN_CAST(DN_ISize) 0)) {
    if (DN_Check(DN_CAST(uintmax_t) val <= UINT64_MAX))
      result = DN_CAST(DN_U64) val;
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
  DN_ISize result = DN_CAST(int64_t) DN_Clamp(val, DN_ISIZE_MIN, DN_ISIZE_MAX);
  return result;
}

DN_API int8_t DN_SaturateCastI64ToI8(int64_t val)
{
  DN_Check(val >= INT8_MIN && val <= INT8_MAX);
  int8_t result = DN_CAST(int8_t) DN_Clamp(val, INT8_MIN, INT8_MAX);
  return result;
}

DN_API DN_I16 DN_SaturateCastI64ToI16(int64_t val)
{
  DN_Check(val >= INT16_MIN && val <= INT16_MAX);
  DN_I16 result = DN_CAST(DN_I16) DN_Clamp(val, INT16_MIN, INT16_MAX);
  return result;
}

DN_API DN_I32 DN_SaturateCastI64ToI32(int64_t val)
{
  DN_Check(val >= INT32_MIN && val <= INT32_MAX);
  DN_I32 result = DN_CAST(DN_I32) DN_Clamp(val, INT32_MIN, INT32_MAX);
  return result;
}

DN_API unsigned int DN_SaturateCastI64ToUInt(int64_t val)
{
  unsigned int result = 0;
  if (DN_Check(val >= DN_CAST(int64_t) 0)) {
    if (DN_Check(DN_CAST(uintmax_t) val <= UINT_MAX))
      result = DN_CAST(unsigned int) val;
    else
      result = UINT_MAX;
  }
  return result;
}

DN_API DN_ISize DN_SaturateCastI64ToUSize(int64_t val)
{
  DN_USize result = 0;
  if (DN_Check(val >= DN_CAST(int64_t) 0)) {
    if (DN_Check(DN_CAST(uintmax_t) val <= DN_USIZE_MAX))
      result = DN_CAST(DN_USize) val;
    else
      result = DN_USIZE_MAX;
  }
  return result;
}

DN_API DN_U8 DN_SaturateCastI64ToU8(int64_t val)
{
  DN_U8 result = 0;
  if (DN_Check(val >= DN_CAST(int64_t) 0)) {
    if (DN_Check(DN_CAST(uintmax_t) val <= UINT8_MAX))
      result = DN_CAST(DN_U8) val;
    else
      result = UINT8_MAX;
  }
  return result;
}

DN_API DN_U16 DN_SaturateCastI64ToU16(int64_t val)
{
  DN_U16 result = 0;
  if (DN_Check(val >= DN_CAST(int64_t) 0)) {
    if (DN_Check(DN_CAST(uintmax_t) val <= UINT16_MAX))
      result = DN_CAST(DN_U16) val;
    else
      result = UINT16_MAX;
  }
  return result;
}

DN_API DN_U32 DN_SaturateCastI64ToU32(int64_t val)
{
  DN_U32 result = 0;
  if (DN_Check(val >= DN_CAST(int64_t) 0)) {
    if (DN_Check(DN_CAST(uintmax_t) val <= UINT32_MAX))
      result = DN_CAST(DN_U32) val;
    else
      result = UINT32_MAX;
  }
  return result;
}

DN_API DN_U64 DN_SaturateCastI64ToU64(int64_t val)
{
  DN_U64 result = 0;
  if (DN_Check(val >= DN_CAST(int64_t) 0)) {
    if (DN_Check(DN_CAST(uintmax_t) val <= UINT64_MAX))
      result = DN_CAST(DN_U64) val;
    else
      result = UINT64_MAX;
  }
  return result;
}

DN_API int8_t DN_SaturateCastIntToI8(int val)
{
  DN_Check(val >= INT8_MIN && val <= INT8_MAX);
  int8_t result = DN_CAST(int8_t) DN_Clamp(val, INT8_MIN, INT8_MAX);
  return result;
}

DN_API DN_I16 DN_SaturateCastIntToI16(int val)
{
  DN_Check(val >= INT16_MIN && val <= INT16_MAX);
  DN_I16 result = DN_CAST(DN_I16) DN_Clamp(val, INT16_MIN, INT16_MAX);
  return result;
}

DN_API DN_U8 DN_SaturateCastIntToU8(int val)
{
  DN_U8 result = 0;
  if (DN_Check(val >= DN_CAST(DN_ISize) 0)) {
    if (DN_Check(DN_CAST(uintmax_t) val <= UINT8_MAX))
      result = DN_CAST(DN_U8) val;
    else
      result = UINT8_MAX;
  }
  return result;
}

DN_API DN_U16 DN_SaturateCastIntToU16(int val)
{
  DN_U16 result = 0;
  if (DN_Check(val >= DN_CAST(DN_ISize) 0)) {
    if (DN_Check(DN_CAST(uintmax_t) val <= UINT16_MAX))
      result = DN_CAST(DN_U16) val;
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
    result = DN_CAST(DN_U32) val;
  return result;
}

DN_API DN_U64 DN_SaturateCastIntToU64(int val)
{
  static_assert(sizeof(val) <= sizeof(DN_U64), "Sanity check to allow simplifying of casting");
  DN_U64 result = 0;
  if (DN_Check(val >= 0))
    result = DN_CAST(DN_U64) val;
  return result;
}

// NOTE: DN_Asan ////////////////////////////////////////////////////////////////////////// ////////
static_assert(DN_IsPowerOfTwoAligned(DN_ASAN_POISON_GUARD_SIZE, DN_ASAN_POISON_ALIGNMENT),
              "ASAN poison guard size must be a power-of-two and aligned to ASAN's alignment"
              "requirement (8 bytes)");

DN_API void DN_ASAN_PoisonMemoryRegion(void const volatile *ptr, DN_USize size)
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

DN_API void DN_ASAN_UnpoisonMemoryRegion(void const volatile *ptr, DN_USize size)
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
