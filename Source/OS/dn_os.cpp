#define DN_OS_CPP

#if defined(DN_PLATFORM_POSIX)
#include <sys/sysinfo.h> // get_nprocs
#include <unistd.h>      // getpagesize
#endif

static DN_OSCore *g_dn_os_core_;

static void DN_OS_LOGEmitFromTypeTypeFV_(DN_LOGTypeParam type, void *user_data, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_Assert(user_data);
  DN_OSCore *core = DN_CAST(DN_OSCore *)user_data;

  // NOTE: Open log file for appending if requested ////////////////////////////////////////////////
  DN_TicketMutex_Begin(&core->log_file_mutex);
  if (core->log_to_file && !core->log_file.handle && !core->log_file.error) {
    DN_OSTLSTMem tmem     = DN_OS_TLSTMem(nullptr);
    DN_MSVC_WARNING_PUSH
    DN_MSVC_WARNING_DISABLE(6284) // Object passed as _Param_(3) when a string is required in call to 'DN_OS_PathF' Actual type: 'struct DN_Str8'.
    DN_Str8      log_path = DN_OS_PathF(tmem.arena, "%S/dn.log", DN_OS_EXEDir(tmem.arena));
    DN_MSVC_WARNING_POP
    core->log_file        = DN_OS_FileOpen(log_path, DN_OSFileOpen_CreateAlways, DN_OSFileAccess_AppendOnly, nullptr);
  }
  DN_TicketMutex_End(&core->log_file_mutex);

  DN_LOGStyle style = {};
  if (!core->log_no_colour) {
    style.colour = true;
    style.bold   = DN_LOGBold_Yes;
    if (type.is_u32_enum) {
      switch (type.u32) {
        case DN_LOGType_Debug: {
          style.colour = false;
          style.bold   = DN_LOGBold_No;
        } break;

        case DN_LOGType_Info: {
          style.g = 0x87;
          style.b = 0xff;
        } break;

        case DN_LOGType_Warning: {
          style.r = 0xff;
          style.g = 0xff;
        } break;

        case DN_LOGType_Error: {
          style.r = 0xff;
        } break;
      }
    }
  }

  DN_OSDateTime os_date  = DN_OS_DateLocalTimeNow();
  DN_LOGDate    log_date = {};
  log_date.year          = os_date.year;
  log_date.month         = os_date.month;
  log_date.day           = os_date.day;
  log_date.hour          = os_date.hour;
  log_date.minute        = os_date.minutes;
  log_date.second        = os_date.seconds;

  char             prefix_buffer[128] = {};
  DN_LOGPrefixSize prefix_size        = DN_LOG_MakePrefix(style, type, call_site, log_date, prefix_buffer, sizeof(prefix_buffer));

  va_list args_copy;
  va_copy(args_copy, args);
  DN_TicketMutex_Begin(&core->log_file_mutex);
  {
    DN_OS_FileWrite(&core->log_file, DN_Str8_Init(prefix_buffer, prefix_size.size), nullptr);
    DN_OS_FileWriteF(&core->log_file, nullptr, "%*s ", DN_CAST(int)prefix_size.padding, "");
    DN_OS_FileWriteFV(&core->log_file, nullptr, fmt, args_copy);
    DN_OS_FileWrite(&core->log_file, DN_STR8("\n"), nullptr);
  }
  DN_TicketMutex_End(&core->log_file_mutex);
  va_end(args_copy);

  DN_OSPrintDest dest = (type.is_u32_enum && type.u32 == DN_LOGType_Error) ? DN_OSPrintDest_Err : DN_OSPrintDest_Out;
  DN_OS_Print(dest, DN_Str8_Init(prefix_buffer, prefix_size.size));
  DN_OS_PrintF(dest, "%*s ", DN_CAST(int)prefix_size.padding, "");
  DN_OS_PrintLnFV(dest, fmt, args);
}

