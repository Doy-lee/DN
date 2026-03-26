#define DN_BASE_CPP

#if defined(_CLANGD)
  #include "../dn.h"
#endif

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

DN_CPUFeatureDecl g_dn_cpu_feature_decl[DN_CPUFeature_Count];

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

DN_API DN_USize DN_AlignUpPowerOfTwoUSize(DN_USize val)
{
  DN_USize leading_zeros = DN_CountLeadingZerosUSize(val);
  DN_USize bits          = sizeof(DN_USize) * 8 - 1;
  DN_USize result        = leading_zeros == 0 ? SIZE_MAX : 1ULL << (bits - leading_zeros + 1);
  return result;
}

DN_API DN_U64 DN_AlignUpPowerOfTwoU64(DN_U64 val)
{
  DN_U64 leading_zeros = DN_CountLeadingZerosU64(val);
  DN_U64 result        = leading_zeros == 0 ? UINT64_MAX : 1ULL << (63 - leading_zeros + 1);
  return result;
}

DN_API DN_U32 DN_AlignUpPowerOfTwoU32(DN_U32 val)
{
  DN_U32 leading_zeros = DN_CountLeadingZerosU32(val);
  DN_U32 result        = leading_zeros == 0 ? UINT32_MAX : 1ULL << (31 - leading_zeros + 1);
  return result;
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

// NOTE: DN_Asan
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
    DN_LeakTrackAlloc(&g_dn_->leak, result, result->reserve, alloc_can_leak);

  return result;
}

static bool DN_ArenaHasPoison_(DN_ArenaFlags flags)
{
  DN_MSVC_WARNING_PUSH
  DN_MSVC_WARNING_DISABLE(6237) // warning C6237: (<zero> && <expression>) is always zero.  <expression> is never evaluated and might have side effects.
  bool result = DN_ASAN_POISON && DN_BitIsNotSet(flags, DN_ArenaFlags_NoPoison);
  DN_MSVC_WARNING_POP
  return result;
}

