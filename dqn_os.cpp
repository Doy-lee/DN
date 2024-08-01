#pragma once
#include "dqn.h"

/*
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//    $$$$$$\   $$$$$$\
//   $$  __$$\ $$  __$$\
//   $$ /  $$ |$$ /  \__|
//   $$ |  $$ |\$$$$$$\
//   $$ |  $$ | \____$$\
//   $$ |  $$ |$$\   $$ |
//    $$$$$$  |\$$$$$$  |
//    \______/  \______/
//
//   dqn_os.cpp
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

// NOTE: [$DATE] Date //////////////////////////////////////////////////////////////////////////////
DQN_API Dqn_OSDateTimeStr8 Dqn_OS_DateLocalTimeStr8(Dqn_OSDateTime time, char date_separator, char hms_separator)
{
    Dqn_OSDateTimeStr8 result = {};
    result.hms_size           = DQN_CAST(uint8_t) DQN_SNPRINTF(result.hms,
                                                                DQN_ARRAY_ICOUNT(result.hms),
                                                                "%02hhu%c%02hhu%c%02hhu",
                                                                time.hour,
                                                                hms_separator,
                                                                time.minutes,
                                                                hms_separator,
                                                                time.seconds);

    result.date_size = DQN_CAST(uint8_t) DQN_SNPRINTF(result.date,
                                                      DQN_ARRAY_ICOUNT(result.date),
                                                      "%hu%c%02hhu%c%02hhu",
                                                      time.year,
                                                      date_separator,
                                                      time.month,
                                                      date_separator,
                                                      time.day);

    DQN_ASSERT(result.hms_size < DQN_ARRAY_UCOUNT(result.hms));
    DQN_ASSERT(result.date_size < DQN_ARRAY_UCOUNT(result.date));
    return result;
}

DQN_API Dqn_OSDateTimeStr8 Dqn_OS_DateLocalTimeStr8Now(char date_separator, char hms_separator)
{
    Dqn_OSDateTime time       = Dqn_OS_DateLocalTimeNow();
    Dqn_OSDateTimeStr8 result = Dqn_OS_DateLocalTimeStr8(time, date_separator, hms_separator);
    return result;
}

DQN_API bool Dqn_OS_DateIsValid(Dqn_OSDateTime date)
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
DQN_API Dqn_Str8 Dqn_OS_EXEDir(Dqn_Arena *arena)
{
    Dqn_Str8 result = {};
    if (!arena)
        return result;
    Dqn_TLSTMem               tmem      = Dqn_TLS_TMem(arena);
    Dqn_Str8                  exe_path     = Dqn_OS_EXEPath(tmem.arena);
    Dqn_Str8                  separators[] = {DQN_STR8("/"), DQN_STR8("\\")};
    Dqn_Str8BinarySplitResult split        = Dqn_Str8_BinarySplitLastArray(exe_path, separators, DQN_ARRAY_UCOUNT(separators));
    result                                 = Dqn_Str8_Copy(arena, split.lhs);
    return result;
}

// NOTE: Counters //////////////////////////////////////////////////////////////////////////////////
DQN_API Dqn_f64 Dqn_OS_PerfCounterS(uint64_t begin, uint64_t end)
{
    uint64_t frequency = Dqn_OS_PerfCounterFrequency();
    uint64_t ticks     = end - begin;
    Dqn_f64  result    = ticks / DQN_CAST(Dqn_f64)frequency;
    return result;
}

DQN_API Dqn_f64 Dqn_OS_PerfCounterMs(uint64_t begin, uint64_t end)
{
    uint64_t frequency = Dqn_OS_PerfCounterFrequency();
    uint64_t ticks     = end - begin;
    Dqn_f64  result    = (ticks * 1'000) / DQN_CAST(Dqn_f64)frequency;
    return result;
}

DQN_API Dqn_f64 Dqn_OS_PerfCounterUs(uint64_t begin, uint64_t end)
{
    uint64_t frequency = Dqn_OS_PerfCounterFrequency();
    uint64_t ticks     = end - begin;
    Dqn_f64  result    = (ticks * 1'000'000) / DQN_CAST(Dqn_f64)frequency;
    return result;
}

DQN_API Dqn_f64 Dqn_OS_PerfCounterNs(uint64_t begin, uint64_t end)
{
    uint64_t frequency = Dqn_OS_PerfCounterFrequency();
    uint64_t ticks     = end - begin;
    Dqn_f64  result    = (ticks * 1'000'000'000) / DQN_CAST(Dqn_f64)frequency;
    return result;
}


DQN_API Dqn_OSTimer Dqn_OS_TimerBegin()
{
    Dqn_OSTimer result = {};
    result.start       = Dqn_OS_PerfCounterNow();
    return result;
}

DQN_API void Dqn_OS_TimerEnd(Dqn_OSTimer *timer)
{
    timer->end = Dqn_OS_PerfCounterNow();
}

DQN_API Dqn_f64 Dqn_OS_TimerS(Dqn_OSTimer timer)
{
    Dqn_f64 result = Dqn_OS_PerfCounterS(timer.start, timer.end);
    return result;
}

DQN_API Dqn_f64 Dqn_OS_TimerMs(Dqn_OSTimer timer)
{
    Dqn_f64 result = Dqn_OS_PerfCounterMs(timer.start, timer.end);
    return result;
}

DQN_API Dqn_f64 Dqn_OS_TimerUs(Dqn_OSTimer timer)
{
    Dqn_f64 result = Dqn_OS_PerfCounterUs(timer.start, timer.end);
    return result;
}

DQN_API Dqn_f64 Dqn_OS_TimerNs(Dqn_OSTimer timer)
{
    Dqn_f64 result = Dqn_OS_PerfCounterNs(timer.start, timer.end);
    return result;
}

DQN_API uint64_t Dqn_OS_EstimateTSCPerSecond(uint64_t duration_ms_to_gauge_tsc_frequency)
{
    uint64_t os_frequency      = Dqn_OS_PerfCounterFrequency();
    uint64_t os_target_elapsed = duration_ms_to_gauge_tsc_frequency * os_frequency / 1000ULL;
    uint64_t tsc_begin         = Dqn_CPU_TSC();
    uint64_t result            = 0;
    if (tsc_begin) {
        uint64_t os_elapsed = 0;
        for (uint64_t os_begin = Dqn_OS_PerfCounterNow(); os_elapsed < os_target_elapsed; )
            os_elapsed = Dqn_OS_PerfCounterNow() - os_begin;
        uint64_t tsc_end     = Dqn_CPU_TSC();
        uint64_t tsc_elapsed = tsc_end - tsc_begin;
        result               = tsc_elapsed / os_elapsed * os_frequency;
    }
    return result;
}

#if !defined(DQN_NO_OS_FILE_API)
// NOTE: [$FILE] Dqn_OSPathInfo/File ///////////////////////////////////////////////////////////////
DQN_API bool Dqn_OS_FileIsOlderThan(Dqn_Str8 file, Dqn_Str8 check_against)
{
    Dqn_OSPathInfo file_info          = Dqn_OS_PathInfo(file);
    Dqn_OSPathInfo check_against_info = Dqn_OS_PathInfo(check_against);
    bool           result             = !file_info.exists || file_info.last_write_time_in_s < check_against_info.last_write_time_in_s;
    return result;
}

DQN_API bool Dqn_OS_FileWrite(Dqn_OSFile *file, Dqn_Str8 buffer, Dqn_ErrorSink *error)
{
    bool result = Dqn_OS_FileWritePtr(file, buffer.data, buffer.size, error);
    return result;
}

DQN_API bool Dqn_OS_FileWriteFV(Dqn_OSFile *file, Dqn_ErrorSink *error, DQN_FMT_ATTRIB char const *fmt, va_list args)
{
    bool result = false;
    if (!file || !fmt)
        return result;
    Dqn_TLSTMem tmem = Dqn_TLS_TMem(nullptr);
    Dqn_Str8    buffer  = Dqn_Str8_InitFV(tmem.arena, fmt, args);
    result              = Dqn_OS_FileWritePtr(file, buffer.data, buffer.size, error);
    return result;
}

DQN_API bool Dqn_OS_FileWriteF(Dqn_OSFile *file, Dqn_ErrorSink *error, DQN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    bool result = Dqn_OS_FileWriteFV(file, error, fmt, args);
    va_end(args);
    return result;
}

// NOTE: R/W Entire File ///////////////////////////////////////////////////////////////////////////
DQN_API Dqn_Str8 Dqn_OS_ReadAll(Dqn_Arena *arena, Dqn_Str8 path, Dqn_ErrorSink *error)
{
    Dqn_Str8 result = {};
    if (!arena)
        return result;

    // NOTE: Query file size + allocate buffer /////////////////////////////////////////////////////
    Dqn_OSPathInfo path_info = Dqn_OS_PathInfo(path);
    if (!path_info.exists) {
        Dqn_ErrorSink_MakeF(error, 1, "File does not exist/could not be queried for reading '%.*s'", DQN_STR_FMT(path));
        return result;
    }

    Dqn_ArenaTempMem temp_mem = Dqn_Arena_TempMemBegin(arena);
    result                    = Dqn_Str8_Alloc(arena, path_info.size, Dqn_ZeroMem_No);
    if (!Dqn_Str8_HasData(result)) {
        Dqn_TLSTMem tmem          = Dqn_TLS_TMem(nullptr);
        Dqn_Str8    buffer_size_str8 = Dqn_U64ToByteSizeStr8(tmem.arena, path_info.size, Dqn_U64ByteSizeType_Auto);
        Dqn_ErrorSink_MakeF(error, 1 /*error_code*/, "Failed to allocate %.*s for reading file '%.*s'", DQN_STR_FMT(buffer_size_str8), DQN_STR_FMT(path));
        Dqn_Arena_TempMemEnd(temp_mem);
        result = {};
        return result;
    }

    // NOTE: Read the file from disk ///////////////////////////////////////////////////////////////
    Dqn_OSFile file        = Dqn_OS_FileOpen(path, Dqn_OSFileOpen_OpenIfExist, Dqn_OSFileAccess_Read, error);
    bool       read_failed = !Dqn_OS_FileRead(&file, result.data, result.size, error);
    if (file.error || read_failed) {
        Dqn_Arena_TempMemEnd(temp_mem);
        result = {};
    }
    Dqn_OS_FileClose(&file);

    return result;
}
DQN_API bool Dqn_OS_WriteAll(Dqn_Str8 path, Dqn_Str8 buffer, Dqn_ErrorSink *error)
{
    Dqn_OSFile file = Dqn_OS_FileOpen(path, Dqn_OSFileOpen_CreateAlways, Dqn_OSFileAccess_Write, error);
    bool result     = Dqn_OS_FileWrite(&file, buffer, error);
    Dqn_OS_FileClose(&file);
    return result;
}

