#if !defined(DN_UT_H)
  #error dn_utest.h must be included before this
#endif

#if !defined(DN_UT_IMPLEMENTATION)
  #error DN_UT_IMPLEMENTATION must be defined before dn_utest.h
#endif

#include <inttypes.h>

struct DN_TestsResult
{
  bool passed;
  int  total_tests;
  int  total_good_tests;
};

enum DN_TestsPrint
{
  DN_TestsPrint_No,
  DN_TestsPrint_OnFailure,
  DN_TestsPrint_Yes,
};

// NOTE: Taken from MSDN __cpuid example implementation
// https://learn.microsoft.com/en-us/cpp/intrinsics/cpuid-cpuidex?view=msvc-170
#if defined(DN_PLATFORM_WIN32) && defined(DN_COMPILER_MSVC)
struct DN_RefImplCPUReport {
  unsigned int    nIds_            = 0;
  unsigned int    nExIds_          = 0;
  char            vendor_[0x20]    = {};
  int             vendorSize_      = 0;
  char            brand_[0x40]     = {};
  int             brandSize_       = 0;
  bool            isIntel_         = false;
  bool            isAMD_           = false;
  uint32_t        f_1_ECX_         = 0;
  uint32_t        f_1_EDX_         = 0;
  uint32_t        f_7_EBX_         = 0;
  uint32_t        f_7_ECX_         = 0;
  uint32_t        f_81_ECX_        = 0;
  uint32_t        f_81_EDX_        = 0;
  int             data_[400][4]    = {};
  size_t          dataSize_        = 0;
  int             extdata_[400][4] = {};
  size_t          extdataSize_     = 0;

  bool SSE3(void)        const { return f_1_ECX_  &              (1 << 0); }
  bool PCLMULQDQ(void)   const { return f_1_ECX_  &              (1 << 1); }
  bool MONITOR(void)     const { return f_1_ECX_  &              (1 << 3); }
  bool SSSE3(void)       const { return f_1_ECX_  &              (1 << 9); }
  bool FMA(void)         const { return f_1_ECX_  &              (1 << 12); }
  bool CMPXCHG16B(void)  const { return f_1_ECX_  &              (1 << 13); }
  bool SSE41(void)       const { return f_1_ECX_  &              (1 << 19); }
  bool SSE42(void)       const { return f_1_ECX_  &              (1 << 20); }
  bool MOVBE(void)       const { return f_1_ECX_  &              (1 << 22); }
  bool POPCNT(void)      const { return f_1_ECX_  &              (1 << 23); }
  bool AES(void)         const { return f_1_ECX_  &              (1 << 25); }
  bool XSAVE(void)       const { return f_1_ECX_  &              (1 << 26); }
  bool OSXSAVE(void)     const { return f_1_ECX_  &              (1 << 27); }
  bool AVX(void)         const { return f_1_ECX_  &              (1 << 28); }
  bool F16C(void)        const { return f_1_ECX_  &              (1 << 29); }
  bool RDRAND(void)      const { return f_1_ECX_  &              (1 << 30); }

  bool MSR(void)         const { return f_1_EDX_  &              (1 << 5); }
  bool CX8(void)         const { return f_1_EDX_  &              (1 << 8); }
  bool SEP(void)         const { return f_1_EDX_  &              (1 << 11); }
  bool CMOV(void)        const { return f_1_EDX_  &              (1 << 15); }
  bool CLFSH(void)       const { return f_1_EDX_  &              (1 << 19); }
  bool MMX(void)         const { return f_1_EDX_  &              (1 << 23); }
  bool FXSR(void)        const { return f_1_EDX_  &              (1 << 24); }
  bool SSE(void)         const { return f_1_EDX_  &              (1 << 25); }
  bool SSE2(void)        const { return f_1_EDX_  &              (1 << 26); }

  bool FSGSBASE(void)    const { return f_7_EBX_  &              (1 << 0); }
  bool BMI1(void)        const { return f_7_EBX_  &              (1 << 3); }
  bool HLE(void)         const { return isIntel_  && f_7_EBX_ &  (1 << 4); }
  bool AVX2(void)        const { return f_7_EBX_  &              (1 << 5); }
  bool BMI2(void)        const { return f_7_EBX_  &              (1 << 8); }
  bool ERMS(void)        const { return f_7_EBX_  &              (1 << 9); }
  bool INVPCID(void)     const { return f_7_EBX_  &              (1 << 10); }
  bool RTM(void)         const { return isIntel_  && f_7_EBX_ &  (1 << 11); }
  bool AVX512F(void)     const { return f_7_EBX_  &              (1 << 16); }
  bool RDSEED(void)      const { return f_7_EBX_  &              (1 << 18); }
  bool ADX(void)         const { return f_7_EBX_  &              (1 << 19); }
  bool AVX512PF(void)    const { return f_7_EBX_  &              (1 << 26); }
  bool AVX512ER(void)    const { return f_7_EBX_  &              (1 << 27); }
  bool AVX512CD(void)    const { return f_7_EBX_  &              (1 << 28); }
  bool SHA(void)         const { return f_7_EBX_  &              (1 << 29); }

  bool PREFETCHWT1(void) const { return f_7_ECX_  &              (1 << 0); }

  bool LAHF(void)        const { return f_81_ECX_ &              (1 << 0); }
  bool LZCNT(void)       const { return isIntel_  && f_81_ECX_ & (1 << 5); }
  bool ABM(void)         const { return isAMD_    && f_81_ECX_ & (1 << 5); }
  bool SSE4a(void)       const { return isAMD_    && f_81_ECX_ & (1 << 6); }
  bool XOP(void)         const { return isAMD_    && f_81_ECX_ & (1 << 11); }
  bool TBM(void)         const { return isAMD_    && f_81_ECX_ & (1 << 21); }

  bool SYSCALL(void)     const { return isIntel_  && f_81_EDX_ & (1 << 11); }
  bool MMXEXT(void)      const { return isAMD_    && f_81_EDX_ & (1 << 22); }
  bool RDTSCP(void)      const { return f_81_EDX_ &              (1 << 27); }
  bool _3DNOWEXT(void)   const { return isAMD_    && f_81_EDX_ & (1 << 30); }
  bool _3DNOW(void)      const { return isAMD_    && f_81_EDX_ & (1 << 31); }
};

static DN_RefImplCPUReport DN_RefImplCPUReport_Init()
{
  DN_RefImplCPUReport result = {};

  // int cpuInfo[4] = {-1};
  int cpui[4];

  // Calling __cpuid with 0x0 as the function_id argument
  // gets the number of the highest valid function ID.
  __cpuid(cpui, 0);
  result.nIds_ = cpui[0];

  for (unsigned int i = 0; i <= result.nIds_; ++i) {
    __cpuidex(cpui, i, 0);
    memcpy(result.data_[result.dataSize_++], cpui, sizeof(cpui));
  }

  // Capture vendor string
  *reinterpret_cast<int *>(result.vendor_)     = result.data_[0][1];
  *reinterpret_cast<int *>(result.vendor_ + 4) = result.data_[0][3];
  *reinterpret_cast<int *>(result.vendor_ + 8) = result.data_[0][2];
  result.vendorSize_                           = (int)strlen(result.vendor_);

  if (strcmp(result.vendor_, "GenuineIntel") == 0)
    result.isIntel_ = true;
  else if (strcmp(result.vendor_, "AuthenticAMD") == 0)
    result.isAMD_ = true;

  // load bitset with flags for function 0x00000001
  if (result.nIds_ >= 1) {
    result.f_1_ECX_ = result.data_[1][2];
    result.f_1_EDX_ = result.data_[1][3];
  }

  // load bitset with flags for function 0x00000007
  if (result.nIds_ >= 7) {
    result.f_7_EBX_ = result.data_[7][1];
    result.f_7_ECX_ = result.data_[7][2];
  }

  // Calling __cpuid with 0x80000000 as the function_id argument
  // gets the number of the highest valid extended ID.
  __cpuid(cpui, 0x80000000);
  result.nExIds_ = cpui[0];

  for (unsigned int i = 0x80000000; i <= result.nExIds_; ++i) {
    __cpuidex(cpui, i, 0);
    memcpy(result.extdata_[result.extdataSize_++], cpui, sizeof(cpui));
  }

  // load bitset with flags for function 0x80000001
  if (result.nExIds_ >= 0x80000001) {
    result.f_81_ECX_ = result.extdata_[1][2];
    result.f_81_EDX_ = result.extdata_[1][3];
  }

  // Interpret CPU brand string if reported
  if (result.nExIds_ >= 0x80000004) {
    memcpy(result.brand_, result.extdata_[2], sizeof(cpui));
    memcpy(result.brand_ + 16, result.extdata_[3], sizeof(cpui));
    memcpy(result.brand_ + 32, result.extdata_[4], sizeof(cpui));
    result.brandSize_ = (int)strlen(result.brand_);
  }

  return result;
}

#if 0
static void DN_RefImpl_CPUReportDump() // Print out supported instruction set features
{
    auto support_message = [](std::string isa_feature, bool is_supported) {
        printf("%s %s\n", isa_feature.c_str(), is_supported ? "supported" : "not supported");
    };

    printf("%s\n", DN_RefImplCPUReport::Vendor().c_str());
    printf("%s\n", DN_RefImplCPUReport::Brand().c_str());

    support_message("3DNOW",       DN_RefImplCPUReport::_3DNOW());
    support_message("3DNOWEXT",    DN_RefImplCPUReport::_3DNOWEXT());
    support_message("ABM",         DN_RefImplCPUReport::ABM());
    support_message("ADX",         DN_RefImplCPUReport::ADX());
    support_message("AES",         DN_RefImplCPUReport::AES());
    support_message("AVX",         DN_RefImplCPUReport::AVX());
    support_message("AVX2",        DN_RefImplCPUReport::AVX2());
    support_message("AVX512CD",    DN_RefImplCPUReport::AVX512CD());
    support_message("AVX512ER",    DN_RefImplCPUReport::AVX512ER());
    support_message("AVX512F",     DN_RefImplCPUReport::AVX512F());
    support_message("AVX512PF",    DN_RefImplCPUReport::AVX512PF());
    support_message("BMI1",        DN_RefImplCPUReport::BMI1());
    support_message("BMI2",        DN_RefImplCPUReport::BMI2());
    support_message("CLFSH",       DN_RefImplCPUReport::CLFSH());
    support_message("CMPXCHG16B",  DN_RefImplCPUReport::CMPXCHG16B());
    support_message("CX8",         DN_RefImplCPUReport::CX8());
    support_message("ERMS",        DN_RefImplCPUReport::ERMS());
    support_message("F16C",        DN_RefImplCPUReport::F16C());
    support_message("FMA",         DN_RefImplCPUReport::FMA());
    support_message("FSGSBASE",    DN_RefImplCPUReport::FSGSBASE());
    support_message("FXSR",        DN_RefImplCPUReport::FXSR());
    support_message("HLE",         DN_RefImplCPUReport::HLE());
    support_message("INVPCID",     DN_RefImplCPUReport::INVPCID());
    support_message("LAHF",        DN_RefImplCPUReport::LAHF());
    support_message("LZCNT",       DN_RefImplCPUReport::LZCNT());
    support_message("MMX",         DN_RefImplCPUReport::MMX());
    support_message("MMXEXT",      DN_RefImplCPUReport::MMXEXT());
    support_message("MONITOR",     DN_RefImplCPUReport::MONITOR());
    support_message("MOVBE",       DN_RefImplCPUReport::MOVBE());
    support_message("MSR",         DN_RefImplCPUReport::MSR());
    support_message("OSXSAVE",     DN_RefImplCPUReport::OSXSAVE());
    support_message("PCLMULQDQ",   DN_RefImplCPUReport::PCLMULQDQ());
    support_message("POPCNT",      DN_RefImplCPUReport::POPCNT());
    support_message("PREFETCHWT1", DN_RefImplCPUReport::PREFETCHWT1());
    support_message("RDRAND",      DN_RefImplCPUReport::RDRAND());
    support_message("RDSEED",      DN_RefImplCPUReport::RDSEED());
    support_message("RDTSCP",      DN_RefImplCPUReport::RDTSCP());
    support_message("RTM",         DN_RefImplCPUReport::RTM());
    support_message("SEP",         DN_RefImplCPUReport::SEP());
    support_message("SHA",         DN_RefImplCPUReport::SHA());
    support_message("SSE",         DN_RefImplCPUReport::SSE());
    support_message("SSE2",        DN_RefImplCPUReport::SSE2());
    support_message("SSE3",        DN_RefImplCPUReport::SSE3());
    support_message("SSE4.1",      DN_RefImplCPUReport::SSE41());
    support_message("SSE4.2",      DN_RefImplCPUReport::SSE42());
    support_message("SSE4a",       DN_RefImplCPUReport::SSE4a());
    support_message("SSSE3",       DN_RefImplCPUReport::SSSE3());
    support_message("SYSCALL",     DN_RefImplCPUReport::SYSCALL());
    support_message("TBM",         DN_RefImplCPUReport::TBM());
    support_message("XOP",         DN_RefImplCPUReport::XOP());
    support_message("XSAVE",       DN_RefImplCPUReport::XSAVE());
};
#endif
#endif // defined(DN_PLATFORM_WIN32) && defined(DN_COMPILER_MSVC)