static DN_ArenaBlock *DN_ArenaBlockFlagsFromMemFuncs_(DN_U64 reserve, DN_U64 commit, DN_ArenaFlags flags, DN_ArenaMemFuncs mem_funcs)
{
  bool           track_alloc    = (flags & DN_ArenaFlags_NoAllocTrack) == 0;
  bool           alloc_can_leak = flags & DN_ArenaFlags_AllocCanLeak;
  DN_ArenaBlock *result         = DN_ArenaBlockFromMemFuncs_(reserve, commit, track_alloc, alloc_can_leak, mem_funcs);
  if (result && DN_ArenaHasPoison_(flags))
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
  if (block && DN_ArenaHasPoison_(flags))
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

  if (DN_ArenaHasPoison_(arena->flags))
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

  if (DN_ArenaHasPoison_(arena->flags))
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
  bool           poison     = DN_ArenaHasPoison_(arena->flags);
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
    if (poison && DN_BitIsNotSet(arena->flags, DN_ArenaFlags_SimAlloc))
      DN_ASanPoisonMemoryRegion(commit_ptr, commit_size);
    curr->commit              = end_commit;
    arena->stats.info.commit += commit_size;
    arena->stats.hwm.commit   = DN_Max(arena->stats.hwm.commit, arena->stats.info.commit);
  }

  void *result            = DN_Cast(char *) curr + offset_pos;
  curr->used             += alloc_size;
  arena->stats.info.used += alloc_size;
  arena->stats.hwm.used   = DN_Max(arena->stats.hwm.used, arena->stats.info.used);

  if (poison && DN_BitIsNotSet(arena->flags, DN_ArenaFlags_SimAlloc))
    DN_ASanUnpoisonMemoryRegion(result, size);

  if (z_mem == DN_ZMem_Yes && DN_BitIsNotSet(arena->flags, DN_ArenaFlags_SimAlloc)) {
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
  if (DN_ArenaHasPoison_(arena->flags)) {
    char    *poison_ptr  = (char *)curr + DN_AlignUpPowerOfTwo(curr->used, DN_ASAN_POISON_ALIGNMENT);
    DN_USize poison_size = ((char *)curr + curr->commit) - poison_ptr;
    DN_ASanPoisonMemoryRegion(poison_ptr, poison_size);
  }
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
  // NOTE: Blocks, Used, Commit, Reserve
  result             = DN_Str8x64FromFmt("B=%u U=%.*s C=%.*s R=%.*s", DN_Cast(DN_U32)info.blocks, DN_Str8PrintFmt(used), DN_Str8PrintFmt(commit), DN_Str8PrintFmt(reserve));
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

static void DN_ErrSinkCheck_(DN_ErrSink const *err)
{
  DN_Assert(err->arena);
  if (err->stack_size == 0)
    return;

  DN_ErrSinkNode const *node = err->stack + (err->stack_size - 1);
  DN_Assert(node->mode >= DN_ErrSinkMode_Nil && node->mode <= DN_ErrSinkMode_ExitOnError);
  DN_Assert(node->msg_sentinel);

  // NOTE: Walk the list ensuring we eventually terminate at the sentinel (e.g. we have a
  // well formed doubly-linked-list terminated by a sentinel, or otherwise we will hit the
  // walk limit or dereference a null pointer and assert)
  size_t WALK_LIMIT = 99'999;
  size_t walk       = 0;
  for (DN_ErrSinkMsg *it = node->msg_sentinel->next; it != node->msg_sentinel; it = it->next, walk++) {
    DN_AssertF(it, "Encountered null pointer which should not happen in a sentinel DLL");
    DN_Assert(walk < WALK_LIMIT);
  }
}

DN_API DN_ErrSink* DN_ErrSinkBegin_(DN_ErrSink *err, DN_ErrSinkMode mode, DN_CallSite call_site)
{
  DN_USize arena_pos = DN_ArenaPos(err->arena);
  if (err->stack_size == DN_ArrayCountU(err->stack)) {
    DN_Str8Builder builder = DN_Str8BuilderFromArena(err->arena);
    for (DN_ForItSize(it, DN_ErrSinkNode, err->stack, err->stack_size))
      DN_Str8BuilderAppendF(&builder, "  [%04zu] %.*s:%u %.*s\n", it.index, DN_Str8PrintFmt(it.data->call_site.file), it.data->call_site.line, DN_Str8PrintFmt(it.data->call_site.function));
    DN_Str8 msg = DN_Str8BuilderBuild(&builder, err->arena);
    DN_AssertF(err->stack_size < DN_ArrayCountU(err->stack), "Error sink has run out of error scopes, potential leak. Scopes were\n%.*s", DN_Str8PrintFmt(msg));
    DN_ArenaPopTo(err->arena, arena_pos);
  }

  DN_ErrSinkNode *node = err->stack + err->stack_size++;
  node->arena_pos      = arena_pos;
  node->mode           = mode;
  node->call_site      = call_site;
  DN_SentinelDoublyLLInitArena(node->msg_sentinel, DN_ErrSinkMsg, err->arena);

  // NOTE: Handle allocation error
  if (!DN_Check(node && node->msg_sentinel)) {
    DN_ArenaPopTo(err->arena, arena_pos);
    node->msg_sentinel = nullptr;
    err->stack_size--;
  }
  DN_ErrSink *result = err;
  return result;
}

DN_API bool DN_ErrSinkHasError(DN_ErrSink *err)
{
  bool result = false;
  if (err && err->stack_size) {
    DN_ErrSinkNode *node = err->stack + (err->stack_size - 1);
    result               = DN_SentinelDoublyLLHasItems(node->msg_sentinel);
  }
  return result;
}

DN_API DN_ErrSinkMsg *DN_ErrSinkEnd(DN_Arena *arena, DN_ErrSink *err)
{
  DN_ErrSinkMsg *result = nullptr;
  DN_ErrSinkCheck_(err);
  DN_AssertF(arena != err->arena, "You are not allowed to reuse the arena for ending the error sink because the memory would get popped and lost");

  // NOTE: Walk the list and allocate it onto the user's arena
  DN_ErrSinkNode *node = err->stack + (err->stack_size - 1);
  DN_ErrSinkMsg  *prev = nullptr;
  for (DN_ErrSinkMsg *it = node->msg_sentinel->next; it != node->msg_sentinel; it = it->next) {
    DN_ErrSinkMsg *entry = DN_ArenaNew(arena, DN_ErrSinkMsg, DN_ZMem_Yes);
    entry->msg           = DN_Str8FromStr8Arena(arena, it->msg);
    entry->call_site     = it->call_site;
    entry->error_code    = it->error_code;
    if (!result)
      result = entry; // Assign first entry if we haven't yet
    if (prev)
      prev->next = entry; // Link the prev message to the current one
    prev = entry;         // Update prev to latest
  }

  // NOTE: Deallocate all the memory for this scope
  err->stack_size--;
  DN_ArenaPopTo(err->arena, node->arena_pos);
  return result;
}

static void DN_ErrSinkAddMsgToStr8Builder_(DN_Str8Builder *builder, DN_ErrSinkMsg *msg, DN_ErrSinkMsg *end)
{
  if (msg == end) // NOTE: No error messages to add
    return;

  if (msg->next == end) {
    DN_ErrSinkMsg *it        = msg;
    DN_Str8        file_name = DN_Str8FileNameFromPath(it->call_site.file);
    DN_Str8BuilderAppendF(builder,
                           "%.*s:%05I32u:%.*s %.*s",
                           DN_Str8PrintFmt(file_name),
                           it->call_site.line,
                           DN_Str8PrintFmt(it->call_site.function),
                           DN_Str8PrintFmt(it->msg));
  } else {
    // NOTE: More than one message
    for (DN_ErrSinkMsg *it = msg; it != end; it = it->next) {
      DN_Str8 file_name = DN_Str8FileNameFromPath(it->call_site.file);
      DN_Str8BuilderAppendF(builder,
                             "%s  - %.*s:%05I32u:%.*s%s%.*s",
                             it == msg ? "" : "\n",
                             DN_Str8PrintFmt(file_name),
                             it->call_site.line,
                             DN_Str8PrintFmt(it->call_site.function),
                             it->msg.size ? " " : "",
                             DN_Str8PrintFmt(it->msg));
    }
  }
}

DN_API DN_Str8 DN_ErrSinkEndStr8(DN_Arena *arena, DN_ErrSink *err)
{
  DN_Str8 result = {};
  DN_ErrSinkCheck_(err);
  if (err->stack_size == 0)
    return result;

  DN_AssertF(arena != err->arena, "You are not allowed to reuse the arena for ending the error sink because the memory would get popped and lost");

  // NOTE: Walk the list and allocate it onto the user's arena
  DN_Str8Builder  builder = DN_Str8BuilderFromArena(err->arena);
  DN_ErrSinkNode *node    = err->stack + (err->stack_size - 1);
  DN_ErrSinkAddMsgToStr8Builder_(&builder, node->msg_sentinel->next, node->msg_sentinel);

  // NOTE: Deallocate all the memory for this scope
  err->stack_size--;
  DN_U64 arena_pos = node->arena_pos;
  DN_ArenaPopTo(err->arena, arena_pos);

  result = DN_Str8BuilderBuild(&builder, arena);
  return result;
}

DN_API void DN_ErrSinkEndIgnore(DN_ErrSink *err)
{
  DN_ErrSinkEnd(nullptr, err);
}

DN_API bool DN_ErrSinkEndLogError_(DN_ErrSink *err, DN_CallSite call_site, DN_Str8 err_msg)
{
  DN_ErrSinkNode *node = err->stack + (err->stack_size - 1);
  DN_AssertF(err->stack_size, "Begin must be called before calling end");
  DN_AssertF(node->msg_sentinel, "Begin must be called before calling end");
  bool result = false;
  if (node->msg_sentinel != node->msg_sentinel->next) {
    result = true;
    // NOTE: Build the error string
    DN_Str8Builder builder = DN_Str8BuilderFromArena(err->arena);
    {
      if (err_msg.size) {
        DN_Str8BuilderAppendRef(&builder, err_msg);
        DN_Str8BuilderAppendRef(&builder, DN_Str8Lit(":"));
      } else {
        DN_Str8BuilderAppendRef(&builder, DN_Str8Lit("Error(s) encountered:"));
      }
      if (node->msg_sentinel->next->next != node->msg_sentinel) // NOTE: More than 1 message
        DN_Str8BuilderAppendRef(&builder, DN_Str8Lit("\n"));
      DN_ErrSinkAddMsgToStr8Builder_(&builder, node->msg_sentinel->next, node->msg_sentinel);
    }

    // NOTE: Log the error
    DN_Str8 log = DN_Str8BuilderBuild(&builder, err->arena);
    DN_LogPrint(DN_LogTypeParamFromType(DN_LogType_Error), call_site, "%.*s", DN_Str8PrintFmt(log));

    if (node->mode == DN_ErrSinkMode_DebugBreakOnErrorLog)
      DN_DebugBreak;

    // NOTE: Deallocate the error node's memory and pop it from the stack
    DN_ArenaPopTo(err->arena, node->arena_pos);
    err->stack_size--;
  }
  return result;
}

DN_API bool DN_ErrSinkEndLogErrorFV_(DN_ErrSink *err, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_Str8 log    = DN_Str8FromFmtVArena(err->arena, fmt, args);
  bool    result = DN_ErrSinkEndLogError_(err, call_site, log);
  return result;
}

DN_API bool DN_ErrSinkEndLogErrorF_(DN_ErrSink *err, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8    log    = DN_Str8FromFmtVArena(err->arena, fmt, args);
  bool       result = DN_ErrSinkEndLogError_(err, call_site, log);
  va_end(args);
  return result;
}

DN_API void DN_ErrSinkEndExitIfErrorFV_(DN_ErrSink *err, DN_CallSite call_site, DN_U32 exit_val, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  if (DN_ErrSinkEndLogErrorFV_(err, call_site, fmt, args)) {
    DN_DebugBreak;
    DN_OS_Exit(exit_val);
  }
}

DN_API void DN_ErrSinkEndExitIfErrorF_(DN_ErrSink *err, DN_CallSite call_site, DN_U32 exit_val, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_ErrSinkEndExitIfErrorFV_(err, call_site, exit_val, fmt, args);
  va_end(args);
}

DN_API void DN_ErrSinkAppendFV_(DN_ErrSink *err, DN_U32 error_code, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  if (!err)
    return;

  DN_Assert(err->stack_size);
  DN_ErrSinkNode *node = err->stack + (err->stack_size - 1);
  DN_AssertF(node, "Error sink must be begun by calling 'Begin' before using this function.");

  DN_ErrSinkMsg *msg = DN_ArenaNew(err->arena, DN_ErrSinkMsg, DN_ZMem_Yes);
  if (DN_Check(msg)) {
    msg->msg        = DN_Str8FromFmtVArena(err->arena, fmt, args);
    msg->error_code = error_code;
    msg->call_site  = call_site;
    DN_SentinelDoublyLLPrepend(node->msg_sentinel, msg);
    if (node->mode == DN_ErrSinkMode_ExitOnError)
      DN_ErrSinkEndExitIfErrorF_(err, msg->call_site, error_code, "Fatal error %u", error_code);
  }
}

DN_API void DN_ErrSinkAppendF_(DN_ErrSink *err, DN_U32 error_code, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_ErrSinkAppendFV_(err, error_code, call_site, fmt, args);
  va_end(args);
}

DN_THREAD_LOCAL DN_TCCore *g_dn_thread_context;

DN_API void DN_TCInit(DN_TCCore *tc, DN_U64 thread_id, DN_Arena *main_arena, DN_Arena *temp_a_arena, DN_Arena *temp_b_arena, DN_Arena *err_sink_arena)
{
  tc->thread_id      = thread_id;
  tc->main_arena     = main_arena;
  tc->temp_a_arena   = temp_a_arena;
  tc->temp_b_arena   = temp_b_arena;
  tc->err_sink.arena = err_sink_arena;
}

DN_API void DN_TCInitFromMemFuncs(DN_TCCore *tc, DN_U64 thread_id, DN_TCInitArgs *args, DN_ArenaMemFuncs mem_funcs)
{
  DN_U64 main_reserve     = (args && args->main_reserve)     ? args->main_reserve     : DN_Kilobytes(64);
  DN_U64 main_commit      = (args && args->main_commit)      ? args->main_commit      : DN_Kilobytes(4);
  DN_U64 temp_reserve     = (args && args->temp_reserve)     ? args->temp_reserve     : DN_Kilobytes(64);
  DN_U64 temp_commit      = (args && args->temp_commit)      ? args->temp_commit      : DN_Kilobytes(4);
  DN_U64 err_sink_reserve = (args && args->err_sink_reserve) ? args->err_sink_reserve : DN_Kilobytes(64);
  DN_U64 err_sink_commit  = (args && args->err_sink_commit)  ? args->err_sink_commit  : DN_Kilobytes(4);

  tc->main_arena_         = DN_ArenaFromMemFuncs(main_reserve,     main_commit,     DN_ArenaFlags_AllocCanLeak | DN_ArenaFlags_NoAllocTrack, mem_funcs);
  tc->temp_a_arena_       = DN_ArenaFromMemFuncs(temp_reserve,     temp_commit,     DN_ArenaFlags_AllocCanLeak | DN_ArenaFlags_NoAllocTrack, mem_funcs);
  tc->temp_b_arena_       = DN_ArenaFromMemFuncs(temp_reserve,     temp_commit,     DN_ArenaFlags_AllocCanLeak | DN_ArenaFlags_NoAllocTrack, mem_funcs);
  tc->err_sink_arena_     = DN_ArenaFromMemFuncs(err_sink_reserve, err_sink_commit, DN_ArenaFlags_AllocCanLeak | DN_ArenaFlags_NoAllocTrack, mem_funcs);

  DN_TCInit(tc, thread_id, &tc->main_arena_, &tc->temp_a_arena_, &tc->temp_b_arena_, &tc->err_sink_arena_);
}

DN_API void DN_TCDeinit(DN_TCCore *tc)
{
  DN_ArenaDeinit(tc->main_arena);
  DN_ArenaDeinit(tc->temp_a_arena);
  DN_ArenaDeinit(tc->temp_b_arena);
  DN_ArenaDeinit(tc->err_sink.arena);
}

DN_API void DN_TCEquip(DN_TCCore *tc)
{
  g_dn_thread_context = tc;
}

DN_API DN_TCCore *DN_TCGet()
{
  DN_RawAssert(g_dn_thread_context &&
               "This thread's thread context has not been equipped yet. Ensure that DN_TCInit(...) "
               "has been called to create a thread context and call DN_TCEquip(...) in the current "
               "thread to make it retrievable via this function");
  return g_dn_thread_context;
}

DN_API DN_Arena *DN_TCMainArena()
{
  DN_TCCore *tc     = DN_TCGet();
  DN_Arena  *result = tc->main_arena;
  return result;
}

DN_API DN_Arena *DN_TCTempArena(DN_Arena **conflicts, DN_USize count)
{
  DN_TCCore *tc           = DN_TCGet();
  DN_Arena  *candidates[] = {tc->temp_a_arena, tc->temp_b_arena};
  DN_Arena  *result       = nullptr;
  for (DN_ForItCArray(it, DN_Arena *, candidates)) {
    bool is_usable = false;
    for (DN_ForItSize(conflict_it, DN_Arena *, conflicts, count)) {
      if (*conflict_it.data == *it.data)
        continue;
      is_usable = true;
      break;
    }

    if (count == 0 || is_usable) {
      result = *it.data;
      break;
    }
  }
  return result;
}

#if defined(__cplusplus)
DN_TCScratchCpp::DN_TCScratchCpp(DN_Arena **conflicts, DN_USize count)
{
  this->data = DN_TCScratchBegin(conflicts, count);
}

DN_TCScratchCpp::~DN_TCScratchCpp()
{
  DN_TCScratchEnd(&this->data);
}
#endif

DN_API DN_TCScratch DN_TCScratchBegin(DN_Arena **conflicts, DN_USize count)
{
  DN_TCScratch result = {};
  result.arena        = DN_TCTempArena(conflicts, count);
  result.temp_mem     = DN_ArenaTempMemBegin(result.arena);
  return result;
}

DN_API void DN_TCScratchEnd(DN_TCScratch *scratch)
{
  DN_Assert(scratch->destructed == false);
  DN_ArenaTempMemEnd(scratch->temp_mem);
  scratch->destructed = true;
  scratch->arena      = nullptr;
  scratch->temp_mem   = {};
}

DN_API void DN_TCSetFrameArena(DN_Arena *arena)
{
  DN_TCCore *tc   = DN_TCGet();
  tc->frame_arena = arena;
}

DN_API DN_Arena *DN_TCFrameArena()
{
  DN_TCCore *tc     = DN_TCGet();
  DN_Arena  *result = tc->frame_arena;
  return result;
}

DN_API DN_ErrSink *DN_TCErrSink()
{
  DN_TCCore  *tc     = DN_TCGet();
  DN_ErrSink *result = &tc->err_sink;
  return result;
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

DN_API void DN_ByteSwapU64Ptr(DN_U8 *dest, DN_U64 src)
{
  dest[0] = DN_Cast(DN_U8)((src >> 56) & 0xFF);
  dest[1] = DN_Cast(DN_U8)((src >> 48) & 0xFF);
  dest[2] = DN_Cast(DN_U8)((src >> 40) & 0xFF);
  dest[3] = DN_Cast(DN_U8)((src >> 32) & 0xFF);
  dest[4] = DN_Cast(DN_U8)((src >> 24) & 0xFF);
  dest[5] = DN_Cast(DN_U8)((src >> 16) & 0xFF);
  dest[6] = DN_Cast(DN_U8)((src >> 8) & 0xFF);
  dest[7] = DN_Cast(DN_U8)(src & 0xFF);
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
  DN_FmtAppendResult result         = {};
  DN_USize           starting_size  = *buf_size;
  result.size_req                   = DN_VSNPrintF(buf + *buf_size, DN_Cast(int)(buf_max - *buf_size), fmt, args);
  *buf_size                        += result.size_req;
  if (*buf_size >= (buf_max - 1))
    *buf_size = buf_max - 1;
  DN_Assert(*buf_size <= (buf_max - 1));
  result.str8      = DN_Str8FromPtr(buf, *buf_size);
  result.truncated = result.str8.size != (starting_size + result.size_req);
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

DN_API DN_Str8 DN_Str8AllocArena(DN_Arena *arena, DN_USize size, DN_ZMem z_mem)
{
  DN_Str8 result = {};
  result.data    = DN_ArenaNewArray(arena, char, size + 1, z_mem);
  if (result.data)
    result.size = size;
  result.data[result.size] = 0;
  return result;
}

DN_API DN_Str8 DN_Str8AllocPool(DN_Pool *pool, DN_USize size)
{
  DN_Str8 result = {};
  result.data    = DN_PoolNewArray(pool, char, size + 1);
  if (result.data)
    result.size = size;
  result.data[result.size] = 0;
  return result;
}

DN_API DN_Str8 DN_Str8FromCStr8(char const *src)
{
  DN_USize size   = DN_CStr8Size(src);
  DN_Str8  result = DN_Str8FromPtr(src, size);
  return result;
}

DN_API DN_Str8 DN_Str8FromPtrArena(DN_Arena *arena, void const *data, DN_USize size)
{
  DN_Str8 result = DN_Str8AllocArena(arena, size, DN_ZMem_No);
  if (result.size)
    DN_Memcpy(result.data, data, size);
  return result;
}

DN_API DN_Str8 DN_Str8FromPtrPool(DN_Pool *pool, void const *data, DN_USize size)
{
  DN_Str8 result = DN_Str8AllocPool(pool, size);
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
  DN_Str8  result = DN_Str8AllocArena(arena, size, DN_ZMem_No);
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
  DN_Str8  result = DN_Str8AllocPool(pool, size);
  if (result.data) {
    DN_USize written = 0;
    DN_FmtVAppend(result.data, &written, result.size + 1, fmt, args);
    DN_Assert(written == result.size);
  }
  va_end(args);
  return result;
}

DN_API DN_Str8x16 DN_Str8x16FromFmt(DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8x16 result = {};
  DN_FmtVAppend(result.data, &result.size, sizeof(result.data), fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8x16 DN_Str8x16FromFmtV(DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_Str8x16 result = {};
  DN_FmtVAppend(result.data, &result.size, sizeof(result.data), fmt, args);
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

DN_API DN_Str8x32 DN_Str8x32FromFmtV(DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_Str8x32 result = {};
  DN_FmtVAppend(result.data, &result.size, sizeof(result.data), fmt, args);
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

DN_API DN_Str8x64 DN_Str8x64FromFmtV(DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_Str8x64 result = {};
  DN_FmtVAppend(result.data, &result.size, sizeof(result.data), fmt, args);
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

DN_API DN_Str8x128 DN_Str8x128FromFmtV(DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_Str8x128 result = {};
  DN_FmtVAppend(result.data, &result.size, sizeof(result.data), fmt, args);
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

DN_API DN_Str8x256 DN_Str8x256FromFmtV(DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_Str8x256 result = {};
  DN_FmtVAppend(result.data, &result.size, sizeof(result.data), fmt, args);
  return result;
}

DN_API void DN_Str8x16AppendFmt(DN_Str8x16 *str, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8x16AppendFmtV(str, fmt, args);
  va_end(args);
}

DN_API void DN_Str8x16AppendFmtV(DN_Str8x16 *str, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_FmtVAppend(str->data, &str->size, sizeof(str->data), fmt, args);
}

DN_API void DN_Str8x32AppendFmt(DN_Str8x32 *str, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8x32AppendFmtV(str, fmt, args);
  va_end(args);
}

DN_API void DN_Str8x32AppendFmtV(DN_Str8x32 *str, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_FmtVAppend(str->data, &str->size, sizeof(str->data), fmt, args);
}

DN_API void DN_Str8x64AppendFmt(DN_Str8x64 *str, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8x64AppendFmtV(str, fmt, args);
  va_end(args);
}

DN_API void DN_Str8x64AppendFmtV(DN_Str8x64 *str, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_FmtVAppend(str->data, &str->size, sizeof(str->data), fmt, args);
}

DN_API void DN_Str8x128AppendFmt(DN_Str8x128 *str, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8x128AppendFmtV(str, fmt, args);
  va_end(args);
}

DN_API void DN_Str8x128AppendFmtV(DN_Str8x128 *str, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_FmtVAppend(str->data, &str->size, sizeof(str->data), fmt, args);
}

DN_API void DN_Str8x256AppendFmt(DN_Str8x256 *str, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8x256AppendFmtV(str, fmt, args);
  va_end(args);
}

DN_API void DN_Str8x256AppendFmtV(DN_Str8x256 *str, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_FmtVAppend(str->data, &str->size, sizeof(str->data), fmt, args);
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

DN_API DN_Str8 DN_Str8Subset(DN_Str8 string, DN_USize offset, DN_USize size)
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
  DN_Str8 result = DN_Str8Subset(string, amount, DN_USIZE_MAX);
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
      DN_Str8 string_slice = DN_Str8Subset(string, index, find_item.size);
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
      DN_Str8 string_slice = DN_Str8Subset(string, index, find_item.size);
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
      DN_Str8 string_slice = DN_Str8Subset(string, index, find_item.size);
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

DN_API DN_Str8FindResult DN_Str8Find(DN_Str8 string, DN_Str8FindFlag flags)
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
  DN_Str8  result          = DN_Str8AllocArena(arena, src.size + segments, DN_ZMem_Yes);
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
  DN_Str8  result          = DN_Str8AllocArena(arena, src.size + segments, DN_ZMem_Yes);
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
  DN_Str8 result = DN_Str8AllocArena(arena, string.size + append.size, DN_ZMem_No);
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
  DN_Str8 result = DN_Str8AllocArena(arena, count * fill.size, DN_ZMem_No);
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

  DN_Str8 head     = DN_Str8Subset(str8, 0, side_size);
  DN_Str8 tail     = DN_Str8Subset(str8, str8.size - side_size, side_size);
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

DN_API DN_Str8 DN_Str8Replace(DN_Str8       string,
                              DN_Str8       find,
                              DN_Str8       replace,
                              DN_USize      start_index,
                              DN_Arena     *arena,
                              DN_Str8EqCase eq_case)
{
  // TODO: Implement this without requiring TLS so it can go into base strings
  DN_Str8 result = {};
  if (string.size == 0 || find.size == 0 || find.size > string.size || find.size == 0 || string.size == 0) {
    result = DN_Str8FromStr8Arena(arena, string);
    return result;
  }

  DN_TCScratch   scratch        = DN_TCScratchBegin(&arena, 1);
  DN_Str8Builder string_builder = DN_Str8BuilderFromArena(scratch.arena);
  DN_USize       max            = string.size - find.size;
  DN_USize       head           = start_index;

  for (DN_USize tail = head; tail <= max; tail++) {
    DN_Str8 check = DN_Str8Subset(string, tail, find.size);
    if (!DN_Str8Eq(check, find, eq_case))
      continue;

    if (start_index > 0 && string_builder.string_size == 0) {
      // User provided a hint in the string to start searching from, we
      // need to add the string up to the hint. We only do this if there's
      // a replacement action, otherwise we have a special case for no
      // replacements, where the entire string gets copied.
      DN_Str8 slice = DN_Str8FromPtr(string.data, head);
      DN_Str8BuilderAppendRef(&string_builder, slice);
    }

    DN_Str8 range = DN_Str8Subset(string, head, (tail - head));
    DN_Str8BuilderAppendRef(&string_builder, range);
    DN_Str8BuilderAppendRef(&string_builder, replace);
    head = tail + find.size;
    tail += find.size - 1; // NOTE: -1 since the for loop will post increment us past the end of the find string
  }

  if (string_builder.string_size == 0) {
    // NOTE: No replacement possible, so we just do a full-copy
    result = DN_Str8FromStr8Arena(arena, string);
  } else {
    DN_Str8 remainder = DN_Str8FromPtr(string.data + head, string.size - head);
    DN_Str8BuilderAppendRef(&string_builder, remainder);
    result = DN_Str8BuilderBuild(&string_builder, arena);
  }
  DN_TCScratchEnd(&scratch);
  return result;
}

DN_API DN_Str8 DN_Str8ReplaceSensitive(DN_Str8 string, DN_Str8 find, DN_Str8 replace, DN_USize start_index, DN_Arena *arena)
{
  DN_Str8 result = DN_Str8Replace(string, find, replace, start_index, arena, DN_Str8EqCase_Sensitive);
  return result;
}

DN_API DN_Str8 DN_Str8ReplaceInsensitive(DN_Str8 string, DN_Str8 find, DN_Str8 replace, DN_USize start_index, DN_Arena *arena)
{
  DN_Str8 result = DN_Str8Replace(string, find, replace, start_index, arena, DN_Str8EqCase_Insensitive);
  return result;
}

DN_API DN_Str8 DN_Str8SliceRender(DN_Str8Slice slice, DN_Str8 separator, DN_Arena *arena)
{
  DN_Str8 result = {};
  if (!arena)
    return result;

  DN_USize total_size = 0;
  for (DN_USize index = 0; index < slice.count; index++) {
    if (index)
      total_size += separator.size;
    DN_Str8 item = slice.data[index];
    total_size += item.size;
  }

  result = DN_Str8AllocArena(arena, total_size, DN_ZMem_No);
  if (result.data) {
    DN_USize write_index = 0;
    for (DN_USize index = 0; index < slice.count; index++) {
      if (index) {
        DN_Memcpy(result.data + write_index, separator.data, separator.size);
        write_index += separator.size;
      }
      DN_Str8 item = slice.data[index];
      DN_Memcpy(result.data + write_index, item.data, item.size);
      write_index += item.size;
    }
  }

  return result;
}

DN_API DN_Str8 DN_Str8RenderSpaceSep(DN_Str8Slice slice, DN_Arena *arena)
{
  DN_Str8 result = DN_Str8SliceRender(slice, DN_Str8Lit(" "), arena);
  return result;
}

DN_API DN_Str16 DN_Str16SliceRender(DN_Str16Slice slice, DN_Str16 separator, DN_Arena *arena)
{
  DN_Str16 result = {};
  if (!arena)
    return result;

  DN_USize total_size = 0;
  for (DN_USize index = 0; index < slice.count; index++) {
    if (index)
      total_size += separator.size;
    DN_Str16 item = slice.data[index];
    total_size += item.size;
  }

  result = {DN_ArenaNewArray(arena, wchar_t, total_size + 1, DN_ZMem_No), total_size};
  if (result.data) {
    DN_USize write_index = 0;
    for (DN_USize index = 0; index < slice.count; index++) {
      if (index) {
        DN_Memcpy(result.data + write_index, separator.data, separator.size * sizeof(result.data[0]));
        write_index += separator.size;
      }
      DN_Str16 item = slice.data[index];
      DN_Memcpy(result.data + write_index, item.data, item.size * sizeof(result.data[0]));
      write_index += item.size;
    }
  }

  result.data[total_size] = 0;
  return result;
}

DN_API DN_Str16 DN_Str16RenderSpaceSep(DN_Str16Slice slice, DN_Arena *arena)
{
  DN_Str16 result = DN_Str16SliceRender(slice, DN_Str16Lit(L" "), arena);
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

DN_API DN_Str8Slice DN_Str8BuilderBuildSlice(DN_Str8Builder const *builder, DN_Arena *arena)
{
  DN_Str8Slice result = {};
  if (!builder || builder->string_size <= 0 || builder->count <= 0)
    return result;

  if (!DN_ISliceAllocArena(DN_Str8, &result, builder->count, DN_ZMem_No, arena))
    return result;

  DN_USize slice_index = 0;
  for (DN_Str8Link *link = builder->head; link; link = link->next)
    result.data[slice_index++] = DN_Str8FromStr8Arena(arena, link->string);

  DN_Assert(slice_index == builder->count);
  return result;
}

// NOTE: DN_UTF
DN_API int DN_UTF8Encode(DN_U8 utf8[4], DN_U32 codepoint)
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

DN_API DN_UTF8DecodeResult DN_UTF8Decode(DN_Str8 stream)
{
  DN_UTF8DecodeResult result = {};
  result.remaining           = stream;
  if (stream.size <= 0)
    return result;

  DN_U8 b0 = DN_Cast(DN_U8)stream.data[0];
  DN_U8 b1 = DN_Cast(DN_U8)(stream.size >= 2 ? stream.data[1] : 0);
  DN_U8 b2 = DN_Cast(DN_U8)(stream.size >= 3 ? stream.data[2] : 0);
  DN_U8 b3 = DN_Cast(DN_U8)(stream.size >= 4 ? stream.data[3] : 0);

  if ((b0 & 0b1000'0000) == 0) {
    result.codepoint = b0;
    result.success   = true;
    result.remaining = DN_Str8FromPtr(stream.data + 1, stream.size - 1);
    return result;
  }

  if ((b0 & 0b1110'0000) == 0b1100'0000) {
    if (stream.size < 2)
      return result;
    if ((b1 & 0b1100'0000) != 0b1000'0000)
      return result;
    DN_U32 cp = ((b0 & 0b0001'1111) << 6) | ((b1 & 0b0011'1111) << 0);
    if (cp < 0x80)
      return result;
    result.codepoint  = cp;
    result.success   = true;
    result.remaining = DN_Str8FromPtr(stream.data + 2, stream.size - 2);
    return result;
  }

  if ((b0 & 0b1111'0000) == 0b1110'0000) {
    if (stream.size < 3)
      return result;
    if ((b1 & 0b1100'0000) != 0b1000'0000)
      return result;
    if ((b2 & 0b1100'0000) != 0b1000'0000)
      return result;
    DN_U32 cp = ((b0 & 0b0000'1111) << 12) | ((b1 & 0b0011'1111) << 6) | ((b2 & 0b0011'1111) << 0);
    if (cp < 0x800)
      return result;
    result.codepoint  = cp;
    result.success   = true;
    result.remaining = DN_Str8FromPtr(stream.data + 3, stream.size - 3);
    return result;
  }

  if ((b0 & 0b1111'1000) == 0b1111'0000) {
    if (stream.size < 4)
      return result;
    if ((b1 & 0b1100'0000) != 0b1000'0000)
      return result;
    if ((b2 & 0b1100'0000) != 0b1000'0000)
      return result;
    if ((b3 & 0b1100'0000) != 0b1000'0000)
      return result;
    DN_U32 cp = ((b0 & 0b0000'0111) << 18)  |
                 ((b1 & 0b0011'1111) << 12) |
                 ((b2 & 0b0011'1111) << 6)  |
                 ((b3 & 0b0011'1111) << 0);
    if (cp < 0x10000 || cp > 0x10FFFF)
      return result;
    result.codepoint = cp;
    result.success   = true;
    result.remaining = DN_Str8FromPtr(stream.data + 4, stream.size - 4);
    return result;
  }

  return result;
}

DN_API bool DN_UTF8DecodeIterate(DN_UTF8DecodeIterator *it, DN_Str8 utf8)
{
  if (it->init) {
    it->codepoint_index++;
  } else {
    it->remaining = utf8;
    it->init      = true;
  }
  DN_UTF8DecodeResult decode = DN_UTF8Decode(it->remaining);
  it->success                = decode.success;
  it->remaining              = decode.remaining;
  it->codepoint              = decode.codepoint;
  bool result                = it->success;
  return result;
}

DN_API int DN_UTF16Encode(DN_U16 utf16[2], DN_U32 codepoint)
{
  // NOTE: Table from https://www.reedbeta.com/blog/programmers-intro-to-unicode/
  // ----------------------------------------+------------------------------------+------------------+
  // UTF-16 (binary)                         | Code point (binary)                | Range            |
  // ----------------------------------------+------------------------------------+------------------+
  // xxxx'xxxx'xxxx'xxxx                     | xxxx'xxxx'xxxx'xxxx                | U+0000???U+FFFF    |
  // 1101'10xx'xxxx'xxxx 1101'11yy'yyyy'yyyy | xxxx'xxxx'xxyy'yyyy'yyyy + 0x10000 | U+10000???U+10FFFF |
  // ----------------------------------------+------------------------------------+------------------+

  if (codepoint <= 0b1111'1111'1111'1111) {
    utf16[0] = DN_Cast(DN_U16) codepoint;
    return 1;
  }

  if (codepoint <= 0b1111'1111'1111'1111'1111) {
    DN_U32 surrogate_codepoint = codepoint + 0x10000;
    utf16[0]                   = 0b1101'1000'0000'0000 | ((surrogate_codepoint >> 10) & 0b11'1111'1111); // x
    utf16[1]                   = 0b1101'1100'0000'0000 | ((surrogate_codepoint >> 0) & 0b11'1111'1111);  // y
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

  DN_Str8 unit_suffix = {};
  if (units & DN_AgeUnit_Year) {
    unit_suffix             = DN_Str8Lit("y");
    DN_USize   value_usize  = remainder_ms / (DN_SecFromYears(1) * 1000);
    remainder_ms           -= DN_SecFromYears(value_usize) * 1000;
    if (value_usize)
      DN_FmtAppend(result.data, &result.size, sizeof(result.data), "%s%zu%.*s", result.size ? " " : "", value_usize, DN_Str8PrintFmt(unit_suffix));
  }

  if (units & DN_AgeUnit_Week) {
    unit_suffix             = DN_Str8Lit("w");
    DN_USize value_usize    = remainder_ms / (DN_SecFromWeeks(1) * 1000);
    remainder_ms           -= DN_SecFromWeeks(value_usize) * 1000;
    if (value_usize)
      DN_FmtAppend(result.data, &result.size, sizeof(result.data), "%s%zu%.*s", result.size ? " " : "", value_usize, DN_Str8PrintFmt(unit_suffix));
  }

  if (units & DN_AgeUnit_Day) {
    unit_suffix             = DN_Str8Lit("d");
    DN_USize value_usize    = remainder_ms / (DN_SecFromDays(1) * 1000);
    remainder_ms           -= DN_SecFromDays(value_usize) * 1000;
    if (value_usize)
      DN_FmtAppend(result.data, &result.size, sizeof(result.data), "%s%zu%.*s", result.size ? " " : "", value_usize, DN_Str8PrintFmt(unit_suffix));
  }

  if (units & DN_AgeUnit_Hr) {
    unit_suffix             = DN_Str8Lit("h");
    DN_USize value_usize    = remainder_ms / (DN_SecFromHours(1) * 1000);
    remainder_ms           -= DN_SecFromHours(value_usize) * 1000;
    if (value_usize)
      DN_FmtAppend(result.data, &result.size, sizeof(result.data), "%s%zu%.*s", result.size ? " " : "", value_usize, DN_Str8PrintFmt(unit_suffix));
  }

  if (units & DN_AgeUnit_Min) {
    unit_suffix             = DN_Str8Lit("m");
    DN_USize value_usize    = remainder_ms / (DN_SecFromMins(1) * 1000);
    remainder_ms           -= DN_SecFromMins(value_usize) * 1000;
    if (value_usize)
      DN_FmtAppend(result.data, &result.size, sizeof(result.data), "%s%zu%.*s", result.size ? " " : "", value_usize, DN_Str8PrintFmt(unit_suffix));
  }

  if (units & DN_AgeUnit_Sec) {
    unit_suffix = DN_Str8Lit("s");
    if (units & DN_AgeUnit_FractionalSec) {
      DN_F64 remainder_s = remainder_ms / 1000.0;
      DN_FmtAppend(result.data, &result.size, sizeof(result.data), "%s%.3f%.*s", result.size ? " " : "", remainder_s, DN_Str8PrintFmt(unit_suffix));
      remainder_ms = 0;
    } else {
      DN_USize value_usize  = remainder_ms / 1000;
      remainder_ms         -= DN_Cast(DN_USize)(value_usize * 1000);
      if (value_usize)
        DN_FmtAppend(result.data, &result.size, sizeof(result.data), "%s%zu%.*s", result.size ? " " : "", value_usize, DN_Str8PrintFmt(unit_suffix));
    }
  }

  if (units & DN_AgeUnit_Ms) {
    unit_suffix = DN_Str8Lit("ms");
    DN_Assert((units & DN_AgeUnit_FractionalSec) == 0);
    DN_USize value_usize  = remainder_ms;
    remainder_ms         -= value_usize;
    if (value_usize || result.size == 0)
      DN_FmtAppend(result.data, &result.size, sizeof(result.data), "%s%zu%.*s", result.size ? " " : "", value_usize, DN_Str8PrintFmt(unit_suffix));
  }

  if (result.size == 0)
    DN_FmtAppend(result.data, &result.size, sizeof(result.data), "0%.*s", DN_Str8PrintFmt(unit_suffix));
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

DN_API int DN_IsLeapYear(int year)
{
  if (year % 4 != 0)
    return 0;
  if (year % 100 != 0)
    return 1;
  return (year % 400 == 0);
}

DN_API bool DN_DateIsValid(DN_Date date)
{
  if (date.year < 1970)
    return false;
  if (date.month <= 0 || date.month >= 13)
    return false;
  if (date.day <= 0 || date.day >= 32)
    return false;
  if (date.hour >= 24)
    return false;
  if (date.minutes >= 60)
    return false;
  if (date.seconds >= 60)
    return false;
  return true;
}

DN_API DN_Date DN_DateFromUnixTimeMs(DN_USize unix_ts_ms)
{
  DN_Date  result        = {};
  DN_USize ms            = unix_ts_ms % 1000;
  DN_USize total_seconds = unix_ts_ms / 1000;
  result.milliseconds    = (DN_U16)ms;

  DN_USize secs_in_day = total_seconds % 86400;
  DN_USize days        = total_seconds / 86400;

  result.hour    = (DN_U8)(secs_in_day / 3600);
  result.minutes = (DN_U8)((secs_in_day % 3600) / 60);
  result.seconds = (DN_U8)(secs_in_day % 60);

  DN_U16   days_in_month[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  DN_USize days_left         = days;
  DN_U16   year              = 1970;

  while (days_left >= (DN_IsLeapYear(year) ? 366 : 365)) {
    DN_USize days_in_year  = DN_IsLeapYear(year) ? 366 : 365;
    days_left             -= days_in_year;
    year++;
  }

  DN_U8 month = 1;
  for (;;) {
    DN_U16 day_count = days_in_month[month];
    if (month == 2 && DN_IsLeapYear(year))
      day_count = 29;
    if (days_left < day_count)
      break;
    days_left -= day_count;
    month++;
  }

  result.year  = year;
  result.month = month;
  result.day   = (DN_U8)days_left + 1;
  return result;
}

DN_API DN_U64 DN_UnixTimeMsFromDate(DN_Date date)
{
  DN_Assert(DN_DateIsValid(date));

  // Precomputed cumulative days before each month (non-leap year)
  const DN_U16 days_before_month[13] = {
      0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};

  DN_U16 y = date.year;
  DN_U8  m = date.month;
  DN_U8  d = date.day;

  DN_U32 days  = d - 1;                    // day of month starts at 0 internally
  days        += days_before_month[m - 1]; // Add days from previous months this year

  if (m > 2 && DN_IsLeapYear(y)) // Add February 29 if leap year and month > 2
    days += 1;

  // Add full years from 1970 to y-1
  for (DN_U16 year = 1970; year < y; ++year)
    days += DN_IsLeapYear(year) ? 366 : 365;

  // Convert to seconds
  DN_U64 seconds  = DN_Cast(DN_U64)days * 86400ULL;
  seconds        += DN_Cast(DN_U64)date.hour * 3600ULL;
  seconds        += DN_Cast(DN_U64)date.minutes * 60ULL;
  seconds        += DN_Cast(DN_U64)date.seconds;
  DN_U64 result   = seconds * 1000ULL + date.milliseconds;
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

#define DN_PCG_DEFAULT_MULTIPLIER_64 6364136223846793005ULL
#define DN_PCG_DEFAULT_INCREMENT_64  1442695040888963407ULL
DN_API DN_PCG32 DN_PCG32Init(DN_U64 seed)
{
  DN_PCG32 result = {};
  DN_PCG32Next(&result);
  result.state += seed;
  DN_PCG32Next(&result);
  return result;
}

DN_API DN_U32 DN_PCG32Next(DN_PCG32 *rng)
{
  DN_U64 state = rng->state;
  rng->state     = state * DN_PCG_DEFAULT_MULTIPLIER_64 + DN_PCG_DEFAULT_INCREMENT_64;

  // XSH-RR
  DN_U32 value = (DN_U32)((state ^ (state >> 18)) >> 27);
  int      rot   = state >> 59;
  return rot ? (value >> rot) | (value << (32 - rot)) : value;
}

DN_API DN_U64 DN_PCG32Next64(DN_PCG32 *rng)
{
  DN_U64 value = DN_PCG32Next(rng);
  value <<= 32;
  value |= DN_PCG32Next(rng);
  return value;
}

DN_API DN_U32 DN_PCG32Range(DN_PCG32 *rng, DN_U32 low, DN_U32 high)
{
  DN_U32 bound     = high - low;
  DN_U32 threshold = -(DN_I32)bound % bound;

  for (;;) {
    DN_U32 r = DN_PCG32Next(rng);
    if (r >= threshold)
      return low + (r % bound);
  }
}

DN_API DN_F32 DN_PCG32NextF32(DN_PCG32 *rng)
{
  DN_U32 x = DN_PCG32Next(rng);
  return (DN_F32)(DN_I32)(x >> 8) * 0x1.0p-24f;
}

DN_API DN_F64 DN_PCG32NextF64(DN_PCG32 *rng)
{
  DN_U64 x = DN_PCG32Next64(rng);
  return (DN_F64)(int64_t)(x >> 11) * 0x1.0p-53;
}

DN_API void DN_PCG32Advance(DN_PCG32 *rng, DN_U64 delta)
{
  DN_U64 cur_mult = DN_PCG_DEFAULT_MULTIPLIER_64;
  DN_U64 cur_plus = DN_PCG_DEFAULT_INCREMENT_64;

  DN_U64 acc_mult = 1;
  DN_U64 acc_plus = 0;

  while (delta != 0) {
    if (delta & 1) {
      acc_mult *= cur_mult;
      acc_plus = acc_plus * cur_mult + cur_plus;
    }
    cur_plus = (cur_mult + 1) * cur_plus;
    cur_mult *= cur_mult;
    delta >>= 1;
  }

  rng->state = acc_mult * rng->state + acc_plus;
}

// Default values recommended by: http://isthe.com/chongo/tech/comp/fnv/
DN_API DN_U32 DN_FNV1AHashU32FromBytes(void const *bytes, DN_USize size, DN_U32 hash)
{
    auto buffer = DN_Cast(DN_U8 const *)bytes;
    for (DN_USize i = 0; i < size; i++)
        hash = (buffer[i] ^ hash) * 16777619 /*FNV Prime*/;
    return hash;
}

DN_API DN_U64 DN_FNV1AHashU64FromBytes(void const *bytes, DN_USize size, DN_U64 hash)
{
    auto buffer = DN_Cast(DN_U8 const *)bytes;
    for (DN_USize i = 0; i < size; i++)
        hash = (buffer[i] ^ hash) * 1099511628211 /*FNV Prime*/;
    return hash;
}

#if defined(DN_COMPILER_MSVC) || defined(DN_COMPILER_CLANG_CL)
    #define DN_MMH3_ROTL32(x, y) _rotl(x, y)
    #define DN_MMH3_ROTL64(x, y) _rotl64(x, y)
#else
    #define DN_MMH3_ROTL32(x, y) ((x) << (y)) | ((x) >> (32 - (y)))
    #define DN_MMH3_ROTL64(x, y) ((x) << (y)) | ((x) >> (64 - (y)))
#endif

//-----------------------------------------------------------------------------
// Block read - if your platform needs to do endian-swapping or can only
// handle aligned reads, do the conversion here
DN_FORCE_INLINE DN_U32 DN_MurmurHash3GetBlock32_(DN_U32 const *p, int i)
{
    return p[i];
}

DN_FORCE_INLINE DN_U64 DN_MurmurHash3GetBlock64_(DN_U64 const *p, int i)
{
    return p[i];
}

//-----------------------------------------------------------------------------
// Finalization mix - force all bits of a hash block to avalanche

DN_FORCE_INLINE DN_U32 DN_MurmurHash3FMix32_(DN_U32 h)
{
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

DN_FORCE_INLINE DN_U64 DN_MurmurHash3FMix64_(DN_U64 k)
{
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccd;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53;
    k ^= k >> 33;
    return k;
}

DN_API DN_U32 DN_MurmurHash3HashU128FromBytesX86(void const *bytes, int len, DN_U32 seed)
{
    const DN_U8 *data = (const DN_U8 *)bytes;
    const int nblocks   = len / 4;

    DN_U32 h1 = seed;

    const DN_U32 c1 = 0xcc9e2d51;
    const DN_U32 c2 = 0x1b873593;

    //----------
    // body

    const DN_U32 *blocks = (const DN_U32 *)(data + nblocks * 4);

    for (int i = -nblocks; i; i++)
    {
        DN_U32 k1 = DN_MurmurHash3GetBlock32_(blocks, i);

        k1 *= c1;
        k1 = DN_MMH3_ROTL32(k1, 15);
        k1 *= c2;

        h1 ^= k1;
        h1 = DN_MMH3_ROTL32(h1, 13);
        h1 = h1 * 5 + 0xe6546b64;
    }

    //----------
    // tail

    const DN_U8 *tail = (const DN_U8 *)(data + nblocks * 4);

    DN_U32 k1 = 0;

    switch (len & 3)
    {
        case 3:
            k1 ^= tail[2] << 16;
        case 2:
            k1 ^= tail[1] << 8;
        case 1:
            k1 ^= tail[0];
            k1 *= c1;
            k1 = DN_MMH3_ROTL32(k1, 15);
            k1 *= c2;
            h1 ^= k1;
    };

    //----------
    // finalization

    h1 ^= len;

    h1 = DN_MurmurHash3FMix32_(h1);

    return h1;
}

DN_API DN_MurmurHash3 DN_MurmurHash3HashU128FromBytesX64(void const *bytes, int len, DN_U32 seed)
{
    const DN_U8 *data = (const DN_U8 *)bytes;
    const int nblocks   = len / 16;

    DN_U64 h1 = seed;
    DN_U64 h2 = seed;

    const DN_U64 c1 = 0x87c37b91114253d5;
    const DN_U64 c2 = 0x4cf5ad432745937f;

    //----------
    // body

    const DN_U64 *blocks = (const DN_U64 *)(data);

    for (int i = 0; i < nblocks; i++)
    {
        DN_U64 k1 = DN_MurmurHash3GetBlock64_(blocks, i * 2 + 0);
        DN_U64 k2 = DN_MurmurHash3GetBlock64_(blocks, i * 2 + 1);

        k1 *= c1;
        k1 = DN_MMH3_ROTL64(k1, 31);
        k1 *= c2;
        h1 ^= k1;

        h1 = DN_MMH3_ROTL64(h1, 27);
        h1 += h2;
        h1 = h1 * 5 + 0x52dce729;

        k2 *= c2;
        k2 = DN_MMH3_ROTL64(k2, 33);
        k2 *= c1;
        h2 ^= k2;

        h2 = DN_MMH3_ROTL64(h2, 31);
        h2 += h1;
        h2 = h2 * 5 + 0x38495ab5;
    }

    //----------
    // tail

    const DN_U8 *tail = (const DN_U8 *)(data + nblocks * 16);

    DN_U64 k1 = 0;
    DN_U64 k2 = 0;

    switch (len & 15)
    {
        case 15:
            k2 ^= ((DN_U64)tail[14]) << 48;
        case 14:
            k2 ^= ((DN_U64)tail[13]) << 40;
        case 13:
            k2 ^= ((DN_U64)tail[12]) << 32;
        case 12:
            k2 ^= ((DN_U64)tail[11]) << 24;
        case 11:
            k2 ^= ((DN_U64)tail[10]) << 16;
        case 10:
            k2 ^= ((DN_U64)tail[9]) << 8;
        case 9:
            k2 ^= ((DN_U64)tail[8]) << 0;
            k2 *= c2;
            k2 = DN_MMH3_ROTL64(k2, 33);
            k2 *= c1;
            h2 ^= k2;

        case 8:
            k1 ^= ((DN_U64)tail[7]) << 56;
        case 7:
            k1 ^= ((DN_U64)tail[6]) << 48;
        case 6:
            k1 ^= ((DN_U64)tail[5]) << 40;
        case 5:
            k1 ^= ((DN_U64)tail[4]) << 32;
        case 4:
            k1 ^= ((DN_U64)tail[3]) << 24;
        case 3:
            k1 ^= ((DN_U64)tail[2]) << 16;
        case 2:
            k1 ^= ((DN_U64)tail[1]) << 8;
        case 1:
            k1 ^= ((DN_U64)tail[0]) << 0;
            k1 *= c1;
            k1 = DN_MMH3_ROTL64(k1, 31);
            k1 *= c2;
            h1 ^= k1;
    };

    //----------
    // finalization

    h1 ^= len;
    h2 ^= len;

    h1 += h2;
    h2 += h1;

    h1 = DN_MurmurHash3FMix64_(h1);
    h2 = DN_MurmurHash3FMix64_(h2);

    h1 += h2;
    h2 += h1;

    DN_MurmurHash3 result = {};
    result.e[0]            = h1;
    result.e[1]            = h2;
    return result;
}

DN_API DN_U64 DN_MurmurHash3HashU64FromBytesX64(void const *bytes, int len, DN_U32 seed)
{
  DN_MurmurHash3 hash   = DN_MurmurHash3HashU128FromBytesX64(bytes, len, seed);
  DN_U64         result = hash.e[0];
  return result;
}

DN_API DN_U32 DN_MurmurHash3HashU32FromBytesX64(void const *bytes, int len, DN_U32 seed)
{
  DN_MurmurHash3 hash   = DN_MurmurHash3HashU128FromBytesX64(bytes, len, seed);
  DN_U32         result = DN_Cast(DN_U32)hash.e[0];
  return result;
}

DN_API DN_Str8 DN_LogColourEscapeCodeStr8FromRGB(DN_LogColourType colour, DN_U8 r, DN_U8 g, DN_U8 b)
{
  DN_THREAD_LOCAL char buffer[32];
  buffer[0]      = 0;
  DN_Str8 result = {};
  result.size    = DN_SNPrintF(buffer,
                            DN_ArrayCountU(buffer),
                            "\x1b[%d;2;%u;%u;%um",
                            colour == DN_LogColourType_Fg ? 38 : 48,
                            r,
                            g,
                            b);
  result.data    = buffer;
  return result;
}

DN_API DN_Str8 DN_LogColourEscapeCodeStr8FromU32(DN_LogColourType colour, DN_U32 value)
{
  DN_U8   r      = DN_Cast(DN_U8)(value >> 24);
  DN_U8   g      = DN_Cast(DN_U8)(value >> 16);
  DN_U8   b      = DN_Cast(DN_U8)(value >> 8);
  DN_Str8 result = DN_LogColourEscapeCodeStr8FromRGB(colour, r, g, b);
  return result;
}

DN_API DN_LogPrefixSize DN_LogMakePrefix(DN_LogStyle style, DN_LogTypeParam type, DN_CallSite call_site, DN_LogDate date, char *dest, DN_USize dest_size)
{
  DN_Str8 type_str8 = type.str8;
  if (type.is_u32_enum) {
    switch (type.u32) {
      case DN_LogType_Debug:   type_str8 = DN_Str8Lit("DEBUG"); break;
      case DN_LogType_Info:    type_str8 = DN_Str8Lit("INFO "); break;
      case DN_LogType_Warning: type_str8 = DN_Str8Lit("WARN");  break;
      case DN_LogType_Error:   type_str8 = DN_Str8Lit("ERROR"); break;
      case DN_LogType_Count:   type_str8 = DN_Str8Lit("BADXX"); break;
    }
  }

  static DN_USize max_type_length = 0;
  max_type_length                 = DN_Max(max_type_length, type_str8.size);
  int type_padding                = DN_Cast(int)(max_type_length - type_str8.size);

  DN_Str8 colour_esc = {};
  DN_Str8 bold_esc   = {};
  DN_Str8 reset_esc  = {};
  if (style.colour) {
    bold_esc   = DN_Str8Lit(DN_LogBoldEscapeCode);
    reset_esc  = DN_Str8Lit(DN_LogResetEscapeCode);
    colour_esc = DN_LogColourEscapeCodeStr8FromRGB(DN_LogColourType_Fg, style.r, style.g, style.b);
  }

  DN_Str8 file_name = DN_Str8FileNameFromPath(call_site.file);
  DN_GCC_WARNING_PUSH
  DN_GCC_WARNING_DISABLE(-Wformat)
  DN_GCC_WARNING_DISABLE(-Wformat-extra-args)
  DN_MSVC_WARNING_PUSH
  DN_MSVC_WARNING_DISABLE(4477)
  int     size      = DN_SNPrintF(dest,
                         DN_Cast(int)dest_size,
                         "%04u-%02u-%02uT%02u:%02u:%02u" // date
                         "%S"                            // colour
                         "%S"                            // bold
                         " %S"                           // type
                         "%.*s"                          // type padding
                         "%S"                            // reset
                         " %S"                           // file name
                         ":%05I32u "                     // line number
                         ,
                         date.year,
                         date.month,
                         date.day,
                         date.hour,
                         date.minute,
                         date.second,
                         colour_esc,      // colour
                         bold_esc,        // bold
                         type_str8,       // type
                         DN_Cast(int) type_padding,
                         "",              // type padding
                         reset_esc,       // reset
                         file_name,       // file name
                         call_site.line); // line number
  DN_MSVC_WARNING_POP // '%S' requires an argument of type 'wchar_t *', but variadic argument 7 has type 'DN_Str8'
  DN_GCC_WARNING_POP

  static DN_USize max_header_length  = 0;
  DN_USize        size_no_ansi_codes = size - colour_esc.size - reset_esc.size - bold_esc.size;
  max_header_length                  = DN_Max(max_header_length, size_no_ansi_codes);
  DN_USize header_padding            = max_header_length - size_no_ansi_codes;

  DN_LogPrefixSize result = {};
  result.size             = size;
  result.padding          = header_padding;
  return result;
}

DN_API void DN_LogSetPrintFunc(DN_LogPrintFunc *print_func, void *user_data)
{
  DN_Core *dn            = DN_Get();
  dn->print_func         = print_func;
  dn->print_func_context = user_data;
}

DN_API void DN_LogPrint(DN_LogTypeParam type, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, ...)
{
  DN_Core         *dn   = DN_Get();
  DN_LogPrintFunc *func = dn->print_func;
  if (func) {
    va_list args;
    va_start(args, fmt);
    func(type, dn->print_func_context, call_site, fmt, args);
    va_end(args);
  }
}

DN_API DN_LogTypeParam DN_LogTypeParamFromType(DN_LogType type)
{
  DN_LogTypeParam result = {};
  result.is_u32_enum     = true;
  result.u32             = type;
  return result;
}

DN_API DN_F32 DN_F32Lerp(DN_F32 a, DN_F32 t, DN_F32 b)
{
  DN_F32 result = a + ((b - a) * t);
  return result;
}

DN_API DN_F32 DN_F32Floor(DN_F32 val)
{
  DN_I32 val_i32 = DN_Cast(DN_I32) val;
  if (val < 0 && val != DN_Cast(DN_F32) val_i32)
    val_i32 -= 1;
  DN_F32 result = DN_Cast(DN_F32)val_i32;
  return result;
}

DN_API DN_F32 DN_F32Ceil(DN_F32 val)
{
  DN_I32 val_i32 = DN_Cast(DN_I32)(val);
  if (val > 0 && val != DN_Cast(DN_F32) val_i32)
    val_i32 += 1;
  DN_F32 result = DN_Cast(DN_F32) val_i32;
  return result;
}

DN_API DN_F32 DN_F32RoundHalfUp(DN_F32 val)
{
  DN_F32 result = val >= 0 ? DN_F32Floor(val + 0.5f) : DN_F32Ceil(val - 0.5f);
  return result;
}

DN_API bool operator==(DN_V2I32 lhs, DN_V2I32 rhs)
{
  bool result = (lhs.x == rhs.x) && (lhs.y == rhs.y);
  return result;
}

DN_API bool operator!=(DN_V2I32 lhs, DN_V2I32 rhs)
{
  bool result = !(lhs == rhs);
  return result;
}

DN_API bool operator>=(DN_V2I32 lhs, DN_V2I32 rhs)
{
  bool result = (lhs.x >= rhs.x) && (lhs.y >= rhs.y);
  return result;
}

DN_API bool operator<=(DN_V2I32 lhs, DN_V2I32 rhs)
{
  bool result = (lhs.x <= rhs.x) && (lhs.y <= rhs.y);
  return result;
}

DN_API bool operator<(DN_V2I32 lhs, DN_V2I32 rhs)
{
  bool result = (lhs.x < rhs.x) && (lhs.y < rhs.y);
  return result;
}

DN_API bool operator>(DN_V2I32 lhs, DN_V2I32 rhs)
{
  bool result = (lhs.x > rhs.x) && (lhs.y > rhs.y);
  return result;
}

DN_API DN_V2I32 operator-(DN_V2I32 lhs, DN_V2I32 rhs)
{
  DN_V2I32 result = DN_V2I32From2N(lhs.x - rhs.x, lhs.y - rhs.y);
  return result;
}

DN_API DN_V2I32 operator-(DN_V2I32 lhs)
{
  DN_V2I32 result = DN_V2I32From2N(-lhs.x, -lhs.y);
  return result;
}

DN_API DN_V2I32 operator+(DN_V2I32 lhs, DN_V2I32 rhs)
{
  DN_V2I32 result = DN_V2I32From2N(lhs.x + rhs.x, lhs.y + rhs.y);
  return result;
}

DN_API DN_V2I32 operator*(DN_V2I32 lhs, DN_V2I32 rhs)
{
  DN_V2I32 result = DN_V2I32From2N(lhs.x * rhs.x, lhs.y * rhs.y);
  return result;
}

DN_API DN_V2I32 operator*(DN_V2I32 lhs, DN_F32 rhs)
{
  DN_V2I32 result = DN_V2I32From2N(lhs.x * rhs, lhs.y * rhs);
  return result;
}

DN_API DN_V2I32 operator*(DN_V2I32 lhs, DN_I32 rhs)
{
  DN_V2I32 result = DN_V2I32From2N(lhs.x * rhs, lhs.y * rhs);
  return result;
}

DN_API DN_V2I32 operator/(DN_V2I32 lhs, DN_V2I32 rhs)
{
  DN_V2I32 result = DN_V2I32From2N(lhs.x / rhs.x, lhs.y / rhs.y);
  return result;
}

DN_API DN_V2I32 operator/(DN_V2I32 lhs, DN_F32 rhs)
{
  DN_V2I32 result = DN_V2I32From2N(lhs.x / rhs, lhs.y / rhs);
  return result;
}

DN_API DN_V2I32 operator/(DN_V2I32 lhs, DN_I32 rhs)
{
  DN_V2I32 result = DN_V2I32From2N(lhs.x / rhs, lhs.y / rhs);
  return result;
}

DN_API DN_V2I32 &operator*=(DN_V2I32 &lhs, DN_V2I32 rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V2I32 &operator*=(DN_V2I32 &lhs, DN_F32 rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V2I32 &operator*=(DN_V2I32 &lhs, DN_I32 rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V2I32 &operator/=(DN_V2I32 &lhs, DN_V2I32 rhs)
{
  lhs = lhs / rhs;
  return lhs;
}

DN_API DN_V2I32 &operator/=(DN_V2I32 &lhs, DN_F32 rhs)
{
  lhs = lhs / rhs;
  return lhs;
}

DN_API DN_V2I32 &operator/=(DN_V2I32 &lhs, DN_I32 rhs)
{
  lhs = lhs / rhs;
  return lhs;
}

DN_API DN_V2I32 &operator-=(DN_V2I32 &lhs, DN_V2I32 rhs)
{
  lhs = lhs - rhs;
  return lhs;
}

DN_API DN_V2I32 &operator+=(DN_V2I32 &lhs, DN_V2I32 rhs)
{
  lhs = lhs + rhs;
  return lhs;
}

DN_API DN_V2I32 DN_V2I32Min(DN_V2I32 a, DN_V2I32 b)
{
  DN_V2I32 result = DN_V2I32From2N(DN_Min(a.x, b.x), DN_Min(a.y, b.y));
  return result;
}

DN_API DN_V2I32 DN_V2I32Max(DN_V2I32 a, DN_V2I32 b)
{
  DN_V2I32 result = DN_V2I32From2N(DN_Max(a.x, b.x), DN_Max(a.y, b.y));
  return result;
}

DN_API DN_V2I32 DN_V2I32Abs(DN_V2I32 a)
{
  DN_V2I32 result = DN_V2I32From2N(DN_Abs(a.x), DN_Abs(a.y));
  return result;
}

DN_API bool operator!=(DN_V2U16 lhs, DN_V2U16 rhs)
{
  bool result = !(lhs == rhs);
  return result;
}

DN_API bool operator==(DN_V2U16 lhs, DN_V2U16 rhs)
{
  bool result = (lhs.x == rhs.x) && (lhs.y == rhs.y);
  return result;
}

DN_API bool operator>=(DN_V2U16 lhs, DN_V2U16 rhs)
{
  bool result = (lhs.x >= rhs.x) && (lhs.y >= rhs.y);
  return result;
}

DN_API bool operator<=(DN_V2U16 lhs, DN_V2U16 rhs)
{
  bool result = (lhs.x <= rhs.x) && (lhs.y <= rhs.y);
  return result;
}

DN_API bool operator<(DN_V2U16 lhs, DN_V2U16 rhs)
{
  bool result = (lhs.x < rhs.x) && (lhs.y < rhs.y);
  return result;
}

DN_API bool operator>(DN_V2U16 lhs, DN_V2U16 rhs)
{
  bool result = (lhs.x > rhs.x) && (lhs.y > rhs.y);
  return result;
}

DN_API DN_V2U16 operator-(DN_V2U16 lhs, DN_V2U16 rhs)
{
  DN_V2U16 result = DN_V2U16From2N(lhs.x - rhs.x, lhs.y - rhs.y);
  return result;
}

DN_API DN_V2U16 operator+(DN_V2U16 lhs, DN_V2U16 rhs)
{
  DN_V2U16 result = DN_V2U16From2N(lhs.x + rhs.x, lhs.y + rhs.y);
  return result;
}

DN_API DN_V2U16 operator*(DN_V2U16 lhs, DN_V2U16 rhs)
{
  DN_V2U16 result = DN_V2U16From2N(lhs.x * rhs.x, lhs.y * rhs.y);
  return result;
}

DN_API DN_V2U16 operator*(DN_V2U16 lhs, DN_F32 rhs)
{
  DN_V2U16 result = DN_V2U16From2N(lhs.x * rhs, lhs.y * rhs);
  return result;
}

DN_API DN_V2U16 operator*(DN_V2U16 lhs, DN_I32 rhs)
{
  DN_V2U16 result = DN_V2U16From2N(lhs.x * rhs, lhs.y * rhs);
  return result;
}

DN_API DN_V2U16 operator/(DN_V2U16 lhs, DN_V2U16 rhs)
{
  DN_V2U16 result = DN_V2U16From2N(lhs.x / rhs.x, lhs.y / rhs.y);
  return result;
}

DN_API DN_V2U16 operator/(DN_V2U16 lhs, DN_F32 rhs)
{
  DN_V2U16 result = DN_V2U16From2N(lhs.x / rhs, lhs.y / rhs);
  return result;
}

DN_API DN_V2U16 operator/(DN_V2U16 lhs, DN_I32 rhs)
{
  DN_V2U16 result = DN_V2U16From2N(lhs.x / rhs, lhs.y / rhs);
  return result;
}

DN_API DN_V2U16 &operator*=(DN_V2U16 &lhs, DN_V2U16 rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V2U16 &operator*=(DN_V2U16 &lhs, DN_F32 rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V2U16 &operator*=(DN_V2U16 &lhs, DN_I32 rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V2U16 &operator/=(DN_V2U16 &lhs, DN_V2U16 rhs)
{
  lhs = lhs / rhs;
  return lhs;
}

DN_API DN_V2U16 &operator/=(DN_V2U16 &lhs, DN_F32 rhs)
{
  lhs = lhs / rhs;
  return lhs;
}

DN_API DN_V2U16 &operator/=(DN_V2U16 &lhs, DN_I32 rhs)
{
  lhs = lhs / rhs;
  return lhs;
}

DN_API DN_V2U16 &operator-=(DN_V2U16 &lhs, DN_V2U16 rhs)
{
  lhs = lhs - rhs;
  return lhs;
}

DN_API DN_V2U16 &operator+=(DN_V2U16 &lhs, DN_V2U16 rhs)
{
  lhs = lhs + rhs;
  return lhs;
}

DN_API DN_V2F32 DN_V2F32Lerp(DN_V2F32 a, DN_F32 t, DN_V2F32 b)
{
  DN_V2F32 result = {};
  result.x        = a.x + ((b.x - a.x) * t);
  result.y        = a.y + ((b.y - a.y) * t);
  return result;
}

DN_API bool operator!=(DN_V2F32 lhs, DN_V2F32 rhs)
{
  bool result = !(lhs == rhs);
  return result;
}

DN_API bool operator==(DN_V2F32 lhs, DN_V2F32 rhs)
{
  bool result = (lhs.x == rhs.x) && (lhs.y == rhs.y);
  return result;
}

DN_API bool operator>=(DN_V2F32 lhs, DN_V2F32 rhs)
{
  bool result = (lhs.x >= rhs.x) && (lhs.y >= rhs.y);
  return result;
}

DN_API bool operator<=(DN_V2F32 lhs, DN_V2F32 rhs)
{
  bool result = (lhs.x <= rhs.x) && (lhs.y <= rhs.y);
  return result;
}

DN_API bool operator<(DN_V2F32 lhs, DN_V2F32 rhs)
{
  bool result = (lhs.x < rhs.x) && (lhs.y < rhs.y);
  return result;
}

DN_API bool operator>(DN_V2F32 lhs, DN_V2F32 rhs)
{
  bool result = (lhs.x > rhs.x) && (lhs.y > rhs.y);
  return result;
}

DN_API DN_V2F32 operator-(DN_V2F32 lhs)
{
  DN_V2F32 result = DN_V2F32From2N(-lhs.x, -lhs.y);
  return result;
}

DN_API DN_V2F32 operator-(DN_V2F32 lhs, DN_V2F32 rhs)
{
  DN_V2F32 result = DN_V2F32From2N(lhs.x - rhs.x, lhs.y - rhs.y);
  return result;
}

DN_API DN_V2F32 operator-(DN_V2F32 lhs, DN_V2I32 rhs)
{
  DN_V2F32 result = DN_V2F32From2N(lhs.x - rhs.x, lhs.y - rhs.y);
  return result;
}

DN_API DN_V2F32 operator-(DN_V2F32 lhs, DN_F32 rhs)
{
  DN_V2F32 result = DN_V2F32From2N(lhs.x - rhs, lhs.y - rhs);
  return result;
}

DN_API DN_V2F32 operator-(DN_V2F32 lhs, DN_I32 rhs)
{
  DN_V2F32 result = DN_V2F32From2N(lhs.x - rhs, lhs.y - rhs);
  return result;
}

DN_API DN_V2F32 operator+(DN_V2F32 lhs, DN_V2F32 rhs)
{
  DN_V2F32 result = DN_V2F32From2N(lhs.x + rhs.x, lhs.y + rhs.y);
  return result;
}

DN_API DN_V2F32 operator+(DN_V2F32 lhs, DN_V2I32 rhs)
{
  DN_V2F32 result = DN_V2F32From2N(lhs.x + rhs.x, lhs.y + rhs.y);
  return result;
}

DN_API DN_V2F32 operator+(DN_V2F32 lhs, DN_F32 rhs)
{
  DN_V2F32 result = DN_V2F32From2N(lhs.x + rhs, lhs.y + rhs);
  return result;
}

DN_API DN_V2F32 operator+(DN_V2F32 lhs, DN_I32 rhs)
{
  DN_V2F32 result = DN_V2F32From2N(lhs.x + rhs, lhs.y + rhs);
  return result;
}

DN_API DN_V2F32 operator*(DN_V2F32 lhs, DN_V2F32 rhs)
{
  DN_V2F32 result = DN_V2F32From2N(lhs.x * rhs.x, lhs.y * rhs.y);
  return result;
}

DN_API DN_V2F32 operator*(DN_V2F32 lhs, DN_V2I32 rhs)
{
  DN_V2F32 result = DN_V2F32From2N(lhs.x * rhs.x, lhs.y * rhs.y);
  return result;
}

DN_API DN_V2F32 operator*(DN_V2F32 lhs, DN_F32 rhs)
{
  DN_V2F32 result = DN_V2F32From2N(lhs.x * rhs, lhs.y * rhs);
  return result;
}

DN_API DN_V2F32 operator*(DN_V2F32 lhs, DN_I32 rhs)
{
  DN_V2F32 result = DN_V2F32From2N(lhs.x * rhs, lhs.y * rhs);
  return result;
}

DN_API DN_V2F32 operator/(DN_V2F32 lhs, DN_V2F32 rhs)
{
  DN_V2F32 result = DN_V2F32From2N(lhs.x / rhs.x, lhs.y / rhs.y);
  return result;
}

DN_API DN_V2F32 operator/(DN_V2F32 lhs, DN_V2I32 rhs)
{
  DN_V2F32 result = DN_V2F32From2N(lhs.x / rhs.x, lhs.y / rhs.y);
  return result;
}

DN_API DN_V2F32 operator/(DN_V2F32 lhs, DN_F32 rhs)
{
  DN_V2F32 result = DN_V2F32From2N(lhs.x / rhs, lhs.y / rhs);
  return result;
}

DN_API DN_V2F32 operator/(DN_V2F32 lhs, DN_I32 rhs)
{
  DN_V2F32 result = DN_V2F32From2N(lhs.x / rhs, lhs.y / rhs);
  return result;
}

DN_API DN_V2F32 &operator*=(DN_V2F32 &lhs, DN_V2F32 rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V2F32 &operator*=(DN_V2F32 &lhs, DN_V2I32 rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V2F32 &operator*=(DN_V2F32 &lhs, DN_F32 rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V2F32 &operator*=(DN_V2F32 &lhs, DN_I32 rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V2F32 &operator/=(DN_V2F32 &lhs, DN_V2F32 rhs)
{
  lhs = lhs / rhs;
  return lhs;
}

DN_API DN_V2F32 &operator/=(DN_V2F32 &lhs, DN_V2I32 rhs)
{
  lhs = lhs / rhs;
  return lhs;
}

DN_API DN_V2F32 &operator/=(DN_V2F32 &lhs, DN_F32 rhs)
{
  lhs = lhs / rhs;
  return lhs;
}

DN_API DN_V2F32 &operator/=(DN_V2F32 &lhs, DN_I32 rhs)
{
  lhs = lhs / rhs;
  return lhs;
}

DN_API DN_V2F32 &operator-=(DN_V2F32 &lhs, DN_V2F32 rhs)
{
  lhs = lhs - rhs;
  return lhs;
}

DN_API DN_V2F32 &operator-=(DN_V2F32 &lhs, DN_V2I32 rhs)
{
  lhs = lhs - rhs;
  return lhs;
}

DN_API DN_V2F32 &operator-=(DN_V2F32 &lhs, DN_F32 rhs)
{
  lhs = lhs - rhs;
  return lhs;
}

DN_API DN_V2F32 &operator-=(DN_V2F32 &lhs, DN_I32 rhs)
{
  lhs = lhs - rhs;
  return lhs;
}

DN_API DN_V2F32 &operator+=(DN_V2F32 &lhs, DN_V2F32 rhs)
{
  lhs = lhs + rhs;
  return lhs;
}

DN_API DN_V2F32 &operator+=(DN_V2F32 &lhs, DN_V2I32 rhs)
{
  lhs = lhs + rhs;
  return lhs;
}

DN_API DN_V2F32 &operator+=(DN_V2F32 &lhs, DN_F32 rhs)
{
  lhs = lhs + rhs;
  return lhs;
}

DN_API DN_V2F32 &operator+=(DN_V2F32 &lhs, DN_I32 rhs)
{
  lhs = lhs + rhs;
  return lhs;
}

DN_API DN_V2F32 DN_V2F32Min(DN_V2F32 a, DN_V2F32 b)
{
  DN_V2F32 result = DN_V2F32From2N(DN_Min(a.x, b.x), DN_Min(a.y, b.y));
  return result;
}

DN_API DN_V2F32 DN_V2F32Max(DN_V2F32 a, DN_V2F32 b)
{
  DN_V2F32 result = DN_V2F32From2N(DN_Max(a.x, b.x), DN_Max(a.y, b.y));
  return result;
}

DN_API DN_V2F32 DN_V2F32Abs(DN_V2F32 a)
{
  DN_V2F32 result = DN_V2F32From2N(DN_Abs(a.x), DN_Abs(a.y));
  return result;
}

DN_API DN_F32 DN_V2F32Dot(DN_V2F32 a, DN_V2F32 b)
{
  // NOTE: Scalar projection of B onto A /////////////////////////////////////////////////////////
  //
  // Scalar projection calculates the signed distance between `b` and `a`
  // where `a` is a unit vector then, the dot product calculates the projection
  // of `b` onto the infinite line that the direction of `a` represents. This
  // calculation is the signed distance.
  //
  // signed_distance = dot_product(a, b) = (a.x * b.x) + (a.y * b.y)
  //
  // Y
  // ^      b
  // |     /|
  // |    / |
  // |   /  |
  // |  /   | Projection
  // | /    |
  // |/     V
  // +--->--------> X
  // .   a  .
  // .      .
  // |------| <- Calculated signed distance
  //
  // The signed-ness of the result indicates the relationship:
  //
  // Distance <0  means `b` is behind           `a`
  // Distance >0  means `b` is in-front of      `a`
  // Distance ==0 means `b` is perpendicular to `a`
  //
  // If `a` is not normalized then the signed-ness of the result still holds
  // however result no longer represents the actual distance between the
  // 2 objects. One of the vectors must be normalised (e.g. turned into a unit
  // vector).
  //
  // NOTE: DN_V projection /////////////////////////////////////////////////////////////////////
  //
  // DN_V projection calculates the exact X,Y coordinates of where `b` meets
  // `a` when it was projected. This is calculated by multipying the
  // 'scalar projection' result by the unit vector of `a`
  //
  // vector_projection = a * signed_distance = a * dot_product(a, b)

  DN_F32 result = (a.x * b.x) + (a.y * b.y);
  return result;
}

DN_API DN_F32 DN_V2F32LengthSq2V2(DN_V2F32 lhs, DN_V2F32 rhs)
{
  // NOTE: Pythagoras's theorem (a^2 + b^2 = c^2) without the square root
  DN_F32 a         = rhs.x - lhs.x;
  DN_F32 b         = rhs.y - lhs.y;
  DN_F32 c_squared = DN_Squared(a) + DN_Squared(b);
  DN_F32 result    = c_squared;
  return result;
}

DN_API bool DN_V2F32LengthSqIsWithin2V2(DN_V2F32 lhs, DN_V2F32 rhs, DN_F32 within_amount_sq)
{
  DN_F32 dist   = DN_V2F32LengthSq2V2(lhs, rhs);
  bool   result = dist <= within_amount_sq;
  return result;
}

DN_API DN_F32 DN_V2F32Length2V2(DN_V2F32 lhs, DN_V2F32 rhs)
{
  DN_F32 result_squared = DN_V2F32LengthSq2V2(lhs, rhs);
  DN_F32 result         = DN_SqrtF32(result_squared);
  return result;
}

DN_API DN_F32 DN_V2F32LengthSq(DN_V2F32 lhs)
{
  // NOTE: Pythagoras's theorem without the square root
  DN_F32 c_squared = DN_Squared(lhs.x) + DN_Squared(lhs.y);
  DN_F32 result    = c_squared;
  return result;
}

DN_API DN_F32 DN_V2F32Length(DN_V2F32 lhs)
{
  DN_F32 c_squared = DN_V2F32LengthSq(lhs);
  DN_F32 result    = DN_SqrtF32(c_squared);
  return result;
}

DN_API DN_V2F32 DN_V2F32Normalise(DN_V2F32 a)
{
  DN_F32   length = DN_V2F32Length(a);
  DN_V2F32 result = a / length;
  return result;
}

DN_API DN_V2F32 DN_V2F32Perpendicular(DN_V2F32 a)
{
  // NOTE: Matrix form of a 2D vector can be defined as
  //
  // x' = x cos(t) - y sin(t)
  // y' = x sin(t) + y cos(t)
  //
  // Calculate a line perpendicular to a vector means rotating the vector by
  // 90 degrees
  //
  // x' = x cos(90) - y sin(90)
  // y' = x sin(90) + y cos(90)
  //
  // Where `cos(90) = 0` and `sin(90) = 1` then,
  //
  // x' = -y
  // y' = +x

  DN_V2F32 result = DN_V2F32From2N(-a.y, a.x);
  return result;
}

DN_API DN_V2F32 DN_V2F32Reflect(DN_V2F32 in, DN_V2F32 surface)
{
  DN_V2F32 normal      = DN_V2F32Perpendicular(surface);
  DN_V2F32 normal_norm = DN_V2F32Normalise(normal);
  DN_F32   signed_dist = DN_V2F32Dot(in, normal_norm);
  DN_V2F32 result      = DN_V2F32From2N(in.x, in.y + (-signed_dist * 2.f));
  return result;
}

DN_API DN_F32 DN_V2F32Area(DN_V2F32 a)
{
  DN_F32 result = a.w * a.h;
  return result;
}

DN_API bool operator!=(DN_V3F32 lhs, DN_V3F32 rhs)
{
  bool result = !(lhs == rhs);
  return result;
}

DN_API bool operator==(DN_V3F32 lhs, DN_V3F32 rhs)
{
  bool result = (lhs.x == rhs.x) && (lhs.y == rhs.y) && (lhs.z == rhs.z);
  return result;
}

DN_API bool operator>=(DN_V3F32 lhs, DN_V3F32 rhs)
{
  bool result = (lhs.x >= rhs.x) && (lhs.y >= rhs.y) && (lhs.z >= rhs.z);
  return result;
}

DN_API bool operator<=(DN_V3F32 lhs, DN_V3F32 rhs)
{
  bool result = (lhs.x <= rhs.x) && (lhs.y <= rhs.y) && (lhs.z <= rhs.z);
  return result;
}

DN_API bool operator<(DN_V3F32 lhs, DN_V3F32 rhs)
{
  bool result = (lhs.x < rhs.x) && (lhs.y < rhs.y) && (lhs.z < rhs.z);
  return result;
}

DN_API bool operator>(DN_V3F32 lhs, DN_V3F32 rhs)
{
  bool result = (lhs.x > rhs.x) && (lhs.y > rhs.y) && (lhs.z > rhs.z);
  return result;
}

DN_API DN_V3F32 operator-(DN_V3F32 lhs, DN_V3F32 rhs)
{
  DN_V3F32 result = DN_V3F32From3N(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
  return result;
}

DN_API DN_V3F32 operator-(DN_V3F32 lhs)
{
  DN_V3F32 result = DN_V3F32From3N(-lhs.x, -lhs.y, -lhs.z);
  return result;
}

DN_API DN_V3F32 operator+(DN_V3F32 lhs, DN_V3F32 rhs)
{
  DN_V3F32 result = DN_V3F32From3N(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);
  return result;
}

DN_API DN_V3F32 operator*(DN_V3F32 lhs, DN_V3F32 rhs)
{
  DN_V3F32 result = DN_V3F32From3N(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z);
  return result;
}

DN_API DN_V3F32 operator*(DN_V3F32 lhs, DN_F32 rhs)
{
  DN_V3F32 result = DN_V3F32From3N(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs);
  return result;
}

DN_API DN_V3F32 operator*(DN_V3F32 lhs, DN_I32 rhs)
{
  DN_V3F32 result = DN_V3F32From3N(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs);
  return result;
}

DN_API DN_V3F32 operator/(DN_V3F32 lhs, DN_V3F32 rhs)
{
  DN_V3F32 result = DN_V3F32From3N(lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z);
  return result;
}

DN_API DN_V3F32 operator/(DN_V3F32 lhs, DN_F32 rhs)
{
  DN_V3F32 result = DN_V3F32From3N(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs);
  return result;
}

DN_API DN_V3F32 operator/(DN_V3F32 lhs, DN_I32 rhs)
{
  DN_V3F32 result = DN_V3F32From3N(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs);
  return result;
}

DN_API DN_V3F32 &operator*=(DN_V3F32 &lhs, DN_V3F32 rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V3F32 &operator*=(DN_V3F32 &lhs, DN_F32 rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V3F32 &operator*=(DN_V3F32 &lhs, DN_I32 rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V3F32 &operator/=(DN_V3F32 &lhs, DN_V3F32 rhs)
{
  lhs = lhs / rhs;
  return lhs;
}

DN_API DN_V3F32 &operator/=(DN_V3F32 &lhs, DN_F32 rhs)
{
  lhs = lhs / rhs;
  return lhs;
}

DN_API DN_V3F32 &operator/=(DN_V3F32 &lhs, DN_I32 rhs)
{
  lhs = lhs / rhs;
  return lhs;
}

DN_API DN_V3F32 &operator-=(DN_V3F32 &lhs, DN_V3F32 rhs)
{
  lhs = lhs - rhs;
  return lhs;
}

DN_API DN_V3F32 &operator+=(DN_V3F32 &lhs, DN_V3F32 rhs)
{
  lhs = lhs + rhs;
  return lhs;
}

DN_API DN_V3F32 DN_V3F32Lerp(DN_V3F32 lhs, DN_F32 t01, DN_V3F32 rhs)
{
  DN_V3F32 result = {};
  result.x        = lhs.x + ((rhs.x - lhs.x) * t01);
  result.y        = lhs.y + ((rhs.y - lhs.y) * t01);
  result.z        = lhs.z + ((rhs.z - lhs.z) * t01);
  return result;
}

DN_API DN_F32 DN_V3_LengthSq(DN_V3F32 a)
{
  DN_F32 result = DN_Squared(a.x) + DN_Squared(a.y) + DN_Squared(a.z);
  return result;
}

DN_API DN_F32 DN_V3_Length(DN_V3F32 a)
{
  DN_F32 length_sq = DN_Squared(a.x) + DN_Squared(a.y) + DN_Squared(a.z);
  DN_F32 result    = DN_SqrtF32(length_sq);
  return result;
}

DN_API DN_V3F32 DN_V3_Normalise(DN_V3F32 a)
{
  DN_F32   length = DN_V3_Length(a);
  DN_V3F32 result = a / length;
  return result;
}

DN_API DN_V4F32 DN_V4F32Lerp(DN_V4F32 lhs, DN_F32 t01, DN_V4F32 rhs)
{
  DN_V4F32 result = {};
  result.x        = lhs.x + (rhs.x - lhs.x) * t01;
  result.y        = lhs.y + (rhs.y - lhs.y) * t01;
  result.z        = lhs.z + (rhs.z - lhs.z) * t01;
  result.w        = lhs.w + (rhs.w - lhs.w) * t01;
  return result;
}

DN_API bool DN_V4F32RGBA01IsValid(DN_V4F32 rgba01)
{
  bool result = rgba01.r >= 0 && rgba01.r <= 1.f &&
                rgba01.g >= 0 && rgba01.g <= 1.f &&
                rgba01.b >= 0 && rgba01.b <= 1.f &&
                rgba01.a >= 0 && rgba01.a <= 1.f;
  return result;
}

DN_API DN_V4F32 DN_V4F32RGBA01FromRGBU32(DN_U32 u32)
{
  DN_U8    r      = (DN_U8)((u32 & 0x00FF0000) >> 16);
  DN_U8    g      = (DN_U8)((u32 & 0x0000FF00) >> 8);
  DN_U8    b      = (DN_U8)((u32 & 0x000000FF) >> 0);
  DN_V4F32 result = DN_V4F32RGBA01FromRGBU8(r, g, b);
  return result;
}

DN_API DN_V4F32 DN_V4F32RGBA01FromRGBAU32(DN_U32 u32)
{
  DN_U8    r      = (DN_U8)((u32 & 0xFF000000) >> 24);
  DN_U8    g      = (DN_U8)((u32 & 0x00FF0000) >> 16);
  DN_U8    b      = (DN_U8)((u32 & 0x0000FF00) >> 8);
  DN_U8    a      = (DN_U8)((u32 & 0x000000FF) >> 0);
  DN_V4F32 result = DN_V4F32RGBA01FromRGBAU8(r, g, b, a);
  return result;
}

#define DN_SRGB_COEFFICIENT_F32 2.2f
DN_API DN_V4F32 DN_V4F32Linear01FromSRGB01(DN_V4F32 srgb01)
{
  DN_Assert(srgb01.x >= 0.f && srgb01.x <= 1.f);
  DN_Assert(srgb01.y >= 0.f && srgb01.y <= 1.f);
  DN_Assert(srgb01.z >= 0.f && srgb01.z <= 1.f);
  DN_Assert(srgb01.a >= 0.f && srgb01.a <= 1.f);
  DN_V4F32 result = {};
  result.r        = DN_PowF32(srgb01.r, DN_SRGB_COEFFICIENT_F32);
  result.g        = DN_PowF32(srgb01.g, DN_SRGB_COEFFICIENT_F32);
  result.b        = DN_PowF32(srgb01.b, DN_SRGB_COEFFICIENT_F32);
  result.a        = srgb01.a;
  return result;
}

DN_API DN_V4F32 DN_V4F32Linear01Desaturate(DN_V4F32 linear01, DN_F32 t01)
{
  DN_F32   luminance = (linear01.r * DN_V3F32_RGB_LUMINANCE.r) + (linear01.g * DN_V3F32_RGB_LUMINANCE.g) + (linear01.b * DN_V3F32_RGB_LUMINANCE.b);
  DN_V4F32 result    = linear01;
  result.rgb         = DN_V3F32Lerp(result.rgb, t01, DN_V3F32From1N(luminance));
  return result;
}

DN_API DN_V4F32 DN_V4F32SRGB01FromLinear01(DN_V4F32 linear01)
{
  DN_Assert(linear01.x >= 0.f && linear01.x <= 1.f);
  DN_Assert(linear01.y >= 0.f && linear01.y <= 1.f);
  DN_Assert(linear01.z >= 0.f && linear01.z <= 1.f);
  DN_Assert(linear01.a >= 0.f && linear01.a <= 1.f);
  DN_V4F32 result = {};
  result.r        = DN_PowF32(linear01.r, 1.f / DN_SRGB_COEFFICIENT_F32);
  result.g        = DN_PowF32(linear01.g, 1.f / DN_SRGB_COEFFICIENT_F32);
  result.b        = DN_PowF32(linear01.b, 1.f / DN_SRGB_COEFFICIENT_F32);
  result.a        = linear01.a;
  return result;
}

DN_API bool operator==(DN_V4F32 lhs, DN_V4F32 rhs)
{
  bool result = (lhs.x == rhs.x) && (lhs.y == rhs.y) && (lhs.z == rhs.z) && (lhs.w == rhs.w);
  return result;
}

DN_API bool operator!=(DN_V4F32 lhs, DN_V4F32 rhs)
{
  bool result = !(lhs == rhs);
  return result;
}

DN_API bool operator>=(DN_V4F32 lhs, DN_V4F32 rhs)
{
  bool result = (lhs.x >= rhs.x) && (lhs.y >= rhs.y) && (lhs.z >= rhs.z) && (lhs.w >= rhs.w);
  return result;
}

DN_API bool operator<=(DN_V4F32 lhs, DN_V4F32 rhs)
{
  bool result = (lhs.x <= rhs.x) && (lhs.y <= rhs.y) && (lhs.z <= rhs.z) && (lhs.w <= rhs.w);
  return result;
}

DN_API bool operator<(DN_V4F32 lhs, DN_V4F32 rhs)
{
  bool result = (lhs.x < rhs.x) && (lhs.y < rhs.y) && (lhs.z < rhs.z) && (lhs.w < rhs.w);
  return result;
}

DN_API bool operator>(DN_V4F32 lhs, DN_V4F32 rhs)
{
  bool result = (lhs.x > rhs.x) && (lhs.y > rhs.y) && (lhs.z > rhs.z) && (lhs.w > rhs.w);
  return result;
}

DN_API DN_V4F32 operator-(DN_V4F32 lhs, DN_V4F32 rhs)
{
  DN_V4F32 result = DN_V4F32From4N(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w);
  return result;
}

DN_API DN_V4F32 operator-(DN_V4F32 lhs)
{
  DN_V4F32 result = DN_V4F32From4N(-lhs.x, -lhs.y, -lhs.z, -lhs.w);
  return result;
}

DN_API DN_V4F32 operator+(DN_V4F32 lhs, DN_V4F32 rhs)
{
  DN_V4F32 result = DN_V4F32From4N(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w);
  return result;
}

DN_API DN_V4F32 operator*(DN_V4F32 lhs, DN_V4F32 rhs)
{
  DN_V4F32 result = DN_V4F32From4N(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w);
  return result;
}

DN_API DN_V4F32 operator*(DN_V4F32 lhs, DN_F32 rhs)
{
  DN_V4F32 result = DN_V4F32From4N(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs);
  return result;
}

DN_API DN_V4F32 operator*(DN_V4F32 lhs, DN_I32 rhs)
{
  DN_V4F32 result = DN_V4F32From4N(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs);
  return result;
}

DN_API DN_V4F32 operator/(DN_V4F32 lhs, DN_F32 rhs)
{
  DN_V4F32 result = DN_V4F32From4N(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs, lhs.w / rhs);
  return result;
}

DN_API DN_V4F32 &operator*=(DN_V4F32 &lhs, DN_V4F32 rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V4F32 &operator*=(DN_V4F32 &lhs, DN_F32 rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V4F32 &operator*=(DN_V4F32 &lhs, DN_I32 rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V4F32 &operator-=(DN_V4F32 &lhs, DN_V4F32 rhs)
{
  lhs = lhs - rhs;
  return lhs;
}

DN_API DN_V4F32 &operator+=(DN_V4F32 &lhs, DN_V4F32 rhs)
{
  lhs = lhs + rhs;
  return lhs;
}

DN_API DN_F32 DN_V4F32Dot(DN_V4F32 a, DN_V4F32 b)
{
  DN_F32 result = (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
  return result;
}

DN_API DN_M4 DN_M4Identity()
{
  DN_M4 result =
      {
          {
           {1, 0, 0, 0},
           {0, 1, 0, 0},
           {0, 0, 1, 0},
           {0, 0, 0, 1},
           }
  };

  return result;
}

DN_API DN_M4 DN_M4ScaleF(DN_F32 x, DN_F32 y, DN_F32 z)
{
  DN_M4 result =
      {
          {
           {x, 0, 0, 0},
           {0, y, 0, 0},
           {0, 0, z, 0},
           {0, 0, 0, 1},
           }
  };

  return result;
}

DN_API DN_M4 DN_M4Scale(DN_V3F32 xyz)
{
  DN_M4 result =
      {
          {
           {xyz.x, 0, 0, 0},
           {0, xyz.y, 0, 0},
           {0, 0, xyz.z, 0},
           {0, 0, 0, 1},
           }
  };

  return result;
}

DN_API DN_M4 DN_M4TranslateF(DN_F32 x, DN_F32 y, DN_F32 z)
{
  DN_M4 result =
      {
          {
           {1, 0, 0, 0},
           {0, 1, 0, 0},
           {0, 0, 1, 0},
           {x, y, z, 1},
           }
  };

  return result;
}

DN_API DN_M4 DN_M4Translate(DN_V3F32 xyz)
{
  DN_M4 result =
      {
          {
           {1, 0, 0, 0},
           {0, 1, 0, 0},
           {0, 0, 1, 0},
           {xyz.x, xyz.y, xyz.z, 1},
           }
  };

  return result;
}

DN_API DN_M4 DN_M4Transpose(DN_M4 mat)
{
  DN_M4 result = {};
  for (int col = 0; col < 4; col++)
    for (int row = 0; row < 4; row++)
      result.columns[col][row] = mat.columns[row][col];
  return result;
}

DN_API DN_M4 DN_M4Rotate(DN_V3F32 axis01, DN_F32 radians)
{
  DN_AssertF(DN_Abs(DN_V3_Length(axis01) - 1.f) <= 0.01f,
             "Rotation axis must be normalised, length = %f",
             DN_V3_Length(axis01));

  DN_F32 sin           = DN_SinF32(radians);
  DN_F32 cos           = DN_CosF32(radians);
  DN_F32 one_minus_cos = 1.f - cos;

  DN_F32 x  = axis01.x;
  DN_F32 y  = axis01.y;
  DN_F32 z  = axis01.z;
  DN_F32 x2 = DN_Squared(x);
  DN_F32 y2 = DN_Squared(y);
  DN_F32 z2 = DN_Squared(z);

  DN_M4 result =
      {
          {
           {cos + x2 * one_minus_cos, y * x * one_minus_cos + z * sin, z * x * one_minus_cos - y * sin, 0}, // Col 1
              {x * y * one_minus_cos - z * sin, cos + y2 * one_minus_cos, z * y * one_minus_cos + x * sin, 0}, // Col 2
              {x * z * one_minus_cos + y * sin, y * z * one_minus_cos - x * sin, cos + z2 * one_minus_cos, 0}, // Col 3
              {0, 0, 0, 1},                                                                                    // Col 4
          }
  };

  return result;
}

DN_API DN_M4 DN_M4Orthographic(DN_F32 left, DN_F32 right, DN_F32 bottom, DN_F32 top, DN_F32 z_near, DN_F32 z_far)
{
  // NOTE: Here is the matrix in column major for readability. Below it's
  // transposed due to how you have to declare column major matrices in C/C++.
  //
  // m = [2/r-l, 0,      0,     -1*(r+l)/(r-l)]
  //     [0,     2/t-b,  0,      1*(t+b)/(t-b)]
  //     [0,     0,     -2/f-n, -1*(f+n)/(f-n)]
  //     [0,     0,      0,      1            ]

  DN_M4 result =
      {
          {
           {2.f / (right - left), 0.f, 0.f, 0.f},
           {0.f, 2.f / (top - bottom), 0.f, 0.f},
           {0.f, 0.f, -2.f / (z_far - z_near), 0.f},
           {(-1.f * (right + left)) / (right - left), (-1.f * (top + bottom)) / (top - bottom), (-1.f * (z_far + z_near)) / (z_far - z_near), 1.f},
           }
  };

  return result;
}

DN_API DN_M4 DN_M4Perspective(DN_F32 fov /*radians*/, DN_F32 aspect, DN_F32 z_near, DN_F32 z_far)
{
  DN_F32 tan_fov = DN_TanF32(fov / 2.f);
  DN_M4  result =
      {
          {
           {1.f / (aspect * tan_fov), 0.f, 0.f, 0.f},
           {0, 1.f / tan_fov, 0.f, 0.f},
           {0.f, 0.f, (z_near + z_far) / (z_near - z_far), -1.f},
           {0.f, 0.f, (2.f * z_near * z_far) / (z_near - z_far), 0.f},
           }
  };

  return result;
}

DN_API DN_M4 DN_M4Add(DN_M4 lhs, DN_M4 rhs)
{
  DN_M4 result;
  for (int col = 0; col < 4; col++)
    for (int it = 0; it < 4; it++)
      result.columns[col][it] = lhs.columns[col][it] + rhs.columns[col][it];
  return result;
}

DN_API DN_M4 DN_M4Sub(DN_M4 lhs, DN_M4 rhs)
{
  DN_M4 result;
  for (int col = 0; col < 4; col++)
    for (int it = 0; it < 4; it++)
      result.columns[col][it] = lhs.columns[col][it] - rhs.columns[col][it];
  return result;
}

DN_API DN_M4 DN_M4Mul(DN_M4 lhs, DN_M4 rhs)
{
  DN_M4 result;
  for (int col = 0; col < 4; col++) {
    for (int row = 0; row < 4; row++) {
      DN_F32 sum = 0;
      for (int f32_it = 0; f32_it < 4; f32_it++)
        sum += lhs.columns[f32_it][row] * rhs.columns[col][f32_it];

      result.columns[col][row] = sum;
    }
  }
  return result;
}

DN_API DN_M4 DN_M4Div(DN_M4 lhs, DN_M4 rhs)
{
  DN_M4 result;
  for (int col = 0; col < 4; col++)
    for (int it = 0; it < 4; it++)
      result.columns[col][it] = lhs.columns[col][it] / rhs.columns[col][it];
  return result;
}

DN_API DN_M4 DN_M4AddF(DN_M4 lhs, DN_F32 rhs)
{
  DN_M4 result;
  for (int col = 0; col < 4; col++)
    for (int it = 0; it < 4; it++)
      result.columns[col][it] = lhs.columns[col][it] + rhs;
  return result;
}

DN_API DN_M4 DN_M4SubF(DN_M4 lhs, DN_F32 rhs)
{
  DN_M4 result;
  for (int col = 0; col < 4; col++)
    for (int it = 0; it < 4; it++)
      result.columns[col][it] = lhs.columns[col][it] - rhs;
  return result;
}

DN_API DN_M4 DN_M4MulF(DN_M4 lhs, DN_F32 rhs)
{
  DN_M4 result;
  for (int col = 0; col < 4; col++)
    for (int it = 0; it < 4; it++)
      result.columns[col][it] = lhs.columns[col][it] * rhs;
  return result;
}

DN_API DN_M4 DN_M4DivF(DN_M4 lhs, DN_F32 rhs)
{
  DN_M4 result;
  for (int col = 0; col < 4; col++)
    for (int it = 0; it < 4; it++)
      result.columns[col][it] = lhs.columns[col][it] / rhs;
  return result;
}

DN_API DN_Str8x256 DN_M4ColumnMajorString(DN_M4 mat)
{
  DN_Str8x256 result = {};
  for (int row = 0; row < 4; row++) {
    for (int it = 0; it < 4; it++) {
      if (it == 0)
        DN_FmtAppend(result.data, &result.size, sizeof(result.data), "|");
      DN_FmtAppend(result.data, &result.size, sizeof(result.data), "%.5f", mat.columns[it][row]);
      if (it != 3)
        DN_FmtAppend(result.data, &result.size, sizeof(result.data), ", ");
      else
        DN_FmtAppend(result.data, &result.size, sizeof(result.data), "|\n");
    }
  }
  return result;
}

DN_API bool operator==(DN_M2x3 const &lhs, DN_M2x3 const &rhs)
{
  bool result = DN_Memcmp(lhs.e, rhs.e, sizeof(lhs.e[0]) * DN_ArrayCountU(lhs.e)) == 0;
  return result;
}

DN_API bool operator!=(DN_M2x3 const &lhs, DN_M2x3 const &rhs)
{
  bool result = !(lhs == rhs);
  return result;
}

DN_API DN_M2x3 DN_M2x3Identity()
{
  DN_M2x3 result = {
      {
       1,
       0,
       0,
       0,
       1,
       0,
       }
  };
  return result;
}

DN_API DN_M2x3 DN_M2x3Translate(DN_V2F32 offset)
{
  DN_M2x3 result = {
      {
       1,
       0,
       offset.x,
       0,
       1,
       offset.y,
       }
  };
  return result;
}

DN_API DN_V2F32 DN_M2x3ScaleGet(DN_M2x3 m2x3)
{
  DN_V2F32 result = DN_V2F32From2N(m2x3.row[0][0], m2x3.row[1][1]);
  return result;
}

DN_API DN_M2x3 DN_M2x3Scale(DN_V2F32 scale)
{
  DN_M2x3 result = {{
    scale.x, 0,       0,
    0,       scale.y, 0,
  }};
  return result;
}

DN_API DN_M2x3 DN_M2x3Rotate(DN_F32 radians)
{
  DN_M2x3 result = {{
     DN_CosF32(radians), DN_SinF32(radians), 0,
    -DN_SinF32(radians), DN_CosF32(radians), 0,
  }};
  return result;
}

DN_API DN_M2x3 DN_M2x3ProjFromV2F32(DN_V2F32 size, DN_M2x3ProjOrigin origin)
{
  DN_M2x3 result = {};

  // NOTE: Maps coordinates within a rectangle of `size` into NDC where (-1, +1) is top left, (+1, -1) is bot right
  if (origin == DN_M2x3ProjOrigin_TopLeft) {
    result = {{
       2.f/size.w, 0,           -1.f,
       0,          -2.f/size.h, +1.f,
    }};
  } else {
    DN_Assert(origin == DN_M2x3ProjOrigin_Center);
    result = {{
       2.f/size.w, 0,           0.f,
       0,          -2.f/size.h, 0.f,
    }};
  }
  return result;
}

DN_API DN_M2x3XForm DN_M2x3XFormFromM2x3(DN_M2x3 forward, DN_M2x3 inverse)
{
  DN_M2x3XForm result = {};
  result.forward      = forward;
  result.inverse      = inverse;
  return result;
}

DN_API DN_M2x3XForm DN_M2x3XFormFromTRS(DN_V2F32 pos, DN_V2F32 scale, DN_F32 rotate_rads, DN_V2F32 pivot_pos)
{
  DN_M2x3XForm result = {};
  result.forward      = DN_M2x3Identity();
  result.inverse      = DN_M2x3Identity();

  if (scale.x == 0)
    scale.x = 1;
  if (scale.y == 0)
    scale.y = 1;

  result.forward      = DN_M2x3Mul(result.forward, DN_M2x3Translate(pivot_pos));
  result.forward      = DN_M2x3Mul(result.forward, DN_M2x3Rotate(rotate_rads));
  result.forward      = DN_M2x3Mul(result.forward, DN_M2x3Scale(scale));
  result.forward      = DN_M2x3Mul(result.forward, DN_M2x3Translate(-pivot_pos));
  result.forward      = DN_M2x3Mul(result.forward, DN_M2x3Translate(pos));

  DN_V2F32 inverse_scale = DN_V2F32From1N(1) / scale;
  result.inverse      = DN_M2x3Mul(result.inverse, DN_M2x3Translate(-pos));
  result.inverse      = DN_M2x3Mul(result.inverse, DN_M2x3Translate(pivot_pos));
  result.inverse      = DN_M2x3Mul(result.inverse, DN_M2x3Scale(inverse_scale));
  result.inverse      = DN_M2x3Mul(result.inverse, DN_M2x3Rotate(-rotate_rads));
  result.inverse      = DN_M2x3Mul(result.inverse, DN_M2x3Translate(-pivot_pos));
  return result;
}

DN_API DN_M2x3XForm DN_M2x3XFormIdentity()
{
  DN_M2x3XForm result = {};
  result.forward      = DN_M2x3Identity();
  result.inverse      = DN_M2x3Identity();
  return result;
}

DN_API DN_M2x3XForm DN_M2x3XFormMul(DN_M2x3XForm m1, DN_M2x3XForm m2)
{
  DN_M2x3XForm result = {};
  result.forward = DN_M2x3Mul(m1.forward, m2.forward);
  result.inverse = DN_M2x3Mul(m2.inverse, m1.inverse);
  return result;
}

DN_API DN_M2x3 DN_M2x3Mul(DN_M2x3 m1, DN_M2x3 m2)
{
  // NOTE: Ordinarily you can't multiply M2x3 with M2x3 because column count
  // (3) != row count (2). We pretend we have two 3x3 matrices with the last
  // row set to [0 0 1] and perform a 3x3 matrix multiply.
  //
  // | (0)a (1)b (2)c |   | (0)g (1)h (2)i |
  // | (3)d (4)e (5)f | x | (3)j (4)k (5)l |
  // | (6)0 (7)0 (8)1 |   | (6)0 (7)0 (8)1 |

  DN_M2x3 result = {
      {
          m1.e[0] * m2.e[0] + m1.e[1] * m2.e[3],           // a*g + b*j + c*0[omitted],
          m1.e[0] * m2.e[1] + m1.e[1] * m2.e[4],           // a*h + b*k + c*0[omitted],
          m1.e[0] * m2.e[2] + m1.e[1] * m2.e[5] + m1.e[2], // a*i + b*l + c*1,

          m1.e[3] * m2.e[0] + m1.e[4] * m2.e[3],           // d*g + e*j + f*0[omitted],
          m1.e[3] * m2.e[1] + m1.e[4] * m2.e[4],           // d*h + e*k + f*0[omitted],
          m1.e[3] * m2.e[2] + m1.e[4] * m2.e[5] + m1.e[5], // d*i + e*l + f*1,
      }
  };

  return result;
}

DN_API DN_V2F32 DN_M2x3Mul2F32(DN_M2x3 m1, DN_F32 x, DN_F32 y)
{
  // NOTE: Ordinarily you can't multiply M2x3 with V2 because column count (3)
  // != row count (2). We pretend we have a V3 with `z` set to `1`.
  //
  // | (0)a (1)b (2)c |   | x |
  // | (3)d (4)e (5)f | x | y |
  //                      | 1 |

  DN_V2F32 result = {
      {
       m1.e[0] * x + m1.e[1] * y + m1.e[2], // a*x + b*y + c*1
          m1.e[3] * x + m1.e[4] * y + m1.e[5], // d*x + e*y + f*1
      }
  };
  return result;
}

DN_API DN_V2F32 DN_M2x3MulV2F32(DN_M2x3 m1, DN_V2F32 v2)
{
  DN_V2F32 result = DN_M2x3Mul2F32(m1, v2.x, v2.y);
  return result;
}

DN_API DN_Rect DN_M2x3MulRect(DN_M2x3 m1, DN_Rect rect)
{
  DN_2V2F32    rect_range   = DN_RectRange(rect);
  DN_2V2F32    result_range = {};
  result_range.min          = DN_M2x3MulV2F32(m1, rect_range.min);
  result_range.max          = DN_M2x3MulV2F32(m1, rect_range.max);
  DN_Rect      result       = DN_RectFrom2V2(result_range.min, result_range.max - result_range.min);
  return result;
}

DN_API bool operator==(const DN_Rect &lhs, const DN_Rect &rhs)
{
  bool result = (lhs.pos == rhs.pos) && (lhs.size == rhs.size);
  return result;
}

DN_API DN_V2F32 DN_RectCenter(DN_Rect rect)
{
  DN_V2F32 result = rect.pos + (rect.size * .5f);
  return result;
}

DN_API bool DN_RectContainsPoint(DN_Rect rect, DN_V2F32 p)
{
  DN_V2F32 min    = rect.pos;
  DN_V2F32 max    = rect.pos + rect.size;
  bool     result = (p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y);
  return result;
}

DN_API bool DN_RectContainsRect(DN_Rect a, DN_Rect b)
{
  DN_V2F32 a_min  = a.pos;
  DN_V2F32 a_max  = a.pos + a.size;
  DN_V2F32 b_min  = b.pos;
  DN_V2F32 b_max  = b.pos + b.size;
  bool     result = (b_min >= a_min && b_max <= a_max);
  return result;
}

DN_API DN_Rect DN_RectExpand(DN_Rect a, DN_F32 amount)
{
  DN_Rect result = a;
  result.pos -= amount;
  result.size += (amount * 2.f);
  return result;
}

DN_API DN_Rect DN_RectExpandV2(DN_Rect a, DN_V2F32 amount)
{
  DN_Rect result = a;
  result.pos -= amount;
  result.size += (amount * 2.f);
  return result;
}

DN_API bool DN_RectIntersects(DN_Rect a, DN_Rect b)
{
  DN_V2F32 a_min    = a.pos;
  DN_V2F32 a_max    = a.pos + a.size;
  DN_V2F32 b_min    = b.pos;
  DN_V2F32 b_max    = b.pos + b.size;
  bool     has_size = a.size.x && a.size.y && b.size.x && b.size.y;
  bool     result   = false;
  if (has_size)
    result = (a_min.x <= b_max.x && a_max.x >= b_min.x) &&
             (a_min.y <= b_max.y && a_max.y >= b_min.y);
  return result;
}

DN_API DN_Rect DN_RectIntersection(DN_Rect a, DN_Rect b)
{
  DN_Rect result = DN_RectFrom2V2(a.pos, DN_V2F32From1N(0));
  if (DN_RectIntersects(a, b)) {
    DN_V2F32 a_min = a.pos;
    DN_V2F32 a_max = a.pos + a.size;
    DN_V2F32 b_min = b.pos;
    DN_V2F32 b_max = b.pos + b.size;

    DN_V2F32 min = {};
    DN_V2F32 max = {};
    min.x        = DN_Max(a_min.x, b_min.x);
    min.y        = DN_Max(a_min.y, b_min.y);
    max.x        = DN_Min(a_max.x, b_max.x);
    max.y        = DN_Min(a_max.y, b_max.y);
    result       = DN_RectFrom2V2(min, max - min);
  }
  return result;
}

DN_API DN_Rect DN_RectUnion(DN_Rect a, DN_Rect b)
{
  DN_V2F32 a_min = a.pos;
  DN_V2F32 a_max = a.pos + a.size;
  DN_V2F32 b_min = b.pos;
  DN_V2F32 b_max = b.pos + b.size;

  DN_V2F32 min, max;
  min.x          = DN_Min(a_min.x, b_min.x);
  min.y          = DN_Min(a_min.y, b_min.y);
  max.x          = DN_Max(a_max.x, b_max.x);
  max.y          = DN_Max(a_max.y, b_max.y);
  DN_Rect result = DN_RectFrom2V2(min, max - min);
  return result;
}

DN_API DN_2V2F32 DN_RectRange(DN_Rect a)
{
  DN_2V2F32 result = {};
  result.min       = a.pos;
  result.max       = a.pos + a.size;
  return result;
}

DN_API bool DN_RectEq(DN_Rect lhs, DN_Rect rhs)
{
  bool result = lhs.pos == rhs.pos && lhs.size == rhs.size;
  return result;
}

DN_API DN_F32 DN_RectArea(DN_Rect a)
{
  DN_F32 result = a.size.w * a.size.h;
  return result;
}

DN_API DN_Rect DN_RectCutLeftClip(DN_Rect *rect, DN_F32 amount, DN_RectCutClip clip)
{
  DN_F32 min_x        = rect->pos.x;
  DN_F32 max_x        = rect->pos.x + rect->size.w;
  DN_F32 result_max_x = min_x + amount;
  if (clip)
    result_max_x = DN_Min(result_max_x, max_x);
  DN_Rect result = DN_RectFrom4N(min_x, rect->pos.y, result_max_x - min_x, rect->size.h);
  rect->pos.x    = result_max_x;
  rect->size.w   = max_x - result_max_x;
  return result;
}

DN_API DN_Rect DN_RectCutRightClip(DN_Rect *rect, DN_F32 amount, DN_RectCutClip clip)
{
  DN_F32 min_x        = rect->pos.x;
  DN_F32 max_x        = rect->pos.x + rect->size.w;
  DN_F32 result_min_x = max_x - amount;
  if (clip)
    result_min_x = DN_Max(result_min_x, 0);
  DN_Rect result = DN_RectFrom4N(result_min_x, rect->pos.y, max_x - result_min_x, rect->size.h);
  rect->size.w   = result_min_x - min_x;
  return result;
}

DN_API DN_Rect DN_RectCutTopClip(DN_Rect *rect, DN_F32 amount, DN_RectCutClip clip)
{
  DN_F32 min_y        = rect->pos.y;
  DN_F32 max_y        = rect->pos.y + rect->size.h;
  DN_F32 result_max_y = min_y + amount;
  if (clip)
    result_max_y = DN_Min(result_max_y, max_y);
  DN_Rect result = DN_RectFrom4N(rect->pos.x, min_y, rect->size.w, result_max_y - min_y);
  rect->pos.y    = result_max_y;
  rect->size.h   = max_y - result_max_y;
  return result;
}

DN_API DN_Rect DN_RectCutBottomClip(DN_Rect *rect, DN_F32 amount, DN_RectCutClip clip)
{
  DN_F32 min_y        = rect->pos.y;
  DN_F32 max_y        = rect->pos.y + rect->size.h;
  DN_F32 result_min_y = max_y - amount;
  if (clip)
    result_min_y = DN_Max(result_min_y, 0);
  DN_Rect result = DN_RectFrom4N(rect->pos.x, result_min_y, rect->size.w, max_y - result_min_y);
  rect->size.h   = result_min_y - min_y;
  return result;
}

DN_API DN_Rect DN_RectCutCut(DN_RectCut rect_cut, DN_V2F32 size, DN_RectCutClip clip)
{
  DN_Rect result = {};
  if (rect_cut.rect) {
    switch (rect_cut.side) {
      case DN_RectCutSide_Left: result = DN_RectCutLeftClip(rect_cut.rect, size.w, clip); break;
      case DN_RectCutSide_Right: result = DN_RectCutRightClip(rect_cut.rect, size.w, clip); break;
      case DN_RectCutSide_Top: result = DN_RectCutTopClip(rect_cut.rect, size.h, clip); break;
      case DN_RectCutSide_Bottom: result = DN_RectCutBottomClip(rect_cut.rect, size.h, clip); break;
    }
  }
  return result;
}

DN_API DN_V2F32 DN_RectInterpV2F32(DN_Rect rect, DN_V2F32 t01)
{
  DN_V2F32 result = DN_V2F32From2N(rect.pos.w + (rect.size.w * t01.x),
                                    rect.pos.h + (rect.size.h * t01.y));
  return result;
}

DN_API DN_V2F32 DN_RectTopLeft(DN_Rect rect)
{
  DN_V2F32 result = DN_RectInterpV2F32(rect, DN_V2F32From2N(0, 0));
  return result;
}

DN_API DN_V2F32 DN_RectTopRight(DN_Rect rect)
{
  DN_V2F32 result = DN_RectInterpV2F32(rect, DN_V2F32From2N(1, 0));
  return result;
}

DN_API DN_V2F32 DN_RectBottomLeft(DN_Rect rect)
{
  DN_V2F32 result = DN_RectInterpV2F32(rect, DN_V2F32From2N(0, 1));
  return result;
}

DN_API DN_V2F32 DN_RectBottomRight(DN_Rect rect)
{
  DN_V2F32 result = DN_RectInterpV2F32(rect, DN_V2F32From2N(1, 1));
  return result;
}

DN_API DN_RaycastV2 DN_RaycastLineIntersectV2(DN_V2F32 origin_a, DN_V2F32 dir_a, DN_V2F32 origin_b, DN_V2F32 dir_b)
{
  // NOTE: Parametric equation of a line
  //
  // p = o + (t*d)
  //
  // - o is the starting 2d point
  // - d is the direction of the line
  // - t is a scalar that scales along the direction of the point
  //
  // To determine if a ray intersections a ray, we want to solve
  //
  // (o_a + (t_a * d_a)) = (o_b + (t_b * d_b))
  //
  // Where '_a' and '_b' represent the 1st and 2nd point's origin, direction
  // and 't' components respectively. This is 2 equations with 2 unknowns
  // (`t_a` and `t_b`) which we can solve for by expressing the equation in
  // terms of `t_a` and `t_b`.
  //
  // Working that math out produces the formula below for 't'.

  DN_RaycastV2 result      = {};
  DN_F32       denominator = ((dir_b.y * dir_a.x) - (dir_b.x * dir_a.y));
  if (denominator != 0.0f) {
    result.t_a = (((origin_a.y - origin_b.y) * dir_b.x) + ((origin_b.x - origin_a.x) * dir_b.y)) / denominator;
    result.t_b = (((origin_a.y - origin_b.y) * dir_a.x) + ((origin_b.x - origin_a.x) * dir_a.y)) / denominator;
    result.hit = true;
  }
  return result;
}
