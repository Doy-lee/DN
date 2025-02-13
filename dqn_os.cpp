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
DN_API DN_OSDateTimeStr8 DN_OS_DateLocalTimeStr8(DN_OSDateTime time, char date_separator, char hms_separator)
{
    DN_OSDateTimeStr8 result = {};
    result.hms_size           = DN_CAST(uint8_t) DN_SNPRINTF(result.hms,
                                                                DN_ARRAY_ICOUNT(result.hms),
                                                                "%02hhu%c%02hhu%c%02hhu",
                                                                time.hour,
                                                                hms_separator,
                                                                time.minutes,
                                                                hms_separator,
                                                                time.seconds);

    result.date_size = DN_CAST(uint8_t) DN_SNPRINTF(result.date,
                                                      DN_ARRAY_ICOUNT(result.date),
                                                      "%hu%c%02hhu%c%02hhu",
                                                      time.year,
                                                      date_separator,
                                                      time.month,
                                                      date_separator,
                                                      time.day);

    DN_ASSERT(result.hms_size < DN_ARRAY_UCOUNT(result.hms));
    DN_ASSERT(result.date_size < DN_ARRAY_UCOUNT(result.date));
    return result;
}

DN_API DN_OSDateTimeStr8 DN_OS_DateLocalTimeStr8Now(char date_separator, char hms_separator)
{
    DN_OSDateTime time       = DN_OS_DateLocalTimeNow();
    DN_OSDateTimeStr8 result = DN_OS_DateLocalTimeStr8(time, date_separator, hms_separator);
    return result;
}

