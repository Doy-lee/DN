#define DN_INC_CPP

#include "dn_inc.h"

DN_Core *g_dn_;

static void DN_InitOS_(DN_OSCore *os, DN_InitArgs *args)
{
  #if defined(DN_OS_H) && defined(DN_OS_CPP)
  // NOTE: OS
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

  // NOTE: Setup logging
  DN_OS_EmitLogsWithOSPrintFunctions(os);

  {
    #if defined(DN_PLATFORM_EMSCRIPTEN)
    os->arena = DN_ArenaFromHeap(DN_Megabytes(1), DN_ArenaFlags_NoAllocTrack);
    #else
    os->arena = DN_ArenaFromVMem(DN_Megabytes(1), DN_Kilobytes(4), DN_ArenaFlags_NoAllocTrack);
    #endif

    #if defined(DN_PLATFORM_WIN32)
    os->platform_context = DN_ArenaNew(&os->arena, DN_W32Core, DN_ZMem_Yes);
    #elif defined(DN_PLATFORM_POSIX) || defined(DN_PLATFORM_EMSCRIPTEN)
    os->platform_context = DN_ArenaNew(&os->arena, DN_POSIXCore, DN_ZMem_Yes);
    #endif

    #if defined(DN_PLATFORM_WIN32)
    DN_W32Core *w32 = DN_Cast(DN_W32Core *) os->platform_context;
    InitializeCriticalSection(&w32->sync_primitive_free_list_mutex);

    QueryPerformanceFrequency(&w32->qpc_frequency);
    HMODULE module = LoadLibraryA("kernel32.dll");
    if (module) {
      w32->set_thread_description = DN_Cast(DN_W32SetThreadDescriptionFunc *) GetProcAddress(module, "SetThreadDescription");
      FreeLibrary(module);
    }

    // NOTE: win32 bcrypt
    wchar_t const     BCRYPT_ALGORITHM[] = L"RNG";
    long /*NTSTATUS*/ init_status        = BCryptOpenAlgorithmProvider(&w32->bcrypt_rng_handle, BCRYPT_ALGORITHM, nullptr /*implementation*/, 0 /*flags*/);
    if (w32->bcrypt_rng_handle && init_status == 0)
      w32->bcrypt_init_success = true;
    else
      DN_LOG_ErrorF("Failed to initialise Windows secure random number generator, error: %d", init_status);
    #else
    DN_Posix_Init(DN_Cast(DN_POSIXCore *)os->platform_context);
    #endif
  }

  // NOTE: Initialise tmem arenas which allocate memory and will be
  // recorded to the now initialised allocation table. The initialisation
  // of tmem memory may request tmem memory itself in leak tracing mode.
  // This is supported as the tmem arenas defer allocation tracking until
  // initialisation is done.
  DN_OSTLSInitArgs tls_init_args = {};
  if (args) {
    tls_init_args.commit           = args->os_tls_commit;
    tls_init_args.reserve          = args->os_tls_reserve;
    tls_init_args.err_sink_reserve = args->os_tls_err_sink_reserve;
    tls_init_args.err_sink_commit  = args->os_tls_err_sink_commit;
  }

  DN_OS_TLSInit(&os->tls, tls_init_args);
  DN_OS_TLSSetCurrentThreadTLS(&os->tls);
  os->cpu_report = DN_CPUGetReport();

  #define DN_CPU_FEAT_XENTRY(label) g_dn_cpu_feature_decl[DN_CPUFeature_##label] = {DN_CPUFeature_##label, DN_Str8Lit(#label)};
  DN_CPU_FEAT_XMACRO
  #undef DN_CPU_FEAT_XENTRY
  DN_Assert(g_dn_);
  #endif // defined(DN_OS_H) && defined(DN_OS_CPP)
}


DN_API void DN_Init(DN_Core *dn, DN_InitFlags flags, DN_InitArgs *args)
{
  g_dn_ = dn;

  if (flags & DN_InitFlags_OS) {
    #if defined(DN_OS_H) && defined(DN_OS_CPP)
    DN_InitOS_(&dn->os, args);
    if (flags & DN_InitFlags_OSLeakTracker) {
      // NOTE: Setup the allocation table with allocation tracking turned off on
      // the arena we're using to initialise the table.
      dn->leak.alloc_table_arena = DN_ArenaFromVMem(DN_Megabytes(1), DN_Kilobytes(512), DN_ArenaFlags_NoAllocTrack | DN_ArenaFlags_AllocCanLeak);
      dn->leak.alloc_table       = DN_DSMap_Init<DN_LeakAlloc>(&dn->leak.alloc_table_arena, 4096, DN_DSMapFlags_Nil);
    }
    #endif
  }

  // NOTE: Print out init features
  char buf[4096];
  DN_USize buf_size = 0;
  if (flags & DN_InitFlags_LogLibFeatures) {
    DN_FmtAppendTruncate(buf, &buf_size, sizeof(buf), DN_Str8Lit("..."), "DN initialised:\n");
    #if defined(DN_OS_CPP)
    DN_F32 page_size_kib         = dn->os.page_size / 1024.0f;
    DN_F32 alloc_granularity_kib = dn->os.alloc_granularity / 1024.0f;
    DN_FmtAppendTruncate(buf,
                         &buf_size,
                         sizeof(buf),
                         DN_Str8Lit("..."),
                         "  OS Page Size/Alloc Granularity: %.1f/%.1fKiB\n"
                         "  Logical Processor Count: %u\n",
                         page_size_kib,
                         alloc_granularity_kib,
                         dn->os.logical_processor_count);
    #endif

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
    DN_POSIXCore *posix = DN_Cast(DN_POSIXCore *)g_dn_->os.platform_context;
    DN_FmtAppendTruncate(buf, &buf_size, sizeof(buf), DN_Str8Lit("..."), "  Clock GetTime: %S\n", posix->clock_monotonic_raw ? DN_Str8Lit("CLOCK_MONOTONIC_RAW") : DN_Str8Lit("CLOCK_MONOTONIC"));
    #endif
    // TODO(doyle): Add stacktrace feature log
  }

  if (flags & DN_InitFlags_LogCPUFeatures) {
    DN_CPUReport const *report = &dn->os.cpu_report;
    DN_Str8             brand  = DN_Str8TrimWhitespaceAround(DN_Str8FromPtr(report->brand, sizeof(report->brand) - 1));
    DN_MSVC_WARNING_PUSH
    DN_MSVC_WARNING_DISABLE(6284) // Object passed as _Param_(3) when a string is required in call to 'DN_Str8BuilderAppendF' Actual type: 'struct DN_Str8'.
    DN_FmtAppendTruncate(buf, &buf_size, sizeof(buf), DN_Str8Lit("..."), "  CPU '%S' from '%s' detected:\n", brand, report->vendor);
    DN_MSVC_WARNING_POP

    DN_USize longest_feature_name = 0;
    for (DN_ForIndexU(feature_index, DN_CPUFeature_Count)) {
      DN_CPUFeatureDecl feature_decl = g_dn_cpu_feature_decl[feature_index];
      longest_feature_name           = DN_Max(longest_feature_name, feature_decl.label.size);
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
                           DN_Cast(int)(longest_feature_name - feature_decl.label.size),
                           "",
                           has_feature ? "available" : "not available");
    }
  }

  if (buf_size)
    DN_LOG_DebugF("%.*s", DN_Cast(int)buf_size, buf);
}

DN_API void DN_BeginFrame()
{
  DN_AtomicSetValue64(&g_dn_->os.mem_allocs_frame, 0);
}