DQN_API bool Dqn_OS_WriteAllFV(Dqn_Str8 file_path, Dqn_ErrorSink *error, DQN_FMT_ATTRIB char const *fmt, va_list args)
{
    Dqn_TLSTMem tmem = Dqn_TLS_TMem(nullptr);
    Dqn_Str8    buffer  = Dqn_Str8_InitFV(tmem.arena, fmt, args);
    bool        result  = Dqn_OS_WriteAll(file_path, buffer, error);
    return result;
}

DQN_API bool Dqn_OS_WriteAllF(Dqn_Str8 file_path, Dqn_ErrorSink *error, DQN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    bool result = Dqn_OS_WriteAllFV(file_path, error, fmt, args);
    va_end(args);
    return result;
}

DQN_API bool Dqn_OS_WriteAllSafe(Dqn_Str8 path, Dqn_Str8 buffer, Dqn_ErrorSink *error)
{
    Dqn_TLSTMem tmem  = Dqn_TLS_TMem(nullptr);
    Dqn_Str8    tmp_path = Dqn_Str8_InitF(tmem.arena, "%.*s.tmp", DQN_STR_FMT(path));
    if (!Dqn_OS_WriteAll(tmp_path, buffer, error))
        return false;
    if (!Dqn_OS_CopyFile(tmp_path, path, true /*overwrite*/, error))
        return false;
    if (!Dqn_OS_PathDelete(tmp_path))
        return false;
    return true;
}

