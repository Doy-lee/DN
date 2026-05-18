#if defined(_CLANGD)
  #define DN_H_WITH_OS 1
  #include "../dn.h"
  #include "../Extra/dn_net.h"
  #include "../Extra/dn_net_curl.h"
  #include "../Standalone/dn_utest.h"

  #define DN_UNIT_TESTS_WITH_KECCAK
  #define DN_UNIT_TESTS_WITH_NET
  #define DN_UNIT_TESTS_WITH_CURL
  #define DN_SHA3_IMPLEMENTATION
#endif

#if !defined(DN_UT_H)
  #error dn_utest.h must be included before this
#endif

#if !defined(DN_UT_IMPLEMENTATION)
  #error DN_UT_IMPLEMENTATION must be defined before dn_utest.h
#endif

#include <inttypes.h>

struct DN_TSTResult
{
  bool passed;
  int  total_tests;
  int  total_good_tests;
};

enum DN_TSTPrint
{
  DN_TSTPrint_No,
  DN_TSTPrint_OnFailure,
  DN_TSTPrint_Yes,
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
  DN_U32        f_1_ECX_         = 0;
  DN_U32        f_1_EDX_         = 0;
  DN_U32        f_7_EBX_         = 0;
  DN_U32        f_7_ECX_         = 0;
  DN_U32        f_81_ECX_        = 0;
  DN_U32        f_81_EDX_        = 0;
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

static DN_UTCore DN_TST_Base()
{
  DN_UTCore result = DN_UT_Init();
  DN_UT_LogF(&result, "Base\n");
  {
    #if defined(DN_PLATFORM_WIN32) && defined(DN_COMPILER_MSVC)
    DN_RefImplCPUReport ref_cpu_report = DN_RefImplCPUReport_Init();
    for (DN_UT_Test(&result, "Query CPUID")) {
      DN_CPUReport cpu_report = DN_CPUGetReport();

      // NOTE: Sanity check our report against MSDN's example ////////////////////////////////////////
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_3DNow) == ref_cpu_report._3DNOW());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_3DNowExt) == ref_cpu_report._3DNOWEXT());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_ABM) == ref_cpu_report.ABM());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_AES) == ref_cpu_report.AES());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_AVX) == ref_cpu_report.AVX());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_AVX2) == ref_cpu_report.AVX2());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_AVX512CD) == ref_cpu_report.AVX512CD());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_AVX512ER) == ref_cpu_report.AVX512ER());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_AVX512F) == ref_cpu_report.AVX512F());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_AVX512PF) == ref_cpu_report.AVX512PF());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_CMPXCHG16B) == ref_cpu_report.CMPXCHG16B());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_F16C) == ref_cpu_report.F16C());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_FMA) == ref_cpu_report.FMA());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_MMX) == ref_cpu_report.MMX());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_MmxExt) == ref_cpu_report.MMXEXT());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_MONITOR) == ref_cpu_report.MONITOR());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_MOVBE) == ref_cpu_report.MOVBE());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_PCLMULQDQ) == ref_cpu_report.PCLMULQDQ());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_POPCNT) == ref_cpu_report.POPCNT());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_RDRAND) == ref_cpu_report.RDRAND());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_RDSEED) == ref_cpu_report.RDSEED());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_RDTSCP) == ref_cpu_report.RDTSCP());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_SHA) == ref_cpu_report.SHA());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_SSE) == ref_cpu_report.SSE());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_SSE2) == ref_cpu_report.SSE2());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_SSE3) == ref_cpu_report.SSE3());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_SSE41) == ref_cpu_report.SSE41());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_SSE42) == ref_cpu_report.SSE42());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_SSE4A) == ref_cpu_report.SSE4a());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_SSSE3) == ref_cpu_report.SSSE3());

      // NOTE: Feature flags we haven't bothered detecting yet but are in MSDN's example /////////////
      /*
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_ADX)         == DN_RefImplCPUReport::ADX());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_BMI1)        == DN_RefImplCPUReport::BMI1());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_BMI2)        == DN_RefImplCPUReport::BMI2());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_CLFSH)       == DN_RefImplCPUReport::CLFSH());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_CX8)         == DN_RefImplCPUReport::CX8());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_ERMS)        == DN_RefImplCPUReport::ERMS());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_FSGSBASE)    == DN_RefImplCPUReport::FSGSBASE());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_FXSR)        == DN_RefImplCPUReport::FXSR());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_HLE)         == DN_RefImplCPUReport::HLE());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_INVPCID)     == DN_RefImplCPUReport::INVPCID());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_LAHF)        == DN_RefImplCPUReport::LAHF());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_LZCNT)       == DN_RefImplCPUReport::LZCNT());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_MSR)         == DN_RefImplCPUReport::MSR());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_OSXSAVE)     == DN_RefImplCPUReport::OSXSAVE());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_PREFETCHWT1) == DN_RefImplCPUReport::PREFETCHWT1());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_RTM)         == DN_RefImplCPUReport::RTM());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_SEP)         == DN_RefImplCPUReport::SEP());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_SYSCALL)     == DN_RefImplCPUReport::SYSCALL());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_TBM)         == DN_RefImplCPUReport::TBM());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_XOP)         == DN_RefImplCPUReport::XOP());
      DN_UT_Assert(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_XSAVE)       == DN_RefImplCPUReport::XSAVE());
      */
    }
    #endif // defined(DN_PLATFORM_WIN32) && defined(DN_COMPILER_MSVC)

    for (DN_UT_Test(&result, "Age from U64: 1001 converts to 1s 1ms (seconds and ms)")) {
      DN_Str8x128 str8   = DN_AgeStr8FromMsU64(1001, DN_AgeUnit_Sec | DN_AgeUnit_Ms);
      DN_Str8     expect = DN_Str8Lit("1s 1ms");
      DN_UT_AssertF(&result, DN_MemEq(str8.data, str8.size, expect.data, expect.size), "str8=%.*s, expect=%.*s", DN_Str8PrintFmt(str8), DN_Str8PrintFmt(expect));
    }

    for (DN_UT_Test(&result, "Age from U64: 1001 converts to 1.001s (fractional)")) {
      DN_Str8x128 str8   = DN_AgeStr8FromMsU64(1001, DN_AgeUnit_FractionalSec);
      DN_Str8     expect = DN_Str8Lit("1.001s");
      DN_UT_AssertF(&result, DN_MemEq(str8.data, str8.size, expect.data, expect.size), "str8=%.*s, expect=%.*s", DN_Str8PrintFmt(str8), DN_Str8PrintFmt(expect));
    }

    for (DN_UT_Test(&result, "FmtAppendTruncate: String truncates with 3 dots")) {
      char               buf[8]   = {};
      DN_USize           buf_size = 0;
      DN_FmtAppendResult buf_str8 = DN_FmtAppendTruncate(buf, &buf_size, sizeof(buf), DN_Str8Lit("..."), "This string is longer than %d characters", DN_Cast(int)(sizeof(buf) - 1));
      DN_Str8            expect   = DN_Str8Lit("This..."); // 7 characters long, 1 byte reserved for null-terminator
      DN_UT_Assert(&result, buf_str8.truncated);
      DN_UT_AssertF(&result, DN_Str8Eq(buf_str8.str8, expect), "buf_str8=%.*s, expect=%.*s", DN_Str8PrintFmt(buf_str8.str8), DN_Str8PrintFmt(expect));
    }

    for (DN_UT_Test(&result, "TicketMutex: Start and stop")) {
      // TODO: We don't have a meaningful result but since atomics are
      // implemented with a macro this ensures that we result that they are
      // written correctly.
      DN_TicketMutex mutex = {};
      DN_TicketMutex_Begin(&mutex);
      DN_TicketMutex_End(&mutex);
      DN_UT_Assert(&result, mutex.ticket == mutex.serving);
    }

    for (DN_UT_Test(&result, "TicketMutex: Start and stop w/ advanced API")) {
      DN_TicketMutex mutex    = {};
      unsigned int   ticket_a = DN_TicketMutex_MakeTicket(&mutex);
      unsigned int   ticket_b = DN_TicketMutex_MakeTicket(&mutex);
      DN_UT_Assert(&result, DN_Cast(bool) DN_TicketMutex_CanLock(&mutex, ticket_b) == false);
      DN_UT_Assert(&result, DN_Cast(bool) DN_TicketMutex_CanLock(&mutex, ticket_a) == true);

      DN_TicketMutex_BeginTicket(&mutex, ticket_a);
      DN_TicketMutex_End(&mutex);
      DN_TicketMutex_BeginTicket(&mutex, ticket_b);
      DN_TicketMutex_End(&mutex);

      DN_UT_Assert(&result, mutex.ticket == mutex.serving);
      DN_UT_Assert(&result, mutex.ticket == ticket_b + 1);
    }

    // NOTE: MSVC SAL complains that we are using Interlocked functionality on
    // variables it has detected as *not* being shared across threads. This is
    // fine, we're just running some basic results, so permit it.
    //
    // Warning 28112 is a knock-on effect of this that it doesn't like us
    // reading the value of the variable that has been used in an Interlocked
    // function locally.
    DN_MSVC_WARNING_PUSH
    DN_MSVC_WARNING_DISABLE(28113) // Accessing a local variable val via an Interlocked function.
    DN_MSVC_WARNING_DISABLE(28112) // A variable (val) which is accessed via an Interlocked function must always be accessed via an Interlocked function. See line 759.
    {
      // TODO(dn): We don't have meaningful results here, but since
      // atomics/intrinsics are implemented using macros we ensure the macro was
      // written properly with these results.
      for (DN_UT_Test(&result, "AtomicAddU32")) {
        DN_U32 val = 0;
        DN_AtomicAddU32(&val, 1);
        DN_UT_AssertF(&result, val == 1, "val: %u", val);
      }

      for (DN_UT_Test(&result, "AtomicAddU64")) {
        uint64_t val = 0;
        DN_AtomicAddU64(&val, 1);
        DN_UT_AssertF(&result, val == 1, "val: %" PRIu64, val);
      }

      for (DN_UT_Test(&result, "AtomicSubU32")) {
        DN_U32 val = 1;
        DN_AtomicSubU32(&val, 1);
        DN_UT_AssertF(&result, val == 0, "val: %u", val);
      }

      for (DN_UT_Test(&result, "AtomicSubU64")) {
        uint64_t val = 1;
        DN_AtomicSubU64(&val, 1);
        DN_UT_AssertF(&result, val == 0, "val: %" PRIu64, val);
      }

      for (DN_UT_Test(&result, "AtomicSetValue32")) {
        DN_U32 a = 0;
        DN_U32 b = 111;
        DN_AtomicSetValue32(&a, b);
        DN_UT_AssertF(&result, a == b, "a: %ld, b: %ld", a, b);
      }

      for (DN_UT_Test(&result, "AtomicSetValue64")) {
        int64_t a = 0;
        int64_t b = 111;
        DN_AtomicSetValue64(DN_Cast(uint64_t *) & a, b);
        DN_UT_AssertF(&result, a == b, "a: %" PRId64 ", b: %" PRId64, a, b);
      }

      DN_UT_BeginF(&result, "CPUGetTSC: Compile check");
      DN_CPUGetTSC();
      DN_UT_End(&result);

      DN_UT_BeginF(&result, "CompilerReadBarrierAndCPUReadFence: Compile check");
      DN_CompilerReadBarrierAndCPUReadFence;
      DN_UT_End(&result);

      DN_UT_BeginF(&result, "CompilerWriteBarrierAndCPUWriteFence: Compile check");
      DN_CompilerWriteBarrierAndCPUWriteFence;
      DN_UT_End(&result);
    }
    DN_MSVC_WARNING_POP
  }
  return result;
}