static DN_UTCore DN_Tests_Base()
{
  DN_UTCore result = DN_UT_Init();
#if defined(DN_PLATFORM_WIN32) && defined(DN_COMPILER_MSVC)
  DN_RefImplCPUReport ref_cpu_report = DN_RefImplCPUReport_Init();
  DN_UT_LogF(&result, "DN_Base\n");
  {
    for (DN_UT_Test(&result, "Query CPUID")) {
      DN_CPUReport cpu_report = DN_CPU_Report();

      // NOTE: Sanity check our report against MSDN's example ////////////////////////////////////////
      DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_3DNow) == ref_cpu_report._3DNOW());
      DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_3DNowExt) == ref_cpu_report._3DNOWEXT());
      DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_ABM) == ref_cpu_report.ABM());
      DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_AES) == ref_cpu_report.AES());
      DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_AVX) == ref_cpu_report.AVX());
      DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_AVX2) == ref_cpu_report.AVX2());
      DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_AVX512CD) == ref_cpu_report.AVX512CD());
      DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_AVX512ER) == ref_cpu_report.AVX512ER());
      DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_AVX512F) == ref_cpu_report.AVX512F());
      DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_AVX512PF) == ref_cpu_report.AVX512PF());
      DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_CMPXCHG16B) == ref_cpu_report.CMPXCHG16B());
      DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_F16C) == ref_cpu_report.F16C());
      DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_FMA) == ref_cpu_report.FMA());
      DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_MMX) == ref_cpu_report.MMX());
      DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_MmxExt) == ref_cpu_report.MMXEXT());
      DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_MONITOR) == ref_cpu_report.MONITOR());
      DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_MOVBE) == ref_cpu_report.MOVBE());
      DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_PCLMULQDQ) == ref_cpu_report.PCLMULQDQ());
      DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_POPCNT) == ref_cpu_report.POPCNT());
      DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_RDRAND) == ref_cpu_report.RDRAND());
      DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_RDSEED) == ref_cpu_report.RDSEED());
      DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_RDTSCP) == ref_cpu_report.RDTSCP());
      DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_SHA) == ref_cpu_report.SHA());
      DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_SSE) == ref_cpu_report.SSE());
      DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_SSE2) == ref_cpu_report.SSE2());
      DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_SSE3) == ref_cpu_report.SSE3());
      DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_SSE41) == ref_cpu_report.SSE41());
      DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_SSE42) == ref_cpu_report.SSE42());
      DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_SSE4A) == ref_cpu_report.SSE4a());
      DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_SSSE3) == ref_cpu_report.SSSE3());

  // NOTE: Feature flags we haven't bothered detecting yet but are in MSDN's example /////////////
  #if 0
            DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_ADX)         == DN_RefImplCPUReport::ADX());
            DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_BMI1)        == DN_RefImplCPUReport::BMI1());
            DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_BMI2)        == DN_RefImplCPUReport::BMI2());
            DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_CLFSH)       == DN_RefImplCPUReport::CLFSH());
            DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_CX8)         == DN_RefImplCPUReport::CX8());
            DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_ERMS)        == DN_RefImplCPUReport::ERMS());
            DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_FSGSBASE)    == DN_RefImplCPUReport::FSGSBASE());
            DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_FXSR)        == DN_RefImplCPUReport::FXSR());
            DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_HLE)         == DN_RefImplCPUReport::HLE());
            DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_INVPCID)     == DN_RefImplCPUReport::INVPCID());
            DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_LAHF)        == DN_RefImplCPUReport::LAHF());
            DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_LZCNT)       == DN_RefImplCPUReport::LZCNT());
            DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_MSR)         == DN_RefImplCPUReport::MSR());
            DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_OSXSAVE)     == DN_RefImplCPUReport::OSXSAVE());
            DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_PREFETCHWT1) == DN_RefImplCPUReport::PREFETCHWT1());
            DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_RTM)         == DN_RefImplCPUReport::RTM());
            DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_SEP)         == DN_RefImplCPUReport::SEP());
            DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_SYSCALL)     == DN_RefImplCPUReport::SYSCALL());
            DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_TBM)         == DN_RefImplCPUReport::TBM());
            DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_XOP)         == DN_RefImplCPUReport::XOP());
            DN_UT_Assert(&result, DN_CPU_HasFeature(&cpu_report, DN_CPUFeature_XSAVE)       == DN_RefImplCPUReport::XSAVE());
  #endif
    }
  }
#endif // defined(DN_PLATFORM_WIN32) && defined(DN_COMPILER_MSVC)
  return result;
}

static DN_UTCore DN_Tests_Arena()
{
  DN_UTCore result = DN_UT_Init();
  DN_UT_LogF(&result, "DN_Arena\n");
  {
    for (DN_UT_Test(&result, "Reused memory is zeroed out")) {
      uint8_t  alignment  = 1;
      DN_USize alloc_size = DN_Kilobytes(128);
      DN_Arena arena      = DN_Arena_InitFromOSVMem(0, 0, DN_ArenaFlags_Nil);
      DN_DEFER
      {
        DN_Arena_Deinit(&arena);
      };

      // NOTE: Allocate 128 kilobytes, fill it with garbage, then reset the arena
      uintptr_t first_ptr_address = 0;
      {
        DN_ArenaTempMem temp_mem = DN_Arena_TempMemBegin(&arena);
        void           *ptr      = DN_Arena_Alloc(&arena, alloc_size, alignment, DN_ZeroMem_Yes);
        first_ptr_address        = DN_CAST(uintptr_t) ptr;
        DN_Memset(ptr, 'z', alloc_size);
        DN_Arena_TempMemEnd(temp_mem);
      }

      // NOTE: Reallocate 128 kilobytes
      char *ptr = DN_CAST(char *) DN_Arena_Alloc(&arena, alloc_size, alignment, DN_ZeroMem_Yes);

      // NOTE: Double check we got the same pointer
      DN_UT_Assert(&result, first_ptr_address == DN_CAST(uintptr_t) ptr);

      // NOTE: Check that the bytes are set to 0
      for (DN_USize i = 0; i < alloc_size; i++)
        DN_UT_Assert(&result, ptr[i] == 0);
    }

    for (DN_UT_Test(&result, "Test arena grows naturally, 1mb + 4mb")) {
      // NOTE: Allocate 1mb, then 4mb, this should force the arena to grow
      DN_Arena arena = DN_Arena_InitFromOSVMem(DN_Megabytes(2), DN_Megabytes(2), DN_ArenaFlags_Nil);
      DN_DEFER
      {
        DN_Arena_Deinit(&arena);
      };

      char *ptr_1mb = DN_Arena_NewArray(&arena, char, DN_Megabytes(1), DN_ZeroMem_Yes);
      char *ptr_4mb = DN_Arena_NewArray(&arena, char, DN_Megabytes(4), DN_ZeroMem_Yes);
      DN_UT_Assert(&result, ptr_1mb);
      DN_UT_Assert(&result, ptr_4mb);

      DN_ArenaBlock const *block_4mb_begin = arena.curr;
      char const          *block_4mb_end   = DN_CAST(char *) block_4mb_begin + block_4mb_begin->reserve;

      DN_ArenaBlock const *block_1mb_begin = block_4mb_begin->prev;
      DN_UT_AssertF(&result, block_1mb_begin, "New block should have been allocated");
      char const *block_1mb_end = DN_CAST(char *) block_1mb_begin + block_1mb_begin->reserve;

      DN_UT_AssertF(&result, block_1mb_begin != block_4mb_begin, "New block should have been allocated and linked");
      DN_UT_AssertF(&result, ptr_1mb >= DN_CAST(char *) block_1mb_begin && ptr_1mb <= block_1mb_end, "Pointer was not allocated from correct memory block");
      DN_UT_AssertF(&result, ptr_4mb >= DN_CAST(char *) block_4mb_begin && ptr_4mb <= block_4mb_end, "Pointer was not allocated from correct memory block");
    }

    for (DN_UT_Test(&result, "Test arena grows naturally, 1mb, temp memory 4mb")) {
      DN_Arena arena = DN_Arena_InitFromOSVMem(DN_Megabytes(2), DN_Megabytes(2), DN_ArenaFlags_Nil);
      DN_DEFER
      {
        DN_Arena_Deinit(&arena);
      };

      // NOTE: Allocate 1mb, then 4mb, this should force the arena to grow
      char *ptr_1mb = DN_CAST(char *) DN_Arena_Alloc(&arena, DN_Megabytes(1), 1 /*align*/, DN_ZeroMem_Yes);
      DN_UT_Assert(&result, ptr_1mb);

      DN_ArenaTempMem temp_memory = DN_Arena_TempMemBegin(&arena);
      {
        char *ptr_4mb = DN_Arena_NewArray(&arena, char, DN_Megabytes(4), DN_ZeroMem_Yes);
        DN_UT_Assert(&result, ptr_4mb);

        DN_ArenaBlock const *block_4mb_begin = arena.curr;
        char const          *block_4mb_end   = DN_CAST(char *) block_4mb_begin + block_4mb_begin->reserve;

        DN_ArenaBlock const *block_1mb_begin = block_4mb_begin->prev;
        char const          *block_1mb_end   = DN_CAST(char *) block_1mb_begin + block_1mb_begin->reserve;

        DN_UT_AssertF(&result, block_1mb_begin != block_4mb_begin, "New block should have been allocated and linked");
        DN_UT_AssertF(&result, ptr_1mb >= DN_CAST(char *) block_1mb_begin && ptr_1mb <= block_1mb_end, "Pointer was not allocated from correct memory block");
        DN_UT_AssertF(&result, ptr_4mb >= DN_CAST(char *) block_4mb_begin && ptr_4mb <= block_4mb_end, "Pointer was not allocated from correct memory block");
      }
      DN_Arena_TempMemEnd(temp_memory);
      DN_UT_Assert(&result, arena.curr->prev == nullptr);
      DN_UT_AssertF(&result,
                    arena.curr->reserve >= DN_Megabytes(1),
                    "size=%" PRIu64 "MiB (%" PRIu64 "B), expect=%" PRIu64 "B",
                    (arena.curr->reserve / 1024 / 1024),
                    arena.curr->reserve,
                    DN_Megabytes(1));
    }
  }
  return result;
}

static DN_UTCore DN_Tests_Bin()
{
  DN_OSTLSTMem tmem = DN_OS_TLSTMem(nullptr);
  DN_UTCore    test = DN_UT_Init();
  DN_UT_LogF(&test, "DN_Bin\n");
  {
    for (DN_UT_Test(&test, "Convert 0x123")) {
      uint64_t result = DN_CVT_HexToU64(DN_STR8("0x123"));
      DN_UT_AssertF(&test, result == 0x123, "result: %" PRIu64, result);
    }

    for (DN_UT_Test(&test, "Convert 0xFFFF")) {
      uint64_t result = DN_CVT_HexToU64(DN_STR8("0xFFFF"));
      DN_UT_AssertF(&test, result == 0xFFFF, "result: %" PRIu64, result);
    }

    for (DN_UT_Test(&test, "Convert FFFF")) {
      uint64_t result = DN_CVT_HexToU64(DN_STR8("FFFF"));
      DN_UT_AssertF(&test, result == 0xFFFF, "result: %" PRIu64, result);
    }

    for (DN_UT_Test(&test, "Convert abCD")) {
      uint64_t result = DN_CVT_HexToU64(DN_STR8("abCD"));
      DN_UT_AssertF(&test, result == 0xabCD, "result: %" PRIu64, result);
    }

    for (DN_UT_Test(&test, "Convert 0xabCD")) {
      uint64_t result = DN_CVT_HexToU64(DN_STR8("0xabCD"));
      DN_UT_AssertF(&test, result == 0xabCD, "result: %" PRIu64, result);
    }

    for (DN_UT_Test(&test, "Convert 0x")) {
      uint64_t result = DN_CVT_HexToU64(DN_STR8("0x"));
      DN_UT_AssertF(&test, result == 0x0, "result: %" PRIu64, result);
    }

    for (DN_UT_Test(&test, "Convert 0X")) {
      uint64_t result = DN_CVT_HexToU64(DN_STR8("0X"));
      DN_UT_AssertF(&test, result == 0x0, "result: %" PRIu64, result);
    }

    for (DN_UT_Test(&test, "Convert 3")) {
      uint64_t result = DN_CVT_HexToU64(DN_STR8("3"));
      DN_UT_AssertF(&test, result == 3, "result: %" PRIu64, result);
    }

    for (DN_UT_Test(&test, "Convert f")) {
      uint64_t result = DN_CVT_HexToU64(DN_STR8("f"));
      DN_UT_AssertF(&test, result == 0xf, "result: %" PRIu64, result);
    }

    for (DN_UT_Test(&test, "Convert g")) {
      uint64_t result = DN_CVT_HexToU64(DN_STR8("g"));
      DN_UT_AssertF(&test, result == 0, "result: %" PRIu64, result);
    }

    for (DN_UT_Test(&test, "Convert -0x3")) {
      uint64_t result = DN_CVT_HexToU64(DN_STR8("-0x3"));
      DN_UT_AssertF(&test, result == 0, "result: %" PRIu64, result);
    }

    uint32_t number = 0xd095f6;
    for (DN_UT_Test(&test, "Convert %x to string", number)) {
      DN_Str8 number_hex = DN_CVT_BytesToHex(tmem.arena, &number, sizeof(number));
      DN_UT_AssertF(&test, DN_Str8_Eq(number_hex, DN_STR8("f695d000")), "number_hex=%.*s", DN_STR_FMT(number_hex));
    }

    number = 0xf6ed00;
    for (DN_UT_Test(&test, "Convert %x to string", number)) {
      DN_Str8 number_hex = DN_CVT_BytesToHex(tmem.arena, &number, sizeof(number));
      DN_UT_AssertF(&test, DN_Str8_Eq(number_hex, DN_STR8("00edf600")), "number_hex=%.*s", DN_STR_FMT(number_hex));
    }

    DN_Str8 hex = DN_STR8("0xf6ed00");
    for (DN_UT_Test(&test, "Convert %.*s to bytes", DN_STR_FMT(hex))) {
      DN_Str8 bytes = DN_CVT_HexToBytes(tmem.arena, hex);
      DN_UT_AssertF(&test,
                    DN_Str8_Eq(bytes, DN_STR8("\xf6\xed\x00")),
                    "number_hex=%.*s",
                    DN_STR_FMT(DN_CVT_BytesToHex(tmem.arena, bytes.data, bytes.size)));
    }
  }
  return test;
}

static DN_UTCore DN_Tests_BinarySearch()
{
  DN_UTCore result = DN_UT_Init();
  DN_UT_LogF(&result, "DN_BinarySearch\n");
  {
    for (DN_UT_Test(&result, "Search array of 1 item")) {
      uint32_t              array[] = {1};
      DN_BinarySearchResult search  = {};

      // NOTE: Match =============================================================================
      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 0U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 1U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 2U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 1);

      // NOTE: Lower bound =======================================================================
      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 0U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 1U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 2U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 1);

      // NOTE: Upper bound =======================================================================
      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 0U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 1U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 1);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 2U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 1);
    }

    for (DN_UT_Test(&result, "Search array of 2 items")) {
      uint32_t              array[] = {1};
      DN_BinarySearchResult search  = {};

      // NOTE: Match =============================================================================
      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 0U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 1U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 2U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 1);

      // NOTE: Lower bound =======================================================================
      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 0U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 1U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 2U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 1);

      // NOTE: Upper bound =======================================================================
      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 0U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 1U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 1);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 2U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 1);
    }

    for (DN_UT_Test(&result, "Search array of 3 items")) {
      uint32_t              array[] = {1, 2, 3};
      DN_BinarySearchResult search  = {};

      // NOTE: Match =============================================================================
      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 0U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 1U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 2U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 1);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 3U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 2);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 4U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 3);

      // NOTE: Lower bound =======================================================================
      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 0U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 1U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 2U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 1);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 3U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 2);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 4U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 3);

      // NOTE: Upper bound =======================================================================
      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 0U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 1U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 1);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 2U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 2);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 3U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 3);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 4U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 3);
    }

    for (DN_UT_Test(&result, "Search array of 4 items")) {
      uint32_t              array[] = {1, 2, 3, 4};
      DN_BinarySearchResult search  = {};

      // NOTE: Match =============================================================================
      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 0U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 1U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 2U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 1);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 3U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 2);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 4U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 3);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 5U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 4);

      // NOTE: Lower bound =======================================================================
      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 0U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 1U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 2U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 1);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 3U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 2);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 4U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 3);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 5U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 4);

      // NOTE: Upper bound =======================================================================
      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 0U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 1U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 1);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 2U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 2);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 3U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 3);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 4U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 4);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 5U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 4);
    }

    for (DN_UT_Test(&result, "Search array with duplicate items")) {
      uint32_t              array[] = {1, 1, 2, 2, 3};
      DN_BinarySearchResult search  = {};

      // NOTE: Match =============================================================================
      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 0U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 1U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 2U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 2);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 3U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 4);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 4U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 5);

      // NOTE: Lower bound =======================================================================
      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 0U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 1U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 2U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 2);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 3U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 4);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 4U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 5);

      // NOTE: Upper bound =======================================================================
      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 0U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 1U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 2);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 2U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 4);

      search = DN_BinarySearch<uint32_t>(array, DN_ArrayCountU(array), 3U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 5);
    }
  }
  return result;
}