DN_API void DN_OS_Init(DN_OSCore *os, DN_OSInitArgs *args)
{
  g_dn_os_core_ = os;

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
    os->arena = DN_Arena_FromHeap(DN_Megabytes(1), DN_ArenaFlags_NoAllocTrack);
    #else
    os->arena = DN_Arena_FromVMem(DN_Megabytes(1), DN_Kilobytes(4), DN_ArenaFlags_NoAllocTrack);
    #endif

    #if defined(DN_PLATFORM_WIN32)
    os->platform_context = DN_Arena_New(&os->arena, DN_W32Core, DN_ZeroMem_Yes);
    #elif defined(DN_PLATFORM_POSIX) || defined(DN_PLATFORM_EMSCRIPTEN)
    os->platform_context = DN_Arena_New(&os->arena, DN_POSIXCore, DN_ZeroMem_Yes);
    #endif

    #if defined(DN_PLATFORM_WIN32)
    DN_W32Core *w32 = DN_CAST(DN_W32Core *) os->platform_context;
    InitializeCriticalSection(&w32->sync_primitive_free_list_mutex);

    QueryPerformanceFrequency(&w32->qpc_frequency);
    HMODULE module              = LoadLibraryA("kernel32.dll");
    w32->set_thread_description = DN_CAST(DN_W32SetThreadDescriptionFunc *) GetProcAddress(module, "SetThreadDescription");
    FreeLibrary(module);

    // NOTE: win32 bcrypt
    wchar_t const     BCRYPT_ALGORITHM[] = L"RNG";
    long /*NTSTATUS*/ init_status        = BCryptOpenAlgorithmProvider(&w32->bcrypt_rng_handle, BCRYPT_ALGORITHM, nullptr /*implementation*/, 0 /*flags*/);
    if (w32->bcrypt_rng_handle && init_status == 0)
      w32->bcrypt_init_success = true;
    else
      DN_LOG_ErrorF("Failed to initialise Windows secure random number generator, error: %d", init_status);
    #else
    DN_Posix_Init(DN_CAST(DN_POSIXCore *)os->platform_context);
    #endif
  }

  // NOTE: Initialise tmem arenas which allocate memory and will be
  // recorded to the now initialised allocation table. The initialisation
  // of tmem memory may request tmem memory itself in leak tracing mode.
  // This is supported as the tmem arenas defer allocation tracking until
  // initialisation is done.
  DN_OSTLSInitArgs tls_init_args = {};
  if (args) {
    tls_init_args.commit           = args->tls_commit;
    tls_init_args.reserve          = args->tls_reserve;
    tls_init_args.err_sink_reserve = args->tls_err_sink_reserve;
    tls_init_args.err_sink_commit  = args->tls_err_sink_commit;
  }

  DN_OS_TLSInit(&os->tls, tls_init_args);
  DN_OS_TLSSetCurrentThreadTLS(&os->tls);
  os->cpu_report = DN_CPUGetReport();

  #define DN_CPU_FEAT_XENTRY(label) g_dn_cpu_feature_decl[DN_CPUFeature_##label] = {DN_CPUFeature_##label, DN_STR8(#label)};
  DN_CPU_FEAT_XMACRO
  #undef DN_CPU_FEAT_XENTRY
  DN_Assert(g_dn_os_core_);
}

DN_API void DN_OS_EmitLogsWithOSPrintFunctions(DN_OSCore *os)
{
  DN_Assert(os);
  DN_LOG_SetEmitFromTypeFVFunc(DN_OS_LOGEmitFromTypeTypeFV_, os);
}