DQN_API bool Dqn_OS_WriteAllSafeFV(Dqn_Str8 path, Dqn_ErrorSink *error, DQN_FMT_ATTRIB char const *fmt, va_list args)
{
    Dqn_TLSTMem tmem = Dqn_TLS_TMem(nullptr);
    Dqn_Str8    buffer  = Dqn_Str8_InitFV(tmem.arena, fmt, args);
    bool        result  = Dqn_OS_WriteAllSafe(path, buffer, error);
    return result;
}

DQN_API bool Dqn_OS_WriteAllSafeF(Dqn_Str8 path, Dqn_ErrorSink *error, DQN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    bool result = Dqn_OS_WriteAllSafeFV(path, error, fmt, args);
    return result;
}
#endif // !defined(DQN_NO_OS_FILE_API)

// NOTE: [$PATH] Dqn_OSPath ////////////////////////////////////////////////////////////////////////
DQN_API bool Dqn_OS_PathAddRef(Dqn_Arena *arena, Dqn_OSPath *fs_path, Dqn_Str8 path)
{
    if (!arena || !fs_path || !Dqn_Str8_HasData(path))
        return false;

    if (path.size <= 0)
        return true;

    Dqn_Str8 const delimiter_array[] = {
        DQN_STR8("\\"),
        DQN_STR8("/")
    };

    if (fs_path->links_size == 0) {
        fs_path->has_prefix_path_separator = (path.data[0] == '/');
    }

    for (;;) {
        Dqn_Str8BinarySplitResult delimiter  = Dqn_Str8_BinarySplitArray(path, delimiter_array, DQN_ARRAY_UCOUNT(delimiter_array));
        for (; delimiter.lhs.data; delimiter = Dqn_Str8_BinarySplitArray(delimiter.rhs, delimiter_array, DQN_ARRAY_UCOUNT(delimiter_array))) {
            if (delimiter.lhs.size <= 0)
                continue;

            Dqn_OSPathLink *link = Dqn_Arena_New(arena, Dqn_OSPathLink, Dqn_ZeroMem_Yes);
            if (!link)
                return false;

            link->string = delimiter.lhs;
            link->prev   = fs_path->tail;
            if (fs_path->tail) {
                fs_path->tail->next = link;
            } else {
                fs_path->head = link;
            }
            fs_path->tail         = link;
            fs_path->links_size  += 1;
            fs_path->string_size += delimiter.lhs.size;
        }

        if (!delimiter.lhs.data)
            break;
    }

    return true;
}