static DN_UTCore DN_Tests_BaseContainers()
{
  DN_UTCore result = DN_UT_Init();

  DN_UT_LogF(&result, "DN_DSMap\n");
  {
    DN_OSTLSTMem tmem = DN_OS_TLSTMem(nullptr);
    {
      DN_Arena           arena    = DN_Arena_InitFromOSVMem(0, 0, DN_ArenaFlags_Nil);
      uint32_t const     MAP_SIZE = 64;
      DN_DSMap<uint64_t> map      = DN_DSMap_Init<uint64_t>(&arena, MAP_SIZE, DN_DSMapFlags_Nil);
      DN_DEFER
      {
        DN_DSMap_Deinit(&map, DN_ZeroMem_Yes);
      };

      for (DN_UT_Test(&result, "Find non-existent value")) {
        DN_DSMapResult<uint64_t> find = DN_DSMap_FindKeyStr8(&map, DN_STR8("Foo"));
        DN_UT_Assert(&result, !find.found);
        DN_UT_Assert(&result, map.size == MAP_SIZE);
        DN_UT_Assert(&result, map.initial_size == MAP_SIZE);
        DN_UT_Assert(&result, map.occupied == 1 /*Sentinel*/);
      }

      DN_DSMapKey key = DN_DSMap_KeyCStr8(&map, "Bar");
      for (DN_UT_Test(&result, "Insert value and lookup")) {
        uint64_t  desired_value = 0xF00BAA;
        uint64_t *slot_value    = DN_DSMap_Set(&map, key, desired_value).value;
        DN_UT_Assert(&result, slot_value);
        DN_UT_Assert(&result, map.size == MAP_SIZE);
        DN_UT_Assert(&result, map.initial_size == MAP_SIZE);
        DN_UT_Assert(&result, map.occupied == 2);

        uint64_t *value = DN_DSMap_Find(&map, key).value;
        DN_UT_Assert(&result, value);
        DN_UT_Assert(&result, *value == desired_value);
      }

      for (DN_UT_Test(&result, "Remove key")) {
        DN_DSMap_Erase(&map, key);
        DN_UT_Assert(&result, map.size == MAP_SIZE);
        DN_UT_Assert(&result, map.initial_size == MAP_SIZE);
        DN_UT_Assert(&result, map.occupied == 1 /*Sentinel*/);
      }
    }

    enum DSMapTestType
    {
      DSMapTestType_Set,
      DSMapTestType_MakeSlot,
      DSMapTestType_Count
    };

    for (int result_type = 0; result_type < DSMapTestType_Count; result_type++) {
      DN_Str8 prefix = {};
      switch (result_type) {
        case DSMapTestType_Set: prefix = DN_STR8("Set"); break;
        case DSMapTestType_MakeSlot: prefix = DN_STR8("Make slot"); break;
      }

      DN_ArenaTempMemScope temp_mem_scope = DN_ArenaTempMemScope(tmem.arena);
      DN_Arena             arena          = DN_Arena_InitFromOSVMem(0, 0, DN_ArenaFlags_Nil);
      uint32_t const       MAP_SIZE       = 64;
      DN_DSMap<uint64_t>   map            = DN_DSMap_Init<uint64_t>(&arena, MAP_SIZE, DN_DSMapFlags_Nil);
      DN_DEFER
      {
        DN_DSMap_Deinit(&map, DN_ZeroMem_Yes);
      };

      for (DN_UT_Test(&result, "%.*s: Test growing", DN_STR_FMT(prefix))) {
        uint64_t map_start_size = map.size;
        uint64_t value          = 0;
        uint64_t grow_threshold = map_start_size * 3 / 4;
        for (; map.occupied != grow_threshold; value++) {
          DN_DSMapKey key = DN_DSMap_KeyU64(&map, value);
          DN_UT_Assert(&result, !DN_DSMap_Find<uint64_t>(&map, key).found);
          DN_DSMapResult<uint64_t> make_result = {};
          if (result_type == DSMapTestType_Set)
            make_result = DN_DSMap_Set(&map, key, value);
          else
            make_result = DN_DSMap_Make(&map, key);
          DN_UT_Assert(&result, !make_result.found);
          DN_UT_Assert(&result, DN_DSMap_Find<uint64_t>(&map, key).value);
        }
        DN_UT_Assert(&result, map.initial_size == MAP_SIZE);
        DN_UT_Assert(&result, map.size == map_start_size);
        DN_UT_Assert(&result, map.occupied == 1 /*Sentinel*/ + value);

        { // NOTE: One more item should cause the table to grow by 2x
          DN_DSMapKey              key         = DN_DSMap_KeyU64(&map, value);
          DN_DSMapResult<uint64_t> make_result = {};
          if (result_type == DSMapTestType_Set)
            make_result = DN_DSMap_Set(&map, key, value);
          else
            make_result = DN_DSMap_Make(&map, key);

          value++;
          DN_UT_Assert(&result, !make_result.found);
          DN_UT_Assert(&result, map.size == map_start_size * 2);
          DN_UT_Assert(&result, map.initial_size == MAP_SIZE);
          DN_UT_Assert(&result, map.occupied == 1 /*Sentinel*/ + value);
        }
      }

      for (DN_UT_Test(&result, "%.*s: Check the sentinel is present", DN_STR_FMT(prefix))) {
        DN_DSMapSlot<uint64_t> NIL_SLOT = {};
        DN_DSMapSlot<uint64_t> sentinel = map.slots[DN_DS_MAP_SENTINEL_SLOT];
        DN_UT_Assert(&result, DN_Memcmp(&sentinel, &NIL_SLOT, sizeof(NIL_SLOT)) == 0);
      }

      for (DN_UT_Test(&result, "%.*s: Recheck all the hash tables values after growing", DN_STR_FMT(prefix))) {
        for (uint64_t index = 1 /*Sentinel*/; index < map.occupied; index++) {
          DN_DSMapSlot<uint64_t> const *slot = map.slots + index;

          // NOTE: Validate each slot value
          uint64_t    value_result = index - 1;
          DN_DSMapKey key          = DN_DSMap_KeyU64(&map, value_result);
          DN_UT_Assert(&result, DN_DSMap_KeyEquals(slot->key, key));
          if (result_type == DSMapTestType_Set)
            DN_UT_Assert(&result, slot->value == value_result);
          else
            DN_UT_Assert(&result, slot->value == 0); // NOTE: Make slot does not set the key so should be 0
          DN_UT_Assert(&result, slot->key.hash == DN_DSMap_Hash(&map, slot->key));

          // NOTE: Check the reverse lookup is correct
          DN_DSMapResult<uint64_t> check = DN_DSMap_Find(&map, slot->key);
          DN_UT_Assert(&result, slot->value == *check.value);
        }
      }

      for (DN_UT_Test(&result, "%.*s: Test shrinking", DN_STR_FMT(prefix))) {
        uint64_t start_map_size     = map.size;
        uint64_t start_map_occupied = map.occupied;
        uint64_t value              = 0;
        uint64_t shrink_threshold   = map.size * 1 / 4;
        for (; map.occupied != shrink_threshold; value++) {
          DN_DSMapKey key = DN_DSMap_KeyU64(&map, value);
          DN_UT_Assert(&result, DN_DSMap_Find<uint64_t>(&map, key).found);
          DN_DSMap_Erase(&map, key);
          DN_UT_Assert(&result, !DN_DSMap_Find<uint64_t>(&map, key).found);
        }
        DN_UT_Assert(&result, map.size == start_map_size);
        DN_UT_Assert(&result, map.occupied == start_map_occupied - value);

        { // NOTE: One more item should cause the table to shrink by 2x
          DN_DSMapKey key = DN_DSMap_KeyU64(&map, value);
          DN_DSMap_Erase(&map, key);
          value++;

          DN_UT_Assert(&result, map.size == start_map_size / 2);
          DN_UT_Assert(&result, map.occupied == start_map_occupied - value);
        }

        { // NOTE: Check the sentinel is present
          DN_DSMapSlot<uint64_t> NIL_SLOT = {};
          DN_DSMapSlot<uint64_t> sentinel = map.slots[DN_DS_MAP_SENTINEL_SLOT];
          DN_UT_Assert(&result, DN_Memcmp(&sentinel, &NIL_SLOT, sizeof(NIL_SLOT)) == 0);
        }

        // NOTE: Recheck all the hash table values after shrinking
        for (uint64_t index = 1 /*Sentinel*/; index < map.occupied; index++) {
          // NOTE: Generate the key
          uint64_t    value_result = value + (index - 1);
          DN_DSMapKey key          = DN_DSMap_KeyU64(&map, value_result);

          // NOTE: Validate each slot value
          DN_DSMapResult<uint64_t> find_result = DN_DSMap_Find(&map, key);
          DN_UT_Assert(&result, find_result.value);
          DN_UT_Assert(&result, find_result.slot->key == key);
          if (result_type == DSMapTestType_Set)
            DN_UT_Assert(&result, *find_result.value == value_result);
          else
            DN_UT_Assert(&result, *find_result.value == 0); // NOTE: Make slot does not set the key so should be 0
          DN_UT_Assert(&result, find_result.slot->key.hash == DN_DSMap_Hash(&map, find_result.slot->key));

          // NOTE: Check the reverse lookup is correct
          DN_DSMapResult<uint64_t> check = DN_DSMap_Find(&map, find_result.slot->key);
          DN_UT_Assert(&result, *find_result.value == *check.value);
        }

        for (; map.occupied != 1; value++) { // NOTE: Remove all items from the table
          DN_DSMapKey key = DN_DSMap_KeyU64(&map, value);
          DN_UT_Assert(&result, DN_DSMap_Find<uint64_t>(&map, key).found);
          DN_DSMap_Erase(&map, key);
          DN_UT_Assert(&result, !DN_DSMap_Find<uint64_t>(&map, key).found);
        }
        DN_UT_Assert(&result, map.initial_size == MAP_SIZE);
        DN_UT_Assert(&result, map.size == map.initial_size);
        DN_UT_Assert(&result, map.occupied == 1 /*Sentinel*/);
      }
    }
  }

  DN_UT_LogF(&result, "DN_IArray\n");
  {
    struct CustomArray
    {
      int     *data;
      DN_USize size;
      DN_USize max;
    };

    int         array_buffer[16];
    CustomArray array = {};
    array.data        = array_buffer;
    array.max         = DN_ArrayCountU(array_buffer);

    for (DN_UT_Test(&result, "Make item")) {
      int *item = DN_IArray_Make(&array, DN_ZeroMem_Yes);
      DN_UT_Assert(&result, item && array.size == 1);
    }
  }

  DN_UT_LogF(&result, "DN_CArray2");
  {
    for (DN_UT_Test(&result, "Positive count, middle of array, stable erase")) {
      int                 arr[]      = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      DN_USize            size       = 10;
      DN_ArrayEraseResult erase      = DN_CArray2_EraseRange(arr, &size, sizeof(arr[0]), 3, 2, DN_ArrayErase_Stable);
      int                 expected[] = {0, 1, 2, 5, 6, 7, 8, 9};
      DN_UT_Assert(&result, erase.items_erased == 2);
      DN_UT_Assert(&result, erase.it_index == 3);
      DN_UT_Assert(&result, size == 8);
      DN_UT_Assert(&result, DN_Memcmp(arr, expected, size * sizeof(arr[0])) == 0);
    }

    for (DN_UT_Test(&result, "Negative count, middle of array, stable erase")) {
      int                 arr[]      = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      DN_USize            size       = 10;
      DN_ArrayEraseResult erase      = DN_CArray2_EraseRange(arr, &size, sizeof(arr[0]), 5, -3, DN_ArrayErase_Stable);
      int                 expected[] = {0, 1, 2, 6, 7, 8, 9};
      DN_UT_Assert(&result, erase.items_erased == 3);
      DN_UT_Assert(&result, erase.it_index == 3);
      DN_UT_Assert(&result, size == 7);
      DN_UT_Assert(&result, DN_Memcmp(arr, expected, size * sizeof(arr[0])) == 0);
    }

    for (DN_UT_Test(&result, "count = -1, stable erase")) {
      int                 arr[]      = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      DN_USize            size       = 10;
      DN_ArrayEraseResult erase      = DN_CArray2_EraseRange(arr, &size, sizeof(arr[0]), 5, -1, DN_ArrayErase_Stable);
      int                 expected[] = {0, 1, 2, 3, 4, 6, 7, 8, 9};
      DN_UT_Assert(&result, erase.items_erased == 1);
      DN_UT_Assert(&result, erase.it_index == 5);
      DN_UT_Assert(&result, size == 9);
      DN_UT_Assert(&result, DN_Memcmp(arr, expected, size * sizeof(arr[0])) == 0);
    }

    for (DN_UT_Test(&result, "Positive count, unstable erase")) {
      int                 arr[]      = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      DN_USize            size       = 10;
      DN_ArrayEraseResult erase      = DN_CArray2_EraseRange(arr, &size, sizeof(arr[0]), 3, 2, DN_ArrayErase_Unstable);
      int                 expected[] = {0, 1, 2, 8, 9, 5, 6, 7};
      DN_UT_Assert(&result, erase.items_erased == 2);
      DN_UT_Assert(&result, erase.it_index == 3);
      DN_UT_Assert(&result, size == 8);
      DN_UT_Assert(&result, DN_Memcmp(arr, expected, size * sizeof(arr[0])) == 0);
    }

    for (DN_UT_Test(&result, "Negative count, unstable erase")) {
      int                 arr[]      = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      DN_USize            size       = 10;
      DN_ArrayEraseResult erase      = DN_CArray2_EraseRange(arr, &size, sizeof(arr[0]), 5, -3, DN_ArrayErase_Unstable);
      int                 expected[] = {0, 1, 2, 7, 8, 9, 6};
      DN_UT_Assert(&result, erase.items_erased == 3);
      DN_UT_Assert(&result, erase.it_index == 3);
      DN_UT_Assert(&result, size == 7);
      DN_UT_Assert(&result, DN_Memcmp(arr, expected, size * sizeof(arr[0])) == 0);
    }

    for (DN_UT_Test(&result, "Edge case - begin_index at start, negative count")) {
      int                 arr[]      = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      DN_USize            size       = 10;
      DN_ArrayEraseResult erase      = DN_CArray2_EraseRange(arr, &size, sizeof(arr[0]), 0, -2, DN_ArrayErase_Stable);
      int                 expected[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      DN_UT_Assert(&result, erase.items_erased == 0);
      DN_UT_Assert(&result, erase.it_index == 0);
      DN_UT_Assert(&result, size == 10);
      DN_UT_Assert(&result, DN_Memcmp(arr, expected, size * sizeof(arr[0])) == 0);
    }

    for (DN_UT_Test(&result, "Edge case - begin_index at end, positive count")) {
      int                 arr[]      = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      DN_USize            size       = 10;
      DN_ArrayEraseResult erase      = DN_CArray2_EraseRange(arr, &size, sizeof(arr[0]), 9, 2, DN_ArrayErase_Stable);
      int                 expected[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
      DN_UT_Assert(&result, erase.items_erased == 1);
      DN_UT_Assert(&result, erase.it_index == 9);
      DN_UT_Assert(&result, size == 9);
      DN_UT_Assert(&result, DN_Memcmp(arr, expected, size * sizeof(arr[0])) == 0);
    }

    for (DN_UT_Test(&result, "Invalid input - count = 0")) {
      int                 arr[]      = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      DN_USize            size       = 10;
      DN_ArrayEraseResult erase      = DN_CArray2_EraseRange(arr, &size, sizeof(arr[0]), 5, 0, DN_ArrayErase_Stable);
      int                 expected[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      DN_UT_Assert(&result, erase.items_erased == 0);
      DN_UT_Assert(&result, erase.it_index == 0);
      DN_UT_Assert(&result, size == 10);
      DN_UT_Assert(&result, DN_Memcmp(arr, expected, size * sizeof(arr[0])) == 0);
    }

    for (DN_UT_Test(&result, "Invalid input - null data")) {
      DN_USize            size  = 10;
      DN_ArrayEraseResult erase = DN_CArray2_EraseRange(nullptr, &size, sizeof(int), 5, 2, DN_ArrayErase_Stable);
      DN_UT_Assert(&result, erase.items_erased == 0);
      DN_UT_Assert(&result, erase.it_index == 0);
      DN_UT_Assert(&result, size == 10);
    }

    for (DN_UT_Test(&result, "Invalid input - null size")) {
      int                 arr[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      DN_ArrayEraseResult erase = DN_CArray2_EraseRange(arr, NULL, sizeof(arr[0]), 5, 2, DN_ArrayErase_Stable);
      DN_UT_Assert(&result, erase.items_erased == 0);
      DN_UT_Assert(&result, erase.it_index == 0);
    }

    for (DN_UT_Test(&result, "Invalid input - empty array")) {
      int                 arr[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      DN_USize            size  = 0;
      DN_ArrayEraseResult erase = DN_CArray2_EraseRange(arr, &size, sizeof(arr[0]), 5, 2, DN_ArrayErase_Stable);
      DN_UT_Assert(&result, erase.items_erased == 0);
      DN_UT_Assert(&result, erase.it_index == 0);
      DN_UT_Assert(&result, size == 0);
    }

    for (DN_UT_Test(&result, "Out-of-bounds begin_index")) {
      int                 arr[]      = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      DN_USize            size       = 10;
      DN_ArrayEraseResult erase      = DN_CArray2_EraseRange(arr, &size, sizeof(arr[0]), 15, 2, DN_ArrayErase_Stable);
      int                 expected[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      DN_UT_Assert(&result, erase.items_erased == 0);
      DN_UT_Assert(&result, erase.it_index == 10);
      DN_UT_Assert(&result, size == 10);
      DN_UT_Assert(&result, DN_Memcmp(arr, expected, size * sizeof(arr[0])) == 0);
    }
  }

  DN_UT_LogF(&result, "DN_FArray\n");
  {
    for (DN_UT_Test(&result, "Initialise from raw array")) {
      int  raw_array[] = {1, 2};
      auto array       = DN_FArray_Init<int, 4>(raw_array, DN_ArrayCountU(raw_array));
      DN_UT_Assert(&result, array.size == 2);
      DN_UT_Assert(&result, array.data[0] == 1);
      DN_UT_Assert(&result, array.data[1] == 2);
    }

    for (DN_UT_Test(&result, "Erase stable 1 element from array")) {
      int  raw_array[] = {1, 2, 3};
      auto array       = DN_FArray_Init<int, 4>(raw_array, DN_ArrayCountU(raw_array));
      DN_FArray_EraseRange(&array, 1 /*begin_index*/, 1 /*count*/, DN_ArrayErase_Stable);
      DN_UT_Assert(&result, array.size == 2);
      DN_UT_Assert(&result, array.data[0] == 1);
      DN_UT_Assert(&result, array.data[1] == 3);
    }

    for (DN_UT_Test(&result, "Erase unstable 1 element from array")) {
      int  raw_array[] = {1, 2, 3};
      auto array       = DN_FArray_Init<int, 4>(raw_array, DN_ArrayCountU(raw_array));
      DN_FArray_EraseRange(&array, 0 /*begin_index*/, 1 /*count*/, DN_ArrayErase_Unstable);
      DN_UT_Assert(&result, array.size == 2);
      DN_UT_Assert(&result, array.data[0] == 3);
      DN_UT_Assert(&result, array.data[1] == 2);
    }

    for (DN_UT_Test(&result, "Add 1 element to array")) {
      int const ITEM        = 2;
      int       raw_array[] = {1};
      auto      array       = DN_FArray_Init<int, 4>(raw_array, DN_ArrayCountU(raw_array));
      DN_FArray_Add(&array, ITEM);
      DN_UT_Assert(&result, array.size == 2);
      DN_UT_Assert(&result, array.data[0] == 1);
      DN_UT_Assert(&result, array.data[1] == ITEM);
    }

    for (DN_UT_Test(&result, "Clear array")) {
      int  raw_array[] = {1};
      auto array       = DN_FArray_Init<int, 4>(raw_array, DN_ArrayCountU(raw_array));
      DN_FArray_Clear(&array);
      DN_UT_Assert(&result, array.size == 0);
    }
  }

  DN_UT_LogF(&result, "DN_VArray\n");
  {
    {
      DN_VArray<uint32_t> array = DN_VArray_InitByteSize<uint32_t>(DN_Kilobytes(64));
      DN_DEFER
      {
        DN_VArray_Deinit(&array);
      };

      for (DN_UT_Test(&result, "Test adding an array of items to the array")) {
        uint32_t array_literal[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
        DN_VArray_AddArray<uint32_t>(&array, array_literal, DN_ArrayCountU(array_literal));
        DN_UT_Assert(&result, array.size == DN_ArrayCountU(array_literal));
        DN_UT_Assert(&result, DN_Memcmp(array.data, array_literal, DN_ArrayCountU(array_literal) * sizeof(array_literal[0])) == 0);
      }

      for (DN_UT_Test(&result, "Test stable erase, 1 item, the '2' value from the array")) {
        DN_VArray_EraseRange(&array, 2 /*begin_index*/, 1 /*count*/, DN_ArrayErase_Stable);
        uint32_t array_literal[] = {0, 1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
        DN_UT_Assert(&result, array.size == DN_ArrayCountU(array_literal));
        DN_UT_Assert(&result, DN_Memcmp(array.data, array_literal, DN_ArrayCountU(array_literal) * sizeof(array_literal[0])) == 0);
      }

      for (DN_UT_Test(&result, "Test unstable erase, 1 item, the '1' value from the array")) {
        DN_VArray_EraseRange(&array, 1 /*begin_index*/, 1 /*count*/, DN_ArrayErase_Unstable);
        uint32_t array_literal[] = {0, 15, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
        DN_UT_Assert(&result, array.size == DN_ArrayCountU(array_literal));
        DN_UT_Assert(&result, DN_Memcmp(array.data, array_literal, DN_ArrayCountU(array_literal) * sizeof(array_literal[0])) == 0);
      }

      DN_ArrayErase erase_enums[] = {DN_ArrayErase_Stable, DN_ArrayErase_Unstable};
      for (DN_UT_Test(&result, "Test un/stable erase, OOB")) {
        for (DN_ArrayErase erase : erase_enums) {
          uint32_t array_literal[] = {0, 15, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
          DN_VArray_EraseRange(&array, DN_ArrayCountU(array_literal) /*begin_index*/, DN_ArrayCountU(array_literal) + 100 /*count*/, erase);
          DN_UT_Assert(&result, array.size == DN_ArrayCountU(array_literal));
          DN_UT_Assert(&result, DN_Memcmp(array.data, array_literal, DN_ArrayCountU(array_literal) * sizeof(array_literal[0])) == 0);
        }
      }

      for (DN_UT_Test(&result, "Test flipped begin/end index stable erase, 2 items, the '15, 3' value from the array")) {
        DN_VArray_EraseRange(&array, 2 /*begin_index*/, -2 /*count*/, DN_ArrayErase_Stable);
        uint32_t array_literal[] = {0, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
        DN_UT_Assert(&result, array.size == DN_ArrayCountU(array_literal));
        DN_UT_Assert(&result, DN_Memcmp(array.data, array_literal, DN_ArrayCountU(array_literal) * sizeof(array_literal[0])) == 0);
      }

      for (DN_UT_Test(&result, "Test flipped begin/end index unstable erase, 2 items, the '4, 5' value from the array")) {
        DN_VArray_EraseRange(&array, 2 /*begin_index*/, -2 /*count*/, DN_ArrayErase_Unstable);
        uint32_t array_literal[] = {0, 13, 14, 6, 7, 8, 9, 10, 11, 12};
        DN_UT_Assert(&result, array.size == DN_ArrayCountU(array_literal));
        DN_UT_Assert(&result, DN_Memcmp(array.data, array_literal, DN_ArrayCountU(array_literal) * sizeof(array_literal[0])) == 0);
      }

      for (DN_UT_Test(&result, "Test stable erase range, 2+1 (oob) item, the '13, 14, +1 OOB' value from the array")) {
        DN_VArray_EraseRange(&array, 8 /*begin_index*/, 3 /*count*/, DN_ArrayErase_Stable);
        uint32_t array_literal[] = {0, 13, 14, 6, 7, 8, 9, 10};
        DN_UT_Assert(&result, array.size == DN_ArrayCountU(array_literal));
        DN_UT_Assert(&result, DN_Memcmp(array.data, array_literal, DN_ArrayCountU(array_literal) * sizeof(array_literal[0])) == 0);
      }

      for (DN_UT_Test(&result, "Test unstable erase range, 3+1 (oob) item, the '11, 12, +1 OOB' value from the array")) {
        DN_VArray_EraseRange(&array, 6 /*begin_index*/, 3 /*count*/, DN_ArrayErase_Unstable);
        uint32_t array_literal[] = {0, 13, 14, 6, 7, 8};
        DN_UT_Assert(&result, array.size == DN_ArrayCountU(array_literal));
        DN_UT_Assert(&result, DN_Memcmp(array.data, array_literal, DN_ArrayCountU(array_literal) * sizeof(array_literal[0])) == 0);
      }

      for (DN_UT_Test(&result, "Test stable erase -overflow OOB, erasing the '0, 13' value from the array")) {
        DN_VArray_EraseRange(&array, 1 /*begin_index*/, -DN_ISIZE_MAX /*count*/, DN_ArrayErase_Stable);
        uint32_t array_literal[] = {14, 6, 7, 8};
        DN_UT_Assert(&result, array.size == DN_ArrayCountU(array_literal));
        DN_UT_Assert(&result, DN_Memcmp(array.data, array_literal, DN_ArrayCountU(array_literal) * sizeof(array_literal[0])) == 0);
      }

      for (DN_UT_Test(&result, "Test unstable erase +overflow OOB, erasing the '7, 8' value from the array")) {
        DN_VArray_EraseRange(&array, 2 /*begin_index*/, DN_ISIZE_MAX /*count*/, DN_ArrayErase_Unstable);
        uint32_t array_literal[] = {14, 6};
        DN_UT_Assert(&result, array.size == DN_ArrayCountU(array_literal));
        DN_UT_Assert(&result, DN_Memcmp(array.data, array_literal, DN_ArrayCountU(array_literal) * sizeof(array_literal[0])) == 0);
      }

      for (DN_UT_Test(&result, "Test adding an array of items after erase")) {
        uint32_t array_literal[] = {0, 1, 2, 3};
        DN_VArray_AddArray<uint32_t>(&array, array_literal, DN_ArrayCountU(array_literal));

        uint32_t expected_literal[] = {14, 6, 0, 1, 2, 3};
        DN_UT_Assert(&result, array.size == DN_ArrayCountU(expected_literal));
        DN_UT_Assert(&result, DN_Memcmp(array.data, expected_literal, DN_ArrayCountU(expected_literal) * sizeof(expected_literal[0])) == 0);
      }
    }

    for (DN_UT_Test(&result, "Array of unaligned objects are contiguously laid out in memory")) {
      // NOTE: Since we allocate from a virtual memory block, each time
      // we request memory from the block we can demand some alignment
      // on the returned pointer from the memory block. If there's
      // additional alignment done in that function then we can no
      // longer access the items in the array contiguously leading to
      // confusing memory "corruption" errors.
      //
      // This result makes sure that the unaligned objects are allocated
      // from the memory block (and hence the array) contiguously
      // when the size of the object is not aligned with the required
      // alignment of the object.
      DN_MSVC_WARNING_PUSH
      DN_MSVC_WARNING_DISABLE(4324) // warning C4324: 'TestVArray::UnalignedObject': structure was padded due to alignment specifier

      struct alignas(8) UnalignedObject
      {
        char data[511];
      };

      DN_MSVC_WARNING_POP

      DN_VArray<UnalignedObject> array = DN_VArray_InitByteSize<UnalignedObject>(DN_Kilobytes(64));
      DN_DEFER
      {
        DN_VArray_Deinit(&array);
      };

      // NOTE: Verify that the items returned from the data array are
      // contiguous in memory.
      UnalignedObject *make_item_a = DN_VArray_MakeArray(&array, 1, DN_ZeroMem_Yes);
      UnalignedObject *make_item_b = DN_VArray_MakeArray(&array, 1, DN_ZeroMem_Yes);
      DN_Memset(make_item_a->data, 'a', sizeof(make_item_a->data));
      DN_Memset(make_item_b->data, 'b', sizeof(make_item_b->data));
      DN_UT_Assert(&result, (uintptr_t)make_item_b == (uintptr_t)(make_item_a + 1));

      // NOTE: Verify that accessing the items from the data array yield
      // the same object.
      DN_UT_Assert(&result, array.size == 2);
      UnalignedObject *data_item_a = array.data + 0;
      UnalignedObject *data_item_b = array.data + 1;
      DN_UT_Assert(&result, (uintptr_t)data_item_b == (uintptr_t)(data_item_a + 1));
      DN_UT_Assert(&result, (uintptr_t)data_item_b == (uintptr_t)(make_item_a + 1));
      DN_UT_Assert(&result, (uintptr_t)data_item_b == (uintptr_t)make_item_b);

      for (DN_USize i = 0; i < sizeof(data_item_a->data); i++)
        DN_UT_Assert(&result, data_item_a->data[i] == 'a');

      for (DN_USize i = 0; i < sizeof(data_item_b->data); i++)
        DN_UT_Assert(&result, data_item_b->data[i] == 'b');
    }
  }
  return result;
}

static DN_UTCore DN_Tests_FStr8()
{
  DN_UTCore result = DN_UT_Init();
  DN_UT_LogF(&result, "DN_FStr8\n");
  {
    for (DN_UT_Test(&result, "Append too much fails")) {
      DN_FStr8<4> str = {};
      DN_UT_Assert(&result, !DN_FStr8_Add(&str, DN_STR8("abcde")));
    }

    for (DN_UT_Test(&result, "Append format string too much fails")) {
      DN_FStr8<4> str = {};
      DN_UT_Assert(&result, !DN_FStr8_AddF(&str, "abcde"));
    }
  }
  return result;
}

static DN_UTCore DN_Tests_Intrinsics()
{
  DN_UTCore result = DN_UT_Init();
  // TODO(dn): We don't have meaningful results here, but since
  // atomics/intrinsics are implemented using macros we ensure the macro was
  // written properly with these results.

  DN_MSVC_WARNING_PUSH

  // NOTE: MSVC SAL complains that we are using Interlocked functionality on
  // variables it has detected as *not* being shared across threads. This is
  // fine, we're just running some basic results, so permit it.
  //
  // Warning 28112 is a knock-on effect of this that it doesn't like us
  // reading the value of the variable that has been used in an Interlocked
  // function locally.
  DN_MSVC_WARNING_DISABLE(28113) // Accessing a local variable val via an Interlocked function.
  DN_MSVC_WARNING_DISABLE(28112) // A variable (val) which is accessed via an Interlocked function must always be accessed via an Interlocked function. See line 759.

  DN_UT_LogF(&result, "DN_Atomic\n");
  {
    for (DN_UT_Test(&result, "DN_Atomic_AddU32")) {
      uint32_t val = 0;
      DN_Atomic_AddU32(&val, 1);
      DN_UT_AssertF(&result, val == 1, "val: %u", val);
    }

    for (DN_UT_Test(&result, "DN_Atomic_AddU64")) {
      uint64_t val = 0;
      DN_Atomic_AddU64(&val, 1);
      DN_UT_AssertF(&result, val == 1, "val: %" PRIu64, val);
    }

    for (DN_UT_Test(&result, "DN_Atomic_SubU32")) {
      uint32_t val = 1;
      DN_Atomic_SubU32(&val, 1);
      DN_UT_AssertF(&result, val == 0, "val: %u", val);
    }

    for (DN_UT_Test(&result, "DN_Atomic_SubU64")) {
      uint64_t val = 1;
      DN_Atomic_SubU64(&val, 1);
      DN_UT_AssertF(&result, val == 0, "val: %" PRIu64, val);
    }

    for (DN_UT_Test(&result, "DN_Atomic_SetValue32")) {
      DN_U32 a = 0;
      DN_U32 b = 111;
      DN_Atomic_SetValue32(&a, b);
      DN_UT_AssertF(&result, a == b, "a: %ld, b: %ld", a, b);
    }

    for (DN_UT_Test(&result, "DN_Atomic_SetValue64")) {
      int64_t a = 0;
      int64_t b = 111;
      DN_Atomic_SetValue64(DN_CAST(uint64_t *) & a, b);
      DN_UT_AssertF(&result, a == b, "a: %" PRId64 ", b: %" PRId64, a, b);
    }

    DN_UT_BeginF(&result, "DN_CPU_TSC");
    DN_CPU_TSC();
    DN_UT_End(&result);

    DN_UT_BeginF(&result, "DN_CompilerReadBarrierAndCPUReadFence");
    DN_CompilerReadBarrierAndCPUReadFence;
    DN_UT_End(&result);

    DN_UT_BeginF(&result, "DN_CompilerWriteBarrierAndCPUWriteFence");
    DN_CompilerWriteBarrierAndCPUWriteFence;
    DN_UT_End(&result);
  }
  DN_MSVC_WARNING_POP

  return result;
}

#if defined(DN_UNIT_TESTS_WITH_KECCAK)
DN_GCC_WARNING_PUSH
DN_GCC_WARNING_DISABLE(-Wunused - parameter)
DN_GCC_WARNING_DISABLE(-Wsign - compare)

DN_MSVC_WARNING_PUSH
DN_MSVC_WARNING_DISABLE(4244)
DN_MSVC_WARNING_DISABLE(4100)
DN_MSVC_WARNING_DISABLE(6385)
// NOTE: Keccak Reference Implementation ///////////////////////////////////////////////////////////
// A very compact Keccak implementation taken from the reference implementation
// repository
//
// https://github.com/XKCP/XKCP/blob/master/Standalone/CompactFIPS202/C/Keccak-more-compact.c

  #define FOR(i, n) for (i = 0; i < n; ++i)
void DN_RefImpl_Keccak_(int r, int c, const uint8_t *in, uint64_t inLen, uint8_t sfx, uint8_t *out, uint64_t outLen);

void DN_RefImpl_FIPS202_SHAKE128_(const uint8_t *in, uint64_t inLen, uint8_t *out, uint64_t outLen)
{
  DN_RefImpl_Keccak_(1344, 256, in, inLen, 0x1F, out, outLen);
}

void DN_RefImpl_FIPS202_SHAKE256_(const uint8_t *in, uint64_t inLen, uint8_t *out, uint64_t outLen)
{
  DN_RefImpl_Keccak_(1088, 512, in, inLen, 0x1F, out, outLen);
}

void DN_RefImpl_FIPS202_SHA3_224_(const uint8_t *in, uint64_t inLen, uint8_t *out)
{
  DN_RefImpl_Keccak_(1152, 448, in, inLen, 0x06, out, 28);
}

void DN_RefImpl_FIPS202_SHA3_256_(const uint8_t *in, uint64_t inLen, uint8_t *out)
{
  DN_RefImpl_Keccak_(1088, 512, in, inLen, 0x06, out, 32);
}

void DN_RefImpl_FIPS202_SHA3_384_(const uint8_t *in, uint64_t inLen, uint8_t *out)
{
  DN_RefImpl_Keccak_(832, 768, in, inLen, 0x06, out, 48);
}

void DN_RefImpl_FIPS202_SHA3_512_(const uint8_t *in, uint64_t inLen, uint8_t *out)
{
  DN_RefImpl_Keccak_(576, 1024, in, inLen, 0x06, out, 64);
}

int DN_RefImpl_LFSR86540_(uint8_t *R)
{
  (*R) = ((*R) << 1) ^ (((*R) & 0x80) ? 0x71 : 0);
  return ((*R) & 2) >> 1;
}

  #define ROL(a, o) ((((uint64_t)a) << o) ^ (((uint64_t)a) >> (64 - o)))

static uint64_t DN_RefImpl_load64_(const uint8_t *x)
{
  int      i;
  uint64_t u = 0;
  FOR(i, 8)
  {
    u <<= 8;
    u |= x[7 - i];
  }
  return u;
}

static void DN_RefImpl_store64_(uint8_t *x, uint64_t u)
{
  int i;
  FOR(i, 8)
  {
    x[i] = u;
    u >>= 8;
  }
}

static void DN_RefImpl_xor64_(uint8_t *x, uint64_t u)
{
  int i;
  FOR(i, 8)
  {
    x[i] ^= u;
    u >>= 8;
  }
}

  #define rL(x, y)    DN_RefImpl_load64_((uint8_t *)s + 8 * (x + 5 * y))
  #define wL(x, y, l) DN_RefImpl_store64_((uint8_t *)s + 8 * (x + 5 * y), l)
  #define XL(x, y, l) DN_RefImpl_xor64_((uint8_t *)s + 8 * (x + 5 * y), l)

void DN_RefImpl_Keccak_F1600(void *s)
{
  int      r, x, y, i, j, Y;
  uint8_t  R = 0x01;
  uint64_t C[5], D;
  for (i = 0; i < 24; i++) {
    /*??*/ FOR(x, 5) C[x] = rL(x, 0) ^ rL(x, 1) ^ rL(x, 2) ^ rL(x, 3) ^ rL(x, 4);
    FOR(x, 5)
    {
      D = C[(x + 4) % 5] ^ ROL(C[(x + 1) % 5], 1);
      FOR(y, 5)
      XL(x, y, D);
    }
    /*????*/ x = 1;
    y = r = 0;
    D     = rL(x, y);
    FOR(j, 24)
    {
      r += j + 1;
      Y    = (2 * x + 3 * y) % 5;
      x    = y;
      y    = Y;
      C[0] = rL(x, y);
      wL(x, y, ROL(D, r % 64));
      D = C[0];
    }
    /*??*/ FOR(y, 5)
    {
      FOR(x, 5)
      C[x] = rL(x, y);
      FOR(x, 5)
      wL(x, y, C[x] ^ ((~C[(x + 1) % 5]) & C[(x + 2) % 5]));
    }
    /*??*/ FOR(j, 7) if (DN_RefImpl_LFSR86540_(&R)) XL(0, 0, (uint64_t)1 << ((1 << j) - 1));
  }
}

void DN_RefImpl_Keccak_(int r, int c, const uint8_t *in, uint64_t inLen, uint8_t sfx, uint8_t *out, uint64_t outLen)
{
  /*initialize*/ uint8_t s[200];
  int                    R = r / 8;
  int                    i, b = 0;
  FOR(i, 200)
  s[i] = 0;
  /*absorb*/ while (inLen > 0) {
    b = (inLen < R) ? inLen : R;
    FOR(i, b)
    s[i] ^= in[i];
    in += b;
    inLen -= b;
    if (b == R) {
      DN_RefImpl_Keccak_F1600(s);
      b = 0;
    }
  }
  /*pad*/ s[b] ^= sfx;
  if ((sfx & 0x80) && (b == (R - 1)))
    DN_RefImpl_Keccak_F1600(s);
  s[R - 1] ^= 0x80;
  DN_RefImpl_Keccak_F1600(s);
  /*squeeze*/ while (outLen > 0) {
    b = (outLen < R) ? outLen : R;
    FOR(i, b)
    out[i] = s[i];
    out += b;
    outLen -= b;
    if (outLen > 0)
      DN_RefImpl_Keccak_F1600(s);
  }
}

  #undef XL
  #undef wL
  #undef rL
  #undef ROL
  #undef FOR
DN_MSVC_WARNING_POP
DN_GCC_WARNING_POP

  #define DN_KC_IMPLEMENTATION
  #include "../Standalone/dn_keccak.h"

  #define DN_UT_HASH_X_MACRO                     \
    DN_UT_HASH_X_ENTRY(SHA3_224, "SHA3-224")     \
    DN_UT_HASH_X_ENTRY(SHA3_256, "SHA3-256")     \
    DN_UT_HASH_X_ENTRY(SHA3_384, "SHA3-384")     \
    DN_UT_HASH_X_ENTRY(SHA3_512, "SHA3-512")     \
    DN_UT_HASH_X_ENTRY(Keccak_224, "Keccak-224") \
    DN_UT_HASH_X_ENTRY(Keccak_256, "Keccak-256") \
    DN_UT_HASH_X_ENTRY(Keccak_384, "Keccak-384") \
    DN_UT_HASH_X_ENTRY(Keccak_512, "Keccak-512") \
    DN_UT_HASH_X_ENTRY(Count, "Keccak-512")

enum DN_Tests__HashType
{

  #define DN_UT_HASH_X_ENTRY(enum_val, string) Hash_##enum_val,
  DN_UT_HASH_X_MACRO
  #undef DN_UT_HASH_X_ENTRY
};

DN_Str8 const DN_UT_HASH_STRING_[] =
    {
  #define DN_UT_HASH_X_ENTRY(enum_val, string) DN_STR8(string),
        DN_UT_HASH_X_MACRO
  #undef DN_UT_HASH_X_ENTRY
};

void DN_Tests_KeccakDispatch_(DN_UTCore *test, int hash_type, DN_Str8 input)
{
  DN_OSTLSTMem tmem      = DN_OS_TLSTMem(nullptr);
  DN_Str8      input_hex = DN_CVT_BytesToHex(tmem.arena, input.data, input.size);

  switch (hash_type) {
    case Hash_SHA3_224: {
      DN_KCBytes28 hash = DN_KC_SHA3_224Str8(input);
      DN_KCBytes28 expect;
      DN_RefImpl_FIPS202_SHA3_224_(DN_CAST(uint8_t *) input.data, input.size, (uint8_t *)expect.data);
      DN_UT_AssertF(test,
                    DN_KC_Bytes28Equals(&hash, &expect),
                    "\ninput:  %.*s"
                    "\nhash:   %.*s"
                    "\nexpect: %.*s",
                    DN_STR_FMT(input_hex),
                    DN_KC_STRING56_FMT(DN_KC_Bytes28ToHex(&hash).data),
                    DN_KC_STRING56_FMT(DN_KC_Bytes28ToHex(&expect).data));
    } break;

    case Hash_SHA3_256: {
      DN_KCBytes32 hash = DN_KC_SHA3_256Str8(input);
      DN_KCBytes32 expect;
      DN_RefImpl_FIPS202_SHA3_256_(DN_CAST(uint8_t *) input.data, input.size, (uint8_t *)expect.data);
      DN_UT_AssertF(test,
                    DN_KC_Bytes32Equals(&hash, &expect),
                    "\ninput:  %.*s"
                    "\nhash:   %.*s"
                    "\nexpect: %.*s",
                    DN_STR_FMT(input_hex),
                    DN_KC_STRING64_FMT(DN_KC_Bytes32ToHex(&hash).data),
                    DN_KC_STRING64_FMT(DN_KC_Bytes32ToHex(&expect).data));
    } break;

    case Hash_SHA3_384: {
      DN_KCBytes48 hash = DN_KC_SHA3_384Str8(input);
      DN_KCBytes48 expect;
      DN_RefImpl_FIPS202_SHA3_384_(DN_CAST(uint8_t *) input.data, input.size, (uint8_t *)expect.data);
      DN_UT_AssertF(test,
                    DN_KC_Bytes48Equals(&hash, &expect),
                    "\ninput:  %.*s"
                    "\nhash:   %.*s"
                    "\nexpect: %.*s",
                    DN_STR_FMT(input_hex),
                    DN_KC_STRING96_FMT(DN_KC_Bytes48ToHex(&hash).data),
                    DN_KC_STRING96_FMT(DN_KC_Bytes48ToHex(&expect).data));
    } break;

    case Hash_SHA3_512: {
      DN_KCBytes64 hash = DN_KC_SHA3_512Str8(input);
      DN_KCBytes64 expect;
      DN_RefImpl_FIPS202_SHA3_512_(DN_CAST(uint8_t *) input.data, input.size, (uint8_t *)expect.data);
      DN_UT_AssertF(test,
                    DN_KC_Bytes64Equals(&hash, &expect),
                    "\ninput:  %.*s"
                    "\nhash:   %.*s"
                    "\nexpect: %.*s",
                    DN_STR_FMT(input_hex),
                    DN_KC_STRING128_FMT(DN_KC_Bytes64ToHex(&hash).data),
                    DN_KC_STRING128_FMT(DN_KC_Bytes64ToHex(&expect).data));
    } break;

    case Hash_Keccak_224: {
      DN_KCBytes28 hash = DN_KC_Keccak224Str8(input);
      DN_KCBytes28 expect;
      DN_RefImpl_Keccak_(1152, 448, DN_CAST(uint8_t *) input.data, input.size, 0x01, (uint8_t *)expect.data, sizeof(expect));
      DN_UT_AssertF(test,
                    DN_KC_Bytes28Equals(&hash, &expect),
                    "\ninput:  %.*s"
                    "\nhash:   %.*s"
                    "\nexpect: %.*s",
                    DN_STR_FMT(input_hex),
                    DN_KC_STRING56_FMT(DN_KC_Bytes28ToHex(&hash).data),
                    DN_KC_STRING56_FMT(DN_KC_Bytes28ToHex(&expect).data));
    } break;

    case Hash_Keccak_256: {
      DN_KCBytes32 hash = DN_KC_Keccak256Str8(input);
      DN_KCBytes32 expect;
      DN_RefImpl_Keccak_(1088, 512, DN_CAST(uint8_t *) input.data, input.size, 0x01, (uint8_t *)expect.data, sizeof(expect));
      DN_UT_AssertF(test,
                    DN_KC_Bytes32Equals(&hash, &expect),
                    "\ninput:  %.*s"
                    "\nhash:   %.*s"
                    "\nexpect: %.*s",
                    DN_STR_FMT(input_hex),
                    DN_KC_STRING64_FMT(DN_KC_Bytes32ToHex(&hash).data),
                    DN_KC_STRING64_FMT(DN_KC_Bytes32ToHex(&expect).data));
    } break;

    case Hash_Keccak_384: {
      DN_KCBytes48 hash = DN_KC_Keccak384Str8(input);
      DN_KCBytes48 expect;
      DN_RefImpl_Keccak_(832, 768, DN_CAST(uint8_t *) input.data, input.size, 0x01, (uint8_t *)expect.data, sizeof(expect));
      DN_UT_AssertF(test,
                    DN_KC_Bytes48Equals(&hash, &expect),
                    "\ninput:  %.*s"
                    "\nhash:   %.*s"
                    "\nexpect: %.*s",
                    DN_STR_FMT(input_hex),
                    DN_KC_STRING96_FMT(DN_KC_Bytes48ToHex(&hash).data),
                    DN_KC_STRING96_FMT(DN_KC_Bytes48ToHex(&expect).data));
    } break;

    case Hash_Keccak_512: {
      DN_KCBytes64 hash = DN_KC_Keccak512Str8(input);
      DN_KCBytes64 expect;
      DN_RefImpl_Keccak_(576, 1024, DN_CAST(uint8_t *) input.data, input.size, 0x01, (uint8_t *)expect.data, sizeof(expect));
      DN_UT_AssertF(test,
                    DN_KC_Bytes64Equals(&hash, &expect),
                    "\ninput:  %.*s"
                    "\nhash:   %.*s"
                    "\nexpect: %.*s",
                    DN_STR_FMT(input_hex),
                    DN_KC_STRING128_FMT(DN_KC_Bytes64ToHex(&hash).data),
                    DN_KC_STRING128_FMT(DN_KC_Bytes64ToHex(&expect).data));
    } break;
  }
}

DN_UTCore DN_Tests_Keccak()
{
  DN_UTCore     test     = DN_UT_Init();
  DN_Str8 const INPUTS[] = {
      DN_STR8("abc"),
      DN_STR8(""),
      DN_STR8("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"),
      DN_STR8("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmno"
              "pqrstnopqrstu"),
  };

  DN_UT_LogF(&test, "DN_KC\n");
  {
    for (int hash_type = 0; hash_type < Hash_Count; hash_type++) {
      DN_PCG32 rng = DN_PCG32_Init(0xd48e'be21'2af8'733d);
      for (DN_Str8 input : INPUTS) {
        DN_UT_BeginF(&test, "%.*s - Input: %.*s", DN_STR_FMT(DN_UT_HASH_STRING_[hash_type]), DN_CAST(int) DN_Min(input.size, 54), input.data);
        DN_Tests_KeccakDispatch_(&test, hash_type, input);
        DN_UT_End(&test);
      }

      DN_UT_BeginF(&test, "%.*s - Deterministic random inputs", DN_STR_FMT(DN_UT_HASH_STRING_[hash_type]));
      for (DN_USize index = 0; index < 128; index++) {
        char     src[4096] = {};
        uint32_t src_size  = DN_PCG32_Range(&rng, 0, sizeof(src));

        for (DN_USize src_index = 0; src_index < src_size; src_index++)
          src[src_index] = DN_CAST(char) DN_PCG32_Range(&rng, 0, 255);

        DN_Str8 input = DN_Str8_Init(src, src_size);
        DN_Tests_KeccakDispatch_(&test, hash_type, input);
      }
      DN_UT_End(&test);
    }
  }
  return test;
}
#endif // defined(DN_UNIT_TESTS_WITH_KECCAK)

static DN_UTCore DN_Tests_M4()
{
  DN_UTCore result = DN_UT_Init();
  DN_UT_LogF(&result, "DN_M4\n");
  {
    for (DN_UT_Test(&result, "Simple translate and scale matrix")) {
      DN_M4 translate  = DN_M4_TranslateF(1, 2, 3);
      DN_M4 scale      = DN_M4_ScaleF(2, 2, 2);
      DN_M4 mul_result = DN_M4_Mul(translate, scale);

      const DN_M4 EXPECT = {
          {
           {2, 0, 0, 0},
           {0, 2, 0, 0},
           {0, 0, 2, 0},
           {1, 2, 3, 1},
           }
      };

      DN_UT_AssertF(&result,
                    memcmp(mul_result.columns, EXPECT.columns, sizeof(EXPECT)) == 0,
                    "\nresult =\n%s\nexpected =\n%s",
                    DN_M4_ColumnMajorString(mul_result).data,
                    DN_M4_ColumnMajorString(EXPECT).data);
    }
  }
  return result;
}

static DN_UTCore DN_Tests_OS()
{
  DN_UTCore result = DN_UT_Init();

#if defined(DN_OS_INC_CPP) || 1
  DN_UT_LogF(&result, "DN_OS\n");
  {
    for (DN_UT_Test(&result, "Generate secure RNG bytes with nullptr")) {
      DN_B32 os_result = DN_OS_SecureRNGBytes(nullptr, 1);
      DN_UT_Assert(&result, os_result == false);
    }

    for (DN_UT_Test(&result, "Generate secure RNG 32 bytes")) {
      char const ZERO[32]  = {};
      char       buf[32]   = {};
      bool       os_result = DN_OS_SecureRNGBytes(buf, DN_ArrayCountU(buf));
      DN_UT_Assert(&result, os_result);
      DN_UT_Assert(&result, DN_Memcmp(buf, ZERO, DN_ArrayCountU(buf)) != 0);
    }

    for (DN_UT_Test(&result, "Generate secure RNG 0 bytes")) {
      char buf[32]     = {};
      buf[0]           = 'Z';
      DN_B32 os_result = DN_OS_SecureRNGBytes(buf, 0);
      DN_UT_Assert(&result, os_result);
      DN_UT_Assert(&result, buf[0] == 'Z');
    }

    for (DN_UT_Test(&result, "Query executable directory")) {
      DN_OSTLSTMem tmem      = DN_OS_TLSTMem(nullptr);
      DN_Str8      os_result = DN_OS_EXEDir(tmem.arena);
      DN_UT_Assert(&result, DN_Str8_HasData(os_result));
      DN_UT_AssertF(&result, DN_OS_DirExists(os_result), "result(%zu): %.*s", os_result.size, DN_STR_FMT(os_result));
    }

    for (DN_UT_Test(&result, "DN_OS_PerfCounterNow")) {
      uint64_t os_result = DN_OS_PerfCounterNow();
      DN_UT_Assert(&result, os_result != 0);
    }

    for (DN_UT_Test(&result, "Consecutive ticks are ordered")) {
      uint64_t a = DN_OS_PerfCounterNow();
      uint64_t b = DN_OS_PerfCounterNow();
      DN_UT_AssertF(&result, b >= a, "a: %" PRIu64 ", b: %" PRIu64, a, b);
    }

    for (DN_UT_Test(&result, "Ticks to time are a correct order of magnitude")) {
      uint64_t a  = DN_OS_PerfCounterNow();
      uint64_t b  = DN_OS_PerfCounterNow();
      DN_F64   s  = DN_OS_PerfCounterS(a, b);
      DN_F64   ms = DN_OS_PerfCounterMs(a, b);
      DN_F64   us = DN_OS_PerfCounterUs(a, b);
      DN_F64   ns = DN_OS_PerfCounterNs(a, b);
      DN_UT_AssertF(&result, s <= ms, "s:  %f, ms: %f", s, ms);
      DN_UT_AssertF(&result, ms <= us, "ms: %f, us: %f", ms, us);
      DN_UT_AssertF(&result, us <= ns, "us: %f, ns: %f", us, ns);
    }
  }

  DN_UT_LogF(&result, "\nDN_OS Filesystem\n");
  {
    for (DN_UT_Test(&result, "Make directory recursive \"abcd/efgh\"")) {
      DN_UT_AssertF(&result, DN_OS_MakeDir(DN_STR8("abcd/efgh")), "Failed to make directory");
      DN_UT_AssertF(&result, DN_OS_DirExists(DN_STR8("abcd")), "Directory was not made");
      DN_UT_AssertF(&result, DN_OS_DirExists(DN_STR8("abcd/efgh")), "Subdirectory was not made");
      DN_UT_AssertF(&result, DN_OS_FileExists(DN_STR8("abcd")) == false, "This function should only return true for files");
      DN_UT_AssertF(&result, DN_OS_FileExists(DN_STR8("abcd/efgh")) == false, "This function should only return true for files");
      DN_UT_AssertF(&result, DN_OS_PathDelete(DN_STR8("abcd/efgh")), "Failed to delete directory");
      DN_UT_AssertF(&result, DN_OS_PathDelete(DN_STR8("abcd")), "Failed to cleanup directory");
    }

    for (DN_UT_Test(&result, "File write, read, copy, move and delete")) {
      // NOTE: Write step
      DN_Str8 const SRC_FILE     = DN_STR8("dn_result_file");
      DN_B32        write_result = DN_OS_WriteAll(SRC_FILE, DN_STR8("1234"), nullptr);
      DN_UT_Assert(&result, write_result);
      DN_UT_Assert(&result, DN_OS_FileExists(SRC_FILE));

      // NOTE: Read step
      DN_OSTLSTMem tmem      = DN_OS_TLSTMem(nullptr);
      DN_Str8      read_file = DN_OS_ReadAll(tmem.arena, SRC_FILE, nullptr);
      DN_UT_AssertF(&result, DN_Str8_HasData(read_file), "Failed to load file");
      DN_UT_AssertF(&result, read_file.size == 4, "File read wrong amount of bytes (%zu)", read_file.size);
      DN_UT_AssertF(&result, DN_Str8_Eq(read_file, DN_STR8("1234")), "Read %zu bytes instead of the expected 4: '%.*s'", read_file.size, DN_STR_FMT(read_file));

      // NOTE: Copy step
      DN_Str8 const COPY_FILE   = DN_STR8("dn_result_file_copy");
      DN_B32        copy_result = DN_OS_CopyFile(SRC_FILE, COPY_FILE, true /*overwrite*/, nullptr);
      DN_UT_Assert(&result, copy_result);
      DN_UT_Assert(&result, DN_OS_FileExists(COPY_FILE));

      // NOTE: Move step
      DN_Str8 const MOVE_FILE   = DN_STR8("dn_result_file_move");
      DN_B32        move_result = DN_OS_MoveFile(COPY_FILE, MOVE_FILE, true /*overwrite*/, nullptr);
      DN_UT_Assert(&result, move_result);
      DN_UT_Assert(&result, DN_OS_FileExists(MOVE_FILE));
      DN_UT_AssertF(&result, DN_OS_FileExists(COPY_FILE) == false, "Moving a file should remove the original");

      // NOTE: Delete step
      DN_B32 delete_src_file   = DN_OS_PathDelete(SRC_FILE);
      DN_B32 delete_moved_file = DN_OS_PathDelete(MOVE_FILE);
      DN_UT_Assert(&result, delete_src_file);
      DN_UT_Assert(&result, delete_moved_file);

      // NOTE: Deleting non-existent file fails
      DN_B32 delete_non_existent_src_file   = DN_OS_PathDelete(SRC_FILE);
      DN_B32 delete_non_existent_moved_file = DN_OS_PathDelete(MOVE_FILE);
      DN_UT_Assert(&result, delete_non_existent_moved_file == false);
      DN_UT_Assert(&result, delete_non_existent_src_file == false);
    }
  }

  DN_UT_LogF(&result, "\nSemaphore\n");
  {
    DN_OSSemaphore sem = DN_OS_SemaphoreInit(0);

    for (DN_UT_Test(&result, "Wait timeout")) {
      DN_U64                   begin       = DN_OS_PerfCounterNow();
      DN_OSSemaphoreWaitResult wait_result = DN_OS_SemaphoreWait(&sem, 100 /*timeout_ms*/);
      DN_U64                   end         = DN_OS_PerfCounterNow();
      DN_UT_AssertF(&result, wait_result == DN_OSSemaphoreWaitResult_Timeout, "Received wait result %zu", wait_result);
      DN_F64 elapsed_ms = DN_OS_PerfCounterMs(begin, end);
      DN_UT_AssertF(&result, elapsed_ms >= 99 && elapsed_ms <= 120, "Expected to sleep for ~100ms, slept %f ms", elapsed_ms);
    }

    for (DN_UT_Test(&result, "Wait success")) {
      DN_OS_SemaphoreIncrement(&sem, 1);
      DN_OSSemaphoreWaitResult wait_result = DN_OS_SemaphoreWait(&sem, 0 /*timeout_ms*/);
      DN_UT_AssertF(&result, wait_result == DN_OSSemaphoreWaitResult_Success, "Received wait result %zu", wait_result);
    }

    DN_OS_SemaphoreDeinit(&sem);
  }

  DN_UT_LogF(&result, "\nMutex\n");
  {
    DN_OSMutex mutex = DN_OS_MutexInit();
    for (DN_UT_Test(&result, "Lock")) {
      DN_OS_MutexLock(&mutex);
      DN_OS_MutexUnlock(&mutex);
    }
    DN_OS_MutexDeinit(&mutex);
  }

  DN_UT_LogF(&result, "\nCondition Variable\n");
  {
    DN_OSMutex             mutex = DN_OS_MutexInit();
    DN_OSConditionVariable cv    = DN_OS_ConditionVariableInit();
    for (DN_UT_Test(&result, "Lock and timeout")) {
      DN_U64 begin = DN_OS_PerfCounterNow();
      DN_OS_ConditionVariableWait(&cv, &mutex, 100 /*sleep_ms*/);
      DN_U64 end        = DN_OS_PerfCounterNow();
      DN_F64 elapsed_ms = DN_OS_PerfCounterMs(begin, end);
      DN_UT_AssertF(&result, elapsed_ms >= 99 && elapsed_ms <= 120, "Expected to sleep for ~100ms, slept %f ms", elapsed_ms);
    }
    DN_OS_MutexDeinit(&mutex);
    DN_OS_ConditionVariableDeinit(&cv);
  }
#endif

  return result;
}

static DN_UTCore DN_Tests_Rect()
{
  DN_UTCore result = DN_UT_Init();
  DN_UT_LogF(&result, "DN_Rect\n");
  {
    for (DN_UT_Test(&result, "No intersection")) {
      DN_Rect a  = DN_Rect_Init2V2(DN_V2F32_Init1N(0), DN_V2F32_Init2N(100, 100));
      DN_Rect b  = DN_Rect_Init2V2(DN_V2F32_Init2N(200, 0), DN_V2F32_Init2N(200, 200));
      DN_Rect ab = DN_Rect_Intersection(a, b);

      DN_V2F32 ab_max = ab.pos + ab.size;
      DN_UT_AssertF(&result,
                    ab.pos.x == 0 && ab.pos.y == 0 && ab_max.x == 0 && ab_max.y == 0,
                    "ab = { min.x = %.2f, min.y = %.2f, max.x = %.2f. max.y = %.2f }",
                    ab.pos.x,
                    ab.pos.y,
                    ab_max.x,
                    ab_max.y);
    }

    for (DN_UT_Test(&result, "A's min intersects B")) {
      DN_Rect a  = DN_Rect_Init2V2(DN_V2F32_Init2N(50, 50), DN_V2F32_Init2N(100, 100));
      DN_Rect b  = DN_Rect_Init2V2(DN_V2F32_Init2N(0, 0), DN_V2F32_Init2N(100, 100));
      DN_Rect ab = DN_Rect_Intersection(a, b);

      DN_V2F32 ab_max = ab.pos + ab.size;
      DN_UT_AssertF(&result,
                    ab.pos.x == 50 && ab.pos.y == 50 && ab_max.x == 100 && ab_max.y == 100,
                    "ab = { min.x = %.2f, min.y = %.2f, max.x = %.2f. max.y = %.2f }",
                    ab.pos.x,
                    ab.pos.y,
                    ab_max.x,
                    ab_max.y);
    }

    for (DN_UT_Test(&result, "B's min intersects A")) {
      DN_Rect a  = DN_Rect_Init2V2(DN_V2F32_Init2N(0, 0), DN_V2F32_Init2N(100, 100));
      DN_Rect b  = DN_Rect_Init2V2(DN_V2F32_Init2N(50, 50), DN_V2F32_Init2N(100, 100));
      DN_Rect ab = DN_Rect_Intersection(a, b);

      DN_V2F32 ab_max = ab.pos + ab.size;
      DN_UT_AssertF(&result,
                    ab.pos.x == 50 && ab.pos.y == 50 && ab_max.x == 100 && ab_max.y == 100,
                    "ab = { min.x = %.2f, min.y = %.2f, max.x = %.2f. max.y = %.2f }",
                    ab.pos.x,
                    ab.pos.y,
                    ab_max.x,
                    ab_max.y);
    }

    for (DN_UT_Test(&result, "A's max intersects B")) {
      DN_Rect a  = DN_Rect_Init2V2(DN_V2F32_Init2N(-50, -50), DN_V2F32_Init2N(100, 100));
      DN_Rect b  = DN_Rect_Init2V2(DN_V2F32_Init2N(0, 0), DN_V2F32_Init2N(100, 100));
      DN_Rect ab = DN_Rect_Intersection(a, b);

      DN_V2F32 ab_max = ab.pos + ab.size;
      DN_UT_AssertF(&result,
                    ab.pos.x == 0 && ab.pos.y == 0 && ab_max.x == 50 && ab_max.y == 50,
                    "ab = { min.x = %.2f, min.y = %.2f, max.x = %.2f. max.y = %.2f }",
                    ab.pos.x,
                    ab.pos.y,
                    ab_max.x,
                    ab_max.y);
    }

    for (DN_UT_Test(&result, "B's max intersects A")) {
      DN_Rect a  = DN_Rect_Init2V2(DN_V2F32_Init2N(0, 0), DN_V2F32_Init2N(100, 100));
      DN_Rect b  = DN_Rect_Init2V2(DN_V2F32_Init2N(-50, -50), DN_V2F32_Init2N(100, 100));
      DN_Rect ab = DN_Rect_Intersection(a, b);

      DN_V2F32 ab_max = ab.pos + ab.size;
      DN_UT_AssertF(&result,
                    ab.pos.x == 0 && ab.pos.y == 0 && ab_max.x == 50 && ab_max.y == 50,
                    "ab = { min.x = %.2f, min.y = %.2f, max.x = %.2f. max.y = %.2f }",
                    ab.pos.x,
                    ab.pos.y,
                    ab_max.x,
                    ab_max.y);
    }

    for (DN_UT_Test(&result, "B contains A")) {
      DN_Rect a  = DN_Rect_Init2V2(DN_V2F32_Init2N(25, 25), DN_V2F32_Init2N(25, 25));
      DN_Rect b  = DN_Rect_Init2V2(DN_V2F32_Init2N(0, 0), DN_V2F32_Init2N(100, 100));
      DN_Rect ab = DN_Rect_Intersection(a, b);

      DN_V2F32 ab_max = ab.pos + ab.size;
      DN_UT_AssertF(&result,
                    ab.pos.x == 25 && ab.pos.y == 25 && ab_max.x == 50 && ab_max.y == 50,
                    "ab = { min.x = %.2f, min.y = %.2f, max.x = %.2f. max.y = %.2f }",
                    ab.pos.x,
                    ab.pos.y,
                    ab_max.x,
                    ab_max.y);
    }

    for (DN_UT_Test(&result, "A contains B")) {
      DN_Rect a  = DN_Rect_Init2V2(DN_V2F32_Init2N(0, 0), DN_V2F32_Init2N(100, 100));
      DN_Rect b  = DN_Rect_Init2V2(DN_V2F32_Init2N(25, 25), DN_V2F32_Init2N(25, 25));
      DN_Rect ab = DN_Rect_Intersection(a, b);

      DN_V2F32 ab_max = ab.pos + ab.size;
      DN_UT_AssertF(&result,
                    ab.pos.x == 25 && ab.pos.y == 25 && ab_max.x == 50 && ab_max.y == 50,
                    "ab = { min.x = %.2f, min.y = %.2f, max.x = %.2f. max.y = %.2f }",
                    ab.pos.x,
                    ab.pos.y,
                    ab_max.x,
                    ab_max.y);
    }

    for (DN_UT_Test(&result, "A equals B")) {
      DN_Rect a  = DN_Rect_Init2V2(DN_V2F32_Init2N(0, 0), DN_V2F32_Init2N(100, 100));
      DN_Rect b  = a;
      DN_Rect ab = DN_Rect_Intersection(a, b);

      DN_V2F32 ab_max = ab.pos + ab.size;
      DN_UT_AssertF(&result,
                    ab.pos.x == 0 && ab.pos.y == 0 && ab_max.x == 100 && ab_max.y == 100,
                    "ab = { min.x = %.2f, min.y = %.2f, max.x = %.2f. max.y = %.2f }",
                    ab.pos.x,
                    ab.pos.y,
                    ab_max.x,
                    ab_max.y);
    }
  }
  return result;
}

static DN_UTCore DN_Tests_Str8()
{
  DN_UTCore result = DN_UT_Init();
  DN_UT_LogF(&result, "DN_Str8\n");
  {
    for (DN_UT_Test(&result, "Initialise with string literal w/ macro")) {
      DN_Str8 string = DN_STR8("AB");
      DN_UT_AssertF(&result, string.size == 2, "size: %zu", string.size);
      DN_UT_AssertF(&result, string.data[0] == 'A', "string[0]: %c", string.data[0]);
      DN_UT_AssertF(&result, string.data[1] == 'B', "string[1]: %c", string.data[1]);
    }

    for (DN_UT_Test(&result, "Initialise with format string")) {
      DN_OSTLSTMem tmem   = DN_OS_TLSTMem(nullptr);
      DN_Str8      string = DN_Str8_InitF(tmem.arena, "%s", "AB");
      DN_UT_AssertF(&result, string.size == 2, "size: %zu", string.size);
      DN_UT_AssertF(&result, string.data[0] == 'A', "string[0]: %c", string.data[0]);
      DN_UT_AssertF(&result, string.data[1] == 'B', "string[1]: %c", string.data[1]);
      DN_UT_AssertF(&result, string.data[2] == 0, "string[2]: %c", string.data[2]);
    }

    for (DN_UT_Test(&result, "Copy string")) {
      DN_OSTLSTMem tmem   = DN_OS_TLSTMem(nullptr);
      DN_Str8      string = DN_STR8("AB");
      DN_Str8      copy   = DN_Str8_Copy(tmem.arena, string);
      DN_UT_AssertF(&result, copy.size == 2, "size: %zu", copy.size);
      DN_UT_AssertF(&result, copy.data[0] == 'A', "copy[0]: %c", copy.data[0]);
      DN_UT_AssertF(&result, copy.data[1] == 'B', "copy[1]: %c", copy.data[1]);
      DN_UT_AssertF(&result, copy.data[2] == 0, "copy[2]: %c", copy.data[2]);
    }

    for (DN_UT_Test(&result, "Trim whitespace around string")) {
      DN_Str8 string = DN_Str8_TrimWhitespaceAround(DN_STR8(" AB "));
      DN_UT_AssertF(&result, DN_Str8_Eq(string, DN_STR8("AB")), "[string=%.*s]", DN_STR_FMT(string));
    }

    for (DN_UT_Test(&result, "Allocate string from arena")) {
      DN_OSTLSTMem tmem   = DN_OS_TLSTMem(nullptr);
      DN_Str8      string = DN_Str8_Alloc(tmem.arena, 2, DN_ZeroMem_No);
      DN_UT_AssertF(&result, string.size == 2, "size: %zu", string.size);
    }

    // NOTE: TrimPrefix/Suffix /////////////////////////////////////////////////////////////////////
    for (DN_UT_Test(&result, "Trim prefix with matching prefix")) {
      DN_Str8 input      = DN_STR8("nft/abc");
      DN_Str8 str_result = DN_Str8_TrimPrefix(input, DN_STR8("nft/"));
      DN_UT_AssertF(&result, DN_Str8_Eq(str_result, DN_STR8("abc")), "%.*s", DN_STR_FMT(str_result));
    }

    for (DN_UT_Test(&result, "Trim prefix with non matching prefix")) {
      DN_Str8 input      = DN_STR8("nft/abc");
      DN_Str8 str_result = DN_Str8_TrimPrefix(input, DN_STR8(" ft/"));
      DN_UT_AssertF(&result, DN_Str8_Eq(str_result, input), "%.*s", DN_STR_FMT(str_result));
    }

    for (DN_UT_Test(&result, "Trim suffix with matching suffix")) {
      DN_Str8 input      = DN_STR8("nft/abc");
      DN_Str8 str_result = DN_Str8_TrimSuffix(input, DN_STR8("abc"));
      DN_UT_AssertF(&result, DN_Str8_Eq(str_result, DN_STR8("nft/")), "%.*s", DN_STR_FMT(str_result));
    }

    for (DN_UT_Test(&result, "Trim suffix with non matching suffix")) {
      DN_Str8 input      = DN_STR8("nft/abc");
      DN_Str8 str_result = DN_Str8_TrimSuffix(input, DN_STR8("ab"));
      DN_UT_AssertF(&result, DN_Str8_Eq(str_result, input), "%.*s", DN_STR_FMT(str_result));
    }

    // NOTE: DN_Str8_IsAllDigits //////////////////////////////////////////////////////////////
    for (DN_UT_Test(&result, "Is all digits fails on non-digit string")) {
      DN_B32 str_result = DN_Str8_IsAll(DN_STR8("@123string"), DN_Str8IsAll_Digits);
      DN_UT_Assert(&result, str_result == false);
    }

    for (DN_UT_Test(&result, "Is all digits fails on nullptr")) {
      DN_B32 str_result = DN_Str8_IsAll(DN_Str8_Init(nullptr, 0), DN_Str8IsAll_Digits);
      DN_UT_Assert(&result, str_result == false);
    }

    for (DN_UT_Test(&result, "Is all digits fails on nullptr w/ size")) {
      DN_B32 str_result = DN_Str8_IsAll(DN_Str8_Init(nullptr, 1), DN_Str8IsAll_Digits);
      DN_UT_Assert(&result, str_result == false);
    }

    for (DN_UT_Test(&result, "Is all digits fails on string w/ 0 size")) {
      char const buf[]      = "@123string";
      DN_B32     str_result = DN_Str8_IsAll(DN_Str8_Init(buf, 0), DN_Str8IsAll_Digits);
      DN_UT_Assert(&result, !str_result);
    }

    for (DN_UT_Test(&result, "Is all digits success")) {
      DN_B32 str_result = DN_Str8_IsAll(DN_STR8("23"), DN_Str8IsAll_Digits);
      DN_UT_Assert(&result, DN_CAST(bool) str_result == true);
    }

    for (DN_UT_Test(&result, "Is all digits fails on whitespace")) {
      DN_B32 str_result = DN_Str8_IsAll(DN_STR8("23 "), DN_Str8IsAll_Digits);
      DN_UT_Assert(&result, DN_CAST(bool) str_result == false);
    }

    // NOTE: DN_Str8_BinarySplit ///////////////////////////////////////////////////////////////////
    {
      {
        char const *TEST_FMT  = "Binary split \"%.*s\" with \"%.*s\"";
        DN_Str8     delimiter = DN_STR8("/");
        DN_Str8     input     = DN_STR8("abcdef");
        for (DN_UT_Test(&result, TEST_FMT, DN_STR_FMT(input), DN_STR_FMT(delimiter))) {
          DN_Str8BinarySplitResult split = DN_Str8_BinarySplit(input, delimiter);
          DN_UT_AssertF(&result, DN_Str8_Eq(split.lhs, DN_STR8("abcdef")), "[lhs=%.*s]", DN_STR_FMT(split.lhs));
          DN_UT_AssertF(&result, DN_Str8_Eq(split.rhs, DN_STR8("")), "[rhs=%.*s]", DN_STR_FMT(split.rhs));
        }

        input = DN_STR8("abc/def");
        for (DN_UT_Test(&result, TEST_FMT, DN_STR_FMT(input), DN_STR_FMT(delimiter))) {
          DN_Str8BinarySplitResult split = DN_Str8_BinarySplit(input, delimiter);
          DN_UT_AssertF(&result, DN_Str8_Eq(split.lhs, DN_STR8("abc")), "[lhs=%.*s]", DN_STR_FMT(split.lhs));
          DN_UT_AssertF(&result, DN_Str8_Eq(split.rhs, DN_STR8("def")), "[rhs=%.*s]", DN_STR_FMT(split.rhs));
        }

        input = DN_STR8("/abcdef");
        for (DN_UT_Test(&result, TEST_FMT, DN_STR_FMT(input), DN_STR_FMT(delimiter))) {
          DN_Str8BinarySplitResult split = DN_Str8_BinarySplit(input, delimiter);
          DN_UT_AssertF(&result, DN_Str8_Eq(split.lhs, DN_STR8("")), "[lhs=%.*s]", DN_STR_FMT(split.lhs));
          DN_UT_AssertF(&result, DN_Str8_Eq(split.rhs, DN_STR8("abcdef")), "[rhs=%.*s]", DN_STR_FMT(split.rhs));
        }
      }

      {
        DN_Str8 delimiter = DN_STR8("-=-");
        DN_Str8 input     = DN_STR8("123-=-456");
        for (DN_UT_Test(&result, "Binary split \"%.*s\" with \"%.*s\"", DN_STR_FMT(input), DN_STR_FMT(delimiter))) {
          DN_Str8BinarySplitResult split = DN_Str8_BinarySplit(input, delimiter);
          DN_UT_AssertF(&result, DN_Str8_Eq(split.lhs, DN_STR8("123")), "[lhs=%.*s]", DN_STR_FMT(split.lhs));
          DN_UT_AssertF(&result, DN_Str8_Eq(split.rhs, DN_STR8("456")), "[rhs=%.*s]", DN_STR_FMT(split.rhs));
        }
      }
    }

    // NOTE: DN_Str8_ToI64 /////////////////////////////////////////////////////////////////////////
    for (DN_UT_Test(&result, "To I64: Convert null string")) {
      DN_Str8ToI64Result str_result = DN_Str8_ToI64(DN_Str8_Init(nullptr, 5), 0);
      DN_UT_Assert(&result, str_result.success);
      DN_UT_Assert(&result, str_result.value == 0);
    }

    for (DN_UT_Test(&result, "To I64: Convert empty string")) {
      DN_Str8ToI64Result str_result = DN_Str8_ToI64(DN_STR8(""), 0);
      DN_UT_Assert(&result, str_result.success);
      DN_UT_Assert(&result, str_result.value == 0);
    }

    for (DN_UT_Test(&result, "To I64: Convert \"1\"")) {
      DN_Str8ToI64Result str_result = DN_Str8_ToI64(DN_STR8("1"), 0);
      DN_UT_Assert(&result, str_result.success);
      DN_UT_Assert(&result, str_result.value == 1);
    }

    for (DN_UT_Test(&result, "To I64: Convert \"-0\"")) {
      DN_Str8ToI64Result str_result = DN_Str8_ToI64(DN_STR8("-0"), 0);
      DN_UT_Assert(&result, str_result.success);
      DN_UT_Assert(&result, str_result.value == 0);
    }

    for (DN_UT_Test(&result, "To I64: Convert \"-1\"")) {
      DN_Str8ToI64Result str_result = DN_Str8_ToI64(DN_STR8("-1"), 0);
      DN_UT_Assert(&result, str_result.success);
      DN_UT_Assert(&result, str_result.value == -1);
    }

    for (DN_UT_Test(&result, "To I64: Convert \"1.2\"")) {
      DN_Str8ToI64Result str_result = DN_Str8_ToI64(DN_STR8("1.2"), 0);
      DN_UT_Assert(&result, !str_result.success);
      DN_UT_Assert(&result, str_result.value == 1);
    }

    for (DN_UT_Test(&result, "To I64: Convert \"1,234\"")) {
      DN_Str8ToI64Result str_result = DN_Str8_ToI64(DN_STR8("1,234"), ',');
      DN_UT_Assert(&result, str_result.success);
      DN_UT_Assert(&result, str_result.value == 1234);
    }

    for (DN_UT_Test(&result, "To I64: Convert \"1,2\"")) {
      DN_Str8ToI64Result str_result = DN_Str8_ToI64(DN_STR8("1,2"), ',');
      DN_UT_Assert(&result, str_result.success);
      DN_UT_Assert(&result, str_result.value == 12);
    }

    for (DN_UT_Test(&result, "To I64: Convert \"12a3\"")) {
      DN_Str8ToI64Result str_result = DN_Str8_ToI64(DN_STR8("12a3"), 0);
      DN_UT_Assert(&result, !str_result.success);
      DN_UT_Assert(&result, str_result.value == 12);
    }

    // NOTE: DN_Str8_ToU64 /////////////////////////////////////////////////////////////////////////
    for (DN_UT_Test(&result, "To U64: Convert nullptr")) {
      DN_Str8ToU64Result str_result = DN_Str8_ToU64(DN_Str8_Init(nullptr, 5), 0);
      DN_UT_Assert(&result, str_result.success);
      DN_UT_AssertF(&result, str_result.value == 0, "result: %" PRIu64, str_result.value);
    }

    for (DN_UT_Test(&result, "To U64: Convert empty string")) {
      DN_Str8ToU64Result str_result = DN_Str8_ToU64(DN_STR8(""), 0);
      DN_UT_Assert(&result, str_result.success);
      DN_UT_AssertF(&result, str_result.value == 0, "result: %" PRIu64, str_result.value);
    }

    for (DN_UT_Test(&result, "To U64: Convert \"1\"")) {
      DN_Str8ToU64Result str_result = DN_Str8_ToU64(DN_STR8("1"), 0);
      DN_UT_Assert(&result, str_result.success);
      DN_UT_AssertF(&result, str_result.value == 1, "result: %" PRIu64, str_result.value);
    }

    for (DN_UT_Test(&result, "To U64: Convert \"-0\"")) {
      DN_Str8ToU64Result str_result = DN_Str8_ToU64(DN_STR8("-0"), 0);
      DN_UT_Assert(&result, !str_result.success);
      DN_UT_AssertF(&result, str_result.value == 0, "result: %" PRIu64, str_result.value);
    }

    for (DN_UT_Test(&result, "To U64: Convert \"-1\"")) {
      DN_Str8ToU64Result str_result = DN_Str8_ToU64(DN_STR8("-1"), 0);
      DN_UT_Assert(&result, !str_result.success);
      DN_UT_AssertF(&result, str_result.value == 0, "result: %" PRIu64, str_result.value);
    }

    for (DN_UT_Test(&result, "To U64: Convert \"1.2\"")) {
      DN_Str8ToU64Result str_result = DN_Str8_ToU64(DN_STR8("1.2"), 0);
      DN_UT_Assert(&result, !str_result.success);
      DN_UT_AssertF(&result, str_result.value == 1, "result: %" PRIu64, str_result.value);
    }

    for (DN_UT_Test(&result, "To U64: Convert \"1,234\"")) {
      DN_Str8ToU64Result str_result = DN_Str8_ToU64(DN_STR8("1,234"), ',');
      DN_UT_Assert(&result, str_result.success);
      DN_UT_AssertF(&result, str_result.value == 1234, "result: %" PRIu64, str_result.value);
    }

    for (DN_UT_Test(&result, "To U64: Convert \"1,2\"")) {
      DN_Str8ToU64Result str_result = DN_Str8_ToU64(DN_STR8("1,2"), ',');
      DN_UT_Assert(&result, str_result.success);
      DN_UT_AssertF(&result, str_result.value == 12, "result: %" PRIu64, str_result.value);
    }

    for (DN_UT_Test(&result, "To U64: Convert \"12a3\"")) {
      DN_Str8ToU64Result str_result = DN_Str8_ToU64(DN_STR8("12a3"), 0);
      DN_UT_Assert(&result, !str_result.success);
      DN_UT_AssertF(&result, str_result.value == 12, "result: %" PRIu64, str_result.value);
    }

    // NOTE: DN_Str8_Find /////////////////////////////////////////////////////////////////////
    for (DN_UT_Test(&result, "Find: String (char) is not in buffer")) {
      DN_Str8           buf        = DN_STR8("836a35becd4e74b66a0d6844d51f1a63018c7ebc44cf7e109e8e4bba57eefb55");
      DN_Str8           find       = DN_STR8("2");
      DN_Str8FindResult str_result = DN_Str8_FindStr8(buf, find, DN_Str8EqCase_Sensitive);
      DN_UT_Assert(&result, !str_result.found);
      DN_UT_Assert(&result, str_result.index == 0);
      DN_UT_Assert(&result, str_result.match.data == nullptr);
      DN_UT_Assert(&result, str_result.match.size == 0);
    }

    for (DN_UT_Test(&result, "Find: String (char) is in buffer")) {
      DN_Str8           buf        = DN_STR8("836a35becd4e74b66a0d6844d51f1a63018c7ebc44cf7e109e8e4bba57eefb55");
      DN_Str8           find       = DN_STR8("6");
      DN_Str8FindResult str_result = DN_Str8_FindStr8(buf, find, DN_Str8EqCase_Sensitive);
      DN_UT_Assert(&result, str_result.found);
      DN_UT_Assert(&result, str_result.index == 2);
      DN_UT_Assert(&result, str_result.match.data[0] == '6');
    }

    // NOTE: DN_Str8_FileNameFromPath //////////////////////////////////////////////////////////////
    for (DN_UT_Test(&result, "File name from Windows path")) {
      DN_Str8 buf        = DN_STR8("C:\\ABC\\str_result.exe");
      DN_Str8 str_result = DN_Str8_FileNameFromPath(buf);
      DN_UT_AssertF(&result, str_result == DN_STR8("str_result.exe"), "%.*s", DN_STR_FMT(str_result));
    }

    for (DN_UT_Test(&result, "File name from Linux path")) {
      DN_Str8 buf        = DN_STR8("/ABC/str_result.exe");
      DN_Str8 str_result = DN_Str8_FileNameFromPath(buf);
      DN_UT_AssertF(&result, str_result == DN_STR8("str_result.exe"), "%.*s", DN_STR_FMT(str_result));
    }

    // NOTE: DN_Str8_TrimPrefix ////////////////////////////////////////////////////////////////////
    for (DN_UT_Test(&result, "Trim prefix")) {
      DN_Str8 prefix     = DN_STR8("@123");
      DN_Str8 buf        = DN_STR8("@123string");
      DN_Str8 str_result = DN_Str8_TrimPrefix(buf, prefix, DN_Str8EqCase_Sensitive);
      DN_UT_Assert(&result, str_result == DN_STR8("string"));
    }
  }
  return result;
}

static DN_UTCore DN_Tests_TicketMutex()
{
  DN_UTCore result = DN_UT_Init();
  DN_UT_LogF(&result, "DN_TicketMutex\n");
  {
    for (DN_UT_Test(&result, "Ticket mutex start and stop")) {
      // TODO: We don't have a meaningful result but since atomics are
      // implemented with a macro this ensures that we result that they are
      // written correctly.
      DN_TicketMutex mutex = {};
      DN_TicketMutex_Begin(&mutex);
      DN_TicketMutex_End(&mutex);
      DN_UT_Assert(&result, mutex.ticket == mutex.serving);
    }

    for (DN_UT_Test(&result, "Ticket mutex start and stop w/ advanced API")) {
      DN_TicketMutex mutex    = {};
      unsigned int   ticket_a = DN_TicketMutex_MakeTicket(&mutex);
      unsigned int   ticket_b = DN_TicketMutex_MakeTicket(&mutex);
      DN_UT_Assert(&result, DN_CAST(bool) DN_TicketMutex_CanLock(&mutex, ticket_b) == false);
      DN_UT_Assert(&result, DN_CAST(bool) DN_TicketMutex_CanLock(&mutex, ticket_a) == true);

      DN_TicketMutex_BeginTicket(&mutex, ticket_a);
      DN_TicketMutex_End(&mutex);
      DN_TicketMutex_BeginTicket(&mutex, ticket_b);
      DN_TicketMutex_End(&mutex);

      DN_UT_Assert(&result, mutex.ticket == mutex.serving);
      DN_UT_Assert(&result, mutex.ticket == ticket_b + 1);
    }
  }
  return result;
}

#if defined(DN_PLATFORM_WIN32)
static DN_UTCore DN_Tests_Win()
{
  DN_UTCore result = DN_UT_Init();
  DN_UT_LogF(&result, "OS Win32\n");
  {
    DN_OSTLSTMem tmem    = DN_OS_TLSTMem(nullptr);
    DN_Str8      input8  = DN_STR8("String");
    DN_Str16     input16 = DN_Str16{(wchar_t *)(L"String"), sizeof(L"String") / sizeof(L"String"[0]) - 1};

    for (DN_UT_Test(&result, "Str8 to Str16")) {
      DN_Str16 str_result = DN_W32_Str8ToStr16(tmem.arena, input8);
      DN_UT_Assert(&result, str_result == input16);
    }

    for (DN_UT_Test(&result, "Str16 to Str8")) {
      DN_Str8 str_result = DN_W32_Str16ToStr8(tmem.arena, input16);
      DN_UT_Assert(&result, str_result == input8);
    }

    for (DN_UT_Test(&result, "Str16 to Str8: Null terminates string")) {
      int   size_required = DN_W32_Str16ToStr8Buffer(input16, nullptr, 0);
      char *string        = DN_Arena_NewArray(tmem.arena, char, size_required + 1, DN_ZeroMem_No);

      // Fill the string with error sentinels
      DN_Memset(string, 'Z', size_required + 1);

      int        size_returned = DN_W32_Str16ToStr8Buffer(input16, string, size_required + 1);
      char const EXPECTED[]    = {'S', 't', 'r', 'i', 'n', 'g', 0};

      DN_UT_AssertF(&result, size_required == size_returned, "string_size: %d, result: %d", size_required, size_returned);
      DN_UT_AssertF(&result, size_returned == DN_ArrayCountU(EXPECTED) - 1, "string_size: %d, expected: %zu", size_returned, DN_ArrayCountU(EXPECTED) - 1);
      DN_UT_Assert(&result, DN_Memcmp(EXPECTED, string, sizeof(EXPECTED)) == 0);
    }

    for (DN_UT_Test(&result, "Str16 to Str8: Arena null terminates string")) {
      DN_Str8    string8       = DN_W32_Str16ToStr8(tmem.arena, input16);
      int        size_returned = DN_W32_Str16ToStr8Buffer(input16, nullptr, 0);
      char const EXPECTED[]    = {'S', 't', 'r', 'i', 'n', 'g', 0};

      DN_UT_AssertF(&result, DN_CAST(int) string8.size == size_returned, "string_size: %d, result: %d", DN_CAST(int) string8.size, size_returned);
      DN_UT_AssertF(&result, DN_CAST(int) string8.size == DN_ArrayCountU(EXPECTED) - 1, "string_size: %d, expected: %zu", DN_CAST(int) string8.size, DN_ArrayCountU(EXPECTED) - 1);
      DN_UT_Assert(&result, DN_Memcmp(EXPECTED, string8.data, sizeof(EXPECTED)) == 0);
    }
  }
  return result;
}
#endif // DN_PLATFORM_WIN32

DN_TestsResult DN_Tests_RunSuite(DN_TestsPrint print)
{
  DN_UTCore tests[] =
      {
          DN_Tests_Base(),
          DN_Tests_Arena(),
          DN_Tests_Bin(),
          DN_Tests_BinarySearch(),
          DN_Tests_BaseContainers(),
          DN_Tests_FStr8(),
          DN_Tests_Intrinsics(),
#if defined(DN_UNIT_TESTS_WITH_KECCAK)
          DN_Tests_Keccak(),
#endif
          DN_Tests_M4(),
          DN_Tests_OS(),
          DN_Tests_Rect(),
          DN_Tests_Str8(),
          DN_Tests_TicketMutex(),
#if defined(DN_PLATFORM_WIN32)
          DN_Tests_Win(),
#endif
      };

  DN_TestsResult result = {};
  for (const DN_UTCore &test : tests) {
    result.total_tests += test.num_tests_in_group;
    result.total_good_tests += test.num_tests_ok_in_group;
  }
  result.passed = result.total_tests == result.total_good_tests;

  bool print_summary = false;
  for (DN_UTCore &test : tests) {
    bool do_print = print == DN_TestsPrint_Yes;
    if (print == DN_TestsPrint_OnFailure && test.num_tests_ok_in_group != test.num_tests_in_group)
      do_print = true;

    if (do_print) {
      print_summary = true;
      DN_UT_PrintTests(&test);
    }
    DN_UT_Deinit(&test);
  }

  if (print_summary)
    fprintf(stdout, "Summary: %d/%d tests succeeded\n", result.total_good_tests, result.total_tests);

  return result;
}
