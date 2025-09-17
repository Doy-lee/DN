#if !defined(DN_CORE_H)
#define DN_CORE_H

// NOTE: DN_Core
struct DN_Core
{
  // NOTE: Leak Tracing
  #if defined(DN_LEAK_TRACKING)
  DN_DSMap<DN_DebugAlloc> alloc_table;
  DN_TicketMutex          alloc_table_mutex;
  DN_Arena                alloc_table_arena;
  #endif
  DN_U64                  alloc_table_bytes_allocated_for_stack_traces;
};

enum DN_CoreOnInit
{
  DN_CoreOnInit_Nil            = 0,
  DN_CoreOnInit_LogLibFeatures = 1 << 0,
  DN_CoreOnInit_LogCPUFeatures = 1 << 1,
  DN_CoreOnInit_LogAllFeatures = DN_CoreOnInit_LogLibFeatures | DN_CoreOnInit_LogCPUFeatures,
};

DN_API void DN_Core_Init      (DN_Core *core, DN_CoreOnInit on_init);
DN_API void DN_Core_BeginFrame();
#endif // !defined(DN_CORE_H)
