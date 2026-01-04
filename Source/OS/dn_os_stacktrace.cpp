#define DN_CORE_DEBUG_CPP

#if defined(_CLANGD)
#define DN_H_WITH_OS 1
#include "../dn.h"
#endif

DN_API DN_StackTraceWalkResult DN_StackTraceWalk(DN_Arena *arena, uint16_t limit)
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
      DN_OSTLSTMem tmem  = DN_OS_TLSTMem(arena);
      DN_W32Error  error = DN_W32_LastError(tmem.arena);
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

static void DN_StackTraceAddWalkToStr8Builder(DN_StackTraceWalkResult const *walk, DN_Str8Builder *builder, DN_USize skip)
{
  DN_StackTraceRawFrame raw_frame = {};
  raw_frame.process               = walk->process;
  for (DN_USize index = skip; index < walk->size; index++) {
    raw_frame.base_addr      = walk->base_addr[index];
    DN_StackTraceFrame frame = DN_StackTraceRawFrameToFrame(builder->arena, raw_frame);
    DN_Str8BuilderAppendF(builder, "%.*s(%zu): %.*s%s", DN_Str8PrintFmt(frame.file_name), frame.line_number, DN_Str8PrintFmt(frame.function_name), (DN_Cast(int) index == walk->size - 1) ? "" : "\n");
  }
}

DN_API bool DN_StackTraceWalkResultIterate(DN_StackTraceWalkResultIterator *it, DN_StackTraceWalkResult const *walk)
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

DN_API DN_Str8 DN_StackTraceWalkResultToStr8(DN_Arena *arena, DN_StackTraceWalkResult const *walk, uint16_t skip)
{
  DN_Str8 result{};
  if (!walk || !arena)
    return result;

  DN_OSTLSTMem   tmem    = DN_OS_TLSTMem(arena);
  DN_Str8Builder builder = DN_Str8BuilderFromArena(tmem.arena);
  DN_StackTraceAddWalkToStr8Builder(walk, &builder, skip);
  result = DN_Str8BuilderBuild(&builder, arena);
  return result;
}

DN_API DN_Str8 DN_StackTraceWalkStr8(DN_Arena *arena, uint16_t limit, uint16_t skip)
{
  DN_OSTLSTMem            tmem   = DN_OS_TLSPushTMem(arena);
  DN_StackTraceWalkResult walk   = DN_StackTraceWalk(tmem.arena, limit);
  DN_Str8                 result = DN_StackTraceWalkResultToStr8(arena, &walk, skip);
  return result;
}

DN_API DN_Str8 DN_StackTraceWalkStr8FromHeap(uint16_t limit, uint16_t skip)
{
  // NOTE: We don't use WalkResultToStr8 because that uses the TLS arenas which
  // does not use the OS heap.
  DN_Arena                arena   = DN_ArenaFromHeap(DN_Kilobytes(64), DN_ArenaFlags_NoAllocTrack);
  DN_Str8Builder          builder = DN_Str8BuilderFromArena(&arena);
  DN_StackTraceWalkResult walk    = DN_StackTraceWalk(&arena, limit);
  DN_StackTraceAddWalkToStr8Builder(&walk, &builder, skip);
  DN_Str8 result = DN_Str8BuilderBuildFromOSHeap(&builder);
  DN_ArenaDeinit(&arena);
  return result;
}

DN_API DN_Slice<DN_StackTraceFrame> DN_StackTraceGetFrames(DN_Arena *arena, uint16_t limit)
{
  DN_Slice<DN_StackTraceFrame> result = {};
  if (!arena)
    return result;

  DN_OSTLSTMem            tmem = DN_OS_TLSTMem(arena);
  DN_StackTraceWalkResult walk = DN_StackTraceWalk(tmem.arena, limit);
  if (!walk.size)
    return result;

  DN_USize slice_index = 0;
  result               = DN_Slice_Alloc<DN_StackTraceFrame>(arena, walk.size, DN_ZMem_No);
  for (DN_StackTraceWalkResultIterator it = {}; DN_StackTraceWalkResultIterate(&it, &walk);)
    result.data[slice_index++] = DN_StackTraceRawFrameToFrame(arena, it.raw_frame);
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

  DN_Str16 file_name16     = DN_Str16{line.FileName, DN_CStr16Size(line.FileName)};
  DN_Str16 function_name16 = DN_Str16{symbol->Name, symbol->NameLen};

  DN_StackTraceFrame result = {};
  result.address            = raw_frame.base_addr;
  result.line_number        = line.LineNumber;
  result.file_name          = DN_W32_Str16ToStr8(arena, file_name16);
  result.function_name      = DN_W32_Str16ToStr8(arena, function_name16);

  if (result.function_name.size == 0)
    result.function_name = DN_Str8Lit("<unknown function>");
  if (result.file_name.size == 0)
    result.file_name = DN_Str8Lit("<unknown file>");
#else
  DN_StackTraceFrame result = {};
#endif
  return result;
}

DN_API void DN_StackTracePrint(uint16_t limit)
{
  DN_OSTLSTMem                 tmem        = DN_OS_TLSTMem(nullptr);
  DN_Slice<DN_StackTraceFrame> stack_trace = DN_StackTraceGetFrames(tmem.arena, limit);
  for (DN_StackTraceFrame &frame : stack_trace)
    DN_OS_PrintErrLnF("%.*s(%I64u): %.*s", DN_Str8PrintFmt(frame.file_name), frame.line_number, DN_Str8PrintFmt(frame.function_name));
}

DN_API void DN_StackTraceReloadSymbols()
{
#if defined(DN_OS_WIN32)
  HANDLE process = GetCurrentProcess();
  SymRefreshModuleList(process);
#endif
}
