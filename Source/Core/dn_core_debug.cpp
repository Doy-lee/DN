#define DN_CORE_DEBUG_CPP

#include "../dn_base_inc.h"
#include "../dn_os_inc.h"
#include "../dn_core_inc.h"

DN_API DN_StackTraceWalkResult DN_StackTrace_Walk(DN_Arena *arena, uint16_t limit)
{
  DN_StackTraceWalkResult result = {};
#if defined(DN_OS_WIN32)
  if (!arena)
    return result;

  static DN_TicketMutex mutex = {};
  DN_TicketMutex_Begin(&mutex);

  HANDLE thread  = GetCurrentThread();
  result.process = GetCurrentProcess();

  DN_W32Core *w32 = DN_OS_GetW32Core_();
  if (!w32->sym_initialised) {
    w32->sym_initialised = true;
    SymSetOptions(SYMOPT_LOAD_LINES);
    if (!SymInitialize(result.process, nullptr /*UserSearchPath*/, true /*fInvadeProcess*/)) {
      DN_OSTLSTMem  tmem  = DN_OS_TLSTMem(arena);
      DN_W32Error error = DN_W32_LastError(tmem.arena);
      DN_LOG_ErrorF("SymInitialize failed, stack trace can not be generated (%lu): %.*s\n", error.code, DN_Str8PrintFmt(error.msg));
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

  DN_FArray<uint64_t, 256> raw_frames = {};
  while (raw_frames.size < limit) {
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
    DN_FArray_Add(&raw_frames, frame.AddrPC.Offset);
  }
  DN_TicketMutex_End(&mutex);

  result.base_addr = DN_ArenaNewArray(arena, uint64_t, raw_frames.size, DN_ZMem_No);
  result.size      = DN_Cast(uint16_t) raw_frames.size;
  DN_Memcpy(result.base_addr, raw_frames.data, raw_frames.size * sizeof(raw_frames.data[0]));
#else
  (void)limit;
  (void)arena;
#endif
  return result;
}

static void DN_StackTrace_AddWalkToStr8Builder(DN_StackTraceWalkResult const *walk, DN_Str8Builder *builder, DN_USize skip)
{
  DN_StackTraceRawFrame raw_frame = {};
  raw_frame.process               = walk->process;
  for (DN_USize index = skip; index < walk->size; index++) {
    raw_frame.base_addr      = walk->base_addr[index];
    DN_StackTraceFrame frame = DN_StackTrace_RawFrameToFrame(builder->arena, raw_frame);
    DN_Str8BuilderAppendF(builder, "%.*s(%zu): %.*s%s", DN_Str8PrintFmt(frame.file_name), frame.line_number, DN_Str8PrintFmt(frame.function_name), (DN_Cast(int) index == walk->size - 1) ? "" : "\n");
  }
}

DN_API bool DN_StackTrace_WalkResultIterate(DN_StackTraceWalkResultIterator *it, DN_StackTraceWalkResult const *walk)
{
  bool result = false;
  if (!it || !walk || !walk->base_addr || !walk->process)
    return result;

  if (it->index >= walk->size)
    return false;

  result                  = true;
  it->raw_frame.process   = walk->process;
  it->raw_frame.base_addr = walk->base_addr[it->index++];
  return result;
}

DN_API DN_Str8 DN_StackTrace_WalkResultToStr8(DN_Arena *arena, DN_StackTraceWalkResult const *walk, uint16_t skip)
{
  DN_Str8 result{};
  if (!walk || !arena)
    return result;

  DN_OSTLSTMem     tmem    = DN_OS_TLSTMem(arena);
  DN_Str8Builder builder = DN_Str8BuilderFromArena(tmem.arena);
  DN_StackTrace_AddWalkToStr8Builder(walk, &builder, skip);
  result = DN_Str8BuilderBuild(&builder, arena);
  return result;
}

DN_API DN_Str8 DN_StackTrace_WalkStr8(DN_Arena *arena, uint16_t limit, uint16_t skip)
{
  DN_OSTLSTMem              tmem   = DN_OS_TLSPushTMem(arena);
  DN_StackTraceWalkResult walk   = DN_StackTrace_Walk(tmem.arena, limit);
  DN_Str8                 result = DN_StackTrace_WalkResultToStr8(arena, &walk, skip);
  return result;
}

DN_API DN_Str8 DN_StackTrace_WalkStr8FromHeap(uint16_t limit, uint16_t skip)
{
  // NOTE: We don't use WalkResultToStr8 because that uses the TLS arenas which
  // does not use the OS heap.
  DN_Arena       arena         = DN_ArenaFromHeap(DN_Kilobytes(64), DN_ArenaFlags_NoAllocTrack);
  DN_Str8Builder builder       = DN_Str8BuilderFromArena(&arena);
  DN_StackTraceWalkResult walk = DN_StackTrace_Walk(&arena, limit);
  DN_StackTrace_AddWalkToStr8Builder(&walk, &builder, skip);
  DN_Str8 result               = DN_Str8BuilderBuildFromOSHeap(&builder);
  DN_ArenaDeinit(&arena);
  return result;
}

DN_API DN_Slice<DN_StackTraceFrame> DN_StackTrace_GetFrames(DN_Arena *arena, uint16_t limit)
{
    DN_Slice<DN_StackTraceFrame> result = {};
    if (!arena)
        return result;

    DN_OSTLSTMem              tmem = DN_OS_TLSTMem(arena);
    DN_StackTraceWalkResult walk = DN_StackTrace_Walk(tmem.arena, limit);
    if (!walk.size)
        return result;

    DN_USize slice_index = 0;
    result                = DN_Slice_Alloc<DN_StackTraceFrame>(arena, walk.size, DN_ZMem_No);
    for (DN_StackTraceWalkResultIterator it = {}; DN_StackTrace_WalkResultIterate(&it, &walk); ) {
        result.data[slice_index++] = DN_StackTrace_RawFrameToFrame(arena, it.raw_frame);
    }
    return result;
}

DN_API DN_StackTraceFrame DN_StackTrace_RawFrameToFrame(DN_Arena *arena, DN_StackTraceRawFrame raw_frame)
{
    #if defined(DN_OS_WIN32)
    // NOTE: Get line+filename /////////////////////////////////////////////////////////////////////

    // TODO: Why does zero-initialising this with `line = {};` cause
    // SymGetLineFromAddr64 function to fail once we are at
    // __scrt_commain_main_seh and hit BaseThreadInitThunk frame? The
    // line and file number are still valid in the result which we use, so,
    // we silently ignore this error.
    IMAGEHLP_LINEW64 line;
    line.SizeOfStruct       = sizeof(line);
    DWORD line_displacement = 0;
    if (!SymGetLineFromAddrW64(raw_frame.process, raw_frame.base_addr, &line_displacement, &line)) {
        line = {};
    }

    // NOTE: Get function name /////////////////////////////////////////////////////////////////////

    alignas(SYMBOL_INFOW) char buffer[sizeof(SYMBOL_INFOW) + (MAX_SYM_NAME * sizeof(wchar_t))] = {};
    SYMBOL_INFOW *symbol = DN_Cast(SYMBOL_INFOW *)buffer;
    symbol->SizeOfStruct = sizeof(*symbol);
    symbol->MaxNameLen   = sizeof(buffer) - sizeof(*symbol);

    uint64_t symbol_displacement = 0; // Offset to the beginning of the symbol to the address
    SymFromAddrW(raw_frame.process, raw_frame.base_addr, &symbol_displacement, symbol);

    // NOTE: Construct result //////////////////////////////////////////////////////////////////////

    DN_Str16 file_name16     = DN_Str16{line.FileName, DN_CStr16Size(line.FileName)};
    DN_Str16 function_name16 = DN_Str16{symbol->Name, symbol->NameLen};

    DN_StackTraceFrame result = {};
    result.address             = raw_frame.base_addr;
    result.line_number         = line.LineNumber;
    result.file_name           = DN_W32_Str16ToStr8(arena, file_name16);
    result.function_name       = DN_W32_Str16ToStr8(arena, function_name16);

    if (result.function_name.size == 0)
        result.function_name = DN_Str8Lit("<unknown function>");
    if (result.file_name.size == 0)
        result.file_name = DN_Str8Lit("<unknown file>");
    #else
    DN_StackTraceFrame result = {};
    #endif
    return result;
}

DN_API void DN_StackTrace_Print(uint16_t limit)
{
    DN_OSTLSTMem                    tmem        = DN_OS_TLSTMem(nullptr);
    DN_Slice<DN_StackTraceFrame> stack_trace = DN_StackTrace_GetFrames(tmem.arena, limit);
    for (DN_StackTraceFrame &frame : stack_trace)
        DN_OS_PrintErrLnF("%.*s(%I64u): %.*s", DN_Str8PrintFmt(frame.file_name), frame.line_number, DN_Str8PrintFmt(frame.function_name));
}

DN_API void DN_StackTrace_ReloadSymbols()
{
    #if defined(DN_OS_WIN32)
    HANDLE process = GetCurrentProcess();
    SymRefreshModuleList(process);
    #endif
}

// NOTE: DN_Debug //////////////////////////////////////////////////////////////////////////////////
#if defined(DN_LEAK_TRACKING)
DN_API void DN_DBGTrackAlloc(void *ptr, DN_USize size, bool leak_permitted)
{
  if (!ptr)
    return;

  DN_TicketMutex_Begin(&g_dn_core->alloc_table_mutex);
  DN_DEFER
  {
    DN_TicketMutex_End(&g_dn_core->alloc_table_mutex);
  };

  // NOTE: If the entry was not added, we are reusing a pointer that has been freed.
  // TODO: Add API for always making the item but exposing a var to indicate if the item was newly created or it
  // already existed.
  DN_Str8                       stack_trace = DN_StackTrace_WalkStr8FromHeap(128, 3 /*skip*/);
  DN_DSMap<DN_DebugAlloc>      *alloc_table = &g_dn_core->alloc_table;
  DN_DSMapResult<DN_DebugAlloc> alloc_entry = DN_DSMap_MakeKeyU64(alloc_table, DN_Cast(uint64_t) ptr);
  DN_DebugAlloc                *alloc       = alloc_entry.value;
  if (alloc_entry.found) {
    if ((alloc->flags & DN_DebugAllocFlag_Freed) == 0) {
      DN_Str8x32 alloc_size     = DN_ByteCountStr8x32(alloc->size);
      DN_Str8x32 new_alloc_size = DN_ByteCountStr8x32(size);
      DN_HardAssertF(
          alloc->flags & DN_DebugAllocFlag_Freed,
          "This pointer is already in the leak tracker, however it has not been freed yet. This "
          "same pointer is being ask to be tracked twice in the allocation table, e.g. one if its "
          "previous free calls has not being marked freed with an equivalent call to "
          "DN_DBGTrackDealloc()\n"
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
    g_dn_core->alloc_table_bytes_allocated_for_stack_traces -= alloc->stack_trace.size;
    g_dn_core->alloc_table_bytes_allocated_for_stack_traces -= alloc->freed_stack_trace.size;

    DN_OS_MemDealloc(alloc->stack_trace.data);
    DN_OS_MemDealloc(alloc->freed_stack_trace.data);
    *alloc = {};
  }

  alloc->ptr         = ptr;
  alloc->size        = size;
  alloc->stack_trace = stack_trace;
  alloc->flags |= leak_permitted ? DN_DebugAllocFlag_LeakPermitted : 0;
  g_dn_core->alloc_table_bytes_allocated_for_stack_traces += alloc->stack_trace.size;
}

DN_API void DN_DBGTrackDealloc(void *ptr)
{
    if (!ptr)
        return;

    DN_TicketMutex_Begin(&g_dn_core->alloc_table_mutex);
    DN_DEFER { DN_TicketMutex_End(&g_dn_core->alloc_table_mutex); };

    DN_Str8                        stack_trace = DN_StackTrace_WalkStr8FromHeap(128, 3 /*skip*/);
    DN_DSMap<DN_DebugAlloc>      *alloc_table  = &g_dn_core->alloc_table;
    DN_DSMapResult<DN_DebugAlloc> alloc_entry  = DN_DSMap_FindKeyU64(alloc_table, DN_Cast(uintptr_t) ptr);
    DN_HardAssertF(alloc_entry.found,
                     "Allocated pointer can not be removed as it does not exist in the "
                     "allocation table. When this memory was allocated, the pointer was "
                     "not added to the allocation table [ptr=%p]",
                     ptr);

    DN_DebugAlloc *alloc = alloc_entry.value;
    if (alloc->flags & DN_DebugAllocFlag_Freed) {
        DN_Str8x32 freed_size = DN_ByteCountStr8x32(alloc->freed_size);
        DN_HardAssertF((alloc->flags & DN_DebugAllocFlag_Freed) == 0,
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
                         "%.*s\n"
                         ,
                         ptr, DN_Str8PrintFmt(freed_size),
                         DN_Str8PrintFmt(alloc->stack_trace),
                         DN_Str8PrintFmt(alloc->freed_stack_trace),
                         DN_Str8PrintFmt(stack_trace));
    }

    DN_Assert(alloc->freed_stack_trace.size == 0);
    alloc->flags             |= DN_DebugAllocFlag_Freed;
    alloc->freed_stack_trace  = stack_trace;
    g_dn_core->alloc_table_bytes_allocated_for_stack_traces += alloc->freed_stack_trace.size;
}

DN_API void DN_DBGDumpLeaks()
{
    uint64_t leak_count   = 0;
    uint64_t leaked_bytes = 0;
    for (DN_USize index = 1; index < g_dn_core->alloc_table.occupied; index++) {
        DN_DSMapSlot<DN_DebugAlloc> *slot           = g_dn_core->alloc_table.slots + index;
        DN_DebugAlloc                *alloc          = &slot->value;
        bool                           alloc_leaked   = (alloc->flags & DN_DebugAllocFlag_Freed) == 0;
        bool                           leak_permitted = (alloc->flags & DN_DebugAllocFlag_LeakPermitted);
        if (alloc_leaked && !leak_permitted) {
            leaked_bytes += alloc->size;
            leak_count++;
            DN_Str8x32 alloc_size = DN_ByteCountStr8x32(alloc->size);
            DN_LOG_WarningF("Pointer (0x%p) leaked %.*s at:\n"
                             "%.*s",
                             alloc->ptr, DN_Str8PrintFmt(alloc_size),
                             DN_Str8PrintFmt(alloc->stack_trace));
        }
    }

    if (leak_count) {
      DN_Str8x32 leak_size = DN_ByteCountStr8x32(leaked_bytes);
      DN_LOG_WarningF("There were %I64u leaked allocations totalling %.*s", leak_count, DN_Str8PrintFmt(leak_size));
    }
}
#endif // DN_LEAK_TRACKING

// NOTE: DN_Profiler
DN_API DN_Profiler DN_Profiler_Init(DN_ProfilerAnchor *anchors, DN_USize count, DN_USize anchors_per_frame, DN_ProfilerTSC tsc, DN_U64 tsc_frequency)
{
  DN_Profiler result       = {};
  result.anchors           = anchors;
  result.anchors_count     = count;
  result.anchors_per_frame = anchors_per_frame;
  result.tsc               = tsc;
  result.tsc_frequency     = tsc_frequency;
  return result;
}

DN_API DN_USize DN_Profiler_FrameCount(DN_Profiler const *profiler)
{
  DN_USize result = profiler->anchors_count / profiler->anchors_per_frame;
  return result;
}

DN_API DN_ProfilerAnchorArray DN_Profiler_FrameAnchorsFromIndex(DN_Profiler *profiler, DN_USize frame_index)
{
  DN_ProfilerAnchorArray result        = {};
  DN_USize               anchor_offset = frame_index * profiler->anchors_per_frame;
  result.data                          = profiler->anchors + anchor_offset;
  result.count                         = profiler->anchors_per_frame;
  return result;
}

DN_API DN_ProfilerAnchorArray DN_Profiler_FrameAnchors(DN_Profiler *profiler)
{
  DN_ProfilerAnchorArray result = DN_Profiler_FrameAnchorsFromIndex(profiler, profiler->frame_index);
  return result;
}

DN_API DN_ProfilerZone DN_Profiler_BeginZone(DN_Profiler *profiler, DN_Str8 name, DN_U16 anchor_index)
{
  DN_ProfilerZone result = {};
  if (profiler->paused)
    return result;

  DN_Assert(anchor_index < profiler->anchors_per_frame);
  DN_ProfilerAnchor *anchor = DN_Profiler_FrameAnchors(profiler).data + anchor_index;
  anchor->name              = name;

  // TODO: We need per-thread-local-storage profiler so that we can use these apis
  // across threads. For now, we let them overwrite each other but this is not tenable.
  #if 0
    if (DN_Str8HasData(anchor->name) && anchor->name != name)
        DN_AssertF(name == anchor->name, "Potentially overwriting a zone by accident? Anchor is '%.*s', name is '%.*s'", DN_Str8PrintFmt(anchor->name), DN_Str8PrintFmt(name));
  #endif

  if (profiler->tsc == DN_ProfilerTSC_RDTSC)
    result.begin_tsc = DN_CPUGetTSC();
  else
    result.begin_tsc = DN_OS_PerfCounterNow();
  result.anchor_index              = anchor_index;
  result.parent_zone               = profiler->parent_zone;
  result.elapsed_tsc_at_zone_start = anchor->tsc_inclusive;
  profiler->parent_zone            = anchor_index;
  return result;
}

DN_API void DN_Profiler_EndZone(DN_Profiler *profiler, DN_ProfilerZone zone)
{
  if (profiler->paused)
    return;

  DN_Assert(zone.anchor_index < profiler->anchors_per_frame);
  DN_Assert(zone.parent_zone < profiler->anchors_per_frame);

  DN_ProfilerAnchorArray array       = DN_Profiler_FrameAnchors(profiler);
  DN_ProfilerAnchor     *anchor      = array.data + zone.anchor_index;
  DN_U64                 tsc_now     = profiler->tsc == DN_ProfilerTSC_RDTSC ? DN_CPUGetTSC() : DN_OS_PerfCounterNow();
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

DN_API void DN_Profiler_NewFrame(DN_Profiler *profiler)
{
  if (profiler->paused)
    return;

  // NOTE: End the frame's zone
  DN_Profiler_EndZone(profiler, profiler->frame_zone);
  DN_ProfilerAnchorArray old_frame_anchors = DN_Profiler_FrameAnchors(profiler);
  DN_ProfilerAnchor      old_frame_anchor  = old_frame_anchors.data[0];
  profiler->frame_avg_tsc                  = (profiler->frame_avg_tsc + old_frame_anchor.tsc_inclusive) / 2.f;

  // NOTE: Bump to the next frame
  DN_USize frame_count                = profiler->anchors_count / profiler->anchors_per_frame;
  profiler->frame_index               = (profiler->frame_index + 1) % frame_count;

  // NOTE: Zero out the anchors
  DN_ProfilerAnchorArray next_anchors = DN_Profiler_FrameAnchors(profiler);
  DN_Memset(next_anchors.data, 0, sizeof(*profiler->anchors) * next_anchors.count);

  // NOTE: Start the frame's zone
  profiler->frame_zone = DN_Profiler_BeginZone(profiler, DN_Str8Lit("Profiler Frame"), 0);
}

DN_API void DN_Profiler_Dump(DN_Profiler *profiler)
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

DN_API DN_F64 DN_Profiler_SecFromTSC(DN_Profiler *profiler, DN_U64 duration_tsc)
{
  DN_F64 result = DN_Cast(DN_F64)duration_tsc / profiler->tsc_frequency;
  return result;
}

DN_API DN_F64 DN_Profiler_MsFromTSC(DN_Profiler *profiler, DN_U64 duration_tsc)
{
  DN_F64 result = DN_Cast(DN_F64)duration_tsc / profiler->tsc_frequency * 1000.0;
  return result;
}
