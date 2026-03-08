#if (_CLANGD)
  #define DN_H_WITH_OS 1
  #include "dn.h"
#endif

#include "Base/dn_base.cpp"
#include "Base/dn_base_containers.cpp"
#include "Base/dn_base_leak.cpp"

#if DN_H_WITH_OS
#include "OS/dn_os.cpp"
#if defined(DN_PLATFORM_POSIX) || defined(DN_PLATFORM_EMSCRIPTEN)
  #include "OS/dn_os_posix.cpp"
#elif defined(DN_PLATFORM_WIN32)
  #include "OS/dn_os_w32.cpp"
#else
  #error Please define a platform e.g. 'DN_PLATFORM_WIN32' to enable the correct implementation for platform APIs
#endif
#endif

DN_Core *g_dn_;

DN_API void DN_Init(DN_Core *dn, DN_InitFlags flags, DN_InitArgs *args)
{
  DN_Set(dn);
  dn->init_flags = flags;

  if (DN_BitIsSet(flags, DN_InitFlags_OS)) {
    #if DN_H_WITH_OS
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
      #if defined(DN_PLATFORM_EMSCRIPTEN)
      os->arena = DN_ArenaFromHeap(DN_Megabytes(1), DN_ArenaFlags_NoAllocTrack);
      #else
      os->arena = DN_ArenaFromVMem(DN_Megabytes(1), DN_Kilobytes(4), DN_ArenaFlags_NoAllocTrack);
      #endif

      #if defined(DN_PLATFORM_WIN32)
      os->platform_context = DN_ArenaNew(&os->arena, DN_OSW32Core, DN_ZMem_Yes);
      #elif defined(DN_PLATFORM_POSIX) || defined(DN_PLATFORM_EMSCRIPTEN)
      os->platform_context = DN_ArenaNew(&os->arena, DN_OSPOSIXCore, DN_ZMem_Yes);
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
  }

  if (DN_BitIsSet(flags, DN_InitFlags_LeakTracker)) {
    DN_Assert(dn->os_init);
    #if DN_H_WITH_OS
    // NOTE: Setup the allocation table with allocation tracking turned off on
    // the arena we're using to initialise the table.
    dn->leak.alloc_table_arena = DN_ArenaFromVMem(DN_Megabytes(1), DN_Kilobytes(512), DN_ArenaFlags_NoAllocTrack | DN_ArenaFlags_AllocCanLeak);
    dn->leak.alloc_table       = DN_DSMapInit<DN_LeakAlloc>(&dn->leak.alloc_table_arena, 4096, DN_DSMapFlags_Nil);
    #endif
  }

  if (DN_BitIsSet(flags, DN_InitFlags_ThreadContext)) {
    DN_Assert(dn->os_init);
    #if DN_H_WITH_OS
    DN_TCInitArgs *tc_init_args = args ? &args->thread_context_init_args : nullptr;
    DN_TCInitFromMemFuncs(&dn->main_tc, DN_OS_ThreadID(), tc_init_args, DN_ArenaMemFuncsGetDefaults());
    DN_TCEquip(&dn->main_tc);
    #endif
  }

  // NOTE: Print out init features
  char buf[4096];
  DN_USize buf_size = 0;
  if (DN_BitIsSet(flags, DN_InitFlags_LogLibFeatures)) {
    DN_FmtAppendTruncate(buf, &buf_size, sizeof(buf), DN_Str8Lit("..."), "DN initialised:\n");
    #if DN_H_WITH_OS
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
    if (DN_BitIsSet(flags, DN_InitFlags_ThreadContext)) {
      DN_Arena *arena     = dn->main_tc.main_arena;
      DN_Str8   mem_funcs = DN_Str8Lit("");
      switch (arena->mem_funcs.type) {
        case DN_ArenaMemFuncType_Nil:   break;
        case DN_ArenaMemFuncType_Basic: mem_funcs = DN_Str8Lit("Basic"); break;
        case DN_ArenaMemFuncType_VMem:  mem_funcs = DN_Str8Lit("VMem"); break;
      }
      DN_Str8x32 main_commit  = DN_ByteCountStr8x32(dn->main_tc.main_arena->curr->commit);
      DN_Str8x32 main_reserve = DN_ByteCountStr8x32(dn->main_tc.main_arena->curr->reserve);
      DN_Str8x32 temp_commit  = DN_ByteCountStr8x32(dn->main_tc.temp_a_arena->curr->commit);
      DN_Str8x32 temp_reserve = DN_ByteCountStr8x32(dn->main_tc.temp_a_arena->curr->reserve);
      DN_Str8x32 err_commit   = DN_ByteCountStr8x32(dn->main_tc.err_sink.arena->curr->commit);
      DN_Str8x32 err_reserve  = DN_ByteCountStr8x32(dn->main_tc.err_sink.arena->curr->reserve);
      DN_FmtAppendTruncate(buf,
                           &buf_size,
                           sizeof(buf),
                           DN_Str8Lit("..."),
                           "M %.*s/%.*s S(x2) %.*s/%.*s E %.*s/%.*s (%.*s)\n",
                           DN_Str8PrintFmt(main_commit),
                           DN_Str8PrintFmt(main_reserve),
                           DN_Str8PrintFmt(temp_commit),
                           DN_Str8PrintFmt(temp_reserve),
                           DN_Str8PrintFmt(err_commit),
                           DN_Str8PrintFmt(err_reserve),
                           DN_Str8PrintFmt(mem_funcs));
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
    #if DN_H_WITH_OS
    DN_CPUReport const *report = &dn->os.cpu_report;
    DN_Str8             brand  = DN_Str8TrimWhitespaceAround(DN_Str8FromPtr(report->brand, sizeof(report->brand) - 1));
    DN_FmtAppendTruncate(buf, &buf_size, sizeof(buf), DN_Str8Lit("..."), "  CPU '%.*s' from '%s' detected:\n", DN_Str8PrintFmt(brand), report->vendor);

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
  #if DN_H_WITH_OS
  DN_AtomicSetValue64(&g_dn_->os.mem_allocs_frame, 0);
  #endif
}

#if DN_H_WITH_HELPERS
#include "Extra/dn_helpers.cpp"
#endif

#if DN_H_WITH_ASYNC
#include "Extra/dn_async.cpp"
#endif

#if DN_H_WITH_NET
#include "Extra/dn_net.cpp"
#endif

#if DN_CPP_WITH_TESTS
#include "Extra/dn_tests.cpp"
#endif

#if DN_CPP_WITH_DEMO
#include "Extra/dn_demo.cpp"
#endif