static DN_UTCore DN_TST_BaseArena()
{
  DN_UTCore result = DN_UT_Init();
  DN_UT_LogF(&result, "Arena\n");
  {
    for (DN_UT_Test(&result, "Reused memory is zeroed out")) {
      uint8_t    alignment  = 1;
      DN_USize   alloc_size = DN_Kilobytes(128);
      DN_MemList mem        = DN_MemListFromVMem(0, 0, DN_MemFlags_Nil);
      DN_Arena   arena      = DN_ArenaFromMemList(&mem);
      DN_DEFER { DN_MemListDeinit(&mem); };

      // NOTE: Allocate 128 kilobytes, fill it with garbage, then reset the arena
      uintptr_t first_ptr_address = 0;
      {
        DN_Arena temp_mem = DN_ArenaTempBeginFromArena(&arena);
        void    *ptr      = DN_ArenaAlloc(&arena, alloc_size, alignment, DN_ZMem_Yes);
        first_ptr_address = DN_Cast(uintptr_t) ptr;
        DN_Memset(ptr, 'z', alloc_size);
        DN_ArenaTempEnd(&temp_mem, DN_ArenaReset_Yes);
      }

      // NOTE: Reallocate 128 kilobytes
      char *ptr = DN_Cast(char *) DN_ArenaAlloc(&arena, alloc_size, alignment, DN_ZMem_Yes);

      // NOTE: Double check we got the same pointer
      DN_UT_Assert(&result, first_ptr_address == DN_Cast(uintptr_t) ptr);

      // NOTE: Check that the bytes are set to 0
      for (DN_USize i = 0; i < alloc_size; i++)
        DN_UT_Assert(&result, ptr[i] == 0);
    }

    for (DN_UT_Test(&result, "Test arena grows naturally, 1mb + 4mb")) {
      // NOTE: Allocate 1mb, then 4mb, this should force the arena to grow
      DN_MemList mem   = DN_MemListFromVMem(DN_Megabytes(2), DN_Megabytes(2), DN_MemFlags_Nil);
      DN_Arena   arena = DN_ArenaFromMemList(&mem);
      DN_DEFER { DN_MemListDeinit(&mem); };

      char *ptr_1mb = DN_ArenaNewArray(&arena, char, DN_Megabytes(1), DN_ZMem_Yes);
      char *ptr_4mb = DN_ArenaNewArray(&arena, char, DN_Megabytes(4), DN_ZMem_Yes);
      DN_UT_Assert(&result, ptr_1mb);
      DN_UT_Assert(&result, ptr_4mb);

      DN_MemBlock const *block_4mb_begin = arena.mem->curr;
      char const        *block_4mb_end   = DN_Cast(char *) block_4mb_begin + block_4mb_begin->reserve;

      DN_MemBlock const *block_1mb_begin = block_4mb_begin->prev;
      DN_UT_AssertF(&result, block_1mb_begin, "New block should have been allocated");
      char const *block_1mb_end = DN_Cast(char *) block_1mb_begin + block_1mb_begin->reserve;

      DN_UT_AssertF(&result, block_1mb_begin != block_4mb_begin, "New block should have been allocated and linked");
      DN_UT_AssertF(&result, ptr_1mb >= DN_Cast(char *) block_1mb_begin && ptr_1mb <= block_1mb_end, "Pointer was not allocated from correct memory block");
      DN_UT_AssertF(&result, ptr_4mb >= DN_Cast(char *) block_4mb_begin && ptr_4mb <= block_4mb_end, "Pointer was not allocated from correct memory block");
    }

    for (DN_UT_Test(&result, "Test arena grows naturally, 1mb, temp memory 4mb")) {
      DN_MemList mem   = DN_MemListFromVMem(DN_Megabytes(2), DN_Megabytes(2), DN_MemFlags_Nil);
      DN_Arena   arena = DN_ArenaFromMemList(&mem);
      DN_DEFER { DN_MemListDeinit(&mem); };

      // NOTE: Allocate 1mb, then 4mb, this should force the arena to grow
      char *ptr_1mb = DN_Cast(char *) DN_ArenaAlloc(&arena, DN_Megabytes(1), 1 /*align*/, DN_ZMem_Yes);
      DN_UT_Assert(&result, ptr_1mb);

      DN_Arena temp_memory = DN_ArenaTempBeginFromArena(&arena);
      {
        char *ptr_4mb = DN_ArenaNewArray(&arena, char, DN_Megabytes(4), DN_ZMem_Yes);
        DN_UT_Assert(&result, ptr_4mb);

        DN_MemBlock const *block_4mb_begin = arena.mem->curr;
        char const        *block_4mb_end   = DN_Cast(char *) block_4mb_begin + block_4mb_begin->reserve;

        DN_MemBlock const *block_1mb_begin = block_4mb_begin->prev;
        char const        *block_1mb_end   = DN_Cast(char *) block_1mb_begin + block_1mb_begin->reserve;

        DN_UT_AssertF(&result, block_1mb_begin != block_4mb_begin, "New block should have been allocated and linked");
        DN_UT_AssertF(&result, ptr_1mb >= DN_Cast(char *) block_1mb_begin && ptr_1mb <= block_1mb_end, "Pointer was not allocated from correct memory block");
        DN_UT_AssertF(&result, ptr_4mb >= DN_Cast(char *) block_4mb_begin && ptr_4mb <= block_4mb_end, "Pointer was not allocated from correct memory block");
      }
      DN_ArenaTempEnd(&temp_memory, DN_ArenaReset_Yes);
      DN_UT_Assert(&result, arena.mem->curr->prev == nullptr);
      DN_UT_AssertF(&result,
                    arena.mem->curr->reserve >= DN_Megabytes(1),
                    "size=%" PRIu64 "MiB (%" PRIu64 "B), expect=%" PRIu64 "B",
                    (arena.mem->curr->reserve / 1024 / 1024),
                    arena.mem->curr->reserve,
                    DN_Megabytes(1));
    }
  }
  return result;
}

static DN_UTCore DN_TST_BaseBytesHex()
{
  DN_TCScratch scratch = DN_TCScratchBegin(nullptr, 0);
  DN_UTCore    test    = DN_UT_Init();
  DN_UT_LogF(&test, "Bytes <-> Hex\n");
  {
    for (DN_UT_Test(&test, "Convert 0x123")) {
      uint64_t result = DN_U64FromHexStr8Unsafe(DN_Str8Lit("0x123"));
      DN_UT_AssertF(&test, result == 0x123, "result: %" PRIu64, result);
    }

    for (DN_UT_Test(&test, "Convert 0xFFFF")) {
      uint64_t result = DN_U64FromHexStr8Unsafe(DN_Str8Lit("0xFFFF"));
      DN_UT_AssertF(&test, result == 0xFFFF, "result: %" PRIu64, result);
    }

    for (DN_UT_Test(&test, "Convert FFFF")) {
      uint64_t result = DN_U64FromHexStr8Unsafe(DN_Str8Lit("FFFF"));
      DN_UT_AssertF(&test, result == 0xFFFF, "result: %" PRIu64, result);
    }

    for (DN_UT_Test(&test, "Convert abCD")) {
      uint64_t result = DN_U64FromHexStr8Unsafe(DN_Str8Lit("abCD"));
      DN_UT_AssertF(&test, result == 0xabCD, "result: %" PRIu64, result);
    }

    for (DN_UT_Test(&test, "Convert 0xabCD")) {
      uint64_t result = DN_U64FromHexStr8Unsafe(DN_Str8Lit("0xabCD"));
      DN_UT_AssertF(&test, result == 0xabCD, "result: %" PRIu64, result);
    }

    for (DN_UT_Test(&test, "Convert 0x")) {
      uint64_t result = DN_U64FromHexStr8Unsafe(DN_Str8Lit("0x"));
      DN_UT_AssertF(&test, result == 0x0, "result: %" PRIu64, result);
    }

    for (DN_UT_Test(&test, "Convert 0X")) {
      uint64_t result = DN_U64FromHexStr8Unsafe(DN_Str8Lit("0X"));
      DN_UT_AssertF(&test, result == 0x0, "result: %" PRIu64, result);
    }

    for (DN_UT_Test(&test, "Convert 3")) {
      uint64_t result = DN_U64FromHexStr8Unsafe(DN_Str8Lit("3"));
      DN_UT_AssertF(&test, result == 3, "result: %" PRIu64, result);
    }

    for (DN_UT_Test(&test, "Convert f")) {
      DN_U64FromResult result = DN_U64FromHexStr8(DN_Str8Lit("f"));
      DN_UT_Assert(&test, result.success);
      DN_UT_Assert(&test, result.value == 0xf);
    }

    for (DN_UT_Test(&test, "Convert g")) {
      DN_U64FromResult result = DN_U64FromHexStr8(DN_Str8Lit("g"));
      DN_UT_Assert(&test, !result.success);
    }

    for (DN_UT_Test(&test, "Convert -0x3")) {
      DN_U64FromResult result = DN_U64FromHexStr8(DN_Str8Lit("-0x3"));
      DN_UT_Assert(&test, !result.success);
    }

    DN_U32 number = 0xd095f6;
    for (DN_UT_Test(&test, "Convert %x to string", number)) {
      DN_Str8 number_hex = DN_HexFromPtrBytesArena(&number, sizeof(number), &scratch.arena, DN_TrimLeadingZero_No);
      DN_UT_AssertF(&test, DN_Str8Eq(number_hex, DN_Str8Lit("f695d000")), "number_hex=%.*s", DN_Str8PrintFmt(number_hex));
    }

    number = 0xf6ed00;
    for (DN_UT_Test(&test, "Convert %x to string", number)) {
      DN_Str8 number_hex = DN_HexFromPtrBytesArena(&number, sizeof(number), &scratch.arena, DN_TrimLeadingZero_No);
      DN_UT_AssertF(&test, DN_Str8Eq(number_hex, DN_Str8Lit("00edf600")), "number_hex=%.*s", DN_Str8PrintFmt(number_hex));
    }

    DN_Str8 hex = DN_Str8Lit("0xf6ed00");
    for (DN_UT_Test(&test, "Convert %.*s to bytes", DN_Str8PrintFmt(hex))) {
      DN_Str8 bytes = DN_BytesFromHexArena(hex, &scratch.arena);
      DN_UT_AssertF(&test,
                    DN_Str8Eq(bytes, DN_Str8Lit("\xf6\xed\x00")),
                    "number_hex=%.*s",
                    DN_Str8PrintFmt(DN_HexFromPtrBytesArena(bytes.data, bytes.size, &scratch.arena, DN_TrimLeadingZero_No)));
    }

    for (DN_UT_Test(&test, "Convert empty bytes to string", number)) {
      DN_Str8 bytes  = DN_Str8Lit("");
      DN_Str8 as_hex = DN_HexFromPtrBytesArena(bytes.data, bytes.size, &scratch.arena, DN_TrimLeadingZero_No);
      DN_UT_AssertF(&test, DN_Str8Eq(as_hex, DN_Str8Lit("")), "as_hex=%.*s", DN_Str8PrintFmt(as_hex));
    }
  }
  DN_TCScratchEnd(&scratch);
  return test;
}

#if DN_H_WITH_HELPERS
static DN_UTCore DN_TST_BinarySearch()
{
  DN_UTCore result = DN_UT_Init();
  DN_UT_LogF(&result, "DN_BinarySearch\n");
  {
    for (DN_UT_Test(&result, "Search array of 1 item")) {
      DN_U32              array[] = {1};
      DN_BinarySearchResult search  = {};

      // NOTE: Match =============================================================================
      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 0U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 1U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 2U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 1);

      // NOTE: Lower bound =======================================================================
      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 0U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 1U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 2U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 1);

      // NOTE: Upper bound =======================================================================
      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 0U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 1U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 1);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 2U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 1);
    }

    for (DN_UT_Test(&result, "Search array of 2 items")) {
      DN_U32              array[] = {1};
      DN_BinarySearchResult search  = {};

      // NOTE: Match =============================================================================
      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 0U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 1U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 2U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 1);

      // NOTE: Lower bound =======================================================================
      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 0U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 1U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 2U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 1);

      // NOTE: Upper bound =======================================================================
      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 0U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 1U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 1);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 2U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 1);
    }

    for (DN_UT_Test(&result, "Search array of 3 items")) {
      DN_U32              array[] = {1, 2, 3};
      DN_BinarySearchResult search  = {};

      // NOTE: Match =============================================================================
      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 0U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 1U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 2U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 1);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 3U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 2);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 4U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 3);

      // NOTE: Lower bound =======================================================================
      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 0U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 1U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 2U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 1);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 3U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 2);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 4U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 3);

      // NOTE: Upper bound =======================================================================
      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 0U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 1U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 1);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 2U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 2);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 3U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 3);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 4U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 3);
    }

    for (DN_UT_Test(&result, "Search array of 4 items")) {
      DN_U32              array[] = {1, 2, 3, 4};
      DN_BinarySearchResult search  = {};

      // NOTE: Match =============================================================================
      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 0U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 1U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 2U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 1);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 3U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 2);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 4U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 3);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 5U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 4);

      // NOTE: Lower bound =======================================================================
      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 0U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 1U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 2U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 1);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 3U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 2);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 4U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 3);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 5U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 4);

      // NOTE: Upper bound =======================================================================
      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 0U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 1U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 1);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 2U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 2);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 3U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 3);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 4U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 4);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 5U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 4);
    }

    for (DN_UT_Test(&result, "Search array with duplicate items")) {
      DN_U32              array[] = {1, 1, 2, 2, 3};
      DN_BinarySearchResult search  = {};

      // NOTE: Match =============================================================================
      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 0U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 1U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 2U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 2);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 3U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 4);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 4U /*find*/, DN_BinarySearchType_Match);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 5);

      // NOTE: Lower bound =======================================================================
      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 0U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 1U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 2U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 2);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 3U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 4);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 4U /*find*/, DN_BinarySearchType_LowerBound);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 5);

      // NOTE: Upper bound =======================================================================
      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 0U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 0);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 1U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 2);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 2U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, search.found);
      DN_UT_Assert(&result, search.index == 4);

      search = DN_BinarySearch<DN_U32>(array, DN_ArrayCountU(array), 3U /*find*/, DN_BinarySearchType_UpperBound);
      DN_UT_Assert(&result, !search.found);
      DN_UT_Assert(&result, search.index == 5);
    }
  }
  return result;
}
#endif // DN_H_WITH_HELPERS