DN_API void DN_OS_DumpThreadContextArenaStat(DN_Str8 file_path)
{
#if defined(DN_DEBUG_THREAD_CONTEXT)
  // NOTE: Open a file to write the arena stats to
  FILE *file = nullptr;
  fopen_s(&file, file_path.data, "a+b");
  if (file) {
    DN_LOG_ErrorF("Failed to dump thread context arenas [file=%.*s]", DN_STR_FMT(file_path));
    return;
  }

  // NOTE: Copy the stats from library book-keeping
  // NOTE: Extremely short critical section, copy the stats then do our
  // work on it.
  DN_ArenaStat stats[DN_CArray_CountI(g_dn_core->thread_context_arena_stats)];
  int          stats_size = 0;

  DN_TicketMutex_Begin(&g_dn_core->thread_context_mutex);
  stats_size = g_dn_core->thread_context_arena_stats_count;
  DN_Memcpy(stats, g_dn_core->thread_context_arena_stats, sizeof(stats[0]) * stats_size);
  DN_TicketMutex_End(&g_dn_core->thread_context_mutex);

  // NOTE: Print the cumulative stat
  DN_DateHMSTimeStr now = DN_Date_HMSLocalTimeStrNow();
  fprintf(file,
          "Time=%.*s %.*s | Thread Context Arenas | Count=%d\n",
          now.date_size,
          now.date,
          now.hms_size,
          now.hms,
          g_dn_core->thread_context_arena_stats_count);

  // NOTE: Write the cumulative thread arena data
  {
    DN_ArenaStat stat = {};
    for (DN_USize index = 0; index < stats_size; index++) {
      DN_ArenaStat const *current = stats + index;
      stat.capacity += current->capacity;
      stat.used += current->used;
      stat.wasted += current->wasted;
      stat.blocks += current->blocks;

      stat.capacity_hwm = DN_Max(stat.capacity_hwm, current->capacity_hwm);
      stat.used_hwm     = DN_Max(stat.used_hwm, current->used_hwm);
      stat.wasted_hwm   = DN_Max(stat.wasted_hwm, current->wasted_hwm);
      stat.blocks_hwm   = DN_Max(stat.blocks_hwm, current->blocks_hwm);
    }

    DN_ArenaStatStr stats_string = DN_Arena_StatStr(&stat);
    fprintf(file, "  [ALL] CURR %.*s\n", stats_string.size, stats_string.data);
  }

  // NOTE: Print individual thread arena data
  for (DN_USize index = 0; index < stats_size; index++) {
    DN_ArenaStat const *current        = stats + index;
    DN_ArenaStatStr     current_string = DN_Arena_StatStr(current);
    fprintf(file, "  [%03d] CURR %.*s\n", DN_CAST(int) index, current_string.size, current_string.data);
  }

  fclose(file);
  DN_LOG_InfoF("Dumped thread context arenas [file=%.*s]", DN_STR_FMT(file_path));
#else
  (void)file_path;
#endif // #if defined(DN_DEBUG_THREAD_CONTEXT)
}

// NOTE: Date //////////////////////////////////////////////////////////////////////////////////////
DN_API DN_OSDateTimeStr8 DN_OS_DateLocalTimeStr8(DN_OSDateTime time, char date_separator, char hms_separator)
{
  DN_OSDateTimeStr8 result = {};
  result.hms_size          = DN_CAST(uint8_t) DN_SNPrintF(result.hms,
                                                 DN_ArrayCountI(result.hms),
                                                 "%02hhu%c%02hhu%c%02hhu",
                                                 time.hour,
                                                 hms_separator,
                                                 time.minutes,
                                                 hms_separator,
                                                 time.seconds);

  result.date_size = DN_CAST(uint8_t) DN_SNPrintF(result.date,
                                                  DN_ArrayCountI(result.date),
                                                  "%hu%c%02hhu%c%02hhu",
                                                  time.year,
                                                  date_separator,
                                                  time.month,
                                                  date_separator,
                                                  time.day);

  DN_Assert(result.hms_size < DN_ArrayCountU(result.hms));
  DN_Assert(result.date_size < DN_ArrayCountU(result.date));
  return result;
}

DN_API DN_OSDateTimeStr8 DN_OS_DateLocalTimeStr8Now(char date_separator, char hms_separator)
{
  DN_OSDateTime     time   = DN_OS_DateLocalTimeNow();
  DN_OSDateTimeStr8 result = DN_OS_DateLocalTimeStr8(time, date_separator, hms_separator);
  return result;
}

DN_API bool DN_OS_DateIsValid(DN_OSDateTime date)
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

// NOTE: Other /////////////////////////////////////////////////////////////////////////////////////
DN_API DN_Str8 DN_OS_EXEDir(DN_Arena *arena)
{
  DN_Str8 result = {};
  if (!arena)
    return result;
  DN_OSTLSTMem               tmem         = DN_OS_TLSTMem(arena);
  DN_Str8                  exe_path     = DN_OS_EXEPath(tmem.arena);
  DN_Str8                  separators[] = {DN_STR8("/"), DN_STR8("\\")};
  DN_Str8BSplitResult split        = DN_Str8_BSplitLastArray(exe_path, separators, DN_ArrayCountU(separators));
  result                                = DN_Str8_FromStr8(arena, split.lhs);
  return result;
}

// NOTE: Counters //////////////////////////////////////////////////////////////////////////////////
DN_API DN_F64 DN_OS_PerfCounterS(uint64_t begin, uint64_t end)
{
  uint64_t frequency = DN_OS_PerfCounterFrequency();
  uint64_t ticks     = end - begin;
  DN_F64   result    = ticks / DN_CAST(DN_F64) frequency;
  return result;
}