DQN_API bool Dqn_OS_PathAdd(Dqn_Arena *arena, Dqn_OSPath *fs_path, Dqn_Str8 path)
{
    Dqn_Str8 copy = Dqn_Str8_Copy(arena, path);
    bool result   = Dqn_Str8_HasData(copy) ? true : Dqn_OS_PathAddRef(arena, fs_path, copy);
    return result;
}

DQN_API bool Dqn_OS_PathAddF(Dqn_Arena *arena, Dqn_OSPath *fs_path, DQN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Dqn_Str8 path = Dqn_Str8_InitFV(arena, fmt, args);
    va_end(args);
    bool result = Dqn_OS_PathAddRef(arena, fs_path, path);
    return result;
}

DQN_API bool Dqn_OS_PathPop(Dqn_OSPath *fs_path)
{
    if (!fs_path)
        return false;

    if (fs_path->tail) {
        DQN_ASSERT(fs_path->head);
        fs_path->links_size  -= 1;
        fs_path->string_size -= fs_path->tail->string.size;
        fs_path->tail         = fs_path->tail->prev;
        if (fs_path->tail) {
            fs_path->tail->next = nullptr;
        } else {
            fs_path->head = nullptr;
        }
    } else {
        DQN_ASSERT(!fs_path->head);
    }

    return true;
}

DQN_API Dqn_Str8 Dqn_OS_PathTo(Dqn_Arena *arena, Dqn_Str8 path, Dqn_Str8 path_separator)
{
    Dqn_OSPath fs_path = {};
    Dqn_OS_PathAddRef(arena, &fs_path, path);
    Dqn_Str8 result = Dqn_OS_PathBuildWithSeparator(arena, &fs_path, path_separator);
    return result;
}

DQN_API Dqn_Str8 Dqn_OS_PathToF(Dqn_Arena *arena, Dqn_Str8 path_separator, DQN_FMT_ATTRIB char const *fmt, ...)
{
    Dqn_TLSTMem tmem = Dqn_TLS_TMem(arena);
    va_list args;
    va_start(args, fmt);
    Dqn_Str8 path = Dqn_Str8_InitFV(tmem.arena, fmt, args);
    va_end(args);
    Dqn_Str8 result = Dqn_OS_PathTo(arena, path, path_separator);
    return result;
}

DQN_API Dqn_Str8 Dqn_OS_Path(Dqn_Arena *arena, Dqn_Str8 path)
{
    Dqn_Str8 result = Dqn_OS_PathTo(arena, path, Dqn_OSPathSeperatorString);
    return result;
}

DQN_API Dqn_Str8 Dqn_OS_PathF(Dqn_Arena *arena, DQN_FMT_ATTRIB char const *fmt, ...)
{
    Dqn_TLSTMem tmem = Dqn_TLS_TMem(arena);
    va_list args;
    va_start(args, fmt);
    Dqn_Str8 path = Dqn_Str8_InitFV(tmem.arena, fmt, args);
    va_end(args);
    Dqn_Str8 result = Dqn_OS_Path(arena, path);
    return result;
}

