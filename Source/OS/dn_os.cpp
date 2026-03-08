#define DN_OS_CPP

#if defined(_CLANGD)
  #define DN_H_WITH_OS 1
  #define DN_H_WITH_CORE 1
  #include "../dn.h"
#endif

#if defined(DN_PLATFORM_POSIX)
#include <sys/sysinfo.h> // get_nprocs
#include <unistd.h>      // getpagesize
#endif

static void *DN_ArenaBasicAllocFromOSHeap(DN_USize size)
{
  void *result = DN_OS_MemAlloc(size, DN_ZMem_Yes);
  return result;
}

DN_API DN_ArenaMemFuncs DN_ArenaMemFuncsGet(DN_ArenaMemFuncType type)
{
  DN_ArenaMemFuncs result = {};
  result.type = type;
  switch (type) {
    case DN_ArenaMemFuncType_Nil: break;
    case DN_ArenaMemFuncType_Basic: {
      result.type          = DN_ArenaMemFuncType_Basic;
      result.basic_alloc   = DN_ArenaBasicAllocFromOSHeap;
      result.basic_dealloc = DN_OS_MemDealloc;
    };

    case DN_ArenaMemFuncType_VMem: {
      DN_Assert(g_dn_->init_flags & DN_InitFlags_OS);
      result.type           = DN_ArenaMemFuncType_VMem;
      result.vmem_page_size = g_dn_->os.page_size;
      result.vmem_reserve   = DN_OS_MemReserve;
      result.vmem_commit    = DN_OS_MemCommit;
      result.vmem_release   = DN_OS_MemRelease;
    } break;
  }
  return result;
}

DN_API DN_ArenaMemFuncs DN_ArenaMemFuncsGetDefaults()
{
  DN_ArenaMemFuncType type = DN_ArenaMemFuncType_Basic;
  if (g_dn_->os_init) {
    #if !defined(DN_PLATFORM_EMSCRIPTEN)
    type = DN_ArenaMemFuncType_VMem;
    #endif
  }
  DN_ArenaMemFuncs result = DN_ArenaMemFuncsGet(type);
  return result;
}

DN_API DN_Arena DN_ArenaFromHeap(DN_U64 size, DN_ArenaFlags flags)
{
  DN_ArenaMemFuncs mem_funcs = {};
  mem_funcs.type             = DN_ArenaMemFuncType_Basic;
  mem_funcs.basic_alloc      = DN_ArenaBasicAllocFromOSHeap;
  mem_funcs.basic_dealloc    = DN_OS_MemDealloc;
  DN_Arena result            = DN_ArenaFromMemFuncs(size, size, flags, mem_funcs);
  return result;
}

DN_API DN_Arena DN_ArenaFromVMem(DN_U64 reserve, DN_U64 commit, DN_ArenaFlags flags)
{
  DN_ArenaMemFuncs mem_funcs = {};
  mem_funcs.type             = DN_ArenaMemFuncType_VMem;
  mem_funcs.vmem_page_size   = g_dn_->os.page_size;
  mem_funcs.vmem_reserve     = DN_OS_MemReserve;
  mem_funcs.vmem_commit      = DN_OS_MemCommit;
  mem_funcs.vmem_release     = DN_OS_MemRelease;
  DN_Arena result            = DN_ArenaFromMemFuncs(reserve, commit, flags, mem_funcs);
  return result;
}