static DN_UTCore DN_TST_BaseDSMap()
{
  DN_UTCore result = DN_UT_Init();
  DN_UT_LogF(&result, "DN_DSMap\n");
  {
    DN_TCScratch scratch = DN_TCScratchBegin(nullptr, 0);
    {
      DN_MemList         mem      = DN_MemListFromVMem(0, 0, DN_MemFlags_Nil);
      DN_Arena           arena    = DN_ArenaFromMemList(&mem);
      DN_U32 const       MAP_SIZE = 64;
      DN_DSMap<uint64_t> map      = DN_DSMapInit<uint64_t>(&arena, MAP_SIZE, DN_DSMapFlags_Nil);
      DN_DEFER
      {
        DN_DSMapDeinit(&map, DN_ZMem_Yes);
      };

      for (DN_UT_Test(&result, "Find non-existent value")) {
        DN_DSMapResult<uint64_t> find = DN_DSMapFindKeyStr8(&map, DN_Str8Lit("Foo"));
        DN_UT_Assert(&result, !find.found);
        DN_UT_Assert(&result, map.size == MAP_SIZE);
        DN_UT_Assert(&result, map.initial_size == MAP_SIZE);
        DN_UT_Assert(&result, map.occupied == 1 /*Sentinel*/);
      }

      DN_DSMapKey key = DN_DSMapKeyCStr8(&map, "Bar");
      for (DN_UT_Test(&result, "Insert value and lookup")) {
        uint64_t  desired_value = 0xF00BAA;
        uint64_t *slot_value    = DN_DSMapSet(&map, key, desired_value).value;
        DN_UT_Assert(&result, slot_value);
        DN_UT_Assert(&result, map.size == MAP_SIZE);
        DN_UT_Assert(&result, map.initial_size == MAP_SIZE);
        DN_UT_Assert(&result, map.occupied == 2);

        uint64_t *value = DN_DSMapFind(&map, key).value;
        DN_UT_Assert(&result, value);
        DN_UT_Assert(&result, *value == desired_value);
      }

      for (DN_UT_Test(&result, "Remove key")) {
        DN_DSMapErase(&map, key);
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
        case DSMapTestType_Set: prefix = DN_Str8Lit("Set"); break;
        case DSMapTestType_MakeSlot: prefix = DN_Str8Lit("Make slot"); break;
      }

      DN_MemList         mem      = DN_MemListFromVMem(0, 0, DN_MemFlags_Nil);
      DN_Arena           arena    = DN_ArenaFromMemList(&mem);
      DN_U32 const       MAP_SIZE = 64;
      DN_DSMap<uint64_t> map      = DN_DSMapInit<uint64_t>(&arena, MAP_SIZE, DN_DSMapFlags_Nil);
      DN_DEFER
      {
        DN_DSMapDeinit(&map, DN_ZMem_Yes);
      };

      for (DN_UT_Test(&result, "%.*s: Test growing", DN_Str8PrintFmt(prefix))) {
        uint64_t map_start_size = map.size;
        uint64_t value          = 0;
        uint64_t grow_threshold = map_start_size * 3 / 4;
        for (; map.occupied != grow_threshold; value++) {
          DN_DSMapKey key = DN_DSMapKeyU64(&map, value);
          DN_UT_Assert(&result, !DN_DSMapFind<uint64_t>(&map, key).found);
          DN_DSMapResult<uint64_t> make_result = {};
          if (result_type == DSMapTestType_Set)
            make_result = DN_DSMapSet(&map, key, value);
          else
            make_result = DN_DSMapMake(&map, key);
          DN_UT_Assert(&result, !make_result.found);
          DN_UT_Assert(&result, DN_DSMapFind<uint64_t>(&map, key).value);
        }
        DN_UT_Assert(&result, map.initial_size == MAP_SIZE);
        DN_UT_Assert(&result, map.size == map_start_size);
        DN_UT_Assert(&result, map.occupied == 1 /*Sentinel*/ + value);

        { // NOTE: One more item should cause the table to grow by 2x
          DN_DSMapKey              key         = DN_DSMapKeyU64(&map, value);
          DN_DSMapResult<uint64_t> make_result = {};
          if (result_type == DSMapTestType_Set)
            make_result = DN_DSMapSet(&map, key, value);
          else
            make_result = DN_DSMapMake(&map, key);

          value++;
          DN_UT_Assert(&result, !make_result.found);
          DN_UT_Assert(&result, map.size == map_start_size * 2);
          DN_UT_Assert(&result, map.initial_size == MAP_SIZE);
          DN_UT_Assert(&result, map.occupied == 1 /*Sentinel*/ + value);
        }
      }

      for (DN_UT_Test(&result, "%.*s: Check the sentinel is present", DN_Str8PrintFmt(prefix))) {
        DN_DSMapSlot<uint64_t> NIL_SLOT = {};
        DN_DSMapSlot<uint64_t> sentinel = map.slots[DN_DS_MAP_SENTINEL_SLOT];
        DN_UT_Assert(&result, DN_Memcmp(&sentinel, &NIL_SLOT, sizeof(NIL_SLOT)) == 0);
      }

      for (DN_UT_Test(&result, "%.*s: Recheck all the hash tables values after growing", DN_Str8PrintFmt(prefix))) {
        for (uint64_t index = 1 /*Sentinel*/; index < map.occupied; index++) {
          DN_DSMapSlot<uint64_t> const *slot = map.slots + index;

          // NOTE: Validate each slot value
          uint64_t    value_result = index - 1;
          DN_DSMapKey key          = DN_DSMapKeyU64(&map, value_result);
          DN_UT_Assert(&result, DN_DSMapKeyEquals(slot->key, key));
          if (result_type == DSMapTestType_Set)
            DN_UT_Assert(&result, slot->value == value_result);
          else
            DN_UT_Assert(&result, slot->value == 0); // NOTE: Make slot does not set the key so should be 0
          DN_UT_Assert(&result, slot->key.hash == DN_DSMapHash(&map, slot->key));

          // NOTE: Check the reverse lookup is correct
          DN_DSMapResult<uint64_t> check = DN_DSMapFind(&map, slot->key);
          DN_UT_Assert(&result, slot->value == *check.value);
        }
      }

      for (DN_UT_Test(&result, "%.*s: Test shrinking", DN_Str8PrintFmt(prefix))) {
        uint64_t start_map_size     = map.size;
        uint64_t start_map_occupied = map.occupied;
        uint64_t value              = 0;
        uint64_t shrink_threshold   = map.size * 1 / 4;
        for (; map.occupied != shrink_threshold; value++) {
          DN_DSMapKey key = DN_DSMapKeyU64(&map, value);
          DN_UT_Assert(&result, DN_DSMapFind<uint64_t>(&map, key).found);
          DN_DSMapErase(&map, key);
          DN_UT_Assert(&result, !DN_DSMapFind<uint64_t>(&map, key).found);
        }
        DN_UT_Assert(&result, map.size == start_map_size);
        DN_UT_Assert(&result, map.occupied == start_map_occupied - value);

        { // NOTE: One more item should cause the table to shrink by 2x
          DN_DSMapKey key = DN_DSMapKeyU64(&map, value);
          DN_DSMapErase(&map, key);
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
          DN_DSMapKey key          = DN_DSMapKeyU64(&map, value_result);

          // NOTE: Validate each slot value
          DN_DSMapResult<uint64_t> find_result = DN_DSMapFind(&map, key);
          DN_UT_Assert(&result, find_result.value);
          DN_UT_Assert(&result, find_result.slot->key == key);
          if (result_type == DSMapTestType_Set)
            DN_UT_Assert(&result, *find_result.value == value_result);
          else
            DN_UT_Assert(&result, *find_result.value == 0); // NOTE: Make slot does not set the key so should be 0
          DN_UT_Assert(&result, find_result.slot->key.hash == DN_DSMapHash(&map, find_result.slot->key));

          // NOTE: Check the reverse lookup is correct
          DN_DSMapResult<uint64_t> check = DN_DSMapFind(&map, find_result.slot->key);
          DN_UT_Assert(&result, *find_result.value == *check.value);
        }

        for (; map.occupied != 1; value++) { // NOTE: Remove all items from the table
          DN_DSMapKey key = DN_DSMapKeyU64(&map, value);
          DN_UT_Assert(&result, DN_DSMapFind<uint64_t>(&map, key).found);
          DN_DSMapErase(&map, key);
          DN_UT_Assert(&result, !DN_DSMapFind<uint64_t>(&map, key).found);
        }
        DN_UT_Assert(&result, map.initial_size == MAP_SIZE);
        DN_UT_Assert(&result, map.size == map.initial_size);
        DN_UT_Assert(&result, map.occupied == 1 /*Sentinel*/);
      }
    }
    DN_TCScratchEnd(&scratch);
  }
  return result;
}

static DN_UTCore DN_TST_BaseIArray()
{
  DN_UTCore result = DN_UT_Init();
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
      int *item = DN_IArrayMake(&array, DN_ZMem_Yes);
      DN_UT_Assert(&result, item && array.size == 1);
    }
  }
  return result;
}