DN_API DN_F64 DN_OS_PerfCounterMs(uint64_t begin, uint64_t end)
{
  uint64_t frequency = DN_OS_PerfCounterFrequency();
  uint64_t ticks     = end - begin;
  DN_F64   result    = (ticks * 1'000) / DN_CAST(DN_F64) frequency;
  return result;
}

DN_API DN_F64 DN_OS_PerfCounterUs(uint64_t begin, uint64_t end)
{
  uint64_t frequency = DN_OS_PerfCounterFrequency();
  uint64_t ticks     = end - begin;
  DN_F64   result    = (ticks * 1'000'000) / DN_CAST(DN_F64) frequency;
  return result;
}

DN_API DN_F64 DN_OS_PerfCounterNs(uint64_t begin, uint64_t end)
{
  uint64_t frequency = DN_OS_PerfCounterFrequency();
  uint64_t ticks     = end - begin;
  DN_F64   result    = (ticks * 1'000'000'000) / DN_CAST(DN_F64) frequency;
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

#if !defined(DN_NO_OS_FILE_API)
// NOTE: DN_OSPathInfo/File ////////////////////////////////////////////////////////////////////////
DN_API bool DN_OS_FileIsOlderThan(DN_Str8 file, DN_Str8 check_against)
{
  DN_OSPathInfo file_info          = DN_OS_PathInfo(file);
  DN_OSPathInfo check_against_info = DN_OS_PathInfo(check_against);
  bool          result             = !file_info.exists || file_info.last_write_time_in_s < check_against_info.last_write_time_in_s;
  return result;
}

DN_API bool DN_OS_FileWrite(DN_OSFile *file, DN_Str8 buffer, DN_OSErrSink *error)
{
  bool result = DN_OS_FileWritePtr(file, buffer.data, buffer.size, error);
  return result;
}

struct DN_OSFileWriteChunker_
{
  DN_OSErrSink *err;
  DN_OSFile  *file;
  bool        success;
};

static char *DN_OS_FileWriteChunker_(const char *buf, void *user, int len)
{
  DN_OSFileWriteChunker_ *chunker = DN_CAST(DN_OSFileWriteChunker_ *)user;
  chunker->success                = DN_OS_FileWritePtr(chunker->file, buf, len, chunker->err);
  char *result                    = chunker->success ? DN_CAST(char *) buf : nullptr;
  return result;
}

DN_API bool DN_OS_FileWriteFV(DN_OSFile *file, DN_OSErrSink *error, DN_FMT_ATTRIB char const *fmt, va_list args)
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

DN_API bool DN_OS_FileWriteF(DN_OSFile *file, DN_OSErrSink *error, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  bool result = DN_OS_FileWriteFV(file, error, fmt, args);
  va_end(args);
  return result;
}

// NOTE: R/W Entire File ///////////////////////////////////////////////////////////////////////////
DN_API DN_Str8 DN_OS_ReadAll(DN_Arena *arena, DN_Str8 path, DN_OSErrSink *error)
{
  DN_Str8 result = {};
  if (!arena)
    return result;

  // NOTE: Query file size + allocate buffer /////////////////////////////////////////////////////
  DN_OSPathInfo path_info = DN_OS_PathInfo(path);
  if (!path_info.exists) {
    DN_OS_ErrSinkAppendF(error, 1, "File does not exist/could not be queried for reading '%.*s'", DN_STR_FMT(path));
    return result;
  }

  DN_ArenaTempMem temp_mem = DN_Arena_TempMemBegin(arena);
  result                   = DN_Str8_Alloc(arena, path_info.size, DN_ZeroMem_No);
  if (!DN_Str8_HasData(result)) {
    DN_OSTLSTMem tmem             = DN_OS_TLSTMem(nullptr);
    DN_Str8      buffer_size_str8 = DN_CVT_BytesStr8FromU64AutoTLS(path_info.size);
    DN_OS_ErrSinkAppendF(error, 1 /*error_code*/, "Failed to allocate %.*s for reading file '%.*s'", DN_STR_FMT(buffer_size_str8), DN_STR_FMT(path));
    DN_Arena_TempMemEnd(temp_mem);
    result = {};
    return result;
  }

  // NOTE: Read the file from disk ///////////////////////////////////////////////////////////////
  DN_OSFile     file = DN_OS_FileOpen(path, DN_OSFileOpen_OpenIfExist, DN_OSFileAccess_Read, error);
  DN_OSFileRead read = DN_OS_FileRead(&file, result.data, result.size, error);
  if (file.error || !read.success) {
    DN_Arena_TempMemEnd(temp_mem);
    result = {};
  }
  DN_OS_FileClose(&file);

  return result;
}

DN_API bool DN_OS_WriteAll(DN_Str8 path, DN_Str8 buffer, DN_OSErrSink *error)
{
  DN_OSFile file   = DN_OS_FileOpen(path, DN_OSFileOpen_CreateAlways, DN_OSFileAccess_Write, error);
  bool      result = DN_OS_FileWrite(&file, buffer, error);
  DN_OS_FileClose(&file);
  return result;
}

DN_API bool DN_OS_WriteAllFV(DN_Str8 file_path, DN_OSErrSink *error, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_OSTLSTMem tmem   = DN_OS_TLSTMem(nullptr);
  DN_Str8    buffer = DN_Str8_FromFV(tmem.arena, fmt, args);
  bool       result = DN_OS_WriteAll(file_path, buffer, error);
  return result;
}

DN_API bool DN_OS_WriteAllF(DN_Str8 file_path, DN_OSErrSink *error, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  bool result = DN_OS_WriteAllFV(file_path, error, fmt, args);
  va_end(args);
  return result;
}

DN_API bool DN_OS_WriteAllSafe(DN_Str8 path, DN_Str8 buffer, DN_OSErrSink *error)
{
  DN_OSTLSTMem tmem     = DN_OS_TLSTMem(nullptr);
  DN_Str8      tmp_path = DN_Str8_FromF(tmem.arena, "%.*s.tmp", DN_STR_FMT(path));
  if (!DN_OS_WriteAll(tmp_path, buffer, error))
    return false;
  if (!DN_OS_CopyFile(tmp_path, path, true /*overwrite*/, error))
    return false;
  if (!DN_OS_PathDelete(tmp_path))
    return false;
  return true;
}

DN_API bool DN_OS_WriteAllSafeFV(DN_Str8 path, DN_OSErrSink *error, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_OSTLSTMem tmem   = DN_OS_TLSTMem(nullptr);
  DN_Str8    buffer = DN_Str8_FromFV(tmem.arena, fmt, args);
  bool       result = DN_OS_WriteAllSafe(path, buffer, error);
  return result;
}

DN_API bool DN_OS_WriteAllSafeF(DN_Str8 path, DN_OSErrSink *error, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  bool result = DN_OS_WriteAllSafeFV(path, error, fmt, args);
  return result;
}
#endif // !defined(DN_NO_OS_FILE_API)

// NOTE: DN_OSPath /////////////////////////////////////////////////////////////////////////////////
DN_API bool DN_OS_PathAddRef(DN_Arena *arena, DN_OSPath *fs_path, DN_Str8 path)
{
  if (!arena || !fs_path || !DN_Str8_HasData(path))
    return false;

  if (path.size <= 0)
    return true;

  DN_Str8 const delimiter_array[] = {
      DN_STR8("\\"),
      DN_STR8("/")};

  if (fs_path->links_size == 0)
    fs_path->has_prefix_path_separator = (path.data[0] == '/');

  for (;;) {
    DN_Str8BSplitResult delimiter = DN_Str8_BSplitArray(path, delimiter_array, DN_ArrayCountU(delimiter_array));
    for (; delimiter.lhs.data; delimiter = DN_Str8_BSplitArray(delimiter.rhs, delimiter_array, DN_ArrayCountU(delimiter_array))) {
      if (delimiter.lhs.size <= 0)
        continue;

      DN_OSPathLink *link = DN_Arena_New(arena, DN_OSPathLink, DN_ZeroMem_Yes);
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
  DN_Str8 copy   = DN_Str8_FromStr8(arena, path);
  bool    result = DN_Str8_HasData(copy) ? true : DN_OS_PathAddRef(arena, fs_path, copy);
  return result;
}

DN_API bool DN_OS_PathAddF(DN_Arena *arena, DN_OSPath *fs_path, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8 path = DN_Str8_FromFV(arena, fmt, args);
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
  DN_OSTLSTMem tmem = DN_OS_TLSTMem(arena);
  va_list    args;
  va_start(args, fmt);
  DN_Str8 path = DN_Str8_FromFV(tmem.arena, fmt, args);
  va_end(args);
  DN_Str8 result = DN_OS_PathTo(arena, path, path_separator);
  return result;
}

DN_API DN_Str8 DN_OS_Path(DN_Arena *arena, DN_Str8 path)
{
  DN_Str8 result = DN_OS_PathTo(arena, path, DN_OSPathSeperatorString);
  return result;
}

DN_API DN_Str8 DN_OS_PathF(DN_Arena *arena, DN_FMT_ATTRIB char const *fmt, ...)
{
  DN_OSTLSTMem tmem = DN_OS_TLSTMem(arena);
  va_list    args;
  va_start(args, fmt);
  DN_Str8 path = DN_Str8_FromFV(tmem.arena, fmt, args);
  va_end(args);
  DN_Str8 result = DN_OS_Path(arena, path);
  return result;
}

DN_API DN_Str8 DN_OS_PathBuildWithSeparator(DN_Arena *arena, DN_OSPath const *fs_path, DN_Str8 path_separator)
{
  DN_Str8 result = {};
  if (!fs_path || fs_path->links_size <= 0)
    return result;

  // NOTE: Each link except the last one needs the path separator appended to it, '/' or '\\'
  DN_USize string_size = (fs_path->has_prefix_path_separator ? path_separator.size : 0) + fs_path->string_size + ((fs_path->links_size - 1) * path_separator.size);
  result               = DN_Str8_Alloc(arena, string_size, DN_ZeroMem_No);
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

// NOTE: DN_OSExec /////////////////////////////////////////////////////////////////////////////////
DN_API DN_OSExecResult DN_OS_Exec(DN_Slice<DN_Str8> cmd_line,
                                  DN_OSExecArgs    *args,
                                  DN_Arena         *arena,
                                  DN_OSErrSink       *error)
{
  DN_OSExecAsyncHandle async_handle = DN_OS_ExecAsync(cmd_line, args, error);
  DN_OSExecResult      result       = DN_OS_ExecWait(async_handle, arena, error);
  return result;
}

DN_API DN_OSExecResult DN_OS_ExecOrAbort(DN_Slice<DN_Str8> cmd_line, DN_OSExecArgs *args, DN_Arena *arena)
{
  DN_OSErrSink  *error  = DN_OS_ErrSinkBegin(DN_OSErrSinkMode_Nil);
  DN_OSExecResult result = DN_OS_Exec(cmd_line, args, arena, error);
  if (result.os_error_code)
    DN_OS_ErrSinkEndAndExitIfErrorF(error, result.os_error_code, "OS failed to execute the requested command returning the error code %u", result.os_error_code);

  if (result.exit_code)
    DN_OS_ErrSinkEndAndExitIfErrorF(error, result.exit_code, "OS executed command and returned non-zero exit code %u", result.exit_code);
  DN_OS_ErrSinkEndAndIgnore(error);
  return result;
}

// NOTE: DN_OSThread ///////////////////////////////////////////////////////////////////////////////
static void DN_OS_ThreadExecute_(void *user_context)
{
  DN_OSThread *thread = DN_CAST(DN_OSThread *) user_context;
  DN_OS_TLSInit(&thread->tls, thread->tls_init_args);
  DN_OS_TLSSetCurrentThreadTLS(&thread->tls);
  DN_OS_SemaphoreWait(&thread->init_semaphore, DN_OS_SEMAPHORE_INFINITE_TIMEOUT);
  thread->func(thread);
}

DN_API void DN_OS_ThreadSetName(DN_Str8 name)
{
  DN_OSTLS *tls  = DN_OS_TLSGet();
  tls->name_size = DN_CAST(uint8_t) DN_Min(name.size, sizeof(tls->name) - 1);
  DN_Memcpy(tls->name, name.data, tls->name_size);
  tls->name[tls->name_size] = 0;

#if defined(DN_PLATFORM_WIN32)
  DN_W32_ThreadSetName(name);
#else
  DN_Posix_ThreadSetName(name);
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
  DN_OSTLSTMem        tmem   = DN_OS_TLSTMem(arena);
  result.tmem_arena        = tmem.arena;

  DN_OS_HttpRequestAsync(&result, arena, host, path, secure, method, body, headers);
  DN_OS_HttpRequestWait(&result);
  return result;
}
