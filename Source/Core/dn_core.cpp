static DN_Core *g_dn_core;

DN_API void DN_Core_Init(DN_Core *core, DN_CoreOnInit on_init)
{
  DN_Assert(g_dn_os_core_);
  g_dn_core = core;

  // NOTE Initialise fields //////////////////////////////////////////////////////////////////////
  #if !defined(DN_NO_PROFILER)
  core->profiler = &core->profiler_default_instance;
  #endif

  #if defined(DN_LEAK_TRACKING)
  // NOTE: Setup the allocation table with allocation tracking turned off on
  // the arena we're using to initialise the table.
  core->alloc_table_arena = DN_Arena_InitFromOSVMem(DN_Megabytes(1), DN_Kilobytes(512), DN_ArenaFlags_NoAllocTrack | DN_ArenaFlags_AllocCanLeak);
  core->alloc_table       = DN_DSMap_Init<DN_DebugAlloc>(&core->alloc_table_arena, 4096, DN_DSMapFlags_Nil);
  #endif

  // NOTE: Print out init features ///////////////////////////////////////////////////////////////
  DN_OSTLSTMem   tmem    = DN_OS_TLSPushTMem(nullptr);
  DN_Str8Builder builder = DN_Str8Builder_Init(tmem.arena);
  if (on_init & DN_CoreOnInit_LogLibFeatures) {
    DN_Str8Builder_AppendRef(&builder, DN_STR8("DN initialised:\n"));

    DN_F32 page_size_kib         = g_dn_os_core_->page_size / 1024.0f;
    DN_F32 alloc_granularity_kib = g_dn_os_core_->alloc_granularity / 1024.0f;
    DN_Str8Builder_AppendF(&builder,
                           "  OS Page Size/Alloc Granularity: %.1f/%.1fKiB\n"
                           "  Logical Processor Count: %u\n",
                           page_size_kib,
                           alloc_granularity_kib,
                           g_dn_os_core_->logical_processor_count);

    #if DN_HAS_FEATURE(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
    if (DN_ASAN_POISON) {
      DN_Str8Builder_AppendF(
          &builder, "  ASAN manual poisoning%s\n", DN_ASAN_VET_POISON ? " (+vet sanity checks)" : "");
      DN_Str8Builder_AppendF(&builder, "  ASAN poison guard size: %u\n", DN_ASAN_POISON_GUARD_SIZE);
    }
    #endif

    #if defined(DN_LEAK_TRACKING)
    DN_Str8Builder_AppendRef(&builder, DN_STR8("  Allocation leak tracing\n"));
    #endif

    #if !defined(DN_NO_PROFILER)
    DN_Str8Builder_AppendRef(&builder, DN_STR8("  TSC profiler available\n"));
    #endif

    #if defined(DN_PLATFORM_EMSCRIPTEN) || defined(DN_PLATFORM_POSIX)
    DN_POSIXCore *posix = DN_CAST(DN_POSIXCore *)g_dn_os_core_->platform_context;
    DN_Str8Builder_AppendF(&builder, "  Clock GetTime: %S\n", posix->clock_monotonic_raw ? DN_STR8("CLOCK_MONOTONIC_RAW") : DN_STR8("CLOCK_MONOTONIC"));
    #endif
    // TODO(doyle): Add stacktrace feature log
  }

  if (on_init & DN_CoreOnInit_LogCPUFeatures) {
    DN_CPUReport const *report = &g_dn_os_core_->cpu_report;
    DN_Str8             brand  = DN_Str8_TrimWhitespaceAround(DN_Str8_Init(report->brand, sizeof(report->brand) - 1));
    DN_MSVC_WARNING_PUSH
    DN_MSVC_WARNING_DISABLE(6284) // Object passed as _Param_(3) when a string is required in call to 'DN_Str8Builder_AppendF' Actual type: 'struct DN_Str8'.
    DN_Str8Builder_AppendF(&builder, "  CPU '%S' from '%s' detected:\n", brand, report->vendor);
    DN_MSVC_WARNING_POP

    DN_USize longest_feature_name = 0;
    for (DN_ForIndexU(feature_index, DN_CPUFeature_Count)) {
      DN_CPUFeatureDecl feature_decl = g_dn_cpu_feature_decl[feature_index];
      longest_feature_name           = DN_Max(longest_feature_name, feature_decl.label.size);
    }

    for (DN_ForIndexU(feature_index, DN_CPUFeature_Count)) {
      DN_CPUFeatureDecl feature_decl = g_dn_cpu_feature_decl[feature_index];
      bool              has_feature  = DN_CPU_HasFeature(report, feature_decl.value);
      DN_Str8Builder_AppendF(&builder,
                             "    %.*s:%*s%s\n",
                             DN_STR_FMT(feature_decl.label),
                             DN_CAST(int)(longest_feature_name - feature_decl.label.size),
                             "",
                             has_feature ? "available" : "not available");
    }
  }

  DN_Str8 info_log = DN_Str8Builder_Build(&builder, tmem.arena);
  if (DN_Str8_HasData(info_log))
    DN_LOG_DebugF("%.*s", DN_STR_FMT(info_log));
}

DN_API void DN_Core_BeginFrame()
{
  DN_Atomic_SetValue64(&g_dn_os_core_->mem_allocs_frame, 0);
}

#if !defined(DN_NO_PROFILER)
DN_API void DN_Core_SetProfiler(DN_Profiler *profiler)
{
  if (profiler)
    g_dn_core->profiler = profiler;
}
#endif