static DN_UTCore DN_TST_BaseCArray2()
{
  DN_UTCore result = DN_UT_Init();
  DN_UT_LogF(&result, "DN_CArray2\n");
  {
    for (DN_UT_Test(&result, "Positive count, middle of array, stable erase")) {
      int                 arr[]      = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      DN_USize            size       = 10;
      DN_ArrayEraseResult erase      = DN_CArrayEraseRange(arr, &size, sizeof(arr[0]), 3, 2, DN_ArrayErase_Stable);
      int                 expected[] = {0, 1, 2, 5, 6, 7, 8, 9};
      DN_UT_Assert(&result, erase.items_erased == 2);
      DN_UT_Assert(&result, erase.it_index == 3);
      DN_UT_Assert(&result, size == 8);
      DN_UT_Assert(&result, DN_Memcmp(arr, expected, size * sizeof(arr[0])) == 0);
    }

    for (DN_UT_Test(&result, "Negative count, middle of array, stable erase")) {
      int                 arr[]      = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      DN_USize            size       = 10;
      DN_ArrayEraseResult erase      = DN_CArrayEraseRange(arr, &size, sizeof(arr[0]), 5, -3, DN_ArrayErase_Stable);
      int                 expected[] = {0, 1, 2, 6, 7, 8, 9};
      DN_UT_Assert(&result, erase.items_erased == 3);
      DN_UT_Assert(&result, erase.it_index == 3);
      DN_UT_Assert(&result, size == 7);
      DN_UT_Assert(&result, DN_Memcmp(arr, expected, size * sizeof(arr[0])) == 0);
    }

    for (DN_UT_Test(&result, "count = -1, stable erase")) {
      int                 arr[]      = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      DN_USize            size       = 10;
      DN_ArrayEraseResult erase      = DN_CArrayEraseRange(arr, &size, sizeof(arr[0]), 5, -1, DN_ArrayErase_Stable);
      int                 expected[] = {0, 1, 2, 3, 4, 6, 7, 8, 9};
      DN_UT_Assert(&result, erase.items_erased == 1);
      DN_UT_Assert(&result, erase.it_index == 5);
      DN_UT_Assert(&result, size == 9);
      DN_UT_Assert(&result, DN_Memcmp(arr, expected, size * sizeof(arr[0])) == 0);
    }

    for (DN_UT_Test(&result, "Positive count, unstable erase")) {
      int                 arr[]      = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      DN_USize            size       = 10;
      DN_ArrayEraseResult erase      = DN_CArrayEraseRange(arr, &size, sizeof(arr[0]), 3, 2, DN_ArrayErase_Unstable);
      int                 expected[] = {0, 1, 2, 8, 9, 5, 6, 7};
      DN_UT_Assert(&result, erase.items_erased == 2);
      DN_UT_Assert(&result, erase.it_index == 3);
      DN_UT_Assert(&result, size == 8);
      DN_UT_Assert(&result, DN_Memcmp(arr, expected, size * sizeof(arr[0])) == 0);
    }

    for (DN_UT_Test(&result, "Negative count, unstable erase")) {
      int                 arr[]      = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      DN_USize            size       = 10;
      DN_ArrayEraseResult erase      = DN_CArrayEraseRange(arr, &size, sizeof(arr[0]), 5, -3, DN_ArrayErase_Unstable);
      int                 expected[] = {0, 1, 2, 7, 8, 9, 6};
      DN_UT_Assert(&result, erase.items_erased == 3);
      DN_UT_Assert(&result, erase.it_index == 3);
      DN_UT_Assert(&result, size == 7);
      DN_UT_Assert(&result, DN_Memcmp(arr, expected, size * sizeof(arr[0])) == 0);
    }

    for (DN_UT_Test(&result, "Edge case - begin_index at start, negative count")) {
      int                 arr[]      = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      DN_USize            size       = 10;
      DN_ArrayEraseResult erase      = DN_CArrayEraseRange(arr, &size, sizeof(arr[0]), 0, -2, DN_ArrayErase_Stable);
      int                 expected[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
      DN_UT_Assert(&result, erase.items_erased == 1);
      DN_UT_Assert(&result, erase.it_index == 0);
      DN_UT_Assert(&result, size == 9);
      DN_UT_Assert(&result, DN_Memcmp(arr, expected, size * sizeof(arr[0])) == 0);
    }

    for (DN_UT_Test(&result, "Edge case - begin_index at end, positive count")) {
      int                 arr[]      = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      DN_USize            size       = 10;
      DN_ArrayEraseResult erase      = DN_CArrayEraseRange(arr, &size, sizeof(arr[0]), 9, 2, DN_ArrayErase_Stable);
      int                 expected[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
      DN_UT_Assert(&result, erase.items_erased == 1);
      DN_UT_Assert(&result, erase.it_index == 9);
      DN_UT_Assert(&result, size == 9);
      DN_UT_Assert(&result, DN_Memcmp(arr, expected, size * sizeof(arr[0])) == 0);
    }

    for (DN_UT_Test(&result, "Invalid input - count = 0")) {
      int                 arr[]      = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      DN_USize            size       = 10;
      DN_ArrayEraseResult erase      = DN_CArrayEraseRange(arr, &size, sizeof(arr[0]), 5, 0, DN_ArrayErase_Stable);
      int                 expected[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      DN_UT_Assert(&result, erase.items_erased == 0);
      DN_UT_Assert(&result, erase.it_index == 0);
      DN_UT_Assert(&result, size == 10);
      DN_UT_Assert(&result, DN_Memcmp(arr, expected, size * sizeof(arr[0])) == 0);
    }

    for (DN_UT_Test(&result, "Invalid input - null data")) {
      DN_USize            size  = 10;
      DN_ArrayEraseResult erase = DN_CArrayEraseRange(nullptr, &size, sizeof(int), 5, 2, DN_ArrayErase_Stable);
      DN_UT_Assert(&result, erase.items_erased == 0);
      DN_UT_Assert(&result, erase.it_index == 0);
      DN_UT_Assert(&result, size == 10);
    }

    for (DN_UT_Test(&result, "Invalid input - null size")) {
      int                 arr[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      DN_ArrayEraseResult erase = DN_CArrayEraseRange(arr, NULL, sizeof(arr[0]), 5, 2, DN_ArrayErase_Stable);
      DN_UT_Assert(&result, erase.items_erased == 0);
      DN_UT_Assert(&result, erase.it_index == 0);
    }

    for (DN_UT_Test(&result, "Invalid input - empty array")) {
      int                 arr[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      DN_USize            size  = 0;
      DN_ArrayEraseResult erase = DN_CArrayEraseRange(arr, &size, sizeof(arr[0]), 5, 2, DN_ArrayErase_Stable);
      DN_UT_Assert(&result, erase.items_erased == 0);
      DN_UT_Assert(&result, erase.it_index == 0);
      DN_UT_Assert(&result, size == 0);
    }

    for (DN_UT_Test(&result, "Out-of-bounds begin_index")) {
      int                 arr[]      = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      DN_USize            size       = 10;
      DN_ArrayEraseResult erase      = DN_CArrayEraseRange(arr, &size, sizeof(arr[0]), 15, 2, DN_ArrayErase_Stable);
      int                 expected[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      DN_UT_Assert(&result, erase.items_erased == 0);
      DN_UT_Assert(&result, erase.it_index == 10);
      DN_UT_Assert(&result, size == 10);
      DN_UT_Assert(&result, DN_Memcmp(arr, expected, size * sizeof(arr[0])) == 0);
    }
  }
  return result;
}

static DN_UTCore DN_TST_BaseVArray()
{
  DN_UTCore result = DN_UT_Init();
  DN_UT_LogF(&result, "DN_VArray\n");
  {
    {
      DN_VArray<DN_U32> array = DN_OS_VArrayInitByteSize<DN_U32>(DN_Kilobytes(64));
      DN_DEFER
      {
        DN_OS_VArrayDeinit(&array);
      };

      for (DN_UT_Test(&result, "Test adding an array of items to the array")) {
        DN_U32 array_literal[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
        DN_OS_VArrayAddArray<DN_U32>(&array, array_literal, DN_ArrayCountU(array_literal));
        DN_UT_Assert(&result, array.size == DN_ArrayCountU(array_literal));
        DN_UT_Assert(&result, DN_Memcmp(array.data, array_literal, DN_ArrayCountU(array_literal) * sizeof(array_literal[0])) == 0);
      }

      for (DN_UT_Test(&result, "Test stable erase, 1 item, the '2' value from the array")) {
        DN_OS_VArrayEraseRange(&array, 2 /*begin_index*/, 1 /*count*/, DN_ArrayErase_Stable);
        DN_U32 array_literal[] = {0, 1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
        DN_UT_Assert(&result, array.size == DN_ArrayCountU(array_literal));
        DN_UT_Assert(&result, DN_Memcmp(array.data, array_literal, DN_ArrayCountU(array_literal) * sizeof(array_literal[0])) == 0);
      }

      for (DN_UT_Test(&result, "Test unstable erase, 1 item, the '1' value from the array")) {
        DN_OS_VArrayEraseRange(&array, 1 /*begin_index*/, 1 /*count*/, DN_ArrayErase_Unstable);
        DN_U32 array_literal[] = {0, 15, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
        DN_UT_Assert(&result, array.size == DN_ArrayCountU(array_literal));
        DN_UT_Assert(&result, DN_Memcmp(array.data, array_literal, DN_ArrayCountU(array_literal) * sizeof(array_literal[0])) == 0);
      }

      DN_ArrayErase erase_enums[] = {DN_ArrayErase_Stable, DN_ArrayErase_Unstable};
      for (DN_UT_Test(&result, "Test un/stable erase, OOB")) {
        for (DN_ArrayErase erase : erase_enums) {
          DN_U32 array_literal[] = {0, 15, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
          DN_OS_VArrayEraseRange(&array, DN_ArrayCountU(array_literal) /*begin_index*/, DN_ArrayCountU(array_literal) + 100 /*count*/, erase);
          DN_UT_Assert(&result, array.size == DN_ArrayCountU(array_literal));
          DN_UT_Assert(&result, DN_Memcmp(array.data, array_literal, DN_ArrayCountU(array_literal) * sizeof(array_literal[0])) == 0);
        }
      }

      for (DN_UT_Test(&result, "Test flipped begin/end index stable erase, 2 items, the '15, 3' value from the array")) {
        DN_OS_VArrayEraseRange(&array, 2 /*begin_index*/, -2 /*count*/, DN_ArrayErase_Stable);
        DN_U32 array_literal[] = {0, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
        DN_UT_Assert(&result, array.size == DN_ArrayCountU(array_literal));
        DN_UT_Assert(&result, DN_Memcmp(array.data, array_literal, DN_ArrayCountU(array_literal) * sizeof(array_literal[0])) == 0);
      }

      for (DN_UT_Test(&result, "Test flipped begin/end index unstable erase, 2 items, the '4, 5' value from the array")) {
        DN_OS_VArrayEraseRange(&array, 2 /*begin_index*/, -2 /*count*/, DN_ArrayErase_Unstable);
        DN_U32 array_literal[] = {0, 13, 14, 6, 7, 8, 9, 10, 11, 12};
        DN_UT_Assert(&result, array.size == DN_ArrayCountU(array_literal));
        DN_UT_Assert(&result, DN_Memcmp(array.data, array_literal, DN_ArrayCountU(array_literal) * sizeof(array_literal[0])) == 0);
      }

      for (DN_UT_Test(&result, "Test stable erase range, 2+1 (oob) item, the '13, 14, +1 OOB' value from the array")) {
        DN_OS_VArrayEraseRange(&array, 8 /*begin_index*/, 3 /*count*/, DN_ArrayErase_Stable);
        DN_U32 array_literal[] = {0, 13, 14, 6, 7, 8, 9, 10};
        DN_UT_Assert(&result, array.size == DN_ArrayCountU(array_literal));
        DN_UT_Assert(&result, DN_Memcmp(array.data, array_literal, DN_ArrayCountU(array_literal) * sizeof(array_literal[0])) == 0);
      }

      for (DN_UT_Test(&result, "Test unstable erase range, 3+1 (oob) item, the '11, 12, +1 OOB' value from the array")) {
        DN_OS_VArrayEraseRange(&array, 6 /*begin_index*/, 3 /*count*/, DN_ArrayErase_Unstable);
        DN_U32 array_literal[] = {0, 13, 14, 6, 7, 8};
        DN_UT_Assert(&result, array.size == DN_ArrayCountU(array_literal));
        DN_UT_Assert(&result, DN_Memcmp(array.data, array_literal, DN_ArrayCountU(array_literal) * sizeof(array_literal[0])) == 0);
      }

      for (DN_UT_Test(&result, "Test stable erase -overflow OOB, erasing the '0, 13' value from the array")) {
        DN_OS_VArrayEraseRange(&array, 1 /*begin_index*/, -DN_ISIZE_MAX /*count*/, DN_ArrayErase_Stable);
        DN_U32 array_literal[] = {14, 6, 7, 8};
        DN_UT_Assert(&result, array.size == DN_ArrayCountU(array_literal));
        DN_UT_Assert(&result, DN_Memcmp(array.data, array_literal, DN_ArrayCountU(array_literal) * sizeof(array_literal[0])) == 0);
      }

      for (DN_UT_Test(&result, "Test unstable erase +overflow OOB, erasing the '7, 8' value from the array")) {
        DN_OS_VArrayEraseRange(&array, 2 /*begin_index*/, DN_ISIZE_MAX /*count*/, DN_ArrayErase_Unstable);
        DN_U32 array_literal[] = {14, 6};
        DN_UT_Assert(&result, array.size == DN_ArrayCountU(array_literal));
        DN_UT_Assert(&result, DN_Memcmp(array.data, array_literal, DN_ArrayCountU(array_literal) * sizeof(array_literal[0])) == 0);
      }

      for (DN_UT_Test(&result, "Test adding an array of items after erase")) {
        DN_U32 array_literal[] = {0, 1, 2, 3};
        DN_OS_VArrayAddArray<DN_U32>(&array, array_literal, DN_ArrayCountU(array_literal));

        DN_U32 expected_literal[] = {14, 6, 0, 1, 2, 3};
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

      DN_VArray<UnalignedObject> array = DN_OS_VArrayInitByteSize<UnalignedObject>(DN_Kilobytes(64));
      DN_DEFER
      {
        DN_OS_VArrayDeinit(&array);
      };

      // NOTE: Verify that the items returned from the data array are
      // contiguous in memory.
      UnalignedObject *make_item_a = DN_OS_VArrayMakeArray(&array, 1, DN_ZMem_Yes);
      UnalignedObject *make_item_b = DN_OS_VArrayMakeArray(&array, 1, DN_ZMem_Yes);
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

#if defined(DN_UNIT_TESTS_WITH_KECCAK)
DN_GCC_WARNING_PUSH
DN_GCC_WARNING_DISABLE(-Wunused-parameter)
DN_GCC_WARNING_DISABLE(-Wsign-compare)

DN_MSVC_WARNING_PUSH
DN_MSVC_WARNING_DISABLE(4244)
DN_MSVC_WARNING_DISABLE(4100)
DN_MSVC_WARNING_DISABLE(6385)
// NOTE: Keccak Reference Implementation
// A very compact Keccak implementation taken from the reference implementation
// repository
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

  #define DN_SHA3_IMPLEMENTATION
  #include "../Standalone/dn_sha3.h"

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

enum DN_TST__HashType
{

  #define DN_UT_HASH_X_ENTRY(enum_val, string) Hash_##enum_val,
  DN_UT_HASH_X_MACRO
  #undef DN_UT_HASH_X_ENTRY
};

DN_Str8 const DN_UT_HASH_STRING_[] =
    {
  #define DN_UT_HASH_X_ENTRY(enum_val, string) DN_Str8Lit(string),
        DN_UT_HASH_X_MACRO
  #undef DN_UT_HASH_X_ENTRY
};

void DN_TST_KeccakDispatch_(DN_UTCore *test, int hash_type, DN_Str8 input)
{
  DN_TCScratch scratch   = DN_TCScratchBegin(nullptr, 0);
  DN_Str8      input_hex = DN_HexFromPtrBytesArena(input.data, input.size, &scratch.arena, DN_TrimLeadingZero_No);

  switch (hash_type) {
    case Hash_SHA3_224: {
      DN_SHA3U8x28 hash = DN_SHA3Hash224b(input.data, input.size);
      DN_SHA3U8x28 expect;
      DN_RefImpl_FIPS202_SHA3_224_(DN_Cast(uint8_t *) input.data, input.size, (uint8_t *)expect.data);

      DN_Str8 hash_hex   = DN_HexFromPtrBytesArena(hash.data, DN_ArrayCountU(hash.data), &scratch.arena, DN_TrimLeadingZero_No);
      DN_Str8 expect_hex = DN_HexFromPtrBytesArena(expect.data, DN_ArrayCountU(expect.data), &scratch.arena, DN_TrimLeadingZero_No);
      DN_UT_AssertF(test,
                    DN_MemEq(hash.data, sizeof(hash.data), expect.data, sizeof(expect.data)),
                    "\ninput:  %.*s"
                    "\nhash:   %.*s"
                    "\nexpect: %.*s",
                    DN_Str8PrintFmt(input_hex),
                    DN_Str8PrintFmt(hash_hex),
                    DN_Str8PrintFmt(expect_hex));
    } break;

    case Hash_SHA3_256: {
      DN_SHA3U8x32 hash = DN_SHA3Hash256b(input.data, input.size);
      DN_SHA3U8x32 expect;
      DN_RefImpl_FIPS202_SHA3_256_(DN_Cast(uint8_t *) input.data, input.size, (uint8_t *)expect.data);

      DN_Str8 hash_hex   = DN_HexFromPtrBytesArena(hash.data, DN_ArrayCountU(hash.data), &scratch.arena, DN_TrimLeadingZero_No);
      DN_Str8 expect_hex = DN_HexFromPtrBytesArena(expect.data, DN_ArrayCountU(expect.data), &scratch.arena, DN_TrimLeadingZero_No);
      DN_UT_AssertF(test,
                    DN_MemEq(hash.data, sizeof(hash.data), expect.data, sizeof(expect.data)),
                    "\ninput:  %.*s"
                    "\nhash:   %.*s"
                    "\nexpect: %.*s",
                    DN_Str8PrintFmt(input_hex),
                    DN_Str8PrintFmt(hash_hex),
                    DN_Str8PrintFmt(expect_hex));
    } break;

    case Hash_SHA3_384: {
      DN_SHA3U8x48 hash = DN_SHA3Hash384b(input.data, input.size);
      DN_SHA3U8x48 expect;
      DN_RefImpl_FIPS202_SHA3_384_(DN_Cast(uint8_t *) input.data, input.size, (uint8_t *)expect.data);

      DN_Str8 hash_hex   = DN_HexFromPtrBytesArena(hash.data, DN_ArrayCountU(hash.data), &scratch.arena, DN_TrimLeadingZero_No);
      DN_Str8 expect_hex = DN_HexFromPtrBytesArena(expect.data, DN_ArrayCountU(expect.data), &scratch.arena, DN_TrimLeadingZero_No);
      DN_UT_AssertF(test,
                    DN_MemEq(hash.data, sizeof(hash.data), expect.data, sizeof(expect.data)),
                    "\ninput:  %.*s"
                    "\nhash:   %.*s"
                    "\nexpect: %.*s",
                    DN_Str8PrintFmt(input_hex),
                    DN_Str8PrintFmt(hash_hex),
                    DN_Str8PrintFmt(expect_hex));
    } break;

    case Hash_SHA3_512: {
      DN_SHA3U8x64 hash = DN_SHA3Hash512b(input.data, input.size);
      DN_SHA3U8x64 expect;
      DN_RefImpl_FIPS202_SHA3_512_(DN_Cast(uint8_t *) input.data, input.size, (uint8_t *)expect.data);

      DN_Str8 hash_hex   = DN_HexFromPtrBytesArena(hash.data, DN_ArrayCountU(hash.data), &scratch.arena, DN_TrimLeadingZero_No);
      DN_Str8 expect_hex = DN_HexFromPtrBytesArena(expect.data, DN_ArrayCountU(expect.data), &scratch.arena, DN_TrimLeadingZero_No);
      DN_UT_AssertF(test,
                    DN_MemEq(hash.data, sizeof(hash.data), expect.data, sizeof(expect.data)),
                    "\ninput:  %.*s"
                    "\nhash:   %.*s"
                    "\nexpect: %.*s",
                    DN_Str8PrintFmt(input_hex),
                    DN_Str8PrintFmt(hash_hex),
                    DN_Str8PrintFmt(expect_hex));
    } break;

    case Hash_Keccak_224: {
      DN_SHA3U8x28 hash = DN_KeccakHash224b(input.data, input.size);
      DN_SHA3U8x28 expect;
      DN_RefImpl_Keccak_(1152, 448, DN_Cast(uint8_t *) input.data, input.size, 0x01, (uint8_t *)expect.data, sizeof(expect));

      DN_Str8 hash_hex   = DN_HexFromPtrBytesArena(hash.data, DN_ArrayCountU(hash.data), &scratch.arena, DN_TrimLeadingZero_No);
      DN_Str8 expect_hex = DN_HexFromPtrBytesArena(expect.data, DN_ArrayCountU(expect.data), &scratch.arena, DN_TrimLeadingZero_No);
      DN_UT_AssertF(test,
                    DN_MemEq(hash.data, sizeof(hash.data), expect.data, sizeof(expect.data)),
                    "\ninput:  %.*s"
                    "\nhash:   %.*s"
                    "\nexpect: %.*s",
                    DN_Str8PrintFmt(input_hex),
                    DN_Str8PrintFmt(hash_hex),
                    DN_Str8PrintFmt(expect_hex));
    } break;

    case Hash_Keccak_256: {
      DN_SHA3U8x32 hash = DN_KeccakHash256b(input.data, input.size);
      DN_SHA3U8x32 expect;
      DN_RefImpl_Keccak_(1088, 512, DN_Cast(uint8_t *) input.data, input.size, 0x01, (uint8_t *)expect.data, sizeof(expect));

      DN_Str8 hash_hex   = DN_HexFromPtrBytesArena(hash.data, DN_ArrayCountU(hash.data), &scratch.arena, DN_TrimLeadingZero_No);
      DN_Str8 expect_hex = DN_HexFromPtrBytesArena(expect.data, DN_ArrayCountU(expect.data), &scratch.arena, DN_TrimLeadingZero_No);
      DN_UT_AssertF(test,
                    DN_MemEq(hash.data, sizeof(hash.data), expect.data, sizeof(expect.data)),
                    "\ninput:  %.*s"
                    "\nhash:   %.*s"
                    "\nexpect: %.*s",
                    DN_Str8PrintFmt(input_hex),
                    DN_Str8PrintFmt(hash_hex),
                    DN_Str8PrintFmt(expect_hex));
    } break;

    case Hash_Keccak_384: {
      DN_SHA3U8x48 hash = DN_KeccakHash384b(input.data, input.size);
      DN_SHA3U8x48 expect;
      DN_RefImpl_Keccak_(832, 768, DN_Cast(uint8_t *) input.data, input.size, 0x01, (uint8_t *)expect.data, sizeof(expect));

      DN_Str8 hash_hex   = DN_HexFromPtrBytesArena(hash.data, DN_ArrayCountU(hash.data), &scratch.arena, DN_TrimLeadingZero_No);
      DN_Str8 expect_hex = DN_HexFromPtrBytesArena(expect.data, DN_ArrayCountU(expect.data), &scratch.arena, DN_TrimLeadingZero_No);
      DN_UT_AssertF(test,
                    DN_MemEq(hash.data, sizeof(hash.data), expect.data, sizeof(expect.data)),
                    "\ninput:  %.*s"
                    "\nhash:   %.*s"
                    "\nexpect: %.*s",
                    DN_Str8PrintFmt(input_hex),
                    DN_Str8PrintFmt(hash_hex),
                    DN_Str8PrintFmt(expect_hex));
    } break;

    case Hash_Keccak_512: {
      DN_SHA3U8x64 hash = DN_KeccakHash512b(input.data, input.size);
      DN_SHA3U8x64 expect;
      DN_RefImpl_Keccak_(576, 1024, DN_Cast(uint8_t *) input.data, input.size, 0x01, (uint8_t *)expect.data, sizeof(expect));

      DN_Str8 hash_hex   = DN_HexFromPtrBytesArena(hash.data, DN_ArrayCountU(hash.data), &scratch.arena, DN_TrimLeadingZero_No);
      DN_Str8 expect_hex = DN_HexFromPtrBytesArena(expect.data, DN_ArrayCountU(expect.data), &scratch.arena, DN_TrimLeadingZero_No);
      DN_UT_AssertF(test,
                    DN_MemEq(hash.data, sizeof(hash.data), expect.data, sizeof(expect.data)),
                    "\ninput:  %.*s"
                    "\nhash:   %.*s"
                    "\nexpect: %.*s",
                    DN_Str8PrintFmt(input_hex),
                    DN_Str8PrintFmt(hash_hex),
                    DN_Str8PrintFmt(expect_hex));
    } break;
  }
  DN_TCScratchEnd(&scratch);
}
#endif // defined(DN_UNIT_TESTS_WITH_KECCAK)

DN_UTCore DN_TST_Keccak()
{
  DN_UTCore result = DN_UT_Init();
  #if defined(DN_UNIT_TESTS_WITH_KECCAK)
  DN_Str8 const INPUTS[] = {
      DN_Str8Lit("abc"),
      DN_Str8Lit(""),
      DN_Str8Lit("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"),
      DN_Str8Lit("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmno"
                 "pqrstnopqrstu"),
  };

  DN_UT_LogF(&result, "DN_KC\n");
  {
    for (int hash_type = 0; hash_type < Hash_Count; hash_type++) {
      DN_PCG32 rng = DN_PCG32Init(0xd48e'be21'2af8'733d);
      for (DN_Str8 input : INPUTS) {
        DN_UT_BeginF(&result, "%.*s - Input: %.*s", DN_Str8PrintFmt(DN_UT_HASH_STRING_[hash_type]), DN_Cast(int) DN_Min(input.size, 54), input.data);
        DN_TST_KeccakDispatch_(&result, hash_type, input);
        DN_UT_End(&result);
      }

      DN_UT_BeginF(&result, "%.*s - Deterministic random inputs", DN_Str8PrintFmt(DN_UT_HASH_STRING_[hash_type]));
      for (DN_USize index = 0; index < 128; index++) {
        char   src[4096] = {};
        DN_U32 src_size  = DN_PCG32Range(&rng, 0, sizeof(src));

        for (DN_USize src_index = 0; src_index < src_size; src_index++)
          src[src_index] = DN_Cast(char) DN_PCG32Range(&rng, 0, 255);

        DN_Str8 input = DN_Str8FromPtr(src, src_size);
        DN_TST_KeccakDispatch_(&result, hash_type, input);
      }
      DN_UT_End(&result);
    }
  }
  #endif
  return result;
}

static DN_UTCore DN_TST_M4()
{
  DN_UTCore result = DN_UT_Init();
  DN_UT_LogF(&result, "DN_M4\n");
  {
    for (DN_UT_Test(&result, "Simple translate and scale matrix")) {
      DN_M4 translate  = DN_M4TranslateF(1, 2, 3);
      DN_M4 scale      = DN_M4ScaleF(2, 2, 2);
      DN_M4 mul_result = DN_M4Mul(translate, scale);

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
                    DN_M4ColumnMajorString(mul_result).data,
                    DN_M4ColumnMajorString(EXPECT).data);
    }
  }
  return result;
}

static DN_UTCore DN_TST_OS()
{
  DN_UTCore result = DN_UT_Init();

#if defined(DN_OS_INC_CPP) || 1
  DN_UT_LogF(&result, "DN_OS\n");
  {
    for (DN_UT_Test(&result, "Generate secure RNG 32 bytes")) {
      char const ZERO[32]  = {};
      char       buf[32]   = {};
      DN_OS_GenBytesSecure(buf, DN_ArrayCountU(buf));
      DN_UT_Assert(&result, DN_Memcmp(buf, ZERO, DN_ArrayCountU(buf)) != 0);
    }

    for (DN_UT_Test(&result, "Query executable directory")) {
      DN_TCScratch scratch      = DN_TCScratchBegin(nullptr, 0);
      DN_Str8      os_result = DN_OS_EXEDir(&scratch.arena);
      DN_UT_Assert(&result, os_result.size);
      DN_UT_AssertF(&result, DN_OS_PathIsDir(os_result), "result(%zu): %.*s", os_result.size, DN_Str8PrintFmt(os_result));
      DN_TCScratchEnd(&scratch);
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
      DN_UT_AssertF(&result, DN_OS_PathMakeDir(DN_Str8Lit("abcd/efgh")), "Failed to make directory");
      DN_UT_AssertF(&result, DN_OS_PathIsDir(DN_Str8Lit("abcd")), "Directory was not made");
      DN_UT_AssertF(&result, DN_OS_PathIsDir(DN_Str8Lit("abcd/efgh")), "Subdirectory was not made");
      DN_UT_AssertF(&result, DN_OS_PathIsFile(DN_Str8Lit("abcd")) == false, "This function should only return true for files");
      DN_UT_AssertF(&result, DN_OS_PathIsFile(DN_Str8Lit("abcd/efgh")) == false, "This function should only return true for files");
      DN_UT_AssertF(&result, DN_OS_PathDelete(DN_Str8Lit("abcd/efgh")), "Failed to delete directory");
      DN_UT_AssertF(&result, DN_OS_PathDelete(DN_Str8Lit("abcd")), "Failed to cleanup directory");
    }

    for (DN_UT_Test(&result, "File write, read, copy, move and delete")) {
      // NOTE: Write step
      DN_Str8 const SRC_FILE     = DN_Str8Lit("dn_result_file");
      DN_B32        write_result = DN_OS_FileWriteAll(SRC_FILE, DN_Str8Lit("1234"), nullptr);
      DN_UT_Assert(&result, write_result);
      DN_UT_Assert(&result, DN_OS_PathIsFile(SRC_FILE));

      // NOTE: Read step
      DN_TCScratch scratch      = DN_TCScratchBegin(nullptr, 0);
      DN_Str8      read_file = DN_OS_FileReadAllArena(&scratch.arena, SRC_FILE, nullptr);
      DN_UT_AssertF(&result, read_file.size, "Failed to load file");
      DN_UT_AssertF(&result, read_file.size == 4, "File read wrong amount of bytes (%zu)", read_file.size);
      DN_UT_AssertF(&result, DN_Str8Eq(read_file, DN_Str8Lit("1234")), "Read %zu bytes instead of the expected 4: '%.*s'", read_file.size, DN_Str8PrintFmt(read_file));

      // NOTE: Copy step
      DN_Str8 const COPY_FILE   = DN_Str8Lit("dn_result_file_copy");
      DN_B32        copy_result = DN_OS_FileCopy(SRC_FILE, COPY_FILE, true /*overwrite*/, nullptr);
      DN_UT_Assert(&result, copy_result);
      DN_UT_Assert(&result, DN_OS_PathIsFile(COPY_FILE));

      // NOTE: Move step
      DN_Str8 const MOVE_FILE   = DN_Str8Lit("dn_result_file_move");
      DN_B32        move_result = DN_OS_FileMove(COPY_FILE, MOVE_FILE, true /*overwrite*/, nullptr);
      DN_UT_Assert(&result, move_result);
      DN_UT_Assert(&result, DN_OS_PathIsFile(MOVE_FILE));
      DN_UT_AssertF(&result, DN_OS_PathIsFile(COPY_FILE) == false, "Moving a file should remove the original");

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
      DN_TCScratchEnd(&scratch);
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
      DN_UT_AssertF(&result, elapsed_ms >= 80 && elapsed_ms <= 120, "Expected to sleep for ~100ms, slept %f ms", elapsed_ms);
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

static DN_UTCore DN_TST_Rect()
{
  DN_UTCore result = DN_UT_Init();
  DN_UT_LogF(&result, "DN_Rect\n");
  {
    for (DN_UT_Test(&result, "No intersection")) {
      DN_Rect a  = DN_RectFrom2V2(DN_V2F32From1N(0), DN_V2F32From2N(100, 100));
      DN_Rect b  = DN_RectFrom2V2(DN_V2F32From2N(200, 0), DN_V2F32From2N(200, 200));
      DN_Rect ab = DN_RectIntersection(a, b);

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
      DN_Rect a  = DN_RectFrom2V2(DN_V2F32From2N(50, 50), DN_V2F32From2N(100, 100));
      DN_Rect b  = DN_RectFrom2V2(DN_V2F32From2N(0, 0), DN_V2F32From2N(100, 100));
      DN_Rect ab = DN_RectIntersection(a, b);

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
      DN_Rect a  = DN_RectFrom2V2(DN_V2F32From2N(0, 0), DN_V2F32From2N(100, 100));
      DN_Rect b  = DN_RectFrom2V2(DN_V2F32From2N(50, 50), DN_V2F32From2N(100, 100));
      DN_Rect ab = DN_RectIntersection(a, b);

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
      DN_Rect a  = DN_RectFrom2V2(DN_V2F32From2N(-50, -50), DN_V2F32From2N(100, 100));
      DN_Rect b  = DN_RectFrom2V2(DN_V2F32From2N(0, 0), DN_V2F32From2N(100, 100));
      DN_Rect ab = DN_RectIntersection(a, b);

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
      DN_Rect a  = DN_RectFrom2V2(DN_V2F32From2N(0, 0), DN_V2F32From2N(100, 100));
      DN_Rect b  = DN_RectFrom2V2(DN_V2F32From2N(-50, -50), DN_V2F32From2N(100, 100));
      DN_Rect ab = DN_RectIntersection(a, b);

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
      DN_Rect a  = DN_RectFrom2V2(DN_V2F32From2N(25, 25), DN_V2F32From2N(25, 25));
      DN_Rect b  = DN_RectFrom2V2(DN_V2F32From2N(0, 0), DN_V2F32From2N(100, 100));
      DN_Rect ab = DN_RectIntersection(a, b);

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
      DN_Rect a  = DN_RectFrom2V2(DN_V2F32From2N(0, 0), DN_V2F32From2N(100, 100));
      DN_Rect b  = DN_RectFrom2V2(DN_V2F32From2N(25, 25), DN_V2F32From2N(25, 25));
      DN_Rect ab = DN_RectIntersection(a, b);

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
      DN_Rect a  = DN_RectFrom2V2(DN_V2F32From2N(0, 0), DN_V2F32From2N(100, 100));
      DN_Rect b  = a;
      DN_Rect ab = DN_RectIntersection(a, b);

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

static DN_UTCore DN_TST_BaseStrings()
{
  DN_UTCore result = DN_UT_Init();
  DN_UT_LogF(&result, "Strings\n");
  {
    for (DN_UT_Test(&result, "Str8 literal")) {
      DN_Str8 string = DN_Str8Lit("AB");
      DN_UT_AssertF(&result, string.size == 2, "size: %zu", string.size);
      DN_UT_AssertF(&result, string.data[0] == 'A', "string[0]: %c", string.data[0]);
      DN_UT_AssertF(&result, string.data[1] == 'B', "string[1]: %c", string.data[1]);
    }

    for (DN_UT_Test(&result, "C-string length")) {
      DN_USize size = DN_CStr8Size("hello");
      DN_UT_AssertF(&result, size == 5, "size=%zu", size);
    }

    char arena_base[512];
    for (DN_UT_Test(&result, "Str8 format from arena")) {
      DN_MemList mem    = DN_MemListFromBuffer(arena_base, sizeof(arena_base), DN_MemFlags_Nil);
      DN_Arena   arena  = DN_ArenaFromMemList(&mem);
      DN_Str8  str8   = DN_Str8FromFmtArena(&arena, "Foo Bar %d", 5);
      DN_Str8  expect = DN_Str8Lit("Foo Bar 5");
      DN_UT_AssertF(&result, DN_Str8Eq(str8, expect), "str8=%.*s", DN_Str8PrintFmt(str8), DN_Str8PrintFmt(expect));
    }

    for (DN_UT_Test(&result, "Str8 format from pool")) {
      DN_MemList mem    = DN_MemListFromBuffer(arena_base, sizeof(arena_base), DN_MemFlags_Nil);
      DN_Arena   arena  = DN_ArenaFromMemList(&mem);
      DN_Pool    pool   = DN_PoolFromArena(&arena, 0);
      DN_Str8    str8   = DN_Str8FromFmtPool(&pool, "Foo Bar %d", 5);
      DN_Str8    expect = DN_Str8Lit("Foo Bar 5");
      DN_UT_AssertF(&result, DN_Str8Eq(str8, expect), "str8=%.*s", DN_Str8PrintFmt(str8), DN_Str8PrintFmt(expect));
    }

    for (DN_UT_Test(&result, "Str8x32 from U64")) {
      DN_Str8x32 str8   = DN_Str8x32FromU64(123456, ' ');
      DN_Str8    expect = DN_Str8Lit("123 456");
      DN_UT_AssertF(&result, DN_Str8Eq(DN_Str8FromStruct(&str8), expect), "buf_str8=%.*s, expect=%.*s", DN_Str8PrintFmt(str8), DN_Str8PrintFmt(expect));
    }

    for (DN_UT_Test(&result, "Initialise with format string")) {
      DN_TCScratch scratch   = DN_TCScratchBegin(nullptr, 0);
      DN_Str8      string = DN_Str8FromFmtArena(&scratch.arena, "%s", "AB");
      DN_UT_AssertF(&result, string.size == 2, "size: %zu", string.size);
      DN_UT_AssertF(&result, string.data[0] == 'A', "string[0]: %c", string.data[0]);
      DN_UT_AssertF(&result, string.data[1] == 'B', "string[1]: %c", string.data[1]);
      DN_UT_AssertF(&result, string.data[2] == 0, "string[2]: %c", string.data[2]);
      DN_TCScratchEnd(&scratch);
    }

    for (DN_UT_Test(&result, "Copy string")) {
      DN_TCScratch scratch   = DN_TCScratchBegin(nullptr, 0);
      DN_Str8      string = DN_Str8Lit("AB");
      DN_Str8      copy   = DN_Str8FromStr8Arena(string, &scratch.arena);
      DN_UT_AssertF(&result, copy.size == 2, "size: %zu", copy.size);
      DN_UT_AssertF(&result, copy.data[0] == 'A', "copy[0]: %c", copy.data[0]);
      DN_UT_AssertF(&result, copy.data[1] == 'B', "copy[1]: %c", copy.data[1]);
      DN_UT_AssertF(&result, copy.data[2] == 0, "copy[2]: %c", copy.data[2]);
      DN_TCScratchEnd(&scratch);
    }

    for (DN_UT_Test(&result, "Trim whitespace around string")) {
      DN_Str8 string = DN_Str8TrimWhitespaceAround(DN_Str8Lit(" AB "));
      DN_UT_AssertF(&result, DN_Str8Eq(string, DN_Str8Lit("AB")), "[string=%.*s]", DN_Str8PrintFmt(string));
    }

    for (DN_UT_Test(&result, "Allocate string from arena")) {
      DN_TCScratch scratch = DN_TCScratchBegin(nullptr, 0);
      DN_Str8      string  = DN_Str8AllocArena(2, DN_ZMem_No, &scratch.arena);
      DN_UT_AssertF(&result, string.size == 2, "size: %zu", string.size);
      DN_TCScratchEnd(&scratch);
    }

    // NOTE: TrimPrefix/Suffix /////////////////////////////////////////////////////////////////////
    for (DN_UT_Test(&result, "Trim prefix with matching prefix")) {
      DN_Str8 input      = DN_Str8Lit("nft/abc");
      DN_Str8 str_result = DN_Str8TrimPrefix(input, DN_Str8Lit("nft/"));
      DN_UT_AssertF(&result, DN_Str8Eq(str_result, DN_Str8Lit("abc")), "%.*s", DN_Str8PrintFmt(str_result));
    }

    for (DN_UT_Test(&result, "Trim prefix with non matching prefix")) {
      DN_Str8 input      = DN_Str8Lit("nft/abc");
      DN_Str8 str_result = DN_Str8TrimPrefix(input, DN_Str8Lit(" ft/"));
      DN_UT_AssertF(&result, DN_Str8Eq(str_result, input), "%.*s", DN_Str8PrintFmt(str_result));
    }

    for (DN_UT_Test(&result, "Trim suffix with matching suffix")) {
      DN_Str8 input      = DN_Str8Lit("nft/abc");
      DN_Str8 str_result = DN_Str8TrimSuffix(input, DN_Str8Lit("abc"));
      DN_UT_AssertF(&result, DN_Str8Eq(str_result, DN_Str8Lit("nft/")), "%.*s", DN_Str8PrintFmt(str_result));
    }

    for (DN_UT_Test(&result, "Trim suffix with non matching suffix")) {
      DN_Str8 input      = DN_Str8Lit("nft/abc");
      DN_Str8 str_result = DN_Str8TrimSuffix(input, DN_Str8Lit("ab"));
      DN_UT_AssertF(&result, DN_Str8Eq(str_result, input), "%.*s", DN_Str8PrintFmt(str_result));
    }

    // NOTE: DN_Str8IsAllDigits //////////////////////////////////////////////////////////////
    for (DN_UT_Test(&result, "Is all digits fails on non-digit string")) {
      DN_B32 str_result = DN_Str8IsAll(DN_Str8Lit("@123string"), DN_Str8IsAllType_Digits);
      DN_UT_Assert(&result, str_result == false);
    }

    for (DN_UT_Test(&result, "Is all digits fails on nullptr")) {
      DN_B32 str_result = DN_Str8IsAll(DN_Str8FromPtr(nullptr, 0), DN_Str8IsAllType_Digits);
      DN_UT_Assert(&result, str_result == false);
    }

    for (DN_UT_Test(&result, "Is all digits fails on string w/ 0 size")) {
      char const buf[]      = "@123string";
      DN_B32     str_result = DN_Str8IsAll(DN_Str8FromPtr(buf, 0), DN_Str8IsAllType_Digits);
      DN_UT_Assert(&result, !str_result);
    }

    for (DN_UT_Test(&result, "Is all digits success")) {
      DN_B32 str_result = DN_Str8IsAll(DN_Str8Lit("23"), DN_Str8IsAllType_Digits);
      DN_UT_Assert(&result, DN_Cast(bool) str_result == true);
    }

    for (DN_UT_Test(&result, "Is all digits fails on whitespace")) {
      DN_B32 str_result = DN_Str8IsAll(DN_Str8Lit("23 "), DN_Str8IsAllType_Digits);
      DN_UT_Assert(&result, DN_Cast(bool) str_result == false);
    }

    // NOTE: DN_Str8BSplit ///////////////////////////////////////////////////////////////////
    {
      {
        char const *TEST_FMT  = "Binary split \"%.*s\" with \"%.*s\"";
        DN_Str8     delimiter = DN_Str8Lit("/");
        DN_Str8     input     = DN_Str8Lit("abcdef");
        for (DN_UT_Test(&result, TEST_FMT, DN_Str8PrintFmt(input), DN_Str8PrintFmt(delimiter))) {
          DN_Str8BSplitResult split = DN_Str8BSplit(input, delimiter);
          DN_UT_AssertF(&result, DN_Str8Eq(split.lhs, DN_Str8Lit("abcdef")), "[lhs=%.*s]", DN_Str8PrintFmt(split.lhs));
          DN_UT_AssertF(&result, DN_Str8Eq(split.rhs, DN_Str8Lit("")), "[rhs=%.*s]", DN_Str8PrintFmt(split.rhs));
        }

        input = DN_Str8Lit("abc/def");
        for (DN_UT_Test(&result, TEST_FMT, DN_Str8PrintFmt(input), DN_Str8PrintFmt(delimiter))) {
          DN_Str8BSplitResult split = DN_Str8BSplit(input, delimiter);
          DN_UT_AssertF(&result, DN_Str8Eq(split.lhs, DN_Str8Lit("abc")), "[lhs=%.*s]", DN_Str8PrintFmt(split.lhs));
          DN_UT_AssertF(&result, DN_Str8Eq(split.rhs, DN_Str8Lit("def")), "[rhs=%.*s]", DN_Str8PrintFmt(split.rhs));
        }

        input = DN_Str8Lit("/abcdef");
        for (DN_UT_Test(&result, TEST_FMT, DN_Str8PrintFmt(input), DN_Str8PrintFmt(delimiter))) {
          DN_Str8BSplitResult split = DN_Str8BSplit(input, delimiter);
          DN_UT_AssertF(&result, DN_Str8Eq(split.lhs, DN_Str8Lit("")), "[lhs=%.*s]", DN_Str8PrintFmt(split.lhs));
          DN_UT_AssertF(&result, DN_Str8Eq(split.rhs, DN_Str8Lit("abcdef")), "[rhs=%.*s]", DN_Str8PrintFmt(split.rhs));
        }
      }

      {
        DN_Str8 delimiter = DN_Str8Lit("-=-");
        DN_Str8 input     = DN_Str8Lit("123-=-456");
        for (DN_UT_Test(&result, "Binary split \"%.*s\" with \"%.*s\"", DN_Str8PrintFmt(input), DN_Str8PrintFmt(delimiter))) {
          DN_Str8BSplitResult split = DN_Str8BSplit(input, delimiter);
          DN_UT_AssertF(&result, DN_Str8Eq(split.lhs, DN_Str8Lit("123")), "[lhs=%.*s]", DN_Str8PrintFmt(split.lhs));
          DN_UT_AssertF(&result, DN_Str8Eq(split.rhs, DN_Str8Lit("456")), "[rhs=%.*s]", DN_Str8PrintFmt(split.rhs));
        }
      }
    }

    // NOTE: DN_I64FromStr8
    for (DN_UT_Test(&result, "To I64: Convert empty string")) {
      DN_I64FromResult str_result = DN_I64FromStr8(DN_Str8Lit(""), 0);
      DN_UT_Assert(&result, str_result.success);
      DN_UT_Assert(&result, str_result.value == 0);
    }

    for (DN_UT_Test(&result, "To I64: Convert \"1\"")) {
      DN_I64FromResult str_result = DN_I64FromStr8(DN_Str8Lit("1"), 0);
      DN_UT_Assert(&result, str_result.success);
      DN_UT_Assert(&result, str_result.value == 1);
    }

    for (DN_UT_Test(&result, "To I64: Convert \"-0\"")) {
      DN_I64FromResult str_result = DN_I64FromStr8(DN_Str8Lit("-0"), 0);
      DN_UT_Assert(&result, str_result.success);
      DN_UT_Assert(&result, str_result.value == 0);
    }

    for (DN_UT_Test(&result, "To I64: Convert \"-1\"")) {
      DN_I64FromResult str_result = DN_I64FromStr8(DN_Str8Lit("-1"), 0);
      DN_UT_Assert(&result, str_result.success);
      DN_UT_Assert(&result, str_result.value == -1);
    }

    for (DN_UT_Test(&result, "To I64: Convert \"1.2\"")) {
      DN_I64FromResult str_result = DN_I64FromStr8(DN_Str8Lit("1.2"), 0);
      DN_UT_Assert(&result, !str_result.success);
      DN_UT_Assert(&result, str_result.value == 1);
    }

    for (DN_UT_Test(&result, "To I64: Convert \"1,234\"")) {
      DN_I64FromResult str_result = DN_I64FromStr8(DN_Str8Lit("1,234"), ',');
      DN_UT_Assert(&result, str_result.success);
      DN_UT_Assert(&result, str_result.value == 1234);
    }

    for (DN_UT_Test(&result, "To I64: Convert \"1,2\"")) {
      DN_I64FromResult str_result = DN_I64FromStr8(DN_Str8Lit("1,2"), ',');
      DN_UT_Assert(&result, str_result.success);
      DN_UT_Assert(&result, str_result.value == 12);
    }

    for (DN_UT_Test(&result, "To I64: Convert \"12a3\"")) {
      DN_I64FromResult str_result = DN_I64FromStr8(DN_Str8Lit("12a3"), 0);
      DN_UT_Assert(&result, !str_result.success);
      DN_UT_Assert(&result, str_result.value == 12);
    }

    // NOTE: DN_U64FromStr8
    for (DN_UT_Test(&result, "To U64: Convert empty string")) {
      DN_U64FromResult str_result = DN_U64FromStr8(DN_Str8Lit(""), 0);
      DN_UT_Assert(&result, str_result.success);
      DN_UT_AssertF(&result, str_result.value == 0, "result: %" PRIu64, str_result.value);
    }

    for (DN_UT_Test(&result, "To U64: Convert \"1\"")) {
      DN_U64FromResult str_result = DN_U64FromStr8(DN_Str8Lit("1"), 0);
      DN_UT_Assert(&result, str_result.success);
      DN_UT_AssertF(&result, str_result.value == 1, "result: %" PRIu64, str_result.value);
    }

    for (DN_UT_Test(&result, "To U64: Convert \"-0\"")) {
      DN_U64FromResult str_result = DN_U64FromStr8(DN_Str8Lit("-0"), 0);
      DN_UT_Assert(&result, !str_result.success);
      DN_UT_AssertF(&result, str_result.value == 0, "result: %" PRIu64, str_result.value);
    }

    for (DN_UT_Test(&result, "To U64: Convert \"-1\"")) {
      DN_U64FromResult str_result = DN_U64FromStr8(DN_Str8Lit("-1"), 0);
      DN_UT_Assert(&result, !str_result.success);
      DN_UT_AssertF(&result, str_result.value == 0, "result: %" PRIu64, str_result.value);
    }

    for (DN_UT_Test(&result, "To U64: Convert \"1.2\"")) {
      DN_U64FromResult str_result = DN_U64FromStr8(DN_Str8Lit("1.2"), 0);
      DN_UT_Assert(&result, !str_result.success);
      DN_UT_AssertF(&result, str_result.value == 1, "result: %" PRIu64, str_result.value);
    }

    for (DN_UT_Test(&result, "To U64: Convert \"1,234\"")) {
      DN_U64FromResult str_result = DN_U64FromStr8(DN_Str8Lit("1,234"), ',');
      DN_UT_Assert(&result, str_result.success);
      DN_UT_AssertF(&result, str_result.value == 1234, "result: %" PRIu64, str_result.value);
    }

    for (DN_UT_Test(&result, "To U64: Convert \"1,2\"")) {
      DN_U64FromResult str_result = DN_U64FromStr8(DN_Str8Lit("1,2"), ',');
      DN_UT_Assert(&result, str_result.success);
      DN_UT_AssertF(&result, str_result.value == 12, "result: %" PRIu64, str_result.value);
    }

    for (DN_UT_Test(&result, "To U64: Convert \"12a3\"")) {
      DN_U64FromResult str_result = DN_U64FromStr8(DN_Str8Lit("12a3"), 0);
      DN_UT_Assert(&result, !str_result.success);
      DN_UT_AssertF(&result, str_result.value == 12, "result: %" PRIu64, str_result.value);
    }

    // NOTE: DN_Str8Find
    for (DN_UT_Test(&result, "Find: String (char) is not in buffer")) {
      DN_Str8           buf        = DN_Str8Lit("836a35becd4e74b66a0d6844d51f1a63018c7ebc44cf7e109e8e4bba57eefb55");
      DN_Str8           find       = DN_Str8Lit("2");
      DN_Str8FindResult str_result = DN_Str8FindStr8(buf, find, DN_Str8EqCase_Sensitive);
      DN_UT_Assert(&result, !str_result.found);
      DN_UT_Assert(&result, str_result.index == 0);
      DN_UT_Assert(&result, str_result.match.data == nullptr);
      DN_UT_Assert(&result, str_result.match.size == 0);
    }

    for (DN_UT_Test(&result, "Find: String (char) is in buffer")) {
      DN_Str8           buf        = DN_Str8Lit("836a35becd4e74b66a0d6844d51f1a63018c7ebc44cf7e109e8e4bba57eefb55");
      DN_Str8           find       = DN_Str8Lit("6");
      DN_Str8FindResult str_result = DN_Str8FindStr8(buf, find, DN_Str8EqCase_Sensitive);
      DN_UT_Assert(&result, str_result.found);
      DN_UT_Assert(&result, str_result.index == 2);
      DN_UT_Assert(&result, str_result.match.data[0] == '6');
    }

    // NOTE: DN_Str8FileNameFromPath
    for (DN_UT_Test(&result, "File name from Windows path")) {
      DN_Str8 buf        = DN_Str8Lit("C:\\ABC\\str_result.exe");
      DN_Str8 str_result = DN_Str8FileNameFromPath(buf);
      DN_UT_AssertF(&result, DN_Str8Eq(str_result, DN_Str8Lit("str_result.exe")), "%.*s", DN_Str8PrintFmt(str_result));
    }

    for (DN_UT_Test(&result, "File name from Linux path")) {
      DN_Str8 buf        = DN_Str8Lit("/ABC/str_result.exe");
      DN_Str8 str_result = DN_Str8FileNameFromPath(buf);
      DN_UT_AssertF(&result, DN_Str8Eq(str_result, DN_Str8Lit("str_result.exe")), "%.*s", DN_Str8PrintFmt(str_result));
    }

    // NOTE: DN_Str8TrimPrefix
    for (DN_UT_Test(&result, "Trim prefix")) {
      DN_Str8 prefix     = DN_Str8Lit("@123");
      DN_Str8 buf        = DN_Str8Lit("@123string");
      DN_Str8 str_result = DN_Str8TrimPrefix(buf, prefix, DN_Str8EqCase_Sensitive);
      DN_UT_Assert(&result, DN_Str8Eq(str_result, DN_Str8Lit("string")));
    }

    // NOTE: DN_Str8TruncMiddle
    {
      for (DN_UT_Test(&result, "TruncMiddlePtr: Short string is not truncated")) {
        DN_Str8            str      = DN_Str8Lit("Hello");
        DN_Str8            trunc    = DN_Str8Lit("...");
        char               dest[64] = {};
        DN_Str8TruncResult res      = DN_Str8TruncMiddlePtr(str, 5, trunc, dest, sizeof(dest));
        DN_UT_Assert(&result, !res.truncated);
        DN_UT_Assert(&result, res.size_req == 5);
        DN_UT_AssertF(&result, DN_Str8Eq(res.str8, DN_Str8Lit("Hello")), "%.*s", DN_Str8PrintFmt(res.str8));
      }

      for (DN_UT_Test(&result, "TruncMiddlePtr: Exact boundary (2*side_size) is not truncated")) {
        DN_Str8            str      = DN_Str8Lit("HelloWorld"); // 10 chars
        DN_Str8            trunc    = DN_Str8Lit("...");
        char               dest[64] = {};
        DN_Str8TruncResult res      = DN_Str8TruncMiddlePtr(str, 5, trunc, dest, sizeof(dest));
        DN_UT_Assert(&result, !res.truncated);
        DN_UT_Assert(&result, res.size_req == 10);
        DN_UT_AssertF(&result, DN_Str8Eq(res.str8, DN_Str8Lit("HelloWorld")), "%.*s", DN_Str8PrintFmt(res.str8));
      }

      for (DN_UT_Test(&result, "TruncMiddlePtr: Long string is truncated in the middle")) {
        DN_Str8            str      = DN_Str8Lit("HelloBeautifulWorld");
        DN_Str8            trunc    = DN_Str8Lit("...");
        char               dest[64] = {};
        DN_Str8TruncResult res      = DN_Str8TruncMiddlePtr(str, 5, trunc, dest, sizeof(dest));
        DN_UT_Assert(&result, res.truncated);
        DN_UT_Assert(&result, res.size_req == 13); // 5 + 3 + 5
        DN_UT_AssertF(&result, DN_Str8Eq(res.str8, DN_Str8Lit("Hello...World")), "%.*s", DN_Str8PrintFmt(res.str8));
      }

      for (DN_UT_Test(&result, "TruncMiddlePtr: Empty truncator concatenates head and tail")) {
        DN_Str8            str      = DN_Str8Lit("HelloBeautifulWorld");
        DN_Str8            trunc    = DN_Str8Lit("");
        char               dest[64] = {};
        DN_Str8TruncResult res      = DN_Str8TruncMiddlePtr(str, 5, trunc, dest, sizeof(dest));
        DN_UT_Assert(&result, res.truncated);
        DN_UT_Assert(&result, res.size_req == 10); // 5 + 0 + 5
        DN_UT_AssertF(&result, DN_Str8Eq(res.str8, DN_Str8Lit("HelloWorld")), "%.*s", DN_Str8PrintFmt(res.str8));
      }

      for (DN_UT_Test(&result, "TruncMiddlePtr: side_size of 0 returns just truncator")) {
        DN_Str8            str      = DN_Str8Lit("HelloWorld");
        DN_Str8            trunc    = DN_Str8Lit("...");
        char               dest[64] = {};
        DN_Str8TruncResult res      = DN_Str8TruncMiddlePtr(str, 0, trunc, dest, sizeof(dest));
        DN_UT_Assert(&result, res.truncated);
        DN_UT_Assert(&result, res.size_req == 3);
        DN_UT_AssertF(&result, DN_Str8Eq(res.str8, DN_Str8Lit("...")), "%.*s", DN_Str8PrintFmt(res.str8));
      }

      for (DN_UT_Test(&result, "TruncMiddlePtr: Null dest calculates size without writing")) {
        DN_Str8            str   = DN_Str8Lit("HelloBeautifulWorld");
        DN_Str8            trunc = DN_Str8Lit("...");
        DN_Str8TruncResult res   = DN_Str8TruncMiddlePtr(str, 5, trunc, nullptr, 0);
        DN_UT_Assert(&result, res.truncated);
        DN_UT_Assert(&result, res.size_req == 13);
        DN_UT_Assert(&result, res.str8.data == nullptr);
      }

      for (DN_UT_Test(&result, "TruncMiddlePtr: size_req is consistent between dry-run and actual")) {
        DN_Str8            str      = DN_Str8Lit("HelloBeautifulWorld");
        DN_Str8            trunc    = DN_Str8Lit("...");
        DN_Str8TruncResult dry      = DN_Str8TruncMiddlePtr(str, 5, trunc, nullptr, 0);
        char               dest[64] = {};
        DN_Str8TruncResult actual   = DN_Str8TruncMiddlePtr(str, 5, trunc, dest, sizeof(dest));
        DN_UT_Assert(&result, dry.size_req == actual.size_req);
        DN_UT_Assert(&result, dry.truncated == actual.truncated);
      }

      for (DN_UT_Test(&result, "TruncMiddlePtr: Minimum buffer size (2*side_size + trunc.size + 1) is sufficient")) {
        DN_Str8            str      = DN_Str8Lit("HelloBeautifulWorld");
        DN_Str8            trunc    = DN_Str8Lit("...");
        char               dest[14] = {}; // Exactly 2*5 + 3 + 1
        DN_Str8TruncResult res      = DN_Str8TruncMiddlePtr(str, 5, trunc, dest, sizeof(dest));
        DN_UT_Assert(&result, res.truncated);
        DN_UT_Assert(&result, res.size_req == 13);
        DN_UT_AssertF(&result, DN_Str8Eq(res.str8, DN_Str8Lit("Hello...World")), "%.*s", DN_Str8PrintFmt(res.str8));
      }

      for (DN_UT_Test(&result, "TruncMiddlePtr: Single character side size")) {
        DN_Str8            str      = DN_Str8Lit("HelloBeautifulWorld");
        DN_Str8            trunc    = DN_Str8Lit("...");
        char               dest[64] = {};
        DN_Str8TruncResult res      = DN_Str8TruncMiddlePtr(str, 1, trunc, dest, sizeof(dest));
        DN_UT_Assert(&result, res.truncated);
        DN_UT_Assert(&result, res.size_req == 5); // 1 + 3 + 1
        DN_UT_AssertF(&result, DN_Str8Eq(res.str8, DN_Str8Lit("H...d")), "%.*s", DN_Str8PrintFmt(res.str8));
      }

      for (DN_UT_Test(&result, "TruncMiddlePtr: Large side_size falls back to copy")) {
        DN_Str8            str      = DN_Str8Lit("Hello");
        DN_Str8            trunc    = DN_Str8Lit("...");
        char               dest[64] = {};
        DN_Str8TruncResult res      = DN_Str8TruncMiddlePtr(str, 100, trunc, dest, sizeof(dest));
        DN_UT_Assert(&result, !res.truncated);
        DN_UT_Assert(&result, res.size_req == 5);
        DN_UT_AssertF(&result, DN_Str8Eq(res.str8, DN_Str8Lit("Hello")), "%.*s", DN_Str8PrintFmt(res.str8));
      }

      // NOTE: DN_Str8TruncMiddle (arena wrapper)
      for (DN_UT_Test(&result, "TruncMiddle: Arena wrapper allocates and truncates correctly")) {
        DN_TCScratch       scratch = DN_TCScratchBegin(nullptr, 0);
        DN_Str8            str     = DN_Str8Lit("HelloBeautifulWorld");
        DN_Str8            trunc   = DN_Str8Lit("...");
        DN_Str8TruncResult res     = DN_Str8TruncMiddle(str, 5, trunc, &scratch.arena);
        DN_UT_Assert(&result, res.truncated);
        DN_UT_Assert(&result, res.size_req == 13);
        DN_UT_AssertF(&result, DN_Str8Eq(res.str8, DN_Str8Lit("Hello...World")), "%.*s", DN_Str8PrintFmt(res.str8));
        DN_UT_Assert(&result, res.str8.data[res.str8.size] == '\0');
        DN_TCScratchEnd(&scratch);
      }
    }
  }
  return result;
}

static DN_UTCore DN_TST_Win()
{
  DN_UTCore result = DN_UT_Init();
  #if defined(DN_PLATFORM_WIN32)
  DN_UT_LogF(&result, "OS Win32\n");
  {
    DN_TCScratch scratch    = DN_TCScratchBegin(nullptr, 0);
    DN_Str8      input8  = DN_Str8Lit("String");
    DN_Str16     input16 = DN_Str16{(wchar_t *)(L"String"), sizeof(L"String") / sizeof(L"String"[0]) - 1};

    for (DN_UT_Test(&result, "Str8 to Str16")) {
      DN_Str16 str_result = DN_OS_W32Str8ToStr16(&scratch.arena, input8);
      DN_UT_Assert(&result, DN_Str16Eq(str_result, input16));
    }

    for (DN_UT_Test(&result, "Str16 to Str8")) {
      DN_Str8 str_result = DN_OS_W32Str16ToStr8(&scratch.arena, input16);
      DN_UT_Assert(&result, DN_Str8Eq(str_result, input8));
    }

    for (DN_UT_Test(&result, "Str16 to Str8: Null terminates string")) {
      int   size_required = DN_OS_W32Str16ToStr8Buffer(input16, nullptr, 0);
      char *string        = DN_ArenaNewArray(&scratch.arena, char, size_required + 1, DN_ZMem_No);

      // Fill the string with error sentinels
      DN_Memset(string, 'Z', size_required + 1);

      int        size_returned = DN_OS_W32Str16ToStr8Buffer(input16, string, size_required + 1);
      char const EXPECTED[]    = {'S', 't', 'r', 'i', 'n', 'g', 0};

      DN_UT_AssertF(&result, size_required == size_returned, "string_size: %d, result: %d", size_required, size_returned);
      DN_UT_AssertF(&result, size_returned == DN_ArrayCountU(EXPECTED) - 1, "string_size: %d, expected: %zu", size_returned, DN_ArrayCountU(EXPECTED) - 1);
      DN_UT_Assert(&result, DN_Memcmp(EXPECTED, string, sizeof(EXPECTED)) == 0);
    }

    for (DN_UT_Test(&result, "Str16 to Str8: Arena null terminates string")) {
      DN_Str8    string8       = DN_OS_W32Str16ToStr8(&scratch.arena, input16);
      int        size_returned = DN_OS_W32Str16ToStr8Buffer(input16, nullptr, 0);
      char const EXPECTED[]    = {'S', 't', 'r', 'i', 'n', 'g', 0};

      DN_UT_AssertF(&result, DN_Cast(int) string8.size == size_returned, "string_size: %d, result: %d", DN_Cast(int) string8.size, size_returned);
      DN_UT_AssertF(&result, DN_Cast(int) string8.size == DN_ArrayCountU(EXPECTED) - 1, "string_size: %d, expected: %zu", DN_Cast(int) string8.size, DN_ArrayCountU(EXPECTED) - 1);
      DN_UT_Assert(&result, DN_Memcmp(EXPECTED, string8.data, sizeof(EXPECTED)) == 0);
    }
    DN_TCScratchEnd(&scratch);
  }
  #endif // DN_PLATFORM_WIN32
  return result;
}

static DN_UTCore DN_TST_Net()
{
  DN_UTCore result = DN_UT_Init();
#if defined(DN_UNIT_TESTS_WITH_NET)
  DN_Str8         label         = {};
  DN_NETInterface net_interface = {};
#if defined(DN_PLATFORM_EMSCRIPTEN)
  net_interface = DN_NET_EmcInterface();
  label         = DN_Str8Lit("Emscripten");
#elif defined(DN_UNIT_TESTS_WITH_CURL)
  net_interface = DN_NET_CurlInterface();
  label         = DN_Str8Lit("CURL");
#endif

  if (label.size) {
    DN_UT_LogF(&result, "DN_NET\n");

    DN_MemList mem                    = DN_MemListFromHeap(DN_Megabytes(4), DN_MemFlags_Nil);
    DN_Arena   arena                  = DN_ArenaFromMemList(&mem);
    DN_Str8    remote_ws_server_url   = DN_Str8Lit("wss://echo.websocket.org");
    DN_Str8    remote_http_server_url = DN_Str8Lit("https://google.com");

    DN_USize   net_base_size = DN_Megabytes(1);
    char      *net_base      = DN_ArenaNewArray(&arena, char, net_base_size, DN_ZMem_Yes);
    DN_NETCore net           = {};
    net_interface.init(&net, net_base, net_base_size);

    DN_U64 arena_reset_p = DN_MemListPos(arena.mem);
    for (DN_UT_Test(&result, "%.*s WaitForResponse HTTP GET request", DN_Str8PrintFmt(label))) {
      DN_NETRequestHandle request  = net_interface.do_http(&net, remote_http_server_url, DN_Str8Lit("GET"), nullptr);
      DN_NETResponse      response = net_interface.wait_for_response(request, &arena, UINT32_MAX);
      DN_UT_AssertF(&result, response.http_status == 200, "http_status=%u", response.http_status);
      DN_UT_AssertF(&result, response.state == DN_NETResponseState_HTTP, "state=%u", response.state);
      DN_UT_AssertF(&result, response.error_str8.size == 0, "%.*s", DN_Str8PrintFmt(response.error_str8));
      DN_UT_Assert(&result, response.body.size);
    }

    for (DN_UT_Test(&result, "%.*s WaitForResponse HTTP POST request", DN_Str8PrintFmt(label))) {
      net_interface.do_http(&net, remote_http_server_url, DN_Str8Lit("POST"), nullptr);
      DN_NETResponse response = net_interface.wait_for_any_response(&net, &arena, UINT32_MAX);
      DN_UT_AssertF(&result, response.http_status == 200, "http_status=%u", response.http_status);
      DN_UT_AssertF(&result, response.state == DN_NETResponseState_HTTP, "state=%u", response.state);
      DN_UT_AssertF(&result, response.error_str8.size == 0, "error=%.*s", DN_Str8PrintFmt(response.error_str8));
      DN_UT_Assert(&result, response.body.size);
    }

    for (DN_UT_Test(&result, "%.*s WaitForResponse WS request", DN_Str8PrintFmt(label))) {
      DN_NETRequestHandle request       = net_interface.do_ws(&net, remote_ws_server_url);
      DN_USize const      WS_TIMEOUT_MS = 16;

      // NOTE: Wait for WS connection to open
      for (bool done = false; result.state != DN_UTState_TestFailed && !done; DN_MemListPopTo(arena.mem, arena_reset_p)) {
        DN_NETResponse response = net_interface.wait_for_response(request, &arena, WS_TIMEOUT_MS);
        if (response.state == DN_NETResponseState_Nil) // NOTE: Timeout
          continue;
        if (response.state == DN_NETResponseState_Error)
            DN_UT_Log(&result, "ERROR: %.*s", DN_Str8PrintFmt(response.error_str8));
        DN_UT_AssertF(&result, response.state == DN_NETResponseState_WSOpen, "state=%d", response.state);
        done = true;
      }

      // NOTE: Receive the initial text from the echo server
      for (bool done = false; result.state != DN_UTState_TestFailed && !done; DN_MemListPopTo(arena.mem, arena_reset_p)) {
        DN_NETResponse response = net_interface.wait_for_response(request, &arena, WS_TIMEOUT_MS);
        if (response.state == DN_NETResponseState_Nil) // NOTE: Timeout
          continue;
        if (response.state == DN_NETResponseState_Error)
          DN_UT_Log(&result, "ERROR: %.*s", DN_Str8PrintFmt(response.error_str8));
        DN_UT_AssertF(&result, response.state == DN_NETResponseState_WSText, "state=%d", response.state);
        // NOTE: Send the close signal
        net_interface.do_ws_send(request, DN_Str8Lit(""), DN_NETWSSend_Close);
        done = true;
      }

      // NOTE: Expect to hear the close
      for (bool done = false; result.state != DN_UTState_TestFailed && !done; DN_MemListPopTo(arena.mem, arena_reset_p)) {
        DN_NETResponse response = net_interface.wait_for_response(request, &arena, WS_TIMEOUT_MS);
        if (response.state == DN_NETResponseState_Nil) // NOTE: Timeout
          continue;
        if (response.state == DN_NETResponseState_Error)
          DN_UT_Log(&result, "ERROR: %.*s", DN_Str8PrintFmt(response.error_str8));
        DN_UT_AssertF(&result, response.state == DN_NETResponseState_WSClose, "state=%d");
        done = true;
      }
    }
    net_interface.deinit(&net);
    DN_MemListDeinit(arena.mem);
  }
#endif // defined(DN_UNIT_TESTS_WITH_NET)
  return result;
}

DN_TSTResult DN_TST_RunSuite(DN_TSTPrint print)
{
  DN_UTCore tests[] =
      {
          DN_TST_Base(),
          DN_TST_BaseArena(),
          DN_TST_BaseStrings(),
          DN_TST_BaseBytesHex(),
          #if DN_H_WITH_HELPERS
          DN_TST_BinarySearch(),
          #endif
          DN_TST_BaseDSMap(),
          DN_TST_BaseIArray(),
          DN_TST_BaseCArray2(),
          DN_TST_BaseVArray(),
          DN_TST_Keccak(),
          DN_TST_M4(),
          DN_TST_OS(),
          DN_TST_Rect(),
          DN_TST_Win(),
          DN_TST_Net(),
      };

  DN_TSTResult result = {};
  for (const DN_UTCore &test : tests) {
    result.total_tests += test.num_tests_in_group;
    result.total_good_tests += test.num_tests_ok_in_group;
  }
  result.passed = result.total_tests == result.total_good_tests;

  bool print_summary = false;
  for (DN_UTCore &test : tests) {
    if (test.num_tests_in_group <= 0)
      continue;
    bool do_print = print == DN_TSTPrint_Yes;
    if (print == DN_TSTPrint_OnFailure && test.num_tests_ok_in_group != test.num_tests_in_group)
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
