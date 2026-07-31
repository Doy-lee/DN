#if defined(_CLANGD)
  #define DN_WITH_TESTS 1
  #define DN_WITH_OS 1
  #define DN_WITH_NET 1
  #define DN_WITH_NET_CURL 1
  #define DN_ARENA_TEMP_MEM_UAF_GUARD 1
  #include "dn.h"
#endif

#if DN_STR8_AVX512F
  #include <immintrin.h>
#endif

enum DN_ArenaUAFCheckReportType_
{
  DN_ArenaUAFCheckReportType_AllocViolation,
  DN_ArenaUAFCheckReportType_TempEndOutOfOrder,
};

DN_Core *g_dn_;

DN_API void DN_Init(DN_Core *dn, DN_InitFlags flags, DN_TcInitArgs args)
{
  DN_Set(dn);
  dn->init_flags = flags;

  if (DN_BitIsSet(flags, DN_InitFlags_OS)) {
    #if DN_WITH_OS
    DN_OSCore *os = &dn->os;
    dn->os_init   = true;
    DN_OS_SetLogPrintFuncToOS();

    // NOTE: Query OS information
    {
      #if defined(DN_PLATFORM_WIN32)
      SYSTEM_INFO system_info = {};
      GetSystemInfo(&system_info);
      os->logical_processor_count = system_info.dwNumberOfProcessors;
      os->page_size               = system_info.dwPageSize;
      os->alloc_granularity       = system_info.dwAllocationGranularity;
      #else
        #if defined(DN_PLATFORM_EMSCRIPTEN)
        os->logical_processor_count = 1;
        #else
        os->logical_processor_count = get_nprocs();
        #endif
      os->page_size               = getpagesize();
      os->alloc_granularity       = os->page_size;
      #endif
    }

    {
      os->mem   = DN_MemListFromHeap(DN_Megabytes(1), DN_Kilobytes(4), DN_MemFlags_NoAllocTrack, DN_OS_HeapInitDefault());
      os->arena = DN_ArenaFromMemList(&os->mem);

      #if defined(DN_PLATFORM_WIN32)
      os->platform_context = DN_ArenaNew(&os->arena, DN_OSW32Core, DN_ZMem_Yes);
      #elif defined(DN_PLATFORM_POSIX) || defined(DN_PLATFORM_EMSCRIPTEN)
      os->platform_context = DN_ArenaNew(&os->arena, DN_OSPosixCore, DN_ZMem_Yes);
      #endif

      #if defined(DN_PLATFORM_WIN32)
      DN_OSW32Core *w32 = DN_Cast(DN_OSW32Core *) os->platform_context;
      InitializeCriticalSection(&w32->sync_primitive_free_list_mutex);

      QueryPerformanceFrequency(&w32->qpc_frequency);
      HMODULE module = LoadLibraryA("kernel32.dll");
      if (module) {
        w32->set_thread_description = DN_Cast(DN_OSW32SetThreadDescriptionFunc *) GetProcAddress(module, "SetThreadDescription");
        FreeLibrary(module);
      }

      // NOTE: win32 bcrypt
      wchar_t const     BCRYPT_ALGORITHM[] = L"RNG";
      long /*NTSTATUS*/ init_status        = BCryptOpenAlgorithmProvider(&w32->bcrypt_rng_handle, BCRYPT_ALGORITHM, nullptr /*implementation*/, 0 /*flags*/);
      if (w32->bcrypt_rng_handle && init_status == 0)
        w32->bcrypt_init_success = true;
      else
        DN_LogErrorF("Failed to initialise Windows secure random number generator, error: %d", init_status);
      #else
      DN_OS_PosixInit(DN_Cast(DN_OSPosixCore *)os->platform_context);
      #endif
    }

    os->cpu_report = DN_CPUGetReport();

    #define DN_CPU_FEAT_XENTRY(label) g_dn_cpu_feature_decl[DN_CPUFeature_##label] = {DN_CPUFeature_##label, DN_Str8Lit(#label)};
    DN_CPU_FEAT_XMACRO
    #undef DN_CPU_FEAT_XENTRY
    DN_Assert(g_dn_);
    #endif

    // NOTE: Initialise thread context
    DN_TcInitFromHeap(&dn->main_tc, DN_OS_ThreadID(), args, DN_OS_HeapInitDefault());
    DN_TcEquip(&dn->main_tc);
  }

  if (DN_BitIsSet(flags, DN_InitFlags_LeakTracker)) {
    DN_Assert(dn->os_init);
    #if DN_WITH_OS
    // NOTE: Setup the allocation table with allocation tracking turned off on
    // the arena we're using to initialise the table.
    DN_HTableInitArgs table_args = DN_HTableInitArgsDefault(DN_LeakTrackerKV, dn->leak.alloc_table_kvs, hash, key, value);
    dn->leak.alloc_table         = DN_HTableInitHeapAssert(table_args, DN_OS_HeapInitBasic());
    #endif
  }

  // NOTE: Print out init features
  char buf[4096];
  DN_USize buf_size = 0;
  if (DN_BitIsSet(flags, DN_InitFlags_LogLibFeatures)) {
    DN_FmtAppendTruncate(buf, &buf_size, sizeof(buf), DN_Str8Lit("..."), "DN initialised:\n");
    #if DN_WITH_OS
    DN_F32 page_size_kib         = dn->os.page_size / 1024.0f;
    DN_F32 alloc_granularity_kib = dn->os.alloc_granularity / 1024.0f;
    DN_FmtAppendTruncate(buf,
                         &buf_size,
                         sizeof(buf),
                         DN_Str8Lit("..."),
                         "  OS Page/Granularity/Cores: %.0fKiB/%.0fKiB/%u\n",
                         page_size_kib,
                         alloc_granularity_kib,
                         dn->os.logical_processor_count);
    #endif

    DN_FmtAppendTruncate(buf, &buf_size, sizeof(buf), DN_Str8Lit("..."), "  Thread Context: ");
    if (DN_BitIsSet(flags, DN_InitFlags_OS)) {
      DN_Arena *arena = dn->main_tc.main_arena;
      DN_Str8   heap  = DN_Str8Lit("");
      switch (arena->mem->heap.type) {
        case DN_HeapType_Nil:     break;
        case DN_HeapType_Basic:   heap = DN_Str8Lit("Basic"); break;
        case DN_HeapType_Virtual: heap = DN_Str8Lit("Virtual"); break;
      }
      DN_Str8x32 main_commit  = DN_Str8x32FromByteCountU64Auto(dn->main_tc.main_arena->mem->curr->commit);
      DN_Str8x32 main_reserve = DN_Str8x32FromByteCountU64Auto(dn->main_tc.main_arena->mem->curr->reserve);
      DN_Str8x32 err_commit   = DN_Str8x32FromByteCountU64Auto(dn->main_tc.err_sink.arena->mem->curr->commit);
      DN_Str8x32 err_reserve  = DN_Str8x32FromByteCountU64Auto(dn->main_tc.err_sink.arena->mem->curr->reserve);
      DN_FmtAppendTruncate(buf, &buf_size, sizeof(buf), DN_Str8Lit("..."), "M %.*s/%.*s", DN_Str8PrintFmt(main_commit), DN_Str8PrintFmt(main_reserve));
      if (dn->main_tc.temp_arenas_count) {
        DN_Arena  *temp         = dn->main_tc.temp_arenas[0];
        DN_Str8x32 temp_commit  = DN_Str8x32FromByteCountU64Auto(temp->mem->curr->commit);
        DN_Str8x32 temp_reserve = DN_Str8x32FromByteCountU64Auto(temp->mem->curr->reserve);
        DN_FmtAppendTruncate(buf, &buf_size, sizeof(buf), DN_Str8Lit("..."), " T(x%zu) %.*s/%.*s", dn->main_tc.temp_arenas_count, DN_Str8PrintFmt(temp_commit), DN_Str8PrintFmt(temp_reserve));
      }
      DN_FmtAppendTruncate(buf, &buf_size, sizeof(buf), DN_Str8Lit("..."), " E %.*s/%.*s (%.*s)\n", DN_Str8PrintFmt(err_commit), DN_Str8PrintFmt(err_reserve), DN_Str8PrintFmt(heap));
    } else {
      DN_FmtAppendTruncate(buf, &buf_size, sizeof(buf), DN_Str8Lit("..."), "N/A\n");
    }

    #if DN_HAS_FEATURE(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
    if (DN_ASAN_POISON) {
      DN_FmtAppendTruncate(buf, &buf_size, sizeof(buf), DN_Str8Lit("..."), "  ASAN manual poisoning%s\n", DN_ASAN_VET_POISON ? " (+vet sanity checks)" : "");
      DN_FmtAppendTruncate(buf, &buf_size, sizeof(buf), DN_Str8Lit("..."), "  ASAN poison guard size: %u\n", DN_ASAN_POISON_GUARD_SIZE);
    }
    #endif

    #if defined(DN_LEAK_TRACKING)
    DN_FmtAppendTruncate(buf, &buf_size, sizeof(buf), DN_Str8Lit("..."), "  Allocation leak tracing\n");
    #endif

    #if defined(DN_PLATFORM_EMSCRIPTEN) || defined(DN_PLATFORM_POSIX)
    DN_OSPosixCore *posix = DN_Cast(DN_OSPosixCore *)g_dn_->os.platform_context;
    DN_FmtAppendTruncate(buf, &buf_size, sizeof(buf), DN_Str8Lit("..."), "  Clock GetTime: %S\n", posix->clock_monotonic_raw ? DN_Str8Lit("CLOCK_MONOTONIC_RAW") : DN_Str8Lit("CLOCK_MONOTONIC"));
    #endif

    // TODO(doyle): Add stacktrace feature log
  }

  if (DN_BitIsSet(flags, DN_InitFlags_LogCPUFeatures)) {
    DN_Assert(dn->os_init);
    #if DN_WITH_OS
    DN_CPUReport const *report = &dn->os.cpu_report;
    DN_Str8             brand  = DN_Str8TrimWhitespaceAround(DN_Str8FromPtr(report->brand, sizeof(report->brand) - 1));
    DN_FmtAppendTruncate(buf, &buf_size, sizeof(buf), DN_Str8Lit("..."), "  CPU '%.*s' from '%s' detected:\n", DN_Str8PrintFmt(brand), report->vendor);

    DN_USize longest_feature_name = 0;
    for (DN_ForIndexU(feature_index, DN_CPUFeature_Count)) {
      DN_CPUFeatureDecl feature_decl = g_dn_cpu_feature_decl[feature_index];
      longest_feature_name           = DN_Max(longest_feature_name, feature_decl.label.count);
    }

    for (DN_ForIndexU(feature_index, DN_CPUFeature_Count)) {
      DN_CPUFeatureDecl feature_decl = g_dn_cpu_feature_decl[feature_index];
      bool              has_feature  = DN_CPUHasFeature(report, feature_decl.value);
      DN_FmtAppendTruncate(buf,
                           &buf_size,
                           sizeof(buf),
                           DN_Str8Lit("..."),
                           "    %.*s:%*s%s\n",
                           DN_Str8PrintFmt(feature_decl.label),
                           DN_Cast(int)(longest_feature_name - feature_decl.label.count),
                           "",
                           has_feature ? "available" : "not available");
    }
    #endif
  }

  if (buf_size)
    DN_LogDebugF("%.*s", DN_Cast(int)buf_size, buf);
}

DN_API void DN_Set(DN_Core *dn)
{
  g_dn_ = dn;
}

DN_API DN_Core *DN_Get()
{
  DN_Core *result = g_dn_;
  return result;
}

DN_API void DN_BeginFrame()
{
  #if DN_WITH_OS
  DN_AtomicSetValue64(&g_dn_->os.mem_allocs_frame, 0);
  #endif
}

DN_API bool DN_VerifyArgsF(DN_VerifyType type, bool expr, DN_CallSite call_site, DN_Str8 expr_str8, char const *fmt, ...)
{
  bool result = expr;
  if (result)
    return result;

  DN_TcScratch scratch = DN_TcScratchBeginArena(nullptr, 0);
  {
    DN_Str8Builder builder = DN_Str8BuilderFromArena(&scratch.arena);

    // NOTE: Log message prefix
    DN_Str8BuilderAppendF(&builder, "Verify [%.*s] failed%s", DN_Str8PrintFmt(expr_str8), fmt ? ". " : "");

    // NOTE: Log user message
    if (fmt) {
      va_list args;
      va_start(args, fmt);
      DN_Str8BuilderAppendFV(&builder, fmt, args);
      va_end(args);
    }

    // NOTE: Log stack trace
    if (type == DN_VerifyType_Nil && DN_PARANOIA_LEVEL) {
      DN_Str8 trace = DN_Str8FromStackTraceNowArena(&scratch.arena, 128 /*limit*/, 4 /*skip*/);
      DN_Str8BuilderAppendF(&builder, "\nTrace:\n  ");
      DN_Str8BuilderAppendRef(&builder, DN_Str8PadNewLinesArena(trace, DN_Str8Lit("  "), &scratch.arena));
    }

    DN_Str8         log            = DN_Str8FromStr8BuilderArena(&builder, &scratch.arena);
    DN_LogType      log_type       = type == DN_VerifyType_Nil ? DN_LogType_Error : DN_LogType_Warning;
    DN_LogTypeParam log_type_param = DN_LogTypeParamFromType(log_type);
    DN_LogPrintF(log_type_param, call_site, DN_LogFlags_Nil, "%.*s", DN_Str8PrintFmt(log));
  }
  DN_TcScratchEnd(&scratch);

  if (type == DN_VerifyType_Nil && DN_PARANOIA_LEVEL) {
    DN_DebugBreak;
  }
  return result;
}

DN_API bool DN_VerifyArgs(DN_VerifyType type, bool expr, DN_CallSite call_site, DN_Str8 expr_str8) {
  bool result = DN_VerifyArgsF(type, expr, call_site, expr_str8, /*fmt=*/ 0);
  return result;
}

DN_API bool DN_MemStartsWith(void const *lhs, DN_USize lhs_count, void const *rhs, DN_USize rhs_count)
{
  bool result = false;
  if (lhs_count >= rhs_count)
    result = DN_MemEqUnsafe(lhs, rhs, rhs_count);
  return result;
}

DN_API bool DN_MemEq(void const *lhs, DN_USize lhs_count, void const *rhs, DN_USize rhs_count)
{
  bool result = lhs_count == rhs_count && DN_Memcmp(lhs, rhs, rhs_count) == 0;
  return result;
}

DN_API bool DN_MemEqUnsafe(void const *lhs, void const *rhs, DN_USize count)
{
  bool result = DN_Memcmp(lhs, rhs, count) == 0;
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

  // NOTE: Query standard/extended numbers
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

  // NOTE: Extract function count
  int const STANDARD_FUNC_MAX_EAX = fn_0000_[0x0000].reg.eax;
  int const EXTENDED_FUNC_MAX_EAX = fn_8000_[0x0000].reg.eax;

  // NOTE: Enumerate all CPUID results for the known function counts
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

  // NOTE: Query CPU vendor
  {
    DN_Memcpy(result.vendor + 0, &fn_8000_[0x0000].reg.ebx, REGISTER_SIZE);
    DN_Memcpy(result.vendor + 4, &fn_8000_[0x0000].reg.edx, REGISTER_SIZE);
    DN_Memcpy(result.vendor + 8, &fn_8000_[0x0000].reg.ecx, REGISTER_SIZE);
  }

  // NOTE: Query CPU brand
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

  // NOTE: Query CPU features
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
      case DN_CPUFeature_RDTscP:             available = (fn_8000_[0x0001].reg.edx & (1 << 27)); break;
      case DN_CPUFeature_SHA:                available = (fn_0000_[0x0007].reg.ebx & (1 << 29)); break;
      case DN_CPUFeature_SSE:                available = (fn_0000_[0x0001].reg.edx & (1 << 25)); break;
      case DN_CPUFeature_SSE2:               available = (fn_0000_[0x0001].reg.edx & (1 << 26)); break;
      case DN_CPUFeature_SSE3:               available = (fn_0000_[0x0001].reg.ecx & (1 << 0)); break;
      case DN_CPUFeature_SSE41:              available = (fn_0000_[0x0001].reg.ecx & (1 << 19)); break;
      case DN_CPUFeature_SSE42:              available = (fn_0000_[0x0001].reg.ecx & (1 << 20)); break;
      case DN_CPUFeature_SSE4A:              available = (fn_8000_[0x0001].reg.ecx & (1 << 6)); break;
      case DN_CPUFeature_SSSE3:              available = (fn_0000_[0x0001].reg.ecx & (1 << 9)); break;
      case DN_CPUFeature_Tsc:                available = (fn_0000_[0x0001].reg.edx & (1 << 4)); break;
      case DN_CPUFeature_TscInvariant:       available = (fn_8000_[0x0007].reg.edx & (1 << 8)); break;
      case DN_CPUFeature_VAES:               available = (fn_0000_[0x0007].reg.ecx & (1 << 9)); break;
      case DN_CPUFeature_VPCMULQDQ:          available = (fn_0000_[0x0007].reg.ecx & (1 << 10)); break;
      case DN_CPUFeature_Count:              DN_AssertInvalidCodePath; break;
    }

    if (available)
      DN_CPUSetFeature(&result, DN_Cast(DN_CPUFeature) ext_index);
  }
#endif // DN_SUPPORTS_CPU_ID
  return result;
}

DN_API void DN_TicketMutexBegin(DN_TicketMutex *mutex)
{
  DN_UInt ticket = DN_AtomicAddU32(&mutex->ticket, 1);
  DN_TicketMutexBeginTicket(mutex, ticket);
}

DN_API void DN_TicketMutexEnd(DN_TicketMutex *mutex)
{
  DN_AtomicAddU32(&mutex->serving, 1);
}

DN_API DN_UInt DN_TicketMutexMakeTicket(DN_TicketMutex *mutex)
{
  DN_UInt result = DN_AtomicAddU32(&mutex->ticket, 1);
  return result;
}

DN_API void DN_TicketMutexBeginTicket(DN_TicketMutex const *mutex, DN_UInt ticket)
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

DN_API bool DN_TicketMutexCanLock(DN_TicketMutex const *mutex, DN_UInt ticket)
{
  bool result = (ticket == mutex->serving);
  return result;
}

#if defined(DN_COMPILER_MSVC) || defined(DN_COMPILER_CLANG_CL)
  #if !defined(DN_CRT_SECURE_NO_WARNINGS_PREVIOUSLY_DEFINED)
    #undef _CRT_SECURE_NO_WARNINGS
  #endif
#endif

// NOTE: DN_Bit
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
  bool result = DN_Cast(bool)((bits & bits_to_set) == bits_to_set);
  return result;
}

DN_API bool DN_BitIsAny(DN_USize bits, DN_USize bits_to_check)
{
  bool result = DN_Cast(bool)(bits & bits_to_check);
  return result;
}

DN_API bool DN_BitIsNotSet(DN_USize bits, DN_USize bits_to_check)
{
  auto result = !DN_BitIsSet(bits, bits_to_check);
  return result;
}

DN_API DN_I64 DN_SafeAddI64(DN_I64 a, DN_I64 b)
{
  DN_I64 result = a <= INT64_MAX - b ? (a + b) : INT64_MAX;
  return result;
}

DN_API DN_I64 DN_SafeMulI64(DN_I64 a, DN_I64 b)
{
  DN_I64 result = a <= INT64_MAX / b ? (a * b) : INT64_MAX;
  return result;
}

DN_API DN_U64 DN_SafeAddU64(DN_U64 a, DN_U64 b)
{
  DN_U64 result = a <= UINT64_MAX - b ? (a + b) : UINT64_MAX;
  return result;
}

DN_API DN_U64 DN_SafeSubU64(DN_U64 a, DN_U64 b)
{
  DN_U64 result = a >= b ? (a - b) : 0;
  return result;
}

DN_API DN_U64 DN_SafeMulU64(DN_U64 a, DN_U64 b)
{
  DN_U64 result = a <= UINT64_MAX / b ? (a * b) : UINT64_MAX;
  return result;
}

DN_API DN_U32 DN_SafeSubU32(DN_U32 a, DN_U32 b)
{
  DN_U32 result = a >= b ? (a - b) : 0;
  return result;
}

// NOTE: INT*_MAX literals will be promoted to the type of uintmax_t as uintmax_t is the highest
// possible rank (unsigned > signed).
DN_API int DN_SaturateCastUSizeToInt(DN_USize val)
{
  int result = DN_Cast(uintmax_t) val <= INT_MAX ? DN_Cast(int) val : INT_MAX;
  return result;
}

DN_API DN_I8 DN_SaturateCastUSizeToI8(DN_USize val)
{
  DN_I8 result = DN_Cast(uintmax_t) val <= INT8_MAX ? DN_Cast(DN_I8) val : INT8_MAX;
  return result;
}

DN_API DN_I16 DN_SaturateCastUSizeToI16(DN_USize val)
{
  DN_I16 result = DN_Cast(uintmax_t) val <= INT16_MAX ? DN_Cast(DN_I16) val : INT16_MAX;
  return result;
}

DN_API DN_I32 DN_SaturateCastUSizeToI32(DN_USize val)
{
  DN_I32 result = DN_Cast(uintmax_t) val <= INT32_MAX ? DN_Cast(DN_I32) val : INT32_MAX;
  return result;
}

DN_API DN_I64 DN_SaturateCastUSizeToI64(DN_USize val)
{
  DN_I64 result = DN_Cast(uintmax_t) val <= INT64_MAX ? DN_Cast(DN_I64) val : INT64_MAX;
  return result;
}

// NOTE: Both operands are unsigned and the lowest rank operand will be promoted to
// match the highest rank operand.
DN_API DN_U8 DN_SaturateCastUSizeToU8(DN_USize val)
{
  DN_U8 result = val <= UINT8_MAX ? DN_Cast(DN_U8) val : UINT8_MAX;
  return result;
}

DN_API DN_U16 DN_SaturateCastUSizeToU16(DN_USize val)
{
  DN_U16 result = val <= UINT16_MAX ? DN_Cast(DN_U16) val : UINT16_MAX;
  return result;
}

DN_API DN_U32 DN_SaturateCastUSizeToU32(DN_USize val)
{
  DN_U32 result = val <= UINT32_MAX ? DN_Cast(DN_U32) val : UINT32_MAX;
  return result;
}

DN_API DN_U64 DN_SaturateCastUSizeToU64(DN_USize val)
{
  DN_U64 result = DN_Cast(DN_U64) val <= UINT64_MAX ? DN_Cast(DN_U64) val : UINT64_MAX;
  return result;
}

// NOTE: DN_SaturateCastU64To*
DN_API int DN_SaturateCastU64ToInt(DN_U64 val)
{
  int result = val <= INT_MAX ? DN_Cast(int) val : INT_MAX;
  return result;
}

DN_API DN_I8 DN_SaturateCastU64ToI8(DN_U64 val)
{
  DN_I8 result = val <= INT8_MAX ? DN_Cast(DN_I8) val : INT8_MAX;
  return result;
}

DN_API DN_I16 DN_SaturateCastU64ToI16(DN_U64 val)
{
  DN_I16 result = val <= INT16_MAX ? DN_Cast(DN_I16) val : INT16_MAX;
  return result;
}

DN_API DN_I32 DN_SaturateCastU64ToI32(DN_U64 val)
{
  DN_I32 result = val <= INT32_MAX ? DN_Cast(DN_I32) val : INT32_MAX;
  return result;
}

DN_API DN_I64 DN_SaturateCastU64ToI64(DN_U64 val)
{
  DN_I64 result = val <= INT64_MAX ? DN_Cast(DN_I64) val : INT64_MAX;
  return result;
}

// NOTE: Both operands are unsigned and the lowest rank operand will be promoted to match the
// highest rank operand.
DN_API DN_UInt DN_SaturateCastU64ToUInt(DN_U64 val)
{
  DN_UInt result = val <= UINT8_MAX ? DN_Cast(DN_UInt) val : UINT_MAX;
  return result;
}

DN_API DN_U8 DN_SaturateCastU64ToU8(DN_U64 val)
{
  DN_U8 result = val <= UINT8_MAX ? DN_Cast(DN_U8) val : UINT8_MAX;
  return result;
}

DN_API DN_U16 DN_SaturateCastU64ToU16(DN_U64 val)
{
  DN_U16 result = val <= UINT16_MAX ? DN_Cast(DN_U16) val : UINT16_MAX;
  return result;
}

DN_API DN_U32 DN_SaturateCastU64ToU32(DN_U64 val)
{
  DN_U32 result = val <= UINT32_MAX ? DN_Cast(DN_U32) val : UINT32_MAX;
  return result;
}

// NOTE: Both operands are signed so the lowest rank operand will be promoted to match the highest
// rank operand.
DN_API int DN_SaturateCastISizeToInt(DN_ISize val)
{
  DN_Assert(val >= INT_MIN && val <= INT_MAX);
  int result = DN_Cast(int) DN_Clamp(val, INT_MIN, INT_MAX);
  return result;
}

DN_API DN_I8 DN_SaturateCastISizeToI8(DN_ISize val)
{
  DN_Assert(val >= INT8_MIN && val <= INT8_MAX);
  DN_I8 result = DN_Cast(DN_I8) DN_Clamp(val, INT8_MIN, INT8_MAX);
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

DN_API DN_I64 DN_SaturateCastISizeToI64(DN_ISize val)
{
  DN_Assert(DN_Cast(DN_I64) val >= INT64_MIN && DN_Cast(DN_I64) val <= INT64_MAX);
  DN_I64 result = DN_Cast(DN_I64) DN_Clamp(DN_Cast(DN_I64) val, INT64_MIN, INT64_MAX);
  return result;
}

// NOTE: If the value is a negative integer, we clamp to 0. Otherwise, we know that the value is
// >=0, we can upcast safely to bounds check against the maximum allowed value.
DN_API DN_UInt DN_SaturateCastISizeToUInt(DN_ISize val)
{
  DN_UInt result = 0;
  if (val >= DN_Cast(DN_ISize)0) {
    if (DN_Cast(uintmax_t) val <= UINT_MAX)
      result = DN_Cast(DN_UInt) val;
    else
      result = UINT_MAX;
  }
  return result;
}

DN_API DN_U8 DN_SaturateCastISizeToU8(DN_ISize val)
{
  DN_U8 result = 0;
  if (val >= DN_Cast(DN_ISize) 0) {
    if (DN_Cast(uintmax_t) val <= UINT8_MAX)
      result = DN_Cast(DN_U8) val;
    else
      result = UINT8_MAX;
  }
  return result;
}

DN_API DN_U16 DN_SaturateCastISizeToU16(DN_ISize val)
{
  DN_U16 result = 0;
  if (val >= DN_Cast(DN_ISize) 0) {
    if (DN_Cast(uintmax_t) val <= UINT16_MAX)
      result = DN_Cast(DN_U16) val;
    else
      result = UINT16_MAX;
  }
  return result;
}

DN_API DN_U32 DN_SaturateCastISizeToU32(DN_ISize val)
{
  DN_U32 result = 0;
  if (val >= DN_Cast(DN_ISize) 0) {
    if (DN_Cast(uintmax_t) val <= UINT32_MAX)
      result = DN_Cast(DN_U32) val;
    else
      result = UINT32_MAX;
  }
  return result;
}

DN_API DN_U64 DN_SaturateCastISizeToU64(DN_ISize val)
{
  DN_U64 result = 0;
  if (val >= DN_Cast(DN_ISize) 0) {
    if (DN_Cast(uintmax_t) val <= UINT64_MAX)
      result = DN_Cast(DN_U64) val;
    else
      result = UINT64_MAX;
  }
  return result;
}

// NOTE: Both operands are signed so the lowest rank operand will be promoted to match the highest
// rank operand.
DN_API DN_ISize DN_SaturateCastI64ToISize(DN_I64 val)
{
  DN_ISize result = DN_Cast(DN_I64) DN_Clamp(val, DN_ISIZE_MIN, DN_ISIZE_MAX);
  return result;
}

DN_API DN_I8 DN_SaturateCastI64ToI8(DN_I64 val)
{
  DN_I8 result = DN_Cast(DN_I8) DN_Clamp(val, INT8_MIN, INT8_MAX);
  return result;
}

DN_API DN_I16 DN_SaturateCastI64ToI16(DN_I64 val)
{
  DN_I16 result = DN_Cast(DN_I16) DN_Clamp(val, INT16_MIN, INT16_MAX);
  return result;
}

DN_API DN_I32 DN_SaturateCastI64ToI32(DN_I64 val)
{
  DN_I32 result = DN_Cast(DN_I32) DN_Clamp(val, INT32_MIN, INT32_MAX);
  return result;
}

DN_API DN_UInt DN_SaturateCastI64ToUInt(DN_I64 val)
{
  DN_UInt result = 0;
  if (val >= DN_Cast(DN_I64) 0) {
    if (DN_Cast(uintmax_t) val <= UINT_MAX)
      result = DN_Cast(DN_UInt) val;
    else
      result = UINT_MAX;
  }
  return result;
}

DN_API DN_USize DN_SaturateCastI64ToUSize(DN_I64 val)
{
  DN_USize result = 0;
  if (val >= DN_Cast(DN_I64) 0) {
    if (DN_Cast(uintmax_t) val <= DN_USIZE_MAX)
      result = DN_Cast(DN_USize) val;
    else
      result = DN_USIZE_MAX;
  }
  return result;
}

DN_API DN_U8 DN_SaturateCastI64ToU8(DN_I64 val)
{
  DN_U8 result = 0;
  if (val >= DN_Cast(DN_I64) 0) {
    if (DN_Cast(uintmax_t) val <= UINT8_MAX)
      result = DN_Cast(DN_U8) val;
    else
      result = UINT8_MAX;
  }
  return result;
}

DN_API DN_U16 DN_SaturateCastI64ToU16(DN_I64 val)
{
  DN_U16 result = 0;
  if (val >= DN_Cast(DN_I64) 0) {
    if (DN_Cast(uintmax_t) val <= UINT16_MAX)
      result = DN_Cast(DN_U16) val;
    else
      result = UINT16_MAX;
  }
  return result;
}

DN_API DN_U32 DN_SaturateCastI64ToU32(DN_I64 val)
{
  DN_U32 result = 0;
  if (val >= DN_Cast(DN_I64) 0) {
    if (DN_Cast(uintmax_t) val <= UINT32_MAX)
      result = DN_Cast(DN_U32) val;
    else
      result = UINT32_MAX;
  }
  return result;
}

DN_API DN_U64 DN_SaturateCastI64ToU64(DN_I64 val)
{
  DN_U64 result = 0;
  if (val >= DN_Cast(DN_I64) 0) {
    if (DN_Cast(uintmax_t) val <= UINT64_MAX)
      result = DN_Cast(DN_U64) val;
    else
      result = UINT64_MAX;
  }
  return result;
}

DN_API DN_I8 DN_SaturateCastIntToI8(int val)
{
  DN_I8 result = DN_Cast(DN_I8) DN_Clamp(val, INT8_MIN, INT8_MAX);
  return result;
}

DN_API DN_I16 DN_SaturateCastIntToI16(int val)
{
  DN_I16 result = DN_Cast(DN_I16) DN_Clamp(val, INT16_MIN, INT16_MAX);
  return result;
}

DN_API DN_U8 DN_SaturateCastIntToU8(int val)
{
  DN_U8 result = 0;
  if (val >= DN_Cast(DN_ISize) 0) {
    if (DN_Cast(uintmax_t) val <= UINT8_MAX)
      result = DN_Cast(DN_U8) val;
    else
      result = UINT8_MAX;
  }
  return result;
}

DN_API DN_U16 DN_SaturateCastIntToU16(int val)
{
  DN_U16 result = 0;
  if (val >= DN_Cast(DN_ISize) 0) {
    if (DN_Cast(uintmax_t) val <= UINT16_MAX)
      result = DN_Cast(DN_U16) val;
    else
      result = UINT16_MAX;
  }
  return result;
}

DN_API DN_U32 DN_SaturateCastIntToU32(int val)
{
  DN_StaticAssert(sizeof(val) <= sizeof(DN_U32) && "Sanity check to allow simplifying of casting");
  DN_U32 result = 0;
  if (val >= 0)
    result = DN_Cast(DN_U32) val;
  return result;
}

DN_API DN_U64 DN_SaturateCastIntToU64(int val)
{
  DN_StaticAssert(sizeof(val) <= sizeof(DN_U64) && "Sanity check to allow simplifying of casting");
  DN_U64 result = 0;
  if (val >= 0)
    result = DN_Cast(DN_U64) val;
  return result;
}

// NOTE: DN_Asan
DN_StaticAssert(DN_IsPowerOfTwoAligned(DN_ASAN_POISON_GUARD_SIZE, DN_ASAN_POISON_ALIGNMENT) &&
                "ASAN poison guard size must be a power-of-two and aligned to ASAN's alignment" "requirement (8 bytes)");
DN_API void DN_ASanPoisonMemoryRegion(void const volatile *ptr, DN_USize size)
{
  if (!ptr || !size)
    return;

#if DN_HAS_FEATURE(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
  DN_AssertF(DN_IsPowerOfTwoAligned(ptr, 8),
             "Poisoning requires the pointer to be aligned on an 8 byte boundary");

  __asan_poison_memory_region(ptr, size);
  if (DN_ASAN_VET_POISON) {
    DN_AssertAlways(__asan_address_is_poisoned(ptr));
    DN_AssertAlways(__asan_address_is_poisoned((char *)ptr + (size - 1)));
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
    DN_AssertAlways(__asan_region_is_poisoned((void *)ptr, size) == 0);
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

static DN_MemBlock *DN_MemBlockFromHeap_(DN_U64 reserve, DN_U64 commit, bool track_alloc, bool alloc_can_leak, DN_Heap heap)
{
  DN_MemBlock *result = nullptr;
  switch (heap.type) {
    case DN_HeapType_Nil:
      break;

    case DN_HeapType_Basic: {
      DN_AssertF(reserve > DN_ARENA_HEADER_SIZE, "%I64u > %I64u", reserve, DN_ARENA_HEADER_SIZE);
      result = DN_Cast(DN_MemBlock *) heap.basic_alloc(reserve);
      if (!result)
        return result;

      result->used    = DN_ARENA_HEADER_SIZE;
      result->commit  = reserve;
      result->reserve = reserve;
    } break;

    case DN_HeapType_Virtual: {
      DN_AssertF(heap.virtual_page_size,
                 "Page size must be set to a non-zero, power of two value. Virtual memory "
                 "functions are usually initialised by values obtained during initialisation, has "
                 "DN_Init() been called yet?");
      DN_Assert(DN_IsPowerOfTwo(heap.virtual_page_size));

      DN_USize const page_size    = heap.virtual_page_size;
      DN_U64         real_reserve = reserve ? reserve : DN_ARENA_RESERVE_SIZE;
      DN_U64         real_commit  = commit ? commit   : DN_ARENA_COMMIT_SIZE;
      real_reserve                = DN_AlignUpPowerOfTwo(real_reserve, page_size);
      real_commit                 = DN_Min(DN_AlignUpPowerOfTwo(real_commit, page_size), real_reserve);
      DN_AssertF(DN_ARENA_HEADER_SIZE < real_commit && real_commit <= real_reserve, "%zu < %I64u <= %I64u", DN_ARENA_HEADER_SIZE, real_commit, real_reserve);

      DN_MemCommit mem_commit = real_reserve == real_commit ? DN_MemCommit_Yes : DN_MemCommit_No;
      result                  = DN_Cast(DN_MemBlock *) heap.virtual_reserve(real_reserve, mem_commit, DN_MemPage_ReadWrite);
      if (!result)
        return result;

      if (mem_commit == DN_MemCommit_No && !heap.virtual_commit(result, real_commit, DN_MemPage_ReadWrite)) {
        heap.virtual_release(result, real_reserve);
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

static bool DN_ArenaHasPoison_(DN_MemFlags flags)
{
  DN_MSVC_WARNING_PUSH
  DN_MSVC_WARNING_DISABLE(6237) // warning C6237: (<zero> && <expression>) is always zero.  <expression> is never evaluated and might have side effects.
  bool result = DN_ASAN_POISON && DN_BitIsNotSet(flags, DN_MemFlags_NoPoison);
  DN_MSVC_WARNING_POP
  return result;
}

static DN_MemBlock *DN_MemBlockFromHeapFlags_(DN_U64 reserve, DN_U64 commit, DN_MemFlags flags, DN_Heap heap)
{
  bool         track_alloc    = (flags & DN_MemFlags_NoAllocTrack) == 0;
  bool         alloc_can_leak = flags & DN_MemFlags_AllocCanLeak;
  DN_MemBlock *result         = DN_MemBlockFromHeap_(reserve, commit, track_alloc, alloc_can_leak, heap);
  if (result && DN_ArenaHasPoison_(flags)) {
    char *poison         = DN_Cast(char *)result + result->used;
    DN_USize poison_size = result->commit - result->used;
    DN_ASanPoisonMemoryRegion(poison, poison_size);
  }
  return result;
}

static void DN_MemListOnNewBlock_(DN_MemList *mem, DN_MemBlock const *block)
{
  DN_Assert(mem);
  if (block) {
    mem->stats.info.used    += block->used;
    mem->stats.info.commit  += block->commit;
    mem->stats.info.reserve += block->reserve;
    mem->stats.info.blocks  += 1;

    mem->stats.hwm.used    = DN_Max(mem->stats.hwm.used,    mem->stats.info.used);
    mem->stats.hwm.commit  = DN_Max(mem->stats.hwm.commit,  mem->stats.info.commit);
    mem->stats.hwm.reserve = DN_Max(mem->stats.hwm.reserve, mem->stats.info.reserve);
    mem->stats.hwm.blocks  = DN_Max(mem->stats.hwm.blocks,  mem->stats.info.blocks);
  }
}

DN_API DN_MemStats DN_MemStatsSum(DN_MemStats lhs, DN_MemStats rhs)
{
  DN_MemStats array[] = {lhs, rhs};
  DN_MemStats result  = DN_MemStatsSumArray(array, DN_ArrayCountU(array));
  return result;
}

DN_API DN_MemStats DN_MemStatsSumArray(DN_MemStats const *array, DN_USize count)
{
  DN_MemStats result = {};
  for (DN_ForItSize(it, DN_MemStats const, array, count)) {
    DN_MemStats stats    = *it.data;
    result.info.used    += stats.info.used;
    result.info.commit  += stats.info.commit;
    result.info.reserve += stats.info.reserve;
    result.info.blocks  += stats.info.blocks;

    result.hwm.used      = DN_Max(result.hwm.used, result.info.used);
    result.hwm.commit    = DN_Max(result.hwm.commit, result.info.commit);
    result.hwm.reserve   = DN_Max(result.hwm.reserve, result.info.reserve);
    result.hwm.blocks    = DN_Max(result.hwm.blocks, result.info.blocks);
  }
  return result;
}

DN_API DN_Heap DN_HeapInitBasic(DN_HeapBasicAllocFunc *basic_alloc, DN_HeapBasicDeallocFunc *basic_dealloc)
{
  DN_Heap result       = {};
  result.type          = DN_HeapType_Basic;
  result.basic_alloc   = basic_alloc;
  result.basic_dealloc = basic_dealloc;
  return result;
}

DN_API DN_Heap DN_HeapInitVirtual(DN_U32 page_size, DN_HeapVirtualReserveFunc *virtual_reserve, DN_HeapVirtualCommitFunc *virtual_commit, DN_HeapVirtualReleaseFunc *virtual_release)
{
  DN_Heap result           = {};
  result.type              = DN_HeapType_Virtual;
  result.virtual_page_size = page_size;
  result.virtual_reserve   = virtual_reserve;
  result.virtual_commit    = virtual_commit;
  result.virtual_release   = virtual_release;
  return result;
}

DN_API void* DN_HeapAlloc(DN_Heap *heap, DN_USize reserve, DN_USize commit)
{
  void *result = nullptr;
  switch (heap->type) {
    case DN_HeapType_Nil:
      break;

    case DN_HeapType_Basic: result = heap->basic_alloc(reserve); break;
    case DN_HeapType_Virtual: {
      DN_AssertF(heap->virtual_page_size,
                 "Page size must be set to a non-zero, power of two value. Virtual memory "
                 "functions are usually initialised by values obtained during initialisation, has "
                 "DN_Init() been called yet?");
      DN_Assert(DN_IsPowerOfTwo(heap->virtual_page_size));

      DN_USize const page_size    = heap->virtual_page_size;
      DN_U64         real_reserve = reserve ? reserve : DN_ARENA_RESERVE_SIZE;
      DN_U64         real_commit  = commit ? commit   : DN_ARENA_COMMIT_SIZE;
      real_reserve                = DN_AlignUpPowerOfTwo(real_reserve, page_size);
      real_commit                 = DN_Min(DN_AlignUpPowerOfTwo(real_commit, page_size), real_reserve);
      DN_AssertF(DN_ARENA_HEADER_SIZE < real_commit && real_commit <= real_reserve, "%zu < %I64u <= %I64u", DN_ARENA_HEADER_SIZE, real_commit, real_reserve);

      DN_MemCommit mem_commit = real_reserve == real_commit ? DN_MemCommit_Yes : DN_MemCommit_No;
      result                  = DN_Cast(DN_MemBlock *) heap->virtual_reserve(real_reserve, mem_commit, DN_MemPage_ReadWrite);
      if (!result)
        return result;

      if (mem_commit == DN_MemCommit_No && !heap->virtual_commit(result, real_commit, DN_MemPage_ReadWrite)) {
        heap->virtual_release(result, real_reserve);
        return result;
      }
    } break;
  }

  if (result) {
    heap->bytes_alloc += reserve;
    heap->bytes_alloc_total += reserve;
  }
  return result;
}

DN_API void DN_HeapDealloc(DN_Heap *heap, void *ptr, DN_USize size)
{
  heap->bytes_alloc -= size;
  heap->bytes_freed += size;
  if (heap->type == DN_HeapType_Basic)
    heap->basic_dealloc(ptr);
  else
    heap->virtual_release(ptr, size);
}

DN_API DN_MemList DN_MemListFromBuffer(void *buffer, DN_USize size, DN_MemFlags flags)
{
  DN_Assert(buffer);
  DN_AssertF(DN_ARENA_HEADER_SIZE < size, "Buffer (%zu bytes) too small, need atleast %zu bytes to store arena metadata", size, DN_ARENA_HEADER_SIZE);
  DN_AssertF(DN_IsPowerOfTwo(size), "Buffer (%zu bytes) must be a power-of-two", size);

  // NOTE: Init block
  DN_MemBlock *block = DN_Cast(DN_MemBlock *) buffer;
  block->commit        = size;
  block->reserve       = size;
  block->used          = DN_ARENA_HEADER_SIZE;
  if (block && DN_ArenaHasPoison_(flags))
    DN_ASanPoisonMemoryRegion(DN_Cast(char *) block + DN_ARENA_HEADER_SIZE, block->commit - DN_ARENA_HEADER_SIZE);

  DN_MemList result = {};
  result.flags      = flags | DN_MemFlags_NoGrow | DN_MemFlags_NoAllocTrack | DN_MemFlags_AllocCanLeak | DN_MemFlags_UserBuffer;
  result.curr       = block;
  DN_MemListOnNewBlock_(&result, result.curr);
  return result;
}

DN_API DN_MemList DN_MemListFromHeap(DN_U64 reserve, DN_U64 commit, DN_MemFlags flags, DN_Heap heap)
{
  DN_MemList result  = {};
  result.heap        = heap;
  result.flags      |= flags | DN_MemFlags_Heap;
  result.curr        = DN_MemBlockFromHeapFlags_(reserve, commit, flags, heap);
  DN_MemListOnNewBlock_(&result, result.curr);
  return result;
}

static void DN_MemBlockDeinit_(DN_MemList *mem, DN_MemBlock *block)
{
  DN_USize release_size = block->reserve;
  if (DN_BitIsNotSet(mem->flags, DN_MemFlags_NoAllocTrack))
    DN_LeakTrackDealloc(&g_dn_->leak, block);

  if (DN_ArenaHasPoison_(mem->flags))
    DN_ASanUnpoisonMemoryRegion(block, block->commit);

  if (mem->flags & DN_MemFlags_Heap)
    DN_HeapDealloc(&mem->heap, block, release_size);
}

DN_API void DN_MemListDeinit(DN_MemList *mem)
{
  bool mem_allocated_from_itself = DN_MemListOwnsPtr(mem, mem);
  for (DN_MemBlock *block = mem ? mem->curr : nullptr; block;) {
    DN_MemBlock *block_to_free = block;
    block                      = block->prev;
    DN_MemBlockDeinit_(mem, block_to_free);
  }
  if (mem && !mem_allocated_from_itself)
    *mem = {};
}

DN_API bool DN_MemListCommitTo(DN_MemList *mem, DN_U64 pos)
{
  if (!mem || !mem->curr)
    return false;

  // NOTE: Early out if the position to commit to is already committed
  DN_MemBlock *curr = mem->curr;
  if (pos <= curr->commit)
    return true;

  // NOTE: Sanity check position is within the bounds of the memory block
  DN_U64 real_pos = pos;
  if (pos > curr->reserve) {
    DN_Assert(pos <= curr->reserve);
    real_pos = curr->reserve;
  }

  // NOTE: Do the commit
  DN_Assert(mem->heap.virtual_page_size);
  DN_USize end_commit  = DN_AlignUpPowerOfTwo(real_pos, mem->heap.virtual_page_size);
  DN_USize commit_size = end_commit - curr->commit;
  char    *commit_ptr  = DN_Cast(char *) curr + curr->commit;
  if (!mem->heap.virtual_commit(commit_ptr, commit_size, DN_MemPage_ReadWrite))
    return false;

  if (DN_ArenaHasPoison_(mem->flags))
    DN_ASanPoisonMemoryRegion(commit_ptr, commit_size);

  curr->commit = end_commit;
  return true;
}

DN_API bool DN_MemListCommit(DN_MemList *mem, DN_U64 size)
{
  if (!mem || !mem->curr)
    return false;
  DN_U64 pos    = DN_Min(mem->curr->reserve, mem->curr->commit + size);
  bool   result = DN_MemListCommitTo(mem, pos);
  return result;
}

DN_API bool DN_MemListGrow(DN_MemList *mem, DN_U64 reserve, DN_U64 commit)
{
  if (mem->flags & (DN_MemFlags_NoGrow | DN_MemFlags_UserBuffer))
    return false;

  bool result = false;
  DN_MemBlock *new_block = DN_MemBlockFromHeapFlags_(reserve, commit, mem->flags, mem->heap);
  if (new_block) {
    result                 = true;
    new_block->prev        = mem->curr;
    mem->curr              = new_block;
    new_block->reserve_sum = new_block->prev->reserve_sum + new_block->prev->reserve;
    DN_MemListOnNewBlock_(mem, mem->curr);
  }
  return result;
}

DN_API void *DN_MemListAlloc(DN_MemList *mem, DN_U64 size, DN_U8 align, DN_ZMem z_mem)
{
  if (!mem)
    return nullptr;

  if (!mem->curr) {
    mem->curr = DN_MemBlockFromHeapFlags_(DN_ARENA_RESERVE_SIZE, DN_ARENA_COMMIT_SIZE, mem->flags, mem->heap);
    DN_MemListOnNewBlock_(mem, mem->curr);
  }

  if (!mem->curr)
    return nullptr;

  try_alloc_again:
  DN_MemBlock *curr       = mem->curr;
  bool         poison     = DN_ArenaHasPoison_(mem->flags);
  DN_U8        real_align = poison ? DN_Max(align, DN_ASAN_POISON_ALIGNMENT) : align;
  DN_U64       offset_pos = DN_AlignUpPowerOfTwo(curr->used, real_align) + (poison ? DN_ASAN_POISON_GUARD_SIZE : 0);
  DN_U64       end_pos    = offset_pos + size;
  DN_U64       alloc_size = end_pos - curr->used;

  if (end_pos > curr->reserve) {
    if (mem->flags & (DN_MemFlags_NoGrow | DN_MemFlags_UserBuffer))
      return nullptr;
    DN_USize new_reserve = DN_Max(DN_ARENA_HEADER_SIZE + alloc_size, DN_ARENA_RESERVE_SIZE);
    DN_USize new_commit  = DN_Max(DN_ARENA_HEADER_SIZE + alloc_size, DN_ARENA_COMMIT_SIZE);
    if (!DN_MemListGrow(mem, new_reserve, new_commit))
      return nullptr;
    goto try_alloc_again;
  }

  if (end_pos > curr->commit) {
    DN_Assert(mem->heap.virtual_page_size);
    DN_Assert(mem->heap.type == DN_HeapType_Virtual);
    DN_Assert((mem->flags & DN_MemFlags_UserBuffer) == 0);
    DN_USize end_commit  = DN_AlignUpPowerOfTwo(end_pos, mem->heap.virtual_page_size);
    DN_USize commit_size = end_commit - curr->commit;
    char    *commit_ptr  = DN_Cast(char *) curr + curr->commit;
    if (!mem->heap.virtual_commit(commit_ptr, commit_size, DN_MemPage_ReadWrite))
      return nullptr;
    if (poison && DN_BitIsNotSet(mem->flags, DN_MemFlags_SimAlloc))
      DN_ASanPoisonMemoryRegion(commit_ptr, commit_size);
    curr->commit              = end_commit;
    mem->stats.info.commit += commit_size;
    mem->stats.hwm.commit   = DN_Max(mem->stats.hwm.commit, mem->stats.info.commit);
  }

  void *result          = DN_Cast(char *) curr + offset_pos;
  curr->used           += alloc_size;
  mem->stats.info.used += alloc_size;
  mem->stats.hwm.used   = DN_Max(mem->stats.hwm.used, mem->stats.info.used);

  if (poison && DN_BitIsNotSet(mem->flags, DN_MemFlags_SimAlloc))
    DN_ASanUnpoisonMemoryRegion(result, size);

  if (z_mem == DN_ZMem_Yes && DN_BitIsNotSet(mem->flags, DN_MemFlags_SimAlloc))
    DN_Memset(result, 0, size);

  DN_Assert(mem->stats.hwm.used    >= mem->stats.info.used);
  DN_Assert(mem->stats.hwm.commit  >= mem->stats.info.commit);
  DN_Assert(mem->stats.hwm.reserve >= mem->stats.info.reserve);
  DN_Assert(mem->stats.hwm.blocks  >= mem->stats.info.blocks);
  return result;
}

DN_API void *DN_MemListAllocContiguous(DN_MemList *mem, DN_U64 size, DN_U8 align, DN_ZMem z_mem)
{
  DN_MemFlags prev_flags = mem->flags;
  mem->flags |= (DN_MemFlags_NoGrow | DN_MemFlags_NoPoison);
  void *memory = DN_MemListAlloc(mem, size, align, z_mem);
  mem->flags = prev_flags;
  return memory;
}

DN_API void *DN_MemListCopy(DN_MemList *mem, void const *data, DN_U64 size, DN_U8 align)
{
  if (!mem || !data || size == 0)
    return nullptr;
  void *result = DN_MemListAlloc(mem, size, align, DN_ZMem_No);
  if (result)
    DN_Memcpy(result, data, size);
  return result;
}

DN_API void DN_MemListPopTo(DN_MemList *mem, DN_U64 init_used)
{
  if (!mem || !mem->curr)
    return;

  // NOTE: Free any memory blocks until we get back to the starting block
  DN_U64       used = DN_Max(DN_ARENA_HEADER_SIZE, init_used);
  DN_MemBlock *curr = mem->curr;
  while (curr->reserve_sum >= used) {
    DN_MemBlock *block_to_free = curr;
    mem->stats.info.used    -= block_to_free->used;
    mem->stats.info.commit  -= block_to_free->commit;
    mem->stats.info.reserve -= block_to_free->reserve;
    mem->stats.info.blocks  -= 1;
    if (mem->flags & DN_MemFlags_UserBuffer)
      break;
    curr = curr->prev;
    DN_MemBlockDeinit_(mem, block_to_free);
  }

  // NOTE: Revert the memory block we returned to
  DN_U64 old_used = curr->used;
  mem->curr       = curr;

  // NOTE: Undo the used amount on the cumulative used count in the stats. This reverts the used
  // number to how much memory has been used, not including this block.
  mem->stats.info.used -= old_used;

  // NOTE: Calculate the new correct used amount for this block after reversion and then apply it
  curr->used            = used - curr->reserve_sum;
  mem->stats.info.used += curr->used;

  // NOTE: Scrub memory that we used previously in the block but no longer after reverting
  DN_MSVC_WARNING_PUSH
  DN_MSVC_WARNING_DISABLE(4127) // conditional expression is constant
  if (DN_SCRUB_UNINIT_MEM_BYTE) {
    if (old_used > curr->used) {
      char *discarded = (char *)curr + curr->used;
      DN_USize scrub_size = old_used - curr->used;

      // NOTE: If we allocated memory unaligned then the pointer given to the user was aligned up
      // and unpoisoned. If the user snapped a memory list position before that allocation then
      // attempts to revert it, scrubbing from the memory position (which is before alignment was
      // applied!) will cause this code to accidentally scrub the our poison guard bytes. So we
      // unpoison the region unconditionally to ensure that is cleaned up before scrubbing. Since
      // scrubbing is a debug feature and, you have it turned on _with_ ASAN then we let that
      // performance penalty slide.
      if (DN_ArenaHasPoison_(mem->flags))
        DN_ASanUnpoisonMemoryRegion(discarded, scrub_size);

      DN_Memset(discarded, DN_SCRUB_UNINIT_MEM_BYTE, scrub_size);
    }
  }
  DN_MSVC_WARNING_POP

  // NOTE: ASAN Poison
  if (DN_ArenaHasPoison_(mem->flags)) {
    char    *poison_ptr  = (char *)curr + DN_AlignUpPowerOfTwo(curr->used, DN_ASAN_POISON_ALIGNMENT);
    DN_USize poison_size = ((char *)curr + curr->commit) - poison_ptr;
    DN_ASanPoisonMemoryRegion(poison_ptr, poison_size);
  }
}

DN_API void DN_MemListPop(DN_MemList *mem, DN_U64 amount)
{
  DN_MemBlock *curr     = mem->curr;
  DN_USize     used_sum = curr->reserve_sum + curr->used;
  amount                = DN_Min(amount, used_sum);
  DN_USize pop_to       = used_sum - amount;
  DN_MemListPopTo(mem, pop_to);
}

DN_API DN_U64 DN_MemListPos(DN_MemList const *mem)
{
  DN_U64 result = (mem && mem->curr) ? mem->curr->reserve_sum + mem->curr->used : 0;
  return result;
}

DN_API void DN_MemListClear(DN_MemList *mem)
{
  DN_MemListPopTo(mem, 0);
}

DN_API bool DN_MemListOwnsPtr(DN_MemList const *mem, void const *ptr)
{
  bool      result = false;
  DN_UPtr uint_ptr = DN_Cast(DN_UPtr) ptr;
  for (DN_MemBlock const *block = mem ? mem->curr : nullptr; !result && block; block = block->prev) {
    DN_UPtr begin = DN_Cast(DN_UPtr) block + DN_ARENA_HEADER_SIZE;
    DN_UPtr end   = begin + block->reserve;
    result        = uint_ptr >= begin && uint_ptr <= end;
  }
  return result;
}

DN_API DN_Str8x64 DN_MemListInfoStr8x64(DN_MemListInfo info)
{
  DN_Str8x64 result  = {};
  DN_Str8x32 used    = DN_Str8x32FromByteCountU64Auto(info.used);
  DN_Str8x32 commit  = DN_Str8x32FromByteCountU64Auto(info.commit);
  DN_Str8x32 reserve = DN_Str8x32FromByteCountU64Auto(info.reserve);
  // NOTE: Blocks, Used, Commit, Reserve
  result             = DN_Str8x64FromFmt("B=%u U=%.*s C=%.*s R=%.*s", DN_Cast(DN_U32)info.blocks, DN_Str8PrintFmt(used), DN_Str8PrintFmt(commit), DN_Str8PrintFmt(reserve));
  return result;
}

DN_API DN_MemListTemp DN_MemListTempBegin(DN_MemList *mem)
{
  DN_MemListTemp result = {};
  if (mem) {
    result.mem      = mem;
    result.used_sum = mem->curr ? mem->curr->reserve_sum + mem->curr->used : 0;
  }
  return result;
};

DN_API void DN_MemListTempEnd(DN_MemListTemp temp)
{
  DN_MemListPopTo(temp.mem, temp.used_sum);
};

DN_Str8 const DN_MEM_LIST_UAF_TRACING_DISABLED_MORE_INFO_STR8_ = DN_Str8Lit(
  "\n\nSet `DN_MemFlags_TempMemUAFTrace` on the affected arenas or "
  "`#define DN_ARENA_TEMP_MEM_UAF_TRACE_ON_BY_DEFAULT 1` for more information"
);

#if defined(DN_ARENA_TEMP_MEM_UAF_GUARD)
static bool DN_MemListUAFTracingEnabled_(DN_MemList *mem)
{
  bool result = DN_ARENA_TEMP_MEM_UAF_TRACE_ON_BY_DEFAULT;
  if (!result)
    result = mem->flags & DN_MemFlags_TempMemUAFTrace;
  if (mem->flags & DN_MemFlags_TempMemUAFTraceDisable)
    result = false;
  return result;
}
#endif

static void DN_ArenaUAFCheck_(DN_Arena *arena, DN_ArenaUAFCheckReportType_ type)
{
  (void)arena;
  (void)type;
  #if DN_ARENA_TEMP_MEM_UAF_GUARD
  DN_MemList *mem = arena->mem;
  if (!arena || !mem)
    return;

  if ((arena->uaf_guard_temp_mem || mem->uaf_guard_active_temp_mem) && !arena->uaf_guard_is_being_checked) {
    // NOTE: The following functions below allocate memory which might trigger an additional UAF
    // check which would cause infinite recursion so we set a flag here to prevent that.
    arena->uaf_guard_is_being_checked = true;
    if (mem->uaf_guard_active_id != arena->uaf_guard_id) {
      // NOTE: We use the MemList on the arena directly to bypass any potential recursive UAF (if the
      // current arena is triggering the UAF check then it's already violating so we use the
      // underlying primitive to allocate memory).
      DN_Allocator allocator = DN_AllocatorFromMemList(mem);

      // NOTE: MSVC does not recognise %'u which is a STB extension which causes a lot of incorrect
      // format arguments warnings that we mute here.
      DN_MSVC_WARNING_PUSH
      DN_MSVC_WARNING_DISABLE(6271) // Extra argument passed to 'DN_Str8FmtArena'
      DN_MSVC_WARNING_DISABLE(6067) // _Param_(10) in call to 'DN_LogPrint' must be the address of a string. Actual type: 'int'.
      DN_MSVC_WARNING_DISABLE(6273) // Non-integer passed as _Param_(11) when an integer is required in call to 'DN_LogPrint' Actual type: 'char *'.
      DN_Str8 error_msg = {};
      if (type == DN_ArenaUAFCheckReportType_AllocViolation) {
        error_msg = DN_Str8FmtAllocator(allocator,
                                            "\n\nArena use-after-free (UAF) detected in temporary memory usage! This allocation (trace "
                                            "shown above) is attempting to allocate memory inside the active temporary region (id: %'u) "
                                            "but belongs to a different region (id: %'u). This means when the active temporary region is "
                                            "released, this allocation will be released and scrubbed causing a potential UAF.\n\nEnsure "
                                            "that scratch memory is deconflicting correctly, scratch and or temporary memory regions have "
                                            "matching begin and end pairs and only the arena view with the active temporary memory region "
                                            "is being allocated from.",
                                            mem->uaf_guard_active_id,
                                            arena->uaf_guard_id);
      } else {
        error_msg = DN_Str8Lit("The active temporary memory region recorded on the arena is "
                               "different from the current temporary memory region recorded on "
                               "the memory list allocator. This means that a temporary region "
                               "began but was not ended after the region was completed. Temporary "
                               "memory regions are enforced in a first-in-last-out manner (FILO) "
                               "to ensure the developer's intent of what the temporary region "
                               "spans is logically consistent and always strictly ends and begins "
                               "within a known lifetime.");
      }

      DN_Str8 prefix = DN_Str8LineBreakAllocator(error_msg, 100, DN_Str8Lit("\n"), DN_Str8LineBreakMode_AtWord, allocator);
      if (DN_MemListUAFTracingEnabled_(mem)) {
        DN_Str8 curr_stack_trace = DN_Str8Lit("<Unknown: Arena is not using temporary memory>");
        if (arena->uaf_guard_temp_mem)
          curr_stack_trace = DN_Str8FromStackTraceAllocator(allocator, &arena->uaf_guard_temp_mem->trace, 1);
        curr_stack_trace = DN_Str8PadNewLinesAllocator(curr_stack_trace, DN_Str8Lit("  "), allocator);

        DN_Str8 active_stack_trace = DN_Str8Lit("<Unknown: MemList does not have an active temporary memory>");
        if (mem->uaf_guard_active_temp_mem)
          active_stack_trace = DN_Str8FromStackTraceAllocator(allocator, &mem->uaf_guard_active_temp_mem->trace, 1);
        active_stack_trace = DN_Str8PadNewLinesAllocator(active_stack_trace, DN_Str8Lit("  "), allocator);

        DN_AssertF(mem->uaf_guard_active_id == arena->uaf_guard_id,
                   "%.*s\n\nThe originating temporary memory region (id: %'u) was created at:"
                   "\n\n  %.*s\n\nThe active temporary memory region (id: %'u) was created at:\n\n  %.*s\n",
                   DN_Str8PrintFmt(prefix),
                   arena->uaf_guard_id,
                   DN_Str8PrintFmt(curr_stack_trace),
                   mem->uaf_guard_active_id,
                   DN_Str8PrintFmt(active_stack_trace));
      } else {
        DN_Str8 suffix = DN_Str8LineBreakAllocator(DN_MEM_LIST_UAF_TRACING_DISABLED_MORE_INFO_STR8_, 100, DN_Str8Lit("\n"), DN_Str8LineBreakMode_AtWord, allocator);
        DN_AssertF(mem->uaf_guard_active_id == arena->uaf_guard_id, "%.*s%.*s", DN_Str8PrintFmt(prefix), DN_Str8PrintFmt(suffix));
      }
      DN_MSVC_WARNING_POP
    }
    arena->uaf_guard_is_being_checked = false;
  }
  #endif
}

DN_API DN_Arena DN_ArenaFromMemList(DN_MemList *mem)
{
  DN_Arena result = {};
  result.mem      = mem;
  return result;
}

DN_API DN_Arena DN_ArenaFromHeap(DN_U64 reserve, DN_U64 commit, DN_MemFlags flags, DN_Heap heap)
{
  DN_MemList mem     = DN_MemListFromHeap(reserve, commit, flags, heap);
  DN_Arena   result  = {};
  result.flags      |= DN_ArenaFlags_OwnsMemList;
  result.mem         = DN_MemListNewCopy(&mem, DN_MemList, &mem);
  return result;
}

DN_API DN_Arena DN_ArenaTempBeginFromMemList(DN_MemList* mem)
{
  DN_Arena       result   = DN_ArenaFromMemList(mem);
  DN_MemListTemp temp_mem = DN_MemListTempBegin(mem);

#if DN_ARENA_TEMP_MEM_UAF_GUARD
  // NOTE: Below we use the `MemList` and bypass the UAF checks which could cause infinite recursion
  // depending on how, say, stack-traces are implemented.
  if (DN_MemListUAFTracingEnabled_(mem))
    temp_mem.trace = DN_StackTraceFromAllocator(DN_AllocatorFromMemList(mem), 256);

  // NOTE: Create persistent temp mem and set it on the mem list
  result.uaf_guard_temp_mem      = DN_MemListNewCopy(mem, DN_MemListTemp, &temp_mem);
  result.uaf_guard_prev_temp_mem = mem->uaf_guard_active_temp_mem;
  mem->uaf_guard_active_temp_mem = result.uaf_guard_temp_mem;

  // NOTE: Update IDs
  result.uaf_guard_id            = ++mem->uaf_guard_next_id;
  result.uaf_guard_prev_id       = mem->uaf_guard_active_id;
  mem->uaf_guard_active_id       = result.uaf_guard_id;
#else
  result.temp_mem = temp_mem;
#endif
  return result;
}


DN_API DN_Arena DN_ArenaTempBeginFromArena(DN_Arena *arena)
{
  DN_Arena result = DN_ArenaTempBeginFromMemList(arena->mem);
  return result;
}

DN_API void DN_ArenaTempEnd(DN_Arena *arena, DN_ArenaReset reset)
{
  // NOTE: Do the UAF check
#if DN_ARENA_TEMP_MEM_UAF_GUARD
  DN_AssertF(arena->uaf_guard_temp_mem, "Arena was not created with temp memory");
  DN_ArenaUAFCheck_(arena, DN_ArenaUAFCheckReportType_TempEndOutOfOrder);
#else
  DN_AssertF(arena->temp_mem.mem, "Arena was not created with temp memory");
#endif

  // NOTE: Reset the arena
  if (reset == DN_ArenaReset_Yes) {
#if DN_ARENA_TEMP_MEM_UAF_GUARD
    DN_MemListTempEnd(*arena->uaf_guard_temp_mem);
#else
    DN_MemListTempEnd(arena->temp_mem);
#endif
  }

  // NOTE: Pop the UAF guard off (note the UAF was allocated on the temp arena itself, so when we
  // reset the arena, the old UAF guard has been deallocated).
#if DN_ARENA_TEMP_MEM_UAF_GUARD
  DN_MemList *mem                = arena->mem;
  mem->uaf_guard_active_id       = arena->uaf_guard_prev_id;
  mem->uaf_guard_active_temp_mem = arena->uaf_guard_prev_temp_mem;

  arena->uaf_guard_prev_temp_mem = nullptr;
  arena->uaf_guard_prev_id       = 0;
  arena->uaf_guard_temp_mem      = nullptr;
#endif
}

DN_API void *DN_ArenaAlloc(DN_Arena *arena, DN_U64 size, DN_U8 align, DN_ZMem z_mem)
{
  DN_ArenaUAFCheck_(arena, DN_ArenaUAFCheckReportType_AllocViolation);
  void *result = DN_MemListAlloc(arena->mem, size, align, z_mem);
  return result;
}

DN_API void *DN_ArenaAllocContiguous(DN_Arena *arena, DN_U64 size, DN_U8 align, DN_ZMem z_mem)
{
  DN_ArenaUAFCheck_(arena, DN_ArenaUAFCheckReportType_AllocViolation);
  void *result = DN_MemListAllocContiguous(arena->mem, size, align, z_mem);
  return result;
}

DN_API void *DN_ArenaCopy(DN_Arena *arena, void const *data, DN_U64 size, DN_U8 align)
{
  DN_ArenaUAFCheck_(arena, DN_ArenaUAFCheckReportType_AllocViolation);
  void *result = DN_MemListCopy(arena->mem, data, size, align);
  return result;
}

DN_API void DN_ArenaDeinit(DN_Arena *arena)
{
  if (arena->flags & DN_ArenaFlags_OwnsMemList)
    DN_MemListDeinit(arena->mem);
}

DN_API bool DN_ArenaOwnsPtr(DN_Arena const *arena, void *ptr)
{
  bool result = DN_MemListOwnsPtr(arena->mem, ptr);
  return result;
}

DN_API DN_Pool DN_PoolFromArena(DN_Arena *arena, DN_U8 align)
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
  DN_USize const DN_USizeo_slot_offset = 5; // __lzcnt64(32) e.g. DN_PoolSlotSize_32B
  DN_USize       slot_index          = 0;
  if (required_size > 32) {
    // NOTE: Round up if not PoT as the low bits are set.
    DN_USize dist_to_next_msb = DN_CountLeadingZerosUSize(required_size) + 1;
    dist_to_next_msb -= DN_Cast(DN_USize)(!DN_IsPowerOfTwo(required_size));

    DN_USize const register_size = sizeof(DN_USize) * 8;
    DN_AssertF(register_size >= (dist_to_next_msb - DN_USizeo_slot_offset), "lhs=%zu, rhs=%zu", register_size, (dist_to_next_msb - DN_USizeo_slot_offset));
    slot_index = register_size - dist_to_next_msb - DN_USizeo_slot_offset;
  }

  if (slot_index >= DN_PoolSlotSize_Count) {
    DN_AssertF(slot_index < DN_PoolSlotSize_Count, "Chunk pool does not support the requested allocation size");
    return result;
  }

  DN_USize slot_size_in_bytes = 1ULL << (slot_index + DN_USizeo_slot_offset);
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

  DN_Assert(DN_MemListOwnsPtr(pool->arena->mem, ptr));

  char const *one_byte_behind_ptr    = DN_Cast(char *) ptr - 1;
  DN_USize    offset_to_original_ptr = 0;
  DN_Memcpy(&offset_to_original_ptr, one_byte_behind_ptr, 1);
  DN_Assert(offset_to_original_ptr <= sizeof(DN_PoolSlot) + pool->align);

  char           *original_ptr = DN_Cast(char *) ptr - offset_to_original_ptr;
  DN_PoolSlot    *slot         = DN_Cast(DN_PoolSlot *) original_ptr;
  DN_PoolSlotSize slot_index   = DN_Cast(DN_PoolSlotSize)(DN_Cast(uintptr_t) slot->next);
  DN_Assert(slot_index < DN_PoolSlotSize_Count);

  // NOTE: Scrub memory before returning to the pool
  DN_MSVC_WARNING_PUSH
  DN_MSVC_WARNING_DISABLE(4127) // conditional expression is constant
  if (DN_SCRUB_UNINIT_MEM_BYTE) {
    DN_USize slot_size_in_bytes = 1ULL << (slot_index + 5);
    DN_USize data_offset        = (char *)slot->data - (char *)slot;
    DN_Memset(slot->data, DN_SCRUB_UNINIT_MEM_BYTE, slot_size_in_bytes - data_offset);
  }
  DN_MSVC_WARNING_POP

  slot->next              = pool->slots[slot_index];
  pool->slots[slot_index] = slot;
}

static void DN_ErrSinkCheck_(DN_ErrSink const *err)
{
  DN_Assert(err->arena->mem);
  if (err->stack_size == 0)
    return;

  DN_ErrSinkNode const *node = err->stack + (err->stack_size - 1);
  DN_Assert(node->mode >= DN_ErrSinkMode_Nil && node->mode <= DN_ErrSinkMode_ExitOnError);
  DN_Assert(node->msg_sentinel);

  // NOTE: Walk the list ensuring we eventually terminate at the sentinel (e.g. we have a
  // well formed doubly-linked-list terminated by a sentinel, or otherwise we will hit the
  // walk limit or dereference a null pointer and assert)
  DN_USize WALK_LIMIT = 99'999;
  DN_USize walk       = 0;
  for (DN_ErrSinkMsg *it = node->msg_sentinel->next; it != node->msg_sentinel; it = it->next, walk++) {
    DN_AssertF(it, "Encountered null pointer which should not happen in a sentinel DLL");
    DN_Assert(walk < WALK_LIMIT);
  }
}

DN_API DN_ErrSink* DN_ErrSinkBegin_(DN_ErrSink *err, DN_ErrSinkMode mode, DN_CallSite call_site)
{
  // NOTE: OOM error
  if (err->stack_size == DN_ArrayCountU(err->stack)) {
    DN_Str8Builder builder = DN_Str8BuilderFromArena(err->arena);
    for (DN_ForItSize(it, DN_ErrSinkNode, err->stack, err->stack_size))
      DN_Str8BuilderAppendF(&builder, "  [%04zu] %.*s:%u %.*s\n", it.index, DN_Str8PrintFmt(it.data->call_site.file), it.data->call_site.line, DN_Str8PrintFmt(it.data->call_site.function));
    DN_Str8 msg = DN_Str8FromStr8BuilderArena(&builder, err->arena);
    DN_AssertF(err->stack_size < DN_ArrayCountU(err->stack), "Error sink has run out of error scopes, potential leak. Scopes were\n%.*s", DN_Str8PrintFmt(msg));
  }

  // NOTE: Allocate the node
  DN_ErrSinkNode *node = err->stack + err->stack_size++;
  node->arena_pos      = DN_MemListPos(err->arena->mem);
  node->mode           = mode;
  node->call_site      = call_site;
  DN_SentinelDoublyLLInitArena(node->msg_sentinel, DN_ErrSinkMsg, err->arena);

  // NOTE: Handle allocation error
  if (!node || !node->msg_sentinel) {
    DN_MemListPopTo(err->arena->mem, node->arena_pos);
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
    entry->msg           = DN_Str8FromStr8Arena(it->msg, arena);
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
  DN_MemListPopTo(err->arena->mem, node->arena_pos);
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
                             it->msg.count ? " " : "",
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
  DN_MemListPopTo(err->arena->mem, node->arena_pos);

  result = DN_Str8FromStr8BuilderArena(&builder, arena);
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
  err->stack_size--;

  bool result = false;
  if (node->msg_sentinel != node->msg_sentinel->next) {
    result = true;
    // NOTE: Build the error string
    DN_Str8Builder builder = DN_Str8BuilderFromArena(err->arena);
    {
      if (err_msg.count) {
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
    DN_Str8 log = DN_Str8FromStr8BuilderArena(&builder, err->arena);
    DN_LogPrintF(DN_LogTypeParamFromType(DN_LogType_Error), call_site, DN_LogFlags_Nil, "%.*s", DN_Str8PrintFmt(log));

    if (node->mode == DN_ErrSinkMode_DebugBreakOnErrorLog)
      DN_DebugBreak;

    // NOTE: Deallocate the error node's memory and pop it from the stack
    DN_MemListPopTo(err->arena->mem, node->arena_pos);
  }
  return result;
}

DN_API bool DN_ErrSinkEndLogErrorFV_(DN_ErrSink *err, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_Str8 log    = DN_Str8FmtVArena(err->arena, fmt, args);
  bool    result = DN_ErrSinkEndLogError_(err, call_site, log);
  return result;
}

DN_API bool DN_ErrSinkEndLogErrorF_(DN_ErrSink *err, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8    log    = DN_Str8FmtVArena(err->arena, fmt, args);
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
  DN_Assert(msg);
  msg->msg        = DN_Str8FmtVArena(err->arena, fmt, args);
  msg->error_code = error_code;
  msg->call_site  = call_site;
  DN_SentinelDoublyLLPrepend(node->msg_sentinel, msg);
  if (node->mode == DN_ErrSinkMode_ExitOnError)
    DN_ErrSinkEndExitIfErrorF_(err, msg->call_site, error_code, "Fatal error %u", error_code);
}

DN_API void DN_ErrSinkAppendF_(DN_ErrSink *err, DN_U32 error_code, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_ErrSinkAppendFV_(err, error_code, call_site, fmt, args);
  va_end(args);
}

DN_THREAD_LOCAL DN_TcCore *g_dn_thread_context;

DN_API void DN_TcInit(DN_TcCore *tc, DN_U64 thread_id, DN_Arena *main_arena, DN_Arena *temp_arenas, DN_USize temp_arenas_count, DN_Arena *err_sink_arena)
{
  tc->thread_id      = thread_id;
  tc->main_arena     = main_arena;
  tc->main_pool      = DN_PoolFromArena(tc->main_arena, 0);
  tc->err_sink.arena = err_sink_arena;
  DN_Assert(temp_arenas_count < DN_ArrayCountU(tc->temp_arenas));
  for (DN_ForIndexU(index, temp_arenas_count))
    tc->temp_arenas[tc->temp_arenas_count++] = temp_arenas + index;
}

DN_API DN_TcInitArgs DN_TcInitArgsDefault()
{
  DN_TcInitArgs result    = {};
  result.main_reserve     = DN_Kilobytes(64);
  result.main_commit      = DN_Kilobytes(4);
  result.temp_reserve     = DN_Kilobytes(64);
  result.temp_commit      = DN_Kilobytes(4);
  result.temp_count       = 2;
  result.err_sink_reserve = DN_Kilobytes(64);
  result.err_sink_commit  = DN_Kilobytes(4);
  return result;
}

DN_API void DN_TcInitFromHeap(DN_TcCore *tc, DN_U64 thread_id, DN_TcInitArgs args, DN_Heap heap)
{
  DN_Assert(args.temp_count <= DN_ArrayCountU(tc->temp_arenas));
  DN_MemList main_mem_stack = DN_MemListFromHeap(args.main_reserve, args.main_commit, DN_MemFlags_AllocCanLeak | DN_MemFlags_NoAllocTrack, heap);
  DN_MemList *main_mem      = DN_MemListNewCopy(&main_mem_stack, DN_MemList, &main_mem_stack);
  DN_Arena   *main_arena    = DN_MemListNewZ(main_mem, DN_Arena);
  *main_arena               = DN_ArenaFromMemList(main_mem);

  DN_Arena *temp_arenas = DN_MemListNewArrayZ(main_mem, DN_Arena, args.temp_count);
  for (DN_ForIndexU(index, args.temp_count)) {
    DN_MemList temp_mem_stack = DN_MemListFromHeap(args.temp_reserve, args.temp_commit, DN_MemFlags_AllocCanLeak | DN_MemFlags_NoAllocTrack, heap);
    DN_MemList *temp_mem      = DN_MemListNewCopy(main_mem, DN_MemList, &temp_mem_stack);
    temp_arenas[index]        = DN_ArenaFromMemList(temp_mem);
  }

  DN_MemList err_sink_mem_stack = DN_MemListFromHeap(args.err_sink_reserve, args.err_sink_commit, DN_MemFlags_AllocCanLeak | DN_MemFlags_NoAllocTrack, heap);
  DN_MemList *err_sink_mem      = DN_MemListNewCopy(main_mem, DN_MemList, &err_sink_mem_stack);
  DN_Arena *err_sink_arena      = DN_MemListNewZ(main_mem, DN_Arena);
  *err_sink_arena               = DN_ArenaFromMemList(err_sink_mem);

  DN_TcInit(tc, thread_id, main_arena, temp_arenas, args.temp_count, err_sink_arena);
}

DN_API void DN_TcDeinit(DN_TcCore *tc, DN_TcDeinitArenas deinit_arenas)
{
  // NOTE: That we deallocate the main memory last as TC might be allocated in that arena.
  if (deinit_arenas == DN_TcDeinitArenas_Yes) {
    for (DN_ForIndexU(index, tc->temp_arenas_count))
      DN_MemListDeinit(tc->temp_arenas[index]->mem);
    DN_MemListDeinit(tc->err_sink.arena->mem);
    DN_MemListDeinit(tc->main_arena->mem);
  }
}

DN_API void DN_TcEquip(DN_TcCore *tc)
{
  g_dn_thread_context = tc;
}

DN_API DN_TcCore *DN_TcGet()
{
  DN_AssertRaw(g_dn_thread_context &&
               "This thread's thread context has not been equipped yet. Ensure that DN_TcInit(...) "
               "has been called to create a thread context and call DN_TcEquip(...) in the current "
               "thread to make it retrievable via this function");
  return g_dn_thread_context;
}

DN_API DN_Arena *DN_TcMainArena()
{
  DN_TcCore *tc     = DN_TcGet();
  DN_Arena  *result = tc->main_arena;
  return result;
}

DN_API DN_Pool *DN_TcMainPool()
{
  DN_TcCore *tc     = DN_TcGet();
  DN_Pool   *result = &tc->main_pool;
  return result;
}

DN_API DN_Arena DN_TcTempArenaAllocator(DN_Allocator *conflicts, DN_USize count)
{
  DN_MemList *conflict_mem_lists[8];
  DN_USize    conflict_mem_lists_count = 0;
  for (DN_ForItSize(it, DN_Allocator, conflicts, count)) {
    DN_Allocator *allocator = it.data;
    if (!allocator->context)
      continue;

    DN_MemList *mem_list = nullptr;
    switch (allocator->type) {
      case DN_AllocatorType_MemList: mem_list = DN_Cast(DN_MemList *)allocator->context; break;

      case DN_AllocatorType_Arena: {
        DN_Arena *arena = DN_Cast(DN_Arena *) allocator->context;
        mem_list        = arena->mem;
      } break;

      case DN_AllocatorType_Pool: {
        DN_Pool *pool = DN_Cast(DN_Pool *) allocator->context;
        mem_list      = pool->arena ? pool->arena->mem : nullptr;
      } break;
    }

    if (!mem_list)
      continue;

    void *added = DN_LArrayAppend(conflict_mem_lists, &conflict_mem_lists_count, mem_list);
    DN_Assert(added);
  }

  DN_TcCore  *tc     = DN_TcGet();
  DN_Arena    result = {};
  for (DN_ForItSize(it, DN_Arena *, tc->temp_arenas, tc->temp_arenas_count)) {
    bool        is_usable = true;
    DN_Arena   *rhs_arena = *it.data;
    DN_MemList *rhs_mem   = rhs_arena->mem;
    for (DN_ForItSize(conflict_it, DN_MemList*, conflict_mem_lists, conflict_mem_lists_count)) {
      DN_MemList *lhs_mem = *conflict_it.data;
      if (lhs_mem == rhs_mem) {
        is_usable = false;
        break;
      }
    }

    if (is_usable) {
      result = DN_ArenaTempBeginFromMemList(rhs_mem);
      break;
    }
  }

  DN_AssertF(result.mem, "All temp arenas are being used, there are none left to return to the caller");
  return result;
}

DN_API DN_Arena DN_TcTempArenaFromArena(DN_Arena **conflicts, DN_USize count)
{
  DN_TcCore  *tc           = DN_TcGet();
  DN_Arena    result       = {};
  for (DN_ForItSize(it, DN_Arena *, tc->temp_arenas, tc->temp_arenas_count)) {
    bool        is_usable = true;
    DN_Arena   *rhs_arena = *it.data;
    DN_MemList *rhs_mem   = rhs_arena->mem;
    for (DN_ForItSize(conflict_it, DN_Arena *, conflicts, count)) {
      DN_Arena   *lhs_arena = *conflict_it.data;
      DN_MemList *lhs_mem   = lhs_arena->mem;
      if (lhs_mem == rhs_mem) {
        is_usable = false;
        break;
      }
    }

    if (is_usable) {
      result = DN_ArenaTempBeginFromMemList(rhs_mem);
      break;
    }
  }

  DN_AssertF(result.mem, "All temp arenas are being used, there are none left to return to the caller");
  return result;
}

#if defined(__cplusplus)
DN_TcScratchCpp::DN_TcScratchCpp(DN_Arena **conflicts, DN_USize count)
{
  this->data = DN_TcScratchBeginArena(conflicts, count);
}

DN_TcScratchCpp::~DN_TcScratchCpp()
{
  DN_TcScratchEnd(&this->data);
}
#endif

DN_API DN_TcScratch DN_TcScratchBeginAllocator(DN_Allocator *conflicts, DN_USize count)
{
  DN_TcScratch result = {};
  result.arena        = DN_TcTempArenaAllocator(conflicts, count);
  return result;
}

DN_API DN_TcScratch DN_TcScratchBeginArena(DN_Arena **conflicts, DN_USize count)
{
  DN_TcScratch result = {};
  result.arena        = DN_TcTempArenaFromArena(conflicts, count);
  return result;
}

DN_API void DN_TcScratchEnd(DN_TcScratch *scratch)
{
  DN_Assert(scratch->destructed == false);
  DN_ArenaTempEnd(&scratch->arena, DN_ArenaReset_Yes);
  *scratch            = {};
  scratch->destructed = true;
}

DN_API void DN_TcSetFrameArena(DN_Arena *arena)
{
  DN_TcCore *tc   = DN_TcGet();
  tc->frame_arena = arena;
}

DN_API DN_Arena *DN_TcFrameArena()
{
  DN_TcCore *tc     = DN_TcGet();
  DN_Arena  *result = tc->frame_arena;
  return result;
}

DN_API DN_ErrSink *DN_TcErrSink()
{
  DN_TcCore  *tc     = DN_TcGet();
  DN_ErrSink *result = &tc->err_sink;
  return result;
}

DN_API void *DN_PoolCopy(DN_Pool *pool, void const *data, DN_U64 size, DN_U8 align)
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

DN_API DN_U64FromResult DN_U64FromStr8Delimiters(DN_Str8 string, DN_Str8 const *delimiters, DN_USize delimiters_count)
{
  // NOTE: Argument check
  DN_U64FromResult result = {};
  if (string.count == 0) {
    result.success = true;
    return result;
  }

  // NOTE: Sanitize input/output
  DN_Str8 trim_string = DN_Str8TrimWhitespaceAround(string);
  if (trim_string.count == 0) {
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
  for (DN_USize index = start_index; index < trim_string.count; index++) {

    // NOTE: Check for presence of the delimiter, we skip the first character as a U64 string
    // prefixed with a delimiter is not considered valid.
    if (index) {
      DN_USize max_delimiter_match = 0;
      for (DN_ForItSize(it, DN_Str8 const, delimiters, delimiters_count)) {
        DN_Str8 delimiter   = *it.data;
        DN_Str8 check_slice = DN_Str8Subset(trim_string, index, delimiter.count);
        if (DN_Str8EqSensitive(check_slice, delimiter))
          max_delimiter_match = DN_Max(max_delimiter_match, delimiter.count); // Take length, there might be multiple so we keep going
      }

      if (max_delimiter_match) {
        index += max_delimiter_match - 1;
        continue;
      }
    }

    char ch = trim_string.data[index];
    if (!DN_CharIsDigit(ch))
      return result;

    result.value   = DN_SafeMulU64(result.value, 10);
    DN_U64 digit = ch - '0';
    result.value   = DN_SafeAddU64(result.value, digit);
  }

  result.success = true;
  return result;
}

DN_API DN_U64FromResult DN_U64FromStr8Delimiter(DN_Str8 string, DN_Str8 delimiter)
{
  DN_U64FromResult result = DN_U64FromStr8Delimiters(string, &delimiter, 1);
  return result;
}

DN_API DN_U64FromResult DN_U64FromStr8(DN_Str8 string)
{
  DN_U64FromResult result = DN_U64FromStr8Delimiters(string, nullptr, 0);
  return result;
}

DN_API DN_U64FromResult DN_U64FromPtrDelimiter(void const *data, DN_USize size, DN_Str8 delimiter)
{
  DN_Str8          str8   = DN_Str8FromPtr((char *)data, size);
  DN_U64FromResult result = DN_U64FromStr8Delimiter(str8, delimiter);
  return result;
}


DN_API DN_U64FromResult DN_U64FromPtr(void const *data, DN_USize size)
{
  DN_Str8          str8   = DN_Str8FromPtr((char *)data, size);
  DN_U64FromResult result = DN_U64FromStr8Delimiters(str8, nullptr, 0);
  return result;
}

DN_API DN_U64 DN_U64FromPtrUnsafeDelimiter(void const *data, DN_USize size, DN_Str8 delimiter)
{
  DN_U64FromResult from   = DN_U64FromPtrDelimiter(data, size, delimiter);
  DN_U64           result = from.value;
  DN_VerifyWarning(from.success);
  return result;
}


DN_API DN_U64 DN_U64FromPtrUnsafe(void const *data, DN_USize size)
{
  DN_Str8 str8            = DN_Str8FromPtr(data, size);
  DN_U64FromResult from   = DN_U64FromStr8Delimiters(str8, nullptr, 0);
  DN_U64           result = from.value;
  DN_VerifyWarning(from.success);
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
  DN_U64FromResult result = DN_U64FromHexPtr(hex.data, hex.count);
  return result;
}

DN_API DN_U64 DN_U64FromHexStr8Unsafe(DN_Str8 hex)
{
  DN_U64 result = DN_U64FromHexPtrUnsafe(hex.data, hex.count);
  return result;
}

DN_API DN_U64 DN_U64FromU8x32HiBEUnsafe(DN_U8x32 const *val)
{
  DN_U64 result_be = 0; // Last 8 bytes of 32-byte slot (big-endian)
  DN_Memcpy(&result_be, val->data + sizeof(val->data) - sizeof(result_be), sizeof(result_be));
  DN_U64 result = DN_ByteSwap64(result_be);
  return result;
}

DN_API DN_U64FromResult DN_U64FromU8x32HiBE(DN_U8x32 const *val)
{
  DN_U64FromResult result = {};
  if (val) {
    // NOTE: Check that the high bits are not set
    DN_U8x32 zero_mask     = {};
    bool     high_bits_set = DN_Memcmp(val->data, zero_mask.data, sizeof(zero_mask.data) - sizeof(result)) != 0;
    result.success         = !high_bits_set;
    result.value           = DN_U64FromU8x32HiBEUnsafe(val);
  }
  return result;
}

DN_API DN_USize DN_USizeFromU8x32HiBEUnsafe(DN_U8x32 const *val)
{
  DN_USize result_be = 0;
  DN_Memcpy(&result_be, val->data + sizeof(val->data) - sizeof(result_be), sizeof(result_be));
  DN_USize result = DN_ByteSwapUSize(result_be);
  return result;
}

DN_API DN_USizeFromResult DN_USizeFromU8x32HiBE(DN_U8x32 const *val)
{
  DN_USizeFromResult result = {};
  if (val) {
    // NOTE: Check that the high bits are not set
    DN_U8x32 mask = {};
    DN_Memset(mask.data, 1, sizeof(mask.data) - sizeof(result));
    bool high_bits_set = DN_Memcmp(val->data, mask.data, 24) != 0;
    result.success     = !high_bits_set;
    result.value       = DN_USizeFromU8x32HiBEUnsafe(val);
  }
  return result;
}

static DN_U32FromResult DN_U32FromU64FromResult_(DN_U64FromResult u64)
{
  DN_U32FromResult result = {};
  result.value            = DN_Cast(DN_U32)DN_Min(u64.value, UINT32_MAX);
  result.success          = u64.value <= UINT32_MAX;
  return result;
}

DN_API DN_U32FromResult DN_U32FromHexStr8(DN_Str8 hex)
{
  DN_U64FromResult u64    = DN_U64FromHexPtr(hex.data, hex.count);
  DN_U32FromResult result = DN_U32FromU64FromResult_(u64);
  return result;
}

DN_API DN_U32FromResult DN_U32FromStr8Delimiters(DN_Str8 string, DN_Str8 const *delimiters, DN_USize delimiters_count)
{
  DN_U64FromResult u64    = DN_U64FromStr8Delimiters(string, delimiters, delimiters_count);
  DN_U32FromResult result = DN_U32FromU64FromResult_(u64);
  return result;
}

DN_API DN_U32FromResult DN_U32FromStr8Delimiter(DN_Str8 string, DN_Str8 delimiter)
{
  DN_U32FromResult result = DN_U32FromStr8Delimiters(string, &delimiter, 1);
  return result;
}

DN_API DN_U32FromResult DN_U32FromStr8(DN_Str8 string)
{
  DN_U64FromResult u64    = DN_U64FromStr8(string);
  DN_U32FromResult result = DN_U32FromU64FromResult_(u64);
  return result;
}

DN_API DN_U32FromResult DN_U32FromPtr(void const *data, DN_USize size)
{
  DN_U32FromResult result = DN_U32FromStr8(DN_Str8FromPtr(data, size));
  return result;
}

DN_API DN_I64FromResult DN_I64FromStr8Delimiters(DN_Str8 string, DN_Str8 const *delimiters, DN_USize delimiters_count)
{
  // NOTE: Argument check
  DN_I64FromResult result = {};
  if (string.count == 0) {
    result.success = true;
    return result;
  }

  // NOTE: Sanitize input/output
  DN_Str8 trim_string = DN_Str8TrimWhitespaceAround(string);
  if (trim_string.count == 0) {
    result.success = true;
    return result;
  }

  // NOTE: Handle negation
  bool     negative    = false;
  DN_USize start_index = 0;
  if (!DN_CharIsDigit(trim_string.data[0])) {
    negative = (trim_string.data[start_index] == '-');
    if (!negative && trim_string.data[0] != '+')
      return result;
    start_index++;
  }

  // NOTE: Convert the string number to the binary number
  for (DN_USize index = start_index; index < trim_string.count; index++) {

    // NOTE: Check for presence of the delimiter, we skip the first character as a U64 string
    // prefixed with a delimiter is not considered valid.
    if (index) {
      DN_USize max_delimiter_match = 0;
      for (DN_ForItSize(it, DN_Str8 const, delimiters, delimiters_count)) {
        DN_Str8 delimiter   = *it.data;
        DN_Str8 check_slice = DN_Str8Subset(trim_string, index, delimiter.count);
        if (DN_Str8EqSensitive(check_slice, delimiter))
          max_delimiter_match = DN_Max(max_delimiter_match, delimiter.count); // Take length, there might be multiple so we keep going
      }

      if (max_delimiter_match) {
        index += max_delimiter_match - 1;
        continue;
      }
    }

    char ch = trim_string.data[index];
    if (!DN_CharIsDigit(ch))
      return result;

    result.value = DN_SafeMulU64(result.value, 10);
    DN_U64 digit = ch - '0';
    result.value = DN_SafeAddU64(result.value, digit);
  }

  if (negative)
    result.value *= -1;

  result.success = true;
  return result;
}

DN_API DN_I64FromResult DN_I64FromStr8Delimiter(DN_Str8 string, DN_Str8 delimiter)
{
  DN_I64FromResult result = DN_I64FromStr8Delimiters(string, &delimiter, 1);
  return result;
}

DN_API DN_I64FromResult DN_I64FromStr8(DN_Str8 string)
{
  DN_I64FromResult result = DN_I64FromStr8Delimiters(string, nullptr, 0);
  return result;
}

DN_API DN_I64FromResult DN_I64FromPtr(void const *data, DN_USize size)
{
  DN_Str8          str8   = DN_Str8FromPtr((char *)data, size);
  DN_I64FromResult result = DN_I64FromStr8Delimiters(str8, nullptr, 0);
  return result;
}

DN_API DN_I64 DN_I64FromPtrUnsafe(void const *data, DN_USize size)
{
  DN_I64FromResult from   = DN_I64FromPtr(data, size);
  DN_I64           result = from.value;
  DN_Assert(from.success);
  return result;
}

DN_API bool DN_U8x32Eq(DN_U8x32 const *lhs, DN_U8x32 const *rhs)
{
  bool result = DN_MemEqUnsafe(lhs->data, rhs->data, sizeof(lhs->data));
  return result;
}

DN_API DN_U8x32 DN_U8x32FromBytesLeftPadZ(DN_U8 const *ptr, DN_USize size)
{
  DN_U8x32 result = {};
  DN_Assert(size <= sizeof(result.data));
  DN_Memcpy(result.data + sizeof(result.data) - size, ptr, size);
  return result;
}

DN_API DN_U8x32 DN_U8x32FromHexUnsafe(DN_Str8 hex_32b)
{
  DN_U8x32 result = {};
  hex_32b         = DN_Str8TrimHexPrefix(hex_32b);
  DN_Assert(hex_32b.count <= sizeof(result.data) * 2);
  DN_PtrBytesFromPtrHex(hex_32b.data, hex_32b.count, result.data, sizeof(result.data));
  return result;
}

DN_API DN_U8x32FromResult DN_U8x32FromHex(DN_Str8 hex_32b)
{
  DN_U8x32FromResult result        = {};
  DN_USize           bytes_written = DN_PtrBytesFromPtrHex(hex_32b.data, hex_32b.count, result.value.data, sizeof(result.value.data));
  if (bytes_written == sizeof(result.value.data))
    result.success = true;
  return result;
}

DN_API DN_U8x32FromResult DN_U8x32FromDecimalStr8(DN_Str8 decimal)
{
  DN_U8x32FromResult result = {};
  result.success            = true;
  for (DN_USize i = 0; i < decimal.count; i++) {
    DN_U8 digit = decimal.data[i];
    if (!DN_CharIsDigit(digit)) {
      result.success = false;
      break;
    }

    DN_U8 digit_val = digit - '0';

    // NOTE: Goal is to do => (result = result * 10 + digit_val)
    // Multiply current result by 10
    DN_U16 carry = 0;
    for (int j = 31; j >= 0; j--) {
      DN_U16 prod          = DN_Cast(DN_U16)result.value.data[j] * 10 + carry;
      result.value.data[j] = DN_Cast(DN_U8)(prod & 0xFF);
      carry                = prod >> 8;
    }

    // Add the digit
    carry = digit_val;
    for (int j = 31; j >= 0 && carry > 0; j--) {
      DN_U16 sum           = DN_Cast(DN_U16)result.value.data[j] + carry;
      result.value.data[j] = DN_Cast(DN_U8)(sum & 0xFF);
      carry                = sum >> 8;
    }
  }

  return result;
}

DN_API DN_Allocator DN_AllocatorFromMemList(DN_MemList *mem)
{
  DN_Allocator result = {};
  result.type         = DN_AllocatorType_MemList;
  result.context      = mem;
  return result;
}

DN_API DN_Allocator DN_AllocatorFromArena(DN_Arena *arena)
{
  DN_Allocator result = {};
  result.type         = DN_AllocatorType_Arena;
  result.context      = arena;
  return result;
}

DN_API DN_Allocator DN_AllocatorFromPool(DN_Pool *pool)
{
  DN_Allocator result = {};
  result.type         = DN_AllocatorType_Pool;
  result.context      = pool;
  return result;
}

DN_API void *DN_AllocatorAlloc(DN_Allocator allocator, DN_USize size, DN_U8 align, DN_ZMem z_mem)
{
  void *result = nullptr;
  if (allocator.context) {
    switch (allocator.type) {
      case DN_AllocatorType_Arena:   result = DN_ArenaAlloc  (DN_Cast(DN_Arena *) allocator.context,   size + 1, align, z_mem); break;
      case DN_AllocatorType_Pool:    result = DN_PoolAlloc   (DN_Cast(DN_Pool *) allocator.context,    size + 1);               break;
      case DN_AllocatorType_MemList: result = DN_MemListAlloc(DN_Cast(DN_MemList *) allocator.context, size + 1, align, z_mem); break;
    }
  }
  return result;
}

DN_API DN_FmtAppendResult DN_FmtVAppend(char *buf, DN_USize *buf_size, DN_USize buf_max, char const *fmt, va_list args)
{
  DN_FmtAppendResult result         = {};
  DN_USize           starting_size  = *buf_size;
  result.size_req                   = DN_Vsnprintf(buf + *buf_size, DN_Cast(int)(buf_max - *buf_size), fmt, args);
  *buf_size                        += result.size_req;
  if (*buf_size >= (buf_max - 1))
    *buf_size = buf_max - 1;
  DN_Assert(*buf_size <= (buf_max - 1));
  result.str8      = DN_Str8FromPtr(buf, *buf_size);
  result.truncated = result.str8.count != (starting_size + result.size_req);
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
    DN_Memcpy(result.str8.data + result.str8.count - truncator.count, truncator.data, truncator.count);
  va_end(args);
  return result;
}

DN_API DN_USize DN_FmtCount(DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_USize result = DN_Vsnprintf(nullptr, 0, fmt, args);
  va_end(args);
  return result;
}

DN_API DN_USize DN_FmtVCount(DN_FMT_ATTRIB char const *fmt, va_list args)
{
  va_list args_copy;
  va_copy(args_copy, args);
  DN_USize result = DN_Vsnprintf(nullptr, 0, fmt, args_copy);
  va_end(args_copy);
  return result;
}

DN_API DN_USize DN_CStr8Count(char const *src)
{
  DN_USize result = 0;
  for (; src && src[0] != 0; src++, result++)
    ;
  return result;
}

DN_API DN_USize DN_CStr16Count(wchar_t const *src)
{
  DN_USize result = 0;
  for (; src && src[0] != 0; src++, result++)
    ;
  return result;
}

DN_API DN_Str8 DN_Str8AllocAllocator(DN_USize count, DN_ZMem z_mem, DN_Allocator allocator)
{
  DN_Str8 result = {};
  result.data    = DN_Cast(char *) DN_AllocatorAlloc(allocator, count + 1, alignof(char), z_mem);
  if (result.data) {
    result.count              = count;
    result.data[result.count] = 0;
  }
  return result;
}

DN_API DN_Str8 DN_Str8AllocArena(DN_USize count, DN_ZMem z_mem, DN_Arena *arena)
{
  DN_Str8 result = DN_Str8AllocAllocator(count, z_mem, DN_AllocatorFromArena(arena));
  return result;
}

DN_API DN_Str8 DN_Str8AllocPool(DN_USize count, DN_Pool *pool)
{
  DN_Str8 result = DN_Str8AllocAllocator(count, DN_ZMem_No, DN_AllocatorFromPool(pool));
  return result;
}

DN_API DN_Str8 DN_Str8FromCStr8(char const *src)
{
  DN_USize count  = DN_CStr8Count(src);
  DN_Str8  result = DN_Str8FromPtr(src, count);
  return result;
}

DN_API DN_Str8 DN_Str8FromCStr8Arena(char const *src, DN_Arena *arena)
{
  DN_Str8 shallow = DN_Str8FromCStr8(src);
  DN_Str8 result  = DN_Str8FromStr8Arena(shallow, arena);
  return result;
}

DN_API DN_Str8 DN_Str8FromPtrArena(void const *data, DN_USize count, DN_Arena *arena)
{
  DN_Str8 result = DN_Str8AllocArena(count, DN_ZMem_No, arena);
  if (result.count)
    DN_Memcpy(result.data, data, count);
  return result;
}

DN_API DN_Str8 DN_Str8FromPtrPool(void const *data, DN_USize count, DN_Pool *pool)
{
  DN_Str8 result = DN_Str8AllocPool(count, pool);
  if (result.count)
    DN_Memcpy(result.data, data, count);
  return result;
}

DN_API DN_Str8 DN_Str8FromStr8Allocator(DN_Str8 string, DN_Allocator allocator)
{
  DN_Str8 result = {};
  result.data    = DN_Cast(char *) DN_AllocatorAlloc(allocator, string.count + 1, alignof(char), DN_ZMem_No);
  if (result.data) {
    DN_Memcpy(result.data, string.data, string.count);
    result.data[string.count] = 0;
    result.count              = string.count;
  }
  return result;
}

DN_API DN_Str8 DN_Str8FromStr8Arena(DN_Str8 string, DN_Arena *arena)
{
  DN_Str8 result = DN_Str8FromStr8Allocator(string, DN_AllocatorFromArena(arena));
  return result;
}

DN_API DN_Str8 DN_Str8FromStr8Pool(DN_Str8 string, DN_Pool *pool)
{
  DN_Str8 result = DN_Str8FromStr8Allocator(string, DN_AllocatorFromPool(pool));
  return result;
}

DN_API DN_Str8 DN_Str8FmtVAllocator(DN_Allocator allocator, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_USize count  = DN_FmtVCount(fmt, args);
  DN_Str8  result = DN_Str8AllocAllocator(count, DN_ZMem_No, allocator);
  if (result.data) {
    DN_USize written = 0;
    DN_FmtVAppend(result.data, &written, result.count + 1, fmt, args);
    DN_Assert(written == result.count);
  }
  return result;
}

DN_API DN_Str8 DN_Str8FmtVArena(DN_Arena *arena, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_Str8 result = DN_Str8FmtVAllocator(DN_AllocatorFromArena(arena), fmt, args);
  return result;
}

DN_API DN_Str8 DN_Str8FmtAllocator(DN_Allocator allocator, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  DN_Str8 result = DN_Str8FmtVAllocator(allocator, fmt, va);
  va_end(va);
  return result;
}

DN_API DN_Str8 DN_Str8FmtArena(DN_Arena *arena, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  DN_Str8 result = DN_Str8FmtVArena(arena, fmt, va);
  va_end(va);
  return result;
}

DN_API DN_Str8 DN_Str8FmtVPool(DN_Pool *pool, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_Str8 result = DN_Str8FmtVAllocator(DN_AllocatorFromPool(pool), fmt, args);
  return result;
}

DN_API DN_Str8 DN_Str8FmtPool(DN_Pool *pool, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8 result = DN_Str8FmtVPool(pool, fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8x16 DN_Str8x16FromFmt(DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8x16 result = {};
  DN_FmtVAppend(result.data, &result.count, sizeof(result.data), fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8x16 DN_Str8x16FromFmtVArena(DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_Str8x16 result = {};
  DN_FmtVAppend(result.data, &result.count, sizeof(result.data), fmt, args);
  return result;
}

DN_API DN_Str8x32 DN_Str8x32FromFmt(DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8x32 result = {};
  DN_FmtVAppend(result.data, &result.count, sizeof(result.data), fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8x32 DN_Str8x32FromFmtVArena(DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_Str8x32 result = {};
  DN_FmtVAppend(result.data, &result.count, sizeof(result.data), fmt, args);
  return result;
}

DN_API DN_Str8x64 DN_Str8x64FromFmt(DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8x64 result = {};
  DN_FmtVAppend(result.data, &result.count, sizeof(result.data), fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8x64 DN_Str8x64FromFmtV(DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_Str8x64 result = {};
  DN_FmtVAppend(result.data, &result.count, sizeof(result.data), fmt, args);
  return result;
}

DN_API DN_Str8x128 DN_Str8x128FromFmt(DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8x128 result = {};
  DN_FmtVAppend(result.data, &result.count, sizeof(result.data), fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8x128 DN_Str8x128FromFmtVArena(DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_Str8x128 result = {};
  DN_FmtVAppend(result.data, &result.count, sizeof(result.data), fmt, args);
  return result;
}

DN_API DN_Str8x256 DN_Str8x256FromFmt(DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8x256 result = {};
  DN_FmtVAppend(result.data, &result.count, sizeof(result.data), fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8x256 DN_Str8x256FromFmtVArena(DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_Str8x256 result = {};
  DN_FmtVAppend(result.data, &result.count, sizeof(result.data), fmt, args);
  return result;
}

DN_API DN_Str8x512 DN_Str8x512FromFmt(DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8x512 result = {};
  DN_FmtVAppend(result.data, &result.count, sizeof(result.data), fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8x512 DN_Str8x512FromFmtVArena(DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_Str8x512 result = {};
  DN_FmtVAppend(result.data, &result.count, sizeof(result.data), fmt, args);
  return result;
}

DN_API DN_Str8x1024 DN_Str8x1024FromFmt(DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8x1024 result = {};
  DN_FmtVAppend(result.data, &result.count, sizeof(result.data), fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8x1024 DN_Str8x1024FromFmtVArena(DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_Str8x1024 result = {};
  DN_FmtVAppend(result.data, &result.count, sizeof(result.data), fmt, args);
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
  DN_FmtVAppend(str->data, &str->count, sizeof(str->data), fmt, args);
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
  DN_FmtVAppend(str->data, &str->count, sizeof(str->data), fmt, args);
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
  DN_FmtVAppend(str->data, &str->count, sizeof(str->data), fmt, args);
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
  DN_FmtVAppend(str->data, &str->count, sizeof(str->data), fmt, args);
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
  DN_FmtVAppend(str->data, &str->count, sizeof(str->data), fmt, args);
}

DN_API void DN_Str8x512AppendFmt(DN_Str8x512 *str, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8x512AppendFmtV(str, fmt, args);
  va_end(args);
}

DN_API void DN_Str8x512AppendFmtV(DN_Str8x512 *str, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_FmtVAppend(str->data, &str->count, sizeof(str->data), fmt, args);
}

DN_API void DN_Str8x1024AppendFmt(DN_Str8x1024 *str, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8x1024AppendFmtV(str, fmt, args);
  va_end(args);
}

DN_API void DN_Str8x1024AppendFmtV(DN_Str8x1024 *str, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_FmtVAppend(str->data, &str->count, sizeof(str->data), fmt, args);
}

DN_API DN_Str8x32 DN_Str8x32FromU64(DN_U64 val, char seperator)
{
  DN_Str8x32 result     = {};
  DN_Str8x32 temp       = DN_Str8x32FromFmt("%" PRIu64, val);
  DN_USize   temp_index = 0;

  // NOTE: Write the digits the first, up to [0, 2] digits that do not need a thousandth seperator
  DN_USize   range_without_seperator = temp.count % 3;
  for (; temp_index < range_without_seperator; temp_index++)
    result.data[result.count++] = temp.data[temp_index];

  // NOTE: Write the subsequent digits and every 3rd digit, add the seperator
  DN_USize   digit_counter = 0;
  for (; temp_index < temp.count; temp_index++, digit_counter++) {
    if (seperator && temp_index && (digit_counter % 3 == 0))
      result.data[result.count++] = seperator;
    result.data[result.count++] = temp.data[temp_index];
  }
  return result;
}


DN_API bool DN_Str8Is(DN_Str8 string, DN_Str8IsFlags flags)
{
  bool result = string.count;
  if (!result)
    return result;

  if (result && (flags & DN_Str8IsFlags_Digits)) {
    for (DN_USize index = 0; result && index < string.count; index++)
      result = string.data[index] >= '0' && string.data[index] <= '9';
  }

  if (result && (flags & DN_Str8IsFlags_Hex)) {
    DN_Str8 trimmed = DN_Str8TrimPrefix(string, DN_Str8Lit("0x"), DN_Str8EqCase_Insensitive);
    for (DN_USize index = 0; result && index < trimmed.count; index++) {
      char ch = trimmed.data[index];
      result  = (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
    }
  }

  if (result && (flags & DN_Str8IsFlags_Lowercase)) {
    for (DN_USize index = 0; result && index < string.count; index++) {
      if (string.data[index] >= 'A' && string.data[index] <= 'Z')
        result = false;
    }
  }

  if (result && (flags & DN_Str8IsFlags_Uppercase)) {
    for (DN_USize index = 0; result && index < string.count; index++) {
      if (string.data[index] >= 'a' && string.data[index] <= 'z')
        result = false;
    }
  }
  return result;
}

DN_API char *DN_Str8End(DN_Str8 string)
{
  char *result = string.data + string.count;
  return result;
}

DN_API DN_Str8 DN_Str8Subset(DN_Str8 string, DN_USize offset, DN_USize count)
{
  DN_Str8 result = DN_Str8FromPtr(string.data, 0);
  if (string.count == 0)
    return result;

  DN_USize capped_offset = DN_Min(offset, string.count);
  DN_USize max_size      = string.count - capped_offset;
  DN_USize capped_size   = DN_Min(count, max_size);
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
  if (string.count == 0 || !find || find_size == 0)
    return result;

  result.lhs = string;
  for (DN_USize index = 0; !result.rhs.data && index < string.count; index++) {
    for (DN_USize find_index = 0; find_index < find_size; find_index++) {
      DN_Str8 find_item    = find[find_index];
      DN_Str8 string_slice = DN_Str8Subset(string, index, find_item.count);
      if (DN_Str8EqSensitive(string_slice, find_item)) {
        result.input_index = find_index;
        result.lhs.count   = index;
        result.rhs.data    = string_slice.data + find_item.count;
        result.rhs.count   = string.count - (index + find_item.count);
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
  if (string.count == 0 || !find || find_size == 0)
    return result;

  result.lhs = string;
  for (DN_USize index = string.count - 1; !result.rhs.data && index < string.count; index--) {
    for (DN_USize find_index = 0; find_index < find_size; find_index++) {
      DN_Str8 find_item    = find[find_index];
      DN_Str8 string_slice = DN_Str8Subset(string, index, find_item.count);
      if (DN_Str8EqSensitive(string_slice, find_item)) {
        result.lhs.count = index;
        result.rhs.data = string_slice.data + find_item.count;
        result.rhs.count = string.count - (index + find_item.count);
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

DN_API DN_USize DN_Str8Split(DN_Str8 string, DN_Str8 delimiter, DN_Str8 *splits, DN_USize splits_count, DN_Str8SplitFlags flags)
{
  DN_USize result = 0; // The number of splits in the actual string.
  if (string.count == 0 || delimiter.count == 0 || delimiter.count <= 0)
    return result;

  DN_Str8 it                  = string;
  bool    allow_empty_strings = DN_BitIsNotSet(flags, DN_Str8SplitFlags_ExcludeEmptyStrings);
  bool    handle_quotes       = DN_BitIsSet(flags, DN_Str8SplitFlags_HandleQuotedStrings);
  do {
    DN_Str8 item = {};
    if (handle_quotes && DN_Str8StartsWithSensitive(it, DN_Str8Lit("\""))) {
      DN_Str8FindResult find = DN_Str8FindStr8(DN_Str8Advance(it, 1), DN_Str8Lit("\""), DN_Str8EqCase_Sensitive);
      DN_Assert(find.found);
      item = find.start_to_before_match;
      it   = DN_Str8BSplit(find.after_match_to_end_of_buffer, delimiter).rhs;
    } else {
      DN_Str8BSplitResult sub_split = DN_Str8BSplit(it, delimiter);
      item                          = sub_split.lhs;
      it                            = sub_split.rhs;
    }

    if (item.count || allow_empty_strings) {
      if (splits && result < splits_count)
        splits[result] = item;
      result++;
    }
  } while (it.count);

  return result;
}

DN_API DN_Str8SplitResult DN_Str8SplitArena(DN_Str8 string, DN_Str8 delimiter, DN_Str8SplitFlags mode, DN_Arena *arena)
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
  for (DN_USize index = 0; !result.found && index < string.count; index++) {
    for (DN_USize find_index = 0; find_index < find_size; find_index++) {
      DN_Str8 find_item    = find[find_index];
      DN_Str8 string_slice = DN_Str8Subset(string, index, find_item.count);
      if (DN_Str8Eq(string_slice, find_item, eq_case)) {
        result.found                        = true;
        result.index                        = index;
        result.start_to_before_match        = DN_Str8FromPtr(string.data, index);
        result.match                        = DN_Str8FromPtr(string.data + index, find_item.count);
        result.match_to_end_of_buffer       = DN_Str8FromPtr(result.match.data, string.count - index);
        result.after_match_to_end_of_buffer = DN_Str8Advance(result.match_to_end_of_buffer, find_item.count);
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
  for (DN_USize index = 0; !result.found && index < string.count; index++) {
    result.found |= ((flags & DN_Str8FindFlag_Digit) && DN_CharIsDigit(string.data[index]));
    result.found |= ((flags & DN_Str8FindFlag_Alphabet) && DN_CharIsAlphabet(string.data[index]));
    result.found |= ((flags & DN_Str8FindFlag_Whitespace) && DN_CharIsWhitespace(string.data[index]));
    result.found |= ((flags & DN_Str8FindFlag_Plus) && string.data[index] == '+');
    result.found |= ((flags & DN_Str8FindFlag_Minus) && string.data[index] == '-');
    if (result.found) {
      result.index                        = index;
      result.match                        = DN_Str8FromPtr(string.data + index, 1);
      result.match_to_end_of_buffer       = DN_Str8FromPtr(result.match.data, string.count - index);
      result.after_match_to_end_of_buffer = DN_Str8Advance(result.match_to_end_of_buffer, 1);
    }
  }
  return result;
}

DN_API DN_Str8 DN_Str8Segment(DN_Arena *arena, DN_Str8 src, DN_USize segment_size, char segment_char)
{
  if (!segment_size || src.count == 0) {
    DN_Str8 result = DN_Str8FromStr8Arena(src, arena);
    return result;
  }

  DN_USize segments = src.count / segment_size;
  if (src.count % segment_size == 0)
    segments--;

  DN_USize segment_counter = 0;
  DN_Str8  result          = DN_Str8AllocArena(src.count + segments, DN_ZMem_Yes, arena);
  DN_USize write_index     = 0;
  for (DN_ForIndexU(src_index, src.count)) {
    result.data[write_index++] = src.data[src_index];
    if ((src_index + 1) % segment_size == 0 && segment_counter < segments) {
      result.data[write_index++] = segment_char;
      segment_counter++;
    }
    DN_AssertF(write_index <= result.count, "result.count=%zu, write_index=%zu", result.count, write_index);
  }

  DN_AssertF(write_index == result.count, "result.count=%zu, write_index=%zu", result.count, write_index);
  return result;
}

DN_API DN_Str8 DN_Str8ReverseSegment(DN_Arena *arena, DN_Str8 src, DN_USize segment_size, char segment_char)
{
  if (!segment_size || src.count == 0) {
    DN_Str8 result = DN_Str8FromStr8Arena(src, arena);
    return result;
  }

  DN_USize segments = src.count / segment_size;
  if (src.count % segment_size == 0)
    segments--;

  DN_USize write_counter   = 0;
  DN_USize segment_counter = 0;
  DN_Str8  result          = DN_Str8AllocArena(src.count + segments, DN_ZMem_Yes, arena);
  DN_USize write_index     = result.count - 1;

  DN_MSVC_WARNING_PUSH
  DN_MSVC_WARNING_DISABLE(6293) // NOTE: Ill-defined loop
  for (DN_USize src_index = src.count - 1; src_index < src.count; src_index--) {
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
  if (lhs.count != rhs.count)
    return false;
  bool result = true;
  switch (eq_case) {
    case DN_Str8EqCase_Sensitive: {
      result = DN_MemEqUnsafe(lhs.data, rhs.data, lhs.count);
    } break;

    case DN_Str8EqCase_Insensitive: {
      for (DN_USize index = 0; index < lhs.count && result; index++)
        result = (DN_CharToLower(lhs.data[index]) == DN_CharToLower(rhs.data[index]));
    } break;
  }
  return result;
}

DN_API bool DN_Str8EqSensitive(DN_Str8 lhs, DN_Str8 rhs)
{
  bool result = DN_Str8Eq(lhs, rhs, DN_Str8EqCase_Sensitive);
  return result;
}

DN_API bool DN_Str8EqInsensitive(DN_Str8 lhs, DN_Str8 rhs)
{
  bool result = DN_Str8Eq(lhs, rhs, DN_Str8EqCase_Insensitive);
  return result;
}

DN_API bool DN_Str8StartsWith(DN_Str8 string, DN_Str8 prefix, DN_Str8EqCase eq_case)
{
  DN_Str8 substring = {string.data, DN_Min(prefix.count, string.count)};
  bool    result    = DN_Str8Eq(substring, prefix, eq_case);
  return result;
}

DN_API bool DN_Str8StartsWithSensitive(DN_Str8 string, DN_Str8 prefix)
{
  bool result = DN_Str8StartsWith(string, prefix, DN_Str8EqCase_Sensitive);
  return result;
}


DN_API bool DN_Str8StartsWithInsensitive(DN_Str8 string, DN_Str8 prefix)
{
  bool result = DN_Str8StartsWith(string, prefix, DN_Str8EqCase_Insensitive);
  return result;
}

DN_API bool DN_Str8EndsWith(DN_Str8 string, DN_Str8 suffix, DN_Str8EqCase eq_case)
{
  DN_Str8 substring = {string.data + string.count - suffix.count, DN_Min(string.count, suffix.count)};
  bool    result    = DN_Str8Eq(substring, suffix, eq_case);
  return result;
}

DN_API bool DN_Str8EndsWithSensitive(DN_Str8 string, DN_Str8 suffix)
{
  bool result = DN_Str8EndsWith(string, suffix, DN_Str8EqCase_Sensitive);
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
  for (DN_USize index = 0; !result && index < string.count; index++)
    result = string.data[index] == ch;
  return result;
}

DN_API DN_Str8 DN_Str8TrimPrefix(DN_Str8 string, DN_Str8 prefix, DN_Str8EqCase eq_case)
{
  DN_Str8 result = string;
  if (DN_Str8StartsWith(string, prefix, eq_case)) {
    result.data += prefix.count;
    result.count -= prefix.count;
  }
  return result;
}

DN_API DN_Str8 DN_Str8TrimPrefixSensitive(DN_Str8 string, DN_Str8 prefix)
{
  DN_Str8 result = DN_Str8TrimPrefix(string, prefix, DN_Str8EqCase_Sensitive);
  return result;
}

DN_API DN_Str8 DN_Str8TrimPrefixInsensitive(DN_Str8 string, DN_Str8 prefix)
{
  DN_Str8 result = DN_Str8TrimPrefix(string, prefix, DN_Str8EqCase_Insensitive);
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
    result.count -= suffix.count;
  return result;
}

DN_API DN_Str8 DN_Str8TrimSuffixSensitive(DN_Str8 string, DN_Str8 prefix)
{
  DN_Str8 result = DN_Str8TrimSuffix(string, prefix, DN_Str8EqCase_Sensitive);
  return result;
}

DN_API DN_Str8 DN_Str8TrimSuffixInsensitive(DN_Str8 string, DN_Str8 prefix)
{
  DN_Str8 result = DN_Str8TrimSuffix(string, prefix, DN_Str8EqCase_Insensitive);
  return result;
}

DN_API DN_Str8 DN_Str8TrimAround(DN_Str8 string, DN_Str8 trim_string, DN_Str8EqCase eq_case)
{
  DN_Str8 result = DN_Str8TrimPrefix(string, trim_string, eq_case);
  result         = DN_Str8TrimSuffix(result, trim_string, eq_case);
  return result;
}

DN_API DN_Str8 DN_Str8TrimAroundSensitive(DN_Str8 string, DN_Str8 trim_string)
{
  DN_Str8 result = DN_Str8TrimAround(string, trim_string, DN_Str8EqCase_Sensitive);
  return result;
}

DN_API DN_Str8 DN_Str8TrimAroundInsensitive(DN_Str8 string, DN_Str8 trim_string)
{
  DN_Str8 result = DN_Str8TrimAround(string, trim_string, DN_Str8EqCase_Insensitive);
  return result;
}

DN_API DN_Str8 DN_Str8TrimHeadWhitespace(DN_Str8 string)
{
  DN_Str8 result = string;
  if (string.count == 0)
    return result;

  char const *start = string.data;
  char const *end   = string.data + string.count;
  while (start < end && DN_CharIsWhitespace(start[0]))
    start++;

  result = DN_Str8FromPtr(start, end - start);
  return result;
}

DN_API DN_Str8 DN_Str8TrimTailWhitespace(DN_Str8 string)
{
  DN_Str8 result = string;
  if (string.count == 0)
    return result;

  char const *start = string.data;
  char const *end   = string.data + string.count;
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
  if (result.count == 0)
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
  DN_Str8             seperators[] = {DN_Str8Lit("/"), DN_Str8Lit("\\")};
  DN_Str8BSplitResult split        = DN_Str8BSplitLastArray(path, seperators, DN_ArrayCountU(seperators));
  DN_Str8             result       = split.rhs.count ? split.rhs : split.lhs;
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
  DN_Str8             result = split.lhs;
  return result;
}

DN_API DN_Str8 DN_Str8FileExtension(DN_Str8 path)
{
  DN_Str8BSplitResult split  = DN_Str8BSplitLast(path, DN_Str8Lit("."));
  DN_Str8             result = split.rhs;
  return result;
}

DN_API DN_Str8 DN_Str8FileDirectoryFromPath(DN_Str8 path)
{
  DN_Str8             seperators[] = {DN_Str8Lit("/"), DN_Str8Lit("\\")};
  DN_Str8BSplitResult split        = DN_Str8BSplitLastArray(path, seperators, DN_ArrayCountU(seperators));
  DN_Str8             result       = split.rhs.count == 0 ? DN_Str8Lit(".") : split.lhs;
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
  DN_Str8 append = DN_Str8FmtVArena(arena, fmt, args);
  DN_Str8 result = DN_Str8AllocArena(string.count + append.count, DN_ZMem_No, arena);
  DN_Memcpy(result.data, string.data, string.count);
  DN_Memcpy(result.data + string.count, append.data, append.count);
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
  DN_Str8 fill = DN_Str8FmtVArena(arena, fmt, args);
  DN_Str8 result = DN_Str8AllocArena(count * fill.count, DN_ZMem_No, arena);
  for (DN_USize index = 0; index < count; index++) {
    void *dest = result.data + (index * fill.count);
    DN_Memcpy(dest, fill.data, fill.count);
  }
  return result;
}

DN_API void DN_Str8Remove(DN_Str8 *string, DN_USize offset, DN_USize count)
{
  if (!string || string->count)
    return;

  char    *end           = string->data + string->count;
  char    *dest          = DN_Min(string->data + offset, end);
  char    *src           = DN_Min(string->data + offset + count, end);
  DN_USize bytes_to_move = end - src;
  DN_Memmove(dest, src, bytes_to_move);
  string->count -= bytes_to_move;
}

DN_API DN_Str8 DN_Str8TruncateArena(DN_Str8 string, DN_USize max_count, DN_Str8 truncator, DN_Arena *arena)
{
  DN_Str8 result = {};
  if (string.count > max_count) {
    DN_Str8 string_trunc = DN_Str8Subset(string, 0, max_count);
    result               = DN_Str8FmtArena(arena, "%.*s%.*s", DN_Str8PrintFmt(string_trunc), DN_Str8PrintFmt(truncator));
  } else {
    result = DN_Str8FromStr8Arena(string, arena);
  }
  return result;
}

DN_API DN_Str8TruncResult DN_Str8TruncMiddlePtr(DN_Str8 str8, DN_USize side_size, DN_Str8 truncator, char *dest, DN_USize dest_max)
{
  DN_Assert(side_size <= DN_USIZE_MAX / 2);
  if (dest) {
    // NOTE: If the user passes the dest buffer, we expect it to be sized correctly.
    if ((side_size * 2) >= str8.count) {
      DN_Assert(dest_max >= str8.count + 1 /*null*/);
    } else {
      DN_Assert(dest_max >= (2 * side_size + truncator.count) + 1 /*null*/);
    }
  }

  DN_Str8TruncResult result = {};
  if (str8.count <= (side_size * 2)) {
    result.count_req = str8.count;
    if (dest) {
      DN_Memcpy(dest, str8.data, str8.count);
      dest[str8.count] = 0;
      result.str8     = DN_Str8FromPtr(dest, result.count_req);
    }
    return result;
  }

  DN_Str8            head          = DN_Str8Subset(str8, 0, side_size);
  DN_Str8            tail          = DN_Str8Subset(str8, str8.count - side_size, side_size);
  DN_USize           dest_size     = 0;
  if (dest) {
    DN_FmtAppendResult append_result = DN_FmtAppend(dest, &dest_size, dest_max, "%.*s%.*s%.*s", DN_Str8PrintFmt(head), DN_Str8PrintFmt(truncator), DN_Str8PrintFmt(tail));
    result.str8                      = append_result.str8;
    result.truncated                 = true;
    result.count_req                 = result.str8.count;
  } else {
    result.count_req  = DN_FmtCount("%.*s%.*s%.*s", DN_Str8PrintFmt(head), DN_Str8PrintFmt(truncator), DN_Str8PrintFmt(tail));
    result.truncated = true;
  }

  return result;
}

DN_API DN_Str8TruncResult DN_Str8TruncMiddle(DN_Str8 str8, DN_USize side_size, DN_Str8 truncator, DN_Arena *arena)
{
  DN_Str8TruncResult trunc  = DN_Str8TruncMiddlePtr(str8, side_size, truncator, nullptr, 0);
  DN_Str8            dest   = DN_Str8AllocArena(trunc.count_req, DN_ZMem_No, arena);
  DN_Str8TruncResult result = DN_Str8TruncMiddlePtr(str8, side_size, truncator, dest.data, dest.count + 1);
  return result;
}

DN_API DN_Str8 DN_Str8Lower(DN_Str8 string, DN_Arena *arena)
{
  DN_Str8 result = DN_Str8FromStr8Arena(string, arena);
  for (DN_ForIndexU(index, result.count))
    result.data[index] = DN_CharToLower(result.data[index]);
  return result;
}

DN_API DN_Str8 DN_Str8Upper(DN_Str8 string, DN_Arena *arena)
{
  DN_Str8 result = DN_Str8FromStr8Arena(string, arena);
  for (DN_ForIndexU(index, result.count))
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
  DN_Str8 result = {};
  if (string.count == 0 || find.count == 0 || find.count > string.count || find.count == 0 || string.count == 0) {
    result = DN_Str8FromStr8Arena(string, arena);
    return result;
  }

  DN_TcScratch   scratch        = DN_TcScratchBeginArena(&arena, 1);
  DN_Str8Builder string_builder = DN_Str8BuilderFromArena(&scratch.arena);
  DN_USize       max            = string.count - find.count;
  DN_USize       head           = start_index;

  for (DN_USize tail = head; tail <= max; tail++) {
    DN_Str8 check = DN_Str8Subset(string, tail, find.count);
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
    head = tail + find.count;
    tail += find.count - 1; // NOTE: -1 since the for loop will post increment us past the end of the find string
  }

  if (string_builder.string_size == 0) {
    // NOTE: No replacement possible, so we just do a full-copy
    result = DN_Str8FromStr8Arena(string, arena);
  } else {
    DN_Str8 remainder = DN_Str8FromPtr(string.data + head, string.count - head);
    DN_Str8BuilderAppendRef(&string_builder, remainder);
    result = DN_Str8FromStr8BuilderArena(&string_builder, arena);
  }
  DN_TcScratchEnd(&scratch);
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

DN_API DN_Str8 DN_Str8PadNewLinesAllocator(DN_Str8 string, DN_Str8 pad_string, DN_Allocator allocator)
{
  DN_TcScratch   scratch = DN_TcScratchBeginAllocator(&allocator, 1);
  DN_Str8Builder builder = DN_Str8BuilderFromArena(&scratch.arena);
  DN_Str8        it      = string;
  while (it.count) {
    DN_Str8BSplitResult split = DN_Str8BSplit(it, DN_Str8Lit("\n"));
    DN_Str8BuilderAppendRef(&builder, DN_Str8FromPtr(split.lhs.data, split.lhs.count + 1));
    it = split.rhs;
  }

  DN_Str8 result = DN_Str8FromStr8BuilderDelimitAllocator(&builder, pad_string, allocator);
  DN_TcScratchEnd(&scratch);
  return result;
}

DN_API DN_Str8 DN_Str8PadNewLinesArena(DN_Str8 string, DN_Str8 pad_string, DN_Arena *arena)
{
  DN_Str8 result = DN_Str8PadNewLinesAllocator(string, pad_string, DN_AllocatorFromArena(arena));
  return result;
}

DN_API DN_USize DN_USizeCodepointCountFromUTF8(DN_Str8 str, DN_CodepointCountFlags flags)
{
  DN_USize result = 0;

  if (DN_BitIsNotSet(flags, DN_CodepointCountFlags_SkipAnsiCode)) {
    DN_UTF8DecodeIterator it = {};
    while (DN_UTF8DecodeIterate(&it, str))
      ;
    result = it.codepoint_index;
  } else {
    // NOTE: Ansi SGR (Select Graphic Rendition) sequence handling
    // Format:             ESC [ parameter_bytes intermediate_bytes final_byte
    // Common examples:    \x1b[31m (red), \x1b[1;31m (bold red), \x1b[0m (reset)
    // Parameter bytes:    0x30-0x3F (digits and :;<=>?)
    // Intermediate bytes: 0x20-0x2F (space and !"#$%&'()*+,-./)
    // Final byte:         0x40-0x7E (@A-Z[\]^_`a-z{|}~)
    char const *p   = str.data;
    char const *end = DN_Str8End(str);
    while (p < end) {
      if (*p == '\x1b' && p + 1 < end && *(p + 1) == '[') { // Detect CSI sequence: ESC [
        p += 2;
        while (p < end && *p >= 0x30 && *p <= 0x3F)         // Skip parameter bytes (0x30-0x3F)
          p++;
        while (p < end && *p >= 0x20 && *p <= 0x2F)         // Skip intermediate bytes (0x20-0x2F)
          p++;
        if (p < end && *p >= 0x40 && *p <= 0x7E)            // Skip final byte (0x40-0x7E)
          p++;
        continue;
      }

      DN_UTF8DecodeResult decode = DN_UTF8Decode(DN_Str8FromPtr(p, end - p));
      if (!decode.success)
        break;
      p = decode.remaining.data;
      result++;
    }
  }

  return result;
}

DN_API DN_Str8 DN_Str8LineBreakAllocator(DN_Str8 src, DN_USize desired_width, DN_Str8 delimiter, DN_Str8LineBreakMode mode, DN_Allocator allocator)
{
  DN_TcScratch   scratch = DN_TcScratchBeginAllocator(&allocator, 1);
  DN_Str8Builder builder = DN_Str8BuilderFromArena(&scratch.arena);

  if (mode == DN_Str8LineBreakMode_AtWord) {
    char*   start = src.data;
    char*   end   = src.data;
    DN_Str8 it    = src;
    while (it.count) {
      DN_Str8             splitters[]      = {DN_Str8Lit(" "), DN_Str8Lit("\n")};
      DN_Str8BSplitResult split            = DN_Str8BSplitArray(it, splitters, DN_ArrayCountU(splitters));
      DN_USize            curr_line_length = end - start;

      // Handle explicit newlines in input
      if (split.input_index == 1 /*the newline*/) {
        if (curr_line_length == 0 && split.lhs.count)
          start = split.lhs.data;
        if (split.lhs.count)
          end = DN_Str8End(split.lhs);
        DN_Str8BuilderAppendRef(&builder, DN_Str8FromPtr(start, end - start));
        start = split.rhs.data;
        end   = split.rhs.data;
        it    = split.rhs;
        continue;
      }

      // Skip empty segments (multiple spaces, leading/trailing spaces)
      if (split.lhs.count == 0) {
        it = split.rhs;
        continue;
      }

      // First word on this line
      if (curr_line_length == 0) {
        start = split.lhs.data;
        end   = DN_Str8End(split.lhs);
        it    = split.rhs;
        continue;
      }

      // Check if adding this word (plus seperator space) would overflow
      DN_USize combined_length = curr_line_length + 1 + split.lhs.count;
      if (combined_length > desired_width) {
        // Commit current line, start new line with current word
        DN_Str8BuilderAppendRef(&builder, DN_Str8FromPtr(start, end - start));
        start = split.lhs.data;
        end   = DN_Str8End(split.lhs);
        it    = split.rhs;
      } else {
        // Add word to current line
        end = DN_Str8End(split.lhs);
        it  = split.rhs;
      }
    }

    // Append final line
    if (end > start)
      DN_Str8BuilderAppendRef(&builder, DN_Str8FromPtr(start, end - start));
  } else {
    DN_Str8 it = src;
    while (it.count) {
      DN_Str8 chunk = DN_Str8Subset(it, 0, desired_width);
      DN_Str8BuilderAppendRef(&builder, chunk);
      it = DN_Str8Advance(it, desired_width);
    }
  }

  DN_Str8 result = DN_Str8FromStr8BuilderDelimitAllocator(&builder, delimiter, allocator);
  DN_TcScratchEnd(&scratch);
  return result;
}

DN_API DN_Str8 DN_Str8LineBreakArena(DN_Str8 src, DN_USize desired_width, DN_Str8 delimiter, DN_Str8LineBreakMode mode,  DN_Arena *arena)
{
  DN_Str8 result = DN_Str8LineBreakAllocator(src, desired_width, delimiter, mode, DN_AllocatorFromArena(arena));
  return result;
}

DN_API DN_Str8 DN_Str8Table(DN_Str8 const *rows, DN_USize num_rows, DN_USize num_cols, DN_Str8TableFlags flags, DN_Arena *arena)
{
  DN_TcScratch scratch         = DN_TcScratchBeginArena(&arena, 1);
  DN_U16       col_widths[128] = {};
  for (DN_USize i = 0; i < num_cols; i++) {
    for (DN_USize j = 0; j < num_rows; j++) {
      DN_USize index = j * num_cols + i;
      col_widths[i]  = DN_Max(col_widths[i], (DN_U16)DN_USizeCodepointCountFromUTF8(rows[index], DN_CodepointCountFlags_SkipAnsiCode));
    }
  }

  DN_Str8Builder builder = DN_Str8BuilderFromArena(&scratch.arena);
  DN_Str8BuilderAppendF(&builder, "+");
  for (DN_USize i = 0; i < num_cols; i++) {
    for (DN_USize j = 0; j < col_widths[i] + 2; j++)
      DN_Str8BuilderAppendF(&builder, "-");
    DN_Str8BuilderAppendF(&builder, "+");
  }
  DN_Str8BuilderAppendF(&builder, "\n");

  for (DN_USize i = 0; i < num_rows; i++) {
    DN_Str8BuilderAppendF(&builder, "|");
    for (DN_USize j = 0; j < num_cols; j++) {
      DN_USize index = (i * num_cols) + j;
      DN_Str8  item  = rows[index];
      DN_Str8BuilderAppendF(&builder, " %.*s", DN_Str8PrintFmt(item));
      DN_USize item_width = DN_USizeCodepointCountFromUTF8(item, DN_CodepointCountFlags_SkipAnsiCode);
      for (DN_USize k = 0; k < col_widths[j] - item_width; k++)
        DN_Str8BuilderAppendF(&builder, " ");
      DN_Str8BuilderAppendF(&builder, " |");
    }
    DN_Str8BuilderAppendF(&builder, "\n");

    bool print_row_line = i == 0 && DN_BitIsSet(flags, DN_Str8TableFlags_HasHeader);
    if (!print_row_line)
      print_row_line = DN_BitIsSet(flags, DN_Str8TableFlags_RowLines);

    if (print_row_line && (i != num_rows - 1)) {
      DN_Str8BuilderAppendF(&builder, "+");
      for (DN_USize sub_i = 0; sub_i < num_cols; sub_i++) {
        for (DN_USize sub_j = 0; sub_j < col_widths[sub_i] + 2; sub_j++)
          DN_Str8BuilderAppendF(&builder, "-");
        DN_Str8BuilderAppendF(&builder, "+");
      }
      DN_Str8BuilderAppendF(&builder, "\n");
    }
  }

  DN_Str8BuilderAppendF(&builder, "+");
  for (DN_USize i = 0; i < num_cols; i++) {
    for (DN_USize j = 0; j < col_widths[i] + 2; j++)
      DN_Str8BuilderAppendF(&builder, "-");
    DN_Str8BuilderAppendF(&builder, "+");
  }

  DN_Str8 result = DN_Str8FromStr8BuilderArena(&builder, arena);
  DN_TcScratchEnd(&scratch);
  return result;
}

#if DN_WITH_STR8_AVX512F
DN_API DN_Str8FindResult DN_Str8FindStr8AVX512F(DN_Str8 string, DN_Str8 find)
{
  // NOTE: Algorithm as described in http://0x80.pl/articles/simd-strfind.html
  DN_Str8FindResult result = {};
  if (string.count == 0 || find.count == 0 || find.count > string.count)
    return result;

  __m512i const find_first_ch = _mm512_set1_epi8(find.data[0]);
  __m512i const find_last_ch  = _mm512_set1_epi8(find.data[find.count - 1]);

  DN_USize const search_size     = string.count - find.count;
  DN_USize       simd_iterations = search_size / sizeof(__m512i);
  char const    *ptr             = string.data;

  while (simd_iterations--) {
    __m512i find_first_ch_block = _mm512_loadu_si512(ptr);
    __m512i find_last_ch_block  = _mm512_loadu_si512(ptr + find.count - 1);

    // NOTE: AVX512F does not have a cmpeq so we use XOR to place a 0 bit
    // where matches are found.
    __m512i first_ch_matches = _mm512_xor_si512(find_first_ch_block, find_first_ch);

    // NOTE: We can combine the 2nd XOR and merge the 2 XOR results into one
    // operation using the ternarylogic intrinsic.
    //
    // A = first_ch_matches (find_first_ch_block ^ find_first_ch)
    // B = find_last_ch_block
    // C = find_last_ch
    //
    // ternarylogic op => A | (B ^ C) => 0b1111'0110 => 0xf6
    //
    // / A / B / C / B ^ C / A | (B ^ C) /
    // | 0 | 0 | 0 | 0     | 0           |
    // | 0 | 0 | 1 | 1     | 1           |
    // | 0 | 1 | 0 | 1     | 1           |
    // | 0 | 1 | 1 | 0     | 0           |
    // | 1 | 0 | 0 | 0     | 1           |
    // | 1 | 0 | 1 | 1     | 1           |
    // | 1 | 1 | 0 | 1     | 1           |
    // | 1 | 1 | 1 | 0     | 1           |

    __m512i ch_matches = _mm512_ternarylogic_epi32(first_ch_matches, find_last_ch_block, find_last_ch, 0xf6);

    // NOTE: Matches were XOR-ed and are hence indicated as zero so we mask
    // out which 32 bit elements in the vector had zero bytes. This uses a
    // bit twiddling trick
    // https://graphics.stanford.edu/~seander/bithacks.html#ZeroInWord
    __mmask16 zero_byte_mask = {};
    {
      const __m512i v01  = _mm512_set1_epi32(0x01010101u);
      const __m512i v80  = _mm512_set1_epi32(0x80808080u);
      const __m512i v1   = _mm512_sub_epi32(ch_matches, v01);
      const __m512i tmp1 = _mm512_ternarylogic_epi32(v1, ch_matches, v80, 0x20);
      zero_byte_mask     = _mm512_test_epi32_mask(tmp1, tmp1);
    }

    while (zero_byte_mask) {
      uint64_t const lsb_zero_pos = _tzcnt_u64(zero_byte_mask);
      char const    *base_ptr     = ptr + (4 * lsb_zero_pos);

      if (DN_Memcmp(base_ptr + 0, find.data, find.count) == 0) {
        result.found = true;
        result.index = base_ptr - string.data;
      } else if (DN_Memcmp(base_ptr + 1, find.data, find.count) == 0) {
        result.found = true;
        result.index = base_ptr - string.data + 1;
      } else if (DN_Memcmp(base_ptr + 2, find.data, find.count) == 0) {
        result.found = true;
        result.index = base_ptr - string.data + 2;
      } else if (DN_Memcmp(base_ptr + 3, find.data, find.count) == 0) {
        result.found = true;
        result.index = base_ptr - string.data + 3;
      }

      if (result.found) {
        result.start_to_before_match        = DN_Str8FromPtr(string.data, result.index);
        result.match                        = DN_Str8FromPtr(string.data + result.index, find.count);
        result.match_to_end_of_buffer       = DN_Str8FromPtr(result.match.data, string.count - result.index);
        result.after_match_to_end_of_buffer = DN_Str8Advance(result.match_to_end_of_buffer, find.count);
        return result;
      }

      zero_byte_mask = DN_BitClearNextLsb(zero_byte_mask);
    }

    ptr += sizeof(__m512i);
  }

  for (DN_USize index = ptr - string.data; index < string.count; index++) {
    DN_Str8 string_slice = DN_Str8Subset(string, index, find.count);
    if (DN_Str8Eq(string_slice, find)) {
      result.found                        = true;
      result.index                        = index;
      result.start_to_before_match        = DN_Str8FromPtr(string.data, index);
      result.match                        = DN_Str8FromPtr(string.data + index, find.count);
      result.match_to_end_of_buffer       = DN_Str8FromPtr(result.match.data, string.count - index);
      result.after_match_to_end_of_buffer = DN_Str8Advance(result.match_to_end_of_buffer, find.count);
      return result;
    }
  }

  return result;
}

DN_API DN_Str8FindResult DN_Str8FindLastStr8AVX512F(DN_Str8 string, DN_Str8 find)
{
  // NOTE: Algorithm as described in http://0x80.pl/articles/simd-strfind.html
  DN_Str8FindResult result = {};
  if (string.count == 0 || find.count == 0 || find.count > string.count)
    return result;

  __m512i const find_first_ch = _mm512_set1_epi8(find.data[0]);
  __m512i const find_last_ch  = _mm512_set1_epi8(find.data[find.count - 1]);

  DN_USize const search_size     = string.count - find.count;
  DN_USize       simd_iterations = search_size / sizeof(__m512i);
  char const    *ptr             = string.data + search_size + 1;

  while (simd_iterations--) {
    ptr -= sizeof(__m512i);
    __m512i find_first_ch_block = _mm512_loadu_si512(ptr);
    __m512i find_last_ch_block  = _mm512_loadu_si512(ptr + find.count - 1);

    // NOTE: AVX512F does not have a cmpeq so we use XOR to place a 0 bit
    // where matches are found.
    __m512i first_ch_matches = _mm512_xor_si512(find_first_ch_block, find_first_ch);

    // NOTE: We can combine the 2nd XOR and merge the 2 XOR results into one
    // operation using the ternarylogic intrinsic.
    //
    // A = first_ch_matches (find_first_ch_block ^ find_first_ch)
    // B = find_last_ch_block
    // C = find_last_ch
    //
    // ternarylogic op => A | (B ^ C) => 0b1111'0110 => 0xf6
    //
    // / A / B / C / B ^ C / A | (B ^ C) /
    // | 0 | 0 | 0 | 0     | 0           |
    // | 0 | 0 | 1 | 1     | 1           |
    // | 0 | 1 | 0 | 1     | 1           |
    // | 0 | 1 | 1 | 0     | 0           |
    // | 1 | 0 | 0 | 0     | 1           |
    // | 1 | 0 | 1 | 1     | 1           |
    // | 1 | 1 | 0 | 1     | 1           |
    // | 1 | 1 | 1 | 0     | 1           |

    __m512i ch_matches = _mm512_ternarylogic_epi32(first_ch_matches, find_last_ch_block, find_last_ch, 0xf6);

    // NOTE: Matches were XOR-ed and are hence indicated as zero so we mask
    // out which 32 bit elements in the vector had zero bytes. This uses a
    // bit twiddling trick
    // https://graphics.stanford.edu/~seander/bithacks.html#ZeroInWord
    __mmask16 zero_byte_mask = {};
    {
      const __m512i v01  = _mm512_set1_epi32(0x01010101u);
      const __m512i v80  = _mm512_set1_epi32(0x80808080u);
      const __m512i v1   = _mm512_sub_epi32(ch_matches, v01);
      const __m512i tmp1 = _mm512_ternarylogic_epi32(v1, ch_matches, v80, 0x20);
      zero_byte_mask     = _mm512_test_epi32_mask(tmp1, tmp1);
    }

    while (zero_byte_mask) {
      uint64_t const lsb_zero_pos = _tzcnt_u64(zero_byte_mask);
      char const    *base_ptr     = ptr + (4 * lsb_zero_pos);

      if (DN_Memcmp(base_ptr + 0, find.data, find.count) == 0) {
        result.found = true;
        result.index = base_ptr - string.data;
      } else if (DN_Memcmp(base_ptr + 1, find.data, find.count) == 0) {
        result.found = true;
        result.index = base_ptr - string.data + 1;
      } else if (DN_Memcmp(base_ptr + 2, find.data, find.count) == 0) {
        result.found = true;
        result.index = base_ptr - string.data + 2;
      } else if (DN_Memcmp(base_ptr + 3, find.data, find.count) == 0) {
        result.found = true;
        result.index = base_ptr - string.data + 3;
      }

      if (result.found) {
        result.start_to_before_match  = DN_Str8FromPtr(string.data, result.index);
        result.match                  = DN_Str8FromPtr(string.data + result.index, find.count);
        result.match_to_end_of_buffer = DN_Str8FromPtr(result.match.data, string.count - result.index);
        return result;
      }

      zero_byte_mask = DN_BitClearNextLsb(zero_byte_mask);
    }
  }

  for (DN_USize index = ptr - string.data - 1; index < string.count; index--) {
    DN_Str8 string_slice = DN_Str8Subset(string, index, find.count);
    if (DN_Str8Eq(string_slice, find)) {
      result.found                  = true;
      result.index                  = index;
      result.start_to_before_match  = DN_Str8FromPtr(string.data, index);
      result.match                  = DN_Str8FromPtr(string.data + index, find.count);
      result.match_to_end_of_buffer = DN_Str8FromPtr(result.match.data, string.count - index);
      return result;
    }
  }

  return result;
}

DN_API DN_Str8BSplitResult DN_Str8BSplitAVX512F(DN_Str8 string, DN_Str8 find)
{
  DN_Str8BSplitResult result      = {};
  DN_Str8FindResult        find_result = DN_Str8FindAVX512F(string, find);
  if (find_result.found) {
    result.lhs.data = string.data;
    result.lhs.count = find_result.index;
    result.rhs      = DN_Str8Advance(find_result.match_to_end_of_buffer, find.count);
  } else {
    result.lhs = string;
  }

  return result;
}

DN_API DN_Str8BSplitResult DN_Str8BSplitLastAVX512F(DN_Str8 string, DN_Str8 find)
{
  DN_Str8BSplitResult result      = {};
  DN_Str8FindResult   find_result = DN_Str8FindLastAVX512F(string, find);
  if (find_result.found) {
    result.lhs.data = string.data;
    result.lhs.count = find_result.index;
    result.rhs      = DN_Str8Advance(find_result.match_to_end_of_buffer, find.count);
  } else {
    result.lhs = string;
  }

  return result;
}

DN_API DN_USize DN_Str8SplitAVX512F(DN_Str8 string, DN_Str8 delimiter, DN_Str8 *splits, DN_USize splits_count, DN_Str8SplitFlags flags)
{
  DN_USize result = 0; // The number of splits in the actual string.
  if (string.count == 0 || delimiter.count == 0 || delimiter.count <= 0)
    return result;

  DN_Str8BSplitResult split = {};
  DN_Str8             first = string;
  do {
    split = DN_Str8BSplitAVX512F(first, delimiter);
    if (split.lhs.count || DN_BitIsNotSet(flags, DN_Str8SplitFlags_ExcludeEmptyStrings)) {
      if (splits && result < splits_count)
        splits[result] = split.lhs;
      result++;
    }
    first = split.rhs;
  } while (first.count);

  return result;
}

DN_API DN_Str8Slice DN_Str8SplitAllocAVX512F(DN_Arena *arena, DN_Str8 string, DN_Str8 delimiter, DN_Str8SplitFlags flags)
{
  DN_Str8Slice result               = {};
  DN_USize     splits_required      = DN_Str8SplitAVX512F(string, delimiter, /*splits*/ nullptr, /*count*/ 0, flags);
  result.data                       = DN_ArenaNewArray(arena, DN_Str8, splits_required, DN_ZMem_No);
  if (result.data) {
    result.count = DN_Str8SplitAVX512F(string, delimiter, result.data, splits_required, flags);
    DN_Assert(splits_required == result.count);
  }
  return result;
}
#endif // DN_STR8_AVX512F

DN_API DN_Str8 DN_Str8SliceRender(DN_Str8Slice slice, DN_Str8 seperator, DN_Arena *arena)
{
  DN_Str8 result = {};
  if (!arena)
    return result;

  DN_USize total_size = 0;
  for (DN_USize index = 0; index < slice.count; index++) {
    if (index)
      total_size += seperator.count;
    DN_Str8 item = slice.data[index];
    total_size += item.count;
  }

  result = DN_Str8AllocArena(total_size, DN_ZMem_No, arena);
  if (result.data) {
    DN_USize write_index = 0;
    for (DN_USize index = 0; index < slice.count; index++) {
      if (index) {
        DN_Memcpy(result.data + write_index, seperator.data, seperator.count);
        write_index += seperator.count;
      }
      DN_Str8 item = slice.data[index];
      DN_Memcpy(result.data + write_index, item.data, item.count);
      write_index += item.count;
    }
  }

  return result;
}

DN_API DN_Str8 DN_Str8RenderSpaceSep(DN_Str8Slice slice, DN_Arena *arena)
{
  DN_Str8 result = DN_Str8SliceRender(slice, DN_Str8Lit(" "), arena);
  return result;
}

DN_API int DN_Str8CompareNatural(DN_Str8 lhs, DN_Str8 rhs, DN_Str8EqCase eq_case)
{
  const char *lhs_it = lhs.data;
  const char *rhs_it = rhs.data;
  const char *lhs_end = lhs.data + lhs.count;
  const char *rhs_end = rhs.data + rhs.count;

  while (lhs_it < lhs_end && rhs_it < rhs_end) {
    // NOTE: Skip leading spaces
    while (lhs_it < lhs_end && DN_CharIsWhitespace(*lhs_it))
      lhs_it++;
    while (rhs_it < rhs_end && DN_CharIsWhitespace(*rhs_it))
      rhs_it++;

    if (lhs_it >= lhs_end || rhs_it >= rhs_end)
      break;

    // NOTE: Check if current positions are digits
    if (DN_CharIsDigit(*lhs_it) && DN_CharIsDigit(*rhs_it)) {
      // NOTE: Extract full number from lhs
      DN_U64 lhs_num = 0;
      while (lhs_it < lhs_end && DN_CharIsDigit(*lhs_it)) {
        lhs_num = lhs_num * 10 + (*lhs_it - '0');
        lhs_it++;
      }

      // NOTE: Extract full number from rhs
      DN_U64 rhs_num = 0;
      while (rhs_it < rhs_end && DN_CharIsDigit(*rhs_it)) {
        rhs_num = rhs_num * 10 + (*rhs_it - '0');
        rhs_it++;
      }

      if (lhs_num != rhs_num)
        return (lhs_num < rhs_num) ? -1 : 1;
    } else {
      // NOTE: Compare non-digit characters
      char lhs_ch = *lhs_it;
      char rhs_ch = *rhs_it;

      if (eq_case == DN_Str8EqCase_Insensitive) {
        if (DN_CharIsAlphabet(lhs_ch))
          lhs_ch = DN_CharToLower(lhs_ch);
        if (DN_CharIsAlphabet(rhs_ch))
          rhs_ch = DN_CharToLower(rhs_ch);
      }

      if (lhs_ch != rhs_ch)
        return (lhs_ch < rhs_ch) ? -1 : 1;
      lhs_it++;
      rhs_it++;
    }
  }

  // NOTE: One string is prefix of other; shorter comes first
  if (lhs_it < lhs_end)
    return 1;
  if (rhs_it < rhs_end)
    return -1;
  return 0;
}

DN_API int DN_Str8CompareLexicographic(DN_Str8 lhs, DN_Str8 rhs, DN_Str8EqCase eq_case)
{
  const char *lhs_it  = lhs.data;
  const char *rhs_it  = rhs.data;
  const char *lhs_end = lhs.data + lhs.count;
  const char *rhs_end = rhs.data + rhs.count;

  while (lhs_it < lhs_end && rhs_it < rhs_end) {
      char lhs_ch = *lhs_it;
      char rhs_ch = *rhs_it;
      if (eq_case == DN_Str8EqCase_Insensitive) {
        if (DN_CharIsAlphabet(lhs_ch))
          lhs_ch = DN_CharToLower(lhs_ch);
        if (DN_CharIsAlphabet(rhs_ch))
          rhs_ch = DN_CharToLower(rhs_ch);
      }
      if (lhs_ch != rhs_ch)
        return (lhs_ch < rhs_ch) ? -1 : 1;
      lhs_it++;
      rhs_it++;
  }

  // NOTE: One string is prefix of other; shorter comes first
  if (lhs.count < rhs.count)
    return -1;
  if (rhs.count < lhs.count)
    return 1;
  return 0;
}


DN_API bool DN_Str16Eq(DN_Str16 lhs, DN_Str16 rhs)
{
  if (lhs.count != rhs.count)
      return false;
  bool result = (DN_Memcmp(lhs.data, rhs.data, lhs.count) == 0);
  return result;
}


DN_API DN_Str16 DN_Str16SliceRender(DN_Str16Slice slice, DN_Str16 seperator, DN_Arena *arena)
{
  DN_Str16 result = {};
  if (!arena)
    return result;

  DN_USize total_size = 0;
  for (DN_USize index = 0; index < slice.count; index++) {
    if (index)
      total_size += seperator.count;
    DN_Str16 item = slice.data[index];
    total_size += item.count;
  }

  result = {DN_ArenaNewArray(arena, wchar_t, total_size + 1, DN_ZMem_No), total_size};
  if (result.data) {
    DN_USize write_index = 0;
    for (DN_USize index = 0; index < slice.count; index++) {
      if (index) {
        DN_Memcpy(result.data + write_index, seperator.data, seperator.count * sizeof(result.data[0]));
        write_index += seperator.count;
      }
      DN_Str16 item = slice.data[index];
      DN_Memcpy(result.data + write_index, item.data, item.count * sizeof(result.data[0]));
      write_index += item.count;
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

DN_API DN_Str8Builder DN_Str8BuilderFromStr8PtrRef(DN_Arena *arena, DN_Str8 const *strings, DN_USize count)
{
  DN_Str8Builder result = DN_Str8BuilderFromArena(arena);
  DN_Str8BuilderAppendArrayRef(&result, strings, count);
  return result;
}

DN_API DN_Str8Builder DN_Str8BuilderFromStr8PtrCopy(DN_Arena *arena, DN_Str8 const *strings, DN_USize count)
{
  DN_Str8Builder result = DN_Str8BuilderFromArena(arena);
  DN_Str8BuilderAppendArrayCopy(&result, strings, count);
  return result;
}

DN_API DN_Str8Builder DN_Str8BuilderFromBuilder(DN_Arena *arena, DN_Str8Builder const *builder)
{
  DN_Str8Builder result = DN_Str8BuilderFromArena(arena);
  DN_Str8BuilderAppendBuilderCopy(&result, builder);
  return result;
}

DN_API bool DN_Str8BuilderAddArrayRef(DN_Str8Builder *builder, DN_Str8 const *strings, DN_USize count, DN_Str8BuilderAdd add)
{
  if (!builder)
    return false;

  if (!strings || count <= 0)
    return true;

  // NOTE: Allocate the links
  DN_Str8Link *links = DN_ArenaNewArrayNoZ(builder->arena, DN_Str8Link, count);
  if (!links)
    return false;

  if (add == DN_Str8BuilderAdd_Append) {
    for (DN_ForIndexU(index, count)) {
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
      builder->string_size += string.count;
    }
  } else {
    DN_Assert(add == DN_Str8BuilderAdd_Prepend);
    DN_MSVC_WARNING_PUSH
    DN_MSVC_WARNING_DISABLE(6293) // NOTE: Ill-defined loop
    for (DN_USize index = count - 1; index < count; index--) {
      DN_MSVC_WARNING_POP
      DN_Str8      string = strings[index];
      DN_Str8Link *link   = links + index;
      link->string        = string;
      link->next          = builder->head;
      builder->head       = link;
      if (!builder->tail)
        builder->tail = link;
      builder->count++;
      builder->string_size += string.count;
    }
  }
  return true;
}

DN_API bool DN_Str8BuilderAddArrayCopy(DN_Str8Builder *builder, DN_Str8 const *strings, DN_USize count, DN_Str8BuilderAdd add)
{
  if (!builder)
    return false;

  if (!strings || count <= 0)
    return true;

  bool     result       = true;
  DN_U64   arena_p      = DN_MemListPos(builder->arena->mem);
  DN_Str8 *strings_copy = DN_ArenaNewArrayNoZ(builder->arena, DN_Str8, count);
  for (DN_ForIndexU(index, count)) {
    strings_copy[index] = DN_Str8FromStr8Arena(strings[index], builder->arena);
    if (strings_copy[index].count != strings[index].count) {
      result = false;
      break;
    }
  }

  if (result)
    result = DN_Str8BuilderAddArrayRef(builder, strings_copy, count, add);
  else
    DN_MemListPopTo(builder->arena->mem, arena_p);
  return result;
}

DN_API bool DN_Str8BuilderAddFV(DN_Str8Builder *builder, DN_Str8BuilderAdd add, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_Str8 string  = DN_Str8FmtVArena(builder->arena, fmt, args);
  DN_U64  arena_p = DN_MemListPos(builder->arena->mem);
  bool    result  = DN_Str8BuilderAddArrayRef(builder, &string, 1, add);
  if (!result)
    DN_MemListPopTo(builder->arena->mem, arena_p);
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

DN_API bool DN_Str8BuilderAppendBytesRef(DN_Str8Builder *builder, void const *ptr, DN_USize count)
{
  DN_Str8 input  = DN_Str8FromPtr(ptr, count);
  bool    result = DN_Str8BuilderAppendRef(builder, input);
  return result;
}

DN_API bool DN_Str8BuilderAppendBytesCopy(DN_Str8Builder *builder, void const *ptr, DN_USize count)
{
  DN_Str8 input  = DN_Str8FromPtr(ptr, count);
  bool    result = DN_Str8BuilderAppendCopy(builder, input);
  return result;
}

static bool DN_Str8BuilderAppendBuilder_(DN_Str8Builder *dest, DN_Str8Builder const *src, bool copy)
{
  if (!dest)
    return false;
  if (!src || src->string_size == 0)
    return true;

  DN_Arena     arena  = DN_ArenaTempBeginFromArena(dest->arena);
  DN_Str8Link *links  = DN_ArenaNewArrayNoZ(&arena, DN_Str8Link, src->count);
  bool         result = true;
  if (links) {
    DN_Str8Link *first      = nullptr;
    DN_Str8Link *last       = nullptr;
    DN_USize     link_index = 0;
    for (DN_Str8Link const *it = src->head; it; it = it->next) {
      DN_Str8Link *link = links + link_index++;
      link->next        = nullptr;
      link->string      = it->string;

      if (copy) {
        link->string = DN_Str8FromStr8Arena(it->string, &arena);
        if (link->string.count != it->string.count) {
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
  }
  DN_ArenaTempEnd(&arena, result ? DN_ArenaReset_No : DN_ArenaReset_Yes);
  return result;
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
    if (DN_Str8EqSensitive((*it)->string, string)) {
      *it = (*it)->next;
      builder->string_size -= string.count;
      builder->count -= 1;
      return true;
    }
  }
  return false;
}

DN_API DN_Str8 DN_Str8FromStr8BuilderAllocator(DN_Str8Builder const *builder, DN_Allocator allocator)
{
  DN_Str8 result = DN_Str8FromStr8BuilderDelimitAllocator(builder, DN_Str8Lit(""), allocator);
  return result;
}

DN_API DN_Str8 DN_Str8FromStr8BuilderArena(DN_Str8Builder const *builder, DN_Arena *arena)
{
  DN_Str8 result = DN_Str8FromStr8BuilderAllocator(builder, DN_AllocatorFromArena(arena));
  return result;
}

DN_API DN_Str8 DN_Str8FromStr8BuilderDelimitAllocator(DN_Str8Builder const *builder, DN_Str8 delimiter, DN_Allocator allocator)
{
  DN_Str8 result = {};
  if (!builder || builder->string_size <= 0 || builder->count <= 0)
    return result;

  DN_USize count_for_delimiter = delimiter.count ? ((builder->count - 1) * delimiter.count) : 0;
  result                      = DN_Str8AllocAllocator(builder->string_size + count_for_delimiter, DN_ZMem_No, allocator);
  if (!result.data)
    return result;

  DN_USize write_count = 0;
  for (DN_Str8Link *link = builder->head; link; link = link->next) {
    DN_Memcpy(result.data + write_count, link->string.data, link->string.count);
    write_count += link->string.count;
    if (link->next && delimiter.count) {
      DN_Memcpy(result.data + write_count, delimiter.data, delimiter.count);
      write_count += delimiter.count;
    }
  }

  result.data[write_count] = 0;
  DN_Assert(write_count == builder->string_size + count_for_delimiter);
  return result;
}

DN_API DN_Str8 DN_Str8FromStr8BuilderDelimitArena(DN_Str8Builder const *builder, DN_Str8 delimiter, DN_Arena *arena)
{
  DN_Str8 result = DN_Str8FromStr8BuilderDelimitAllocator(builder, delimiter, DN_AllocatorFromArena(arena));
  return result;
}

DN_API bool DN_PathAddRef(DN_Path *path, DN_Str8 add, DN_Arena *arena)
{
  if (!arena || !path)
    return false;

  if (add.count == 0)
    return true;

  DN_Str8 const delimiter_array[] = {
      DN_Str8Lit("\\"),
      DN_Str8Lit("/")};

  if (path->links_size == 0)
    path->has_prefix_path_seperator = (add.data[0] == '/');

  for (;;) {
    DN_Str8BSplitResult delimiter = DN_Str8BSplitArray(add, delimiter_array, DN_ArrayCountU(delimiter_array));
    for (; delimiter.lhs.data; delimiter = DN_Str8BSplitArray(delimiter.rhs, delimiter_array, DN_ArrayCountU(delimiter_array))) {
      if (delimiter.lhs.count <= 0)
        continue;

      DN_Str8Link *link = DN_ArenaNew(arena, DN_Str8Link, DN_ZMem_Yes);
      if (!link)
        return false;

      link->string = delimiter.lhs;
      link->prev   = path->tail;
      if (path->tail)
        path->tail->next = link;
      else
        path->head = link;
      path->tail = link;
      path->links_size += 1;
      path->string_size += delimiter.lhs.count;
    }

    if (!delimiter.lhs.data)
      break;
  }

  return true;
}

DN_API bool DN_PathAdd(DN_Path *path, DN_Str8 add, DN_Arena *arena)
{
  DN_Str8 copy   = DN_Str8FromStr8Arena(add, arena);
  bool    result = copy.count ? true : DN_PathAddRef(path, copy, arena);
  return result;
}

DN_API bool DN_PathAddF(DN_Path *path, DN_Arena *arena, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8 add = DN_Str8FmtVArena(arena, fmt, args);
  va_end(args);
  bool result = DN_PathAddRef(path, add, arena);
  return result;
}

DN_API bool DN_PathPop(DN_Path *path)
{
  if (!path)
    return false;

  if (path->tail) {
    DN_Assert(path->head);
    path->links_size -= 1;
    path->string_size -= path->tail->string.count;
    path->tail = path->tail->prev;
    if (path->tail)
      path->tail->next = nullptr;
    else
      path->head = nullptr;
  } else {
    DN_Assert(!path->head);
  }

  return true;
}

DN_API DN_Str8 DN_Str8FromPath(DN_Path const *path, DN_Str8 path_seperator, DN_Allocator allocator)
{
  DN_Str8 result = {};
  if (!path || path->links_size <= 0)
    return result;

  // NOTE: Each link except the last one needs the path seperator appended to it, '/' or '\\'
  DN_USize string_size = (path->has_prefix_path_seperator ? path_seperator.count : 0) + path->string_size + ((path->links_size - 1) * path_seperator.count);
  result               = DN_Str8AllocAllocator(string_size, DN_ZMem_No, allocator);
  if (result.data) {
    char *dest = result.data;
    if (path->has_prefix_path_seperator) {
      DN_Memcpy(dest, path_seperator.data, path_seperator.count);
      dest += path_seperator.count;
    }

    for (DN_Str8Link *link = path->head; link; link = link->next) {
      DN_Str8 string = link->string;
      DN_Memcpy(dest, string.data, string.count);
      dest += string.count;

      if (link != path->tail) {
        DN_Memcpy(dest, path_seperator.data, path_seperator.count);
        dest += path_seperator.count;
      }
    }
  }

  result.data[string_size] = 0;
  return result;
}

DN_API DN_Str8 DN_Str8FmtVPathArena(DN_Str8 path_seperator, DN_Arena *arena, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_TcScratch scratch   = DN_TcScratchBeginArena(&arena, 1);
  DN_Str8 path           = DN_Str8FmtVArena(arena, fmt, args);
  DN_Path fs_path        = {};
  DN_PathAddRef(&fs_path, path, &scratch.arena);
  DN_Allocator allocator = DN_AllocatorFromArena(arena);
  DN_Str8      result    = DN_Str8FromPath(&fs_path, path_seperator, allocator);
  DN_TcScratchEnd(&scratch);
  return result;
}

DN_API DN_Str8 DN_Str8FmtPathArena(DN_Str8 path_seperator, DN_Arena *arena, DN_FMT_ATTRIB char const *fmt, ...)
{
  DN_TcScratch scratch = DN_TcScratchBeginArena(&arena, 1);
  va_list args;
  va_start(args, fmt);
  DN_Str8 path = DN_Str8FmtVArena(arena, fmt, args);
  va_end(args);

  DN_Path fs_path = {};
  DN_PathAddRef(&fs_path, path, &scratch.arena);

  DN_Allocator allocator = DN_AllocatorFromArena(arena);
  DN_Str8      result    = DN_Str8FromPath(&fs_path, path_seperator, allocator);
  DN_TcScratchEnd(&scratch);
  return result;
}

DN_API DN_Str8 DN_Str8FmtVPathPool(DN_Str8 path_seperator, DN_Pool *pool, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_TcScratch scratch = DN_TcScratchBeginArena(&pool->arena, 1);
  DN_Str8 path         = DN_Str8FmtVPool(pool, fmt, args);
  DN_Path fs_path      = {};
  DN_PathAddRef(&fs_path, path, &scratch.arena);
  DN_Allocator allocator = DN_AllocatorFromPool(pool);
  DN_Str8      result    = DN_Str8FromPath(&fs_path, path_seperator, allocator);
  DN_TcScratchEnd(&scratch);
  return result;
}

DN_API DN_Str8 DN_Str8FmtPathPool(DN_Str8 path_seperator, DN_Pool *pool, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8 result = DN_Str8FmtVPathPool(path_seperator, pool, fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8 DN_Str8FmtOsPathPool(DN_Pool *pool, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8 result = DN_Str8FmtVPathPool(DN_OsPathSeperatorStr8, pool, fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8 DN_Str8FmtOsPathArena(DN_Arena *arena, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8 result = DN_Str8FmtVPathArena(DN_OsPathSeperatorStr8, arena, fmt, args);
  va_end(args);
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
    utf8[0] = DN_Cast(DN_U8) codepoint;
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
  if (stream.count <= 0)
    return result;

  DN_U8 b0 = DN_Cast(DN_U8)stream.data[0];
  DN_U8 b1 = DN_Cast(DN_U8)(stream.count >= 2 ? stream.data[1] : 0);
  DN_U8 b2 = DN_Cast(DN_U8)(stream.count >= 3 ? stream.data[2] : 0);
  DN_U8 b3 = DN_Cast(DN_U8)(stream.count >= 4 ? stream.data[3] : 0);

  if ((b0 & 0b1000'0000) == 0) {
    result.codepoint = b0;
    result.success   = true;
    result.remaining = DN_Str8FromPtr(stream.data + 1, stream.count - 1);
    return result;
  }

  if ((b0 & 0b1110'0000) == 0b1100'0000) {
    if (stream.count < 2)
      return result;
    if ((b1 & 0b1100'0000) != 0b1000'0000)
      return result;
    DN_U32 cp = ((b0 & 0b0001'1111) << 6) | ((b1 & 0b0011'1111) << 0);
    if (cp < 0x80)
      return result;
    result.codepoint  = cp;
    result.success   = true;
    result.remaining = DN_Str8FromPtr(stream.data + 2, stream.count - 2);
    return result;
  }

  if ((b0 & 0b1111'0000) == 0b1110'0000) {
    if (stream.count < 3)
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
    result.remaining = DN_Str8FromPtr(stream.data + 3, stream.count - 3);
    return result;
  }

  if ((b0 & 0b1111'1000) == 0b1111'0000) {
    if (stream.count < 4)
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
    result.remaining = DN_Str8FromPtr(stream.data + 4, stream.count - 4);
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

DN_API DN_USize DN_PtrBytesFromStr8Hex(DN_Str8 hex, void *dest, DN_USize dest_count)
{
  DN_Str8 hex_trimmed = DN_Str8TrimHexPrefix(hex);
  DN_USize result = 0;
  if (hex_trimmed.count > (dest_count * 2))
    return result;

  DN_U8   *ptr   = DN_Cast(DN_U8 *) dest;
  DN_USize index = 0;

  // NOTE: We are given an odd-sized hex string e.g.: 'F' instead of '0F', we 'left-pad' the parser
  // and support reading the single nibble as 'F'
  if (hex_trimmed.count % 2 != 0) {
    DN_U8 nibble0 = 0;
    DN_U8 nibble1 = DN_U8FromHexNibble(hex_trimmed.data[index++]);
    if (nibble1 == 0xFF)
      return result;
    *ptr++        = nibble0 << 4 | nibble1 << 0;
    result++;
  }

  // NOTE: Parse the rest of the hex which is in byte pairs
  for (; index < hex_trimmed.count; index += 2) {
    DN_U8 nibble0 = DN_U8FromHexNibble(hex_trimmed.data[index + 0]);
    DN_U8 nibble1 = DN_U8FromHexNibble(hex_trimmed.data[index + 1]);
    if (nibble0 == 0xFF || nibble1 == 0xFF)
      return result;
    *ptr++ = nibble0 << 4 | nibble1 << 0;
    result++;
  }
  return result;
}

DN_API DN_USize DN_PtrBytesFromPtrHex(char const *hex, DN_USize hex_count, void *dest, DN_USize dest_count)
{
  DN_USize result = DN_PtrBytesFromStr8Hex(DN_Str8FromPtr(hex, hex_count), dest, dest_count);
  return result;
}

DN_API DN_Str8 DN_Str8BytesFromStr8HexArena(DN_Str8 hex, DN_Arena *arena)
{
  DN_Str8 result = DN_Str8BytesFromPtrHexArena(hex.data, hex.count, arena);
  return result;
}

DN_API DN_Str8 DN_Str8BytesFromPtrHexArena(char const *hex, DN_USize hex_count, DN_Arena *arena)
{
  DN_Str8 hex_trimmed = DN_Str8TrimHexPrefix(DN_Str8FromPtr(hex, hex_count));
  DN_Assert(hex_trimmed.count % 2 == 0);
  DN_Str8 result = {};
  result.data    = DN_ArenaNewArray(arena, char, hex_trimmed.count / 2, DN_ZMem_No);
  if (result.data)
    result.count = DN_PtrBytesFromStr8Hex(hex_trimmed, result.data, hex_trimmed.count / 2);
  return result;
}

DN_API DN_Str8 DN_Str8BytesFromPtrHexPool(char const *hex, DN_USize hex_count, DN_Pool *pool)
{
  DN_Str8 hex_trimmed = DN_Str8TrimHexPrefix(DN_Str8FromPtr(hex, hex_count));
  DN_Assert(hex_trimmed.count % 2 == 0);
  DN_Str8 result = {};
  result.data    = DN_PoolNewArray(pool, char, hex_trimmed.count / 2);
  if (result.data)
    result.count = DN_PtrBytesFromStr8Hex(hex_trimmed, result.data, hex_trimmed.count / 2);
  return result;
}


DN_API DN_U8x16 DN_U8x16FromPtrHex32(char const *hex, DN_USize hex_count)
{
  DN_U8x16 result        = {};
  DN_Str8  hex_trimmed   = DN_Str8TrimHexPrefix(DN_Str8FromPtr(hex, hex_count));
  DN_USize bytes_written = DN_PtrBytesFromStr8Hex(hex_trimmed, result.data, sizeof result.data);
  DN_Assert(bytes_written == sizeof result.data);
  return result;
}

DN_API DN_U8x32 DN_U8x32FromPtrHex64(char const *hex, DN_USize hex_count)
{
  DN_U8x32 result        = {};
  DN_Str8  hex_trimmed   = DN_Str8TrimHexPrefix(DN_Str8FromPtr(hex, hex_count));
  DN_USize bytes_written = DN_PtrBytesFromStr8Hex(hex_trimmed, result.data, sizeof result.data);
  DN_Assert(bytes_written == sizeof result.data);
  return result;
}

DN_API DN_HexU64 DN_HexU64FromU64(DN_U64 value, DN_HexFromU64Type type)
{
  DN_HexU64 result = {};
  DN_USize  count  = DN_PtrHexFromPtrBytes(&value, sizeof(value), result.data, sizeof(result.data), DN_TrimLeadingZero_No);
  result.count     = DN_SaturateCastUSizeToU8(count);
  if (type == DN_HexFromU64Type_Uppercase) {
    for (DN_USize index = 0; index < result.count; index++)
      result.data[index] = DN_CharToUpper(result.data[index]);
  }
  return result;
}

DN_API DN_USize DN_PtrHexFromPtrBytes(void const *bytes, DN_USize bytes_count, void *hex, DN_USize hex_count, DN_TrimLeadingZero trim_leading_z)
{
  DN_USize result = 0;
  if ((bytes_count * 2) > hex_count)
    return result;
  DN_U8 const *src_u8        = DN_Cast(DN_U8 const *) bytes;
  DN_U8       *ptr           = DN_Cast(DN_U8 *) hex;
  bool         leading_zeros = true;

  for (DN_USize index = 0; index < bytes_count; index++) {
    char ch = src_u8[index];
    if (leading_zeros)
      leading_zeros = ch == 0;

    if (leading_zeros) {
      if (trim_leading_z == DN_TrimLeadingZero_Yes && ch == 0)
        continue;
    }

    DN_NibbleFromU8Result to_nibbles = DN_NibbleFromU8(ch);
    *ptr++                           = to_nibbles.nibble0;
    *ptr++                           = to_nibbles.nibble1;
    result += 2;
  }

  if (result == 0) {
    *ptr = '0';
    result++;
  }
  return result;
}

DN_API DN_Str8 DN_Str8HexFromPtrBytesArena(void const *bytes, DN_USize bytes_count, DN_Arena *arena, DN_TrimLeadingZero trim_leading_z)
{
  DN_Str8 result = {};
  if (bytes_count) {
    result.data = DN_ArenaNewArray(arena, char, bytes_count * 2, DN_ZMem_No);
    if (result.data)
      result.count = DN_PtrHexFromPtrBytes(bytes, bytes_count, result.data, bytes_count * 2, trim_leading_z);
  }
  return result;
}

DN_API DN_USize DN_PtrHexFromStr8Bytes(DN_Str8 bytes, void *hex, DN_USize hex_count, DN_TrimLeadingZero trim_leading_z)
{
  DN_USize result = DN_PtrHexFromPtrBytes(bytes.data, bytes.count, hex, hex_count, trim_leading_z);
  return result;
}

DN_API DN_Str8 DN_Str8HexFromStr8BytesArena(DN_Str8 bytes, DN_Arena *arena, DN_TrimLeadingZero trim_leading_z)
{
  DN_Str8 result = {};
  if (bytes.count) {
    result.data = DN_ArenaNewArray(arena, char, bytes.count * 2, DN_ZMem_No);
    if (result.data)
      result.count = DN_PtrHexFromStr8Bytes(bytes, result.data, bytes.count * 2, trim_leading_z);
  }
  return result;
}

DN_API DN_Hex32 DN_Hex32FromPtrBytes16(void const *bytes, DN_USize bytes_count, DN_TrimLeadingZero trim_leading_z)
{
  DN_Hex32 result = {};
  DN_Assert(bytes_count * 2 == sizeof result.data - 1);
  result.count = DN_PtrHexFromPtrBytes(bytes, bytes_count, result.data, sizeof result.data, trim_leading_z);
  DN_Assert(result.count <= sizeof result.data - 1);
  return result;
}

DN_API DN_Hex64 DN_Hex64FromPtrBytes32(void const *bytes, DN_USize bytes_count, DN_TrimLeadingZero trim_leading_z)
{
  DN_Hex64 result = {};
  DN_Assert(bytes_count * 2 == sizeof result.data - 1);
  result.count = DN_PtrHexFromPtrBytes(bytes, bytes_count, result.data, sizeof result.data, trim_leading_z);
  DN_Assert(result.count <= sizeof result.data - 1);
  return result;
}

DN_API DN_Hex64 DN_Hex64FromU8x32(DN_U8x32 const *value, DN_TrimLeadingZero trim_leading_z)
{
  DN_Hex64 result = DN_Hex64FromPtrBytes32(value->data, DN_ArrayCountU(value->data), trim_leading_z);
  return result;
}

DN_API DN_Hex128 DN_Hex128FromPtrBytes64(void const *bytes, DN_USize bytes_count, DN_TrimLeadingZero trim_leading_z)
{
  DN_Hex128 result = {};
  DN_Assert(bytes_count * 2 == sizeof result.data - 1);
  result.count = DN_PtrHexFromPtrBytes(bytes, bytes_count, result.data, sizeof result.data, trim_leading_z);
  DN_Assert(result.count <= sizeof result.data - 1);
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
      DN_FmtAppend(result.data, &result.count, sizeof(result.data), "%s%zu%.*s", result.count ? " " : "", value_usize, DN_Str8PrintFmt(unit_suffix));
  }

  if (units & DN_AgeUnit_Week) {
    unit_suffix             = DN_Str8Lit("w");
    DN_USize value_usize    = remainder_ms / (DN_SecFromWeeks(1) * 1000);
    remainder_ms           -= DN_SecFromWeeks(value_usize) * 1000;
    if (value_usize)
      DN_FmtAppend(result.data, &result.count, sizeof(result.data), "%s%zu%.*s", result.count ? " " : "", value_usize, DN_Str8PrintFmt(unit_suffix));
  }

  if (units & DN_AgeUnit_Day) {
    unit_suffix             = DN_Str8Lit("d");
    DN_USize value_usize    = remainder_ms / (DN_SecFromDays(1) * 1000);
    remainder_ms           -= DN_SecFromDays(value_usize) * 1000;
    if (value_usize)
      DN_FmtAppend(result.data, &result.count, sizeof(result.data), "%s%zu%.*s", result.count ? " " : "", value_usize, DN_Str8PrintFmt(unit_suffix));
  }

  if (units & DN_AgeUnit_Hr) {
    unit_suffix             = DN_Str8Lit("h");
    DN_USize value_usize    = remainder_ms / (DN_SecFromHours(1) * 1000);
    remainder_ms           -= DN_SecFromHours(value_usize) * 1000;
    if (value_usize)
      DN_FmtAppend(result.data, &result.count, sizeof(result.data), "%s%zu%.*s", result.count ? " " : "", value_usize, DN_Str8PrintFmt(unit_suffix));
  }

  if (units & DN_AgeUnit_Min) {
    unit_suffix             = DN_Str8Lit("m");
    DN_USize value_usize    = remainder_ms / (DN_SecFromMins(1) * 1000);
    remainder_ms           -= DN_SecFromMins(value_usize) * 1000;
    if (value_usize)
      DN_FmtAppend(result.data, &result.count, sizeof(result.data), "%s%zu%.*s", result.count ? " " : "", value_usize, DN_Str8PrintFmt(unit_suffix));
  }

  if (units & DN_AgeUnit_Sec) {
    unit_suffix = DN_Str8Lit("s");
    if (units & DN_AgeUnit_FractionalSec) {
      DN_F64 remainder_s = remainder_ms / 1000.0;
      DN_FmtAppend(result.data, &result.count, sizeof(result.data), "%s%.3f%.*s", result.count ? " " : "", remainder_s, DN_Str8PrintFmt(unit_suffix));
      remainder_ms = 0;
    } else {
      DN_USize value_usize  = remainder_ms / 1000;
      remainder_ms         -= DN_Cast(DN_USize)(value_usize * 1000);
      if (value_usize)
        DN_FmtAppend(result.data, &result.count, sizeof(result.data), "%s%zu%.*s", result.count ? " " : "", value_usize, DN_Str8PrintFmt(unit_suffix));
    }
  }

  if (units & DN_AgeUnit_Ms) {
    unit_suffix = DN_Str8Lit("ms");
    DN_Assert((units & DN_AgeUnit_FractionalSec) == 0);
    DN_USize value_usize  = remainder_ms;
    remainder_ms         -= value_usize;
    if (value_usize || result.count == 0)
      DN_FmtAppend(result.data, &result.count, sizeof(result.data), "%s%zu%.*s", result.count ? " " : "", value_usize, DN_Str8PrintFmt(unit_suffix));
  }

  if (result.count == 0)
    DN_FmtAppend(result.data, &result.count, sizeof(result.data), "0%.*s", DN_Str8PrintFmt(unit_suffix));
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

DN_API DN_Str8 DN_Str8FromByteType(DN_ByteType type)
{
  DN_Str8 result = DN_Str8Lit("");
  switch (type) {
    case DN_ByteType_B:     result = DN_Str8Lit("B");   break;
    case DN_ByteType_KiB:   result = DN_Str8Lit("KiB"); break;
    case DN_ByteType_MiB:   result = DN_Str8Lit("MiB"); break;
    case DN_ByteType_GiB:   result = DN_Str8Lit("GiB"); break;
    case DN_ByteType_TiB:   result = DN_Str8Lit("TiB"); break;
    case DN_ByteType_Count: result = DN_Str8Lit("");    break;
    case DN_ByteType_Auto:  result = DN_Str8Lit("");    break;
  }
  return result;
}

DN_API DN_ByteCount DN_ByteCountFromU64(DN_U64 bytes, DN_ByteType type)
{
  DN_Assert(type != DN_ByteType_Count);
  DN_ByteCount result = {};
  result.bytes              = DN_Cast(DN_F64) bytes;
  if (type == DN_ByteType_Auto)
    for (; result.type < DN_ByteType_Count && result.bytes >= 1024.0; result.type = DN_Cast(DN_ByteType)(DN_Cast(DN_USize) result.type + 1))
      result.bytes /= 1024.0;
  else
    for (; result.type < type; result.type = DN_Cast(DN_ByteType)(DN_Cast(DN_USize) result.type + 1))
      result.bytes /= 1024.0;
  result.suffix = DN_Str8FromByteType(result.type);
  return result;
}

DN_API DN_Str8x32 DN_Str8x32FromByteCountU64(DN_U64 bytes, DN_ByteType type)
{
  DN_ByteCount byte_count = DN_ByteCountFromU64(bytes, type);
  DN_Str8x32   result     = DN_Str8x32FromFmt("%.2f%.*s", byte_count.bytes, DN_Str8PrintFmt(byte_count.suffix));
  return result;
}

DN_API DN_Profiler DN_ProfilerInit(DN_ProfilerAnchor *anchors, DN_USize count, DN_USize anchors_per_frame, DN_ProfilerTscNowFunc *tsc_now, DN_U64 tsc_frequency)
{
  DN_Profiler result       = {};
  result.anchors           = anchors;
  result.anchors_count     = count;
  result.anchors_per_frame = anchors_per_frame;
  result.tsc_now           = tsc_now;
  result.tsc_frequency     = tsc_frequency;

  DN_AssertF(result.tsc_frequency != 0,
             "You must set this to the frequency of the timestamp counter function (Tsc) (e.g. how "
             "many ticks occur between timestamps). We use this to determine the duration between "
             "each zone's recorded Tsc. For example if the 'tsc_now' was set to Window's "
             "QueryPerformanceCounter then 'tsc_frequency' would be set to the value of "
             "QueryPerformanceFrequency which is typically 10mhz (e.g. The duration between two "
             "consecutive Tsc's is 10mhz)."
             ""
             "Hence frequency can't be zero otherwise it's a divide by 0. If you don't have a Tsc "
             "function and pass in null, the profiler defaults to rdtsc() and you must measure the "
             "frequency of rdtsc yourself. The reason for this is that measuring rdtsc requires "
             "having some alternate timing mechanism to measure the duration between the Tscs "
             "provided by rdtsc and this profiler makes no assumption about what timing primitives "
             "are available other than rdtsc which is a CPU builtin available on basically all "
             "platforms or have an equivalent (e.g. __builtin_readcyclecounter)"
             ""
             "This codebase provides DN_OS_EstimateTscPerSecond() as an example of how to that for "
             "convenience and is available if compiling with the OS layer. Some platforms like "
             "Emscripten don't support rdtsc() so you should use an alternative method like "
             "emscripten_get_now() or clock_gettime with CLOCK_MONOTONIC.");
  return result;
}

DN_API DN_USize DN_ProfilerFrameCount(DN_Profiler const *profiler)
{
  DN_USize result = profiler ? profiler->anchors_count / profiler->anchors_per_frame : 0;
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
  if (!profiler || profiler->paused)
    return result;

  if (anchor_index != 0) {
    DN_AssertF(profiler->frame_zone.profiler, "DN_ProfilerNewFrame() must be called before calling BeginZone");
  }
  DN_Assert(anchor_index < profiler->anchors_per_frame);
  DN_ProfilerAnchor *anchor = DN_ProfilerFrameAnchors(profiler).data + anchor_index;
  anchor->name              = name;

  // TODO: We need per-thread-local-storage profiler so that we can use these apis
  // across threads. For now, we let them overwrite each other but this is not tenable.
  #if 0
    if (anchor->name.count && anchor->name != name)
        DN_AssertF(name == anchor->name, "Potentially overwriting a zone by accident? Anchor is '%.*s', name is '%.*s'", DN_Str8PrintFmt(anchor->name), DN_Str8PrintFmt(name));
  #endif

  result.profiler                  = profiler;
  result.begin_tsc                 = profiler->tsc_now ? profiler->tsc_now() : DN_CPUGetTsc();
  result.anchor_index              = anchor_index;
  result.parent_zone               = profiler->parent_zone;
  result.elapsed_tsc_at_zone_start = anchor->tsc_inclusive;
  profiler->parent_zone            = anchor_index;
  return result;
}

DN_API void DN_ProfilerEndZone(DN_ProfilerZone zone)
{
  DN_Profiler *profiler = zone.profiler;
  if (!profiler || profiler->paused)
    return;

  DN_Assert(zone.anchor_index < profiler->anchors_per_frame);
  DN_Assert(zone.parent_zone < profiler->anchors_per_frame);

  DN_ProfilerAnchorArray array       = DN_ProfilerFrameAnchors(profiler);
  DN_ProfilerAnchor     *anchor      = array.data + zone.anchor_index;
  DN_U64                 tsc_now     = profiler->tsc_now ? profiler->tsc_now() : DN_CPUGetTsc();
  DN_U64                 elapsed_tsc = tsc_now - zone.begin_tsc;

  // NOTE: We snap the elapsed Tsc at the zone start and overwrite every time we end zones. If we
  // nest zones, the nested zones will clobber the inclusive timestamp with their values.
  // This is fine, as long as all the zones and begun and ended correctly, when the top-most zone
  // in the stack ends, it will overwrite the Tsc with the elapsed time overall for just that top
  // most function, unclobbering the elapsed time sitting in the anchor.
  anchor->tsc_inclusive  = zone.elapsed_tsc_at_zone_start + elapsed_tsc;
  anchor->tsc_exclusive += elapsed_tsc;
  anchor->hit_count++;

  if (zone.parent_zone != zone.anchor_index) {
    DN_ProfilerAnchor *parent_anchor  = array.data + zone.parent_zone;
    parent_anchor->tsc_exclusive     -= elapsed_tsc;
  }
  profiler->parent_zone = zone.parent_zone;
}

DN_API void DN_ProfilerNewFrame(DN_Profiler *profiler)
{
  if (!profiler || profiler->paused)
    return;

  // NOTE: End the frame's zone
  DN_ProfilerEndZone(profiler->frame_zone);
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

DN_API DN_USize DN_ProfilerFmtAnchor(DN_ProfilerAnchor anchor, DN_U64 tsc_frequency, char *buffer, DN_USize count)
{
  DN_USize result = 0;
  if (!anchor.hit_count)
    return result;

  DN_U64   tsc_exclusive              = anchor.tsc_exclusive;
  DN_U64   tsc_inclusive              = anchor.tsc_inclusive;
  DN_F64   tsc_exclusive_milliseconds = tsc_exclusive * 1000 / DN_Cast(DN_F64) tsc_frequency;
  if (tsc_exclusive == tsc_inclusive) {
    DN_FmtAppend(buffer, &result, count, "%.*s[%u]: %.1fms", DN_Str8PrintFmt(anchor.name), anchor.hit_count, tsc_exclusive_milliseconds);
  } else {
    DN_F64 tsc_inclusive_milliseconds = tsc_inclusive * 1000 / DN_Cast(DN_F64) tsc_frequency;
    DN_FmtAppend(buffer, &result, count, "%.*s[%u]: %.1f/%.1fms", DN_Str8PrintFmt(anchor.name), anchor.hit_count, tsc_exclusive_milliseconds, tsc_inclusive_milliseconds);
  }
  return result;
}

DN_API DN_Str8 DN_ProfilerFmtAnchorStr8(DN_ProfilerAnchor anchor, DN_U64 tsc_frequency, DN_Arena *arena)
{
  DN_Str8  result   = {};
  DN_USize count_req = DN_ProfilerFmtAnchor(anchor, tsc_frequency, nullptr, 0);
  if (count_req) {
    result = DN_Str8AllocArena(count_req, DN_ZMem_No, arena);
    DN_ProfilerFmtAnchor(anchor, tsc_frequency, result.data, result.count + 1);
  }
  return result;
}

DN_API void DN_ProfilerFmtToStdout(DN_Profiler *profiler)
{
  if (!profiler || profiler->frame_index == 0)
    return;

  DN_USize frame_index = profiler->frame_index - 1;
  DN_ProfilerAnchor *anchors = profiler->anchors + (frame_index * profiler->anchors_per_frame);
  for (DN_USize index = 1; index < profiler->anchors_per_frame; index++) {
    char buffer[2048];
    buffer[0]        = 0;
    DN_USize fmt_len = DN_ProfilerFmtAnchor(anchors[index], profiler->tsc_frequency, buffer, DN_ArrayCountU(buffer));
    DN_Str8  msg     = DN_Str8FromPtr(buffer, fmt_len);
    DN_OS_PrintOutLnF("%.*s", DN_Str8PrintFmt(msg));
  }
}

DN_API DN_F64 DN_ProfilerSecFromTsc(DN_Profiler *profiler, DN_U64 duration_tsc)
{
  DN_F64 result = DN_Cast(DN_F64)duration_tsc / profiler->tsc_frequency;
  return result;
}

DN_API DN_F64 DN_ProfilerMsFromTsc(DN_Profiler *profiler, DN_U64 duration_tsc)
{
  DN_F64 result = DN_Cast(DN_F64)duration_tsc / profiler->tsc_frequency * 1000.0;
  return result;
}

static void DN_QSortSetElem_(void *array, DN_USize elem_size, DN_USize dest_index, DN_USize src_index)
{
  char *src  = DN_Cast(char *) array + (src_index * elem_size);
  char *dest = DN_Cast(char *) array + (dest_index * elem_size);
  DN_Memcpy(dest, src, elem_size);
}

static void DN_QSortSwapElems_(void *array, DN_USize elem_size, DN_USize lhs_index, DN_USize rhs_index)
{
  if (lhs_index == rhs_index)
    return;

  char         temp_buffer[512];
  bool         use_buffer = elem_size <= DN_ArrayCountU(temp_buffer);
  DN_TcScratch scratch    = {};
  char        *temp       = {};
  if (use_buffer) {
    temp = temp_buffer;
  } else {
    scratch = DN_TcScratchBeginArena(nullptr, 0);
    temp    = DN_ArenaNewArray(&scratch.arena, char, elem_size, DN_ZMem_No);
  }

  char *lhs = DN_Cast(char *) array + (lhs_index * elem_size);
  char *rhs = DN_Cast(char *) array + (rhs_index * elem_size);
  DN_Memcpy(temp, lhs, elem_size);
  DN_Memcpy(lhs, rhs, elem_size);
  DN_Memcpy(rhs, temp, elem_size);

  if (!use_buffer)
    DN_TcScratchEnd(&scratch);
}

static void DN_QSortInsertion_(void *array, DN_USize array_size, DN_USize elem_size, void *user_context, DN_QSortCompareFunc *compare)
{
  char         temp_buffer[512];
  bool         use_buffer = elem_size <= DN_ArrayCountU(temp_buffer);
  DN_TcScratch scratch    = {};
  char        *temp       = {};
  if (use_buffer) {
    temp = temp_buffer;
  } else {
    scratch = DN_TcScratchBeginArena(nullptr, 0);
    temp    = DN_ArenaNewArray(&scratch.arena, char, elem_size, DN_ZMem_No);
  }

  DN_U8 *array_u8 = DN_Cast(DN_U8 *)array;
  for (DN_USize item_to_insert_index = 1; item_to_insert_index < array_size; item_to_insert_index++) {
    for (DN_USize index = 0; index < item_to_insert_index; index++) {
      DN_U8 *lhs = array_u8 + (index * elem_size);
      DN_U8 *rhs = array_u8 + (item_to_insert_index * elem_size);
      if (compare(lhs, rhs, user_context))
        continue;

      DN_Memcpy(temp, rhs, elem_size);
      for (DN_USize i = item_to_insert_index; i > index; i--)
        DN_QSortSetElem_(array, elem_size, i, i - 1);
      DN_Memcpy(lhs, temp, elem_size);
      break;
    }
  }

  if (!use_buffer)
    DN_TcScratchEnd(&scratch);
}

DN_API void DN_QSort_(void *array, DN_USize array_size, DN_USize elem_size, void *user_context, DN_QSortCompareFunc *compare)
{
  if (!array || array_size <= 1 || elem_size == 0 || !compare)
      return;

  // NOTE: Insertion Sort, under 24->32 is an optimal amount
  DN_U8          *array_u8       = DN_Cast(DN_U8 *)array;
  DN_USize const QSORT_THRESHOLD = 24;
  if (array_size < QSORT_THRESHOLD) {
    DN_QSortInsertion_(array, array_size, elem_size, user_context, compare);
    return;
  }

  // NOTE: Quick sort, under 24->32 is an optimal amount
  DN_USize last_index      = array_size - 1;
  DN_USize pivot_index     = array_size / 2;
  DN_USize partition_index = 0;
  DN_USize start_index     = 0;

  // Swap pivot with last index, so pivot is always at the end of the array.
  // This makes logic much simpler.
  DN_QSortSwapElems_(array, elem_size, last_index, pivot_index);
  pivot_index = last_index;

  // 4^, 8, 7, 5, 2, 3, 6
  if (compare(array_u8 + (start_index * elem_size), array_u8 + (pivot_index * elem_size), user_context))
    partition_index++;
  start_index++;

  // 4, |8, 7, 5^, 2, 3, 6*
  // 4, 5, |7, 8, 2^, 3, 6*
  // 4, 5, 2, |8, 7, ^3, 6*
  // 4, 5, 2, 3, |7, 8, ^6*
  for (DN_USize index = start_index; index < last_index; index++) {
    if (compare(array_u8 + (index * elem_size), array_u8 + (pivot_index * elem_size), user_context)) {
      DN_QSortSwapElems_(array, elem_size, partition_index, index);
      partition_index++;
    }
  }

  // Move pivot to right of partition
  // 4, 5, 2, 3, |6, 8, ^7*
  DN_QSortSwapElems_(array, elem_size, partition_index, pivot_index);
  DN_QSort_(array_u8, partition_index, elem_size, user_context, compare);

  // Skip the value at partion index since that is guaranteed to be sorted.
  // 4, 5, 2, 3, (x), 8, 7
  DN_USize one_after_partition_index = partition_index + 1;
  DN_QSort_(array_u8 + (one_after_partition_index * elem_size), (array_size - one_after_partition_index), elem_size, user_context, compare);
}

#if defined(__cplusplus)
template <typename T>
DN_API void DN_QSort(T *array, DN_USize array_size, void *user_context, DN_QSortCompareFunc *compare)
{
  DN_QSort_(array, array_size, sizeof(T), user_context, compare);
}
#endif

DN_API bool DN_QSortCompareStr8NaturalAsc(void const* lhs, void const *rhs, void *user_context)
{
  DN_Str8EqCase eq_case  = *DN_Cast(DN_Str8EqCase *) user_context;
  DN_Str8       lhs_str8 = *DN_Cast(DN_Str8 *) lhs;
  DN_Str8       rhs_str8 = *DN_Cast(DN_Str8 *) rhs;
  bool          result   = DN_Str8CompareNatural(lhs_str8, rhs_str8, eq_case) < 0;
  return result;
}

DN_API bool DN_QSortCompareStr8NaturalDesc(void const* lhs, void const *rhs, void *user_context)
{
  DN_Str8EqCase eq_case  = *DN_Cast(DN_Str8EqCase *) user_context;
  DN_Str8       lhs_str8 = *DN_Cast(DN_Str8 *) lhs;
  DN_Str8       rhs_str8 = *DN_Cast(DN_Str8 *) rhs;
  bool          result   = DN_Str8CompareNatural(lhs_str8, rhs_str8, eq_case) > 0;
  return result;
}

DN_API bool DN_QSortCompareStr8LexicographicAsc(void const* lhs, void const *rhs, void *user_context)
{
  DN_Str8EqCase eq_case  = *DN_Cast(DN_Str8EqCase *) user_context;
  DN_Str8       lhs_str8 = *DN_Cast(DN_Str8 *) lhs;
  DN_Str8       rhs_str8 = *DN_Cast(DN_Str8 *) rhs;
  bool          result   = DN_Str8CompareLexicographic(lhs_str8, rhs_str8, eq_case) < 0;
  return result;
}

DN_API bool DN_QSortCompareStr8LexicographicDesc(void const* lhs, void const *rhs, void *user_context)
{
  DN_Str8EqCase eq_case  = *DN_Cast(DN_Str8EqCase *) user_context;
  DN_Str8       lhs_str8 = *DN_Cast(DN_Str8 *) lhs;
  DN_Str8       rhs_str8 = *DN_Cast(DN_Str8 *) rhs;
  bool          result   = DN_Str8CompareLexicographic(lhs_str8, rhs_str8, eq_case) > 0;
  return result;
}

DN_API bool DN_QSortCompareBytesLT(void const* lhs, void const *rhs, void *user_context)
{
  DN_USize elem_size = *DN_Cast(DN_USize *)user_context;
  bool     result    = DN_Memcmp(lhs, rhs, elem_size) < 0;
  return result;
}

DN_API bool DN_QSortCompareBytesGT(void const* lhs, void const *rhs, void *user_context)
{
  DN_USize elem_size = *DN_Cast(DN_USize *)user_context;
  bool result = DN_Memcmp(lhs, rhs, elem_size) > 0;
  return result;
}

DN_API void DN_QSortBytesLT(void *array, DN_USize array_size, DN_USize elem_size)
{
  DN_QSort_(array, array_size, elem_size, &elem_size, DN_QSortCompareBytesLT);
}

DN_API void DN_QSortBytesGT(void *array, DN_USize array_size, DN_USize elem_size)
{
  DN_QSort_(array, array_size, elem_size, &elem_size, DN_QSortCompareBytesGT);
}

DN_API void DN_QSortStr8NaturalAsc(DN_Str8 *array, DN_USize array_size, DN_Str8EqCase eq_case)
{
  DN_QSort_(array, array_size, sizeof(*array), /*user_context=*/ &eq_case, DN_QSortCompareStr8NaturalAsc);
}

DN_API void DN_QSortStr8NaturalDesc(DN_Str8 *array, DN_USize array_size, DN_Str8EqCase eq_case)
{
  DN_QSort_(array, array_size, sizeof(*array), /*user_context=*/ &eq_case, DN_QSortCompareStr8NaturalDesc);
}

DN_API void DN_QSortStr8LexicographicAsc(DN_Str8 *array, DN_USize array_size, DN_Str8EqCase eq_case)
{
  DN_QSort_(array, array_size, sizeof(*array), /*user_context=*/ &eq_case, DN_QSortCompareStr8LexicographicAsc);
}

DN_API void DN_QSortStr8LexicographicDesc(DN_Str8 *array, DN_USize array_size, DN_Str8EqCase eq_case)
{
  DN_QSort_(array, array_size, sizeof(*array), /*user_context=*/ &eq_case, DN_QSortCompareStr8LexicographicDesc);
}

DN_API bool DN_BSearchLessThanBytes(void const *lhs, void const *rhs, void *user_context)
{
  DN_USize elem_size = *DN_Cast(DN_USize *)user_context;
  bool result        = DN_Memcmp(lhs, rhs, elem_size) < 0;
  return result;
}

DN_API DN_BSearchResult DN_BSearch(void const *array, DN_USize count, DN_USize elem_size, void const *find, DN_BSearchType type, void *user_context, DN_BSearchLessThanFunc *less_than)
{
  DN_BSearchResult result = {};
  if (!array || count <= 0 || !less_than || !find)
      return result;

  void const *end   = DN_Cast(char *)array + (count * elem_size);
  void const *first = array;
  void const *last  = end;
  while (first != last) {
    DN_USize dist = (DN_Cast(char *)last - DN_Cast(char *)first) / elem_size;
    void const *it = DN_Cast(char *)first + ((dist / 2) * elem_size);
    bool advance_first = false;
    if (type == DN_BSearchType_UpperBound)
      advance_first = !less_than(find, it, user_context);
    else
      advance_first = less_than(it, find, user_context);

    if (advance_first)
      first = DN_Cast(char *)it + (1 * elem_size);
    else
      last  = it;
  }

  switch (type) {
    case DN_BSearchType_Match: {
        result.found = first != end && !less_than(find, first, user_context);
    } break;

    case DN_BSearchType_LowerBound: /*FALLTHRU*/
    case DN_BSearchType_UpperBound: {
        result.found = first != end;
    } break;
  }

  result.index = (DN_Cast(char *)first - DN_Cast(char *)array) / elem_size;
  return result;
}

DN_API DN_BSearchResult DN_BSearchBytes(void const *array, DN_USize count, DN_USize elem_size, void const *find, DN_BSearchType type)
{
  DN_BSearchResult result = DN_BSearch(array, count, elem_size, find, type, &elem_size, DN_BSearchLessThanBytes);
  return result;
}

DN_API DN_BSearchResult DN_BSearchUSize(DN_USize const *array, DN_USize count, DN_USize find, DN_BSearchType type)
{
  DN_BSearchResult result = DN_BSearchBytes(array, count, sizeof(*array), &find, type);
  return result;
}

DN_API DN_BSearchResult DN_BSearchU64(DN_U64 const *array, DN_USize count, DN_U64 find, DN_BSearchType type)
{
  DN_BSearchResult result = DN_BSearchBytes(array, count, sizeof(*array), &find, type);
  return result;
}

DN_API DN_BSearchResult DN_BSearchU32(DN_U32 const *array, DN_USize count, DN_U32 find, DN_BSearchType type)
{
  DN_BSearchResult result = DN_BSearchBytes(array, count, sizeof(*array), &find, type);
  return result;
}

#define DN_PCG_DEFAULT_MULTIPLIER_64 6364136223846793005ULL
#define DN_PCG_DEFAULT_INCREMENT_64  1442695040888963407ULL
DN_API DN_Pcg32 DN_Pcg32Init(DN_U64 seed)
{
  DN_Pcg32 result = {};
  DN_Pcg32Next(&result);
  result.state += seed;
  DN_Pcg32Next(&result);
  return result;
}

DN_API DN_U32 DN_Pcg32Next(DN_Pcg32 *rng)
{
  DN_U64 state = rng->state;
  rng->state     = state * DN_PCG_DEFAULT_MULTIPLIER_64 + DN_PCG_DEFAULT_INCREMENT_64;

  // XSH-RR
  DN_U32 value = (DN_U32)((state ^ (state >> 18)) >> 27);
  int      rot   = state >> 59;
  return rot ? (value >> rot) | (value << (32 - rot)) : value;
}

DN_API DN_U64 DN_Pcg32Next64(DN_Pcg32 *rng)
{
  DN_U64 value = DN_Pcg32Next(rng);
  value <<= 32;
  value |= DN_Pcg32Next(rng);
  return value;
}

DN_API DN_U32 DN_Pcg32Range(DN_Pcg32 *rng, DN_U32 low, DN_U32 high)
{
  DN_U32 bound     = high - low;
  DN_U32 threshold = -(DN_I32)bound % bound;

  for (;;) {
    DN_U32 r = DN_Pcg32Next(rng);
    if (r >= threshold)
      return low + (r % bound);
  }
}

DN_API DN_F32 DN_Pcg32NextF32(DN_Pcg32 *rng)
{
  DN_U32 x = DN_Pcg32Next(rng);
  return (DN_F32)(DN_I32)(x >> 8) * 0x1.0p-24f;
}

DN_API DN_F64 DN_Pcg32NextF64(DN_Pcg32 *rng)
{
  DN_U64 x = DN_Pcg32Next64(rng);
  return (DN_F64)(DN_I64)(x >> 11) * 0x1.0p-53;
}

DN_API void DN_Pcg32Advance(DN_Pcg32 *rng, DN_U64 delta)
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
DN_API DN_U32 DN_Fnv1aHashU32FromBytes(void const *bytes, DN_USize count, DN_U32 hash)
{
  auto buffer = DN_Cast(DN_U8 const *)bytes;
  for (DN_USize i = 0; i < count; i++)
    hash = (buffer[i] ^ hash) * 16777619 /*FNV Prime*/;
  return hash;
}

DN_API DN_U64 DN_Fnv1aHashU64FromBytes(void const *bytes, DN_USize count, DN_U64 hash)
{
    auto buffer = DN_Cast(DN_U8 const *)bytes;
    for (DN_USize i = 0; i < count; i++)
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
DN_FORCE_INLINE DN_U32 DN_Murmur3GetBlock32_(DN_U32 const *p, int i)
{
    return p[i];
}

DN_FORCE_INLINE DN_U64 DN_Murmur3GetBlock64_(DN_U64 const *p, int i)
{
    return p[i];
}

//-----------------------------------------------------------------------------
// Finalization mix - force all bits of a hash block to avalanche

DN_FORCE_INLINE DN_U32 DN_Murmur3FMix32_(DN_U32 h)
{
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;
  return h;
}

DN_FORCE_INLINE DN_U64 DN_Murmur3FMix64_(DN_U64 k)
{
  k ^= k >> 33;
  k *= 0xff51afd7ed558ccd;
  k ^= k >> 33;
  k *= 0xc4ceb9fe1a85ec53;
  k ^= k >> 33;
  return k;
}

DN_API DN_U32 DN_Murmur3HashU32FromBytesX86(void const *bytes, int len, DN_U32 seed)
{
  const DN_U8 *data = (const DN_U8 *)bytes;
  const int nblocks   = len / 4;

  DN_U32 h1 = seed;

  const DN_U32 c1 = 0xcc9e2d51;
  const DN_U32 c2 = 0x1b873593;

  //----------
  // body

  const DN_U32 *blocks = (const DN_U32 *)(data + nblocks * 4);

  for (int i = -nblocks; i; i++) {
    DN_U32 k1 = DN_Murmur3GetBlock32_(blocks, i);

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

  switch (len & 3) {
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

  h1 = DN_Murmur3FMix32_(h1);

  return h1;
}

DN_API DN_Murmur3 DN_Murmur3HashU128FromBytesX64(void const *bytes, int len, DN_U32 seed)
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

  for (int i = 0; i < nblocks; i++) {
    DN_U64 k1 = DN_Murmur3GetBlock64_(blocks, i * 2 + 0);
    DN_U64 k2 = DN_Murmur3GetBlock64_(blocks, i * 2 + 1);

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

  switch (len & 15) {
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

  h1 = DN_Murmur3FMix64_(h1);
  h2 = DN_Murmur3FMix64_(h2);

  h1 += h2;
  h2 += h1;

  DN_Murmur3 result = {};
  result.e[0]       = h1;
  result.e[1]       = h2;
  return result;
}

DN_API DN_U64 DN_Murmur3HashU64FromBytesX64(void const *bytes, int len, DN_U32 seed)
{
  DN_Murmur3 hash   = DN_Murmur3HashU128FromBytesX64(bytes, len, seed);
  DN_U64     result = hash.e[0];
  return result;
}

DN_API DN_U32 DN_Murmur3HashU32FromBytesX64(void const *bytes, int len, DN_U32 seed)
{
  DN_Murmur3 hash   = DN_Murmur3HashU128FromBytesX64(bytes, len, seed);
  DN_U32     result = DN_Cast(DN_U32)hash.e[0];
  return result;
}

DN_API DN_Str8x32 DN_Str8x32FromAnsiColourCodeU8Rgb(DN_AnsiColourMode mode, DN_U8 r, DN_U8 g, DN_U8 b)
{
  DN_Str8x32 result = DN_Str8x32FromFmt("\x1b[%d;2;%u;%u;%um",
                                        mode == DN_AnsiColourMode_Fg ? 38 : 48,
                                        r,
                                        g,
                                        b);
  return result;
}

DN_API DN_Str8x32 DN_Str8x32FromAnsiColourCodeV3F32Rgb255(DN_AnsiColourMode mode, DN_V3F32 rgb_255)
{
  DN_Str8x32 result = DN_Str8x32FromAnsiColourCodeU8Rgb(mode, DN_Cast(DN_U8)rgb_255.r, DN_Cast(DN_U8)rgb_255.g, DN_Cast(DN_U8)rgb_255.b);
  return result;
}

DN_API DN_Str8x32 DN_Str8x32FromAnsiColourCodeU32Rgb(DN_AnsiColourMode mode, DN_U32 value)
{
  DN_U8      r      = DN_Cast(DN_U8)(value >> 24);
  DN_U8      g      = DN_Cast(DN_U8)(value >> 16);
  DN_U8      b      = DN_Cast(DN_U8)(value >> 8);
  DN_Str8x32 result = DN_Str8x32FromAnsiColourCodeU8Rgb(mode, r, g, b);
  return result;
}

DN_API DN_Str8 DN_Str8FromStr8AnsiColourU8RgbArena(DN_AnsiColourMode mode, DN_Str8 str8, DN_U8 r, DN_U8 g, DN_U8 b, DN_Arena *arena)
{
  DN_Str8x32 ansi   = DN_Str8x32FromAnsiColourCodeU8Rgb(mode, r, g, b);
  DN_Str8    result = DN_Str8FmtArena(arena, "%.*s%.*s%s", DN_Str8PrintFmt(ansi), DN_Str8PrintFmt(str8), DN_AnsiCodeResetLit);
  return result;
}

DN_API DN_Str8 DN_Str8FromStr8AnsiColourV3F32Rgb255Arena(DN_AnsiColourMode mode, DN_Str8 str8, DN_V3F32 rgb_255, DN_Arena *arena)
{
  DN_Str8 result = DN_Str8FromStr8AnsiColourU8RgbArena(mode, str8, DN_Cast(DN_U8)rgb_255.r, DN_Cast(DN_U8)rgb_255.g, DN_Cast(DN_U8)rgb_255.b, arena);
  return result;
}

DN_API DN_Str8 DN_Str8AnsiColourU8RgbFromFmtVArena(DN_AnsiColourMode mode, DN_U8 r, DN_U8 g, DN_U8 b, DN_Arena *arena, char const *fmt, va_list args)
{
  DN_TcScratch scratch = DN_TcScratchBeginArena(&arena, 1);
  DN_Str8      string  = DN_Str8FmtVArena(&scratch.arena, fmt, args);
  DN_Str8      result  = DN_Str8FromStr8AnsiColourU8RgbArena(mode, string, r, g, b, arena);
  DN_TcScratchEnd(&scratch);
  return result;
}

DN_API DN_Str8 DN_Str8FmtAnsiColourU8RgbArena(DN_AnsiColourMode mode, DN_U8 r, DN_U8 g, DN_U8 b, DN_Arena *arena, char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8 result = DN_Str8AnsiColourU8RgbFromFmtVArena(mode, r, g, b, arena, fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8 DN_Str8FmtAnsiColourV3F32Rgb255Arena(DN_AnsiColourMode mode, DN_V3F32 rgb_255, DN_Arena *arena, char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8 result = DN_Str8AnsiColourU8RgbFromFmtVArena(mode, DN_Cast(DN_U8)rgb_255.r, DN_Cast(DN_U8)rgb_255.g, DN_Cast(DN_U8)rgb_255.b, arena, fmt, args);
  va_end(args);
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
  max_type_length                 = DN_Max(max_type_length, type_str8.count);
  int type_padding                = DN_Cast(int)(max_type_length - type_str8.count);

  DN_Str8x32 colour_esc = {};
  DN_Str8    bold_esc   = {};
  DN_Str8    reset_esc  = {};
  if (style.colour) {
    bold_esc   = DN_Str8Lit(DN_AnsiCodeBoldLit);
    reset_esc  = DN_Str8Lit(DN_AnsiCodeResetLit);
    colour_esc = DN_Str8x32FromAnsiColourCodeU8Rgb(DN_AnsiColourMode_Fg, style.r, style.g, style.b);
  }

  DN_Str8 file_name = DN_Str8FileNameFromPath(call_site.file);
  int     size      = DN_Snprintf(dest,
                         DN_Cast(int)dest_size,
                         "%04u-%02u-%02uT%02u:%02u:%02u" // date
                         "%.*s"                          // colour
                         "%.*s"                          // bold
                         " %.*s"                         // type
                         "%.*s"                          // type padding
                         "%.*s"                          // reset
                         " %.*s"                         // file name
                         ":%05u "                        // line number
                         ,
                         date.year,
                         date.month,
                         date.day,
                         date.hour,
                         date.minute,
                         date.second,
                         DN_Str8PrintFmt(colour_esc), // colour
                         DN_Str8PrintFmt(bold_esc),   // bold
                         DN_Str8PrintFmt(type_str8),  // type
                         DN_Cast(int) type_padding,
                         "",                          // type padding
                         DN_Str8PrintFmt(reset_esc),  // reset
                         DN_Str8PrintFmt(file_name),  // file name
                         call_site.line); // line number

  static DN_USize max_header_length  = 0;
  DN_USize        size_no_ansi_codes = size - colour_esc.count - reset_esc.count - bold_esc.count;
  max_header_length                  = DN_Max(max_header_length, size_no_ansi_codes);
  DN_USize header_padding            = max_header_length - size_no_ansi_codes;

  DN_LogPrefixSize result = {};
  result.count            = size;
  result.padding          = header_padding;
  return result;
}

DN_API void DN_LogSetPrintFunc(DN_LogPrintFunc *print_func, void *user_data)
{
  DN_Core *dn            = DN_Get();
  dn->print_func         = print_func;
  dn->print_func_context = user_data;
}

DN_API void DN_LogPrintFV(DN_LogTypeParam type, DN_CallSite call_site, DN_LogFlags flags, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_Core *dn = DN_Get();
  if (type.is_u32_enum) {
    DN_Assert(dn->log_level_to_show_from >= 0);
    if (type.u32 < DN_Cast(DN_U32) dn->log_level_to_show_from)
      return;
  }
  DN_LogPrintFunc *func = dn->print_func;
  if (func)
    func(type, dn->print_func_context, call_site, flags, fmt, args);
}

DN_API void DN_LogPrintF(DN_LogTypeParam type, DN_CallSite call_site, DN_LogFlags flags, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_LogPrintFV(type, call_site, flags, fmt, args);
  va_end(args);
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

DN_API DN_V2F32 DN_V2F32Rotate(DN_V2F32 v, DN_F32 cos_a, DN_F32 sin_a)
{
  DN_V2F32 result = DN_V2F32From2N(v.x * cos_a - v.y * sin_a, v.x * sin_a + v.y * cos_a);
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

DN_API bool DN_V4F32Rgba01IsValid(DN_V4F32 rgba01)
{
  bool result = rgba01.r >= 0 && rgba01.r <= 1.f &&
                rgba01.g >= 0 && rgba01.g <= 1.f &&
                rgba01.b >= 0 && rgba01.b <= 1.f &&
                rgba01.a >= 0 && rgba01.a <= 1.f;
  return result;
}

DN_API DN_V4F32 DN_V4F32Rgba01FromRgbU32(DN_U32 u32)
{
  DN_U8    r      = (DN_U8)((u32 & 0x00FF0000) >> 16);
  DN_U8    g      = (DN_U8)((u32 & 0x0000FF00) >> 8);
  DN_U8    b      = (DN_U8)((u32 & 0x000000FF) >> 0);
  DN_V4F32 result = DN_V4F32Rgba01FromRgbU8(r, g, b);
  return result;
}

DN_API DN_V4F32 DN_V4F32Rgba01FromRgbaU32(DN_U32 u32)
{
  DN_U8    r      = (DN_U8)((u32 & 0xFF000000) >> 24);
  DN_U8    g      = (DN_U8)((u32 & 0x00FF0000) >> 16);
  DN_U8    b      = (DN_U8)((u32 & 0x0000FF00) >> 8);
  DN_U8    a      = (DN_U8)((u32 & 0x000000FF) >> 0);
  DN_V4F32 result = DN_V4F32Rgba01FromRgbaU8(r, g, b, a);
  return result;
}

#define DN_Srgb_COEFFICIENT_F32 2.2f
DN_API DN_V4F32 DN_V4F32Linear01FromSrgb01(DN_V4F32 srgb01)
{
  DN_Assert(srgb01.x >= 0.f && srgb01.x <= 1.f);
  DN_Assert(srgb01.y >= 0.f && srgb01.y <= 1.f);
  DN_Assert(srgb01.z >= 0.f && srgb01.z <= 1.f);
  DN_Assert(srgb01.a >= 0.f && srgb01.a <= 1.f);
  DN_V4F32 result = {};
  result.r        = DN_PowF32(srgb01.r, DN_Srgb_COEFFICIENT_F32);
  result.g        = DN_PowF32(srgb01.g, DN_Srgb_COEFFICIENT_F32);
  result.b        = DN_PowF32(srgb01.b, DN_Srgb_COEFFICIENT_F32);
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

DN_API DN_V4F32 DN_V4F32Srgb01FromLinear01(DN_V4F32 linear01)
{
  DN_Assert(linear01.x >= 0.f && linear01.x <= 1.f);
  DN_Assert(linear01.y >= 0.f && linear01.y <= 1.f);
  DN_Assert(linear01.z >= 0.f && linear01.z <= 1.f);
  DN_Assert(linear01.a >= 0.f && linear01.a <= 1.f);
  DN_V4F32 result = {};
  result.r        = DN_PowF32(linear01.r, 1.f / DN_Srgb_COEFFICIENT_F32);
  result.g        = DN_PowF32(linear01.g, 1.f / DN_Srgb_COEFFICIENT_F32);
  result.b        = DN_PowF32(linear01.b, 1.f / DN_Srgb_COEFFICIENT_F32);
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
        DN_FmtAppend(result.data, &result.count, sizeof(result.data), "|");
      DN_FmtAppend(result.data, &result.count, sizeof(result.data), "%.5f", mat.columns[it][row]);
      if (it != 3)
        DN_FmtAppend(result.data, &result.count, sizeof(result.data), ", ");
      else
        DN_FmtAppend(result.data, &result.count, sizeof(result.data), "|\n");
    }
  }
  return result;
}

DN_API bool DN_M2x3Eq(DN_M2x3 const *lhs, DN_M2x3 const *rhs)
{
  bool result = DN_Memcmp(lhs->e, rhs->e, sizeof(lhs->e[0]) * DN_ArrayCountU(lhs->e)) == 0;
  return result;
}

DN_API bool DN_M2x3NotEq(DN_M2x3 const *lhs, DN_M2x3 const *rhs)
{
  bool result = !DN_M2x3Eq(lhs, rhs);
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
  DN_2V2F32 rect_range   = DN_RectRange(rect);
  DN_V2F32  m1_min       = DN_M2x3MulV2F32(m1, rect_range.min);
  DN_V2F32  m1_max       = DN_M2x3MulV2F32(m1, rect_range.max);

  // NOTE: Re-establish AABB of the rectangle because it has gone through an arbitrary
  // vertex transformation.
  DN_2V2F32 result_range = {};
  result_range.min       = DN_V2F32Min(m1_min, m1_max);
  result_range.max       = DN_V2F32Max(m1_min, m1_max);

  DN_Rect   result       = DN_RectFrom2V2(result_range.min, DN_V2F32Abs(result_range.max - result_range.min));
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

typedef struct DN_ArrayFindEqMemcmpContext_ DN_ArrayFindEqMemcmpContext_;
struct DN_ArrayFindEqMemcmpContext_
{
  DN_USize    elem_size;
  void const *find;
};

DN_API void *DN_SliceAllocArena(void **data, DN_USize *slice_size_field, DN_USize count, DN_USize elem_size, DN_U8 align, DN_ZMem zmem, DN_Arena *arena)
{
  void *result = *data;
  *data        = DN_ArenaAlloc(arena, count * elem_size, align, zmem);
  if (*data)
    *slice_size_field = count;
  return result;
}

DN_API DN_ArrayFindResult DN_ArrayFind(void *data, DN_USize count, DN_USize elem_size, void const *find, DN_ArrayFindEqFunc *eq_func)
{
  DN_ArrayFindResult result = {};
  DN_Assert(data);
  DN_Assert(elem_size);
  if (find) {
    for (DN_ForIndexU(index, count)) {
      DN_U8 *it = DN_Cast(DN_U8 *) data + (index * elem_size);
      if (eq_func(it, find)) {
        result.index   = index;
        result.value   = it;
        result.success = true;
        break;
      }
    }
  }
  return result;
}

static bool DN_ArrayFindEqMemEqUnsafe_(void const *lhs, void const *find)
{
  DN_ArrayFindEqMemcmpContext_ *context = DN_Cast(DN_ArrayFindEqMemcmpContext_ *) find;
  bool                          result  = DN_MemEqUnsafe(lhs, context->find, context->elem_size);
  return result;
}

DN_API DN_ArrayFindResult DN_ArrayFindMemEq(void *data, DN_USize count, DN_USize elem_size, void const *find)
{
  DN_ArrayFindEqMemcmpContext_ context = {};
  context.elem_size                    = elem_size;
  context.find                         = find;
  DN_ArrayFindResult result            = DN_ArrayFind(data, count, elem_size, &context, DN_ArrayFindEqMemEqUnsafe_);
  return result;
}

DN_API void *DN_ArrayInsertArray(void *data, DN_USize *size, DN_USize max, DN_USize elem_size, DN_USize index, void const *items, DN_USize count)
{
  void *result = nullptr;
  if (!data || !size || !items || count <= 0 || ((*size + count) > max))
    return result;

  DN_USize clamped_index = DN_Min(index, *size);
  if (clamped_index != *size) {
    char const *src           = DN_Cast(char *)data + (clamped_index * elem_size);
    char const *dest          = DN_Cast(char *)data + ((clamped_index + count) * elem_size);
    char const *end           = DN_Cast(char *)data + (size[0] * elem_size);
    DN_USize    bytes_to_move = end - src;
    DN_Memmove(DN_Cast(void *) dest, src, bytes_to_move);
  }

  result = DN_Cast(char *)data + (clamped_index * elem_size);
  DN_Memcpy(result, items, elem_size * count);
  *size += count;
  return result;
}

DN_API void *DN_ArrayPopFront(void *data, DN_USize *size, DN_USize elem_size, DN_USize count)
{
  if (!data || !size || *size == 0 || count == 0)
    return nullptr;

  DN_USize pop_count = DN_Min(count, *size);
  void *result = data;

  if (pop_count < *size) {
    char *src = DN_Cast(char *)data + (pop_count * elem_size);
    char *dest = DN_Cast(char *)data;
    DN_USize bytes_to_move = (*size - pop_count) * elem_size;
    DN_Memmove(dest, src, bytes_to_move);
  }

  *size -= pop_count;
  return result;
}

DN_API void *DN_ArrayPopBack(void *data, DN_USize *size, DN_USize elem_size, DN_USize count)
{
  if (!data || !size || *size == 0 || count == 0)
    return nullptr;

  DN_USize pop_count = DN_Min(count, *size);
  *size -= pop_count;

  return DN_Cast(char *)data + (*size * elem_size);
}

DN_API DN_ArrayEraseResult DN_ArrayEraseRange(void *data, DN_USize *count, DN_USize elem_size, DN_USize begin_index, DN_ISize erase_count, DN_ArrayErase erase)
{
  DN_ArrayEraseResult result = {};
  result.it_index            = begin_index;
  if (!data || !count || *count == 0 || erase_count == 0)
    return result;

  // Compute the range to erase
  DN_USize start = 0, end = 0;
  if (erase_count < 0) {
    // Erase backwards from begin_index, not inclusive of begin_index
    // Range: [begin_index + count, begin_index)
    // Which is: [begin_index - abs(count), begin_index)
    DN_USize abs_erase_count = DN_Abs(erase_count);
    start = (begin_index > abs_erase_count) ? (begin_index - abs_erase_count) : 0;
    end   = begin_index;
  } else {
    start = begin_index;
    end   = begin_index + erase_count;
  }

  // Clamp indices to valid bounds
  start = DN_Min(start, *count);
  end   = DN_Min(end,   *count);

  // Erase the range [start, end)
  DN_USize real_erase_count = end > start ? end - start : 0;
  if (real_erase_count) {
    char    *dest      = (char *)data + (elem_size * start);
    char    *array_end = (char *)data + (elem_size * *count);
    char    *src       = dest + (elem_size * real_erase_count);
    if (erase == DN_ArrayErase_Stable) {
      DN_USize move_size = array_end - src;
      DN_Memmove(dest, src, move_size);
    } else {
      char    *unstable_src = array_end - (elem_size * real_erase_count);
      DN_USize move_size    = array_end - unstable_src;
      DN_Memcpy(dest, unstable_src, move_size);
    }
    *count -= real_erase_count;
  }

  result.items_erased = real_erase_count;
  // NOTE: If we are erasing from the current index of the iterator to the end of the array then
  // there's no more elements in the array to iterate. So the returned index should b
  // one-past-last index
  if (begin_index == start && end >= *count) {
    result.it_index = *count;
  } else {
    result.it_index = start ? start - 1 : 0;
  }
  return result;
}

DN_API void *DN_ArrayMakeArray(void *data, DN_USize *count, DN_USize max, DN_USize elem_size, DN_USize make_count, DN_ZMem z_mem)
{
  void    *result   = nullptr;
  DN_USize new_count = *count + make_count;
  if (new_count <= max) {
    result = DN_Cast(char *) data + (elem_size * count[0]);
    *count  = new_count;
    if (z_mem == DN_ZMem_Yes)
      DN_Memset(result, 0, elem_size * make_count);
  }
  return result;
}

DN_API void *DN_ArrayMakeArrayAssert(void *data, DN_USize *count, DN_USize max, DN_USize elem_size, DN_USize make_count, DN_ZMem z_mem, DN_CallSite call_site)
{
  void *result = DN_ArrayMakeArray(data, count, max, elem_size, make_count, z_mem);
  DN_AssertCallSiteF(result, call_site, "Array out of space, failed to add %zu items: array=%p size=%zu max=%zu", make_count, data, *count, max);
  return result;
}

DN_API void *DN_ArrayMakeArrayArena(void **data, DN_USize *count, DN_USize *max, DN_USize elem_size, DN_Arena *arena, DN_USize make_count, DN_ZMem z_mem)
{
  void *result = nullptr;
  if (DN_ArrayPrepareArena(data, *count, max, elem_size, arena, make_count))
    result = DN_ArrayMakeArray(*data, count, *max, elem_size, make_count, z_mem);
  return result;
}

DN_API void *DN_ArrayMakeArrayPool(void **data, DN_USize *count, DN_USize *max, DN_USize elem_size, DN_Pool *pool, DN_USize make_count, DN_ZMem z_mem)
{
  void *result = nullptr;
  if (DN_ArrayPreparePool(data, *count, max, elem_size, pool, make_count))
    result = DN_ArrayMakeArray(*data, count, *max, elem_size, make_count, z_mem);
  return result;
}

DN_API void *DN_ArrayAddArray(void *data, DN_USize *count, DN_USize max, DN_USize elem_size, void const *elems, DN_USize elems_count, DN_ArrayAdd add)
{
  void *result = DN_ArrayMakeArray(data, count, max, elem_size, elems_count, DN_ZMem_No);
  if (result) {
    if (add == DN_ArrayAdd_Append) {
      DN_Memcpy(result, elems, elems_count * elem_size);
    } else {
      char *move_dest = DN_Cast(char *)data + (elems_count * elem_size); // Shift elements forward
      char *move_src  = DN_Cast(char *)data;
      DN_Memmove(move_dest, move_src, elem_size * count[0]);
      DN_Memcpy(data, elems, elem_size * elems_count);
    }
  }
  return result;
}

DN_API void *DN_ArrayAddArrayArena(void **data, DN_USize *count, DN_USize *max, DN_USize elem_size, DN_Arena *arena, void const *elems, DN_USize elems_count, DN_ArrayAdd add)
{
  void *result = nullptr;
  if (DN_ArrayPrepareArena(data, *count, max, elem_size, arena, elems_count))
    result = DN_ArrayAddArray(*data, count, *max, elem_size, elems, elems_count, add);
  return result;
}

DN_API void *DN_ArrayAddArrayPool(void **data, DN_USize *count, DN_USize *max, DN_USize elem_size, DN_Pool *pool, void const *elems, DN_USize elems_count, DN_ArrayAdd add)
{
  void *result = nullptr;
  if (DN_ArrayPreparePool(data, *count, max, elem_size, pool, elems_count))
    result = DN_ArrayAddArray(*data, count, *max, elem_size, elems, elems_count, add);
  return result;
}

DN_API void *DN_ArrayAddArrayAssert(void *data, DN_USize *count, DN_USize max, DN_USize elem_size, void const *elems, DN_USize elems_count, DN_ArrayAdd add, DN_CallSite call_site)
{
  void *result = DN_ArrayAddArray(data, count, max, elem_size, elems, elems_count, add);
  DN_AssertCallSiteF(result, call_site, "Array out of space, failed to add %zu items: array=%p size=%zu max=%zu", elems_count, data, *count, max);
  return result;
}

static bool DN_ArrayResizeAllocator_(void **data, DN_USize *count, DN_USize *max, DN_USize elem_size, DN_Allocator allocator, DN_USize new_max)
{
  bool result = true;
  if (!max || new_max != *max) {
    DN_USize bytes_to_alloc = elem_size * new_max;
    void    *buffer         = DN_AllocatorAlloc(allocator, bytes_to_alloc, alignof(DN_UPtr), DN_ZMem_No);
    if (buffer) {
      DN_USize bytes_to_copy = elem_size * DN_Min(*count, new_max);
      DN_Memcpy(buffer, *data, bytes_to_copy);
      if (allocator.type == DN_AllocatorType_Pool)
        DN_PoolDealloc(DN_Cast(DN_Pool *)allocator.context, *data);
      *data  = buffer;
      *count = DN_Min(*count, new_max);
      if (max)
        *max = new_max;
    } else {
      result = false;
    }
  }

  return result;
}

DN_API bool DN_ArrayResizeArena(void **data, DN_USize *count, DN_USize *max, DN_USize elem_size, DN_Arena *arena, DN_USize new_max)
{
  DN_Allocator allocator = DN_AllocatorFromArena(arena);
  bool result            = DN_ArrayResizeAllocator_(data, count, max, elem_size, allocator, new_max);
  return result;
}

DN_API bool DN_ArrayResizePool(void **data, DN_USize *count, DN_USize *max, DN_USize elem_size, DN_Pool *pool, DN_USize new_max)
{
  DN_Allocator allocator = DN_AllocatorFromPool(pool);
  bool result            = DN_ArrayResizeAllocator_(data, count, max, elem_size, allocator, new_max);
  return result;
}

DN_API bool DN_ArrayReservePool(void **data, DN_USize *max, DN_USize elem_size, DN_Pool *pool, DN_USize new_max)
{
  bool result = true;
  if (!max || new_max > *max) {
    DN_USize count = 0;
    result = DN_ArrayResizePool(data, &count, max, elem_size, pool, new_max);
  }
  return result;
}

DN_API bool DN_ArrayReserveArena(void **data, DN_USize *max, DN_USize elem_size, DN_Arena *arena, DN_USize new_max)
{
  bool result = true;
  if (!max || new_max > *max) {
    DN_USize count = 0;
    result = DN_ArrayResizeArena(data, &count, max, elem_size, arena, new_max);
  }
  return result;
}

DN_API bool DN_ArrayPreparePool(void **data, DN_USize count, DN_USize *max, DN_USize elem_size, DN_Pool *pool, DN_USize add_count)
{
  bool     result   = true;
  DN_USize new_count = count + add_count;
  if (new_count > *max) {
    DN_USize new_max = DN_Max(DN_Max(*max * 2, new_count), 8);
    result           = DN_ArrayResizePool(data, &count, max, elem_size, pool, new_max);
  }
  return result;
}

DN_API bool DN_ArrayPrepareArena(void **data, DN_USize count, DN_USize *max, DN_USize elem_size, DN_Arena *arena, DN_USize add_count)
{
  bool     result   = true;
  DN_USize new_count = count + add_count;
  if (new_count > *max) {
    DN_USize new_max = DN_Max(DN_Max(*max * 2, new_count), 8);
    result           = DN_ArrayResizeArena(data, &count, max, elem_size, arena, new_max);
  }
  return result;
}

DN_API DN_USize DN_ArrayCopyPtrArena(void **data, void const *src, DN_USize count, DN_USize elem_size, DN_Arena *arena)
{
  DN_USize result = 0;
  if (DN_ArrayReserveArena(data, /*max=*/ &result, elem_size, arena, /*new_max=*/ count))
    DN_Memcpy(*data, src, elem_size * count);
  return result;
}

DN_API DN_USize DN_ArrayCopyPtrArenaAssert(void **data, void const *src, DN_USize count, DN_USize elem_size, DN_Arena *arena, DN_CallSite call_site)
{
  DN_USize result = DN_ArrayCopyPtrArena(data, src, count, elem_size, arena);
  DN_AssertCallSiteF(result == count, call_site, "Array copy failed, failed to allocate: %zu items", count);
  return result;
}

DN_API DN_USize DN_ArrayCopyPtrPool(void **data, void const *src, DN_USize count, DN_USize elem_size, DN_Pool *pool)
{
  DN_USize result = 0;
  if (DN_ArrayReservePool(data, /*max=*/ &result, elem_size, pool, /*new_max=*/ count))
    DN_Memcpy(*data, src, elem_size * count);
  return result;
}

DN_API DN_USize DN_ArrayCopyPtrPoolAssert(void **data, void const *src, DN_USize count, DN_USize elem_size, DN_Pool *pool, DN_CallSite call_site)
{
  DN_USize result = DN_ArrayCopyPtrPool(data, src, count, elem_size, pool);
  DN_AssertCallSiteF(result == count, call_site, "Array copy failed, failed to allocate: %zu items", count);
  return result;
}

DN_API void *DN_SinglyLLDetach(void **link, void **next)
{
  void *result = *link;
  if (*link) {
    *link = *next;
    *next = nullptr;
  }
  return result;
}

DN_API bool DN_RingHasSpace(DN_Ring const *ring, DN_U64 size)
{
  DN_U64 avail  = ring->write_pos - ring->read_pos;
  DN_U64 space  = ring->size - avail;
  bool   result = space >= size;
  return result;
}

DN_API bool DN_RingHasData(DN_Ring const *ring, DN_U64 size)
{
  DN_U64 data   = ring->write_pos - ring->read_pos;
  bool   result = data >= size;
  return result;
}

DN_API void DN_RingWrite(DN_Ring *ring, void const *src, DN_U64 src_size)
{
  DN_AssertF(src_size <= ring->size,
            "Payload to write (%s) to ring exceeds the capacity of the ring (%s)",
            DN_Str8x32FromByteCountU64Auto(src_size).data,
            DN_Str8x32FromByteCountU64Auto(ring->size).data);
  DN_U64 offset               = ring->write_pos % ring->size;
  DN_U64 bytes_before_split   = ring->size - offset;
  DN_U64 pre_split_bytes      = DN_Min(bytes_before_split, src_size);
  DN_U64 post_split_bytes     = src_size - pre_split_bytes;
  void const *pre_split_data  = src;
  void const *post_split_data = (DN_Cast(char *)src + pre_split_bytes);
  DN_Memcpy(ring->base + offset, pre_split_data,  pre_split_bytes);
  DN_Memcpy(ring->base,          post_split_data, post_split_bytes);
  ring->write_pos += src_size;
}

DN_API void DN_RingRead(DN_Ring *ring, void *dest, DN_U64 dest_size)
{
  DN_Assert(dest_size <= ring->size);
  DN_U64 offset             = ring->read_pos % ring->size;
  DN_U64 bytes_before_split = ring->size - offset;
  DN_U64 pre_split_bytes    = DN_Min(bytes_before_split, dest_size);
  DN_U64 post_split_bytes   = dest_size - pre_split_bytes;
  DN_Memcpy(dest,                           ring->base + offset, pre_split_bytes);
  DN_Memcpy((char *)dest + pre_split_bytes, ring->base,          post_split_bytes);
  ring->read_pos += dest_size;
}

DN_API DN_U32 DN_HTableHashFuncMurmur3KeyBytes(void const *key, DN_USize size)
{
  DN_U32 const DEFAULT_SEED = 0xb255b383;
  DN_U32       result       = DN_Murmur3HashU32FromBytes(key, DN_Cast(int)size, DEFAULT_SEED);
  return result;
}

DN_API DN_U32 DN_HTableHashFuncMurmur3KeyStr8(void const *key, DN_USize size)
{
  DN_Assert(size == sizeof(DN_Str8));
  DN_Str8 *str8_key = DN_Cast(DN_Str8 *)key;
  DN_U32 result     = DN_HTableHashFuncMurmur3KeyBytes(str8_key->data, str8_key->count);
  return result;
}

DN_API bool DN_HTableKeyEqFuncMemcmp(void const *lhs, void const *rhs, DN_USize size)
{
  bool result = DN_Memcmp(lhs, rhs, size) == 0;
  return result;
}

DN_API bool DN_HTableKeyEqFuncStr8Eq(void const *lhs, void const *rhs, DN_USize)
{
  DN_Str8 lhs_str8 = *DN_Cast(DN_Str8*) lhs;
  DN_Str8 rhs_str8 = *DN_Cast(DN_Str8*) rhs;
  bool    result   = DN_Str8EqSensitive(lhs_str8, rhs_str8);
  return result;
}

DN_API DN_HTableInitArgs DN_HTableInitArgsDefault_(void** kvs, DN_USize size_of_kv, DN_USize offset_of_hash, DN_USize size_of_key, DN_USize offset_of_key, DN_USize size_of_value, DN_USize offset_of_value)
{
  DN_HTableInitArgs result = {};
  DN_AssertF((*kvs) == 0, "The key-value pointer must be null as the hash table will allocate the objects and keep the pointer in sync with the table for you: %p", *kvs);
  result.hash_func         = DN_HTableHashFuncMurmur3KeyBytes;
  result.key_eq_func       = DN_HTableKeyEqFuncMemcmp;
  result.load_factor       = 0.7f;
  result.max               = 0;
  result.kvs               = kvs;
  result.size_of_kv        = size_of_kv;
  result.offset_of_key     = offset_of_key;
  result.offset_of_hash    = offset_of_hash;
  result.size_of_key       = size_of_key;
  result.offset_of_value   = offset_of_value;
  result.size_of_value     = size_of_value;
  return result;
}

DN_API DN_HTable DN_HTableInit_(DN_HTableInitArgs args)
{
  DN_HTable result   = {};
  result.load_factor01   = args.load_factor;
  result.hash_func       = args.hash_func;
  result.key_eq_func     = args.key_eq_func;
  result.kvs             = args.kvs;
  result.size_of_kv      = args.size_of_kv;
  result.offset_of_key   = args.offset_of_key;
  result.offset_of_hash  = args.offset_of_hash;
  result.size_of_key     = args.size_of_key;
  result.offset_of_value = args.offset_of_value;
  result.size_of_value   = args.size_of_value;
  return result;
}

DN_API DN_HTableInitResult DN_HTableInitHeap(DN_HTableInitArgs args, DN_Heap heap)
{
  DN_HTableInitResult result  = {};
  result.table                = DN_HTableInit_(args);
  result.table.heap           = heap;
  result.table.flags         |= DN_HTableFlags_UsingHeap;
  result.success              = DN_HTableResize(&result.table, args.max);
  return result;
}

DN_API DN_HTable DN_HTableInitHeapAssert(DN_HTableInitArgs args, DN_Heap heap)
{
  DN_HTableInitResult init   = DN_HTableInitHeap(args, heap);
  DN_HTable           result = init.table;
  DN_Assert(init.success);
  return result;
}

DN_API DN_HTableInitResult DN_HTableInitPool(DN_HTableInitArgs args, DN_Pool pool, DN_HTableDeallocPoolOnDeinit dealloc_pool)
{
  DN_HTableInitResult result = {};
  result.table               = DN_HTableInit_(args);
  result.table.pool          = pool;
  result.success             = DN_HTableResize(&result.table, args.max);
  if (dealloc_pool == DN_HTableDeallocPoolOnDeinit_Yes)
    result.table.flags |= DN_HTableFlags_DeallocPoolOnDeinit;
  if (!result.success)
    result.table = {};
  return result;
}

DN_API DN_HTable DN_HTableInitPoolAssert(DN_HTableInitArgs args, DN_Pool pool, DN_HTableDeallocPoolOnDeinit dealloc_pool)
{
  DN_HTableInitResult init   = DN_HTableInitPool(args, pool, dealloc_pool);
  DN_HTable           result = init.table;
  DN_Assert(init.success);
  return result;
}

DN_API void DN_HTableDeinit(DN_HTable *table)
{
  if ((table->flags & DN_HTableFlags_UsingHeap)) {
    DN_HeapDealloc(&table->heap, *table->kvs, table->max * table->size_of_kv);
  } else {
    DN_PoolDealloc(&table->pool, *table->kvs);
    if (table->flags & DN_HTableFlags_DeallocPoolOnDeinit)
      DN_ArenaDeinit(table->pool.arena);
  }
  *table->kvs = nullptr;
  *table      = {};
}

static DN_HTableSlot DN_HTableSlotFromArgs_(void *kvs, DN_USize size_of_kv, DN_USize offset_of_hash, DN_USize offset_of_key, DN_USize offset_of_value, DN_USize index)
{
  DN_HTableSlot result = {};
  result.kvs_index     = index;
  result.kv_struct     = (DN_Cast(char*) kvs) + (index * size_of_kv);
  result.key           = DN_Cast(char*) result.kv_struct + offset_of_key;
  result.value         = DN_Cast(char*) result.kv_struct + offset_of_value;
  result.hash          = DN_Cast(char*) result.kv_struct + offset_of_hash;
  result.hash_u32      = *DN_Cast(DN_HTableHashType*) result.hash;
  return result;
}

DN_API DN_HTableLookupResult DN_HTableLookup(DN_HTable const *table, void const *key, DN_HTableAllowTombstone allow_tombstone)
{
  DN_HTableLookupResult result = {};
  if (table->max == 0 || !key)
    return result;

  DN_Assert(DN_IsPowerOfTwo(table->max));
  DN_U32                hash            = table->hash_func(key, table->size_of_key);
  if (!DN_HTableHashIsValue(hash))
    hash += DN_HTableHashSentinel_FirstValid;
  DN_USize const        mask            = table->max - 1;
  DN_USize              index           = hash & mask;
  DN_USize              probe_increment = 1;
  for (DN_USize offset = 0; offset < table->max; offset++, probe_increment++) {
    DN_HTableSlot slot    = DN_HTableSlotFromIndex(table, index);
    bool          matched = DN_HTableSlotIsEmpty(slot);
    if (!matched && allow_tombstone == DN_HTableAllowTombstone_Yes)
      matched = DN_HTableSlotIsTomb(slot);
    if (!matched)
      matched = DN_HTableSlotIsValue(slot) && slot.hash_u32 == hash && table->key_eq_func(slot.key, key, table->size_of_key);
    if (matched) {
      result.slot     = slot;
      result.key_hash = hash;
      break;
    }
    // NOTE: We use triangular number probing, referenced off jblow's implementation. Triangular
    // numbers are guaranteed to hit every entry in the table. Triangular probing can cause more
    // collisions but avoids the localised clustering of collisions when linear probing. It is less
    // cache-friendly than linear but more cache-friendly than double hashing.
    // https://fgiesen.wordpress.com/2015/02/22/triangular-numbers-mod-2n/
    index = (index + probe_increment) & mask;
  }
  return result;
}

DN_API DN_HTableSlot DN_HTableFind(DN_HTable const *table, void const *key)
{
  DN_HTableLookupResult lookup = DN_HTableLookup(table, key, DN_HTableAllowTombstone_No);
  DN_HTableSlot         result = {};
  if (DN_HTableSlotIsValue(lookup.slot))
    result = lookup.slot;
  return result;
}

DN_API void* DN_HTableValueFromFind(DN_HTable const *table, void const *key)
{
  DN_HTableSlot slot    = DN_HTableFind(table, key);
  void          *result = slot.value; // Might be null if the key was not found
  return result;
}

DN_API DN_HTableSlot DN_HTableSlotFromIndex(DN_HTable const* table, DN_USize index)
{
  DN_HTableSlot result = {};
  if (index < table->max)
    result = DN_HTableSlotFromArgs_(*table->kvs, table->size_of_kv, table->offset_of_hash, table->offset_of_key, table->offset_of_value, index);
  return result;
}

DN_API bool DN_HTableResize(DN_HTable *table, DN_USize new_max)
{
  if (new_max == 0)
    new_max = 32;
  if (!DN_IsPowerOfTwo(new_max))
    new_max = DN_AlignUpPowerOfTwoUSize(new_max);

  void* new_kvs         = {};
  DN_USize new_kvs_size = new_max * table->size_of_kv;
  if (table->flags & DN_HTableFlags_UsingHeap)
    new_kvs = DN_HeapAlloc(&table->heap, /*reserve*/ new_kvs_size, /*commit*/ new_kvs_size);
  else
    new_kvs = DN_PoolAlloc(&table->pool, new_max * table->size_of_kv);

  if (new_kvs) {
    // NOTE: Copy over the old table slots into the new table storage
    void*    old_kvs   = *table->kvs;
    DN_USize old_max   = table->max;
    table->count       = table->tombs_count = 0;
    table->max         = new_max;
    *table->kvs        = new_kvs;
    for (DN_ForIndexU(index, old_max)) {
      DN_HTableSlot old_slot = DN_HTableSlotFromArgs_(old_kvs, table->size_of_kv, table->offset_of_hash, table->offset_of_key, table->offset_of_value, index);
      if (DN_HTableSlotIsValue(old_slot))
        DN_HTableAdd(table, old_slot.key, old_slot.value);
    }

    if (old_kvs) {
      if (table->flags & DN_HTableFlags_UsingHeap)
        DN_HeapDealloc(&table->heap, old_kvs, old_max * table->size_of_kv);
      else
        DN_PoolDealloc(&table->pool, old_kvs);
    }
  }

  bool result = new_kvs != nullptr;
  return result;
}

DN_API DN_HTablePrepareResult DN_HTablePrepare(DN_HTable *table, DN_USize add_count)
{
  DN_HTablePrepareResult result = {};
  result.success               = true;
  DN_USize new_count           = table->count + table->tombs_count + add_count;
  DN_F32   new_load_factor     = table->max ? new_count / DN_Cast(DN_F32) table->max : table->load_factor01;
  if (new_load_factor >= table->load_factor01) {
    // NOTE: In this branch, the total number of actively used slots (including tombstones) and the
    // `add_amount` exceeds the load factor of the table. We check if doubling the `count` of just
    // the _active slots_ in the table would still fit under the table's load factor. If it does
    // then the table is currently mostly tombstones, we can avoid doubling the size of the table by
    // rehashing which will wipe out the tombstones.
    DN_USize new_max = 0;
    if (((table->count * 2) + 1) < (table->max * table->load_factor01))
      new_max = table->max; // "Mostly tombstones" branch
    else
      new_max = table->max * 2;

    result.needed_resize = true;
    result.success       = DN_HTableResize(table, new_max);
  }
  return result;
}

DN_API void DN_HTableClear(DN_HTable *table)
{
  for (DN_ForIndexU(index, table->max)) {
    DN_HTableSlot slot = DN_HTableSlotFromIndex(table, index);
    DN_Memset(slot.hash, DN_HTableHashSentinel_Empty, sizeof(slot.hash_u32));
  }
  table->tombs_count = 0;
  table->count       = 0;
}

DN_API DN_HTableAddResult DN_HTableMake(DN_HTable* table, void* key)
{
  // NOTE: First do the lookup ignoring tombstones. This probes the chain looking for the existence
  // of the value.
  DN_HTableLookupResult lookup              = DN_HTableLookup(table, key, DN_HTableAllowTombstone_No);
  bool                  load_factor_is_good = true;
  if (DN_HTableSlotIsValue(lookup.slot)) {
    // NOTE: If it exists, this is the easy path, we can update the slot returned in the lookup.
    //
    // Note that we do not check if `hash_u32` is the empty value, because that empty value might
    // be at the end of a probe chain. If we were to use that slot, this key-value could potentially
    // be placed spatially far away from the optimal position (if there were tombstones closer
    // that we skipped over, we want to use those). Hence we take the else branch and do the lookup
    // again, this time allowing us to match the tombstone or the first empty slot.
  } else {
    // NOTE: The value does not exist, look it up again but match on the first tombstone/empty slot
    lookup                     = DN_HTableLookup(table, key, DN_HTableAllowTombstone_Yes);
    bool add_will_use_new_slot = DN_HTableSlotIsEmpty(lookup.slot);
    if (add_will_use_new_slot) {
      DN_HTablePrepareResult resize = DN_HTablePrepare(table, 1);
      if (resize.success)
        lookup = DN_HTableLookup(table, key, DN_HTableAllowTombstone_Yes);
      else
        load_factor_is_good = false;
    }
  }

  // NOTE: Update the table slot/kv
  DN_HTableAddResult result = {};
  if (load_factor_is_good) {
    result.success = true;
    result.slot    = lookup.slot;
    if (DN_HTableSlotIsEmpty(result.slot)) {
      table->count++;
    } else if (DN_HTableSlotIsTomb(result.slot)) {
      DN_Assert(table->tombs_count);
      table->tombs_count--;
      table->count++;
    } else {
      result.existed = true;
    }

    // NOTE: Update the slot with the given data
    if (result.existed) {
      DN_Assert(result.slot.hash_u32 == lookup.key_hash);
    } else {
      result.slot.hash_u32 = lookup.key_hash;
      DN_Assert(result.slot.hash);
      DN_Assert(result.slot.key);
      DN_Memcpy(result.slot.hash, &lookup.key_hash, sizeof(lookup.key_hash));
      DN_Memcpy(result.slot.key, key, table->size_of_key);
    }
  }
  return result;
}

DN_API DN_HTableAddResult DN_HTableAdd(DN_HTable* table, void* key, void* value)
{
  DN_HTableAddResult result = DN_HTableMake(table, key);
  if (result.success)
    DN_Memcpy(result.slot.value, value, table->size_of_value);
  return result;
}

DN_API bool DN_HTableDel(DN_HTable *table, void *key)
{
  DN_HTableLookupResult lookup = DN_HTableLookup(table, key, DN_HTableAllowTombstone_No);
  bool                  result = DN_HTableSlotIsValue(lookup.slot);
  if (result) {
    DN_HTableHashType sentinel = DN_HTableHashSentinel_Tomb;
    DN_Memcpy(lookup.slot.hash, &sentinel, sizeof(sentinel));
    table->tombs_count++;
    DN_Assert(table->count);
    table->count--;
  }
  return result;
}

DN_API void DN_BinPackU64(DN_BinPack *pack, DN_BinPackMode mode, DN_U64 *item)
{
  DN_U64 const VALUE_MASK   = 0b0111'1111;
  DN_U8 const  CONTINUE_BIT = 0b1000'0000;

  if (mode == DN_BinPackMode_Serialise) {
    DN_U64 it = *item;
    do {
      DN_U8 write_value = DN_Cast(DN_U8)(it & VALUE_MASK);
      it >>= 7;
      if (it)
        write_value |= CONTINUE_BIT;
      DN_Str8BuilderAppendBytesCopy(&pack->writer, &write_value, sizeof(write_value));
    } while (it);
  } else {
    *item              = 0;
    DN_USize bits_read = 0;
    for (DN_U8 src = CONTINUE_BIT; (src & CONTINUE_BIT) && bits_read < 64; bits_read += 7) {
      src              = pack->read.data[pack->read_index++];
      DN_U8 masked_src = src & VALUE_MASK;
      *item |= (DN_Cast(DN_U64) masked_src << bits_read);
    }
  }
}

DN_API void DN_BinPackVarInt_(DN_BinPack *pack, DN_BinPackMode mode, void *item, DN_USize count)
{
  DN_U64 value = 0;
  DN_AssertF(count <= sizeof(value),
             "An item larger than 64 bits (%zu) is trying to be packed as a variable integer which is not supported",
             count * 8);

  if (mode == DN_BinPackMode_Serialise) // Read `item` into U64 `value`
    DN_Memcpy(&value, item, count);

  DN_BinPackU64(pack, mode, &value);

  if (mode == DN_BinPackMode_Deserialise) // Write U64 `value` into `item`
    DN_Memcpy(item, &value, count);
}

DN_API bool DN_BinPackIsEndOfReadStream(DN_BinPack const *pack)
{
  bool result = pack->read_index == pack->read.count;
  return result;
}

DN_API void DN_BinPackUSize(DN_BinPack *pack, DN_BinPackMode mode, DN_USize *item)
{
  DN_BinPackVarInt_(pack, mode, item, sizeof(*item));
}

DN_API void DN_BinPackU32(DN_BinPack *pack, DN_BinPackMode mode, DN_U32 *item)
{
  DN_BinPackVarInt_(pack, mode, item, sizeof(*item));
}

DN_API void DN_BinPackU16(DN_BinPack *pack, DN_BinPackMode mode, DN_U16 *item)
{
  DN_BinPackVarInt_(pack, mode, item, sizeof(*item));
}

DN_API void DN_BinPackU8(DN_BinPack *pack, DN_BinPackMode mode, DN_U8 *item)
{
  DN_BinPackVarInt_(pack, mode, item, sizeof(*item));
}

DN_API void DN_BinPackI64(DN_BinPack *pack, DN_BinPackMode mode, DN_I64 *item)
{
  DN_BinPackVarInt_(pack, mode, item, sizeof(*item));
}

DN_API void DN_BinPackI32(DN_BinPack *pack, DN_BinPackMode mode, DN_I32 *item)
{
  DN_BinPackVarInt_(pack, mode, item, sizeof(*item));
}

DN_API void DN_BinPackI16(DN_BinPack *pack, DN_BinPackMode mode, DN_I16 *item)
{
  DN_BinPackVarInt_(pack, mode, item, sizeof(*item));
}

DN_API void DN_BinPackI8(DN_BinPack *pack, DN_BinPackMode mode, DN_I8 *item)
{
  DN_BinPackVarInt_(pack, mode, item, sizeof(*item));
}

DN_API void DN_BinPackF64(DN_BinPack *pack, DN_BinPackMode mode, DN_F64 *item)
{
  DN_BinPackVarInt_(pack, mode, item, sizeof(*item));
}

DN_API void DN_BinPackF32(DN_BinPack *pack, DN_BinPackMode mode, DN_F32 *item)
{
  DN_BinPackVarInt_(pack, mode, item, sizeof(*item));
}

DN_API void DN_BinPackV2(DN_BinPack *pack, DN_BinPackMode mode, DN_V2F32 *item)
{
  DN_BinPackF32(pack, mode, &item->x);
  DN_BinPackF32(pack, mode, &item->y);
}

DN_API void DN_BinPackV4(DN_BinPack *pack, DN_BinPackMode mode, DN_V4F32 *item)
{
  DN_BinPackF32(pack, mode, &item->x);
  DN_BinPackF32(pack, mode, &item->y);
  DN_BinPackF32(pack, mode, &item->z);
  DN_BinPackF32(pack, mode, &item->w);
}

DN_API void DN_BinPackBool(DN_BinPack *pack, DN_BinPackMode mode, bool *item)
{
  DN_BinPackVarInt_(pack, mode, item, sizeof(*item));
}

DN_API void DN_BinPackStr8FromArena(DN_BinPack *pack, DN_Arena *arena, DN_BinPackMode mode, DN_Str8 *string)
{
  DN_BinPackVarInt_(pack, mode, &string->count, sizeof(string->count));
  if (mode == DN_BinPackMode_Serialise) {
    DN_Str8BuilderAppendBytesCopy(&pack->writer, string->data, string->count);
  } else {
    DN_Str8 src       = DN_Str8Subset(pack->read, pack->read_index, string->count);
    *string           = DN_Str8FromStr8Arena(src, arena);
    pack->read_index += src.count;
  }
}

DN_API void DN_BinPackStr8FromPool(DN_BinPack *pack, DN_Pool *pool, DN_BinPackMode mode, DN_Str8 *string)
{
  DN_BinPackVarInt_(pack, mode, &string->count, sizeof(string->count));
  if (mode == DN_BinPackMode_Serialise) {
    DN_Str8BuilderAppendBytesCopy(&pack->writer, string->data, string->count);
  } else {
    DN_Str8 src       = DN_Str8Subset(pack->read, pack->read_index, string->count);
    *string           = DN_Str8FromStr8Pool(src, pool);
    pack->read_index += src.count;
  }
}

DN_API DN_Str8 DN_BinPackStr8FromBuffer(DN_BinPack *pack, DN_BinPackMode mode, char *ptr, DN_USize *size, DN_USize max)
{
  DN_BinPackCBuffer(pack, mode, ptr, size, max);
  DN_Str8 result = DN_Str8FromPtr(ptr, *size);
  return result;
}

DN_API void DN_BinPackBytesFromArena(DN_BinPack *pack, DN_Arena *arena, DN_BinPackMode mode, void **ptr, DN_USize *size)
{
  DN_Str8 string = DN_Str8FromPtr(*ptr, *size);
  DN_BinPackStr8FromArena(pack, arena, mode, &string);
  *ptr  = string.data;
  *size = string.count;
}

DN_API void DN_BinPackBytesFromPool(DN_BinPack *pack, DN_Pool *pool, DN_BinPackMode mode, void **ptr, DN_USize *size)
{
  DN_Str8 string = DN_Str8FromPtr(*ptr, *size);
  DN_BinPackStr8FromPool(pack, pool, mode, &string);
  *ptr  = string.data;
  *size = string.count;
}

DN_API void DN_BinPackCArray(DN_BinPack *pack, DN_BinPackMode mode, void *ptr, DN_USize size)
{
  DN_BinPackVarInt_(pack, mode, &size, sizeof(size));
  if (mode == DN_BinPackMode_Serialise) {
    DN_Str8BuilderAppendBytesCopy(&pack->writer, ptr, size);
  } else {
    DN_Str8 src = DN_Str8Subset(pack->read, pack->read_index, size);
    DN_Assert(src.count == size);
    DN_Memcpy(ptr, src.data, DN_Min(src.count, size));
    pack->read_index += src.count;
  }
}

DN_API void DN_BinPackCBuffer(DN_BinPack *pack, DN_BinPackMode mode, char *ptr, DN_USize *size, DN_USize max)
{
  if (mode == DN_BinPackMode_Serialise) {
    DN_BinPackUSize(pack, mode, size);
    DN_Str8BuilderAppendBytesCopy(&pack->writer, ptr, *size);
  } else {
    DN_U64 size_u64 = 0;
    DN_BinPackU64(pack, mode, &size_u64);
    DN_Assert(size_u64 < DN_USIZE_MAX);
    DN_Assert(size_u64 <= max);

    *size = DN_Min(size_u64, max);
    DN_Memcpy(ptr, pack->read.data + pack->read_index, *size);
    pack->read_index += size_u64;
  }
}

DN_API DN_Str8 DN_BinPackBuild(DN_BinPack const *pack, DN_Arena *arena)
{
  DN_Str8 result = DN_Str8FromStr8BuilderArena(&pack->writer, arena);
  return result;
}

DN_API DN_CSVTokeniser DN_CSVTokeniserInit(DN_Str8 string, char delimiter)
{
  DN_CSVTokeniser result = {};
  result.string          = string;
  result.delimiter       = delimiter;
  return result;
}

DN_API bool DN_CSVTokeniserValid(DN_CSVTokeniser *tokeniser)
{
  bool result = tokeniser && !tokeniser->bad;
  return result;
}

static void DN_CSVTokeniserEatNewLines_(DN_CSVTokeniser *tokeniser)
{
  char const *end = tokeniser->string.data + tokeniser->string.count;
  while (tokeniser->it[0] == '\n' || tokeniser->it[0] == '\r')
    if (++tokeniser->it == end)
      break;
}

DN_API bool DN_CSVTokeniserNextRow(DN_CSVTokeniser *tokeniser)
{
  bool result = false;
  if (DN_CSVTokeniserValid(tokeniser) && tokeniser->string.count) {
    // NOTE: First time querying row iterator is nil, let tokeniser advance
    if (tokeniser->it) {
      // NOTE: Only advance the tokeniser if we're at the end of the line and
      // there's more to tokenise.
      char const *end = tokeniser->string.data + tokeniser->string.count;
      if (tokeniser->it != end && tokeniser->end_of_line) {
        tokeniser->end_of_line = false;
        result                 = true;
      }
    }
  }

  return result;
}

DN_API DN_Str8 DN_CSVTokeniserNextField(DN_CSVTokeniser *tokeniser)
{
  DN_Str8 result = {};
  if (!DN_CSVTokeniserValid(tokeniser))
    return result;

  if (tokeniser->string.count == 0) {
    tokeniser->bad = true;
    return result;
  }

  // NOTE: First time tokeniser is invoked with a string, set up initial state.
  char const *string_end = tokeniser->string.data + tokeniser->string.count;
  if (!tokeniser->it) {
    tokeniser->it = tokeniser->string.data;
    DN_CSVTokeniserEatNewLines_(tokeniser); // NOTE: Skip any leading new lines
  }

  // NOTE: Tokeniser pointing at end, no more valid data to parse.
  if (tokeniser->it == string_end)
    return result;

  // NOTE: Scan forward until the next control character.
  // 1. '"'                   Double quoted field,  extract everything between the quotes.
  // 2. tokeniser->delimiter  End of the field,     extract everything leading up to the delimiter.
  // 3. '\n'                  Last field in record, extract everything leading up the the new line.
  char const *begin = tokeniser->it;
  while (tokeniser->it != string_end && (tokeniser->it[0] != '"' &&
                                         tokeniser->it[0] != tokeniser->delimiter &&
                                         tokeniser->it[0] != '\n'))
    tokeniser->it++;

  bool quoted_field = (tokeniser->it != string_end) && tokeniser->it[0] == '"';
  if (quoted_field) {
    begin = ++tokeniser->it; // Begin after the quote

  // NOTE: Scan forward until the next '"' which marks the end
  // of the field unless it is escaped by another '"'.
  find_next_quote:
    while (tokeniser->it != string_end && tokeniser->it[0] != '"')
      tokeniser->it++;

    // NOTE: If we encounter a '"' right after, the quotes were escaped
    // and we need to skip to the next instance of a '"'.
    if (tokeniser->it != string_end && tokeniser->it + 1 != string_end && tokeniser->it[1] == '"') {
      tokeniser->it += 2;
      goto find_next_quote;
    }
  }

  // NOTE: Mark the end of the field
  char const *end        = tokeniser->it;
  tokeniser->end_of_line = tokeniser->it == string_end || end[0] == '\n';

  // NOTE: In files with \r\n style new lines ensure that we don't include
  // the \r byte in the CSV field we produce.
  if (end != string_end && end[0] == '\n') {
    DN_Assert((uintptr_t)(end - 1) > (uintptr_t)tokeniser->string.data &&
               "Internal error: The string iterator is pointing behind the start of the string we're reading");
    if (end[-1] == '\r')
      end = end - 1;
  }

  // NOTE: Quoted fields may have whitespace after the closing quote, we skip
  // until we reach the field terminator.
  if (quoted_field)
    while (tokeniser->it != string_end && (tokeniser->it[0] != tokeniser->delimiter && tokeniser->it[0] != '\n'))
      tokeniser->it++;

  // NOTE: Advance the tokeniser past the field terminator.
  if (tokeniser->it != string_end)
    tokeniser->it++;

  // NOTE: Generate the record
  result.data = DN_Cast(char *) begin;
  result.count = DN_Cast(int)(end - begin);
  return result;
}

DN_API DN_Str8 DN_CSVTokeniserNextColumn(DN_CSVTokeniser *tokeniser)
{
  DN_Str8 result = {};
  if (!DN_CSVTokeniserValid(tokeniser))
    return result;

  // NOTE: End of line, the user must explicitly advance to the next row
  if (tokeniser->end_of_line)
    return result;

  // NOTE: Advance tokeniser to the next field in the row
  result = DN_CSVTokeniserNextField(tokeniser);
  return result;
}

DN_API void DN_CSVTokeniserSkipLine(DN_CSVTokeniser *tokeniser)
{
  while (DN_CSVTokeniserValid(tokeniser) && !tokeniser->end_of_line)
    DN_CSVTokeniserNextColumn(tokeniser);
  DN_CSVTokeniserNextRow(tokeniser);
}

DN_API int DN_CSVTokeniserNextN(DN_CSVTokeniser *tokeniser, DN_Str8 *fields, int fields_size, bool column_iterator)
{
  if (!DN_CSVTokeniserValid(tokeniser) || !fields || fields_size <= 0)
    return 0;

  int result = 0;
  for (; result < fields_size; result++) {
    fields[result] = column_iterator ? DN_CSVTokeniserNextColumn(tokeniser) : DN_CSVTokeniserNextField(tokeniser);
    if (!DN_CSVTokeniserValid(tokeniser) || !fields[result].data)
      break;
  }

  return result;
}

DN_API int DN_CSVTokeniserNextColumnN(DN_CSVTokeniser *tokeniser, DN_Str8 *fields, int fields_size)
{
  int result = DN_CSVTokeniserNextN(tokeniser, fields, fields_size, true /*column_iterator*/);
  return result;
}

DN_API int DN_CSVTokeniserNextFieldN(DN_CSVTokeniser *tokeniser, DN_Str8 *fields, int fields_size)
{
  int result = DN_CSVTokeniserNextN(tokeniser, fields, fields_size, false /*column_iterator*/);
  return result;
}

DN_API void DN_CSVTokeniserSkipLineN(DN_CSVTokeniser *tokeniser, int count)
{
  for (int i = 0; i < count && DN_CSVTokeniserValid(tokeniser); i++)
    DN_CSVTokeniserSkipLine(tokeniser);
}

DN_API void DN_CSVPackU64(DN_CSVPack *pack, DN_CSVSerialise serialise, DN_U64 *value)
{
  if (serialise == DN_CSVSerialise_Read) {
    DN_Str8          csv_value = DN_CSVTokeniserNextColumn(&pack->read_tokeniser);
    DN_U64FromResult to_u64    = DN_U64FromStr8(csv_value);
    DN_Assert(to_u64.success);
    *value = to_u64.value;
  } else {
    DN_Str8BuilderAppendF(&pack->write_builder, "%s%I64u", pack->write_column++ ? "," : "", *value);
  }
}

DN_API void DN_CSVPackI64(DN_CSVPack *pack, DN_CSVSerialise serialise, DN_I64 *value)
{
  if (serialise == DN_CSVSerialise_Read) {
    DN_Str8          csv_value = DN_CSVTokeniserNextColumn(&pack->read_tokeniser);
    DN_I64FromResult to_i64    = DN_I64FromStr8(csv_value);
    DN_Assert(to_i64.success);
    *value = to_i64.value;
  } else {
    DN_Str8BuilderAppendF(&pack->write_builder, "%s%I64d", pack->write_column++ ? "," : "", *value);
  }
}

DN_API void DN_CSVPackI32(DN_CSVPack *pack, DN_CSVSerialise serialise, DN_I32 *value)
{
  DN_I64 u64 = *value;
  DN_CSVPackI64(pack, serialise, &u64);
  if (serialise == DN_CSVSerialise_Read)
    *value = DN_SaturateCastI64ToI32(u64);
}

DN_API void DN_CSVPackI16(DN_CSVPack *pack, DN_CSVSerialise serialise, DN_I16 *value)
{
  DN_I64 u64 = *value;
  DN_CSVPackI64(pack, serialise, &u64);
  if (serialise == DN_CSVSerialise_Read)
    *value = DN_SaturateCastI64ToI16(u64);
}

DN_API void DN_CSVPackI8(DN_CSVPack *pack, DN_CSVSerialise serialise, DN_I8 *value)
{
  DN_I64 u64 = *value;
  DN_CSVPackI64(pack, serialise, &u64);
  if (serialise == DN_CSVSerialise_Read)
    *value = DN_SaturateCastI64ToI8(u64);
}

DN_API void DN_CSVPackU32(DN_CSVPack *pack, DN_CSVSerialise serialise, DN_U32 *value)
{
  DN_U64 u64 = *value;
  DN_CSVPackU64(pack, serialise, &u64);
  if (serialise == DN_CSVSerialise_Read)
    *value = DN_SaturateCastU64ToU32(u64);
}

DN_API void DN_CSVPackU16(DN_CSVPack *pack, DN_CSVSerialise serialise, DN_U16 *value)
{
  DN_U64 u64 = *value;
  DN_CSVPackU64(pack, serialise, &u64);
  if (serialise == DN_CSVSerialise_Read)
    *value = DN_SaturateCastU64ToU16(u64);
}

DN_API void DN_CSVPackBoolAsU64(DN_CSVPack *pack, DN_CSVSerialise serialise, bool *value)
{
  DN_U64 u64 = *value;
  DN_CSVPackU64(pack, serialise, &u64);
  if (serialise == DN_CSVSerialise_Read)
    *value = u64 ? 1 : 0;
}

DN_API void DN_CSVPackStr8(DN_CSVPack *pack, DN_CSVSerialise serialise, DN_Str8 *str8, DN_Arena *arena)
{
  if (serialise == DN_CSVSerialise_Read) {
    DN_Str8 csv_value = DN_CSVTokeniserNextColumn(&pack->read_tokeniser);
    *str8             = DN_Str8FromStr8Arena(csv_value, arena);
  } else {
    DN_Str8BuilderAppendF(&pack->write_builder, "%s%.*s", pack->write_column++ ? "," : "", DN_Str8PrintFmt(*str8));
  }
}

DN_API void DN_CSVPackBuffer(DN_CSVPack *pack, DN_CSVSerialise serialise, void *dest, DN_USize *size)
{
  if (serialise == DN_CSVSerialise_Read) {
    DN_Str8 csv_value = DN_CSVTokeniserNextColumn(&pack->read_tokeniser);
    *size             = DN_Min(*size, csv_value.count);
    DN_Memcpy(dest, csv_value.data, *size);
  } else {
    DN_Str8BuilderAppendF(&pack->write_builder, "%s%.*s", pack->write_column++ ? "," : "", DN_Cast(int)(*size), DN_Cast(char *)dest);
  }
}

DN_API void DN_CSVPackBufferWithMax(DN_CSVPack *pack, DN_CSVSerialise serialise, void *dest, DN_USize *size, DN_USize max)
{
  if (serialise == DN_CSVSerialise_Read)
    *size = max;
  DN_CSVPackBuffer(pack, serialise, dest, size);
}

DN_API bool DN_CSVPackNewLine(DN_CSVPack *pack, DN_CSVSerialise serialise)
{
  bool result = true;
  if (serialise == DN_CSVSerialise_Read) {
    result = DN_CSVTokeniserNextRow(&pack->read_tokeniser);
  } else {
    pack->write_column = 0;
    result             = DN_Str8BuilderAppendRef(&pack->write_builder, DN_Str8Lit("\n"));
  }
  return result;
}

DN_API DN_TestCore DN_TestInit(DN_Arena *arena)
{
  DN_TestCore result = {};
  result.arena       = arena;
  result.pool        = DN_PoolFromArena(arena, DN_POOL_DEFAULT_ALIGN);
  DN_PArrayReservePool(result.groups, &result.groups_max, &result.pool, 32);
  return result;
}

DN_API void DN_TestGroupBeginF(DN_TestCore *test, char const *fmt, ...)
{
  DN_AssertF(!test->curr_group, "Previous test group (%.*s) must be ended before starting a new group", DN_Str8PrintFmt(test->curr_group->name));

  // NOTE: Allocate the group
  DN_PArrayPreparePool(test->groups, test->groups_count, &test->groups_max, &test->pool, 1);
  DN_TestGroup *group = DN_PArrayMakeZ(test->groups, &test->groups_count, test->groups_max);

  // NOTE: Initially allocate 32 test entries per group
  DN_PArrayReservePool(group->entries, &group->entries_max, &test->pool, 32);

  // NOTE: Set up the group
  va_list args;
  va_start(args, fmt);
  group->name = DN_Str8FmtVArena(test->arena, fmt, args);
  va_end(args);

  // NOTE: Mark as active
  test->curr_group = group;
  group->ts_begin  = DN_OS_PerfCounterNow();
}

DN_API void DN_TestGroupEnd(DN_TestCore *test)
{
  DN_TestGroup *group = test->curr_group;
  DN_AssertF(group, "Test group must be started before attempting to end it");
  group->ts_end    = DN_OS_PerfCounterNow();
  test->curr_group = nullptr;
}

DN_API void DN_TestBeginF(DN_TestCore *test, char const *fmt, ...)
{
  DN_AssertF(test->curr_group, "Test group must be begun first before creating a test");

  // NOTE:Allocate test entry
  DN_TestGroup *group = test->curr_group;
  DN_PArrayPreparePool(group->entries, group->entries_count, &group->entries_max, &test->pool, 1);
  DN_AssertF(!group->curr_entry, "Test (%.*s) must be ended before starting another test in the group (%.*s)", DN_Str8PrintFmt(group->curr_entry->name), DN_Str8PrintFmt(group->name));
  DN_TestEntry *entry = DN_PArrayMakeZ(group->entries, &group->entries_count, group->entries_max);

  // NOTE: Fill in test entry
  va_list args;
  va_start(args, fmt);
  entry->name = DN_Str8FmtVArena(test->arena, fmt, args);
  va_end(args);

  // NOTE: Mark as active
  group->curr_entry = entry;
  entry->ts_begin   = DN_OS_PerfCounterNow();
  test->total_count++;
}

DN_API void DN_TestEnd(DN_TestCore *test)
{
  DN_TestGroup *group = test->curr_group;
  DN_AssertF(group, "Test group must be started before attempting to end a test in it");
  DN_TestEntry *entry = group->curr_entry;
  DN_AssertF(entry, "Test must be started before attempting to end a test in group (%.*s)", DN_Str8PrintFmt(group->name));

  entry->ts_end     = DN_OS_PerfCounterNow();
  if (entry->failed)
    test->total_failed++;
  else
    test->total_passed++;
  group->curr_entry = nullptr;
}

DN_API DN_Str8 DN_Str8FromTestCore(DN_TestCore const *test, DN_Arena *arena, DN_Str8FromTestCoreFlags flags)
{
  DN_TcScratch scratch       = DN_TcScratchBeginArena(&arena, 1);
  DN_USize     padding_count = 100;
  DN_Str8      padding_line  = DN_Str8FillF(&scratch.arena, padding_count, ".");
  if (flags & DN_Str8FromTestCoreFlags_Colour)
    padding_line = DN_Str8FmtAnsiColourU8RgbArena(DN_AnsiColourMode_Fg, 64, 64, 64, &scratch.arena, "%.*s", DN_Str8PrintFmt(padding_line));

  DN_V3F32 const good_colour = DN_V3F32From3N(0, 255, 0);
  DN_V3F32 const bad_colour  = DN_V3F32From3N(255, 0, 0);
  DN_Str8Builder builder     = DN_Str8BuilderFromArena(arena);
  for (DN_ForItSize(group_it, DN_TestGroup const, test->groups, test->groups_count)) {
    DN_TestGroup const* group           = group_it.data;
    DN_USize            group_successes = 0;
    DN_USize            group_failures  = 0;
    DN_MSVC_WARNING_PUSH
    DN_MSVC_WARNING_DISABLE(6067) // _Param_(5) in call to 'DN_Str8BuilderAppendF' must be the address of a string. Actual type: 'const unsigned __int64'.
    DN_MSVC_WARNING_DISABLE(6271) // Extra argument passed to 'DN_Str8BuilderAppendF'.
    DN_Str8BuilderAppendF(&builder, "[%.*s] (%'zu test%s)\n", DN_Str8PrintFmt(group->name), group->entries_count, (group->entries_count > 1) ? "s" : "");
    DN_MSVC_WARNING_POP
    for (DN_ForItSize(entry_it, DN_TestEntry const, group->entries, group->entries_count)) {
      DN_TestEntry const* entry = entry_it.data;
      if (entry->failed)
        group_failures++;
      else
        group_successes++;

      // NOTE: Extract the test name and pad the line with dots (.)
      DN_Str8  test_prefix = DN_Str8FmtArena(&scratch.arena, "  [%zu/%zu] %.*s ", entry_it.index + 1, group->entries_count, DN_Str8PrintFmt(entry->name));
      if (test_prefix.count < padding_line.count) {
        DN_USize remaining = padding_line.count - test_prefix.count;
        DN_Str8  padder    = DN_Str8Subset(padding_line, 0, remaining);
        test_prefix        = DN_Str8AppendF(&scratch.arena, test_prefix, "%.*s", DN_Str8PrintFmt(padder));
      }

      // NOTE: Construct the test line
      DN_Str8 outcome_str8 = entry->failed ? DN_Str8Lit("FAILED") : DN_Str8Lit("OK");
      if (flags & DN_Str8FromTestCoreFlags_Colour) {
        if (entry->failed)
          outcome_str8 = DN_Str8FmtAnsiColourV3F32Rgb255Arena(DN_AnsiColourMode_Fg, bad_colour, &scratch.arena, "%.*s", DN_Str8PrintFmt(outcome_str8));
        else
          outcome_str8 = DN_Str8FmtAnsiColourV3F32Rgb255Arena(DN_AnsiColourMode_Fg, good_colour, &scratch.arena, "%.*s", DN_Str8PrintFmt(outcome_str8));
      }

      DN_F64 elapsed_ms = DN_OS_PerfCounterMs(entry->ts_begin, entry->ts_end);
      DN_Str8BuilderAppendF(&builder, "%.*s %.*s (%.3fms)\n", DN_Str8PrintFmt(test_prefix), DN_Str8PrintFmt(outcome_str8), elapsed_ms);

      // NOTE: Output the test diagnostics, populated on failure
      if (entry->failed) {
        for (DN_ForItSize(row_it, DN_TestDiagnosticRow, entry->diagnostics, entry->diagnostics_count)) {
          DN_USize          row_count   = 1;
          DN_Str8TableFlags table_flags = DN_Str8TableFlags_None;
          if (row_it.index == 0) {
            row_count++; // Header
            table_flags |= DN_Str8TableFlags_HasHeader;
          }

          DN_USize col_count = 3; // Expression, Eval, Call site
          DN_USize row_index = 0;
          DN_Str8 *rows_str8 = DN_ArenaNewArrayZ(&scratch.arena, DN_Str8, row_count * col_count);
          if (row_it.index == 0) {
            rows_str8[row_index++] = DN_Str8Lit("Expr.");
            rows_str8[row_index++] = DN_Str8Lit("Eval.");
            rows_str8[row_index++] = DN_Str8Lit("Location");
          }

          DN_TestDiagnosticRow *row       = row_it.data;
          DN_Str8               file_name = DN_Str8FileNameFromPath(row->call_site.file);
          rows_str8[row_index++]          = row->expr;
          rows_str8[row_index++]          = row->invariant;
          rows_str8[row_index++]          = DN_Str8FmtArena(&scratch.arena, "%.*s:%u", DN_Str8PrintFmt(file_name), row->call_site.line);

          DN_Str8 table = DN_Str8Table(rows_str8, row_count, col_count, table_flags, &scratch.arena);
          table         = DN_Str8PadNewLinesArena(table, DN_Str8Lit("  "), &scratch.arena);
          DN_Str8BuilderAppendF(&builder, "  %.*s\n", DN_Str8PrintFmt(table));
          if (row->message.count) {
            DN_Str8 line_break = DN_Str8LineBreakArena(row->message, 100, DN_Str8Lit("\n  "), DN_Str8LineBreakMode_AtWord, &scratch.arena);
            DN_Str8BuilderAppendF(&builder, "  User Message: %.*s\n", DN_Str8PrintFmt(line_break));
          }
        }
        DN_Str8BuilderAppendF(&builder, "\n");
      }
    }

    DN_F64  elapsed_ms   = DN_OS_PerfCounterMs(group->ts_begin, group->ts_end);
    DN_MSVC_WARNING_PUSH
    DN_MSVC_WARNING_DISABLE(6067) // _Param_(3) in call to 'DN_Str8BuilderAppendF' must be the address of a string. Actual type: 'const unsigned __int64'.
    DN_MSVC_WARNING_DISABLE(6271) // Extra argument passed to 'DN_Str8BuilderAppendF'.
    DN_Str8 group_prefix = DN_Str8FmtArena(&scratch.arena, "%'zu/%'zu test%s", group_successes, group->entries_count, group_successes > 1 ? "s" :"");
    DN_MSVC_WARNING_POP
    if (flags & DN_Str8FromTestCoreFlags_Colour) {
      if (group_failures == 0)
        group_prefix = DN_Str8FmtAnsiColourV3F32Rgb255Arena(DN_AnsiColourMode_Fg, good_colour, &scratch.arena, "%.*s", DN_Str8PrintFmt(group_prefix));
      else
        group_prefix = DN_Str8FmtAnsiColourV3F32Rgb255Arena(DN_AnsiColourMode_Fg, bad_colour, &scratch.arena, "%.*s", DN_Str8PrintFmt(group_prefix));
    }
    DN_Str8BuilderAppendF(&builder, "\n  %.*s passed in [%.*s] %.3fms (%zu failed)\n", DN_Str8PrintFmt(group_prefix), DN_Str8PrintFmt(group->name), elapsed_ms, group_failures);
  }

  DN_Str8 result = DN_Str8FromStr8BuilderArena(&builder, arena);
  return result;
}

DN_API DN_TestDiagnosticRow *DN_TestVerifySetupFmtV(DN_TestCore *test, DN_CallSite call_site, DN_Str8 expr, bool verify_failed, char const *fmt, va_list args)
{
  DN_AssertF(test->curr_group,             "Test group must be started before attempting to verify a test invariant");
  DN_AssertF(test->curr_group->curr_entry, "Test must be started before attempting to verify a test invariant in a group (%.*s)", DN_Str8PrintFmt(test->curr_group->name));
  DN_TestDiagnosticRow *result = nullptr;
  if (verify_failed) {
    DN_TestGroup *group = test->curr_group;
    DN_TestEntry *entry = group->curr_entry;
    entry->failed = true;

    DN_PArrayPreparePool(entry->diagnostics, entry->diagnostics_count, &entry->diagnostics_max, &test->pool, 1);
    result            = DN_PArrayMakeZ(entry->diagnostics, &entry->diagnostics_count, entry->diagnostics_max);
    result->expr      = expr;
    result->call_site = call_site;
    result->message   = DN_Str8FmtVArena(test->arena, fmt, args);
  }
  return result;
}

DN_API void DN_TestVerifyExprF_(DN_TestCore *test, DN_CallSite call_site, DN_Str8 expr, bool expr_result, char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_TestDiagnosticRow *row = DN_TestVerifySetupFmtV(test, call_site, expr, /*verify_failed=*/ !expr_result, fmt, args);
  va_end(args);
  if (row)
    row->invariant = DN_Str8FmtArena(test->arena, "%s (expected %s)", expr_result ? "true" : "false", expr_result ? "false" : "true");
}

DN_API void DN_TestVerifyF64Fmt(DN_TestCore *test, DN_CallSite call_site, DN_Str8 val_str8, DN_Str8 expect_str8, DN_F64 val, DN_F64 expect, DN_TestLogic logic, char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  bool verify_failed     = false;
  DN_Str8 logic_str8     = {};
  DN_Str8 logic_inv_str8 = {};
  switch (logic) {
    case DN_TestLogic_GreaterThan:   logic_str8 = DN_Str8Lit(">");  logic_inv_str8 = DN_Str8Lit("<="); verify_failed = !(val >  expect); break;
    case DN_TestLogic_GreaterThanEq: logic_str8 = DN_Str8Lit(">="); logic_inv_str8 = DN_Str8Lit("<");  verify_failed = !(val >= expect); break;
    case DN_TestLogic_LessThan:      logic_str8 = DN_Str8Lit("<");  logic_inv_str8 = DN_Str8Lit(">="); verify_failed = !(val <  expect); break;
    case DN_TestLogic_LessThanEq:    logic_str8 = DN_Str8Lit("<="); logic_inv_str8 = DN_Str8Lit(">");  verify_failed = !(val <= expect); break;
    case DN_TestLogic_Eq:            logic_str8 = DN_Str8Lit("=="); logic_inv_str8 = DN_Str8Lit("!="); verify_failed = !(val == expect); break;
    case DN_TestLogic_NotEq:         logic_str8 = DN_Str8Lit("!="); logic_inv_str8 = DN_Str8Lit("=="); verify_failed = !(val != expect); break;
  }
  DN_TestDiagnosticRow *row = DN_TestVerifySetupFmtV(test, call_site, /*expr=*/ DN_Str8Lit(""), verify_failed, fmt, args);
  va_end(args);
  if (row) {
    row->expr      = DN_Str8FmtArena(test->arena, "%.*s %.*s %.*s",        DN_Str8PrintFmt(val_str8), DN_Str8PrintFmt(logic_str8),     DN_Str8PrintFmt(expect_str8));
    row->invariant = DN_Str8FmtArena(test->arena, "%f %.*s %f (expected)", val,                       DN_Str8PrintFmt(logic_inv_str8), expect);
  }
}

DN_API void DN_TestVerifyISizeF(DN_TestCore *test, DN_CallSite call_site, DN_Str8 val_str8, DN_Str8 expect_str8, DN_ISize val, DN_ISize expect, DN_TestLogic logic, char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  bool verify_failed     = false;
  DN_Str8 logic_str8     = {};
  DN_Str8 logic_inv_str8 = {};
  switch (logic) {
    case DN_TestLogic_GreaterThan:   logic_str8 = DN_Str8Lit(">");  logic_inv_str8 = DN_Str8Lit("<="); verify_failed = !(val >  expect); break;
    case DN_TestLogic_GreaterThanEq: logic_str8 = DN_Str8Lit(">="); logic_inv_str8 = DN_Str8Lit("<"); verify_failed = !(val >= expect); break;
    case DN_TestLogic_LessThan:      logic_str8 = DN_Str8Lit("<");  logic_inv_str8 = DN_Str8Lit(">="); verify_failed = !(val <  expect); break;
    case DN_TestLogic_LessThanEq:    logic_str8 = DN_Str8Lit("<="); logic_inv_str8 = DN_Str8Lit(">"); verify_failed = !(val <= expect); break;
    case DN_TestLogic_Eq:            logic_str8 = DN_Str8Lit("=="); logic_inv_str8 = DN_Str8Lit("!="); verify_failed = !(val == expect); break;
    case DN_TestLogic_NotEq:         logic_str8 = DN_Str8Lit("!="); logic_inv_str8 = DN_Str8Lit("=="); verify_failed = !(val != expect); break;
  }
  DN_TestDiagnosticRow *row = DN_TestVerifySetupFmtV(test, call_site, /*expr=*/ DN_Str8Lit(""), verify_failed, fmt, args);
  va_end(args);
  if (row) {
    row->expr      = DN_Str8FmtArena(test->arena, "%.*s %.*s %.*s",            DN_Str8PrintFmt(val_str8), DN_Str8PrintFmt(logic_str8),     DN_Str8PrintFmt(expect_str8));
    DN_MSVC_WARNING_PUSH
    DN_MSVC_WARNING_DISABLE(6067) // _Param_(4) in call to 'DN_Str8FmtArena' must be the address of a string. Actual type: 'int'.
    DN_MSVC_WARNING_DISABLE(6328) // Size mismatch: '__int64' passed as _Param_(3) when 'unsigned int' is required in call to 'DN_Str8FmtArena'
    DN_MSVC_WARNING_DISABLE(6271) // Extra argument passed to 'DN_Str8FmtArena'.
    row->invariant = DN_Str8FmtArena(test->arena, "%'zd %.*s %'zd (expected)", val,                       DN_Str8PrintFmt(logic_inv_str8), expect);
    DN_MSVC_WARNING_POP
  }
}

DN_API void DN_TestVerifyUSizeF(DN_TestCore *test, DN_CallSite call_site, DN_Str8 val_str8, DN_Str8 expect_str8, DN_USize val, DN_USize expect, DN_TestLogic logic, char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  bool verify_failed     = false;
  DN_Str8 logic_str8     = {};
  DN_Str8 logic_inv_str8 = {};
  switch (logic) {
    case DN_TestLogic_GreaterThan:   logic_str8 = DN_Str8Lit(">");  logic_inv_str8 = DN_Str8Lit("<="); verify_failed = !(val >  expect); break;
    case DN_TestLogic_GreaterThanEq: logic_str8 = DN_Str8Lit(">="); logic_inv_str8 = DN_Str8Lit("<"); verify_failed = !(val >= expect); break;
    case DN_TestLogic_LessThan:      logic_str8 = DN_Str8Lit("<");  logic_inv_str8 = DN_Str8Lit(">="); verify_failed = !(val <  expect); break;
    case DN_TestLogic_LessThanEq:    logic_str8 = DN_Str8Lit("<="); logic_inv_str8 = DN_Str8Lit(">"); verify_failed = !(val <= expect); break;
    case DN_TestLogic_Eq:            logic_str8 = DN_Str8Lit("=="); logic_inv_str8 = DN_Str8Lit("!="); verify_failed = !(val == expect); break;
    case DN_TestLogic_NotEq:         logic_str8 = DN_Str8Lit("!="); logic_inv_str8 = DN_Str8Lit("=="); verify_failed = !(val != expect); break;
  }
  DN_TestDiagnosticRow *row = DN_TestVerifySetupFmtV(test, call_site, /*expr=*/ DN_Str8Lit(""), verify_failed, fmt, args);
  va_end(args);
  if (row) {
    row->expr      = DN_Str8FmtArena(test->arena, "%.*s %.*s %.*s",            DN_Str8PrintFmt(val_str8), DN_Str8PrintFmt(logic_str8),     DN_Str8PrintFmt(expect_str8));
    DN_MSVC_WARNING_PUSH
    DN_MSVC_WARNING_DISABLE(6067) // _Param_(4) in call to 'DN_Str8FmtArena' must be the address of a string. Actual type: 'int'.
    DN_MSVC_WARNING_DISABLE(6328) // Size mismatch: '__int64' passed as _Param_(3) when 'unsigned int' is required in call to 'DN_Str8FmtArena'
    DN_MSVC_WARNING_DISABLE(6271) // Extra argument passed to 'DN_Str8FmtArena'.
    row->invariant = DN_Str8FmtArena(test->arena, "%'zu %.*s %'zu (expected)", val,                       DN_Str8PrintFmt(logic_inv_str8), expect);
    DN_MSVC_WARNING_POP
  }
}

DN_API void DN_TestVerifyStr8F(DN_TestCore *test, DN_CallSite call_site, DN_Str8 expr, DN_Str8 str8, DN_Str8 expect, bool expect_eq, char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  bool verify_failed = false;
  if (expect_eq)
    verify_failed = !DN_Str8EqSensitive(str8, expect);
  else
    verify_failed = DN_Str8EqSensitive(str8, expect);

  DN_TestDiagnosticRow *row = DN_TestVerifySetupFmtV(test, call_site, expr, verify_failed, fmt, args);
  va_end(args);
  if (row) {
    if (expect_eq)
      row->invariant = DN_Str8FmtArena(test->arena, "\"%.*s\" != \"%.*s\"", DN_Str8PrintFmt(str8), DN_Str8PrintFmt(expect));
    else
      row->invariant = DN_Str8FmtArena(test->arena, "\"%.*s\" == \"%.*s\"", DN_Str8PrintFmt(str8), DN_Str8PrintFmt(expect));
  }
}

DN_API void DN_TestVerifyBytesF(DN_TestCore *test, DN_CallSite call_site, DN_Str8 expr, DN_Str8 bytes, DN_Str8 expect, bool expect_eq, char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  bool verify_failed = false;
  if (expect_eq)
    verify_failed = !DN_Str8EqSensitive(bytes, expect);
  else
    verify_failed = DN_Str8EqSensitive(bytes, expect);

  DN_TestDiagnosticRow *row = DN_TestVerifySetupFmtV(test, call_site, expr, verify_failed, fmt, args);
  va_end(args);
  if (row) {
    DN_Str8 bytes_hex  = DN_Str8HexFromStr8BytesArena(bytes, test->arena, DN_TrimLeadingZero_No);
    DN_Str8 expect_hex = DN_Str8HexFromStr8BytesArena(expect, test->arena, DN_TrimLeadingZero_No);
    if (expect_eq)
      row->invariant = DN_Str8FmtArena(test->arena, "\"%.*s\" != \"%.*s\"", DN_Str8PrintFmt(bytes_hex), DN_Str8PrintFmt(expect_hex));
    else
      row->invariant = DN_Str8FmtArena(test->arena, "\"%.*s\" == \"%.*s\"", DN_Str8PrintFmt(bytes_hex), DN_Str8PrintFmt(expect_hex));
  }
}

#if DN_WITH_TESTS
#if defined(DN_PLATFORM_WIN32) && defined(DN_COMPILER_MSVC)
// NOTE: Taken from MSDN __cpuid example implementation
// https://learn.microsoft.com/en-us/cpp/intrinsics/cpuid-cpuidex?view=msvc-170
typedef struct DN_RefImplCPUReport DN_RefImplCPUReport;
struct DN_RefImplCPUReport {
  unsigned int    nIds_            = 0;
  unsigned int    nExIds_          = 0;
  char            vendor_[0x20]    = {};
  int             vendorSize_      = 0;
  char            brand_[0x40]     = {};
  int             brandSize_       = 0;
  bool            isIntel_         = false;
  bool            isAMD_           = false;
  DN_U32          f_1_ECX_         = 0;
  DN_U32          f_1_EDX_         = 0;
  DN_U32          f_7_EBX_         = 0;
  DN_U32          f_7_ECX_         = 0;
  DN_U32          f_81_ECX_        = 0;
  DN_U32          f_81_EDX_        = 0;
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
  bool RDTscP(void)      const { return f_81_EDX_ &              (1 << 27); }
  bool _3DNOWEXT(void)   const { return isAMD_    && f_81_EDX_ & (1 << 30); }
  bool _3DNOW(void)      const { return isAMD_    && f_81_EDX_ & (1 << 31); }
};

static DN_RefImplCPUReport DN_RefImplCPUReport_Init()
{
  DN_RefImplCPUReport result = {};

  int cpui[4];

  __cpuid(cpui, 0);
  result.nIds_ = cpui[0];

  for (unsigned int i = 0; i <= result.nIds_; ++i) {
    __cpuidex(cpui, i, 0);
    memcpy(result.data_[result.dataSize_++], cpui, sizeof(cpui));
  }

  *reinterpret_cast<int *>(result.vendor_)     = result.data_[0][1];
  *reinterpret_cast<int *>(result.vendor_ + 4) = result.data_[0][3];
  *reinterpret_cast<int *>(result.vendor_ + 8) = result.data_[0][2];
  result.vendorSize_                           = (int)strlen(result.vendor_);

  if (strcmp(result.vendor_, "GenuineIntel") == 0)
    result.isIntel_ = true;
  else if (strcmp(result.vendor_, "AuthenticAMD") == 0)
    result.isAMD_ = true;

  if (result.nIds_ >= 1) {
    result.f_1_ECX_ = result.data_[1][2];
    result.f_1_EDX_ = result.data_[1][3];
  }

  if (result.nIds_ >= 7) {
    result.f_7_EBX_ = result.data_[7][1];
    result.f_7_ECX_ = result.data_[7][2];
  }

  __cpuid(cpui, 0x80000000);
  result.nExIds_ = cpui[0];

  for (unsigned int i = 0x80000000; i <= result.nExIds_; ++i) {
    __cpuidex(cpui, i, 0);
    memcpy(result.extdata_[result.extdataSize_++], cpui, sizeof(cpui));
  }

  if (result.nExIds_ >= 0x80000001) {
    result.f_81_ECX_ = result.extdata_[1][2];
    result.f_81_EDX_ = result.extdata_[1][3];
  }

  if (result.nExIds_ >= 0x80000004) {
    memcpy(result.brand_, result.extdata_[2], sizeof(cpui));
    memcpy(result.brand_ + 16, result.extdata_[3], sizeof(cpui));
    memcpy(result.brand_ + 32, result.extdata_[4], sizeof(cpui));
    result.brandSize_ = (int)strlen(result.brand_);
  }

  return result;
}
#endif // defined(DN_PLATFORM_WIN32) && defined(DN_COMPILER_MSVC)

DN_MSVC_WARNING_PUSH
DN_MSVC_WARNING_DISABLE(6262) // Function uses '23524' bytes of stack.  Consider moving some data to heap.
DN_API DN_TestCore DN_TestSuite(DN_Arena *arena_)
{
  DN_TestCore result = DN_TestInit(arena_);
  for (DN_TestGroupScopeF(&result, "Base")) {
    // NOTE: AgeStr8From
    {
      for (DN_TestScopeF(&result, "[AgeStr8FromMsU64] '1001' in seconds converts to '1s 1ms'")) {
        DN_Str8x128 str8   = DN_AgeStr8FromMsU64(1001, DN_AgeUnit_Sec | DN_AgeUnit_Ms);
        DN_Str8     expect = DN_Str8Lit("1s 1ms");
        DN_TestVerifyStr8Eq(&result, DN_Str8FromStruct(&str8), expect);
      }

      for (DN_TestScopeF(&result, "[AgeStr8FromMsU64] '1001' in seconds converts to '1.001s' (fractional)")) {
        DN_Str8x128 str8   = DN_AgeStr8FromMsU64(1001, DN_AgeUnit_FractionalSec);
        DN_Str8     expect = DN_Str8Lit("1.001s");
        DN_TestVerifyStr8Eq(&result, DN_Str8FromStruct(&str8), expect);
      }
    }

    // NOTE: TicketMutex
    {
      for (DN_TestScopeF(&result, "[TicketMutex] Start and stop")) {
        DN_TicketMutex mutex = {};
        DN_TicketMutexBegin(&mutex);
        DN_TicketMutexEnd(&mutex);
        DN_TestVerifyUSizeEq(&result, mutex.ticket, mutex.serving);
      }

      for (DN_TestScopeF(&result, "[TicketMutex] Start and stop w/ advanced API")) {
        DN_TicketMutex mutex = {};
        DN_UInt ticket_a     = DN_TicketMutexMakeTicket(&mutex);
        DN_UInt ticket_b     = DN_TicketMutexMakeTicket(&mutex);
        DN_TestVerifyExpr(&result, DN_Cast(bool) DN_TicketMutexCanLock(&mutex, ticket_b) == false);
        DN_TestVerifyExpr(&result, DN_Cast(bool) DN_TicketMutexCanLock(&mutex, ticket_a) == true);

        DN_TicketMutexBeginTicket(&mutex, ticket_a);
        DN_TicketMutexEnd(&mutex);
        DN_TicketMutexBeginTicket(&mutex, ticket_b);
        DN_TicketMutexEnd(&mutex);

        DN_TestVerifyUSizeEq(&result, mutex.ticket, mutex.serving);
        DN_TestVerifyUSizeEq(&result, mutex.ticket, ticket_b + 1);
      }
    }

    // NOTE: QSort
    {
      for (DN_TestScopeF(&result, "[QSort] Str8 Lexicographic Ascending")) {
        DN_Str8 list[] = {
            DN_Str8Lit("z_last"),
            DN_Str8Lit("m_middle"),
            DN_Str8Lit("a_first"),
            DN_Str8Lit("version-1.2.10"),
            DN_Str8Lit("version-1.2.2"),
            DN_Str8Lit("version-1.10.0"),
        };

        DN_QSortStr8LexicographicAsc(list, DN_ArrayCountU(list), DN_Str8EqCase_Insensitive);

        DN_USize list_index = 0;
        DN_TestVerifyStr8Eq(&result, list[list_index++], DN_Str8Lit("a_first"));
        DN_TestVerifyStr8Eq(&result, list[list_index++], DN_Str8Lit("m_middle"));
        DN_TestVerifyStr8Eq(&result, list[list_index++], DN_Str8Lit("version-1.10.0"));
        DN_TestVerifyStr8Eq(&result, list[list_index++], DN_Str8Lit("version-1.2.10"));
        DN_TestVerifyStr8Eq(&result, list[list_index++], DN_Str8Lit("version-1.2.2"));
        DN_TestVerifyStr8Eq(&result, list[list_index++], DN_Str8Lit("z_last"));
      }

      for (DN_TestScopeF(&result, "[QSort] Str8 Natural")) {
        DN_Str8 list[] = {
            DN_Str8Lit("item10"),
            DN_Str8Lit("item2"),
            DN_Str8Lit("item1"),
            DN_Str8Lit("item20"),
            DN_Str8Lit("item12"),
            DN_Str8Lit("Afile"),
            DN_Str8Lit("file2"),
            DN_Str8Lit("file10"),
            DN_Str8Lit("file1"),
            DN_Str8Lit("z_last"),
            DN_Str8Lit("m_middle"),
            DN_Str8Lit("a_first"),
            DN_Str8Lit("version-1.2.10"),
            DN_Str8Lit("version-1.2.2"),
            DN_Str8Lit("version-1.10.0"),
        };

        DN_QSortStr8NaturalAsc(list, DN_ArrayCountU(list), DN_Str8EqCase_Sensitive);

        DN_USize list_index = 0;
        DN_TestVerifyStr8Eq(&result, list[list_index++], DN_Str8Lit("Afile"));
        DN_TestVerifyStr8Eq(&result, list[list_index++], DN_Str8Lit("a_first"));
        DN_TestVerifyStr8Eq(&result, list[list_index++], DN_Str8Lit("file1"));
        DN_TestVerifyStr8Eq(&result, list[list_index++], DN_Str8Lit("file2"));
        DN_TestVerifyStr8Eq(&result, list[list_index++], DN_Str8Lit("file10"));
        DN_TestVerifyStr8Eq(&result, list[list_index++], DN_Str8Lit("item1"));
        DN_TestVerifyStr8Eq(&result, list[list_index++], DN_Str8Lit("item2"));
        DN_TestVerifyStr8Eq(&result, list[list_index++], DN_Str8Lit("item10"));
        DN_TestVerifyStr8Eq(&result, list[list_index++], DN_Str8Lit("item12"));
        DN_TestVerifyStr8Eq(&result, list[list_index++], DN_Str8Lit("item20"));
        DN_TestVerifyStr8Eq(&result, list[list_index++], DN_Str8Lit("m_middle"));
        DN_TestVerifyStr8Eq(&result, list[list_index++], DN_Str8Lit("version-1.2.2"));
        DN_TestVerifyStr8Eq(&result, list[list_index++], DN_Str8Lit("version-1.2.10"));
        DN_TestVerifyStr8Eq(&result, list[list_index++], DN_Str8Lit("version-1.10.0"));
        DN_TestVerifyStr8Eq(&result, list[list_index++], DN_Str8Lit("z_last"));
      }
    }

    // NOTE: CPUID
    #if defined(DN_PLATFORM_WIN32) && defined(DN_COMPILER_MSVC)
    {
      for (DN_TestScopeF(&result, "[CPU] Query CPUID")) {
        DN_RefImplCPUReport ref_cpu_report = DN_RefImplCPUReport_Init();
        DN_CPUReport        cpu_report     = DN_CPUGetReport();

        DN_TestVerifyExpr(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_3DNow) == ref_cpu_report._3DNOW());
        DN_TestVerifyExpr(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_3DNowExt) == ref_cpu_report._3DNOWEXT());
        DN_TestVerifyExpr(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_ABM) == ref_cpu_report.ABM());
        DN_TestVerifyExpr(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_AES) == ref_cpu_report.AES());
        DN_TestVerifyExpr(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_AVX) == ref_cpu_report.AVX());
        DN_TestVerifyExpr(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_AVX2) == ref_cpu_report.AVX2());
        DN_TestVerifyExpr(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_AVX512CD) == ref_cpu_report.AVX512CD());
        DN_TestVerifyExpr(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_AVX512ER) == ref_cpu_report.AVX512ER());
        DN_TestVerifyExpr(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_AVX512F) == ref_cpu_report.AVX512F());
        DN_TestVerifyExpr(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_AVX512PF) == ref_cpu_report.AVX512PF());
        DN_TestVerifyExpr(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_CMPXCHG16B) == ref_cpu_report.CMPXCHG16B());
        DN_TestVerifyExpr(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_F16C) == ref_cpu_report.F16C());
        DN_TestVerifyExpr(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_FMA) == ref_cpu_report.FMA());
        DN_TestVerifyExpr(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_MMX) == ref_cpu_report.MMX());
        DN_TestVerifyExpr(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_MmxExt) == ref_cpu_report.MMXEXT());
        DN_TestVerifyExpr(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_MONITOR) == ref_cpu_report.MONITOR());
        DN_TestVerifyExpr(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_MOVBE) == ref_cpu_report.MOVBE());
        DN_TestVerifyExpr(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_PCLMULQDQ) == ref_cpu_report.PCLMULQDQ());
        DN_TestVerifyExpr(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_POPCNT) == ref_cpu_report.POPCNT());
        DN_TestVerifyExpr(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_RDRAND) == ref_cpu_report.RDRAND());
        DN_TestVerifyExpr(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_RDSEED) == ref_cpu_report.RDSEED());
        DN_TestVerifyExpr(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_RDTscP) == ref_cpu_report.RDTscP());
        DN_TestVerifyExpr(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_SHA) == ref_cpu_report.SHA());
        DN_TestVerifyExpr(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_SSE) == ref_cpu_report.SSE());
        DN_TestVerifyExpr(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_SSE2) == ref_cpu_report.SSE2());
        DN_TestVerifyExpr(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_SSE3) == ref_cpu_report.SSE3());
        DN_TestVerifyExpr(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_SSE41) == ref_cpu_report.SSE41());
        DN_TestVerifyExpr(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_SSE42) == ref_cpu_report.SSE42());
        DN_TestVerifyExpr(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_SSE4A) == ref_cpu_report.SSE4a());
        DN_TestVerifyExpr(&result, DN_CPUHasFeature(&cpu_report, DN_CPUFeature_SSSE3) == ref_cpu_report.SSSE3());
      }
    }
    #endif // defined(DN_PLATFORM_WIN32) && defined(DN_COMPILER_MSVC)

    // NOTE: FmtAppendTruncate
    {
      for (DN_TestScopeF(&result, "[Fmt] FmtAppendTruncate truncates with 3 dots")) {
        char               buf[8]   = {};
        DN_USize           buf_size = 0;
        DN_FmtAppendResult buf_str8 = DN_FmtAppendTruncate(buf, &buf_size, sizeof(buf), DN_Str8Lit("..."), "This string is longer than %d characters", DN_Cast(int)(sizeof(buf) - 1));
        DN_Str8            expect   = DN_Str8Lit("This...");
        DN_TestVerifyExpr(&result, buf_str8.truncated);
        DN_TestVerifyStr8EqF(&result, buf_str8.str8, expect, "buf_str8=%.*s, expect=%.*s", DN_Str8PrintFmt(buf_str8.str8), DN_Str8PrintFmt(expect));
      }
    }

    // NOTE: Atomics
    {
      DN_MSVC_WARNING_PUSH
      DN_MSVC_WARNING_DISABLE(28113)
      DN_MSVC_WARNING_DISABLE(28112)
      {
        for (DN_TestScopeF(&result, "[Atomics] AtomicAddU32")) {
          DN_U32 val = 0;
          DN_AtomicAddU32(&val, 1);
          DN_TestVerifyUSizeEq(&result, val, 1);
        }

        for (DN_TestScopeF(&result, "[Atomics] AtomicAddU64")) {
          uint64_t val = 0;
          DN_AtomicAddU64(&val, 1);
          DN_TestVerifyExprF(&result, val == 1, "val: %" PRIu64, val);
        }

        for (DN_TestScopeF(&result, "[Atomics] AtomicSubU32")) {
          DN_U32 val = 1;
          DN_AtomicSubU32(&val, 1);
          DN_TestVerifyUSizeEq(&result, val, 0);
        }

        for (DN_TestScopeF(&result, "[Atomics] AtomicSubU64")) {
          uint64_t val = 1;
          DN_AtomicSubU64(&val, 1);
          DN_TestVerifyExprF(&result, val == 0, "val: %" PRIu64, val);
        }

        for (DN_TestScopeF(&result, "[Atomics] AtomicSetValue32")) {
          DN_U32 a = 0;
          DN_U32 b = 111;
          DN_AtomicSetValue32(&a, b);
          DN_TestVerifyUSizeEq(&result, a, b);
        }

        for (DN_TestScopeF(&result, "[Atomics] AtomicSetValue64")) {
          int64_t a = 0;
          int64_t b = 111;
          DN_AtomicSetValue64(DN_Cast(uint64_t *) & a, b);
          DN_TestVerifyExprF(&result, a == b, "a: %" PRId64 ", b: %" PRId64, a, b);
        }

        for (DN_TestScopeF(&result, "[Intrinsics] CPUGetTsc compile check")) {
          DN_CPUGetTsc();
        }

        for (DN_TestScopeF(&result, "[Intrinsics] CompilerReadBarrierAndCPUReadFence compile check")) {
          DN_CompilerReadBarrierAndCPUReadFence;
        }

        for (DN_TestScopeF(&result, "[Intrinsics] CompilerWriteBarrierAndCPUWriteFence compile check")) {
          DN_CompilerWriteBarrierAndCPUWriteFence;
        }
      }
      DN_MSVC_WARNING_POP
    }

    // NOTE: Arena
    {
      for (DN_TestScopeF(&result, "[Arena] Reused memory is zeroed out")) {
        uint8_t    alignment  = 1;
        DN_USize   alloc_size = DN_Kilobytes(128);
        DN_MemList mem        = DN_MemListFromHeap(0, 0, DN_MemFlags_Nil, DN_OS_HeapInitVirtual());
        DN_Arena   arena      = DN_ArenaFromMemList(&mem);
        DN_DEFER { DN_MemListDeinit(&mem); };

        uintptr_t first_ptr_address = 0;
        {
          DN_U64 mem_p      = DN_MemListPos(arena.mem);
          void  *ptr        = DN_ArenaAlloc(&arena, alloc_size, alignment, DN_ZMem_Yes);
          first_ptr_address = DN_Cast(uintptr_t) ptr;
          DN_Memset(ptr, 'z', alloc_size);
          DN_MemListPopTo(arena.mem, mem_p);
        }

        char *ptr = DN_Cast(char *) DN_ArenaAlloc(&arena, alloc_size, alignment, DN_ZMem_Yes);

        DN_TestVerifyUSizeEq(&result, first_ptr_address, DN_Cast(uintptr_t) ptr);

        for (DN_USize i = 0; i < alloc_size; i++)
          DN_TestVerifyExpr(&result, ptr[i] == 0);
      }

      for (DN_TestScopeF(&result, "[Arena] Grows naturally, 1mb + 4mb")) {
        DN_MemList mem   = DN_MemListFromHeap(DN_Megabytes(2), DN_Megabytes(2), DN_MemFlags_Nil, DN_OS_HeapInitVirtual());
        DN_Arena   arena = DN_ArenaFromMemList(&mem);
        DN_DEFER { DN_MemListDeinit(&mem); };

        char *ptr_1mb = DN_ArenaNewArray(&arena, char, DN_Megabytes(1), DN_ZMem_Yes);
        char *ptr_4mb = DN_ArenaNewArray(&arena, char, DN_Megabytes(4), DN_ZMem_Yes);
        DN_TestVerifyExpr(&result, ptr_1mb != nullptr);
        DN_TestVerifyExpr(&result, ptr_4mb != nullptr);

        DN_MemBlock const *block_4mb_begin = arena.mem->curr;
        char const        *block_4mb_end   = DN_Cast(char *) block_4mb_begin + block_4mb_begin->reserve;

        DN_MemBlock const *block_1mb_begin = block_4mb_begin->prev;
        DN_TestVerifyExprF(&result, block_1mb_begin != nullptr, "New block should have been allocated");
        char const *block_1mb_end = DN_Cast(char *) block_1mb_begin + block_1mb_begin->reserve;

        DN_TestVerifyExprF(&result, block_1mb_begin != block_4mb_begin, "New block should have been allocated and linked");
        DN_TestVerifyExprF(&result, ptr_1mb >= DN_Cast(char *) block_1mb_begin && ptr_1mb <= block_1mb_end, "Pointer was not allocated from correct memory block");
        DN_TestVerifyExprF(&result, ptr_4mb >= DN_Cast(char *) block_4mb_begin && ptr_4mb <= block_4mb_end, "Pointer was not allocated from correct memory block");
      }

      for (DN_TestScopeF(&result, "[Arena] Grows naturally, 1mb, temp memory 4mb")) {
        DN_MemList mem   = DN_MemListFromHeap(DN_Megabytes(2), DN_Megabytes(2), DN_MemFlags_Nil, DN_OS_HeapInitVirtual());
        DN_Arena   arena = DN_ArenaFromMemList(&mem);
        DN_DEFER { DN_MemListDeinit(&mem); };

        char *ptr_1mb = DN_Cast(char *) DN_ArenaAlloc(&arena, DN_Megabytes(1), 1, DN_ZMem_Yes);
        DN_TestVerifyExpr(&result, ptr_1mb != nullptr);

        DN_Arena temp = DN_ArenaTempBeginFromArena(&arena);
        {
          char *ptr_4mb = DN_ArenaNewArray(&temp, char, DN_Megabytes(4), DN_ZMem_Yes);
          DN_TestVerifyExpr(&result, ptr_4mb != nullptr);

          DN_MemBlock const *block_4mb_begin = arena.mem->curr;
          char const        *block_4mb_end   = DN_Cast(char *) block_4mb_begin + block_4mb_begin->reserve;

          DN_MemBlock const *block_1mb_begin = block_4mb_begin->prev;
          char const        *block_1mb_end   = DN_Cast(char *) block_1mb_begin + block_1mb_begin->reserve;

          DN_TestVerifyExprF(&result, block_1mb_begin != block_4mb_begin, "New block should have been allocated and linked");
          DN_TestVerifyExprF(&result, ptr_1mb >= DN_Cast(char *) block_1mb_begin && ptr_1mb <= block_1mb_end, "Pointer was not allocated from correct memory block");
          DN_TestVerifyExprF(&result, ptr_4mb >= DN_Cast(char *) block_4mb_begin && ptr_4mb <= block_4mb_end, "Pointer was not allocated from correct memory block");
        }
        DN_ArenaTempEnd(&temp, DN_ArenaReset_Yes);
        DN_TestVerifyExpr(&result, arena.mem->curr->prev == nullptr);
        DN_TestVerifyExprF(&result,
                           arena.mem->curr->reserve >= DN_Megabytes(1),
                           "size=%" PRIu64 "MiB (%" PRIu64 "B), expect=%" PRIu64 "B",
                           (arena.mem->curr->reserve / 1024 / 1024),
                           arena.mem->curr->reserve,
                           DN_Megabytes(1));
      }
    }

    // NOTE: Hex/Bytes
    {
      DN_TcScratch scratch = DN_TcScratchBeginArena(&arena_, 1);
      DN_DEFER { DN_TcScratchEnd(&scratch); };

      for (DN_TestScopeF(&result, "[Hex/Bytes] Convert 0x123")) {
        uint64_t val = DN_U64FromHexStr8Unsafe(DN_Str8Lit("0x123"));
        DN_TestVerifyExprF(&result, val == 0x123, "val: %" PRIu64, val);
      }

      for (DN_TestScopeF(&result, "[Hex/Bytes] Convert 0xFFFF")) {
        uint64_t val = DN_U64FromHexStr8Unsafe(DN_Str8Lit("0xFFFF"));
        DN_TestVerifyExprF(&result, val == 0xFFFF, "val: %" PRIu64, val);
      }

      for (DN_TestScopeF(&result, "[Hex/Bytes] Convert FFFF")) {
        uint64_t val = DN_U64FromHexStr8Unsafe(DN_Str8Lit("FFFF"));
        DN_TestVerifyExprF(&result, val == 0xFFFF, "val: %" PRIu64, val);
      }

      for (DN_TestScopeF(&result, "[Hex/Bytes] Convert abCD")) {
        uint64_t val = DN_U64FromHexStr8Unsafe(DN_Str8Lit("abCD"));
        DN_TestVerifyExprF(&result, val == 0xabCD, "val: %" PRIu64, val);
      }

      for (DN_TestScopeF(&result, "[Hex/Bytes] Convert 0xabCD")) {
        uint64_t val = DN_U64FromHexStr8Unsafe(DN_Str8Lit("0xabCD"));
        DN_TestVerifyExprF(&result, val == 0xabCD, "val: %" PRIu64, val);
      }

      for (DN_TestScopeF(&result, "[Hex/Bytes] Convert 0x")) {
        uint64_t val = DN_U64FromHexStr8Unsafe(DN_Str8Lit("0x"));
        DN_TestVerifyExprF(&result, val == 0x0, "val: %" PRIu64, val);
      }

      for (DN_TestScopeF(&result, "[Hex/Bytes] Convert 0X")) {
        uint64_t val = DN_U64FromHexStr8Unsafe(DN_Str8Lit("0X"));
        DN_TestVerifyExprF(&result, val == 0x0, "val: %" PRIu64, val);
      }

      for (DN_TestScopeF(&result, "[Hex/Bytes] Convert 3")) {
        uint64_t val = DN_U64FromHexStr8Unsafe(DN_Str8Lit("3"));
        DN_TestVerifyExprF(&result, val == 3, "val: %" PRIu64, val);
      }

      for (DN_TestScopeF(&result, "[Hex/Bytes] Convert f")) {
        DN_U64FromResult res = DN_U64FromHexStr8(DN_Str8Lit("f"));
        DN_TestVerifyExpr(&result, res.success);
        DN_TestVerifyExpr(&result, res.value == 0xf);
      }

      for (DN_TestScopeF(&result, "[Hex/Bytes] Convert g")) {
        DN_U64FromResult res = DN_U64FromHexStr8(DN_Str8Lit("g"));
        DN_TestVerifyExpr(&result, !res.success);
      }

      for (DN_TestScopeF(&result, "[Hex/Bytes] Convert -0x3")) {
        DN_U64FromResult res = DN_U64FromHexStr8(DN_Str8Lit("-0x3"));
        DN_TestVerifyExpr(&result, !res.success);
      }

      DN_U32 number = 0xd095f6;
      for (DN_TestScopeF(&result, "[Hex/Bytes] Convert %x to string", number)) {
        DN_Str8 number_hex = DN_Str8HexFromPtrBytesArena(&number, sizeof(number), &scratch.arena, DN_TrimLeadingZero_No);
        DN_TestVerifyStr8EqF(&result, number_hex, DN_Str8Lit("f695d000"), "number_hex=%.*s", DN_Str8PrintFmt(number_hex));
      }

      number = 0xf6ed00;
      for (DN_TestScopeF(&result, "[Hex/Bytes] Convert %x to string", number)) {
        DN_Str8 number_hex = DN_Str8HexFromPtrBytesArena(&number, sizeof(number), &scratch.arena, DN_TrimLeadingZero_No);
        DN_TestVerifyStr8EqF(&result, number_hex, DN_Str8Lit("00edf600"), "number_hex=%.*s", DN_Str8PrintFmt(number_hex));
      }

      DN_Str8 hex = DN_Str8Lit("0xf6ed00");
      for (DN_TestScopeF(&result, "[Hex/Bytes] Convert %.*s to bytes", DN_Str8PrintFmt(hex))) {
        DN_Str8 bytes = DN_Str8BytesFromStr8HexArena(hex, &scratch.arena);
        DN_TestVerifyStr8EqF(&result,
                             bytes,
                             DN_Str8Lit("\xf6\xed\x00"),
                             "number_hex=%.*s",
                             DN_Str8PrintFmt(DN_Str8HexFromPtrBytesArena(bytes.data, bytes.count, &scratch.arena, DN_TrimLeadingZero_No)));
      }

      for (DN_TestScopeF(&result, "[Hex/Bytes] Convert empty bytes to string")) {
        DN_Str8 bytes  = DN_Str8Lit("");
        DN_Str8 as_hex = DN_Str8HexFromPtrBytesArena(bytes.data, bytes.count, &scratch.arena, DN_TrimLeadingZero_No);
        DN_TestVerifyStr8EqF(&result, as_hex, DN_Str8Lit(""), "as_hex=%.*s", DN_Str8PrintFmt(as_hex));
      }
    }

    // NOTE: BSearch
    {
      for (DN_TestScopeF(&result, "[BSearch] Search array of 1 item")) {
        DN_U32           array[] = {1};
        DN_BSearchResult search  = {};

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 0U, DN_BSearchType_Match);
        DN_TestVerifyExpr(&result, !search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 0);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 1U, DN_BSearchType_Match);
        DN_TestVerifyExpr(&result, search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 0);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 2U, DN_BSearchType_Match);
        DN_TestVerifyExpr(&result, !search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 1);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 0U, DN_BSearchType_LowerBound);
        DN_TestVerifyExpr(&result, search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 0);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 1U, DN_BSearchType_LowerBound);
        DN_TestVerifyExpr(&result, search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 0);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 2U, DN_BSearchType_LowerBound);
        DN_TestVerifyExpr(&result, !search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 1);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 0U, DN_BSearchType_UpperBound);
        DN_TestVerifyExpr(&result, search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 0);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 1U, DN_BSearchType_UpperBound);
        DN_TestVerifyExpr(&result, !search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 1);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 2U, DN_BSearchType_UpperBound);
        DN_TestVerifyExpr(&result, !search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 1);
      }

      for (DN_TestScopeF(&result, "[BSearch] Search array of 3 items")) {
        DN_U32              array[] = {1, 2, 3};
        DN_BSearchResult search  = {};

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 0U, DN_BSearchType_Match);
        DN_TestVerifyExpr(&result, !search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 0);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 1U, DN_BSearchType_Match);
        DN_TestVerifyExpr(&result, search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 0);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 2U, DN_BSearchType_Match);
        DN_TestVerifyExpr(&result, search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 1);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 3U, DN_BSearchType_Match);
        DN_TestVerifyExpr(&result, search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 2);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 4U, DN_BSearchType_Match);
        DN_TestVerifyExpr(&result, !search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 3);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 0U, DN_BSearchType_LowerBound);
        DN_TestVerifyExpr(&result, search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 0);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 1U, DN_BSearchType_LowerBound);
        DN_TestVerifyExpr(&result, search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 0);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 2U, DN_BSearchType_LowerBound);
        DN_TestVerifyExpr(&result, search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 1);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 3U, DN_BSearchType_LowerBound);
        DN_TestVerifyExpr(&result, search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 2);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 4U, DN_BSearchType_LowerBound);
        DN_TestVerifyExpr(&result, !search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 3);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 0U, DN_BSearchType_UpperBound);
        DN_TestVerifyExpr(&result, search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 0);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 1U, DN_BSearchType_UpperBound);
        DN_TestVerifyExpr(&result, search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 1);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 2U, DN_BSearchType_UpperBound);
        DN_TestVerifyExpr(&result, search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 2);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 3U, DN_BSearchType_UpperBound);
        DN_TestVerifyExpr(&result, !search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 3);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 4U, DN_BSearchType_UpperBound);
        DN_TestVerifyExpr(&result, !search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 3);
      }

      for (DN_TestScopeF(&result, "[BSearch] Search array with duplicate items")) {
        DN_U32              array[] = {1, 1, 2, 2, 3};
        DN_BSearchResult search  = {};

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 0U, DN_BSearchType_Match);
        DN_TestVerifyExpr(&result, !search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 0);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 1U, DN_BSearchType_Match);
        DN_TestVerifyExpr(&result, search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 0);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 2U, DN_BSearchType_Match);
        DN_TestVerifyExpr(&result, search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 2);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 3U, DN_BSearchType_Match);
        DN_TestVerifyExpr(&result, search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 4);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 4U, DN_BSearchType_Match);
        DN_TestVerifyExpr(&result, !search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 5);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 0U, DN_BSearchType_LowerBound);
        DN_TestVerifyExpr(&result, search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 0);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 1U, DN_BSearchType_LowerBound);
        DN_TestVerifyExpr(&result, search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 0);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 2U, DN_BSearchType_LowerBound);
        DN_TestVerifyExpr(&result, search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 2);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 3U, DN_BSearchType_LowerBound);
        DN_TestVerifyExpr(&result, search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 4);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 4U, DN_BSearchType_LowerBound);
        DN_TestVerifyExpr(&result, !search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 5);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 0U, DN_BSearchType_UpperBound);
        DN_TestVerifyExpr(&result, search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 0);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 1U, DN_BSearchType_UpperBound);
        DN_TestVerifyExpr(&result, search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 2);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 2U, DN_BSearchType_UpperBound);
        DN_TestVerifyExpr(&result, search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 4);

        search = DN_BSearchU32(array, DN_ArrayCountU(array), 3U, DN_BSearchType_UpperBound);
        DN_TestVerifyExpr(&result, !search.found);
        DN_TestVerifyUSizeEq(&result, search.index, 5);
      }
    }

    // NOTE: IArray
    {
      for (DN_TestScopeF(&result, "[IArray] Make item")) {
        struct CustomArray {
          int     *data;
          DN_USize count;
          DN_USize max;
        };

        int         array_buffer[16];
        CustomArray array = {};
        array.data        = array_buffer;
        array.max         = DN_ArrayCountU(array_buffer);

        int *item = DN_IArrayMake(&array, DN_ZMem_Yes);
        DN_TestVerifyExpr(&result, item != nullptr);
        DN_TestVerifyUSizeEq(&result, array.count, 1);
      }
    }

    // NOTE: Array
    {
      for (DN_TestScopeF(&result, "[Array] Positive count, middle of array, stable erase")) {
        int                 arr[]      = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        DN_USize            size       = 10;
        DN_ArrayEraseResult erase      = DN_ArrayEraseRange(arr, &size, sizeof(arr[0]), 3, 2, DN_ArrayErase_Stable);
        int                 expected[] = {0, 1, 2, 5, 6, 7, 8, 9};
        DN_TestVerifyUSizeEq(&result, erase.items_erased, 2);
        DN_TestVerifyUSizeEq(&result, erase.it_index, 2);
        DN_TestVerifyUSizeEq(&result, size, 8);
        DN_TestVerifyExpr(&result, DN_Memcmp(arr, expected, size * sizeof(arr[0])) == 0);
      }

      for (DN_TestScopeF(&result, "[Array] Negative count, middle of array, stable erase")) {
        int                 arr[]      = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        DN_USize            size       = 10;
        DN_ArrayEraseResult erase      = DN_ArrayEraseRange(arr, &size, sizeof(arr[0]), 5, -3, DN_ArrayErase_Stable);
        int                 expected[] = {0, 1, 5, 6, 7, 8, 9};
        DN_TestVerifyUSizeEq(&result, erase.items_erased, 3);
        DN_TestVerifyUSizeEq(&result, erase.it_index, 1);
        DN_TestVerifyUSizeEq(&result, size, 7);
        DN_TestVerifyExpr(&result, DN_Memcmp(arr, expected, size * sizeof(arr[0])) == 0);
      }

      for (DN_TestScopeF(&result, "[Array] count = -1, stable erase")) {
        int                 arr[]      = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        DN_USize            size       = 10;
        DN_ArrayEraseResult erase      = DN_ArrayEraseRange(arr, &size, sizeof(arr[0]), 5, -1, DN_ArrayErase_Stable);
        int                 expected[] = {0, 1, 2, 3, 5, 6, 7, 8, 9};
        DN_TestVerifyUSizeEq(&result, erase.items_erased, 1);
        DN_TestVerifyUSizeEqF(&result, erase.it_index, 3, "lhs=%zu", erase.it_index);
        DN_TestVerifyUSizeEq(&result, size, 9);
        DN_TestVerifyExpr(&result, DN_Memcmp(arr, expected, size * sizeof(arr[0])) == 0);
      }

      for (DN_TestScopeF(&result, "[Array] Positive count, unstable erase")) {
        int                 arr[]      = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        DN_USize            size       = 10;
        DN_ArrayEraseResult erase      = DN_ArrayEraseRange(arr, &size, sizeof(arr[0]), 3, 2, DN_ArrayErase_Unstable);
        int                 expected[] = {0, 1, 2, 8, 9, 5, 6, 7};
        DN_TestVerifyUSizeEq(&result, erase.items_erased, 2);
        DN_TestVerifyUSizeEq(&result, erase.it_index, 2);
        DN_TestVerifyUSizeEq(&result, size, 8);
        DN_TestVerifyExpr(&result, DN_Memcmp(arr, expected, size * sizeof(arr[0])) == 0);
      }

      for (DN_TestScopeF(&result, "[Array] Negative count, unstable erase")) {
        int                 arr[]      = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        DN_USize            size       = 10;
        DN_ArrayEraseResult erase      = DN_ArrayEraseRange(arr, &size, sizeof(arr[0]), 5, -3, DN_ArrayErase_Unstable);
        int                 expected[] = {0, 1, 7, 8, 9, 5, 6};
        DN_TestVerifyUSizeEq(&result, erase.items_erased, 3);
        DN_TestVerifyUSizeEq(&result, erase.it_index, 1);
        DN_TestVerifyUSizeEq(&result, size, 7);
        DN_TestVerifyExpr(&result, DN_Memcmp(arr, expected, size * sizeof(arr[0])) == 0);
      }

      for (DN_TestScopeF(&result, "[Array] Edge case - begin_index at start, negative count")) {
        int                 arr[]      = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        DN_USize            size       = 10;
        DN_ArrayEraseResult erase      = DN_ArrayEraseRange(arr, &size, sizeof(arr[0]), 0, -2, DN_ArrayErase_Stable);
        int                 expected[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        DN_TestVerifyUSizeEq(&result, erase.items_erased, 0);
        DN_TestVerifyUSizeEq(&result, erase.it_index, 0);
        DN_TestVerifyUSizeEq(&result, size, 10);
        DN_TestVerifyExpr(&result, DN_Memcmp(arr, expected, size * sizeof(arr[0])) == 0);
      }

      for (DN_TestScopeF(&result, "[Array] Edge case - begin_index at end, positive count")) {
        int                 arr[]      = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        DN_USize            size       = 10;
        DN_ArrayEraseResult erase      = DN_ArrayEraseRange(arr, &size, sizeof(arr[0]), 9, 2, DN_ArrayErase_Stable);
        int                 expected[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
        DN_TestVerifyUSizeEq(&result, erase.items_erased, 1);
        DN_TestVerifyUSizeEq(&result, erase.it_index, 9);
        DN_TestVerifyUSizeEq(&result, size, 9);
        DN_TestVerifyExpr(&result, DN_Memcmp(arr, expected, size * sizeof(arr[0])) == 0);
      }

      for (DN_TestScopeF(&result, "[Array] Invalid input - count = 0")) {
        int                 arr[]      = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        DN_USize            size       = 10;
        DN_ArrayEraseResult erase      = DN_ArrayEraseRange(arr, &size, sizeof(arr[0]), 5, 0, DN_ArrayErase_Stable);
        int                 expected[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        DN_TestVerifyUSizeEq(&result, erase.items_erased, 0);
        DN_TestVerifyUSizeEq(&result, erase.it_index, 5);
        DN_TestVerifyUSizeEq(&result, size, 10);
        DN_TestVerifyExpr(&result, DN_Memcmp(arr, expected, size * sizeof(arr[0])) == 0);
      }

      for (DN_TestScopeF(&result, "[Array] Invalid input - null data")) {
        DN_USize            size  = 10;
        DN_ArrayEraseResult erase = DN_ArrayEraseRange(nullptr, &size, sizeof(int), 5, 2, DN_ArrayErase_Stable);
        DN_TestVerifyUSizeEq(&result, erase.items_erased, 0);
        DN_TestVerifyUSizeEq(&result, erase.it_index, 5);
        DN_TestVerifyUSizeEq(&result, size, 10);
      }

      for (DN_TestScopeF(&result, "[Array] Invalid input - null size")) {
        int                 arr[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        DN_ArrayEraseResult erase = DN_ArrayEraseRange(arr, NULL, sizeof(arr[0]), 5, 2, DN_ArrayErase_Stable);
        DN_TestVerifyUSizeEq(&result, erase.items_erased, 0);
        DN_TestVerifyUSizeEq(&result, erase.it_index, 5);
      }

      for (DN_TestScopeF(&result, "[Array] Invalid input - empty array")) {
        int                 arr[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        DN_USize            size  = 0;
        DN_ArrayEraseResult erase = DN_ArrayEraseRange(arr, &size, sizeof(arr[0]), 5, 2, DN_ArrayErase_Stable);
        DN_TestVerifyUSizeEq(&result, erase.items_erased, 0);
        DN_TestVerifyUSizeEq(&result, erase.it_index, 5);
        DN_TestVerifyUSizeEq(&result, size, 0);
      }

      for (DN_TestScopeF(&result, "[Array] Out-of-bounds begin_index")) {
        int                 arr[]      = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        DN_USize            size       = 10;
        DN_ArrayEraseResult erase      = DN_ArrayEraseRange(arr, &size, sizeof(arr[0]), 15, 2, DN_ArrayErase_Stable);
        int                 expected[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        DN_TestVerifyUSizeEq(&result, erase.items_erased, 0);
        DN_TestVerifyUSizeEq(&result, erase.it_index, 9);
        DN_TestVerifyUSizeEq(&result, size, 10);
        DN_TestVerifyExpr(&result, DN_Memcmp(arr, expected, size * sizeof(arr[0])) == 0);
      }
    }

    // NOTE: M4
    {
      for (DN_TestScopeF(&result, "[M4] Simple translate and scale matrix")) {
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

        DN_TestVerifyExprF(&result,
                           memcmp(mul_result.columns, EXPECT.columns, sizeof(EXPECT)) == 0,
                           "\nresult =\n%s\nexpected =\n%s",
                           DN_M4ColumnMajorString(mul_result).data,
                           DN_M4ColumnMajorString(EXPECT).data);
      }
    }

    // NOTE: OS
    #if DN_WITH_OS
    {
      for (DN_TestScopeF(&result, "[OS] Generate secure RNG 32 bytes")) {
        char const ZERO[32]  = {};
        char       buf[32]   = {};
        DN_OS_GenBytesSecure(buf, DN_ArrayCountU(buf));
        DN_TestVerifyExpr(&result, DN_Memcmp(buf, ZERO, DN_ArrayCountU(buf)) != 0);
      }

      for (DN_TestScopeF(&result, "[OS] Query executable directory")) {
        DN_TcScratch scratch = DN_TcScratchBeginArena(&arena_, 1);
        DN_Str8      os_result = DN_OS_ExeDir(&scratch.arena);
        DN_TestVerifyExpr(&result, os_result.count > 0);
        DN_TestVerifyExprF(&result, DN_OS_PathIsDir(os_result), "result(%zu): %.*s", os_result.count, DN_Str8PrintFmt(os_result));
        DN_TcScratchEnd(&scratch);
      }

      for (DN_TestScopeF(&result, "[OS] DN_OS_PerfCounterNow")) {
        uint64_t os_result = DN_OS_PerfCounterNow();
        DN_TestVerifyExpr(&result, os_result != 0);
      }

      for (DN_TestScopeF(&result, "[OS] Consecutive ticks are ordered")) {
        uint64_t a = DN_OS_PerfCounterNow();
        uint64_t b = DN_OS_PerfCounterNow();
        DN_TestVerifyExprF(&result, b >= a, "a: %" PRIu64 ", b: %" PRIu64, a, b);
      }

      for (DN_TestScopeF(&result, "[OS] Ticks to time are a correct order of magnitude")) {
        uint64_t a  = DN_OS_PerfCounterNow();
        uint64_t b  = DN_OS_PerfCounterNow();
        DN_F64   s  = DN_OS_PerfCounterS(a, b);
        DN_F64   ms = DN_OS_PerfCounterMs(a, b);
        DN_F64   us = DN_OS_PerfCounterUs(a, b);
        DN_F64   ns = DN_OS_PerfCounterNs(a, b);
        DN_TestVerifyF64LessThanEq(&result, s, ms);
        DN_TestVerifyF64LessThanEq(&result, ms, us);
        DN_TestVerifyF64LessThanEq(&result, us, ns);
      }

      for (DN_TestScopeF(&result, "[OS] Make directory recursive \"abcd/efgh\"")) {
        DN_TestVerifyExprF(&result, DN_OS_PathMakeDir(DN_Str8Lit("abcd/efgh")), "Failed to make directory");
        DN_TestVerifyExprF(&result, DN_OS_PathIsDir(DN_Str8Lit("abcd")), "Directory was not made");
        DN_TestVerifyExprF(&result, DN_OS_PathIsDir(DN_Str8Lit("abcd/efgh")), "Subdirectory was not made");
        DN_TestVerifyExprF(&result, DN_OS_PathIsFile(DN_Str8Lit("abcd")) == false, "This function should only return true for files");
        DN_TestVerifyExprF(&result, DN_OS_PathIsFile(DN_Str8Lit("abcd/efgh")) == false, "This function should only return true for files");
        DN_TestVerifyExprF(&result, DN_OS_PathDelete(DN_Str8Lit("abcd/efgh")), "Failed to delete directory");
        DN_TestVerifyExprF(&result, DN_OS_PathDelete(DN_Str8Lit("abcd")), "Failed to cleanup directory");
      }

      for (DN_TestScopeF(&result, "[OS] File write, read, copy, move and delete")) {
        DN_Str8 const SRC_FILE     = DN_Str8Lit("dn_result_file");
        DN_B32        write_result = DN_OS_FileWriteAll(SRC_FILE, DN_Str8Lit("1234"), nullptr);
        DN_TestVerifyExpr(&result, write_result);
        DN_TestVerifyExpr(&result, DN_OS_PathIsFile(SRC_FILE));

        DN_TcScratch scratch = DN_TcScratchBeginArena(&arena_, 1);
        DN_Str8      read_file = DN_OS_FileReadAllArena(&scratch.arena, SRC_FILE, nullptr);
        DN_TestVerifyExprF(&result, read_file.count > 0, "Failed to load file");
        DN_TestVerifyExprF(&result, read_file.count == 4, "File read wrong amount of bytes (%zu)", read_file.count);
        DN_TestVerifyStr8EqF(&result, read_file, DN_Str8Lit("1234"), "Read %zu bytes instead of the expected 4: '%.*s'", read_file.count, DN_Str8PrintFmt(read_file));

        DN_Str8 const COPY_FILE   = DN_Str8Lit("dn_result_file_copy");
        DN_B32        copy_result = DN_OS_FileCopy(SRC_FILE, COPY_FILE, true, nullptr);
        DN_TestVerifyExpr(&result, copy_result);
        DN_TestVerifyExpr(&result, DN_OS_PathIsFile(COPY_FILE));

        DN_Str8 const MOVE_FILE   = DN_Str8Lit("dn_result_file_move");
        DN_B32        move_result = DN_OS_FileMove(COPY_FILE, MOVE_FILE, true, nullptr);
        DN_TestVerifyExpr(&result, move_result);
        DN_TestVerifyExpr(&result, DN_OS_PathIsFile(MOVE_FILE));
        DN_TestVerifyExprF(&result, DN_OS_PathIsFile(COPY_FILE) == false, "Moving a file should remove the original");

        DN_B32 delete_src_file   = DN_OS_PathDelete(SRC_FILE);
        DN_B32 delete_moved_file = DN_OS_PathDelete(MOVE_FILE);
        DN_TestVerifyExpr(&result, delete_src_file);
        DN_TestVerifyExpr(&result, delete_moved_file);

        DN_B32 delete_non_existent_src_file   = DN_OS_PathDelete(SRC_FILE);
        DN_B32 delete_non_existent_moved_file = DN_OS_PathDelete(MOVE_FILE);
        DN_TestVerifyExpr(&result, delete_non_existent_moved_file == false);
        DN_TestVerifyExpr(&result, delete_non_existent_src_file == false);
        DN_TcScratchEnd(&scratch);
      }

      for (DN_TestScopeF(&result, "[OS] Wait timeout")) {
        DN_OSSemaphore sem = DN_OS_SemaphoreInit(0);
        DN_DEFER { DN_OS_SemaphoreDeinit(&sem); };

        DN_U64                   begin       = DN_OS_PerfCounterNow();
        DN_OSSemaphoreWaitResult wait_result = DN_OS_SemaphoreWait(&sem, 100);
        DN_U64                   end         = DN_OS_PerfCounterNow();
        DN_TestVerifyUSizeEqF(&result, wait_result, DN_OSSemaphoreWaitResult_Timeout, "Received wait result %zu", wait_result);
        DN_F64 elapsed_ms = DN_OS_PerfCounterMs(begin, end);
        DN_TestVerifyExprF(&result, elapsed_ms >= 80 && elapsed_ms <= 120, "Expected to sleep for ~100ms, slept %f ms", elapsed_ms);
      }

      for (DN_TestScopeF(&result, "[OS] Wait success")) {
        DN_OSSemaphore sem = DN_OS_SemaphoreInit(0);
        DN_DEFER { DN_OS_SemaphoreDeinit(&sem); };

        DN_OS_SemaphoreIncrement(&sem, 1);
        DN_OSSemaphoreWaitResult wait_result = DN_OS_SemaphoreWait(&sem, 0);
        DN_TestVerifyUSizeEqF(&result, wait_result, DN_OSSemaphoreWaitResult_Success, "Received wait result %zu", wait_result);
      }

      for (DN_TestScopeF(&result, "[OS] Lock mutex")) {
        DN_OSMutex mutex = DN_OS_MutexInit();
        DN_DEFER { DN_OS_MutexDeinit(&mutex); };

        DN_OS_MutexLock(&mutex);
        DN_OS_MutexUnlock(&mutex);
      }

      for (DN_TestScopeF(&result, "[OS] Lock and timeout condition variable")) {
        DN_OSMutex             mutex = DN_OS_MutexInit();
        DN_OSConditionVariable cv    = DN_OS_ConditionVariableInit();
        DN_DEFER {
          DN_OS_MutexDeinit(&mutex);
          DN_OS_ConditionVariableDeinit(&cv);
        };

        DN_U64 begin = DN_OS_PerfCounterNow();
        DN_OS_ConditionVariableWait(&cv, &mutex, 100);
        DN_U64 end        = DN_OS_PerfCounterNow();
        DN_F64 elapsed_ms = DN_OS_PerfCounterMs(begin, end);
        DN_TestVerifyExprF(&result, elapsed_ms >= 99 && elapsed_ms <= 120, "Expected to sleep for ~100ms, slept %f ms", elapsed_ms);
      }
    }
    #endif // #if DN_WITH_OS

    // NOTE: Rect
    {
      for (DN_TestScopeF(&result, "[Rect] No intersection")) {
        DN_Rect a  = DN_RectFrom2V2(DN_V2F32From1N(0), DN_V2F32From2N(100, 100));
        DN_Rect b  = DN_RectFrom2V2(DN_V2F32From2N(200, 0), DN_V2F32From2N(200, 200));
        DN_Rect ab = DN_RectIntersection(a, b);

        DN_V2F32 ab_max = ab.pos + ab.size;
        DN_TestVerifyExprF(&result,
                           ab.pos.x == 0 && ab.pos.y == 0 && ab_max.x == 0 && ab_max.y == 0,
                           "ab = { min.x = %.2f, min.y = %.2f, max.x = %.2f. max.y = %.2f }",
                           ab.pos.x, ab.pos.y, ab_max.x, ab_max.y);
      }

      for (DN_TestScopeF(&result, "[Rect] A's min intersects B")) {
        DN_Rect a  = DN_RectFrom2V2(DN_V2F32From2N(50, 50), DN_V2F32From2N(100, 100));
        DN_Rect b  = DN_RectFrom2V2(DN_V2F32From2N(0, 0), DN_V2F32From2N(100, 100));
        DN_Rect ab = DN_RectIntersection(a, b);

        DN_V2F32 ab_max = ab.pos + ab.size;
        DN_TestVerifyExprF(&result,
                           ab.pos.x == 50 && ab.pos.y == 50 && ab_max.x == 100 && ab_max.y == 100,
                           "ab = { min.x = %.2f, min.y = %.2f, max.x = %.2f. max.y = %.2f }",
                           ab.pos.x, ab.pos.y, ab_max.x, ab_max.y);
      }

      for (DN_TestScopeF(&result, "[Rect] B's min intersects A")) {
        DN_Rect a  = DN_RectFrom2V2(DN_V2F32From2N(0, 0), DN_V2F32From2N(100, 100));
        DN_Rect b  = DN_RectFrom2V2(DN_V2F32From2N(50, 50), DN_V2F32From2N(100, 100));
        DN_Rect ab = DN_RectIntersection(a, b);

        DN_V2F32 ab_max = ab.pos + ab.size;
        DN_TestVerifyExprF(&result,
                           ab.pos.x == 50 && ab.pos.y == 50 && ab_max.x == 100 && ab_max.y == 100,
                           "ab = { min.x = %.2f, min.y = %.2f, max.x = %.2f. max.y = %.2f }",
                           ab.pos.x, ab.pos.y, ab_max.x, ab_max.y);
      }

      for (DN_TestScopeF(&result, "[Rect] A's max intersects B")) {
        DN_Rect a  = DN_RectFrom2V2(DN_V2F32From2N(-50, -50), DN_V2F32From2N(100, 100));
        DN_Rect b  = DN_RectFrom2V2(DN_V2F32From2N(0, 0), DN_V2F32From2N(100, 100));
        DN_Rect ab = DN_RectIntersection(a, b);

        DN_V2F32 ab_max = ab.pos + ab.size;
        DN_TestVerifyExprF(&result,
                           ab.pos.x == 0 && ab.pos.y == 0 && ab_max.x == 50 && ab_max.y == 50,
                           "ab = { min.x = %.2f, min.y = %.2f, max.x = %.2f. max.y = %.2f }",
                           ab.pos.x, ab.pos.y, ab_max.x, ab_max.y);
      }

      for (DN_TestScopeF(&result, "[Rect] B's max intersects A")) {
        DN_Rect a  = DN_RectFrom2V2(DN_V2F32From2N(0, 0), DN_V2F32From2N(100, 100));
        DN_Rect b  = DN_RectFrom2V2(DN_V2F32From2N(-50, -50), DN_V2F32From2N(100, 100));
        DN_Rect ab = DN_RectIntersection(a, b);

        DN_V2F32 ab_max = ab.pos + ab.size;
        DN_TestVerifyExprF(&result,
                           ab.pos.x == 0 && ab.pos.y == 0 && ab_max.x == 50 && ab_max.y == 50,
                           "ab = { min.x = %.2f, min.y = %.2f, max.x = %.2f. max.y = %.2f }",
                           ab.pos.x, ab.pos.y, ab_max.x, ab_max.y);
      }

      for (DN_TestScopeF(&result, "[Rect] B contains A")) {
        DN_Rect a  = DN_RectFrom2V2(DN_V2F32From2N(25, 25), DN_V2F32From2N(25, 25));
        DN_Rect b  = DN_RectFrom2V2(DN_V2F32From2N(0, 0), DN_V2F32From2N(100, 100));
        DN_Rect ab = DN_RectIntersection(a, b);

        DN_V2F32 ab_max = ab.pos + ab.size;
        DN_TestVerifyExprF(&result,
                           ab.pos.x == 25 && ab.pos.y == 25 && ab_max.x == 50 && ab_max.y == 50,
                           "ab = { min.x = %.2f, min.y = %.2f, max.x = %.2f. max.y = %.2f }",
                           ab.pos.x, ab.pos.y, ab_max.x, ab_max.y);
      }

      for (DN_TestScopeF(&result, "[Rect] A contains B")) {
        DN_Rect a  = DN_RectFrom2V2(DN_V2F32From2N(0, 0), DN_V2F32From2N(100, 100));
        DN_Rect b  = DN_RectFrom2V2(DN_V2F32From2N(25, 25), DN_V2F32From2N(25, 25));
        DN_Rect ab = DN_RectIntersection(a, b);

        DN_V2F32 ab_max = ab.pos + ab.size;
        DN_TestVerifyExprF(&result,
                           ab.pos.x == 25 && ab.pos.y == 25 && ab_max.x == 50 && ab_max.y == 50,
                           "ab = { min.x = %.2f, min.y = %.2f, max.x = %.2f. max.y = %.2f }",
                           ab.pos.x, ab.pos.y, ab_max.x, ab_max.y);
      }

      for (DN_TestScopeF(&result, "[Rect] A equals B")) {
        DN_Rect a  = DN_RectFrom2V2(DN_V2F32From2N(0, 0), DN_V2F32From2N(100, 100));
        DN_Rect b  = a;
        DN_Rect ab = DN_RectIntersection(a, b);

        DN_V2F32 ab_max = ab.pos + ab.size;
        DN_TestVerifyExprF(&result,
                           ab.pos.x == 0 && ab.pos.y == 0 && ab_max.x == 100 && ab_max.y == 100,
                           "ab = { min.x = %.2f, min.y = %.2f, max.x = %.2f. max.y = %.2f }",
                           ab.pos.x, ab.pos.y, ab_max.x, ab_max.y);
      }
    }

    // NOTE: Strings
    {
      for (DN_TestScopeF(&result, "[Strings] Str8 literal")) {
        DN_Str8 string = DN_Str8Lit("AB");
        DN_TestVerifyUSizeEqF(&result, string.count, 2, "size: %zu", string.count);
        DN_TestVerifyExprF(&result, string.data[0] == 'A', "string[0]: %c", string.data[0]);
        DN_TestVerifyExprF(&result, string.data[1] == 'B', "string[1]: %c", string.data[1]);
      }

      for (DN_TestScopeF(&result, "[Strings] C-string length")) {
        DN_USize size = DN_CStr8Count("hello");
        DN_TestVerifyUSizeEqF(&result, size, 5, "size=%zu", size);
      }

      char arena_base[512];
      for (DN_TestScopeF(&result, "[Strings] Format from arena")) {
        DN_MemList mem   = DN_MemListFromBuffer(arena_base, sizeof(arena_base), DN_MemFlags_Nil);
        DN_Arena   arena = DN_ArenaFromMemList(&mem);
        DN_Str8  str8   = DN_Str8FmtArena(&arena, "Foo Bar %d", 5);
        DN_Str8  expect = DN_Str8Lit("Foo Bar 5");
        DN_TestVerifyStr8EqF(&result, str8, expect, "str8=%.*s", DN_Str8PrintFmt(str8), DN_Str8PrintFmt(expect));
      }

      for (DN_TestScopeF(&result, "[Strings] Format from pool")) {
        DN_MemList mem   = DN_MemListFromBuffer(arena_base, sizeof(arena_base), DN_MemFlags_Nil);
        DN_Arena   arena = DN_ArenaFromMemList(&mem);
        DN_Pool    pool  = DN_PoolFromArena(&arena, 0);
        DN_Str8    str8   = DN_Str8FmtPool(&pool, "Foo Bar %d", 5);
        DN_Str8    expect = DN_Str8Lit("Foo Bar 5");
        DN_TestVerifyStr8EqF(&result, str8, expect, "str8=%.*s", DN_Str8PrintFmt(str8), DN_Str8PrintFmt(expect));
      }

      for (DN_TestScopeF(&result, "[Strings] Str8x32 from U64")) {
        DN_Str8x32 str8   = DN_Str8x32FromU64(123456, ' ');
        DN_Str8    expect = DN_Str8Lit("123 456");
        DN_TestVerifyStr8EqF(&result, DN_Str8FromStruct(&str8), expect, "buf_str8=%.*s, expect=%.*s", DN_Str8PrintFmt(str8), DN_Str8PrintFmt(expect));
      }

      for (DN_TestScopeF(&result, "[Strings] Initialise with format string")) {
        DN_TcScratch scratch = DN_TcScratchBeginArena(&arena_, 1);
        DN_Str8      string  = DN_Str8FmtArena(&scratch.arena, "%s", "AB");
        DN_TestVerifyUSizeEqF(&result, string.count, 2, "size: %zu", string.count);
        DN_TestVerifyExprF(&result, string.data[0] == 'A', "string[0]: %c", string.data[0]);
        DN_TestVerifyExprF(&result, string.data[1] == 'B', "string[1]: %c", string.data[1]);
        DN_TestVerifyExprF(&result, string.data[2] == 0, "string[2]: %c", string.data[2]);
        DN_TcScratchEnd(&scratch);
      }

      for (DN_TestScopeF(&result, "[Strings] Copy string")) {
        DN_TcScratch scratch = DN_TcScratchBeginArena(&arena_, 1);
        DN_Str8      string  = DN_Str8Lit("AB");
        DN_Str8      copy    = DN_Str8FromStr8Arena(string, &scratch.arena);
        DN_TestVerifyUSizeEqF(&result, copy.count, 2, "size: %zu", copy.count);
        DN_TestVerifyExprF(&result, copy.data[0] == 'A', "copy[0]: %c", copy.data[0]);
        DN_TestVerifyExprF(&result, copy.data[1] == 'B', "copy[1]: %c", copy.data[1]);
        DN_TestVerifyExprF(&result, copy.data[2] == 0, "copy[2]: %c", copy.data[2]);
        DN_TcScratchEnd(&scratch);
      }

      for (DN_TestScopeF(&result, "[Strings] Trim whitespace around string")) {
        DN_Str8 string = DN_Str8TrimWhitespaceAround(DN_Str8Lit(" AB "));
        DN_TestVerifyStr8EqF(&result, string, DN_Str8Lit("AB"), "[string=%.*s]", DN_Str8PrintFmt(string));
      }

      for (DN_TestScopeF(&result, "[Strings] Allocate string from arena")) {
        DN_TcScratch scratch = DN_TcScratchBeginArena(&arena_, 1);
        DN_Str8      string  = DN_Str8AllocArena(2, DN_ZMem_No, &scratch.arena);
        DN_TestVerifyUSizeEqF(&result, string.count, 2, "size: %zu", string.count);
        DN_TcScratchEnd(&scratch);
      }

      for (DN_TestScopeF(&result, "[Strings] Trim prefix with matching prefix")) {
        DN_Str8 input      = DN_Str8Lit("nft/abc");
        DN_Str8 str_result = DN_Str8TrimPrefixSensitive(input, DN_Str8Lit("nft/"));
        DN_TestVerifyStr8EqF(&result, str_result, DN_Str8Lit("abc"), "%.*s", DN_Str8PrintFmt(str_result));
      }

      for (DN_TestScopeF(&result, "[Strings] Trim prefix with non matching prefix")) {
        DN_Str8 input      = DN_Str8Lit("nft/abc");
        DN_Str8 str_result = DN_Str8TrimPrefixSensitive(input, DN_Str8Lit(" ft/"));
        DN_TestVerifyStr8EqF(&result, str_result, input, "%.*s", DN_Str8PrintFmt(str_result));
      }

      for (DN_TestScopeF(&result, "[Strings] Trim suffix with matching suffix")) {
        DN_Str8 input      = DN_Str8Lit("nft/abc");
        DN_Str8 str_result = DN_Str8TrimSuffixSensitive(input, DN_Str8Lit("abc"));
        DN_TestVerifyStr8EqF(&result, str_result, DN_Str8Lit("nft/"), "%.*s", DN_Str8PrintFmt(str_result));
      }

      for (DN_TestScopeF(&result, "[Strings] Trim suffix with non matching suffix")) {
        DN_Str8 input      = DN_Str8Lit("nft/abc");
        DN_Str8 str_result = DN_Str8TrimSuffixSensitive(input, DN_Str8Lit("ab"));
        DN_TestVerifyStr8EqF(&result, str_result, input, "%.*s", DN_Str8PrintFmt(str_result));
      }

      for (DN_TestScopeF(&result, "[Strings] Is all digits fails on non-digit string")) {
        DN_B32 str_result = DN_Str8Is(DN_Str8Lit("@123string"), DN_Str8IsFlags_Digits);
        DN_TestVerifyExpr(&result, str_result == false);
      }

      for (DN_TestScopeF(&result, "[Strings] Is all digits fails on nullptr")) {
        DN_B32 str_result = DN_Str8Is(DN_Str8FromPtr(nullptr, 0), DN_Str8IsFlags_Digits);
        DN_TestVerifyExpr(&result, str_result == false);
      }

      for (DN_TestScopeF(&result, "[Strings] Is all digits fails on string w/ 0 size")) {
        char const buf[]      = "@123string";
        DN_B32     str_result = DN_Str8Is(DN_Str8FromPtr(buf, 0), DN_Str8IsFlags_Digits);
        DN_TestVerifyExpr(&result, !str_result);
      }

      for (DN_TestScopeF(&result, "[Strings] Is all digits success")) {
        DN_B32 str_result = DN_Str8Is(DN_Str8Lit("23"), DN_Str8IsFlags_Digits);
        DN_TestVerifyExpr(&result, DN_Cast(bool) str_result == true);
      }

      for (DN_TestScopeF(&result, "[Strings] Is all digits fails on whitespace")) {
        DN_B32 str_result = DN_Str8Is(DN_Str8Lit("23 "), DN_Str8IsFlags_Digits);
        DN_TestVerifyExpr(&result, DN_Cast(bool) str_result == false);
      }

      // NOTE: DN_Str8BSplit
      {
        DN_Str8 delimiter = DN_Str8Lit("/");
        DN_Str8 input     = DN_Str8Lit("abcdef");
        for (DN_TestScopeF(&result, "[Strings] Binary split \"%.*s\" with \"%.*s\"", DN_Str8PrintFmt(input), DN_Str8PrintFmt(delimiter))) {
          DN_Str8BSplitResult split = DN_Str8BSplit(input, delimiter);
          DN_TestVerifyStr8EqF(&result, split.lhs, DN_Str8Lit("abcdef"), "[lhs=%.*s]", DN_Str8PrintFmt(split.lhs));
          DN_TestVerifyStr8EqF(&result, split.rhs, DN_Str8Lit(""), "[rhs=%.*s]", DN_Str8PrintFmt(split.rhs));
        }

        input = DN_Str8Lit("abc/def");
        for (DN_TestScopeF(&result, "[Strings] Binary split \"%.*s\" with \"%.*s\"", DN_Str8PrintFmt(input), DN_Str8PrintFmt(delimiter))) {
          DN_Str8BSplitResult split = DN_Str8BSplit(input, delimiter);
          DN_TestVerifyStr8EqF(&result, split.lhs, DN_Str8Lit("abc"), "[lhs=%.*s]", DN_Str8PrintFmt(split.lhs));
          DN_TestVerifyStr8EqF(&result, split.rhs, DN_Str8Lit("def"), "[rhs=%.*s]", DN_Str8PrintFmt(split.rhs));
        }

        input = DN_Str8Lit("/abcdef");
        for (DN_TestScopeF(&result, "[Strings] Binary split \"%.*s\" with \"%.*s\"", DN_Str8PrintFmt(input), DN_Str8PrintFmt(delimiter))) {
          DN_Str8BSplitResult split = DN_Str8BSplit(input, delimiter);
          DN_TestVerifyStr8EqF(&result, split.lhs, DN_Str8Lit(""), "[lhs=%.*s]", DN_Str8PrintFmt(split.lhs));
          DN_TestVerifyStr8EqF(&result, split.rhs, DN_Str8Lit("abcdef"), "[rhs=%.*s]", DN_Str8PrintFmt(split.rhs));
        }

        delimiter = DN_Str8Lit("-=-");
        input     = DN_Str8Lit("123-=-456");
        for (DN_TestScopeF(&result, "[Strings] Binary split \"%.*s\" with \"%.*s\"", DN_Str8PrintFmt(input), DN_Str8PrintFmt(delimiter))) {
          DN_Str8BSplitResult split = DN_Str8BSplit(input, delimiter);
          DN_TestVerifyStr8EqF(&result, split.lhs, DN_Str8Lit("123"), "[lhs=%.*s]", DN_Str8PrintFmt(split.lhs));
          DN_TestVerifyStr8EqF(&result, split.rhs, DN_Str8Lit("456"), "[rhs=%.*s]", DN_Str8PrintFmt(split.rhs));
        }
      }

      // NOTE: DN_I64FromStr8
      for (DN_TestScopeF(&result, "[Strings] To I64: Convert empty string")) {
        DN_I64FromResult str_result = DN_I64FromStr8(DN_Str8Lit(""));
        DN_TestVerifyExpr(&result, str_result.success);
        DN_TestVerifyExpr(&result, str_result.value == 0);
      }

      for (DN_TestScopeF(&result, "[Strings] To I64: Convert \"1\"")) {
        DN_I64FromResult str_result = DN_I64FromStr8(DN_Str8Lit("1"));
        DN_TestVerifyExpr(&result, str_result.success);
        DN_TestVerifyExpr(&result, str_result.value == 1);
      }

      for (DN_TestScopeF(&result, "[Strings] To I64: Convert \"-0\"")) {
        DN_I64FromResult str_result = DN_I64FromStr8(DN_Str8Lit("-0"));
        DN_TestVerifyExpr(&result, str_result.success);
        DN_TestVerifyExpr(&result, str_result.value == 0);
      }

      for (DN_TestScopeF(&result, "[Strings] To I64: Convert \"-1\"")) {
        DN_I64FromResult str_result = DN_I64FromStr8(DN_Str8Lit("-1"));
        DN_TestVerifyExpr(&result, str_result.success);
        DN_TestVerifyExpr(&result, str_result.value == -1);
      }

      for (DN_TestScopeF(&result, "[Strings] To I64: Convert \"1.2\"")) {
        DN_I64FromResult str_result = DN_I64FromStr8(DN_Str8Lit("1.2"));
        DN_TestVerifyExpr(&result, !str_result.success);
        DN_TestVerifyExpr(&result, str_result.value == 1);
      }

      for (DN_TestScopeF(&result, "[Strings] To I64: Convert \"1,234\"")) {
        DN_I64FromResult str_result = DN_I64FromStr8Delimiter(DN_Str8Lit("1,234"), DN_Str8Lit(","));
        DN_TestVerifyExpr(&result, str_result.success);
        DN_TestVerifyExpr(&result, str_result.value == 1234);
      }

      for (DN_TestScopeF(&result, "[Strings] To I64: Convert \"1,2\"")) {
        DN_I64FromResult str_result = DN_I64FromStr8Delimiter(DN_Str8Lit("1,2"), DN_Str8Lit(","));
        DN_TestVerifyExpr(&result, str_result.success);
        DN_TestVerifyExpr(&result, str_result.value == 12);
      }

      for (DN_TestScopeF(&result, "[Strings] To I64: Convert \"12a3\"")) {
        DN_I64FromResult str_result = DN_I64FromStr8(DN_Str8Lit("12a3"));
        DN_TestVerifyExpr(&result, !str_result.success);
        DN_TestVerifyExpr(&result, str_result.value == 12);
      }

      // NOTE: DN_U64FromStr8
      for (DN_TestScopeF(&result, "[Strings] To U64: Convert empty string")) {
        DN_U64FromResult str_result = DN_U64FromStr8(DN_Str8Lit(""));
        DN_TestVerifyExpr(&result, str_result.success);
        DN_TestVerifyExprF(&result, str_result.value == 0, "result: %" PRIu64, str_result.value);
      }

      for (DN_TestScopeF(&result, "[Strings] To U64: Convert \"1\"")) {
        DN_U64FromResult str_result = DN_U64FromStr8(DN_Str8Lit("1"));
        DN_TestVerifyExpr(&result, str_result.success);
        DN_TestVerifyExprF(&result, str_result.value == 1, "result: %" PRIu64, str_result.value);
      }

      for (DN_TestScopeF(&result, "[Strings] To U64: Convert \"-0\"")) {
        DN_U64FromResult str_result = DN_U64FromStr8(DN_Str8Lit("-0"));
        DN_TestVerifyExpr(&result, !str_result.success);
        DN_TestVerifyExprF(&result, str_result.value == 0, "result: %" PRIu64, str_result.value);
      }

      for (DN_TestScopeF(&result, "[Strings] To U64: Convert \"-1\"")) {
        DN_U64FromResult str_result = DN_U64FromStr8(DN_Str8Lit("-1"));
        DN_TestVerifyExpr(&result, !str_result.success);
        DN_TestVerifyExprF(&result, str_result.value == 0, "result: %" PRIu64, str_result.value);
      }

      for (DN_TestScopeF(&result, "[Strings] To U64: Convert \"1.2\"")) {
        DN_U64FromResult str_result = DN_U64FromStr8(DN_Str8Lit("1.2"));
        DN_TestVerifyExpr(&result, !str_result.success);
        DN_TestVerifyExprF(&result, str_result.value == 1, "result: %" PRIu64, str_result.value);
      }

      for (DN_TestScopeF(&result, "[Strings] To U64: Convert \"1,234\"")) {
        DN_U64FromResult str_result = DN_U64FromStr8Delimiter(DN_Str8Lit("1,234"), DN_Str8Lit(","));
        DN_TestVerifyExpr(&result, str_result.success);
        DN_TestVerifyExprF(&result, str_result.value == 1234, "result: %" PRIu64, str_result.value);
      }

      for (DN_TestScopeF(&result, "[Strings] To U64: Convert \"1,2\"")) {
        DN_U64FromResult str_result = DN_U64FromStr8Delimiter(DN_Str8Lit("1,2"), DN_Str8Lit(","));
        DN_TestVerifyExpr(&result, str_result.success);
        DN_TestVerifyExprF(&result, str_result.value == 12, "result: %" PRIu64, str_result.value);
      }

      for (DN_TestScopeF(&result, "[Strings] To U64: Convert \"12a3\"")) {
        DN_U64FromResult str_result = DN_U64FromStr8(DN_Str8Lit("12a3"));
        DN_TestVerifyExpr(&result, !str_result.success);
        DN_TestVerifyExprF(&result, str_result.value == 12, "result: %" PRIu64, str_result.value);
      }

      // NOTE: DN_Str8Find
      for (DN_TestScopeF(&result, "[Strings] Find char is not in buffer")) {
        DN_Str8           buf        = DN_Str8Lit("836a35becd4e74b66a0d6844d51f1a63018c7ebc44cf7e109e8e4bba57eefb55");
        DN_Str8           find       = DN_Str8Lit("2");
        DN_Str8FindResult str_result = DN_Str8FindStr8(buf, find, DN_Str8EqCase_Sensitive);
        DN_TestVerifyExpr(&result, !str_result.found);
        DN_TestVerifyUSizeEq(&result, str_result.index, 0);
        DN_TestVerifyExpr(&result, str_result.match.data == nullptr);
        DN_TestVerifyUSizeEq(&result, str_result.match.count, 0);
      }

      for (DN_TestScopeF(&result, "[Strings] Find char is in buffer")) {
        DN_Str8           buf        = DN_Str8Lit("836a35becd4e74b66a0d6844d51f1a63018c7ebc44cf7e109e8e4bba57eefb55");
        DN_Str8           find       = DN_Str8Lit("6");
        DN_Str8FindResult str_result = DN_Str8FindStr8(buf, find, DN_Str8EqCase_Sensitive);
        DN_TestVerifyExpr(&result, str_result.found);
        DN_TestVerifyUSizeEq(&result, str_result.index, 2);
        DN_TestVerifyExpr(&result, str_result.match.data[0] == '6');
      }

      // NOTE: DN_Str8FileNameFromPath
      for (DN_TestScopeF(&result, "[Strings] File name from Windows path")) {
        DN_Str8 buf        = DN_Str8Lit("C:\\ABC\\str_result.exe");
        DN_Str8 str_result = DN_Str8FileNameFromPath(buf);
        DN_TestVerifyStr8EqF(&result, str_result, DN_Str8Lit("str_result.exe"), "%.*s", DN_Str8PrintFmt(str_result));
      }

      for (DN_TestScopeF(&result, "[Strings] File name from Linux path")) {
        DN_Str8 buf        = DN_Str8Lit("/ABC/str_result.exe");
        DN_Str8 str_result = DN_Str8FileNameFromPath(buf);
        DN_TestVerifyStr8EqF(&result, str_result, DN_Str8Lit("str_result.exe"), "%.*s", DN_Str8PrintFmt(str_result));
      }

      for (DN_TestScopeF(&result, "[Strings] Trim prefix")) {
        DN_Str8 prefix     = DN_Str8Lit("@123");
        DN_Str8 buf        = DN_Str8Lit("@123string");
        DN_Str8 str_result = DN_Str8TrimPrefixSensitive(buf, prefix);
        DN_TestVerifyStr8Eq(&result, str_result, DN_Str8Lit("string"));
      }

      // NOTE: DN_Str8TruncMiddle
      for (DN_TestScopeF(&result, "[Strings] TruncMiddlePtr: Short string is not truncated")) {
        DN_Str8            str      = DN_Str8Lit("Hello");
        DN_Str8            trunc    = DN_Str8Lit("...");
        char               dest[64] = {};
        DN_Str8TruncResult res      = DN_Str8TruncMiddlePtr(str, 5, trunc, dest, sizeof(dest));
        DN_TestVerifyExpr(&result, !res.truncated);
        DN_TestVerifyUSizeEq(&result, res.count_req, 5);
        DN_TestVerifyStr8EqF(&result, res.str8, DN_Str8Lit("Hello"), "%.*s", DN_Str8PrintFmt(res.str8));
      }

      for (DN_TestScopeF(&result, "[Strings] TruncMiddlePtr: Exact boundary is not truncated")) {
        DN_Str8            str      = DN_Str8Lit("HelloWorld");
        DN_Str8            trunc    = DN_Str8Lit("...");
        char               dest[64] = {};
        DN_Str8TruncResult res      = DN_Str8TruncMiddlePtr(str, 5, trunc, dest, sizeof(dest));
        DN_TestVerifyExpr(&result, !res.truncated);
        DN_TestVerifyUSizeEq(&result, res.count_req, 10);
        DN_TestVerifyStr8EqF(&result, res.str8, DN_Str8Lit("HelloWorld"), "%.*s", DN_Str8PrintFmt(res.str8));
      }

      for (DN_TestScopeF(&result, "[Strings] TruncMiddlePtr: Long string is truncated in the middle")) {
        DN_Str8            str      = DN_Str8Lit("HelloBeautifulWorld");
        DN_Str8            trunc    = DN_Str8Lit("...");
        char               dest[64] = {};
        DN_Str8TruncResult res      = DN_Str8TruncMiddlePtr(str, 5, trunc, dest, sizeof(dest));
        DN_TestVerifyExpr(&result, res.truncated);
        DN_TestVerifyUSizeEq(&result, res.count_req, 13);
        DN_TestVerifyStr8EqF(&result, res.str8, DN_Str8Lit("Hello...World"), "%.*s", DN_Str8PrintFmt(res.str8));
      }

      for (DN_TestScopeF(&result, "[Strings] TruncMiddlePtr: Empty truncator concatenates head and tail")) {
        DN_Str8            str      = DN_Str8Lit("HelloBeautifulWorld");
        DN_Str8            trunc    = DN_Str8Lit("");
        char               dest[64] = {};
        DN_Str8TruncResult res      = DN_Str8TruncMiddlePtr(str, 5, trunc, dest, sizeof(dest));
        DN_TestVerifyExpr(&result, res.truncated);
        DN_TestVerifyUSizeEq(&result, res.count_req, 10);
        DN_TestVerifyStr8EqF(&result, res.str8, DN_Str8Lit("HelloWorld"), "%.*s", DN_Str8PrintFmt(res.str8));
      }

      for (DN_TestScopeF(&result, "[Strings] TruncMiddlePtr: side_size of 0 returns just truncator")) {
        DN_Str8            str      = DN_Str8Lit("HelloWorld");
        DN_Str8            trunc    = DN_Str8Lit("...");
        char               dest[64] = {};
        DN_Str8TruncResult res      = DN_Str8TruncMiddlePtr(str, 0, trunc, dest, sizeof(dest));
        DN_TestVerifyExpr(&result, res.truncated);
        DN_TestVerifyUSizeEq(&result, res.count_req, 3);
        DN_TestVerifyStr8EqF(&result, res.str8, DN_Str8Lit("..."), "%.*s", DN_Str8PrintFmt(res.str8));
      }

      for (DN_TestScopeF(&result, "[Strings] TruncMiddlePtr: Null dest calculates size without writing")) {
        DN_Str8            str   = DN_Str8Lit("HelloBeautifulWorld");
        DN_Str8            trunc = DN_Str8Lit("...");
        DN_Str8TruncResult res   = DN_Str8TruncMiddlePtr(str, 5, trunc, nullptr, 0);
        DN_TestVerifyExpr(&result, res.truncated);
        DN_TestVerifyUSizeEq(&result, res.count_req, 13);
        DN_TestVerifyExpr(&result, res.str8.data == nullptr);
      }

      for (DN_TestScopeF(&result, "[Strings] TruncMiddlePtr: count_req is consistent between dry-run and actual")) {
        DN_Str8            str      = DN_Str8Lit("HelloBeautifulWorld");
        DN_Str8            trunc    = DN_Str8Lit("...");
        DN_Str8TruncResult dry      = DN_Str8TruncMiddlePtr(str, 5, trunc, nullptr, 0);
        char               dest[64] = {};
        DN_Str8TruncResult actual   = DN_Str8TruncMiddlePtr(str, 5, trunc, dest, sizeof(dest));
        DN_TestVerifyUSizeEq(&result, dry.count_req, actual.count_req);
        DN_TestVerifyExpr(&result, dry.truncated == actual.truncated);
      }

      for (DN_TestScopeF(&result, "[Strings] TruncMiddlePtr: Minimum buffer size is sufficient")) {
        DN_Str8            str      = DN_Str8Lit("HelloBeautifulWorld");
        DN_Str8            trunc    = DN_Str8Lit("...");
        char               dest[14] = {};
        DN_Str8TruncResult res      = DN_Str8TruncMiddlePtr(str, 5, trunc, dest, sizeof(dest));
        DN_TestVerifyExpr(&result, res.truncated);
        DN_TestVerifyUSizeEq(&result, res.count_req, 13);
        DN_TestVerifyStr8EqF(&result, res.str8, DN_Str8Lit("Hello...World"), "%.*s", DN_Str8PrintFmt(res.str8));
      }

      for (DN_TestScopeF(&result, "[Strings] TruncMiddlePtr: Single character side size")) {
        DN_Str8            str      = DN_Str8Lit("HelloBeautifulWorld");
        DN_Str8            trunc    = DN_Str8Lit("...");
        char               dest[64] = {};
        DN_Str8TruncResult res      = DN_Str8TruncMiddlePtr(str, 1, trunc, dest, sizeof(dest));
        DN_TestVerifyExpr(&result, res.truncated);
        DN_TestVerifyUSizeEq(&result, res.count_req, 5);
        DN_TestVerifyStr8EqF(&result, res.str8, DN_Str8Lit("H...d"), "%.*s", DN_Str8PrintFmt(res.str8));
      }

      for (DN_TestScopeF(&result, "[Strings] TruncMiddlePtr: Large side_size falls back to copy")) {
        DN_Str8            str      = DN_Str8Lit("Hello");
        DN_Str8            trunc    = DN_Str8Lit("...");
        char               dest[64] = {};
        DN_Str8TruncResult res      = DN_Str8TruncMiddlePtr(str, 100, trunc, dest, sizeof(dest));
        DN_TestVerifyExpr(&result, !res.truncated);
        DN_TestVerifyUSizeEq(&result, res.count_req, 5);
        DN_TestVerifyStr8EqF(&result, res.str8, DN_Str8Lit("Hello"), "%.*s", DN_Str8PrintFmt(res.str8));
      }

      for (DN_TestScopeF(&result, "[Strings] TruncMiddle: Arena wrapper allocates and truncates correctly")) {
        DN_TcScratch scratch = DN_TcScratchBeginArena(&arena_, 1);
        DN_Str8            str     = DN_Str8Lit("HelloBeautifulWorld");
        DN_Str8            trunc   = DN_Str8Lit("...");
        DN_Str8TruncResult res     = DN_Str8TruncMiddle(str, 5, trunc, &scratch.arena);
        DN_TestVerifyExpr(&result, res.truncated);
        DN_TestVerifyUSizeEq(&result, res.count_req, 13);
        DN_TestVerifyStr8EqF(&result, res.str8, DN_Str8Lit("Hello...World"), "%.*s", DN_Str8PrintFmt(res.str8));
        DN_TestVerifyExpr(&result, res.str8.data[res.str8.count] == '\0');
        DN_TcScratchEnd(&scratch);
      }
    }

    // NOTE: Win
    #if defined(DN_PLATFORM_WIN32)
    {
      DN_TcScratch scratch = DN_TcScratchBeginArena(&arena_, 1);
      DN_Str8      input8  = DN_Str8Lit("String");
      DN_Str16     input16 = DN_Str16{(wchar_t *)(L"String"), sizeof(L"String") / sizeof(L"String"[0]) - 1};

      for (DN_TestScopeF(&result, "[Win] Str8 to Str16")) {
        DN_Str16 str_result = DN_OS_W32Str8ToStr16(&scratch.arena, input8);
        DN_TestVerifyExpr(&result, DN_Str16Eq(str_result, input16));
      }

      for (DN_TestScopeF(&result, "[Win] Str16 to Str8")) {
        DN_Str8 str_result = DN_OS_W32Str16ToStr8(&scratch.arena, input16);
        DN_TestVerifyStr8Eq(&result, str_result, input8);
      }

      for (DN_TestScopeF(&result, "[Win] Str16 to Str8: Null terminates string")) {
        int   size_required = DN_OS_W32Str16ToStr8Buffer(input16, nullptr, 0);
        char *string        = DN_ArenaNewArray(&scratch.arena, char, size_required + 1, DN_ZMem_No);

        DN_Memset(string, 'Z', size_required + 1);

        int        size_returned = DN_OS_W32Str16ToStr8Buffer(input16, string, size_required + 1);
        char const EXPECTED[]    = {'S', 't', 'r', 'i', 'n', 'g', 0};

        DN_TestVerifyUSizeEqF(&result, size_required, size_returned, "string_size: %d, result: %d", size_required, size_returned);
        DN_TestVerifyUSizeEqF(&result, size_returned, DN_ArrayCountU(EXPECTED) - 1, "string_size: %d, expected: %zu", size_returned, DN_ArrayCountU(EXPECTED) - 1);
        DN_TestVerifyExpr(&result, DN_Memcmp(EXPECTED, string, sizeof(EXPECTED)) == 0);
      }

      for (DN_TestScopeF(&result, "[Win] Str16 to Str8: Arena null terminates string")) {
        DN_Str8    string8       = DN_OS_W32Str16ToStr8(&scratch.arena, input16);
        int        size_returned = DN_OS_W32Str16ToStr8Buffer(input16, nullptr, 0);
        char const EXPECTED[]    = {'S', 't', 'r', 'i', 'n', 'g', 0};

        DN_TestVerifyUSizeEqF(&result, DN_Cast(int) string8.count, size_returned, "string_size: %d, result: %d", DN_Cast(int) string8.count, size_returned);
        DN_TestVerifyUSizeEqF(&result, DN_Cast(int) string8.count, DN_ArrayCountU(EXPECTED) - 1, "string_size: %d, expected: %zu", DN_Cast(int) string8.count, DN_ArrayCountU(EXPECTED) - 1);
        DN_TestVerifyExpr(&result, DN_Memcmp(EXPECTED, string8.data, sizeof(EXPECTED)) == 0);
      }

      DN_TcScratchEnd(&scratch);
    }
    #endif // DN_PLATFORM_WIN32

    // NOTE: NET
    #if DN_WITH_NET
    {
      struct NETEntry
      {
        DN_Str8         label;
        DN_NETInterface net_interface;
      };

      NETEntry entries[2]    = {};
      DN_USize entries_count = 0;

      {
        #if defined(DN_PLATFORM_EMSCRIPTEN)
        NETEntry *entry      = entries + entries_count++;
        entry->label         = DN_Str8Lit("[NET] Emscripten");
        entry->net_interface = DN_NET_EmcInterface();
        #endif
      }

      {
        #if DN_WITH_NET_CURL
        NETEntry *entry      = entries + entries_count++;
        entry->label         = DN_Str8Lit("[NET] CURL");
        entry->net_interface = DN_NET_CurlInterface();
        #endif
      }

      for (DN_ForItSize(entry_it, NETEntry, entries, entries_count)) {
        NETEntry *entry                  = entry_it.data;
        DN_Arena  arena                  = DN_ArenaFromHeap(DN_Megabytes(1), DN_Kilobytes(64), DN_MemFlags_Nil, DN_OS_HeapInitVirtual());
        DN_Str8   remote_ws_server_url   = DN_Str8Lit("wss://echo.websocket.org");
        DN_Str8   remote_http_server_url = DN_Str8Lit("https://google.com");

        DN_USize         net_base_size = DN_Megabytes(1);
        char            *net_base      = DN_ArenaNewArray(&arena, char, net_base_size, DN_ZMem_Yes);
        DN_NETCore       net           = {};
        DN_NETInterface *net_interface = &entry->net_interface;
        net_interface->init(&net, net_base, net_base_size);

        DN_U64 arena_reset_p = DN_MemListPos(arena.mem);
        for (DN_TestScopeF(&result, "%.*s: WaitForResponse HTTP GET request", DN_Str8PrintFmt(entry->label))) {
          DN_NETRequestHandle request  = net_interface->do_http(&net, remote_http_server_url, DN_Str8Lit("GET"), nullptr);
          DN_NETResponse      response = net_interface->wait_for_response(request, &arena, UINT32_MAX);
          DN_TestVerifyUSizeNotEq      (&result, response.http_status,      0);
          DN_TestVerifyUSizeEq         (&result, response.state,            DN_NETResponseState_HTTP);
          DN_TestVerifyUSizeEq         (&result, response.error_str8.count, 0);
          DN_TestVerifyUSizeGreaterThan(&result, response.body.count,       0);
        }

        for (DN_TestScopeF(&result, "%.*s: WaitForResponse HTTP POST request", DN_Str8PrintFmt(entry->label))) {
          net_interface->do_http(&net, remote_http_server_url, DN_Str8Lit("POST"), nullptr);
          DN_NETResponse response = net_interface->wait_for_any_response(&net, &arena, UINT32_MAX);
          DN_TestVerifyUSizeNotEq      (&result, response.http_status,      0);
          DN_TestVerifyUSizeEq         (&result, response.state,            DN_NETResponseState_HTTP);
          DN_TestVerifyUSizeEq         (&result, response.error_str8.count, 0);
          DN_TestVerifyUSizeGreaterThan(&result, response.body.count,       0);
        }

        for (DN_TestScopeF(&result, "%.*s: WaitForResponse WS request", DN_Str8PrintFmt(entry->label))) {
          DN_NETRequestHandle request       = net_interface->do_ws(&net, remote_ws_server_url);
          DN_USize const      WS_TIMEOUT_MS = 16;

          // NOTE: Wait for WS connection to open
          for (bool done = false; !done; DN_MemListPopTo(arena.mem, arena_reset_p)) {
            DN_NETResponse response = net_interface->wait_for_response(request, &arena, WS_TIMEOUT_MS);
            if (response.state == DN_NETResponseState_Nil) // NOTE: Timeout
              continue;
            DN_TestVerifyUSizeEqF(&result, response.state, DN_NETResponseState_WSOpen, "ERROR: %.*s", DN_Str8PrintFmt(response.error_str8));
            done = true;
          }

          // NOTE: Receive the initial text from the echo server
          for (bool done = false; !done; DN_MemListPopTo(arena.mem, arena_reset_p)) {
            DN_NETResponse response = net_interface->wait_for_response(request, &arena, WS_TIMEOUT_MS);
            if (response.state == DN_NETResponseState_Nil) // NOTE: Timeout
              continue;
            DN_TestVerifyUSizeEqF(&result, response.state, DN_NETResponseState_WSText, "ERROR: %.*s", DN_Str8PrintFmt(response.error_str8));
            net_interface->do_ws_send(request, DN_Str8Lit(""), DN_NETWSSend_Close);
            done = true;
          }

          // NOTE: Expect to hear the close
          for (bool done = false; !done; DN_MemListPopTo(arena.mem, arena_reset_p)) {
            DN_NETResponse response = net_interface->wait_for_response(request, &arena, WS_TIMEOUT_MS);
            if (response.state == DN_NETResponseState_Nil) // NOTE: Timeout
              continue;
            if (response.state == DN_NETResponseState_Error)
            DN_TestVerifyUSizeEqF(&result, response.state, DN_NETResponseState_WSClose, "ERROR: %.*s", DN_Str8PrintFmt(response.error_str8));
            done = true;
          }
        }
        net_interface->deinit(&net);
        DN_MemListDeinit(arena.mem);
      }
    }
    #endif // #if DN_WITH_NET
  }
  return result;
}
DN_MSVC_WARNING_POP
#endif // #if DN_WITH_TESTS

DN_API void DN_LeakTrackAlloc_(DN_LeakTracker *leak, void *ptr, DN_USize size, bool leak_permitted)
{
  if (!ptr)
    return;

  DN_TicketMutexBegin(&leak->alloc_table_mutex);

  DN_Str8            stack_trace = DN_Str8FromStackTraceNowHeap(128, 3 /*skip*/);
  DN_HTable*         alloc_table = &leak->alloc_table;
  DN_UPtr            uptr        = DN_Cast(DN_UPtr)ptr;
  DN_HTableAddResult alloc_entry = DN_HTableMake(alloc_table, &uptr);
  DN_LeakAlloc*      alloc       = DN_Cast(DN_LeakAlloc *)alloc_entry.slot.value;
  if (alloc_entry.existed) {
    if ((alloc->flags & DN_LeakAllocFlag_Freed) == 0) {
      DN_Str8x32 alloc_size     = DN_Str8x32FromByteCountU64Auto(alloc->size);
      DN_Str8x32 new_alloc_size = DN_Str8x32FromByteCountU64Auto(size);
      DN_AssertAlwaysF(
          alloc->flags & DN_LeakAllocFlag_Freed,
          "This pointer is already in the leak tracker, however it has not been freed yet. This "
          "same pointer is being ask to be tracked twice in the allocation table, e.g. one if its "
          "previous free calls has not being marked freed with an equivalent call to "
          "DN_LeakTrackDealloc()\n"
          "\n"
          "The pointer (0x%p) originally allocated %.*s at:\n"
          "\n"
          "%.*s\n"
          "\n"
          "The pointer is allocating %.*s again at:\n"
          "\n"
          "%.*s\n",
          ptr,
          DN_Str8PrintFmt(alloc_size),
          DN_Str8PrintFmt(alloc->stack_trace),
          DN_Str8PrintFmt(new_alloc_size),
          DN_Str8PrintFmt(stack_trace));
    }

    // NOTE: Pointer was reused, clean up the prior entry
    leak->alloc_table_bytes_allocated_for_stack_traces -= alloc->stack_trace.count;
    leak->alloc_table_bytes_allocated_for_stack_traces -= alloc->freed_stack_trace.count;

    DN_OS_MemDealloc(alloc->stack_trace.data);
    DN_OS_MemDealloc(alloc->freed_stack_trace.data);
    *alloc = {};
  }

  alloc->ptr         = ptr;
  alloc->size        = size;
  alloc->stack_trace = stack_trace;
  alloc->flags |= leak_permitted ? DN_LeakAllocFlag_LeakPermitted : 0;
  leak->alloc_table_bytes_allocated_for_stack_traces += alloc->stack_trace.count;
  DN_TicketMutexEnd(&leak->alloc_table_mutex);
}

DN_API void DN_LeakTrackDealloc_(DN_LeakTracker *leak, void *ptr)
{
  if (!ptr)
    return;

  DN_TicketMutexBegin(&leak->alloc_table_mutex);

  DN_Str8       stack_trace = DN_Str8FromStackTraceNowHeap(128, 3 /*skip*/);
  DN_HTable*    alloc_table = &leak->alloc_table;
  DN_UPtr       uptr        = DN_Cast(DN_UPtr)ptr;
  DN_LeakAlloc* alloc       = DN_Cast(DN_LeakAlloc *)DN_HTableValueFromFind(alloc_table, &uptr);
  DN_AssertAlwaysF(alloc,
                   "Allocated pointer can not be removed as it does not exist in the "
                   "allocation table. When this memory was allocated, the pointer was "
                   "not added to the allocation table [ptr=%p]",
                   ptr);

  if (alloc->flags & DN_LeakAllocFlag_Freed) {
    DN_Str8x32 freed_size = DN_Str8x32FromByteCountU64Auto(alloc->freed_size);
    DN_AssertAlwaysF((alloc->flags & DN_LeakAllocFlag_Freed) == 0,
                   "Double free detected, pointer to free was already marked "
                   "as freed. Either the pointer was reallocated but not "
                   "traced, or, the pointer was freed twice.\n"
                   "\n"
                   "The pointer (0x%p) originally allocated %.*s at:\n"
                   "\n"
                   "%.*s\n"
                   "\n"
                   "The pointer was freed at:\n"
                   "\n"
                   "%.*s\n"
                   "\n"
                   "The pointer is being freed again at:\n"
                   "\n"
                   "%.*s\n",
                   ptr,
                   DN_Str8PrintFmt(freed_size),
                   DN_Str8PrintFmt(alloc->stack_trace),
                   DN_Str8PrintFmt(alloc->freed_stack_trace),
                   DN_Str8PrintFmt(stack_trace));
  }

  DN_Assert(alloc->freed_stack_trace.count == 0);
  alloc->flags |= DN_LeakAllocFlag_Freed;
  alloc->freed_stack_trace = stack_trace;
  leak->alloc_table_bytes_allocated_for_stack_traces += alloc->freed_stack_trace.count;
  DN_TicketMutexEnd(&leak->alloc_table_mutex);
}

DN_API void DN_LeakDump_(DN_LeakTracker *leak)
{
  DN_U64 leak_count   = 0;
  DN_U64 leaked_bytes = 0;
  for (DN_ForItSize(it, DN_LeakTrackerKV, leak->alloc_table_kvs, leak->alloc_table.max)) {
    DN_LeakTrackerKV *kv = it.data;
    if (!DN_HTableHashIsValue(kv->hash))
      continue;

    DN_LeakAlloc *alloc          = &kv->value;
    bool          alloc_leaked   = (alloc->flags & DN_LeakAllocFlag_Freed) == 0;
    bool          leak_permitted = (alloc->flags & DN_LeakAllocFlag_LeakPermitted);
    if (alloc_leaked && !leak_permitted) {
      leaked_bytes += alloc->size;
      leak_count++;
      DN_Str8x32 alloc_size = DN_Str8x32FromByteCountU64Auto(alloc->size);
      DN_LogWarningF(
          "Pointer (0x%p) leaked %.*s at:\n"
          "%.*s",
          alloc->ptr,
          DN_Str8PrintFmt(alloc_size),
          DN_Str8PrintFmt(alloc->stack_trace));
    }
  }

  if (leak_count) {
    DN_Str8x32 leak_size = DN_Str8x32FromByteCountU64Auto(leaked_bytes);
    DN_LogWarningF("There were %I64u leaked allocations totalling %.*s", leak_count, DN_Str8PrintFmt(leak_size));
  }
}

#if DN_WITH_OS
#if defined(DN_PLATFORM_POSIX)
#include <sys/sysinfo.h> // get_nprocs
#include <unistd.h>      // getpagesize
#endif

DN_API DN_Str8 DN_OS_Str8FromStr8BuilderHeap(DN_Str8Builder const *builder)
{
  DN_Str8 result = DN_ZeroInit;
  if (!builder || builder->string_size <= 0 || builder->count <= 0)
    return result;

  result.data = DN_Cast(char *) DN_OS_MemAlloc(builder->string_size + 1, DN_ZMem_No);
  if (!result.data)
    return result;

  for (DN_Str8Link *link = builder->head; link; link = link->next) {
    DN_Memcpy(result.data + result.count, link->string.data, link->string.count);
    result.count += link->string.count;
  }

  result.data[result.count] = 0;
  DN_Assert(result.count == builder->string_size);
  return result;
}

DN_API void DN_OS_LogPrintFV(DN_LogTypeParam type, void *user_data, DN_CallSite call_site, DN_LogFlags flags, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_OSCore *os       = DN_Cast(DN_OSCore *)user_data;
  DN_OSLogger *logger = &os->logger;
  DN_OS_LoggerFV(logger, type, call_site, flags, fmt, args);
}

DN_API void DN_OS_LogPrintF(DN_LogTypeParam type, void *user_data, DN_CallSite call_site, DN_LogFlags flags, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_OS_LogPrintFV(type, user_data, call_site, flags, fmt, args);
  va_end(args);
}

static void DN_OS_LoggerSetFilePathNoMutex_(DN_OSLogger *logger, DN_Pool *pool, DN_Str8 file_path)
{
  if (logger->file_path.data) {
    DN_AssertF(DN_MemListOwnsPtr(pool->arena->mem, logger->file_path.data),
               "If there's a pre-existing file path set in the logger, it must be deallocated by "
               " the caller and cleared. If it has been previously allocated with the exact same "
               " pool was allocated with the same pool then we will deallocate it for you.");
    DN_PoolDealloc(pool, logger->file_path.data);
    logger->file_path = {};
  }
  DN_OS_FileClose(&logger->file);
  logger->file_path = DN_Str8FmtOsPathPool(pool, "%.*s", DN_Str8PrintFmt(file_path));

  // NOTE: Clear the sticky file error flag if it was set
  logger->flags &= ~DN_OSLoggerFlags_FileError;
}

DN_API void DN_OS_LoggerSetFilePath(DN_OSLogger *logger, DN_Pool *pool, DN_Str8 file_path)
{
  DN_TicketMutexBegin(&logger->file_mutex);
  DN_OS_LoggerSetFilePathNoMutex_(logger, pool, file_path);
  DN_TicketMutexEnd(&logger->file_mutex);
}

static void DN_OS_DoLogFileSetupAndRotation_(DN_OSLogger *logger)
{
  DN_TcScratch scratch = DN_TcScratchBeginArena(nullptr, 0);
  DN_Str8 error        = {};
  DN_TicketMutexBegin(&logger->file_mutex);
  if (logger->flags & DN_OSLoggerFlags_File) {

    // NOTE: Set a default file path to log to if it's not been set yet
    if (logger->file_path.count == 0) {
      DN_Str8      exe_dir           = DN_OS_ExeDir(&scratch.arena);
      DN_Str8      default_file_path = DN_Str8FmtOsPathArena(&scratch.arena, "%.*s/dn.log", DN_Str8PrintFmt(exe_dir));
      DN_OS_LoggerSetFilePathNoMutex_(logger, DN_TcMainPool(), default_file_path);
    }

    // NOTE Rotate the log file if the criteria is met
    if (DN_BitIsNotSet(logger->flags, DN_OSLoggerFlags_FileError) && logger->rotate_every_n_bytes && logger->rotate_count) {
      DN_Assert(logger->file_path.count);
      DN_Assert(logger->rotate_every_n_bytes > 0);

      DN_OSPathInfo file_info = DN_OS_PathInfo(logger->file_path);
      bool needs_rotate       = file_info.size >= logger->rotate_every_n_bytes;
      if (needs_rotate) {
        DN_OS_FileClose(&logger->file);
        DN_OS_FileRotate(logger->file_path, logger->rotate_count, DN_Str8Lit("."), DN_OSFileRotateFlags_Nil);
      }

      // NOTE: After rotating, check the file size of the log file we will write to again. If the
      // file size is still greater than the rotate size, then there was an error when we
      // attempted to rotate the logs. Set the sticky file flag error to disable to logger and
      // inform the user.
      if (needs_rotate) {
        DN_OSPathInfo recheck_file_info = DN_OS_PathInfo(logger->file_path);
        if (recheck_file_info.size >= logger->rotate_every_n_bytes) {
          logger->flags |= DN_OSLoggerFlags_FileError;
          error          = DN_Str8FmtArena(&scratch.arena, "Rotating of log files failed at (%.*s). Logging to disk is disabled", DN_Str8PrintFmt(logger->file_path));
        }
      }
    }

    // NOTE: Open the file requested by the logger if it hasn't been opened yet
    if (DN_BitIsNotSet(logger->flags, DN_OSLoggerFlags_FileError)) {
      if (!logger->file.handle && !logger->file.error) {
        DN_OSPathInfo file_path_info  = DN_OS_PathInfo(logger->file_path);
        if (file_path_info.exists && file_path_info.type != DN_OSPathInfoType_File) {
          logger->flags |= DN_OSLoggerFlags_FileError;
          error          = DN_Str8FmtArena(&scratch.arena, "File path to log to (%.*s) exists but is not a writable file. Logging to disk is disabled.", DN_Str8PrintFmt(logger->file_path));
        }

        if (DN_BitIsNotSet(logger->flags, DN_OSLoggerFlags_FileError))
          logger->file = DN_OS_FileOpen(logger->file_path, DN_OSFileOpen_OpenAlways, DN_OSFileAccess_AppendOnly, nullptr);
      }
    }

    // NOTE: Set the sticky error flag on the file if opening failed. The sticky flag ensures we
    // only notify the user once of a file open failure per unique base file path
    if (DN_BitIsNotSet(logger->flags, DN_OSLoggerFlags_FileError)) {
      if (logger->file.error) {
        logger->flags |= DN_OSLoggerFlags_FileError;
        error          = DN_Str8FmtArena(&scratch.arena, "Failed to open file (%.*s) for logging. Logging to disk is disabled", DN_Str8PrintFmt(logger->file_path));
      }
    }
  }
  DN_TicketMutexEnd(&logger->file_mutex);

  // NOTE: Error is logged outside of the mutex since logging will recurse back into the OS logger
  if (error.count)
    DN_LogWarningF("%.*s", DN_Str8PrintFmt(error));
  DN_TcScratchEnd(&scratch);
}

DN_API void DN_OS_LoggerFV(DN_OSLogger *logger, DN_LogTypeParam type, DN_CallSite call_site, DN_LogFlags flags, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_OS_DoLogFileSetupAndRotation_(logger);
  bool print_prefix                          = DN_BitIsNotSet(flags, DN_LogFlags_NoPrefix);
  char             prefix_buffer[128]        = {};
  DN_LogPrefixSize prefix_size               = {};
  char             prefix_colour_buffer[128] = {};
  DN_LogPrefixSize prefix_colour_size        = {};
  if (print_prefix) {
    // NOTE: Generate 2 variants of the style (colour and colour-less) for printing the prefix of
    // the log line
    DN_LogStyle style_colour = {};
    style_colour.bold        = DN_LogBold_Yes;
    style_colour.colour      = true;
    if (type.is_u32_enum) {
      switch (type.u32) {
        case DN_LogType_Debug:   style_colour.colour = false; style_colour.bold = DN_LogBold_No; break;
        case DN_LogType_Info:    style_colour.g = 0x87; style_colour.b = 0xff; break;
        case DN_LogType_Warning: style_colour.r = 0xff; style_colour.g = 0xff; break;
        case DN_LogType_Error:   style_colour.r = 0xff;                        break;
      }
    }

    DN_LogStyle style = style_colour;
    style.colour      = false;

    // NOTE: Build the log prefix
    DN_Date    os_date  = DN_OS_DateLocalTimeNow();
    DN_LogDate log_date = {};
    log_date.year       = os_date.year;
    log_date.month      = os_date.month;
    log_date.day        = os_date.day;
    log_date.hour       = os_date.hour;
    log_date.minute     = os_date.minutes;
    log_date.second     = os_date.seconds;
    prefix_size         = DN_LogMakePrefix(style, type, call_site, log_date, prefix_buffer, DN_ArrayCountU(prefix_buffer));
    prefix_colour_size  = DN_LogMakePrefix(style_colour, type, call_site, log_date, prefix_colour_buffer, DN_ArrayCountU(prefix_colour_buffer));
  }

  // NOTE: Log to disk. Note that a file handle that error-ed is a no-op in these functions so no
  // extra branching is needed to handle that.
  va_list args_copy;
  va_copy(args_copy, args);
  DN_TicketMutexBegin(&logger->file_mutex);
  {
    if (print_prefix) {
      DN_OS_FileWrite(&logger->file, DN_Str8FromPtr(prefix_buffer, prefix_size.count), nullptr);
      DN_OS_FileWriteF(&logger->file, nullptr, "%*s ", DN_Cast(int) prefix_size.padding, "");
    }
    DN_OS_FileWriteFV(&logger->file, nullptr, fmt, args_copy);
    if (!DN_BitIsSet(flags, DN_LogFlags_NoNewLine))
      DN_OS_FileWrite(&logger->file, DN_Str8Lit("\n"), nullptr);
  }
  DN_TicketMutexEnd(&logger->file_mutex);
  va_end(args_copy);

  DN_TicketMutexBegin(&logger->mutex);
  if (DN_BitIsNotSet(logger->flags, DN_OSLoggerFlags_NoOutput)) {
    if (print_prefix) {
      if (DN_BitIsSet(logger->flags, DN_OSLoggerFlags_NoColour))
        DN_OS_PrintF(DN_OSPrintDest_Err, "%.*s%*s ", DN_Cast(int) prefix_size.count, prefix_buffer, DN_Cast(int) prefix_size.padding, "");
      else
        DN_OS_PrintF(DN_OSPrintDest_Err, "%.*s%*s ", DN_Cast(int) prefix_colour_size.count, prefix_colour_buffer, DN_Cast(int) prefix_colour_size.padding, "");
    }

    if (DN_BitIsSet(flags, DN_LogFlags_NoNewLine))
      DN_OS_PrintFV(DN_OSPrintDest_Err, fmt, args);
    else
      DN_OS_PrintLnFV(DN_OSPrintDest_Err, fmt, args);
  }
  DN_TicketMutexEnd(&logger->mutex);
}

DN_API void DN_OS_LoggerF(DN_OSLogger *logger, DN_LogTypeParam type, DN_CallSite call_site, DN_LogFlags flags, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_OS_LoggerFV(logger, type, call_site, flags, fmt, args);
  va_end(args);
}

DN_API void DN_OS_SetLogPrintFuncToOS()
{
  DN_Core *dn = DN_Get();
  DN_LogSetPrintFunc(DN_OS_LogPrintFV, &dn->os);
}

DN_API void *DN_OS_HeapBasicAlloc(DN_USize size)
{
  void *result = DN_OS_MemAlloc(size, DN_ZMem_Yes);
  return result;
}

DN_API void DN_OS_HeapBasicDealloc(void *ptr)
{
  DN_OS_MemDealloc(ptr);
}

DN_API DN_Heap DN_OS_HeapInitBasic()
{
  DN_Heap result = DN_HeapInitBasic(DN_OS_HeapBasicAlloc, DN_OS_HeapBasicDealloc);
  return result;
}

DN_API DN_Heap DN_OS_HeapInitVirtual()
{
  DN_Core *dn = DN_Get();
  DN_Assert(dn->init_flags & DN_InitFlags_OS);
  DN_Assert(dn->os.page_size);
  DN_Heap result = DN_HeapInitVirtual(dn->os.page_size, DN_OS_MemReserve, DN_OS_MemCommit, DN_OS_MemRelease);
  return result;
}

DN_API DN_Heap DN_OS_HeapInitDefault()
{
  DN_Heap result = {};
  #if defined(DN_PLATFORM_EMSCRIPTEN)
    result = DN_OS_HeapInitBasic();
  #else
    result = DN_OS_HeapInitVirtual();
  #endif
  return result;
}

DN_API DN_Arena DN_OS_ArenaFromHeapVirtual(DN_U64 reserve, DN_U64 commit, DN_MemFlags flags)
{
  DN_Heap heap    = DN_OS_HeapInitVirtual();
  DN_Arena result = DN_ArenaFromHeap(reserve, commit, flags, heap);
  return result;
}

DN_API DN_Arena DN_OS_ArenaFromHeapBasic(DN_U64 size, DN_MemFlags flags)
{
  DN_Heap heap    = DN_OS_HeapInitBasic();
  DN_Arena result = DN_ArenaFromHeap(/*reserve=*/ size, /*commit=*/ size, flags, heap);
  return result;
}

// NOTE: Date
DN_API DN_Str8x32 DN_OS_DateLocalTimeStr8(DN_Date time, char date_seperator, char hms_seperator)
{
  DN_Str8x32 result = DN_Str8x32FromFmt("%hu%c%02hhu%c%02hhu %02hhu%c%02hhu%c%02hhu",
                                        time.year,
                                        date_seperator,
                                        time.month,
                                        date_seperator,
                                        time.day,
                                        time.hour,
                                        hms_seperator,
                                        time.minutes,
                                        hms_seperator,
                                        time.seconds);
  return result;
}

DN_API DN_Str8x32 DN_OS_DateLocalTimeStr8Now(char date_seperator, char hms_seperator)
{
  DN_Date    time   = DN_OS_DateLocalTimeNow();
  DN_Str8x32 result = DN_OS_DateLocalTimeStr8(time, date_seperator, hms_seperator);
  return result;
}

DN_API DN_Str8 DN_OS_ExeDir(DN_Arena *arena)
{
  DN_Str8 result = {};
  if (!arena)
    return result;
  DN_TcScratch        scratch      = DN_TcScratchBeginArena(&arena, 1);
  DN_Str8             exe_path     = DN_OS_ExePath(&scratch.arena);
  DN_Str8             seperators[] = {DN_Str8Lit("/"), DN_Str8Lit("\\")};
  DN_Str8BSplitResult split        = DN_Str8BSplitLastArray(exe_path, seperators, DN_ArrayCountU(seperators));
  result                           = DN_Str8FromStr8Arena(split.lhs, arena);
  DN_TcScratchEnd(&scratch);
  return result;
}

// NOTE: Counters
DN_API DN_F64 DN_OS_PerfCounterS(uint64_t begin, uint64_t end)
{
  uint64_t frequency = DN_OS_PerfCounterFrequency();
  uint64_t ticks     = end - begin;
  DN_F64   result    = ticks / DN_Cast(DN_F64) frequency;
  return result;
}

DN_API DN_F64 DN_OS_PerfCounterMs(uint64_t begin, uint64_t end)
{
  uint64_t frequency = DN_OS_PerfCounterFrequency();
  uint64_t ticks     = end - begin;
  DN_F64   result    = (ticks * 1'000) / DN_Cast(DN_F64) frequency;
  return result;
}

DN_API DN_F64 DN_OS_PerfCounterUs(uint64_t begin, uint64_t end)
{
  uint64_t frequency = DN_OS_PerfCounterFrequency();
  uint64_t ticks     = end - begin;
  DN_F64   result    = (ticks * 1'000'000) / DN_Cast(DN_F64) frequency;
  return result;
}

DN_API DN_F64 DN_OS_PerfCounterNs(uint64_t begin, uint64_t end)
{
  uint64_t frequency = DN_OS_PerfCounterFrequency();
  uint64_t ticks     = end - begin;
  DN_F64   result    = (ticks * 1'000'000'000) / DN_Cast(DN_F64) frequency;
  return result;
}

DN_API DN_OSTimer DN_OS_TimerBegin()
{
  DN_OSTimer result = {};
  result.start      = DN_OS_PerfCounterNow();
  return result;
}

DN_API void DN_OS_TimerEnd(DN_OSTimer *timer)
{
  timer->end = DN_OS_PerfCounterNow();
}

DN_API DN_F64 DN_OS_TimerS(DN_OSTimer timer)
{
  DN_F64 result = DN_OS_PerfCounterS(timer.start, timer.end);
  return result;
}

DN_API DN_F64 DN_OS_TimerMs(DN_OSTimer timer)
{
  DN_F64 result = DN_OS_PerfCounterMs(timer.start, timer.end);
  return result;
}

DN_API DN_F64 DN_OS_TimerUs(DN_OSTimer timer)
{
  DN_F64 result = DN_OS_PerfCounterUs(timer.start, timer.end);
  return result;
}

DN_API DN_F64 DN_OS_TimerNs(DN_OSTimer timer)
{
  DN_F64 result = DN_OS_PerfCounterNs(timer.start, timer.end);
  return result;
}

DN_API uint64_t DN_OS_EstimateTscPerSecond(uint64_t duration_ms_to_gauge_tsc_frequency)
{
  uint64_t os_frequency      = DN_OS_PerfCounterFrequency();
  uint64_t os_target_elapsed = duration_ms_to_gauge_tsc_frequency * os_frequency / 1000ULL;
  uint64_t tsc_begin         = DN_CPUGetTsc();
  uint64_t result            = 0;
  if (tsc_begin) {
    uint64_t os_elapsed = 0;
    for (uint64_t os_begin = DN_OS_PerfCounterNow(); os_elapsed < os_target_elapsed;)
      os_elapsed = DN_OS_PerfCounterNow() - os_begin;
    uint64_t tsc_end     = DN_CPUGetTsc();
    uint64_t tsc_elapsed = tsc_end - tsc_begin;
    result               = tsc_elapsed / os_elapsed * os_frequency;
  }
  return result;
}

DN_API bool DN_OS_FileRotate(DN_Str8 base_file_path, DN_USize rotate_count, DN_Str8 suffix, DN_OSFileRotateFlags flags)
{
  // NOTE: Loop through all the rotated files from [rotate_count-1..0]. The rotated file at
  // `rotate_count-1` (if it exists) gets deleted and `rotate_count-2` gets moved into
  // `rotate_count-1` and so forth.
  bool result  = true;
  if (rotate_count) {
    if (rotate_count == 1) {
      // NOTE: Rotate count of 1 just means that the base file should be deleted essentially unless
      // the keep base flag is set.
      if (DN_BitIsNotSet(flags, DN_OSFileRotateFlags_KeepBaseFile))
        DN_OS_PathDelete(base_file_path);
    } else {
      DN_TcScratch scratch = DN_TcScratchBeginArena(nullptr, 0);
      for (DN_USize offset = 0; offset < (rotate_count - 1); offset++) {
        DN_USize const file_index      = rotate_count - (offset + 1);
        bool           last_file_index = file_index - 1 == 0;
        DN_AssertF(file_index != 0, "This index should never hits zero, we iterate in reverse and stop 1 before the last one");

        DN_Str8       file_path           = DN_Str8FmtArena(&scratch.arena, "%.*s%.*s%zu", DN_Str8PrintFmt(base_file_path), DN_Str8PrintFmt(suffix), file_index);
        DN_Str8       prev_file_path      = last_file_index ? base_file_path : DN_Str8FmtArena(&scratch.arena, "%.*s%.*s%zu", DN_Str8PrintFmt(base_file_path), DN_Str8PrintFmt(suffix), file_index - 1);
        DN_OSPathInfo file_path_info      = DN_OS_PathInfo(file_path);
        DN_OSPathInfo prev_file_path_info = DN_OS_PathInfo(prev_file_path);

        if (file_path_info.type == DN_OSPathInfoType_Directory || prev_file_path_info.type == DN_OSPathInfoType_Directory) {
          result = false;
          break;
        }

        if (prev_file_path_info.exists) {
          if (last_file_index && (flags & DN_OSFileRotateFlags_KeepBaseFile))
            result &= DN_OS_FileCopy(prev_file_path, file_path, /*overwrite=*/true, nullptr);
          else
            result &= DN_OS_FileMove(prev_file_path, file_path, /*overwrite=*/true, nullptr);
        }
      }
      DN_TcScratchEnd(&scratch);
    }
  }
  return result;
}

DN_API bool DN_OS_FileWrite(DN_OSFile *file, DN_Str8 buffer, DN_ErrSink *error)
{
  bool result = DN_OS_FileWritePtr(file, buffer.data, buffer.count, error);
  return result;
}

typedef struct DN_OSFileWriteChunker_ DN_OSFileWriteChunker_;
struct DN_OSFileWriteChunker_
{
  DN_ErrSink *err;
  DN_OSFile  *file;
  bool        success;
};

static char *DN_OS_FileWriteChunker_(const char *buf, void *user, int len)
{
  DN_OSFileWriteChunker_ *chunker = DN_Cast(DN_OSFileWriteChunker_ *)user;
  chunker->success                = DN_OS_FileWritePtr(chunker->file, buf, len, chunker->err);
  char *result                    = chunker->success ? DN_Cast(char *) buf : nullptr;
  return result;
}

DN_API bool DN_OS_FileWriteFV(DN_OSFile *file, DN_ErrSink *error, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  bool result = false;
  if (!file || !fmt)
    return result;

  DN_OSFileWriteChunker_ chunker = {};
  chunker.err                    = error;
  chunker.file                   = file;
  char buffer[STB_SPRINTF_MIN];
  STB_SPRINTF_DECORATE(vsprintfcb)(DN_OS_FileWriteChunker_, &chunker, buffer, fmt, args);

  result = chunker.success;
  return result;
}

DN_API bool DN_OS_FileWriteF(DN_OSFile *file, DN_ErrSink *error, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  bool result = DN_OS_FileWriteFV(file, error, fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8 DN_OS_FileReadAll(DN_Allocator allocator, DN_Str8 path, DN_ErrSink *err)
{
  // NOTE: Query file size
  DN_Str8       result    = {};
  DN_OSPathInfo path_info = DN_OS_PathInfo(path);
  if (!path_info.exists) {
    DN_ErrSinkAppendF(err, 1, "File does not exist/could not be queried for reading '%.*s'", DN_Str8PrintFmt(path));
    return result;
  }

  // NOTE: Allocate
  DN_Arena temp_arena = {};
  if (allocator.type == DN_AllocatorType_Arena) {
    DN_Arena *arena = DN_Cast(DN_Arena *) allocator.context;
    temp_arena      = DN_ArenaTempBeginFromArena(arena);
    result          = DN_Str8AllocArena(path_info.size, DN_ZMem_No, &temp_arena);
  } else {
    DN_Pool *pool = DN_Cast(DN_Pool *) allocator.context;
    result        = DN_Str8AllocPool(path_info.size, pool);
  }

  if (!result.data) {
    DN_Str8x32 bytes_str = DN_Str8x32FromByteCountU64Auto(path_info.size);
    DN_ErrSinkAppendF(err, 1 /*err_code*/, "Failed to allocate %.*s for reading file '%.*s'", DN_Str8PrintFmt(bytes_str), DN_Str8PrintFmt(path));
    return result;
  }

  // NOTE: Read all
  DN_OSFile     file   = DN_OS_FileOpen(path, DN_OSFileOpen_OpenIfExist, DN_OSFileAccess_Read, err);
  DN_OSFileRead read   = DN_OS_FileRead(&file, result.data, result.count, err);
  bool          failed = file.error || !read.success;

  if (allocator.type == DN_AllocatorType_Arena) {
    DN_ArenaTempEnd(&temp_arena, failed ? DN_ArenaReset_Yes : DN_ArenaReset_No);
  } else {
    if (failed) {
      DN_Pool *pool = DN_Cast(DN_Pool *) allocator.context;
      DN_PoolDealloc(pool, result.data);
    }
  }

  if (failed)
    result = {};

  DN_OS_FileClose(&file);
  return result;
}

DN_API DN_Str8 DN_OS_FileReadAllArena(DN_Arena *arena, DN_Str8 path, DN_ErrSink *err)
{
  DN_Allocator allocator = {};
  allocator.type         = DN_AllocatorType_Arena;
  allocator.context      = arena;
  DN_Str8 result = DN_OS_FileReadAll(allocator, path, err);
  return result;
}

DN_API DN_Str8 DN_OS_FileReadAllPool(DN_Pool *pool, DN_Str8 path, DN_ErrSink *err)
{
  DN_Allocator allocator = {};
  allocator.type         = DN_AllocatorType_Pool;
  allocator.context      = pool;
  DN_Str8 result         = DN_OS_FileReadAll(allocator, path, err);
  return result;
}

DN_API bool DN_OS_FileWriteAll(DN_Str8 path, DN_Str8 buffer, DN_ErrSink *error)
{
  DN_OSFile file   = DN_OS_FileOpen(path, DN_OSFileOpen_CreateAlways, DN_OSFileAccess_Write, error);
  bool      result = DN_OS_FileWrite(&file, buffer, error);
  DN_OS_FileClose(&file);
  return result;
}

DN_API bool DN_OS_FileWriteAllFV(DN_Str8 file_path, DN_ErrSink *error, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_TcScratch scratch = DN_TcScratchBeginArena(nullptr, 0);
  DN_Str8      buffer  = DN_Str8FmtVArena(&scratch.arena, fmt, args);
  bool         result  = DN_OS_FileWriteAll(file_path, buffer, error);
  DN_TcScratchEnd(&scratch);
  return result;
}

DN_API bool DN_OS_FileWriteAllF(DN_Str8 file_path, DN_ErrSink *error, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  bool result = DN_OS_FileWriteAllFV(file_path, error, fmt, args);
  va_end(args);
  return result;
}

DN_API bool DN_OS_FileWriteAllSafe(DN_Str8 path, DN_Str8 buffer, DN_ErrSink *error)
{
  DN_TcScratch scratch  = DN_TcScratchBeginArena(nullptr, 0);
  DN_Str8      tmp_path = DN_Str8FmtArena(&scratch.arena, "%.*s.tmp", DN_Str8PrintFmt(path));
  if (!DN_OS_FileWriteAll(tmp_path, buffer, error)) {
    DN_TcScratchEnd(&scratch);
    return false;
  }
  if (!DN_OS_FileCopy(tmp_path, path, true /*overwrite*/, error)) {
    DN_TcScratchEnd(&scratch);
    return false;
  }
  if (!DN_OS_PathDelete(tmp_path)) {
    DN_TcScratchEnd(&scratch);
    return false;
  }
  DN_TcScratchEnd(&scratch);
  return true;
}

DN_API bool DN_OS_FileWriteAllSafeFV(DN_Str8 path, DN_ErrSink *error, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_TcScratch scratch = DN_TcScratchBeginArena(nullptr, 0);
  DN_Str8      buffer  = DN_Str8FmtVArena(&scratch.arena, fmt, args);
  bool         result  = DN_OS_FileWriteAllSafe(path, buffer, error);
  DN_TcScratchEnd(&scratch);
  return result;
}

DN_API bool DN_OS_FileWriteAllSafeF(DN_Str8 path, DN_ErrSink *error, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  bool result = DN_OS_FileWriteAllSafeFV(path, error, fmt, args);
  return result;
}

DN_API DN_Str8 DN_OS_Str8FromPathInfoType(DN_OSPathInfoType type)
{
  DN_Str8 result = DN_Str8Lit("BAD PATH INFO TYPE");
  switch(type) {
    case DN_OSPathInfoType_Unknown:   result = DN_Str8Lit("Unknown");   break;
    case DN_OSPathInfoType_Directory: result = DN_Str8Lit("Directory"); break;
    case DN_OSPathInfoType_File:      result = DN_Str8Lit("File");      break;
  }
  return result;
}

DN_API bool DN_OS_PathIsOlderThan(DN_Str8 path, DN_Str8 check_against)
{
  DN_OSPathInfo file_info          = DN_OS_PathInfo(path);
  DN_OSPathInfo check_against_info = DN_OS_PathInfo(check_against);
  bool          result             = !file_info.exists || file_info.last_write_time_in_s < check_against_info.last_write_time_in_s;
  return result;
}

DN_API DN_OSExecArgs DN_OS_ExecArgsDefault()
{
  DN_OSExecArgs result  = {};
  result.flags         |= DN_OSExecFlags_SaveOutput;
  return result;
}

DN_API DN_OSExecResult DN_OS_Exec(DN_Str8Slice cmd_line, DN_OSExecArgs args, DN_Arena *arena, DN_ErrSink *error)
{
  DN_OSExecAsyncHandle async_handle = DN_OS_ExecAsync(cmd_line, args, error);
  DN_OSExecResult      result       = DN_OS_ExecWait(async_handle, arena, error);
  return result;
}

DN_API DN_OSExecResult DN_OS_ExecOrAbort(DN_Str8Slice cmd_line, DN_OSExecArgs args, DN_Arena *arena)
{
  DN_ErrSink     *error  = DN_TcErrSinkBegin(DN_ErrSinkMode_Nil);
  DN_OSExecResult result = DN_OS_Exec(cmd_line, args, arena, error);
  if (result.os_error_code)
    DN_ErrSinkEndExitIfErrorF(error, result.os_error_code, "OS failed to execute the requested command returning the error code %u", result.os_error_code);

  if (result.exit_code)
    DN_ErrSinkEndExitIfErrorF(error, result.exit_code, "OS executed command and returned non-zero exit code %u", result.exit_code);
  DN_ErrSinkEndIgnore(error);
  return result;
}

static void DN_OS_ThreadExecute_(void *user_context)
{
  DN_OSThread *thread = DN_Cast(DN_OSThread *) user_context;

  // NOTE: Wait on the semaphore, the thread that initiated the thread is going to retrieve the
  // thread ID, then set it the value on `thread->thread_id` pointer then increment this semaphore
  // to unblock this thread.
  //
  // This semaphore is deleted in the caller's thread when we unblock them by signalling the
  // semaphore below this.
  DN_OS_SemaphoreWait(&thread->thread_exec_wait_for_thread_id_sem, DN_OS_SEMAPHORE_INFINITE_TIMEOUT);

  // NOTE: Setup thread context (TLS) _after_ thread ID is setup.
  DN_TcInitFromHeap(&thread->context, thread->thread_id, thread->tc_init_args, DN_OS_HeapInitDefault());

  // NOTE: Once all initialisation is done, if the thread is to be detached, make a copy of the
  // thread pointer because the caller is not guaranteeing to keep the thread pointer alive (they
  // can throw away the DN_OSThread since they want to detach the thread)
  //
  // Note that the caller _cannot_ throw away the thread pointer until we increment the semaphore
  // below as the OS implementation _should_ be sleeping on that semaphore.
  if (thread->flags & DN_OSThreadFlags_Detached)
    thread = DN_ArenaNewCopy(thread->context.main_arena, DN_OSThread, thread);

  // NOTE: Equip the pointers into TLS only _after_ the thread context is copied (if it was
  // detached) to avoid potential dangling ref in the TLS.
  DN_TcEquip(&thread->context);
  if (thread->is_lane_set) {
    DN_OS_TcThreadLaneEquip(thread->lane);
    DN_OS_ThreadSetNameFmt("L%02zu/%02zu T%zu", thread->lane.index, thread->lane.count, thread->thread_id);
  } else {
    DN_OS_ThreadSetNameFmt("T%zu", thread->lane.index, thread->lane.count, thread->thread_id);
  }

  // NOTE: Now we can increment the semaphore that the caller's thread is waiting on now that we've
  // safely initialised the thread's contents and made a copy if necessary.
  DN_OS_SemaphoreIncrement(&thread->caller_wait_for_thread_init_to_finish_sem, 1);

  // NOTE: Run the user's code
  thread->func(thread);

  // NOTE: If we're detached, it's this thread's responsibility to cleanup itself. In the platform
  // layer it should have closed any references to the thread so we should just need to cleanup the
  // TLS.
  if (thread->flags & DN_OSThreadFlags_Detached) {
    #if !defined(DN_PLATFORM_WIN32)
    // NOTE: Cleanup the semaphore since we're detached, all left-over resources need to be cleaned
    // up ourselves.
    DN_OS_SemaphoreDeinit(&thread->join_done_sem);
    #endif
    DN_TcDeinit(&thread->context, DN_TcDeinitArenas_Yes);
  } else {
    #if !defined(DN_PLATFORM_WIN32)
    DN_OS_SemaphoreIncrement(&thread->join_done_sem, 1); // NOTE: Signal for DN_OS_ThreadJoin waits on this.
    #endif
  }
}

static void DN_OS_ThreadPreInit_(DN_OSThread *thread, DN_OSThreadFunc *func, DN_OSThreadLane *lane, DN_OSThreadInitArgs init_args, void *user_context)
{
  if (thread) {
    thread->func                                      = func;
    thread->user_context                              = user_context;
    thread->thread_exec_wait_for_thread_id_sem        = DN_OS_SemaphoreInit(0 /*initial_count*/);
    thread->caller_wait_for_thread_init_to_finish_sem = DN_OS_SemaphoreInit(0 /*initial_count*/);
#if !defined(DN_PLATFORM_WIN32)
    thread->join_done_sem                             = DN_OS_SemaphoreInit(0 /*initial_count*/);
#endif
    thread->tc_init_args                              = init_args.tc_args;
    thread->flags                                     = init_args.flags;
    if (lane) {
      thread->is_lane_set = true;
      thread->lane        = *lane;
    }
  }
}

static void DN_OS_ThreadPostInit_(DN_OSThread *thread, bool result)
{
  // NOTE: Ensure that thread_id is set before 'thread->func' is called.
  if (result) {
    // NOTE: Unblock the thread executor now that thread_id and flags have been set
    DN_OS_SemaphoreIncrement(&thread->thread_exec_wait_for_thread_id_sem, 1);

    // NOTE: Wait for the thread to finish initialising before we leave
    DN_OS_SemaphoreWait(&thread->caller_wait_for_thread_init_to_finish_sem,
                        DN_OS_SEMAPHORE_INFINITE_TIMEOUT);
  }

  // NOTE: Clean up the semaphores
  DN_OS_SemaphoreDeinit(&thread->thread_exec_wait_for_thread_id_sem);
  DN_OS_SemaphoreDeinit(&thread->caller_wait_for_thread_init_to_finish_sem);

  if (!result) {
    #if !defined(DN_PLATFORM_WIN32)
    DN_OS_SemaphoreDeinit(&thread->join_done_sem);
    #endif
    *thread = {};
  }
}

DN_API DN_OSThreadInitArgs DN_OS_ThreadInitArgsDefault()
{
  DN_OSThreadInitArgs result = {};
  result.tc_args             = DN_TcInitArgsDefault();
  return result;
}

DN_API bool DN_OS_ThreadInit(DN_OSThread *thread, DN_OSThreadFunc *func, DN_OSThreadInitArgs init_args, void *user_context)
{
  bool result = DN_OS_ThreadInitLane(thread, func, nullptr, init_args, user_context);
  return result;
}

DN_API void DN_OS_ThreadSetNameFmt(char const *fmt, ...)
{
  DN_TcCore *tls = DN_TcGet();
  va_list args;
  va_start(args, fmt);
  tls->name = DN_Str8x64FromFmtV(fmt, args);
  va_end(args);

  DN_Str8 name = DN_Str8FromPtr(tls->name.data, tls->name.count);
#if defined(DN_PLATFORM_WIN32)
  DN_OS_W32ThreadSetName(name);
#else
  DN_OS_PosixThreadSetName(name);
#endif
}

DN_API DN_OSThreadLane DN_OS_ThreadLaneInit(DN_USize index, DN_USize thread_count, DN_OSBarrier barrier, DN_UPtr *shared_mem)
{
  DN_OSThreadLane result = {};
  result.index           = index;
  result.count           = thread_count;
  result.barrier         = barrier;
  result.shared_mem      = shared_mem;
  return result;
}

DN_API void DN_OS_ThreadLaneSync(DN_OSThreadLane *lane, void **ptr_to_share)
{
  if (!lane)
    return;

  // NOTE: Write the pointer into shared memory (if we're the lane producing the data)
  bool sharing = false;
  if (ptr_to_share && *ptr_to_share) {
    DN_Memcpy(lane->shared_mem, ptr_to_share, sizeof(*ptr_to_share));
    sharing = true;
  }

  DN_OS_BarrierWait(&lane->barrier); // NOTE: Ensure sharing lane has completed the write

  // NOTE: Read pointer from shared memory (if we're the other lanes that read the data)
  if (ptr_to_share && !(*ptr_to_share)) {
    sharing = true;
    DN_Memcpy(ptr_to_share, lane->shared_mem, sizeof(*ptr_to_share));
  }

  if (sharing)
    DN_OS_BarrierWait(&lane->barrier); // NOTE: Ensure the reading lanes have completed the read
}

DN_API DN_V2USize DN_OS_ThreadLaneRange(DN_OSThreadLane const *lane, DN_USize values_count)
{
  DN_USize values_per_thread                  = values_count / lane->count;
  DN_USize rem_values                         = values_count % lane->count;
  bool     thread_has_leftovers               = lane->index < rem_values;
  DN_USize leftovers_before_this_thread_index = 0;

  if (thread_has_leftovers)
    leftovers_before_this_thread_index = lane->index;
  else
    leftovers_before_this_thread_index = rem_values;

  DN_USize thread_start_index  = (values_per_thread * lane->index) + leftovers_before_this_thread_index;
  DN_USize thread_values_count = values_per_thread + (thread_has_leftovers ? 1 : 0);

  DN_V2USize result = {};
  result.begin      = thread_start_index;
  result.end        = result.begin + thread_values_count;
  return result;
}

DN_API DN_OSThreadLaneway DN_OS_ThreadLanewayFromArgs(DN_OSThread* threads, DN_USize threads_count, DN_UPtr* shared_mem)
{
  DN_OSThreadLaneway result = {};
  result.threads            = threads;
  result.threads_count      = threads_count;
  result.shared_mem         = shared_mem;
  result.barrier            = DN_OS_BarrierInit(DN_Cast(DN_U32) result.threads_count);
  return result;
}

DN_API DN_OSThreadLaneway DN_OS_ThreadLanewayFromArena(DN_USize threads_count, DN_Arena* arena)
{
  DN_U64             mem_p      = DN_MemListPos(arena->mem);
  DN_OSThreadLaneway result     = {};
  DN_OSThread*       threads    = DN_ArenaNewArray(arena, DN_OSThread, threads_count, DN_ZMem_No);
  DN_UPtr*           shared_mem = DN_ArenaNewZ(arena, DN_UPtr);
  if (threads && shared_mem)
    result = DN_OS_ThreadLanewayFromArgs(threads, threads_count, shared_mem);
  else
    DN_MemListPopTo(arena->mem, mem_p);
  return result;
}

DN_API void DN_OS_ThreadLanewayDispatch(DN_OSThreadLaneway *laneway, DN_OSThreadFunc *entry_point, DN_OSThreadInitArgs init_args, void *user_context)
{
  for (DN_ForItSize(it, DN_OSThread, laneway->threads, laneway->threads_count)) {
    DN_OSThreadLane lane = DN_OS_ThreadLaneInit(it.index, laneway->threads_count, laneway->barrier, laneway->shared_mem);
    DN_OS_ThreadInitLane(it.data, entry_point, &lane, init_args, user_context);
  }
}

DN_API void DN_OS_ThreadLanewayJoin(DN_OSThreadLaneway *laneway, DN_U32 timeout_ms, DN_TcDeinitArenas deinit_arenas)
{
  for (DN_ForItSize(it, DN_OSThread, laneway->threads, laneway->threads_count))
    DN_OS_ThreadJoin(it.data, timeout_ms, deinit_arenas);
  DN_OS_BarrierDeinit(&laneway->barrier);
}

DN_API DN_OSThreadLane *DN_OS_TcThreadLane()
{
  DN_TcCore       *tc     = DN_TcGet();
  DN_OSThreadLane *result = tc ? DN_Cast(DN_OSThreadLane *) tc->lane_opaque : nullptr;
  return result;
}

DN_API void DN_OS_TcThreadLaneSync(void **ptr_to_share)
{
  DN_OSThreadLane *lane = DN_OS_TcThreadLane();
  DN_OS_ThreadLaneSync(lane, ptr_to_share);
}

DN_API DN_OSThreadLane DN_OS_TcThreadLaneEquip(DN_OSThreadLane lane)
{
  DN_TcCore       *tc   = DN_TcGet();
  DN_OSThreadLane *curr = DN_Cast(DN_OSThreadLane *) tc->lane_opaque;
  DN_StaticAssert(sizeof(tc->lane_opaque) >= sizeof(DN_OSThreadLane));
  DN_OSThreadLane result = *curr;
  *curr                  = lane;
  return result;
}

static DN_I32 DN_OS_AsyncThreadEntryPoint_(DN_OSThread *thread)
{
  DN_OS_ThreadSetNameFmt("%.*s", DN_Str8PrintFmt(thread->name));
  DN_OSAsyncCore *async = DN_Cast(DN_OSAsyncCore *) thread->user_context;
  DN_Ring      *ring  = &async->ring;
  for (;;) {
    DN_OS_SemaphoreWait(&async->worker_sem, UINT32_MAX);
    if (async->join_threads)
        break;

    DN_OSAsyncTask task = {};
    for (DN_OS_MutexScope(&async->ring_mutex)) {
      if (DN_RingHasData(ring, sizeof(task)))
        DN_RingRead(ring, &task, sizeof(task));
    }

    if (task.work.func) {
      DN_OS_ConditionVariableBroadcast(&async->ring_write_cv); // Resume any blocked ring write(s)

      DN_OSAsyncWorkArgs args = {};
      args.input            = task.work.input;
      args.thread           = thread;

      DN_AtomicAddU32(&async->busy_threads, 1);
      task.work.func(args);
      DN_AtomicSubU32(&async->busy_threads, 1);

      if (task.completion_sem.handle != 0)
        DN_OS_SemaphoreIncrement(&task.completion_sem, 1);
    }
  }

  return 0;
}

DN_API void DN_OS_AsyncInit(DN_OSAsyncCore *async, char *base, DN_USize base_size, DN_OSThread *threads, DN_U32 threads_size)
{
  DN_Assert(async);
  async->ring.size     = base_size;
  async->ring.base     = base;
  async->ring_mutex    = DN_OS_MutexInit();
  async->ring_write_cv = DN_OS_ConditionVariableInit();
  async->worker_sem    = DN_OS_SemaphoreInit(0);
  async->thread_count  = threads_size;
  async->threads       = threads;
  for (DN_ForIndexU(index, async->thread_count)) {
    DN_OSThread    *thread = async->threads + index;
    DN_OS_ThreadInit(thread, DN_OS_AsyncThreadEntryPoint_, DN_OS_ThreadInitArgsDefault(), async);
  }
}

DN_API void DN_OS_AsyncDeinit(DN_OSAsyncCore *async)
{
  DN_Assert(async);
  DN_AtomicSetValue32(&async->join_threads, true);
  DN_OS_SemaphoreIncrement(&async->worker_sem, async->thread_count);
  for (DN_ForItSize(it, DN_OSThread, async->threads, async->thread_count))
    DN_OS_ThreadJoin(it.data, UINT32_MAX, DN_TcDeinitArenas_Yes);
}

static bool DN_OS_AsyncQueueTask_(DN_OSAsyncCore *async, DN_OSAsyncTask const *task, DN_U64 wait_time_ms) {
  DN_U64 end_time_ms = DN_OS_DateUnixTimeMs() + wait_time_ms;
  bool result = false;
  for (DN_OS_MutexScope(&async->ring_mutex)) {
    for (;;) {
      if (DN_RingHasSpace(&async->ring, sizeof(*task))) {
        DN_RingWriteStruct(&async->ring, task);
        result = true;
        break;
      }
      DN_OS_ConditionVariableWaitUntil(&async->ring_write_cv, &async->ring_mutex, end_time_ms);
      if (DN_OS_DateUnixTimeMs() >= end_time_ms)
        break;
    }
  }

  if (result)
    DN_OS_SemaphoreIncrement(&async->worker_sem, 1); // Flag that a job is available

  return result;
}

DN_API bool DN_OS_AsyncQueueWork(DN_OSAsyncCore *async, DN_OSAsyncWorkFunc *func, void *input, DN_U64 wait_time_ms)
{
  DN_OSAsyncTask task = {};
  task.work.func    = func;
  task.work.input   = input;
  bool result       = DN_OS_AsyncQueueTask_(async, &task, wait_time_ms);
  return result;
}

DN_API DN_OSAsyncTask DN_OS_AsyncQueueTask(DN_OSAsyncCore *async, DN_OSAsyncWorkFunc *func, void *input, DN_U64 wait_time_ms)
{
  DN_OSAsyncTask result   = {};
  result.work.func      = func;
  result.work.input     = input;
  result.completion_sem = DN_OS_SemaphoreInit(0);
  result.queued         = DN_OS_AsyncQueueTask_(async, &result, wait_time_ms);
  if (!result.queued)
    DN_OS_SemaphoreDeinit(&result.completion_sem);
  return result;
}

DN_API bool DN_OS_AsyncWaitTask(DN_OSAsyncTask *task, DN_U32 timeout_ms)
{
  bool result = true;
  if (!task->queued)
    return result;

  DN_OSSemaphoreWaitResult wait = DN_OS_SemaphoreWait(&task->completion_sem, timeout_ms);
  result                        = wait == DN_OSSemaphoreWaitResult_Success;
  if (result)
    DN_OS_SemaphoreDeinit(&task->completion_sem);
  return result;
}

DN_API DN_LogStyle DN_OS_PrintStyleColour(uint8_t r, uint8_t g, uint8_t b, DN_LogBold bold)
{
  DN_LogStyle result = {};
  result.bold          = bold;
  result.colour        = true;
  result.r             = r;
  result.g             = g;
  result.b             = b;
  return result;
}

DN_API DN_LogStyle DN_OS_PrintStyleColourU32(uint32_t rgb, DN_LogBold bold)
{
  uint8_t       r      = (rgb >> 24) & 0xFF;
  uint8_t       g      = (rgb >> 16) & 0xFF;
  uint8_t       b      = (rgb >> 8) & 0xFF;
  DN_LogStyle result = DN_OS_PrintStyleColour(r, g, b, bold);
  return result;
}

DN_API DN_LogStyle DN_OS_PrintStyleBold()
{
  DN_LogStyle result = {};
  result.bold          = DN_LogBold_Yes;
  return result;
}

DN_API void DN_OS_Print(DN_OSPrintDest dest, DN_Str8 string)
{
  DN_Assert(dest == DN_OSPrintDest_Out || dest == DN_OSPrintDest_Err);

#if defined(DN_PLATFORM_WIN32)
  // NOTE: Get the output handles from kernel
  DN_THREAD_LOCAL void *std_out_print_handle     = nullptr;
  DN_THREAD_LOCAL void *std_err_print_handle     = nullptr;
  DN_THREAD_LOCAL bool  std_out_print_to_console = false;
  DN_THREAD_LOCAL bool  std_err_print_to_console = false;

  if (!std_out_print_handle) {
    unsigned long mode = 0;
    (void)mode;
    std_out_print_handle     = GetStdHandle(STD_OUTPUT_HANDLE);
    std_out_print_to_console = GetConsoleMode(std_out_print_handle, &mode) != 0;

    std_err_print_handle     = GetStdHandle(STD_ERROR_HANDLE);
    std_err_print_to_console = GetConsoleMode(std_err_print_handle, &mode) != 0;
  }

  // NOTE: Select the output handle
  void *print_handle     = std_out_print_handle;
  bool  print_to_console = std_out_print_to_console;
  if (dest == DN_OSPrintDest_Err) {
    print_handle     = std_err_print_handle;
    print_to_console = std_err_print_to_console;
  }

  // NOTE: Write the string
  DN_Assert(string.count < DN_Cast(unsigned long) - 1);
  unsigned long bytes_written = 0;
  (void)bytes_written;
  if (print_to_console) {
    DN_TcScratch scratch = DN_TcScratchBeginArena(nullptr, 0);
    DN_Str16 string16 = DN_OS_W32Str8ToStr16(&scratch.arena, string);
    WriteConsoleW(print_handle, string16.data, DN_Cast(unsigned long) string16.count, &bytes_written, nullptr);
    DN_TcScratchEnd(&scratch);
  } else {
    WriteFile(print_handle, string.data, DN_Cast(unsigned long) string.count, &bytes_written, nullptr);
  }
#else
  fprintf(dest == DN_OSPrintDest_Out ? stdout : stderr, "%.*s", DN_Str8PrintFmt(string));
#endif
}

DN_API void DN_OS_PrintF(DN_OSPrintDest dest, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_OS_PrintFV(dest, fmt, args);
  va_end(args);
}

DN_API void DN_OS_PrintFStyle(DN_OSPrintDest dest, DN_LogStyle style, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_OS_PrintFVStyle(dest, style, fmt, args);
  va_end(args);
}

DN_API void DN_OS_PrintStyle(DN_OSPrintDest dest, DN_LogStyle style, DN_Str8 string)
{
  if (string.data && string.count) {
    if (style.colour) {
      DN_Str8x32 colour = DN_Str8x32FromAnsiColourCodeU8Rgb(DN_AnsiColourMode_Fg, style.r, style.g, style.b);
      DN_OS_Print(dest, DN_Str8FromStruct(&colour));
    }
    if (style.bold == DN_LogBold_Yes)
      DN_OS_Print(dest, DN_Str8Lit(DN_AnsiCodeBoldLit));
    DN_OS_Print(dest, string);
    if (style.colour || style.bold == DN_LogBold_Yes)
      DN_OS_Print(dest, DN_Str8Lit(DN_AnsiCodeResetLit));
  }
}

static char *DN_OS_PrintVSPrintfChunker_(const char *buf, void *user, int len)
{
  DN_Str8 string = {};
  string.data    = DN_Cast(char *) buf;
  string.count   = len;

  DN_OSPrintDest dest = DN_Cast(DN_OSPrintDest) DN_Cast(uintptr_t) user;
  DN_OS_Print(dest, string);
  return (char *)buf;
}

DN_API void DN_OS_PrintFV(DN_OSPrintDest dest, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  char buffer[STB_SPRINTF_MIN];
  STB_SPRINTF_DECORATE(vsprintfcb)
  (DN_OS_PrintVSPrintfChunker_, DN_Cast(void *) DN_Cast(uintptr_t) dest, buffer, fmt, args);
}

DN_API void DN_OS_PrintFVStyle(DN_OSPrintDest dest, DN_LogStyle style, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  if (fmt) {
    if (style.colour) {
      DN_Str8x32 colour = DN_Str8x32FromAnsiColourCodeU8Rgb(DN_AnsiColourMode_Fg, style.r, style.g, style.b);
      DN_OS_Print(dest, DN_Str8FromStruct(&colour));
    }
    if (style.bold == DN_LogBold_Yes)
      DN_OS_Print(dest, DN_Str8Lit(DN_AnsiCodeBoldLit));
    DN_OS_PrintFV(dest, fmt, args);
    if (style.colour || style.bold == DN_LogBold_Yes)
      DN_OS_Print(dest, DN_Str8Lit(DN_AnsiCodeResetLit));
  }
}

DN_API void DN_OS_PrintLn(DN_OSPrintDest dest, DN_Str8 string)
{
  DN_OS_Print(dest, string);
  DN_OS_Print(dest, DN_Str8Lit("\n"));
}

DN_API void DN_OS_PrintLnF(DN_OSPrintDest dest, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_OS_PrintLnFV(dest, fmt, args);
  va_end(args);
}

DN_API void DN_OS_PrintLnFV(DN_OSPrintDest dest, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_OS_PrintFV(dest, fmt, args);
  DN_OS_Print(dest, DN_Str8Lit("\n"));
}

DN_API void DN_OS_PrintLnStyle(DN_OSPrintDest dest, DN_LogStyle style, DN_Str8 string)
{
  DN_OS_PrintStyle(dest, style, string);
  DN_OS_Print(dest, DN_Str8Lit("\n"));
}

DN_API void DN_OS_PrintLnFStyle(DN_OSPrintDest dest, DN_LogStyle style, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_OS_PrintLnFVStyle(dest, style, fmt, args);
  va_end(args);
}

DN_API void DN_OS_PrintLnFVStyle(DN_OSPrintDest dest, DN_LogStyle style, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_OS_PrintFVStyle(dest, style, fmt, args);
  DN_OS_Print(dest, DN_Str8Lit("\n"));
}

DN_API DN_StackTrace DN_StackTraceFromAllocator(DN_Allocator allocator, DN_U16 limit)
{
  DN_StackTrace result = {};
#if defined(DN_OS_WIN32)
  if (!allocator.context)
    return result;

  static DN_TicketMutex mutex = {};
  DN_TicketMutexBegin(&mutex);

  HANDLE thread  = GetCurrentThread();
  result.process = GetCurrentProcess();

  DN_OSW32Core *w32 = DN_OS_W32GetCore();
  if (!w32->sym_initialised) {
    w32->sym_initialised = true;
    SymSetOptions(SYMOPT_LOAD_LINES);
    if (!SymInitialize(result.process, nullptr /*UserSearchPath*/, true /*fInvadeProcess*/)) {
      DN_TcScratch  scratch = DN_TcScratchBeginAllocator(&allocator, 1);
      DN_OSW32Error error   = DN_OS_W32LastError(&scratch.arena);
      DN_LogErrorF("SymInitialize failed, stack trace can not be generated (%lu): %.*s\n", error.code, DN_Str8PrintFmt(error.msg));
      DN_TcScratchEnd(&scratch);
    }
  }

  CONTEXT context;
  RtlCaptureContext(&context);

  STACKFRAME64 frame     = {};
  frame.AddrPC.Offset    = context.Rip;
  frame.AddrPC.Mode      = AddrModeFlat;
  frame.AddrFrame.Offset = context.Rbp;
  frame.AddrFrame.Mode   = AddrModeFlat;
  frame.AddrStack.Offset = context.Rsp;
  frame.AddrStack.Mode   = AddrModeFlat;

  DN_U64   raw_frames[256]  = {};
  DN_USize raw_frames_count = 0;
  while (raw_frames_count < limit) {
    if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64,
                     result.process,
                     thread,
                     &frame,
                     &context,
                     nullptr /*ReadMemoryRoutine*/,
                     SymFunctionTableAccess64,
                     SymGetModuleBase64,
                     nullptr /*TranslateAddress*/))
      break;

    // NOTE: It might be useful one day to use frame.AddrReturn.Offset.
    // If AddrPC.Offset == AddrReturn.Offset then we can detect recursion.
    DN_LArrayAppend(raw_frames, &raw_frames_count, frame.AddrPC.Offset);
  }
  DN_TicketMutexEnd(&mutex);

  result.base_addr = DN_Cast(DN_U64 *)DN_AllocatorAlloc(allocator, raw_frames_count * sizeof(DN_U64), alignof(DN_U64), DN_ZMem_No);
  DN_Assert(result.base_addr);

  result.size      = DN_Cast(DN_U16) raw_frames_count;
  DN_Memcpy(result.base_addr, raw_frames, raw_frames_count * sizeof(raw_frames[0]));
#else
  (void)limit;
  (void)allocator;
#endif
  return result;
}


DN_API DN_StackTrace DN_StackTraceFromArena(DN_Arena *arena, DN_U16 limit)
{
  DN_Allocator  allocator = DN_AllocatorFromArena(arena);
  DN_StackTrace result    = DN_StackTraceFromAllocator(allocator, limit);
  return result;
}

static void DN_StackTraceAddToStr8Builder_(DN_StackTrace const *trace, DN_Str8Builder *builder, DN_USize skip)
{
  DN_StackTraceRawFrame raw_frame = {};
  raw_frame.process               = trace->process;
  for (DN_USize index = skip; index < trace->size; index++) {
    raw_frame.base_addr      = trace->base_addr[index];
    DN_StackTraceFrame frame = DN_StackTraceRawFrameToFrame(builder->arena, raw_frame);
    DN_Str8BuilderAppendF(builder, "%.*s(%zu): %.*s%s", DN_Str8PrintFmt(frame.file_name), frame.line_number, DN_Str8PrintFmt(frame.function_name), (DN_Cast(int) index == trace->size - 1) ? "" : "\n");
  }
}

DN_API bool DN_StackTraceIterate(DN_StackTraceIterator *it, DN_StackTrace const *trace)
{
  bool result = false;
  if (!it || !trace || !trace->base_addr || !trace->process)
    return result;

  if (it->index >= trace->size)
    return false;

  result                  = true;
  it->raw_frame.process   = trace->process;
  it->raw_frame.base_addr = trace->base_addr[it->index++];
  return result;
}

DN_API DN_Str8 DN_Str8FromStackTraceAllocator(DN_Allocator allocator, DN_StackTrace const *trace, DN_U16 skip)
{
  DN_Str8 result = {};
  if (!trace)
    return result;

  DN_TcScratch   scratch = DN_TcScratchBeginAllocator(&allocator, 1);
  DN_Str8Builder builder = DN_Str8BuilderFromArena(&scratch.arena);
  DN_StackTraceAddToStr8Builder_(trace, &builder, skip);
  result = DN_Str8FromStr8BuilderAllocator(&builder, allocator);
  DN_TcScratchEnd(&scratch);
  return result;
}

DN_API DN_Str8 DN_Str8FromStackTraceArena(DN_Arena *arena, DN_StackTrace const *trace, DN_U16 skip)
{
  DN_Str8 result = {};
  if (!trace || !arena)
    return result;

  DN_TcScratch   scratch = DN_TcScratchBeginArena(&arena, 1);
  DN_Str8Builder builder = DN_Str8BuilderFromArena(&scratch.arena);
  DN_StackTraceAddToStr8Builder_(trace, &builder, skip);
  result = DN_Str8FromStr8BuilderArena(&builder, arena);
  DN_TcScratchEnd(&scratch);
  return result;
}

DN_API DN_Str8 DN_Str8FromStackTraceNowAllocator(DN_Allocator allocator, DN_U16 limit, DN_U16 skip)
{
  DN_TcScratch  scratch = DN_TcScratchBeginArena(DN_Cast(DN_Arena **) & allocator.context, 1);
  DN_StackTrace walk    = DN_StackTraceFromArena(&scratch.arena, limit);
  DN_Str8       result  = DN_Str8FromStackTraceAllocator(allocator, &walk, skip);
  DN_TcScratchEnd(&scratch);
  return result;
}

DN_API DN_Str8 DN_Str8FromStackTraceNowArena(DN_Arena *arena, DN_U16 limit, DN_U16 skip)
{
  DN_Str8 result = DN_Str8FromStackTraceNowAllocator(DN_AllocatorFromArena(arena), limit, skip);
  return result;
}

DN_API DN_Str8 DN_Str8FromStackTraceNowHeap(DN_U16 limit, DN_U16 skip)
{
  DN_Arena       arena   = DN_ArenaFromHeap(DN_Kilobytes(64), DN_Kilobytes(64), DN_MemFlags_NoAllocTrack, DN_OS_HeapInitBasic());
  DN_Str8Builder builder = DN_Str8BuilderFromArena(&arena);
  DN_StackTrace  walk    = DN_StackTraceFromArena(&arena, limit);
  DN_StackTraceAddToStr8Builder_(&walk, &builder, skip);
  DN_Str8 result = DN_OS_Str8FromStr8BuilderHeap(&builder);
  DN_ArenaDeinit(&arena);
  return result;
}

DN_API DN_StackTraceFrameSlice DN_StackTraceGetFrames(DN_Arena *arena, DN_U16 limit)
{
  DN_StackTraceFrameSlice result = {};
  if (!arena)
    return result;

  DN_TcScratch            scratch = DN_TcScratchBeginArena(&arena, 1);
  DN_StackTrace walk    = DN_StackTraceFromArena(&scratch.arena, limit);
  if (walk.size) {
    if (DN_ISliceAllocArena(&result, walk.size, DN_ZMem_No, arena)) {
      DN_USize slice_index = 0;
      for (DN_StackTraceIterator it = {}; DN_StackTraceIterate(&it, &walk);)
        result.data[slice_index++] = DN_StackTraceRawFrameToFrame(arena, it.raw_frame);
    }
  }
  DN_TcScratchEnd(&scratch);
  return result;
}

DN_API DN_StackTraceFrame DN_StackTraceRawFrameToFrame(DN_Arena *arena, DN_StackTraceRawFrame raw_frame)
{
#if defined(DN_OS_WIN32)
  // NOTE: Get line+filename

  // TODO: Why does zero-initialising this with `line = {};` cause
  // SymGetLineFromAddr64 function to fail once we are at
  // __scrt_commain_main_seh and hit BaseThreadInitThunk frame? The
  // line and file number are still valid in the result which we use, so,
  // we silently ignore this error.
  IMAGEHLP_LINEW64 line;
  line.SizeOfStruct       = sizeof(line);
  DWORD line_displacement = 0;
  if (!SymGetLineFromAddrW64(raw_frame.process, raw_frame.base_addr, &line_displacement, &line))
    line = {};

  // NOTE: Get function name

  alignas(SYMBOL_INFOW) char buffer[sizeof(SYMBOL_INFOW) + (MAX_SYM_NAME * sizeof(wchar_t))] = {};
  SYMBOL_INFOW              *symbol                                                          = DN_Cast(SYMBOL_INFOW *) buffer;
  symbol->SizeOfStruct                                                                       = sizeof(*symbol);
  symbol->MaxNameLen                                                                         = sizeof(buffer) - sizeof(*symbol);

  uint64_t symbol_displacement = 0; // Offset to the beginning of the symbol to the address
  SymFromAddrW(raw_frame.process, raw_frame.base_addr, &symbol_displacement, symbol);

  // NOTE: Construct result

  DN_Str16 file_name16     = DN_Str16FromPtr(line.FileName, DN_CStr16Count(line.FileName));
  DN_Str16 function_name16 = DN_Str16FromPtr(symbol->Name, symbol->NameLen);

  DN_StackTraceFrame result = {};
  result.address            = raw_frame.base_addr;
  result.line_number        = line.LineNumber;
  result.file_name          = DN_OS_W32Str16ToStr8(arena, file_name16);
  result.function_name      = DN_OS_W32Str16ToStr8(arena, function_name16);

  if (result.function_name.count == 0)
    result.function_name = DN_Str8Lit("<unknown function>");
  if (result.file_name.count == 0)
    result.file_name = DN_Str8Lit("<unknown file>");
#else
  DN_StackTraceFrame result = {};
#endif
  return result;
}

DN_API void DN_StackTracePrint(DN_U16 limit)
{
  DN_TcScratch            scratch     = DN_TcScratchBeginArena(nullptr, 0);
  DN_StackTraceFrameSlice stack_trace = DN_StackTraceGetFrames(&scratch.arena, limit);
  for (DN_ForItSize(it, DN_StackTraceFrame, stack_trace.data, stack_trace.count)) {
    DN_StackTraceFrame frame = *it.data;
    DN_OS_PrintErrLnF("%.*s(%I64u): %.*s", DN_Str8PrintFmt(frame.file_name), frame.line_number, DN_Str8PrintFmt(frame.function_name));
  }
  DN_TcScratchEnd(&scratch);
}

DN_API void DN_StackTraceReloadSymbols()
{
#if defined(DN_OS_WIN32)
  HANDLE process = GetCurrentProcess();
  SymRefreshModuleList(process);
#endif
}

#if defined(DN_PLATFORM_POSIX) || defined(DN_PLATFORM_EMSCRIPTEN)
  #include "OS/dn_os_posix.cpp"
#elif defined(DN_PLATFORM_WIN32)
  #include "OS/dn_os_w32.cpp"
#else
  #error Please define a platform e.g. 'DN_PLATFORM_WIN32' to enable the correct implementation for platform APIs
#endif
#endif // DN_WITH_OS

#if DN_WITH_NET
DN_Str8 DN_NET_Str8FromResponseState(DN_NETResponseState state)
{
  DN_Str8 result = {};
  switch (state) {
      case DN_NETResponseState_Nil:      result = DN_Str8Lit("Nil");       break;
      case DN_NETResponseState_Error:    result = DN_Str8Lit("Error");     break;
      case DN_NETResponseState_HTTP:     result = DN_Str8Lit("HTTP");      break;
      case DN_NETResponseState_WSOpen:   result = DN_Str8Lit("WS Open");   break;
      case DN_NETResponseState_WSText:   result = DN_Str8Lit("WS Text");   break;
      case DN_NETResponseState_WSBinary: result = DN_Str8Lit("WS Binary"); break;
      case DN_NETResponseState_WSClose:  result = DN_Str8Lit("WS Close");  break;
      case DN_NETResponseState_WSPing:   result = DN_Str8Lit("WS Ping");   break;
      case DN_NETResponseState_WSPong:   result = DN_Str8Lit("WS Pong");   break;
  }
  return result;
}

DN_NETRequest *DN_NET_RequestFromHandle(DN_NETRequestHandle handle)
{
  DN_NETRequest *ptr    = DN_Cast(DN_NETRequest *) handle.handle;
  DN_NETRequest *result = nullptr;
  if (ptr && ptr->gen == handle.gen)
    result = ptr;
  return result;
}

DN_NETRequestHandle DN_NET_HandleFromRequest(DN_NETRequest *request)
{
  DN_NETRequestHandle result = {};
  if (request) {
    result.handle = DN_Cast(DN_UPtr) request;
    result.gen    = request->gen;
  }
  return result;
}

bool DN_NET_ResponseHasFailed(DN_NETResponse const* resp)
{
  bool result = false;
  if (resp->type == DN_NETRequestType_HTTP)
    result = resp->state == DN_NETResponseState_Error || resp->http_status >= 400;
  else
    result = resp->state == DN_NETResponseState_Error;
  return result;
}

bool DN_NET_ResponseHasSucceeded(DN_NETResponse const* resp)
{
  bool result = !DN_NET_ResponseHasFailed(resp);
  return result;
}

bool DN_NET_ResponseIsReady(DN_NETResponse const* resp)
{
  bool result = resp && resp->state != DN_NETResponseState_Nil;
  return result;
}

DN_Str8 DN_NET_Str8DiagnosticFromResponse(DN_NETResponse const* resp, DN_Arena *arena)
{
  DN_TcScratch   scratch     = DN_TcScratchBeginArena(&arena, 1);
  DN_Str8Builder builder     = DN_Str8BuilderFromArena(&scratch.arena);
  DN_Str8BuilderAppendF(&builder, "Request (%s", resp->type == DN_NETRequestType_HTTP ? "HTTP" : "WS");
  if (resp->type == DN_NETRequestType_HTTP) {
    if (resp->http_status)
      DN_Str8BuilderAppendF(&builder, " %u", resp->http_status);
  }
  DN_Str8BuilderAppendF(&builder, ")");
  if (resp->body.count || resp->error_str8.count) {
    DN_Str8BuilderAppendRef(&builder, DN_Str8Lit(" reported: "));
    if (resp->body.count)
      DN_Str8BuilderAppendF(&builder, "%.*s", DN_Str8PrintFmt(resp->body));
    if (resp->error_str8.count)
      DN_Str8BuilderAppendF(&builder, "%s%.*s", resp->body.count ? ". " : "", DN_Str8PrintFmt(resp->error_str8));
  }
  DN_Str8 result = DN_Str8FromStr8BuilderArena(&builder, arena);
  DN_TcScratchEnd(&scratch);
  return result;
}

void DN_NET_BaseInit(DN_NETCore *net, char *base, DN_U64 base_size)
{
  net->base           = base;
  net->base_size      = base_size;
  net->mem            = DN_MemListFromBuffer(net->base, net->base_size, DN_MemFlags_Nil);
  net->arena          = DN_ArenaFromMemList(&net->mem);
  net->completion_sem = DN_OS_SemaphoreInit(0);
}

DN_NETRequestHandle DN_NET_SetupRequest(DN_NETRequest *request, DN_Str8 url, DN_Str8 method, DN_NETDoHTTPArgs const *args, DN_NETRequestType type)
{
  // NOTE: Setup request
  DN_Assert(request);
  if (request) {
    if (request->mem.curr)
      DN_AssertF(request->arena.mem == nullptr,
                 "DN_NET_RequestRecycle should be called on the request before putting it into the "
                 "free-list and reusing it. This is so that we centralise the one place that we "
                 "reinitialise the temp arena for the request into this codepath.");
    else
      request->mem = DN_MemListFromHeap(DN_Megabytes(1), DN_Kilobytes(1), DN_MemFlags_Nil, DN_OS_HeapInitVirtual());

    request->arena  = DN_ArenaTempBeginFromMemList(&request->mem);
    request->type   = type;
    request->gen    = DN_Max(request->gen + 1, 1);
    request->url    = DN_Str8FromStr8Arena(url, &request->arena);
    request->method = DN_Str8FromStr8Arena(DN_Str8TrimWhitespaceAround(method), &request->arena);

    if (args) {
      request->args.flags    = args->flags;
      request->args.username = DN_Str8FromStr8Arena(args->username, &request->arena);
      request->args.password = DN_Str8FromStr8Arena(args->password, &request->arena);
      if (type == DN_NETRequestType_HTTP)
        request->args.payload = DN_Str8FromStr8Arena(args->payload, &request->arena);

      request->args.headers = DN_ArenaNewArray(&request->arena, DN_Str8, args->headers_size, DN_ZMem_No);
      DN_Assert(request->args.headers);
      if (request->args.headers) {
        for (DN_ForItSize(it, DN_Str8, args->headers, args->headers_size))
          request->args.headers[it.index] = DN_Str8FromStr8Arena(*it.data, &request->arena);
        request->args.headers_size = args->headers_size;
      }
    }

    request->completion_sem       = DN_OS_SemaphoreInit(0);
    request->start_response_arena = DN_ArenaTempBeginFromArena(&request->arena);
  }

  DN_NETRequestHandle result = DN_NET_HandleFromRequest(request);
  request->response.request  = result;
  request->response.type     = request->type;
  return result;
}

void DN_NET_RequestRecycle(DN_NETRequest *request)
{
  DN_NETRequest resetter        = {};
  resetter.mem                  = request->mem;
  resetter.arena                = request->arena;
  resetter.start_response_arena = request->start_response_arena;
  resetter.gen                  = request->gen + 1;
  DN_Memcpy(resetter.context, request->context, sizeof(resetter.context));
  *request               = resetter;

  // NOTE: Deallocate the memory used in the request. Note we have to end the start response arena
  // first to satisfy the UAF checker which requires that the temporary memory arenas are ended in
  // the reverse order that they were created in.
  //
  // The arenas are created when `DN_NET_SetupRequest` is called to reuse the request.
  DN_ArenaTempEnd(&request->start_response_arena, DN_ArenaReset_Yes);
  DN_ArenaTempEnd(&request->arena, DN_ArenaReset_Yes);
  request->arena                = {};
  request->start_response_arena = {};
}
#endif // #if DN_WITH_NET

#if DN_WITH_NET_CURL
typedef struct DN_NETCurlRequest DN_NETCurlRequest;
struct DN_NETCurlRequest
{
  void              *handle;
  struct curl_slist *slist;
  char               error[CURL_ERROR_SIZE];
  bool               ws_has_more;
  DN_Str8Builder     str8_builder;
};

enum DN_NETCurlRingEventType
{
  DN_NETCurlRingEventType_Nil,
  DN_NETCurlRingEventType_DoRequest,
  DN_NETCurlRingEventType_SendWS,
  DN_NETCurlRingEventType_ReceivedWSReceipt,
  DN_NETCurlRingEventType_DeinitRequest,
};

typedef struct DN_NETCurlRingEvent_ DN_NETCurlRingEvent_;
struct DN_NETCurlRingEvent_
{
  DN_NETCurlRingEventType type;
  DN_NETRequestHandle     request;
  DN_USize                ws_send_size;
  DN_NETWSSend            ws_send;
};

static DN_NETCurlRequest *DN_NET_CurlRequestFromRequest_(DN_NETRequest *req)
{
  DN_NETCurlRequest *result = req ? DN_Cast(DN_NETCurlRequest *) req->context[0] : 0;
  return result;
}

static DN_NETCore *DN_NET_CurlNetFromRequest(DN_NETRequest *req)
{
  DN_NETCore *result = req ? DN_Cast(DN_NETCore *) req->context[1] : 0;
  return result;
}

static bool DN_NET_CurlRequestIsInList(DN_NETRequest const *first, DN_NETRequest const *find)
{
  bool result = false;
  for (DN_NETRequest const *it = first; !result && it; it = it->next)
    result = find == it;
  return result;
}

static void DN_NET_CurlMarkRequestDone_(DN_NETCore *net, DN_NETRequest *request)
{
  DN_Assert(request);
  DN_Assert(net);
  // NOTE: The done list in CURL is also used as a place to put websocket requests after removing it
  // from the 'ws_list'. By doing this we are stopping the CURL thread from receiving more data on
  // the socket as that thread ticks the list of 'ws_list' sockets for data.
  //
  // Once the caller waited and has received the data from the websocket, the request is put back
  // into the 'ws_list' which then lets the CURL thread start receiving more data for that socket.
  //
  // Since CURL uses a background thread, we do this behind a mutex
  DN_NETCurlCore *curl = DN_Cast(DN_NETCurlCore *)net->context;
  for (DN_OS_MutexScope(&curl->list_mutex)) {
    DN_Assert(DN_NET_CurlRequestIsInList(curl->thread_request_list, request));

    DN_DoublyLLDetach(curl->thread_request_list, request);
    DN_Assert(curl->thread_request_count);
    curl->thread_request_count--;

    DN_DoublyLLAppend(curl->response_list, request);
    curl->response_count++;
  }
  DN_OS_SemaphoreIncrement(&net->completion_sem, 1);
  DN_OS_SemaphoreIncrement(&request->completion_sem, 1);
}

static DN_USize DN_NET_CurlHTTPCallback_(char *payload, DN_USize size, DN_USize count, void *user_data)
{
  DN_NETRequest     *req          = DN_Cast(DN_NETRequest *) user_data;
  DN_NETCurlRequest *curl_req     = DN_NET_CurlRequestFromRequest_(req);
  DN_USize           result       = 0;
  DN_USize           payload_size = size * count;
  if (DN_Str8BuilderAppendBytesCopy(&curl_req->str8_builder, payload, payload_size))
    result = payload_size;
  return result;
}

static int32_t DN_NET_CurlThreadEntryPoint_(DN_OSThread *thread)
{
  DN_NETCore     *net  = DN_Cast(DN_NETCore *) thread->user_context;
  DN_NETCurlCore *curl = DN_Cast(DN_NETCurlCore *) net->context;
  DN_OS_ThreadSetNameFmt("%.*s", DN_Str8PrintFmt(curl->thread.name));

  while (!curl->kill_thread) {
    DN_TcScratch tmem = DN_TcScratchBeginArena(nullptr, 0);

    // NOTE: Handle events sitting in the ring queue
    for (bool dequeue_ring = true; dequeue_ring;) {
      DN_NETCurlRingEvent_ event = {};
      for (DN_OS_MutexScope(&curl->ring_mutex)) {
        if (DN_RingHasData(&curl->ring, sizeof(event)))
            DN_RingRead(&curl->ring, &event, sizeof(event));
      }

      switch (event.type) {
        case DN_NETCurlRingEventType_Nil: dequeue_ring = false; break;

        case DN_NETCurlRingEventType_DoRequest: {
          DN_NETRequest     *req      = DN_NET_RequestFromHandle(event.request);
          DN_NETCurlRequest *curl_req = DN_NET_CurlRequestFromRequest_(req);
          DN_Assert(req->response.state == DN_NETResponseState_Nil);
          DN_Assert(req->type           != DN_NETRequestType_Nil);

          // NOTE: Attach it to the CURL thread's request list
          for (DN_OS_MutexScope(&curl->list_mutex)) {
            DN_Assert(DN_NET_CurlRequestIsInList(curl->request_list, req));
            DN_DoublyLLDetach(curl->request_list, req);
            DN_Assert(curl->request_count);
            curl->request_count--;
          }
          DN_DoublyLLAppend(curl->thread_request_list, req);
          curl->thread_request_count++;

          // NOTE: Add the connection to CURLM and start ticking it once we finish handling all the
          // ring events
          CURLMcode multi_add = curl_multi_add_handle(curl->thread_curlm, curl_req->handle);
          DN_Assert(multi_add == CURLM_OK);
        } break;

        case DN_NETCurlRingEventType_SendWS: {
          DN_NETRequest     *req      = DN_NET_RequestFromHandle(event.request);
          DN_NETCurlRequest *curl_req = DN_NET_CurlRequestFromRequest_(req);
          DN_Str8 payload = {};
          for (DN_OS_MutexScope(&curl->ring_mutex)) {
            DN_Assert(DN_RingHasData(&curl->ring, event.ws_send_size));
            payload = DN_Str8AllocArena(event.ws_send_size, DN_ZMem_No, &tmem.arena);
            DN_RingRead(&curl->ring, payload.data, payload.count);
          }

          DN_U32 curlws_flag = 0;
          switch (event.ws_send) {
            case DN_NETWSSend_Text:   curlws_flag = CURLWS_TEXT; break;
            case DN_NETWSSend_Binary: curlws_flag = CURLWS_BINARY; break;
            case DN_NETWSSend_Close:  curlws_flag = CURLWS_CLOSE; break;
            case DN_NETWSSend_Ping:   curlws_flag = CURLWS_PING; break;
            case DN_NETWSSend_Pong:   curlws_flag = CURLWS_PONG; break;
          }

          DN_Assert(req->type           == DN_NETRequestType_WS);
          DN_Assert(req->response.state >= DN_NETResponseState_WSOpen && req->response.state <= DN_NETResponseState_WSPong);

          DN_USize sent           = 0;
          CURLcode send_result    = curl_ws_send(curl_req->handle, payload.data, payload.count, &sent, 0, curlws_flag);
          DN_AssertF(send_result == CURLE_OK, "Failed to send: %s", curl_easy_strerror(send_result));
          DN_AssertF(sent        == payload.count, "Failed to send all bytes (%zu vs %zu)", sent, payload.count);
        } break;

        case DN_NETCurlRingEventType_ReceivedWSReceipt: {
          DN_NETRequest     *req      = DN_NET_RequestFromHandle(event.request);
          DN_NETCurlRequest *curl_req = DN_NET_CurlRequestFromRequest_(req);
          DN_Assert(req->type == DN_NETRequestType_WS);
          DN_Assert(req->response.state >= DN_NETResponseState_WSOpen && req->response.state <= DN_NETResponseState_WSPong);
          req->response.state = DN_NETResponseState_WSOpen;

          // NOTE: End the temp memory storing the WS data we just read and the user returned to us
          // (we got their receipt back). Then restart the temp memory scope for the next websocket
          // payload
          DN_ArenaTempEnd(&req->start_response_arena, DN_ArenaReset_Yes);
          req->start_response_arena = DN_ArenaTempBeginFromArena(&req->arena);
          curl_req->str8_builder    = DN_Str8BuilderFromArena(&req->start_response_arena);

          for (DN_OS_MutexScope(&curl->list_mutex)) {
            DN_Assert(DN_NET_CurlRequestIsInList(curl->request_list, req));
            DN_DoublyLLDetach(curl->request_list, req);
            DN_Assert(curl->request_count);
            curl->request_count--;
          }
          DN_DoublyLLAppend(curl->thread_request_list, req);
          curl->thread_request_count++;
        } break;

        case DN_NETCurlRingEventType_DeinitRequest: {
          DN_NETRequest     *req      = DN_NET_RequestFromHandle(event.request);
          DN_NETCurlRequest *curl_req = DN_NET_CurlRequestFromRequest_(req);

          DN_Assert(event.request.handle != 0);
          DN_NETRequest *request = DN_Cast(DN_NETRequest *) event.request.handle;

          // NOTE: Detach the request from the deinit list. This brings the request into this
          // thread's provenance, no other threads modifying the deinit list will race with us.
          for (DN_OS_MutexScope(&curl->list_mutex)) {
            DN_Assert(DN_NET_CurlRequestIsInList(curl->deinit_list, request));
            DN_DoublyLLDetach(curl->deinit_list, request);
            DN_Assert(curl->deinit_count);
            curl->deinit_count--;
          }

          // NOTE: Now we can modify the request, release resources
          DN_OS_SemaphoreDeinit(&request->completion_sem);

          curl_multi_remove_handle(curl->thread_curlm, curl_req->handle);
          curl_slist_free_all(curl_req->slist);
          curl_easy_reset(curl_req->handle);

          CURL *copy       = curl_req->handle;
          *curl_req        = {};
          curl_req->handle = copy;

          // NOTE: Zero the struct preserving just the data we need to retain
          DN_NET_RequestRecycle(request);

          // NOTE: Add it to the free list
          for (DN_OS_MutexScope(&curl->list_mutex)) {
            DN_DoublyLLAppend(curl->free_list, request);
            curl->free_count++;
          }
        } break;
      }
    }

    // NOTE: Pump handles
    int       running_handles = 0;
    CURLMcode perform_result  = curl_multi_perform(curl->thread_curlm, &running_handles);
    if (perform_result != CURLM_OK)
      DN_AssertInvalidCodePath;

    // NOTE: Check pump result
    for (;;) {
      int      msgs_in_queue = 0;
      CURLMsg *msg           = curl_multi_info_read(curl->thread_curlm, &msgs_in_queue);
      if (msg) {
        // NOTE: Get request handle
        DN_NETRequest *req = nullptr;
        curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, DN_Cast(void **) & req);
        DN_Assert(req);
        DN_Assert(DN_NET_CurlRequestIsInList(curl->thread_request_list, req));

        DN_NETCurlRequest *curl_req = DN_NET_CurlRequestFromRequest_(req);
        DN_Assert(curl_req->handle == msg->easy_handle);

        if (msg->data.result == CURLE_OK) {
          // NOTE: Get HTTP response code
          CURLcode get_result = curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &req->response.http_status);
          if (get_result == CURLE_OK) {
            if (req->type == DN_NETRequestType_HTTP) {
              req->response.state = DN_NETResponseState_HTTP;
            } else {
              DN_Assert(req->type == DN_NETRequestType_WS);
              req->response.state = DN_NETResponseState_WSOpen;
            }
          } else {
            req->response.error_str8 = DN_Str8FmtArena(&req->start_response_arena, "Failed to get HTTP response status (CURL %d): %s", msg->data.result, curl_easy_strerror(get_result));
            req->response.state      = DN_NETResponseState_Error;
          }
        } else {
          DN_USize curl_extended_error_size = DN_CStr8Count(curl_req->error);
          req->response.state               = DN_NETResponseState_Error;
          req->response.error_str8          = DN_Str8FmtArena(&req->start_response_arena,
                                                         "HTTP request '%.*s' failed (CURL %d): %s%s%s%s",
                                                         DN_Str8PrintFmt(req->url),
                                                         msg->data.result,
                                                         curl_easy_strerror(msg->data.result),
                                                         curl_extended_error_size ? " (" : "",
                                                         curl_extended_error_size ? curl_req->error : "",
                                                         curl_extended_error_size ? ")" : "");
        }

        if (req->type == DN_NETRequestType_HTTP || req->response.state == DN_NETResponseState_Error) {
          // NOTE: Remove the request from the multi handle if we're a HTTP request
          // because it typically terminates the connection. In websockets the
          // connection remains in the multi-handle to allow you to send and
          // receive WS data from it.
          //
          // If there's an error (either websocket or HTTP) we will also remove the
          // connection from the multi handle as it failed. One a connection has
          // failed, curl will not poll that connection so there's no point keeping
          // it attached to the multi handle.
          curl_multi_remove_handle(curl->thread_curlm, msg->easy_handle);
        }

        DN_NET_CurlMarkRequestDone_(net, req);
      }

      if (msgs_in_queue == 0)
        break;
    }

    // NOTE: Check websockets
    DN_USize ws_count = 0;
    for (DN_NETRequest *req = curl->thread_request_list; req; req = req->next) {
      DN_Assert(req->type == DN_NETRequestType_WS || req->type == DN_NETRequestType_HTTP);
      if (req->type != DN_NETRequestType_WS || !(req->response.state >= DN_NETResponseState_WSOpen && req->response.state <= DN_NETResponseState_WSPong))
        continue;
      ws_count++;
      const curl_ws_frame *meta           = nullptr;
      DN_NETCurlRequest   *curl_req       = DN_NET_CurlRequestFromRequest_(req);
      CURLcode             receive_result = CURLE_OK;
      while (receive_result == CURLE_OK) {
        // NOTE: Determine WS payload size received. Note that since we pass in a null pointer CURL
        // will set meta->len to 0 and say that there's meta->bytesleft in the next chunk.
        DN_USize bytes_read = 0;
        receive_result      = curl_ws_recv(curl_req->handle, nullptr, 0, &bytes_read, &meta);
        if (receive_result != CURLE_OK)
          continue;
        DN_Assert(meta->len == 0);

        if (meta->flags & CURLWS_TEXT)
          req->response.state = DN_NETResponseState_WSText;

        if (meta->flags & CURLWS_BINARY)
          req->response.state = DN_NETResponseState_WSBinary;

        if (meta->flags & CURLWS_PING)
          req->response.state = DN_NETResponseState_WSPing;

        if (meta->flags & CURLWS_PONG)
          req->response.state = DN_NETResponseState_WSPong;

        if (meta->flags & CURLWS_CLOSE)
          req->response.state = DN_NETResponseState_WSClose;

        curl_req->ws_has_more = meta->flags & CURLWS_CONT;
        if (curl_req->ws_has_more) {
          bool is_text_or_binary = req->response.state == DN_NETResponseState_WSText ||
                                   req->response.state == DN_NETResponseState_WSBinary;
          DN_Assert(is_text_or_binary);
        }

        // NOTE: Allocate and read (we use meta->bytesleft as per comment from initial recv)
        if (meta->bytesleft) {
          DN_Str8 buffer  = DN_Str8AllocArena(meta->bytesleft, DN_ZMem_No, &req->start_response_arena);
          DN_Assert(buffer.count == DN_Cast(DN_USize)meta->bytesleft);
          receive_result  = curl_ws_recv(curl_req->handle, buffer.data, buffer.count, &buffer.count, &meta);
          DN_Assert(buffer.count == DN_Cast(DN_USize)meta->len);
          DN_Str8BuilderAppendRef(&curl_req->str8_builder, buffer);
        }

        // NOTE: There are more bytes coming if meta->bytesleft is set, (e.g. the next chunk. We
        // just read the current chunk).
        //
        //   > If this is not a complete fragment, the bytesleft field informs about how many
        //     additional bytes are expected to arrive before this fragment is complete.
        curl_req->ws_has_more |= meta && meta->bytesleft > 0;
        if (!curl_req->ws_has_more)
          break;
      }

      // NOTE: curl_ws_recv returns CURLE_GOT_NOTHING if the associated connection is closed.
      if (receive_result == CURLE_GOT_NOTHING)
        curl_req->ws_has_more = false;

      // NOTE: We read all the possible bytes that CURL has received for this message, but, there are
      // more bytes left that we will receive on subsequent calls. We will continue to the next
      // request and return back to this one when PumpRequests is called again where hopefully that
      // data has arrived.
      if (curl_req->ws_has_more)
        continue;

      // For CURLE_AGAIN
      //
      //   > Instead of blocking, the function returns CURLE_AGAIN. The correct behavior is then to
      //   > wait for the socket to signal readability before calling this function again.
      //
      // In which case we continue ticking the other sockets and eventually exit once all ticked.
      // Right after this we wait on the CURLM instance which will wake us up again when there's
      // data to be read.
      //
      // if we received data, e.g. state was set to Text, Binary ... e.t.c we bypass this and
      // report it to the user first. When the user waits for the response, they consume the data
      // and then that will reinsert it into request list for CURL to read from the socket again.
      bool received_data = (req->response.state >= DN_NETResponseState_WSText && req->response.state <= DN_NETResponseState_WSPong);
      if (receive_result == CURLE_AGAIN && !received_data)
        continue;

      if (!received_data) {
        if (receive_result == CURLE_GOT_NOTHING) {
          req->response.state = DN_NETResponseState_WSClose;
        } else if (receive_result != CURLE_OK) {
          DN_USize curl_extended_error_size = DN_CStr8Count(curl_req->error);
          req->response.state           = DN_NETResponseState_Error;
          req->response.error_str8      = DN_Str8FmtArena(&req->start_response_arena,
                                                                  "Websocket receive '%.*s' failed (CURL %d): %s%s%s%s",
                                                                  DN_Str8PrintFmt(req->url),
                                                                  receive_result,
                                                                  curl_easy_strerror(receive_result),
                                                                  curl_extended_error_size ? " (" : "",
                                                                  curl_extended_error_size ? curl_req->error : "",
                                                                  curl_extended_error_size ? ")" : "");
        }
      }

      DN_NETRequest *request_copy = req;
      req                         = req->prev;
      DN_NET_CurlMarkRequestDone_(net, request_copy);
      if (!req)
        break;
    }

    DN_I32 sleep_time_ms = ws_count > 0 ? 16 : INT32_MAX;
    curl_multi_poll(curl->thread_curlm, nullptr, 0, sleep_time_ms, nullptr);
    DN_TcScratchEnd(&tmem);
  }

  return 0;
}

DN_NETInterface DN_NET_CurlInterface()
{
  DN_NETInterface result      = {};
  result.init                  = DN_NET_CurlInit;
  result.deinit                = DN_NET_CurlDeinit;
  result.do_http               = DN_NET_CurlDoHTTP;
  result.do_ws                 = DN_NET_CurlDoWS;
  result.do_ws_send            = DN_NET_CurlDoWSSend;
  result.wait_for_response     = DN_NET_CurlWaitForResponse;
  result.wait_for_any_response = DN_NET_CurlWaitForAnyResponse;
  return result;
}

void DN_NET_CurlInit(DN_NETCore *net, char *base, DN_U64 base_size)
{
  DN_NET_BaseInit(net, base, base_size);
  DN_NETCurlCore *curl = DN_ArenaNew(&net->arena, DN_NETCurlCore, DN_ZMem_Yes);
  net->context         = curl;
  net->api             = DN_NET_CurlInterface();

  DN_USize arena_bytes_avail = (net->arena.mem->curr->reserve - net->arena.mem->curr->used);
  curl->ring.size            = arena_bytes_avail / 2;
  curl->ring.base            = DN_Cast(char *) DN_ArenaAlloc(&net->arena, curl->ring.size, /*align*/ 1, DN_ZMem_Yes);
  DN_Assert(curl->ring.base);

  curl->ring_mutex   = DN_OS_MutexInit();
  curl->list_mutex   = DN_OS_MutexInit();
  curl->thread_curlm = DN_Cast(CURLM *) curl_multi_init();

  DN_FmtAppend(curl->thread.name.data, &curl->thread.name.count, sizeof(curl->thread.name.data), "NET (CURL)");
  DN_OS_ThreadInit(&curl->thread, DN_NET_CurlThreadEntryPoint_, DN_OS_ThreadInitArgsDefault(), net);
}

void DN_NET_CurlDeinit(DN_NETCore *net)
{
  DN_NETCurlCore *curl = DN_Cast(DN_NETCurlCore *) net->context;
  curl->kill_thread    = true;
  curl_multi_wakeup(curl->thread_curlm);
  DN_OS_ThreadJoin(&curl->thread, UINT32_MAX, DN_TcDeinitArenas_Yes);
}

static DN_NETRequestHandle DN_NET_CurlDoRequest_(DN_NETCore *net, DN_Str8 url, DN_Str8 method, DN_NETDoHTTPArgs const *args, DN_NETRequestType type)
{
  // NOTE: Allocate the request
  DN_NETCurlCore     *curl_core = DN_Cast(DN_NETCurlCore *) net->context;
  DN_NETRequest      *req       = nullptr;
  DN_NETRequestHandle result    = {};
  {
    // NOTE: The free list is modified by both the calling thread and the CURLM thread (which ticks
    // all the requests in the background for us)
    for (DN_OS_MutexScope(&curl_core->list_mutex)) {
      req = curl_core->free_list;
      DN_DoublyLLDetach(curl_core->free_list, req);
      if (req) {
        DN_Assert(curl_core->free_count);
        curl_core->free_count--;
      }
    }

    if (req) {
      DN_AssertF(req->mem.curr->used == DN_ARENA_HEADER_SIZE,
                "A reused request from the free-list should have its memory reset essentially to "
                "zero. If this isn't the case then we're leaking memory and have forgotten to "
                "reset the arena before putting the request back into the free list. Request has "
                "used %s.", DN_Str8x32FromByteCountU64Auto(req->mem.curr->used).data);
    } else{
      // NOTE: None in the free list so allocate one
      DN_OS_MutexLock(&curl_core->list_mutex);
      DN_U64 arena_pos            = DN_MemListPos(net->arena.mem);
      req                         = DN_ArenaNewZ(&net->arena, DN_NETRequest);
      DN_NETCurlRequest *curl_req = DN_ArenaNewZ(&net->arena, DN_NETCurlRequest);
      if (!req || !curl_req) {
        DN_MemListPopTo(net->arena.mem, arena_pos);
        DN_OS_MutexUnlock(&curl_core->list_mutex);
        return result;
      }
      DN_OS_MutexUnlock(&curl_core->list_mutex);

      curl_req->handle = DN_Cast(CURL *) curl_easy_init();
      req->context[0]  = DN_Cast(DN_UPtr) curl_req;
    }
  }

  // NOTE: Setup the request
  DN_NETCurlRequest *curl_req = DN_NET_CurlRequestFromRequest_(req);
  {
    result                 = DN_NET_SetupRequest(req, url, method, args, type);
    req->context[1]        = DN_Cast(DN_UPtr) net;
    curl_req->str8_builder = DN_Str8BuilderFromArena(&req->start_response_arena);
  }

  // NOTE: Setup the request for curl API
  {
    CURL *curl = curl_req->handle;
    curl_easy_setopt(curl, CURLOPT_PRIVATE, req);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_req->error);

    // NOTE: Perform request and read all response headers before handing
    // control back to app.
    curl_easy_setopt(curl, CURLOPT_URL, req->url.data);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

    // NOTE: Setup response handler
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, DN_NET_CurlHTTPCallback_);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, req);

    // NOTE: Assign HTTP headers
    for (DN_ForItSize(it, DN_Str8, req->args.headers, req->args.headers_size)) {
      DN_Assert(it.data->data[it.data->count] == 0);
      curl_req->slist = curl_slist_append(curl_req->slist, it.data->data);
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_req->slist);

    // NOTE: Setup handle for protocol
    switch (req->type) {
      case DN_NETRequestType_Nil: DN_AssertInvalidCodePath; break;

      case DN_NETRequestType_WS: {
        curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 2L);
      } break;

      case DN_NETRequestType_HTTP: {
        DN_Str8 const GET  = DN_Str8Lit("GET");
        DN_Str8 const POST = DN_Str8Lit("POST");

        if (DN_Str8EqInsensitive(req->method, GET)) {
          curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
        } else if (DN_Str8EqInsensitive(req->method, POST)) {
          curl_easy_setopt(curl, CURLOPT_POST, 1);
          if (req->args.payload.count > DN_Gigabytes(2))
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, req->args.payload.count);
          else
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, req->args.payload.count);
          curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, req->args.payload.data);
        } else {
          DN_AssertInvalidCodePathF("Unimplemented");
        }
      } break;
    }

    // NOTE: Handle basic auth
    if (req->args.flags & DN_NETDoHTTPFlags_BasicAuth) {
      if (req->args.username.count && req->args.password.count) {
        DN_Assert(req->args.username.data[req->args.username.count] == 0);
        DN_Assert(req->args.password.data[req->args.password.count] == 0);
        curl_easy_setopt(curl, CURLOPT_USERNAME, req->args.username.data);
        curl_easy_setopt(curl, CURLOPT_PASSWORD, req->args.password.data);
      }
    }

    if (req->args.flags & DN_NETDoHTTPFlags_DisableSSLVerify) {
      // NOTE: Disable peer verification (checks if cert is signed by trusted CA)
      // NOTE: Disable host verification (checks if cert matches hostname)
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    }
  }

  // NOTE: Dispatch the request to the CURL thread
  {
    // NOTE: Immediately add the request to the request list so it happens "atomically" in the
    // calling thread. If the calling thread deinitialises this layer before the CURL thread can be
    // pre-empted, we can lose track of this request.
    for (DN_OS_MutexScope(&curl_core->list_mutex)) {
      DN_DoublyLLAppend(curl_core->request_list, req);
      curl_core->request_count++;
    }

    // NOTE: Enqueue request to go into CURL's ring queue. The CURL thread will sleep and wait for
    // bytes to come in for the request and then dump the response into the done list to be consumed
    // via wait for response
    DN_NETCurlRingEvent_ event = {};
    event.type                 = DN_NETCurlRingEventType_DoRequest;
    event.request              = result;
    for (DN_OS_MutexScope(&curl_core->ring_mutex))
      DN_RingWriteStruct(&curl_core->ring, &event);

    curl_multi_wakeup(curl_core->thread_curlm);
  }

  return result;
}

DN_NETRequestHandle DN_NET_CurlDoHTTP(DN_NETCore *net, DN_Str8 url, DN_Str8 method, DN_NETDoHTTPArgs const *args)
{
  DN_NETRequestHandle result = DN_NET_CurlDoRequest_(net, url, method, args, DN_NETRequestType_HTTP);
  return result;
}

DN_NETRequestHandle DN_NET_CurlDoWSArgs(DN_NETCore *net, DN_Str8 url, DN_NETDoHTTPArgs const *args)
{
  DN_NETRequestHandle result = DN_NET_CurlDoRequest_(net, url, DN_Str8Lit(""), args, DN_NETRequestType_WS);
  return result;
}

DN_NETRequestHandle DN_NET_CurlDoWS(DN_NETCore *net, DN_Str8 url)
{
  DN_NETRequestHandle result = DN_NET_CurlDoWSArgs(net, url, nullptr);
  return result;
}

void DN_NET_CurlDoWSSend(DN_NETRequestHandle handle, DN_Str8 payload, DN_NETWSSend send)
{
  DN_NETRequest *req = DN_NET_RequestFromHandle(handle);
  if (!req)
    return;

  DN_NETCore     *net  = DN_NET_CurlNetFromRequest(req);
  DN_NETCurlCore *curl = DN_Cast(DN_NETCurlCore *) net->context;
  DN_Assert(curl);

  DN_NETCurlRingEvent_ event = {};
  event.type                 = DN_NETCurlRingEventType_SendWS;
  event.request              = handle;
  event.ws_send_size         = payload.count;
  event.ws_send              = send;

  for (DN_OS_MutexScope(&curl->ring_mutex)) {
    DN_Assert(DN_RingHasSpace(&curl->ring, payload.count));
    DN_RingWriteStruct(&curl->ring, &event);
    DN_RingWrite(&curl->ring, payload.data, payload.count);
  }
  curl_multi_wakeup(curl->thread_curlm);
}

static DN_NETResponse DN_NET_CurlHandleFinishedRequest_(DN_NETCurlCore *curl, DN_NETRequest *req, DN_Arena *arena)
{
  // NOTE: Generate the response, copy out the strings into the user given memory
  DN_NETResponse     result   = req->response;
  DN_NETCurlRequest *curl_req = DN_NET_CurlRequestFromRequest_(req);
  {
    result.body = DN_Str8FromStr8BuilderArena(&curl_req->str8_builder, arena);
    if (result.error_str8.count)
      result.error_str8 = DN_Str8FromStr8Arena(result.error_str8, arena);
  }

  bool continue_ws_request = false;
  if (req->type == DN_NETRequestType_WS &&
      req->response.state != DN_NETResponseState_Error &&
      req->response.state != DN_NETResponseState_WSClose) {
    continue_ws_request = true;
  }

  // NOTE: Put the request into the requisite list
  for (DN_OS_MutexScope(&curl->list_mutex)) {
    // NOTE: Dequeue the request, it _must_ have been in the response list at this point for it to
    // have ben waitable in the first place.
    DN_AssertF(DN_NET_CurlRequestIsInList(curl->response_list, req),
               "A completed response should only signal the completion semaphore when it's in the response list");
    DN_DoublyLLDetach(curl->response_list, req);
    DN_Assert(curl->response_count);
    curl->response_count--;

    // NOTE: A websocket that is continuing to get data should go back into the request list because
    // there's more data to be received. All other requests need to go into the deinit list (so that
    // we keep track of it in the time inbetween it takes for the CURL thread to be scheduled and
    // release the CURL handle from CURLM and release resources e.t.c.)
    if (continue_ws_request) {
      DN_DoublyLLAppend(curl->request_list, req);
      curl->request_count++;
    } else {
      DN_DoublyLLAppend(curl->deinit_list, req);
      curl->deinit_count++;
    }
  }

  // NOTE: Submit the post-request event to the CURL thread
  DN_NETCurlRingEvent_ event = {};
  event.request              = DN_NET_HandleFromRequest(req);
  if (continue_ws_request) {
    event.type = DN_NETCurlRingEventType_ReceivedWSReceipt;
  } else {
    // NOTE: Deinit _has_ to be sent to the CURL thread because we need to remove the CURL handle
    // from the CURLM instance and the CURL thread uses the CURLM instance (e.g. CURLM is not thread
    // safe)
    event.type = DN_NETCurlRingEventType_DeinitRequest;
  }

  for (DN_OS_MutexScope(&curl->ring_mutex))
    DN_RingWriteStruct(&curl->ring, &event);
  curl_multi_wakeup(curl->thread_curlm);

  return result;
}

DN_NETResponse DN_NET_CurlWaitForResponse(DN_NETRequestHandle handle, DN_Arena *arena, DN_U32 timeout_ms)
{
  DN_NETResponse result = {};
  DN_NETRequest *req    = DN_NET_RequestFromHandle(handle);
  if (!req)
    return result;

  DN_NETCore     *net  = DN_Cast(DN_NETCore *) req->context[1];
  DN_NETCurlCore *curl = DN_Cast(DN_NETCurlCore *) net->context;
  DN_Assert(curl);

  DN_OSSemaphoreWaitResult wait = DN_OS_SemaphoreWait(&req->completion_sem, timeout_ms);
  if (wait != DN_OSSemaphoreWaitResult_Success)
    return result;

  // NOTE: Decrement the global 'request done' completion semaphore since the user consumed the
  // request individually.
  DN_OSSemaphoreWaitResult net_wait_result = DN_OS_SemaphoreWait(&net->completion_sem, 0 /*timeout_ms*/);
  DN_AssertF(net_wait_result == DN_OSSemaphoreWaitResult_Success, "Wait result was: %zu", DN_Cast(DN_USize) net_wait_result);

  // NOTE: Finish handling the response
  result = DN_NET_CurlHandleFinishedRequest_(curl, req, arena);
  return result;
}

DN_NETResponse DN_NET_CurlWaitForAnyResponse(DN_NETCore *net, DN_Arena *arena, DN_U32 timeout_ms)
{
  DN_NETCurlCore *curl = DN_Cast(DN_NETCurlCore *) net->context;
  DN_Assert(curl);

  DN_NETResponse           result   = {};
  DN_OSSemaphoreWaitResult req_wait = DN_OS_SemaphoreWait(&net->completion_sem, timeout_ms);
  if (req_wait != DN_OSSemaphoreWaitResult_Success)
    return result;

  // NOTE: Just grab the handle, handle finished request will dequeue for us
  DN_NETRequestHandle handle = {};
  for (DN_OS_MutexScope(&curl->list_mutex)) {
    DN_Assert(curl->response_list);
    handle = DN_NET_HandleFromRequest(curl->response_list);
  }

  // NOTE: Decrement the request's completion semaphore since the user consumed the global semaphore
  DN_NETRequest           *req      = DN_NET_RequestFromHandle(handle);
  DN_OSSemaphoreWaitResult net_wait = DN_OS_SemaphoreWait(&req->completion_sem, 0 /*timeout_ms*/);
  DN_AssertF(net_wait == DN_OSSemaphoreWaitResult_Success, "Wait result was: %zu", DN_Cast(DN_USize) net_wait);

  // NOTE: Finish handling the response
  result = DN_NET_CurlHandleFinishedRequest_(curl, req, arena);
  return result;
}
#endif // #if DN_WITH_NET_CURL

#if DN_WITH_NET_EMSCRIPTEN
#include <emscripten.h>
#include <emscripten/fetch.h>
#include <emscripten/websocket.h>

typedef struct DN_NETEmcWSEvent DN_NETEmcWSEvent;
struct DN_NETEmcWSEvent
{
  DN_NETResponseState state;
  DN_Str8             payload;
  DN_NETEmcWSEvent   *next;
};

typedef struct DN_NETEmcCore DN_NETEmcCore;
struct DN_NETEmcCore
{
  DN_Pool        pool;
  DN_NETRequest *response_list; // Responses received that are to be deqeued via wait for response
  DN_NETRequest *free_list;     // Request pool that new requests will use before allocating
};

typedef struct DN_NETEmcRequest DN_NETEmcRequest;
struct DN_NETEmcRequest
{
  int               socket;
  DN_NETEmcWSEvent *first_event;
  DN_NETEmcWSEvent *last_event;
};

DN_NETInterface DN_NET_EmcInterface()
{
  DN_NETInterface result      = {};
  result.init                  = DN_NET_EmcInit;
  result.deinit                = DN_NET_EmcDeinit;
  result.do_http               = DN_NET_EmcDoHTTP;
  result.do_ws                 = DN_NET_EmcDoWS;
  result.do_ws_send            = DN_NET_EmcDoWSSend;
  result.wait_for_response     = DN_NET_EmcWaitForResponse;
  result.wait_for_any_response = DN_NET_EmcWaitForAnyResponse;
  return result;
}

static DN_NETEmcWSEvent *DN_NET_EmcAllocWSEvent_(DN_NETRequest *request)
{
  // NOTE: Allocate the event and attach to the request
  DN_NETEmcRequest *emc_request = DN_Cast(DN_NETEmcRequest *) request->context[1];
  DN_NETEmcWSEvent *result      = DN_ArenaNew(&request->start_response_arena, DN_NETEmcWSEvent, DN_ZMem_Yes);
  DN_Assert(result);
  if (result) {
    if (!emc_request->first_event)
      emc_request->first_event = result;
    if (emc_request->last_event)
      emc_request->last_event->next = result;
    emc_request->last_event = result;
  }
  return result;
}

static void DN_NET_EmcOnRequestDone_(DN_NETCore *net, DN_NETRequest *request)
{
  // NOTE: This may be call multiple times on the same request if we get multiple responses when we
  // yield to the javascript event loop, e.g. the application received multiple WS payloads before
  // it waited and consequently consumed the response from the payload.
  //
  // So if the next pointer is already set, then it should be that the request is already enqueued.
  DN_NETEmcCore *emc = DN_Cast(DN_NETEmcCore *) net->context;
  if (!request->next && !request->prev && request != emc->response_list) {
    request->prev = nullptr;
    request->next = emc->response_list;
    if (emc->response_list)
      emc->response_list->prev = request;
    emc->response_list = request;
  }
  DN_OS_SemaphoreIncrement(&net->completion_sem, 1);
  DN_OS_SemaphoreIncrement(&request->completion_sem, 1);
}

static bool DN_NET_EmcWSOnOpen(int eventType, EmscriptenWebSocketOpenEvent const *event, void *user_data)
{
  DN_NETRequest    *req       = DN_Cast(DN_NETRequest *) user_data;
  DN_NETCore       *net       = DN_Cast(DN_NETCore *) req->context[0];
  DN_NETEmcWSEvent *net_event = DN_NET_EmcAllocWSEvent_(req);
  net_event->state            = DN_NETResponseState_WSOpen;
  DN_NET_EmcOnRequestDone_(net, req);
  return true;
}

static bool DN_NET_EmcWSOnMessage(int eventType, const EmscriptenWebSocketMessageEvent *event, void *user_data)
{
  DN_NETRequest    *req       = DN_Cast(DN_NETRequest *) user_data;
  DN_NETCore       *net       = DN_Cast(DN_NETCore *) req->context[0];
  DN_NETEmcWSEvent *net_event = DN_NET_EmcAllocWSEvent_(req);
  net_event->state            = event->isText ? DN_NETResponseState_WSText : DN_NETResponseState_WSBinary;
  if (event->numBytes > 0)
    net_event->payload = DN_Str8FromPtrArena(event->data, event->numBytes, &req->start_response_arena);
  DN_NET_EmcOnRequestDone_(net, req);
  return true;
}

static bool DN_NET_EmcWSOnError(int eventType, EmscriptenWebSocketErrorEvent const *event, void *user_data)
{
  DN_NETRequest    *req       = DN_Cast(DN_NETRequest *) user_data;
  DN_NETCore       *net       = DN_Cast(DN_NETCore *) req->context[0];
  DN_NETEmcWSEvent *net_event = DN_NET_EmcAllocWSEvent_(req);
  net_event->state            = DN_NETResponseState_Error;
  DN_NET_EmcOnRequestDone_(net, req);
  return true;
}

static bool DN_NET_EmcWSOnClose(int eventType, EmscriptenWebSocketCloseEvent const *event, void *user_data)
{
  DN_NETRequest    *req       = DN_Cast(DN_NETRequest *) user_data;
  DN_NETCore       *net       = DN_Cast(DN_NETCore *) req->context[0];
  DN_NETEmcWSEvent *net_event = DN_NET_EmcAllocWSEvent_(req);
  net_event->state            = DN_NETResponseState_WSClose;
  net_event->payload          = DN_Str8FmtArena(&req->start_response_arena,
                                                    "Websocket closed '%.*s': (%u) %s (was %s close)",
                                                    DN_Str8PrintFmt(req->url),
                                                    event->code,
                                                    event->reason,
                                                    event->wasClean ? "clean" : "unclean");
  DN_NET_EmcOnRequestDone_(net, req);
  return true;
}

static void DN_NET_EmcHTTPSuccessCallback(emscripten_fetch_t *fetch)
{
  DN_NETRequest *req        = DN_Cast(DN_NETRequest *) fetch->userData;
  DN_NETCore    *net        = DN_Cast(DN_NETCore *) req->context[0];
  req->response.http_status = fetch->status;
  req->response.state       = DN_NETResponseState_HTTP;
  req->response.body        = DN_Str8FromStr8Arena(DN_Str8FromPtr(fetch->data, fetch->numBytes - 1), &req->arena);
  DN_NET_EmcOnRequestDone_(net, req);
}

static void DN_NET_EmcHTTPFailCallback(emscripten_fetch_t *fetch)
{
  DN_NETRequest *req        = DN_Cast(DN_NETRequest *) fetch->userData;
  DN_NETCore    *net        = DN_Cast(DN_NETCore *) req->context[0];
  req->response.http_status = fetch->status;
  req->response.state       = DN_NETResponseState_Error;
  DN_NET_EmcOnRequestDone_(net, req);
}

static void DN_NET_EmcHTTPProgressCallback(emscripten_fetch_t *fetch)
{
}

void DN_NET_EmcInit(DN_NETCore *net, char *base, DN_U64 base_size)
{
  DN_NET_BaseInit(net, base, base_size);
  DN_NETEmcCore *emc = DN_ArenaNew(&net->arena, DN_NETEmcCore, DN_ZMem_Yes);
  emc->pool          = DN_PoolFromArena(&net->arena, 0);
  net->context       = emc;
}

void DN_NET_EmcDeinit(DN_NETCore *net)
{
  (void)net;
  // TODO: Track all the request handles and clean it up
}

static DN_NETRequest *DN_NET_EmcAllocRequest_(DN_NETCore *net)
{
  // NOTE: Allocate request
  DN_NETEmcCore *emc = DN_Cast(DN_NETEmcCore *) net->context;
  DN_NETRequest *result = emc->free_list;
  if (result) {
    emc->free_list = emc->free_list->next;
    result->next   = nullptr;
    DN_Assert(result->prev == nullptr);
    if (emc->free_list) {
      DN_Assert(emc->free_list->prev == nullptr);
    }
  } else {
    result             = DN_ArenaNew(&net->arena, DN_NETRequest, DN_ZMem_Yes);
    result->context[1] = DN_Cast(DN_UPtr) DN_ArenaNew(&net->arena, DN_NETEmcRequest, DN_ZMem_Yes);
  }

  // NOTE: Setup some emscripten specific data into our request context
  if (result)
    result->context[0] = DN_Cast(DN_UPtr) net;
  return result;
}

DN_NETRequestHandle DN_NET_EmcDoHTTP(DN_NETCore *net, DN_Str8 url, DN_Str8 method, DN_NETDoHTTPArgs const *args)
{
  DN_NETRequest      *req    = DN_NET_EmcAllocRequest_(net);
  DN_NETRequestHandle result = DN_NET_SetupRequest(req, url, method, args, DN_NETRequestType_HTTP);

  // NOTE: Setup the HTTP request via Emscripten
  emscripten_fetch_attr_t fetch_attribs = {};
  {
    DN_Assert(req->args.payload.data[req->args.payload.count] == 0);
    DN_Assert(req->url.data[req->url.count] == 0);

    // NOTE: Setup request for emscripten
    emscripten_fetch_attr_init(&fetch_attribs);

    fetch_attribs.requestData     = req->args.payload.data;
    fetch_attribs.requestDataSize = req->args.payload.count;
    DN_Assert(req->method.count < DN_ArrayCountU(fetch_attribs.requestMethod));
    DN_Memcpy(fetch_attribs.requestMethod, req->method.data, req->method.count);
    fetch_attribs.requestMethod[req->method.count] = 0;

    // NOTE: Assign HTTP headers
    if (req->args.headers_size) {
      char **headers = DN_ArenaNewArray(&req->start_response_arena, char *, req->args.headers_size + 1, DN_ZMem_Yes);
      for (DN_ForItSize(it, DN_Str8, req->args.headers, req->args.headers_size)) {
      DN_Assert(it.data->data[it.data->count] == 0);
        headers[it.index] = it.data->data;
      }
      fetch_attribs.requestHeaders = headers;
    }

    // NOTE: Handle basic auth
    if (req->args.flags & DN_NETDoHTTPFlags_BasicAuth) {
      if (req->args.username.count && req->args.password.count) {
        DN_Assert(req->args.username.data[req->args.username.count] == 0);
        DN_Assert(req->args.password.data[req->args.password.count] == 0);
        fetch_attribs.withCredentials = true;
        fetch_attribs.userName        = req->args.username.data;
        fetch_attribs.password        = req->args.password.data;
      }
    }

    // NOTE: It would be nice to use EMSCRIPTEN_FETCH_STREAM_DATA however
    // emscripten has this note on the current version I'm using that this is
    // only supported in Firefox so this is a no-go.
    //
    //   > If passed, the intermediate streamed bytes will be passed in to the
    //   > onprogress() handler. If not specified, the onprogress() handler will still
    //   > be called, but without data bytes.  Note: Firefox only as it depends on
    //   > 'moz-chunked-arraybuffer'.
    fetch_attribs.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    fetch_attribs.onsuccess  = DN_NET_EmcHTTPSuccessCallback;
    fetch_attribs.onerror    = DN_NET_EmcHTTPFailCallback;
    fetch_attribs.onprogress = DN_NET_EmcHTTPProgressCallback;
    fetch_attribs.userData   = req;
  }

  // NOTE: Dispatch the asynchronous fetch
  emscripten_fetch(&fetch_attribs, req->url.data);
  return result;
}

DN_NETRequestHandle DN_NET_EmcDoWS(DN_NETCore *net, DN_Str8 url)
{
  DN_Assert(emscripten_websocket_is_supported());
  DN_NETRequest      *req    = DN_NET_EmcAllocRequest_(net);
  DN_NETRequestHandle result = DN_NET_SetupRequest(req, url, /*method=*/DN_Str8Lit(""), /*args=*/nullptr, DN_NETRequestType_WS);
  if (!req)
    return result;

  // NOTE: Create the websocket request and dispatch it via emscripten
  EmscriptenWebSocketCreateAttributes attr;
  emscripten_websocket_init_create_attributes(&attr);
  attr.url = req->url.data;

  DN_NETEmcRequest *emc_request = DN_Cast(DN_NETEmcRequest *) req->context[1];
  emc_request->socket           = emscripten_websocket_new(&attr);
  DN_Assert(emc_request->socket > 0);
  emscripten_websocket_set_onopen_callback(emc_request->socket, /*userData=*/req, DN_NET_EmcWSOnOpen);
  emscripten_websocket_set_onmessage_callback(emc_request->socket, /*userData=*/req, DN_NET_EmcWSOnMessage);
  emscripten_websocket_set_onerror_callback(emc_request->socket, /*userData=*/req, DN_NET_EmcWSOnError);
  emscripten_websocket_set_onclose_callback(emc_request->socket, /*userData=*/req, DN_NET_EmcWSOnClose);

  return result;
}

void DN_NET_EmcDoWSSend(DN_NETRequestHandle handle, DN_Str8 data, DN_NETWSSend send)
{
  DN_AssertF(send == DN_NETWSSend_Binary || send == DN_NETWSSend_Text || send == DN_NETWSSend_Close,
             "Unimplemented, Emscripten only supports some of the available operations");
  int            result      = 0;
  DN_NETRequest *request_ptr = DN_Cast(DN_NETRequest *) handle.handle;
  if (request_ptr && request_ptr->gen == handle.gen) {
    DN_Assert(request_ptr->type == DN_NETRequestType_WS);
    DN_NETEmcRequest *emc_request = DN_Cast(DN_NETEmcRequest *) request_ptr->context[1];
    switch (send) {
      default: DN_AssertInvalidCodePath; break;
      case DN_NETWSSend_Text: {
        DN_U64  pos                  = DN_MemListPos(request_ptr->start_response_arena.mem);
        DN_Str8 data_null_terminated = DN_Str8FromStr8Arena(data, &request_ptr->start_response_arena);
        result                       = emscripten_websocket_send_utf8_text(emc_request->socket, data_null_terminated.data);
        DN_MemListPopTo(request_ptr->start_response_arena.mem, pos);
      } break;

      case DN_NETWSSend_Binary: {
        result = emscripten_websocket_send_binary(emc_request->socket, data.data, data.count);
      } break;

      case DN_NETWSSend_Close: {
        result = emscripten_websocket_close(emc_request->socket, 0, nullptr);
      } break;
    }
  }
  // TODO: Handle result, the header file doesn't really elucidate what this result value is
  (void)result;
}

static DN_NETResponse DN_NET_EmcHandleFinishedRequest_(DN_NETCore *net, DN_NETEmcCore *emc, DN_NETRequestHandle handle, DN_NETRequest *request, DN_Arena *arena)
{
  // NOTE: Generate the response, copy out the strings into the user given memory
  DN_NETEmcRequest *emc_request     = DN_Cast(DN_NETEmcRequest *) request->context[1];
  DN_NETResponse    result          = request->response;
  bool              end_request     = true;
  bool              dequeue_request = true;
  if (request->type == DN_NETRequestType_HTTP) {
    result.body = DN_Str8FromStr8Arena(result.body, arena);
  } else {
    // NOTE: Get emscripten contexts
    DN_NETEmcWSEvent *emc_event = emc_request->first_event;
    DN_Assert(emc_event);

    DN_AssertF((emc_event->state >= DN_NETResponseState_WSOpen && emc_event->state <= DN_NETResponseState_WSPong) || emc_event->state == DN_NETResponseState_Error,
               "emc_event=%p", emc_event);

    // NOTE: Build the result
    result.state   = emc_event->state;
    result.request = handle;
    result.body    = DN_Str8FromStr8Arena(emc_event->payload, arena);

    // NOTE: Advance the event list
    {
      if (emc_request->first_event == emc_request->last_event) {
        emc_request->last_event = emc_request->last_event->next;
        DN_Assert(emc_request->first_event->next == emc_request->last_event);
      }
      emc_request->first_event = emc_event->next;

      // NOTE: If there's still an event on the request then we do not dequeue the request from the
      // response list. The user can still "wait" for a response to read more data from it.
      if (emc_request->first_event)
        dequeue_request = false;
    }

    if (result.state != DN_NETResponseState_WSClose)
      end_request = false;
  }

  // NOTE: Remove request from the response list which is doubly-linked
  if (dequeue_request) {
    if (request->prev) {
      DN_AssertF(request->prev->next == request, "next=%p vs request=%p", request->prev->next, request);
      request->prev->next = request->next;
    }

    if (request->next) {
      DN_AssertF(request->next->prev == request, "prev=%p vs request=%p", request->next->prev, request);
      request->next->prev = request->prev;
    }

    if (request == emc->response_list)
      emc->response_list = emc->response_list->next;

    request->prev = nullptr;
    request->next = nullptr;
    DN_Assert(emc_request->first_event == nullptr);
    DN_Assert(emc_request->last_event == nullptr);

    // NOTE: Deallocate the memory used in the request and reset the string builder (as all
    // payload(s) have been read from the request).
    if (!end_request)
      DN_ArenaTempEnd(&request->start_response_arena, DN_ArenaReset_Yes);
  }

  if (end_request) {
    emscripten_websocket_delete(emc_request->socket);
    emc_request->socket = 0;

    DN_NETEmcCore *emc = DN_Cast(DN_NETEmcCore *) net->context;
    request->next      = emc->free_list;
    request->prev      = nullptr;
    emc->free_list     = request;

    DN_Assert(emc_request->first_event == nullptr);
    DN_Assert(emc_request->last_event == nullptr);
    DN_NET_RequestRecycle(request);
  }

  return result;
}

static DN_OSSemaphoreWaitResult DN_NET_EmcSemaphoreWait_(DN_OSSemaphore *sem, DN_U32 timeout_ms)
{
  // NOTE: In emscripten you can't just block on the semaphore with 'timeout_ms' because it needs
  // to yield to the javascript's event loop otherwise the fetching step cannot progress. Instead
  // we use a timeout of 0 to just immediately check if the semaphore has been signalled, if not,
  // then we yield to the event loop by calling sleep.
  //
  // Once yielded, fetch will execute and eventually in the callback it will signal the semaphore
  // where it'll return and we can break out of the simulated "timeout".
  DN_OSSemaphoreWaitResult result               = {};
  DN_U32                   timeout_remaining_ms = timeout_ms;
  DN_F64                   begin_ms             = emscripten_get_now();
  for (;;) {
    result = DN_OS_SemaphoreWait(sem, 0);
    if (result == DN_OSSemaphoreWaitResult_Success)
      break;
    if (timeout_remaining_ms <= 0)
      break;

    emscripten_sleep(100 /*ms*/);
    DN_F64   end_ms      = emscripten_get_now();
    DN_USize duration_ms = DN_Cast(DN_USize)(end_ms - begin_ms);
    timeout_remaining_ms = timeout_remaining_ms >= duration_ms ? timeout_remaining_ms - duration_ms : 0;
    begin_ms             = end_ms;
  }
  return result;
}

DN_NETResponse DN_NET_EmcWaitForResponse(DN_NETRequestHandle handle, DN_Arena *arena, DN_U32 timeout_ms)
{
  DN_NETResponse result      = {};
  DN_NETRequest *request_ptr = DN_Cast(DN_NETRequest *) handle.handle;
  if (request_ptr && request_ptr->gen == handle.gen) {
    DN_NETCore    *net = DN_Cast(DN_NETCore *) request_ptr->context[0];
    DN_NETEmcCore *emc = DN_Cast(DN_NETEmcCore *) net->context;
    DN_Assert(emc);
    DN_OSSemaphoreWaitResult wait = DN_NET_EmcSemaphoreWait_(&request_ptr->completion_sem, timeout_ms);
    if (wait != DN_OSSemaphoreWaitResult_Success)
      return result;

    result = DN_NET_EmcHandleFinishedRequest_(net, emc, handle, request_ptr, arena);

    // NOTE: Decrement the global 'request done' completion semaphore since the user consumed the
    // request individually.
    DN_OSSemaphoreWaitResult net_wait_result = DN_OS_SemaphoreWait(&net->completion_sem, 0 /*timeout_ms*/);
    DN_AssertF(net_wait_result == DN_OSSemaphoreWaitResult_Success, "Wait result was: %zu", DN_Cast(DN_USize) net_wait_result);
  }
  return result;
}

DN_NETResponse DN_NET_EmcWaitForAnyResponse(DN_NETCore *net, DN_Arena *arena, DN_U32 timeout_ms)
{
  DN_NETEmcCore *emc = DN_Cast(DN_NETEmcCore *) net->context;
  DN_Assert(emc);

  DN_NETResponse          result = {};
  DN_OSSemaphoreWaitResult wait   = DN_NET_EmcSemaphoreWait_(&net->completion_sem, timeout_ms);
  if (wait != DN_OSSemaphoreWaitResult_Success)
      return result;

  DN_AssertF(emc->response_list,
             "This should be set otherwise we bumped the completion sem without queueing into the "
             "done list or we forgot to wait on the global semaphore after a request finished");

  // NOTE: Decrement the request's completion semaphore since the user consumed the global semaphore
  DN_NETRequest           *request_ptr     = emc->response_list;
  DN_OSSemaphoreWaitResult net_wait_result = DN_OS_SemaphoreWait(&request_ptr->completion_sem, 0 /*timeout_ms*/);
  DN_AssertF(net_wait_result == DN_OSSemaphoreWaitResult_Success, "Wait result was: %zu", DN_Cast(DN_USize) net_wait_result);

  DN_NETRequestHandle request = {};
  request.handle              = DN_Cast(DN_UPtr) request_ptr;
  request.gen                 = request_ptr->gen;
  result                      = DN_NET_EmcHandleFinishedRequest_(net, emc, request, request_ptr, arena);

  return result;
}
#endif // #if DN_WITH_NET_EMSCRIPTEN