DN_API DN_Str8 DN_Str8FromHeapF(DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_USize size   = DN_FmtVSize(fmt, args);
  DN_Str8  result = DN_Str8FromHeap(size, DN_ZMem_No);
  DN_VSNPrintF(result.data, DN_Cast(int)(result.size + 1), fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8 DN_Str8FromHeap(DN_USize size, DN_ZMem z_mem)
{
  DN_Str8 result = {};
  result.data    = DN_Cast(char *)DN_OS_MemAlloc(size + 1, z_mem);
  if (result.data) {
    result.size              = size;
    result.data[result.size] = 0;
  }
  return result;
}

DN_API DN_Str8 DN_Str8PadNewLines(DN_Arena *arena, DN_Str8 src, DN_Str8 pad)
{
  // TODO: Implement this without requiring TLS so it can go into base strings
  DN_TCScratch        scratch = DN_TCScratchBegin(&arena, 1);
  DN_Str8Builder      builder = DN_Str8BuilderFromArena(scratch.arena);
  DN_Str8BSplitResult split   = DN_Str8BSplit(src, DN_Str8Lit("\n"));
  while (split.lhs.size) {
    DN_Str8BuilderAppendRef(&builder, pad);
    DN_Str8BuilderAppendRef(&builder, split.lhs);
    split = DN_Str8BSplit(split.rhs, DN_Str8Lit("\n"));
    if (split.lhs.size)
      DN_Str8BuilderAppendRef(&builder, DN_Str8Lit("\n"));
  }
  DN_Str8 result = DN_Str8BuilderBuild(&builder, arena);
  DN_TCScratchEnd(&scratch);
  return result;
}

DN_API DN_Str8 DN_Str8BuilderBuildFromHeap(DN_Str8Builder const *builder)
{
  DN_Str8 result = DN_ZeroInit;
  if (!builder || builder->string_size <= 0 || builder->count <= 0)
    return result;

  result.data = DN_Cast(char *) DN_OS_MemAlloc(builder->string_size + 1, DN_ZMem_No);
  if (!result.data)
    return result;

  for (DN_Str8Link *link = builder->head; link; link = link->next) {
    DN_Memcpy(result.data + result.size, link->string.data, link->string.size);
    result.size += link->string.size;
  }

  result.data[result.size] = 0;
  DN_Assert(result.size == builder->string_size);
  return result;
}

DN_API void DN_OS_LogPrint(DN_LogTypeParam type, void *user_data, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_Assert(user_data);
  DN_OSCore *os = DN_Cast(DN_OSCore *)user_data;

  // NOTE: Open log file for appending if requested
  DN_TicketMutex_Begin(&os->log_file_mutex);
  if (os->log_to_file && !os->log_file.handle && !os->log_file.error) {
    DN_TCScratch scratch  = DN_TCScratchBegin(nullptr, 0);
    DN_Str8      exe_dir  = DN_OS_EXEDir(scratch.arena);
    DN_Str8      log_path = DN_OS_PathF(scratch.arena, "%.*s/dn.log", DN_Str8PrintFmt(exe_dir));
    os->log_file          = DN_OS_FileOpen(log_path, DN_OSFileOpen_CreateAlways, DN_OSFileAccess_AppendOnly, nullptr);
    DN_TCScratchEnd(&scratch);
  }
  DN_TicketMutex_End(&os->log_file_mutex);

  DN_LogStyle style = {};
  if (!os->log_no_colour) {
    style.colour = true;
    style.bold   = DN_LogBold_Yes;
    if (type.is_u32_enum) {
      switch (type.u32) {
        case DN_LogType_Debug: {
          style.colour = false;
          style.bold   = DN_LogBold_No;
        } break;

        case DN_LogType_Info: {
          style.g = 0x87;
          style.b = 0xff;
        } break;

        case DN_LogType_Warning: {
          style.r = 0xff;
          style.g = 0xff;
        } break;

        case DN_LogType_Error: {
          style.r = 0xff;
        } break;
      }
    }
  }

  DN_Date    os_date  = DN_OS_DateLocalTimeNow();
  DN_LogDate log_date = {};
  log_date.year       = os_date.year;
  log_date.month      = os_date.month;
  log_date.day        = os_date.day;
  log_date.hour       = os_date.hour;
  log_date.minute     = os_date.minutes;
  log_date.second     = os_date.seconds;

  char             prefix_buffer[128] = {};
  DN_LogPrefixSize prefix_size        = DN_LogMakePrefix(style, type, call_site, log_date, prefix_buffer, sizeof(prefix_buffer));

  va_list args_copy;
  va_copy(args_copy, args);
  DN_TicketMutex_Begin(&os->log_file_mutex);
  {
    DN_OS_FileWrite(&os->log_file, DN_Str8FromPtr(prefix_buffer, prefix_size.size), nullptr);
    DN_OS_FileWriteF(&os->log_file, nullptr, "%*s ", DN_Cast(int)prefix_size.padding, "");
    DN_OS_FileWriteFV(&os->log_file, nullptr, fmt, args_copy);
    DN_OS_FileWrite(&os->log_file, DN_Str8Lit("\n"), nullptr);
  }
  DN_TicketMutex_End(&os->log_file_mutex);
  va_end(args_copy);

  DN_OSPrintDest dest = (type.is_u32_enum && type.u32 == DN_LogType_Error) ? DN_OSPrintDest_Err : DN_OSPrintDest_Out;
  DN_OS_Print(dest, DN_Str8FromPtr(prefix_buffer, prefix_size.size));
  DN_OS_PrintF(dest, "%*s ", DN_Cast(int)prefix_size.padding, "");
  DN_OS_PrintLnFV(dest, fmt, args);
}

DN_API void DN_OS_SetLogPrintFuncToOS()
{
  DN_LogSetPrintFunc(DN_OS_LogPrint, &g_dn_->os);
}

// NOTE: Date
DN_API DN_Str8x32 DN_OS_DateLocalTimeStr8(DN_Date time, char date_separator, char hms_separator)
{
  DN_Str8x32 result = DN_Str8x32FromFmt("%hu%c%02hhu%c%02hhu %02hhu%c%02hhu%c%02hhu",
                                        time.year,
                                        date_separator,
                                        time.month,
                                        date_separator,
                                        time.day,
                                        time.hour,
                                        hms_separator,
                                        time.minutes,
                                        hms_separator,
                                        time.seconds);
  return result;
}

DN_API DN_Str8x32 DN_OS_DateLocalTimeStr8Now(char date_separator, char hms_separator)
{
  DN_Date    time   = DN_OS_DateLocalTimeNow();
  DN_Str8x32 result = DN_OS_DateLocalTimeStr8(time, date_separator, hms_separator);
  return result;
}

// NOTE: Other
DN_API DN_Str8 DN_OS_EXEDir(DN_Arena *arena)
{
  DN_Str8 result = {};
  if (!arena)
    return result;
  DN_TCScratch        scratch              = DN_TCScratchBegin(&arena, 1);
  DN_Str8             exe_path          = DN_OS_EXEPath(scratch.arena);
  DN_Str8             separators[]      = {DN_Str8Lit("/"), DN_Str8Lit("\\")};
  DN_Str8BSplitResult split             = DN_Str8BSplitLastArray(exe_path, separators, DN_ArrayCountU(separators));
  result                                = DN_Str8FromStr8Arena(arena, split.lhs);
  DN_TCScratchEnd(&scratch);
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

DN_API uint64_t DN_OS_EstimateTSCPerSecond(uint64_t duration_ms_to_gauge_tsc_frequency)
{
  uint64_t os_frequency      = DN_OS_PerfCounterFrequency();
  uint64_t os_target_elapsed = duration_ms_to_gauge_tsc_frequency * os_frequency / 1000ULL;
  uint64_t tsc_begin         = DN_CPUGetTSC();
  uint64_t result            = 0;
  if (tsc_begin) {
    uint64_t os_elapsed = 0;
    for (uint64_t os_begin = DN_OS_PerfCounterNow(); os_elapsed < os_target_elapsed;)
      os_elapsed = DN_OS_PerfCounterNow() - os_begin;
    uint64_t tsc_end     = DN_CPUGetTSC();
    uint64_t tsc_elapsed = tsc_end - tsc_begin;
    result               = tsc_elapsed / os_elapsed * os_frequency;
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

DN_API bool DN_OS_FileWrite(DN_OSFile *file, DN_Str8 buffer, DN_ErrSink *error)
{
  bool result = DN_OS_FileWritePtr(file, buffer.data, buffer.size, error);
  return result;
}

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

DN_API DN_Str8 DN_OS_FileReadAll(DN_Allocator alloc_type, void *allocator, DN_Str8 path, DN_ErrSink *err)
{
  // NOTE: Query file size
  DN_Str8       result    = {};
  DN_OSPathInfo path_info = DN_OS_PathInfo(path);
  if (!path_info.exists) {
    DN_ErrSinkAppendF(err, 1, "File does not exist/could not be queried for reading '%.*s'", DN_Str8PrintFmt(path));
    return result;
  }

  // NOTE: Allocate
  DN_ArenaTempMem arena_tmp = {};
  if (alloc_type == DN_Allocator_Arena) {
    DN_Arena *arena = DN_Cast(DN_Arena *) allocator;
    arena_tmp       = DN_ArenaTempMemBegin(arena);
    result          = DN_Str8FromArena(arena, path_info.size, DN_ZMem_No);
  } else {
    DN_Pool *pool = DN_Cast(DN_Pool *) allocator;
    result        = DN_Str8FromPool(pool, path_info.size);
  }

  if (!result.data) {
    DN_Str8x32 bytes_str = DN_ByteCountStr8x32(path_info.size);
    DN_ErrSinkAppendF(err, 1 /*err_code*/, "Failed to allocate %.*s for reading file '%.*s'", DN_Str8PrintFmt(bytes_str), DN_Str8PrintFmt(path));
    return result;
  }

  // NOTE: Read all
  DN_OSFile     file = DN_OS_FileOpen(path, DN_OSFileOpen_OpenIfExist, DN_OSFileAccess_Read, err);
  DN_OSFileRead read = DN_OS_FileRead(&file, result.data, result.size, err);
  if (file.error || !read.success) {
    if (alloc_type == DN_Allocator_Arena) {
      DN_ArenaTempMemEnd(arena_tmp);
    } else {
      DN_Pool *pool = DN_Cast(DN_Pool *) allocator;
      DN_PoolDealloc(pool, result.data);
    }
    result = {};
  }

  DN_OS_FileClose(&file);
  return result;
}

DN_API DN_Str8 DN_OS_FileReadAllArena(DN_Arena *arena, DN_Str8 path, DN_ErrSink *err)
{
  DN_Str8 result = DN_OS_FileReadAll(DN_Allocator_Arena, arena, path, err);
  return result;
}

DN_API DN_Str8 DN_OS_FileReadAllPool(DN_Pool *pool, DN_Str8 path, DN_ErrSink *err)
{
  DN_Str8 result = DN_OS_FileReadAll(DN_Allocator_Pool, pool, path, err);
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
  DN_TCScratch scratch   = DN_TCScratchBegin(nullptr, 0);
  DN_Str8      buffer = DN_Str8FromFmtVArena(scratch.arena, fmt, args);
  bool         result = DN_OS_FileWriteAll(file_path, buffer, error);
  DN_TCScratchEnd(&scratch);
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
  DN_TCScratch scratch     = DN_TCScratchBegin(nullptr, 0);
  DN_Str8      tmp_path = DN_Str8FromFmtArena(scratch.arena, "%.*s.tmp", DN_Str8PrintFmt(path));
  if (!DN_OS_FileWriteAll(tmp_path, buffer, error)) {
    DN_TCScratchEnd(&scratch);
    return false;
  }
  if (!DN_OS_FileCopy(tmp_path, path, true /*overwrite*/, error)) {
    DN_TCScratchEnd(&scratch);
    return false;
  }
  if (!DN_OS_PathDelete(tmp_path)) {
    DN_TCScratchEnd(&scratch);
    return false;
  }
  DN_TCScratchEnd(&scratch);
  return true;
}

DN_API bool DN_OS_FileWriteAllSafeFV(DN_Str8 path, DN_ErrSink *error, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_TCScratch scratch   = DN_TCScratchBegin(nullptr, 0);
  DN_Str8    buffer = DN_Str8FromFmtVArena(scratch.arena, fmt, args);
  bool       result = DN_OS_FileWriteAllSafe(path, buffer, error);
  DN_TCScratchEnd(&scratch);
  return result;
}

DN_API bool DN_OS_FileWriteAllSafeF(DN_Str8 path, DN_ErrSink *error, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  bool result = DN_OS_FileWriteAllSafeFV(path, error, fmt, args);
  return result;
}

DN_API bool DN_OS_PathAddRef(DN_Arena *arena, DN_OSPath *fs_path, DN_Str8 path)
{
  if (!arena || !fs_path || path.size == 0)
    return false;

  if (path.size <= 0)
    return true;

  DN_Str8 const delimiter_array[] = {
      DN_Str8Lit("\\"),
      DN_Str8Lit("/")};

  if (fs_path->links_size == 0)
    fs_path->has_prefix_path_separator = (path.data[0] == '/');

  for (;;) {
    DN_Str8BSplitResult delimiter = DN_Str8BSplitArray(path, delimiter_array, DN_ArrayCountU(delimiter_array));
    for (; delimiter.lhs.data; delimiter = DN_Str8BSplitArray(delimiter.rhs, delimiter_array, DN_ArrayCountU(delimiter_array))) {
      if (delimiter.lhs.size <= 0)
        continue;

      DN_OSPathLink *link = DN_ArenaNew(arena, DN_OSPathLink, DN_ZMem_Yes);
      if (!link)
        return false;

      link->string = delimiter.lhs;
      link->prev   = fs_path->tail;
      if (fs_path->tail)
        fs_path->tail->next = link;
      else
        fs_path->head = link;
      fs_path->tail = link;
      fs_path->links_size += 1;
      fs_path->string_size += delimiter.lhs.size;
    }

    if (!delimiter.lhs.data)
      break;
  }

  return true;
}

DN_API bool DN_OS_PathAdd(DN_Arena *arena, DN_OSPath *fs_path, DN_Str8 path)
{
  DN_Str8 copy   = DN_Str8FromStr8Arena(arena, path);
  bool    result = copy.size ? true : DN_OS_PathAddRef(arena, fs_path, copy);
  return result;
}

DN_API bool DN_OS_PathAddF(DN_Arena *arena, DN_OSPath *fs_path, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8 path = DN_Str8FromFmtVArena(arena, fmt, args);
  va_end(args);
  bool result = DN_OS_PathAddRef(arena, fs_path, path);
  return result;
}

DN_API bool DN_OS_PathPop(DN_OSPath *fs_path)
{
  if (!fs_path)
    return false;

  if (fs_path->tail) {
    DN_Assert(fs_path->head);
    fs_path->links_size -= 1;
    fs_path->string_size -= fs_path->tail->string.size;
    fs_path->tail = fs_path->tail->prev;
    if (fs_path->tail)
      fs_path->tail->next = nullptr;
    else
      fs_path->head = nullptr;
  } else {
    DN_Assert(!fs_path->head);
  }

  return true;
}

DN_API DN_Str8 DN_OS_PathTo(DN_Arena *arena, DN_Str8 path, DN_Str8 path_separator)
{
  DN_OSPath fs_path = {};
  DN_OS_PathAddRef(arena, &fs_path, path);
  DN_Str8 result = DN_OS_PathBuildWithSeparator(arena, &fs_path, path_separator);
  return result;
}

DN_API DN_Str8 DN_OS_PathToF(DN_Arena *arena, DN_Str8 path_separator, DN_FMT_ATTRIB char const *fmt, ...)
{
  DN_TCScratch scratch = DN_TCScratchBegin(&arena, 1);
  va_list    args;
  va_start(args, fmt);
  DN_Str8 path = DN_Str8FromFmtVArena(scratch.arena, fmt, args);
  va_end(args);
  DN_Str8 result = DN_OS_PathTo(arena, path, path_separator);
  DN_TCScratchEnd(&scratch);
  return result;
}

DN_API DN_Str8 DN_OS_Path(DN_Arena *arena, DN_Str8 path)
{
  DN_Str8 result = DN_OS_PathTo(arena, path, DN_OSPathSeperatorString);
  return result;
}

DN_API DN_Str8 DN_OS_PathF(DN_Arena *arena, DN_FMT_ATTRIB char const *fmt, ...)
{
  DN_TCScratch scratch = DN_TCScratchBegin(&arena, 1);
  va_list    args;
  va_start(args, fmt);
  DN_Str8 path = DN_Str8FromFmtVArena(scratch.arena, fmt, args);
  va_end(args);
  DN_Str8 result = DN_OS_Path(arena, path);
  DN_TCScratchEnd(&scratch);
  return result;
}

DN_API DN_Str8 DN_OS_PathBuildWithSeparator(DN_Arena *arena, DN_OSPath const *fs_path, DN_Str8 path_separator)
{
  DN_Str8 result = {};
  if (!fs_path || fs_path->links_size <= 0)
    return result;

  // NOTE: Each link except the last one needs the path separator appended to it, '/' or '\\'
  DN_USize string_size = (fs_path->has_prefix_path_separator ? path_separator.size : 0) + fs_path->string_size + ((fs_path->links_size - 1) * path_separator.size);
  result               = DN_Str8FromArena(arena, string_size, DN_ZMem_No);
  if (result.data) {
    char *dest = result.data;
    if (fs_path->has_prefix_path_separator) {
      DN_Memcpy(dest, path_separator.data, path_separator.size);
      dest += path_separator.size;
    }

    for (DN_OSPathLink *link = fs_path->head; link; link = link->next) {
      DN_Str8 string = link->string;
      DN_Memcpy(dest, string.data, string.size);
      dest += string.size;

      if (link != fs_path->tail) {
        DN_Memcpy(dest, path_separator.data, path_separator.size);
        dest += path_separator.size;
      }
    }
  }

  result.data[string_size] = 0;
  return result;
}

// NOTE: DN_OSExec
DN_API DN_OSExecResult DN_OS_Exec(DN_Slice<DN_Str8> cmd_line,
                                  DN_OSExecArgs    *args,
                                  DN_Arena         *arena,
                                  DN_ErrSink       *error)
{
  DN_OSExecAsyncHandle async_handle = DN_OS_ExecAsync(cmd_line, args, error);
  DN_OSExecResult      result       = DN_OS_ExecWait(async_handle, arena, error);
  return result;
}

DN_API DN_OSExecResult DN_OS_ExecOrAbort(DN_Slice<DN_Str8> cmd_line, DN_OSExecArgs *args, DN_Arena *arena)
{
  DN_ErrSink     *error  = DN_TCErrSinkBegin(DN_ErrSinkMode_Nil);
  DN_OSExecResult result = DN_OS_Exec(cmd_line, args, arena, error);
  if (result.os_error_code)
    DN_ErrSinkEndExitIfErrorF(error, result.os_error_code, "OS failed to execute the requested command returning the error code %u", result.os_error_code);

  if (result.exit_code)
    DN_ErrSinkEndExitIfErrorF(error, result.exit_code, "OS executed command and returned non-zero exit code %u", result.exit_code);
  DN_ErrSinkEndIgnore(error);
  return result;
}

// NOTE: DN_OSThread
static void DN_OS_ThreadExecute_(void *user_context)
{
  DN_OSThread     *thread    = DN_Cast(DN_OSThread *) user_context;
  DN_ArenaMemFuncs mem_funcs = DN_ArenaMemFuncsGetDefaults();
  DN_TCInitFromMemFuncs(&thread->context, thread->thread_id, /*args=*/nullptr, mem_funcs);
  DN_TCEquip(&thread->context);
  if (thread->is_lane_set)
    DN_TCLaneEquip(thread->lane);
  DN_OS_SemaphoreWait(&thread->init_semaphore, DN_OS_SEMAPHORE_INFINITE_TIMEOUT);
  thread->func(thread);
}

DN_API void DN_OS_ThreadSetNameFmt(char const *fmt, ...)
{
  DN_TCCore *tls = DN_TCGet();
  va_list args;
  va_start(args, fmt);
  tls->name = DN_Str8x64FromFmtV(fmt, args);
  va_end(args);

  DN_Str8 name = DN_Str8FromPtr(tls->name.data, tls->name.size);
#if defined(DN_PLATFORM_WIN32)
  DN_OS_W32ThreadSetName(name);
#else
  DN_OS_PosixThreadSetName(name);
#endif
}

// NOTE: DN_OSHttp /////////////////////////////////////////////////////////////////////////////////
DN_API void DN_OS_HttpRequestWait(DN_OSHttpResponse *response)
{
  if (response && response->on_complete_semaphore.handle != 0)
    DN_OS_SemaphoreWait(&response->on_complete_semaphore, DN_OS_SEMAPHORE_INFINITE_TIMEOUT);
}

DN_API DN_OSHttpResponse DN_OS_HttpRequest(DN_Arena *arena, DN_Str8 host, DN_Str8 path, DN_OSHttpRequestSecure secure, DN_Str8 method, DN_Str8 body, DN_Str8 headers)
{
  // TODO(doyle): Revise the memory allocation and its lifetime
  DN_OSHttpResponse result = {};
  DN_TCScratch        scratch   = DN_TCScratchBegin(&arena, 1);
  result.scratch_arena        = scratch.arena;

  DN_OS_HttpRequestAsync(&result, arena, host, path, secure, method, body, headers);
  DN_OS_HttpRequestWait(&result);
  DN_TCScratchEnd(&scratch);
  return result;
}

// NOTE: DN_OSPrint
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
  DN_Assert(string.size < DN_Cast(unsigned long) - 1);
  unsigned long bytes_written = 0;
  (void)bytes_written;
  if (print_to_console)
    WriteConsoleA(print_handle, string.data, DN_Cast(unsigned long) string.size, &bytes_written, nullptr);
  else
    WriteFile(print_handle, string.data, DN_Cast(unsigned long) string.size, &bytes_written, nullptr);
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
  if (string.data && string.size) {
    if (style.colour)
      DN_OS_Print(dest, DN_LogColourEscapeCodeStr8FromRGB(DN_LogColourType_Fg, style.r, style.g, style.b));
    if (style.bold == DN_LogBold_Yes)
      DN_OS_Print(dest, DN_Str8Lit(DN_LogBoldEscapeCode));
    DN_OS_Print(dest, string);
    if (style.colour || style.bold == DN_LogBold_Yes)
      DN_OS_Print(dest, DN_Str8Lit(DN_LogResetEscapeCode));
  }
}

static char *DN_OS_PrintVSPrintfChunker_(const char *buf, void *user, int len)
{
  DN_Str8 string = {};
  string.data    = DN_Cast(char *) buf;
  string.size    = len;

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
    if (style.colour)
      DN_OS_Print(dest, DN_LogColourEscapeCodeStr8FromRGB(DN_LogColourType_Fg, style.r, style.g, style.b));
    if (style.bold == DN_LogBold_Yes)
      DN_OS_Print(dest, DN_Str8Lit(DN_LogBoldEscapeCode));
    DN_OS_PrintFV(dest, fmt, args);
    if (style.colour || style.bold == DN_LogBold_Yes)
      DN_OS_Print(dest, DN_Str8Lit(DN_LogResetEscapeCode));
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

// NOTE: DN_VArray
template <typename T>
DN_VArray<T> DN_OS_VArrayInitByteSize(DN_USize byte_size)
{
  DN_VArray<T> result = {};
  result.data         = DN_Cast(T *) DN_OS_MemReserve(byte_size, DN_MemCommit_No, DN_MemPage_ReadWrite);
  if (result.data)
    result.max = byte_size / sizeof(T);
  return result;
}

template <typename T>
DN_VArray<T> DN_OS_VArrayInit(DN_USize max)
{
  DN_VArray<T> result = DN_OS_VArrayInitByteSize<T>(max * sizeof(T));
  DN_Assert(result.max >= max);
  return result;
}

template <typename T>
DN_VArray<T> DN_OS_VArrayInitSlice(DN_Slice<T> slice, DN_USize max)
{
  DN_USize     real_max = DN_Max(slice.size, max);
  DN_VArray<T> result   = DN_OS_VArrayInit<T>(real_max);
  if (DN_OS_VArrayIsValid(&result))
    DN_OS_VArrayAddArray(&result, slice.data, slice.size);
  return result;
}

template <typename T, DN_USize N>
DN_VArray<T> DN_OS_VArrayInitCArray(T const (&items)[N], DN_USize max)
{
  DN_USize     real_max = DN_Max(N, max);
  DN_VArray<T> result   = DN_OS_VArrayInitSlice(DN_Slice_Init(items, N), real_max);
  return result;
}

template <typename T>
void DN_OS_VArrayDeinit(DN_VArray<T> *array)
{
  DN_OS_MemRelease(array->data, array->max * sizeof(T));
  *array = {};
}

template <typename T>
bool DN_OS_VArrayIsValid(DN_VArray<T> const *array)
{
  bool result = array->data && array->size <= array->max;
  return result;
}

template <typename T>
T *DN_OS_VArrayAddArray(DN_VArray<T> *array, T const *items, DN_USize count)
{
  T *result = DN_OS_VArrayMakeArray(array, count, DN_ZMem_No);
  if (result)
    DN_Memcpy(result, items, count * sizeof(T));
  return result;
}

template <typename T, DN_USize N>
T *DN_OS_VArrayAddCArray(DN_VArray<T> *array, T const (&items)[N])
{
  T *result = DN_OS_VArrayAddArray(array, items, N);
  return result;
}

template <typename T>
T *DN_OS_VArrayAdd(DN_VArray<T> *array, T const &item)
{
  T *result = DN_OS_VArrayAddArray(array, &item, 1);
  return result;
}

template <typename T>
T *DN_OS_VArrayMakeArray(DN_VArray<T> *array, DN_USize count, DN_ZMem z_mem)
{
  if (!DN_OS_VArrayIsValid(array))
    return nullptr;

  if (!DN_CheckF((array->size + count) < array->max, "Array is out of space (user requested +%zu items, array has %zu/%zu items)", count, array->size, array->max))
    return nullptr;

  if (!DN_OS_VArrayReserve(array, count))
    return nullptr;

  // TODO: Use placement new
  T *result = array->data + array->size;
  array->size += count;
  if (z_mem == DN_ZMem_Yes)
    DN_Memset(result, 0, count * sizeof(T));
  return result;
}

template <typename T>
T *DN_OS_VArrayMake(DN_VArray<T> *array, DN_ZMem z_mem)
{
  T *result = DN_OS_VArrayMakeArray(array, 1, z_mem);
  return result;
}

template <typename T>
T *DN_OS_VArrayInsertArray(DN_VArray<T> *array, DN_USize index, T const *items, DN_USize count)
{
  T *result = nullptr;
  if (!DN_OS_VArrayIsValid(array))
    return result;
  if (DN_OS_VArrayReserve(array, array->size + count))
    result = DN_CArrayInsertArray(array->data, &array->size, array->max, index, items, count);
  return result;
}

template <typename T, DN_USize N>
T *DN_OS_VArrayInsertCArray(DN_VArray<T> *array, DN_USize index, T const (&items)[N])
{
  T *result = DN_OS_VArrayInsertArray(array, index, items, N);
  return result;
}

template <typename T>
T *DN_OS_VArrayInsert(DN_VArray<T> *array, DN_USize index, T const &item)
{
  T *result = DN_OS_VArrayInsertArray(array, index, &item, 1);
  return result;
}

template <typename T>
T *DN_OS_VArrayPopFront(DN_VArray<T> *array, DN_USize count)
{
  T *result = DN_Cast(T *)DN_CArrayPopFront(array->data, &array->size, sizeof(T), count);
  return result;
}

template <typename T>
T *DN_OS_VArrayPopBack(DN_VArray<T> *array, DN_USize count)
{
  T *result = DN_Cast(T *)DN_CArrayPopBack(array->data, &array->size, sizeof(T), count);
  return result;
}

template <typename T>
DN_ArrayEraseResult DN_OS_VArrayEraseRange(DN_VArray<T> *array, DN_USize begin_index, DN_ISize count, DN_ArrayErase erase)
{
  DN_ArrayEraseResult result = {};
  if (!DN_OS_VArrayIsValid(array))
    return result;
  result = DN_CArrayEraseRange(array->data, &array->size, sizeof(T), begin_index, count, erase);
  return result;
}

template <typename T>
void DN_OS_VArrayClear(DN_VArray<T> *array, DN_ZMem z_mem)
{
  if (array) {
    if (z_mem == DN_ZMem_Yes)
      DN_Memset(array->data, 0, array->size * sizeof(T));
    array->size = 0;
  }
}

template <typename T>
bool DN_OS_VArrayReserve(DN_VArray<T> *array, DN_USize count)
{
  if (!DN_OS_VArrayIsValid(array) || count == 0)
    return false;

  DN_USize real_commit    = (array->size + count) * sizeof(T);
  DN_USize aligned_commit = DN_AlignUpPowerOfTwo(real_commit, g_dn_->os.page_size);
  if (array->commit >= aligned_commit)
    return true;
  bool result   = DN_OS_MemCommit(array->data, aligned_commit, DN_MemPage_ReadWrite);
  array->commit = aligned_commit;
  return result;
}

// NOTE: Stack Trace
DN_API DN_StackTraceWalkResult DN_StackTraceWalk(DN_Arena *arena, DN_U16 limit)
{
  DN_StackTraceWalkResult result = {};
#if defined(DN_OS_WIN32)
  if (!arena)
    return result;

  static DN_TicketMutex mutex = {};
  DN_TicketMutex_Begin(&mutex);

  HANDLE thread  = GetCurrentThread();
  result.process = GetCurrentProcess();

  DN_OSW32Core *w32 = DN_OS_W32GetCore();
  if (!w32->sym_initialised) {
    w32->sym_initialised = true;
    SymSetOptions(SYMOPT_LOAD_LINES);
    if (!SymInitialize(result.process, nullptr /*UserSearchPath*/, true /*fInvadeProcess*/)) {
      DN_TCScratch scratch  = DN_TCScratchBegin(&arena, 1);
      DN_OSW32Error  error = DN_OS_W32LastError(scratch.arena);
      DN_LogErrorF("SymInitialize failed, stack trace can not be generated (%lu): %.*s\n", error.code, DN_Str8PrintFmt(error.msg));
      DN_TCScratchEnd(&scratch);
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
  DN_TicketMutex_End(&mutex);

  result.base_addr = DN_ArenaNewArray(arena, DN_U64, raw_frames_count, DN_ZMem_No);
  result.size      = DN_Cast(DN_U16) raw_frames_count;
  DN_Memcpy(result.base_addr, raw_frames, raw_frames_count * sizeof(raw_frames[0]));
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

DN_API DN_Str8 DN_StackTraceWalkResultToStr8(DN_Arena *arena, DN_StackTraceWalkResult const *walk, DN_U16 skip)
{
  DN_Str8 result{};
  if (!walk || !arena)
    return result;

  DN_TCScratch   scratch    = DN_TCScratchBegin(&arena, 1);
  DN_Str8Builder builder = DN_Str8BuilderFromArena(scratch.arena);
  DN_StackTraceAddWalkToStr8Builder(walk, &builder, skip);
  result = DN_Str8BuilderBuild(&builder, arena);
  DN_TCScratchEnd(&scratch);
  return result;
}

DN_API DN_Str8 DN_StackTraceWalkStr8(DN_Arena *arena, DN_U16 limit, DN_U16 skip)
{
  DN_TCScratch            scratch   = DN_TCScratchBegin(&arena, 1);
  DN_StackTraceWalkResult walk   = DN_StackTraceWalk(scratch.arena, limit);
  DN_Str8                 result = DN_StackTraceWalkResultToStr8(arena, &walk, skip);
  DN_TCScratchEnd(&scratch);
  return result;
}

DN_API DN_Str8 DN_StackTraceWalkStr8FromHeap(DN_U16 limit, DN_U16 skip)
{
  // NOTE: We don't use WalkResultToStr8 because that uses the TLS arenas which
  // does not use the OS heap.
  DN_Arena                arena   = DN_ArenaFromHeap(DN_Kilobytes(64), DN_ArenaFlags_NoAllocTrack);
  DN_Str8Builder          builder = DN_Str8BuilderFromArena(&arena);
  DN_StackTraceWalkResult walk    = DN_StackTraceWalk(&arena, limit);
  DN_StackTraceAddWalkToStr8Builder(&walk, &builder, skip);
  DN_Str8 result = DN_Str8BuilderBuildFromHeap(&builder);
  DN_ArenaDeinit(&arena);
  return result;
}

DN_API DN_StackTraceFrameSlice DN_StackTraceGetFrames(DN_Arena *arena, DN_U16 limit)
{
  DN_StackTraceFrameSlice result = {};
  if (!arena)
    return result;

  DN_TCScratch            scratch = DN_TCScratchBegin(&arena, 1);
  DN_StackTraceWalkResult walk    = DN_StackTraceWalk(scratch.arena, limit);
  if (walk.size) {
    if (DN_ISliceAllocArena(DN_StackTraceFrameSlice, &result, walk.size, DN_ZMem_No, arena)) {
      DN_USize slice_index = 0;
      for (DN_StackTraceWalkResultIterator it = {}; DN_StackTraceWalkResultIterate(&it, &walk);)
        result.data[slice_index++] = DN_StackTraceRawFrameToFrame(arena, it.raw_frame);
    }
  }
  DN_TCScratchEnd(&scratch);
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

  DN_Str16 file_name16     = DN_Str16FromPtr(line.FileName, DN_CStr16Size(line.FileName));
  DN_Str16 function_name16 = DN_Str16FromPtr(symbol->Name, symbol->NameLen);

  DN_StackTraceFrame result = {};
  result.address            = raw_frame.base_addr;
  result.line_number        = line.LineNumber;
  result.file_name          = DN_OS_W32Str16ToStr8(arena, file_name16);
  result.function_name      = DN_OS_W32Str16ToStr8(arena, function_name16);

  if (result.function_name.size == 0)
    result.function_name = DN_Str8Lit("<unknown function>");
  if (result.file_name.size == 0)
    result.file_name = DN_Str8Lit("<unknown file>");
#else
  DN_StackTraceFrame result = {};
#endif
  return result;
}

DN_API void DN_StackTracePrint(DN_U16 limit)
{
  DN_TCScratch            scratch     = DN_TCScratchBegin(nullptr, 0);
  DN_StackTraceFrameSlice stack_trace = DN_StackTraceGetFrames(scratch.arena, limit);
  for (DN_ForItSize(it, DN_StackTraceFrame, stack_trace.data, stack_trace.count)) {
    DN_StackTraceFrame frame = *it.data;
    DN_OS_PrintErrLnF("%.*s(%I64u): %.*s", DN_Str8PrintFmt(frame.file_name), frame.line_number, DN_Str8PrintFmt(frame.function_name));
  }
  DN_TCScratchEnd(&scratch);
}

DN_API void DN_StackTraceReloadSymbols()
{
#if defined(DN_OS_WIN32)
  HANDLE process = GetCurrentProcess();
  SymRefreshModuleList(process);
#endif
}