DQN_API Dqn_Str8 Dqn_OS_PathBuildWithSeparator(Dqn_Arena *arena, Dqn_OSPath const *fs_path, Dqn_Str8 path_separator)
{
    Dqn_Str8 result = {};
    if (!fs_path || fs_path->links_size <= 0)
        return result;

    // NOTE: Each link except the last one needs the path separator appended to it, '/' or '\\'
    Dqn_usize string_size = (fs_path->has_prefix_path_separator ? path_separator.size : 0) + fs_path->string_size + ((fs_path->links_size - 1) * path_separator.size);
    result                = Dqn_Str8_Alloc(arena, string_size, Dqn_ZeroMem_No);
    if (result.data) {
        char *dest = result.data;
        if (fs_path->has_prefix_path_separator) {
            DQN_MEMCPY(dest, path_separator.data, path_separator.size);
            dest += path_separator.size;
        }

        for (Dqn_OSPathLink *link = fs_path->head; link; link = link->next) {
            Dqn_Str8 string = link->string;
            DQN_MEMCPY(dest, string.data, string.size);
            dest += string.size;

            if (link != fs_path->tail) {
                DQN_MEMCPY(dest, path_separator.data, path_separator.size);
                dest += path_separator.size;
            }
        }
    }

    result.data[string_size] = 0;
    return result;
}


// NOTE: [$EXEC] Dqn_OSExec ////////////////////////////////////////////////////////////////////////
DQN_API Dqn_OSExecResult Dqn_OS_Exec(Dqn_Slice<Dqn_Str8> cmd_line,
                                     Dqn_Str8            working_dir,
                                     uint8_t             exec_flags,
                                     Dqn_Arena          *arena,
                                     Dqn_ErrorSink      *error)
{
    Dqn_OSExecAsyncHandle async_handle = Dqn_OS_ExecAsync(cmd_line, working_dir, exec_flags, error);
    Dqn_OSExecResult result            = Dqn_OS_ExecWait(async_handle, arena, error);
    return result;
}

DQN_API Dqn_OSExecResult Dqn_OS_ExecOrAbort(Dqn_Slice<Dqn_Str8> cmd_line, Dqn_Str8 working_dir, uint8_t exec_flags, Dqn_Arena *arena)
{
    Dqn_ErrorSink   *error  = Dqn_ErrorSink_Begin(Dqn_ErrorSinkMode_Nil);
    Dqn_OSExecResult result = Dqn_OS_Exec(cmd_line, working_dir, exec_flags, arena, error);
    if (result.os_error_code) {
        Dqn_ErrorSink_EndAndExitIfErrorF(
            error,
            result.os_error_code,
            "OS failed to execute the requested command returning the error code %u",
            result.os_error_code);
    }

    if (result.exit_code) {
        Dqn_ErrorSink_EndAndExitIfErrorF(
            error,
            result.exit_code,
            "OS executed command and returned non-zero exit code %u",
            result.exit_code);
    }

    Dqn_ErrorSink_EndAndIgnore(error);
    return result;
}

// NOTE: [$THRD] Dqn_OSThread //////////////////////////////////////////////////////////////////////
DQN_THREAD_LOCAL Dqn_TLS *g_dqn_os_thread_tls;

static void Dqn_OS_ThreadExecute_(void *user_context)
{
    Dqn_OSThread *thread = DQN_CAST(Dqn_OSThread *)user_context;
    Dqn_TLS_Init(&thread->tls);
    Dqn_OS_ThreadSetTLS(&thread->tls);
    Dqn_OS_SemaphoreWait(&thread->init_semaphore, DQN_OS_SEMAPHORE_INFINITE_TIMEOUT);
    thread->func(thread);
}

DQN_API void Dqn_OS_ThreadSetTLS(Dqn_TLS *tls)
{
    g_dqn_os_thread_tls = tls;
}

// NOTE: [$HTTP] Dqn_OSHttp ////////////////////////////////////////////////////////////////////////
DQN_API void Dqn_OS_HttpRequestWait(Dqn_OSHttpResponse *response)
{
    if (response && Dqn_OS_SemaphoreIsValid(&response->on_complete_semaphore))
        Dqn_OS_SemaphoreWait(&response->on_complete_semaphore, DQN_OS_SEMAPHORE_INFINITE_TIMEOUT);
}

DQN_API Dqn_OSHttpResponse Dqn_OS_HttpRequest(Dqn_Arena *arena, Dqn_Str8 host, Dqn_Str8 path, Dqn_OSHttpRequestSecure secure, Dqn_Str8 method, Dqn_Str8 body, Dqn_Str8 headers)
{
    // TODO(doyle): Revise the memory allocation and its lifetime
    Dqn_OSHttpResponse result = {};
    Dqn_TLSTMem        tmem   = Dqn_TLS_TMem(arena);
    result.tmem_arena         = tmem.arena;

    Dqn_OS_HttpRequestAsync(&result, arena, host, path, secure, method, body, headers);
    Dqn_OS_HttpRequestWait(&result);
    return result;
}