DN_API uint64_t DN_OS_DateUnixTimeS()
{
    uint64_t result = DN_OS_DateUnixTimeNs() / (1'000 /*us*/ * 1'000 /*ms*/ * 1'000 /*s*/);
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
    DN_TLSTMem               tmem         = DN_TLS_TMem(arena);
    DN_Str8                  exe_path     = DN_OS_EXEPath(tmem.arena);
    DN_Str8                  separators[] = {DN_STR8("/"), DN_STR8("\\")};
    DN_Str8BinarySplitResult split        = DN_Str8_BinarySplitLastArray(exe_path, separators, DN_ARRAY_UCOUNT(separators));
    result                                 = DN_Str8_Copy(arena, split.lhs);
    return result;
}

// NOTE: Counters //////////////////////////////////////////////////////////////////////////////////
DN_API DN_F64 DN_OS_PerfCounterS(uint64_t begin, uint64_t end)
{
    uint64_t frequency = DN_OS_PerfCounterFrequency();
    uint64_t ticks     = end - begin;
    DN_F64  result    = ticks / DN_CAST(DN_F64)frequency;
    return result;
}

DN_API DN_F64 DN_OS_PerfCounterMs(uint64_t begin, uint64_t end)
{
    uint64_t frequency = DN_OS_PerfCounterFrequency();
    uint64_t ticks     = end - begin;
    DN_F64  result    = (ticks * 1'000) / DN_CAST(DN_F64)frequency;
    return result;
}

DN_API DN_F64 DN_OS_PerfCounterUs(uint64_t begin, uint64_t end)
{
    uint64_t frequency = DN_OS_PerfCounterFrequency();
    uint64_t ticks     = end - begin;
    DN_F64  result    = (ticks * 1'000'000) / DN_CAST(DN_F64)frequency;
    return result;
}

DN_API DN_F64 DN_OS_PerfCounterNs(uint64_t begin, uint64_t end)
{
    uint64_t frequency = DN_OS_PerfCounterFrequency();
    uint64_t ticks     = end - begin;
    DN_F64  result    = (ticks * 1'000'000'000) / DN_CAST(DN_F64)frequency;
    return result;
}


DN_API DN_OSTimer DN_OS_TimerBegin()
{
    DN_OSTimer result = {};
    result.start       = DN_OS_PerfCounterNow();
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
    uint64_t tsc_begin         = DN_CPU_TSC();
    uint64_t result            = 0;
    if (tsc_begin) {
        uint64_t os_elapsed = 0;
        for (uint64_t os_begin = DN_OS_PerfCounterNow(); os_elapsed < os_target_elapsed; )
            os_elapsed = DN_OS_PerfCounterNow() - os_begin;
        uint64_t tsc_end     = DN_CPU_TSC();
        uint64_t tsc_elapsed = tsc_end - tsc_begin;
        result               = tsc_elapsed / os_elapsed * os_frequency;
    }
    return result;
}

#if !defined(DN_NO_OS_FILE_API)
// NOTE: [$FILE] DN_OSPathInfo/File ///////////////////////////////////////////////////////////////
DN_API bool DN_OS_FileIsOlderThan(DN_Str8 file, DN_Str8 check_against)
{
    DN_OSPathInfo file_info          = DN_OS_PathInfo(file);
    DN_OSPathInfo check_against_info = DN_OS_PathInfo(check_against);
    bool           result             = !file_info.exists || file_info.last_write_time_in_s < check_against_info.last_write_time_in_s;
    return result;
}

DN_API bool DN_OS_FileWrite(DN_OSFile *file, DN_Str8 buffer, DN_ErrSink *error)
{
    bool result = DN_OS_FileWritePtr(file, buffer.data, buffer.size, error);
    return result;
}

DN_API bool DN_OS_FileWriteFV(DN_OSFile *file, DN_ErrSink *error, DN_FMT_ATTRIB char const *fmt, va_list args)
{
    bool result = false;
    if (!file || !fmt)
        return result;
    DN_TLSTMem tmem = DN_TLS_TMem(nullptr);
    DN_Str8    buffer  = DN_Str8_InitFV(tmem.arena, fmt, args);
    result              = DN_OS_FileWritePtr(file, buffer.data, buffer.size, error);
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

// NOTE: R/W Entire File ///////////////////////////////////////////////////////////////////////////
DN_API DN_Str8 DN_OS_ReadAll(DN_Arena *arena, DN_Str8 path, DN_ErrSink *error)
{
    DN_Str8 result = {};
    if (!arena)
        return result;

    // NOTE: Query file size + allocate buffer /////////////////////////////////////////////////////
    DN_OSPathInfo path_info = DN_OS_PathInfo(path);
    if (!path_info.exists) {
        DN_ErrSink_AppendF(error, 1, "File does not exist/could not be queried for reading '%.*s'", DN_STR_FMT(path));
        return result;
    }

    DN_ArenaTempMem temp_mem = DN_Arena_TempMemBegin(arena);
    result                    = DN_Str8_Alloc(arena, path_info.size, DN_ZeroMem_No);
    if (!DN_Str8_HasData(result)) {
        DN_TLSTMem tmem          = DN_TLS_TMem(nullptr);
        DN_Str8    buffer_size_str8 = DN_U64ToByteSizeStr8(tmem.arena, path_info.size, DN_U64ByteSizeType_Auto);
        DN_ErrSink_AppendF(error, 1 /*error_code*/, "Failed to allocate %.*s for reading file '%.*s'", DN_STR_FMT(buffer_size_str8), DN_STR_FMT(path));
        DN_Arena_TempMemEnd(temp_mem);
        result = {};
        return result;
    }

    // NOTE: Read the file from disk ///////////////////////////////////////////////////////////////
    DN_OSFile file        = DN_OS_FileOpen(path, DN_OSFileOpen_OpenIfExist, DN_OSFileAccess_Read, error);
    bool       read_failed = !DN_OS_FileRead(&file, result.data, result.size, error);
    if (file.error || read_failed) {
        DN_Arena_TempMemEnd(temp_mem);
        result = {};
    }
    DN_OS_FileClose(&file);

    return result;
}
DN_API bool DN_OS_WriteAll(DN_Str8 path, DN_Str8 buffer, DN_ErrSink *error)
{
    DN_OSFile file = DN_OS_FileOpen(path, DN_OSFileOpen_CreateAlways, DN_OSFileAccess_Write, error);
    bool result     = DN_OS_FileWrite(&file, buffer, error);
    DN_OS_FileClose(&file);
    return result;
}

DN_API bool DN_OS_WriteAllFV(DN_Str8 file_path, DN_ErrSink *error, DN_FMT_ATTRIB char const *fmt, va_list args)
{
    DN_TLSTMem tmem = DN_TLS_TMem(nullptr);
    DN_Str8    buffer  = DN_Str8_InitFV(tmem.arena, fmt, args);
    bool        result  = DN_OS_WriteAll(file_path, buffer, error);
    return result;
}

DN_API bool DN_OS_WriteAllF(DN_Str8 file_path, DN_ErrSink *error, DN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    bool result = DN_OS_WriteAllFV(file_path, error, fmt, args);
    va_end(args);
    return result;
}

DN_API bool DN_OS_WriteAllSafe(DN_Str8 path, DN_Str8 buffer, DN_ErrSink *error)
{
    DN_TLSTMem tmem  = DN_TLS_TMem(nullptr);
    DN_Str8    tmp_path = DN_Str8_InitF(tmem.arena, "%.*s.tmp", DN_STR_FMT(path));
    if (!DN_OS_WriteAll(tmp_path, buffer, error))
        return false;
    if (!DN_OS_CopyFile(tmp_path, path, true /*overwrite*/, error))
        return false;
    if (!DN_OS_PathDelete(tmp_path))
        return false;
    return true;
}

DN_API bool DN_OS_WriteAllSafeFV(DN_Str8 path, DN_ErrSink *error, DN_FMT_ATTRIB char const *fmt, va_list args)
{
    DN_TLSTMem tmem = DN_TLS_TMem(nullptr);
    DN_Str8    buffer  = DN_Str8_InitFV(tmem.arena, fmt, args);
    bool        result  = DN_OS_WriteAllSafe(path, buffer, error);
    return result;
}

DN_API bool DN_OS_WriteAllSafeF(DN_Str8 path, DN_ErrSink *error, DN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    bool result = DN_OS_WriteAllSafeFV(path, error, fmt, args);
    return result;
}
#endif // !defined(DN_NO_OS_FILE_API)

// NOTE: [$PATH] DN_OSPath ////////////////////////////////////////////////////////////////////////
DN_API bool DN_OS_PathAddRef(DN_Arena *arena, DN_OSPath *fs_path, DN_Str8 path)
{
    if (!arena || !fs_path || !DN_Str8_HasData(path))
        return false;

    if (path.size <= 0)
        return true;

    DN_Str8 const delimiter_array[] = {
        DN_STR8("\\"),
        DN_STR8("/")
    };

    if (fs_path->links_size == 0) {
        fs_path->has_prefix_path_separator = (path.data[0] == '/');
    }

    for (;;) {
        DN_Str8BinarySplitResult delimiter  = DN_Str8_BinarySplitArray(path, delimiter_array, DN_ARRAY_UCOUNT(delimiter_array));
        for (; delimiter.lhs.data; delimiter = DN_Str8_BinarySplitArray(delimiter.rhs, delimiter_array, DN_ARRAY_UCOUNT(delimiter_array))) {
            if (delimiter.lhs.size <= 0)
                continue;

            DN_OSPathLink *link = DN_Arena_New(arena, DN_OSPathLink, DN_ZeroMem_Yes);
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

DN_API bool DN_OS_PathAdd(DN_Arena *arena, DN_OSPath *fs_path, DN_Str8 path)
{
    DN_Str8 copy = DN_Str8_Copy(arena, path);
    bool result   = DN_Str8_HasData(copy) ? true : DN_OS_PathAddRef(arena, fs_path, copy);
    return result;
}

DN_API bool DN_OS_PathAddF(DN_Arena *arena, DN_OSPath *fs_path, DN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    DN_Str8 path = DN_Str8_InitFV(arena, fmt, args);
    va_end(args);
    bool result = DN_OS_PathAddRef(arena, fs_path, path);
    return result;
}

DN_API bool DN_OS_PathPop(DN_OSPath *fs_path)
{
    if (!fs_path)
        return false;

    if (fs_path->tail) {
        DN_ASSERT(fs_path->head);
        fs_path->links_size  -= 1;
        fs_path->string_size -= fs_path->tail->string.size;
        fs_path->tail         = fs_path->tail->prev;
        if (fs_path->tail) {
            fs_path->tail->next = nullptr;
        } else {
            fs_path->head = nullptr;
        }
    } else {
        DN_ASSERT(!fs_path->head);
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
    DN_TLSTMem tmem = DN_TLS_TMem(arena);
    va_list args;
    va_start(args, fmt);
    DN_Str8 path = DN_Str8_InitFV(tmem.arena, fmt, args);
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
    DN_TLSTMem tmem = DN_TLS_TMem(arena);
    va_list args;
    va_start(args, fmt);
    DN_Str8 path = DN_Str8_InitFV(tmem.arena, fmt, args);
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
    result                = DN_Str8_Alloc(arena, string_size, DN_ZeroMem_No);
    if (result.data) {
        char *dest = result.data;
        if (fs_path->has_prefix_path_separator) {
            DN_MEMCPY(dest, path_separator.data, path_separator.size);
            dest += path_separator.size;
        }

        for (DN_OSPathLink *link = fs_path->head; link; link = link->next) {
            DN_Str8 string = link->string;
            DN_MEMCPY(dest, string.data, string.size);
            dest += string.size;

            if (link != fs_path->tail) {
                DN_MEMCPY(dest, path_separator.data, path_separator.size);
                dest += path_separator.size;
            }
        }
    }

    result.data[string_size] = 0;
    return result;
}


// NOTE: [$EXEC] DN_OSExec ////////////////////////////////////////////////////////////////////////
DN_API DN_OSExecResult DN_OS_Exec(DN_Slice<DN_Str8> cmd_line,
                                     DN_OSExecArgs     *args,
                                     DN_Arena          *arena,
                                     DN_ErrSink        *error)
{
    DN_OSExecAsyncHandle async_handle = DN_OS_ExecAsync(cmd_line, args, error);
    DN_OSExecResult result            = DN_OS_ExecWait(async_handle, arena, error);
    return result;
}

DN_API DN_OSExecResult DN_OS_ExecOrAbort(DN_Slice<DN_Str8> cmd_line, DN_OSExecArgs *args, DN_Arena *arena)
{
    DN_ErrSink     *error  = DN_ErrSink_Begin(DN_ErrSinkMode_Nil);
    DN_OSExecResult result = DN_OS_Exec(cmd_line, args, arena, error);
    if (result.os_error_code) {
        DN_ErrSink_EndAndExitIfErrorF(
            error,
            result.os_error_code,
            "OS failed to execute the requested command returning the error code %u",
            result.os_error_code);
    }

    if (result.exit_code) {
        DN_ErrSink_EndAndExitIfErrorF(
            error,
            result.exit_code,
            "OS executed command and returned non-zero exit code %u",
            result.exit_code);
    }

    DN_ErrSink_EndAndIgnore(error);
    return result;
}

// NOTE: [$THRD] DN_OSThread //////////////////////////////////////////////////////////////////////
DN_THREAD_LOCAL DN_TLS *g_dn_os_thread_tls;

static void DN_OS_ThreadExecute_(void *user_context)
{
    DN_OSThread *thread = DN_CAST(DN_OSThread *)user_context;
    DN_TLS_Init(&thread->tls);
    DN_OS_ThreadSetTLS(&thread->tls);
    DN_OS_SemaphoreWait(&thread->init_semaphore, DN_OS_SEMAPHORE_INFINITE_TIMEOUT);
    thread->func(thread);
}

DN_API void DN_OS_ThreadSetTLS(DN_TLS *tls)
{
    g_dn_os_thread_tls = tls;
}

DN_API void DN_OS_ThreadSetName(DN_Str8 name)
{
    DN_TLS *tls   = DN_TLS_Get();
    tls->name_size = DN_CAST(uint8_t)DN_MIN(name.size, sizeof(tls->name) - 1);
    DN_MEMCPY(tls->name, name.data, tls->name_size);
    tls->name[tls->name_size] = 0;

    #if defined(DN_OS_WIN32)
    DN_Win_ThreadSetName(name);
    #else
    DN_Posix_ThreadSetName(name);
    #endif
}

// NOTE: [$HTTP] DN_OSHttp ////////////////////////////////////////////////////////////////////////
DN_API void DN_OS_HttpRequestWait(DN_OSHttpResponse *response)
{
    if (response && DN_OS_SemaphoreIsValid(&response->on_complete_semaphore))
        DN_OS_SemaphoreWait(&response->on_complete_semaphore, DN_OS_SEMAPHORE_INFINITE_TIMEOUT);
}

DN_API DN_OSHttpResponse DN_OS_HttpRequest(DN_Arena *arena, DN_Str8 host, DN_Str8 path, DN_OSHttpRequestSecure secure, DN_Str8 method, DN_Str8 body, DN_Str8 headers)
{
    // TODO(doyle): Revise the memory allocation and its lifetime
    DN_OSHttpResponse result = {};
    DN_TLSTMem        tmem   = DN_TLS_TMem(arena);
    result.tmem_arena         = tmem.arena;

    DN_OS_HttpRequestAsync(&result, arena, host, path, secure, method, body, headers);
    DN_OS_HttpRequestWait(&result);
    return result;
}
