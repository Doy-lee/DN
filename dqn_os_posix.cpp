#pragma once
#include "dqn.h"

#include <dirent.h> // readdir, opendir, closedir

/*
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//    $$$$$$\   $$$$$$\        $$$$$$$\   $$$$$$\   $$$$$$\  $$$$$$\ $$\   $$\
//   $$  __$$\ $$  __$$\       $$  __$$\ $$  __$$\ $$  __$$\ \_$$  _|$$ |  $$ |
//   $$ /  $$ |$$ /  \__|      $$ |  $$ |$$ /  $$ |$$ /  \__|  $$ |  \$$\ $$  |
//   $$ |  $$ |\$$$$$$\        $$$$$$$  |$$ |  $$ |\$$$$$$\    $$ |   \$$$$  /
//   $$ |  $$ | \____$$\       $$  ____/ $$ |  $$ | \____$$\   $$ |   $$  $$<
//   $$ |  $$ |$$\   $$ |      $$ |      $$ |  $$ |$$\   $$ |  $$ |  $$  /\$$\
//    $$$$$$  |\$$$$$$  |      $$ |       $$$$$$  |\$$$$$$  |$$$$$$\ $$ /  $$ |
//    \______/  \______/       \__|       \______/  \______/ \______|\__|  \__|
//
//   dqn_os_posix.cpp
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

// NOTE: [$VMEM] Dqn_OSMem
// //////////////////////////////////////////////////////////////////////////
static uint32_t Dqn_OS_MemConvertPageToOSFlags_(uint32_t protect)
{
    DQN_ASSERT((protect & ~Dqn_OSMemPage_All) == 0);
    DQN_ASSERT(protect != 0);
    uint32_t result = 0;

    if (protect & (Dqn_OSMemPage_NoAccess | Dqn_OSMemPage_Guard)) {
        result = PROT_NONE;
    } else {
        if (protect & Dqn_OSMemPage_Read)
            result = PROT_READ;
        if (protect & Dqn_OSMemPage_Write)
            result = PROT_WRITE;
    }
    return result;
}

DQN_API void *Dqn_OS_MemReserve(Dqn_usize size, Dqn_OSMemCommit commit, uint32_t page_flags)
{
    unsigned long os_page_flags = Dqn_OS_MemConvertPageToOSFlags_(page_flags);

    if (commit == Dqn_OSMemCommit_Yes)
        os_page_flags |= (PROT_READ | PROT_WRITE);

    void *result = mmap(nullptr, size, os_page_flags, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (result == MAP_FAILED)
        result = nullptr;
    return result;
}

DQN_API bool Dqn_OS_MemCommit(void *ptr, Dqn_usize size, uint32_t page_flags)
{
    bool result = false;
    if (!ptr || size == 0)
        return false;

    unsigned long os_page_flags = Dqn_OS_MemConvertPageToOSFlags_(page_flags);
    result                      = mprotect(ptr, size, os_page_flags) == 0;
    return result;
}

DQN_API void Dqn_OS_MemDecommit(void *ptr, Dqn_usize size)
{
    mprotect(ptr, size, PROT_NONE);
    madvise(ptr, size, MADV_FREE);
}

DQN_API void Dqn_OS_MemRelease(void *ptr, Dqn_usize size)
{
    munmap(ptr, size);
}

DQN_API int Dqn_OS_MemProtect(void *ptr, Dqn_usize size, uint32_t page_flags)
{
    if (!ptr || size == 0)
        return 0;

    static Dqn_Str8 const ALIGNMENT_ERROR_MSG = DQN_STR8(
        "Page protection requires pointers to be page aligned because we "
        "can only guard memory at a multiple of the page boundary.");
    DQN_ASSERTF(Dqn_IsPowerOfTwoAligned(DQN_CAST(uintptr_t) ptr, g_dqn_library->os_page_size),
                "%s",
                ALIGNMENT_ERROR_MSG.data);
    DQN_ASSERTF(
        Dqn_IsPowerOfTwoAligned(size, g_dqn_library->os_page_size), "%s", ALIGNMENT_ERROR_MSG.data);

    unsigned long os_page_flags = Dqn_OS_MemConvertPageToOSFlags_(page_flags);
    int           result        = mprotect(ptr, size, os_page_flags);
    DQN_ASSERTF(result == 0, "mprotect failed (%d)", errno);
    return result;
}

// NOTE: [$DATE] Date //////////////////////////////////////////////////////////////////////////////
DQN_API Dqn_OSDateTime Dqn_OS_DateLocalTimeNow()
{
    Dqn_OSDateTime  result = {};
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    // NOTE: localtime_r is used because it is thread safe
    // See: https://linux.die.net/man/3/localtime
    // According to POSIX.1-2004, localtime() is required to behave as though
    // tzset(3) was called, while localtime_r() does not have this requirement.
    // For portable code tzset(3) should be called before localtime_r().
    for (static bool once = true; once; once = false)
        tzset();

    struct tm time = {};
    localtime_r(&ts.tv_sec, &time);

    result.hour    = time.tm_hour;
    result.minutes = time.tm_min;
    result.seconds = time.tm_sec;

    result.day   = DQN_CAST(uint8_t) time.tm_mday;
    result.month = DQN_CAST(uint8_t) time.tm_mon + 1;
    result.year  = 1900 + DQN_CAST(int16_t) time.tm_year;
    return result;
}

DQN_API uint64_t Dqn_OS_DateUnixTime()
{
    uint64_t result = time(nullptr);
    return result;
}

DQN_API uint64_t Dqn_OS_DateToUnixTime(Dqn_OSDateTime date)
{
    DQN_ASSERT(Dqn_OS_DateIsValid(date));
    struct tm timeinfo = {};
    timeinfo.tm_year   = date.year - 1900;
    timeinfo.tm_mon    = date.month - 1;
    timeinfo.tm_mday   = date.day;
    timeinfo.tm_hour   = date.hour;
    timeinfo.tm_min    = date.minutes;
    timeinfo.tm_sec    = date.seconds;
    uint64_t result    = mktime(&timeinfo);
    return result;
}

DQN_API Dqn_OSDateTime Dqn_OS_DateUnixTimeToDate(uint64_t time)
{
    time_t         posix_time = DQN_CAST(time_t) time;
    struct tm      posix_date = *gmtime(&posix_time);
    Dqn_OSDateTime result     = {};
    result.year               = posix_date.tm_year + 1900;
    result.month              = posix_date.tm_mon + 1;
    result.day                = posix_date.tm_mday;
    result.hour               = posix_date.tm_hour;
    result.minutes            = posix_date.tm_min;
    result.seconds            = posix_date.tm_sec;
    return result;
}

DQN_API bool Dqn_OS_SecureRNGBytes(void *buffer, uint32_t size)
{
#if defined(DQN_PLATFORM_EMSCRIPTEN)
    (void)buffer;
    (void)size;
    return false;
#else
    if (!buffer || size < 0)
        return false;

    if (size == 0)
        return true;

    DQN_ASSERTF(size <= 32,
                "We can increase this by chunking the buffer and filling 32 bytes at a time. *Nix "
                "guarantees 32 "
                "bytes can always be fulfilled by this system at a time");
    // TODO(doyle):
    // https://github.com/jedisct1/libsodium/blob/master/src/libsodium/randombytes/sysrandom/randombytes_sysrandom.c
    // TODO(doyle): https://man7.org/linux/man-pages/man2/getrandom.2.html
    uint32_t read_bytes = 0;
    do {
        read_bytes =
            getrandom(buffer, size, 0); // NOTE: EINTR can not be triggered if size <= 32 bytes
    } while (read_bytes != size || errno == EAGAIN);
    return true;
#endif
}

DQN_API Dqn_Str8 Dqn_OS_EXEPath(Dqn_Arena *arena)
{
    Dqn_Str8 result = {};
    if (!arena)
        return result;

    int required_size_wo_null_terminator = 0;
    for (int try_size = 128;; try_size *= 2) {
        auto  scoped_arena  = Dqn_ArenaTempMemScope(arena);
        char *try_buf       = Dqn_Arena_NewArray(arena, char, try_size, Dqn_ZeroMem_No);
        int   bytes_written = readlink("/proc/self/exe", try_buf, try_size);
        if (bytes_written == -1) {
            // Failed, we're unable to determine the executable directory
            break;
        } else if (bytes_written == try_size) {
            // Try again, if returned size was equal- we may of prematurely
            // truncated according to the man pages
            continue;
        } else {
            // readlink will give us the path to the executable. Once we
            // determine the correct buffer size required to get the full file
            // path, we do some post-processing on said string and extract just
            // the directory.

            // TODO(dqn): It'd be nice if there's some way of keeping this
            // try_buf around, memcopy the byte and trash the try_buf from the
            // arena. Instead we just get the size and redo the call one last
            // time after this "calculate" step.
            DQN_ASSERTF(bytes_written < try_size,
                        "bytes_written can never be greater than the try size, function writes at "
                        "most try_size");
            required_size_wo_null_terminator = bytes_written;
            break;
        }
    }

    if (required_size_wo_null_terminator) {
        Dqn_ArenaTempMem temp_mem = Dqn_Arena_TempMemBegin(arena);
        char            *exe_path =
            Dqn_Arena_NewArray(arena, char, required_size_wo_null_terminator + 1, Dqn_ZeroMem_No);
        exe_path[required_size_wo_null_terminator] = 0;

        int bytes_written = readlink("/proc/self/exe", exe_path, required_size_wo_null_terminator);
        if (bytes_written == -1) {
            // Note that if read-link fails again can be because there's
            // a potential race condition here, our exe or directory could have
            // been deleted since the last call, so we need to be careful.
            Dqn_Arena_TempMemEnd(temp_mem);
        } else {
            result = Dqn_Str8_Init(exe_path, required_size_wo_null_terminator);
        }
    }
    return result;
}

DQN_API uint64_t Dqn_OS_PerfCounterFrequency()
{
    // NOTE: On Linux we use clock_gettime(CLOCK_MONOTONIC_RAW) which
    // increments at nanosecond granularity.
    uint64_t result = 1'000'000'000;
    return result;
}

DQN_API uint64_t Dqn_OS_PerfCounterNow()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    uint64_t result = DQN_CAST(uint64_t) ts.tv_sec * 1'000'000'000 + DQN_CAST(uint64_t) ts.tv_nsec;
    return result;
}

#if !defined(DQN_NO_OS_FILE_API)
DQN_API Dqn_OSPathInfo Dqn_OS_PathInfo(Dqn_Str8 path)
{
    Dqn_OSPathInfo result = {};
    if (!Dqn_Str8_HasData(path))
        return result;

    struct stat file_stat;
    if (lstat(path.data, &file_stat) != -1) {
        result.exists                = true;
        result.size                  = file_stat.st_size;
        result.last_access_time_in_s = file_stat.st_atime;
        result.last_write_time_in_s  = file_stat.st_mtime;
        // TODO(dqn): Seems linux does not support creation time via stat. We
        // shoddily deal with this.
        result.create_time_in_s =
            DQN_MIN(result.last_access_time_in_s, result.last_write_time_in_s);

        if (S_ISDIR(file_stat.st_mode)) {
            result.type = Dqn_OSPathInfoType_Directory;
        } else if (S_ISREG(file_stat.st_mode)) {
            result.type = Dqn_OSPathInfoType_File;
        }
    }
    return result;
}

DQN_API bool Dqn_OS_PathDelete(Dqn_Str8 path)
{
    bool result = false;
    if (Dqn_Str8_HasData(path))
        result = remove(path.data) == 0;
    return result;
}

DQN_API bool Dqn_OS_FileExists(Dqn_Str8 path)
{
    bool result = false;
    if (!Dqn_Str8_HasData(path))
        return result;

    struct stat stat_result;
    if (lstat(path.data, &stat_result) != -1)
        result = S_ISREG(stat_result.st_mode) || S_ISLNK(stat_result.st_mode);
    return result;
}

DQN_API bool Dqn_OS_CopyFile(Dqn_Str8 src, Dqn_Str8 dest, bool overwrite, Dqn_ErrorSink *error)
{
    bool result = false;
    #if defined(DQN_PLATFORM_EMSCRIPTEN)
    Dqn_ErrorSink_MakeF(error, 1, "Unsupported on Emscripten because of their VFS model");
    #else
    int src_fd = open(src.data, O_RDONLY);
    if (src_fd == -1) {
        int error_code = errno;
        Dqn_ErrorSink_MakeF(error,
                            error_code,
                            "Failed to open file '%.*s' for copying: (%d) %s",
                            DQN_STR_FMT(src),
                            error_code,
                            strerror(error_code));
        return result;
    }
    DQN_DEFER
    {
        close(src_fd);
    };

    int dest_fd = open(dest.data, O_WRONLY | O_CREAT | (overwrite ? O_TRUNC : 0));
    if (dest_fd == -1) {
        int error_code = errno;
        Dqn_ErrorSink_MakeF(error,
                            error_code,
                            "Failed to open file destination '%.*s' for copying to: (%d) %s",
                            DQN_STR_FMT(src),
                            error_code,
                            strerror(error_code));
        return result;
    }
    DQN_DEFER
    {
        close(dest_fd);
    };

    struct stat stat_existing;
    int         fstat_result = fstat(src_fd, &stat_existing);
    if (fstat_result == -1) {
        int error_code = errno;
        Dqn_ErrorSink_MakeF(error,
                            error_code,
                            "Failed to query file size of '%.*s' for copying: (%d) %s",
                            DQN_STR_FMT(src),
                            error_code,
                            strerror(error_code));
        return result;
    }

    ssize_t bytes_written = sendfile64(dest_fd, src_fd, 0, stat_existing.st_size);
    result                = (bytes_written == stat_existing.st_size);
    if (!result) {
        int         error_code = errno;
        Dqn_TLSTMem tmem       = Dqn_TLS_TMem(nullptr);
        Dqn_Str8    file_size_str8 =
            Dqn_U64ToByteSizeStr8(tmem.arena, stat_existing.st_size, Dqn_U64ByteSizeType_Auto);
        Dqn_Str8 bytes_written_str8 =
            Dqn_U64ToByteSizeStr8(tmem.arena, bytes_written, Dqn_U64ByteSizeType_Auto);
        Dqn_ErrorSink_MakeF(error,
                            error_code,
                            "Failed to copy file '%.*s' to '%.*s', we copied %.*s but the file "
                            "size is %.*s: (%d) %s",
                            DQN_STR_FMT(src),
                            DQN_STR_FMT(dest),
                            DQN_STR_FMT(bytes_written_str8),
                            DQN_STR_FMT(file_size_str8),
                            error_code,
                            strerror(error_code));
    }

    #endif
    return result;
}

DQN_API bool Dqn_OS_MoveFile(Dqn_Str8 src, Dqn_Str8 dest, bool overwrite, Dqn_ErrorSink *error)
{
    // See: https://github.com/gingerBill/gb/blob/master/gb.h
    bool result     = false;
    bool file_moved = true;
    if (link(src.data, dest.data) == -1) {
        // NOTE: Link can fail if we're trying to link across different volumes
        // so we fall back to a binary directory.
        file_moved |= Dqn_OS_CopyFile(src, dest, overwrite, error);
    }

    if (file_moved) {
        result = true;
        int unlink_result = unlink(src.data);
        if (unlink_result == -1) {
            int error_code = errno;
            Dqn_ErrorSink_MakeF(
                error,
                error_code,
                "File '%.*s' was moved but failed to be unlinked from old location: (%d) %s",
                DQN_STR_FMT(src),
                error_code,
                strerror(error_code));
        }
    }
    return result;
}

DQN_API bool Dqn_OS_MakeDir(Dqn_Str8 path)
{
    Dqn_TLSTMem tmem   = Dqn_TLS_TMem(nullptr);
    bool        result = true;

    // TODO(doyle): Implement this without using the path indexes, it's not
    // necessary. See Windows implementation.
    Dqn_usize path_indexes_size = 0;
    uint16_t  path_indexes[64]  = {};

    Dqn_Str8 copy = Dqn_Str8_Copy(tmem.arena, path);
    for (Dqn_usize index = copy.size - 1; index < copy.size; index--) {
        bool first_char = index == (copy.size - 1);
        char ch         = copy.data[index];
        if (ch == '/' || first_char) {
            char temp = copy.data[index];

            if (!first_char)
                copy.data[index] = 0; // Temporarily null terminate it

            bool is_file = Dqn_OS_FileExists(copy);

            if (!first_char)
                copy.data[index] = temp; // Undo null termination

            if (is_file) {
                // NOTE: There's something that exists in at this path, but
                // it's not a directory. This request to make a directory is
                // invalid.
                return false;
            } else if (Dqn_OS_DirExists(copy)) {
                // NOTE: We found a directory, we can stop here and start
                // building up all the directories that didn't exist up to
                // this point.
                break;
            } else {
                // NOTE: There's nothing that exists at this path, we can
                // create a directory here
                path_indexes[path_indexes_size++] = DQN_CAST(uint16_t) index;
            }
        }
    }

    for (Dqn_usize index = path_indexes_size - 1; result && index < path_indexes_size; index--) {
        uint16_t path_index = path_indexes[index];
        char     temp       = copy.data[path_index];

        if (index != 0)
            copy.data[path_index] = 0;
        result |= mkdir(copy.data, 0774) == 0;
        if (index != 0)
            copy.data[path_index] = temp;
    }
    return result;
}

DQN_API bool Dqn_OS_DirExists(Dqn_Str8 path)
{
    bool result = false;
    if (!Dqn_Str8_HasData(path))
        return result;

    struct stat stat_result;
    if (lstat(path.data, &stat_result) != -1)
        result = S_ISDIR(stat_result.st_mode);
    return result;
}

DQN_API bool Dqn_OS_DirIterate(Dqn_Str8 path, Dqn_OS_DirIterator *it)
{
    if (!it->handle) {
        it->handle = opendir(path.data);
        if (!it->handle)
            return false;
    }

    struct dirent *entry;
    for (;;) {
        entry = readdir(DQN_CAST(DIR*)it->handle);
        if (entry == NULL)
            break;

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        Dqn_usize name_size    = Dqn_CStr8_Size(entry->d_name);
        Dqn_usize clamped_size = DQN_MIN(sizeof(it->buffer) - 1, name_size);
        DQN_ASSERTF(name_size == clamped_size, "name: %s, name_size: %zu, clamped_size: %zu", entry->d_name, name_size, clamped_size);
        DQN_MEMCPY(it->buffer, entry->d_name, clamped_size);
        it->buffer[clamped_size] = 0;
        it->file_name = Dqn_Str8_Init(it->buffer, clamped_size);
        return true;
    }

    closedir(DQN_CAST(DIR*)it->handle);
    it->handle    = NULL;
    it->file_name = {};
    it->buffer[0] = 0;
    return false;
}

// NOTE: R/W Stream API ////////////////////////////////////////////////////////////////////////////
DQN_API Dqn_OSFile Dqn_OS_FileOpen(Dqn_Str8       path,
                                   Dqn_OSFileOpen open_mode,
                                   uint32_t       access,
                                   Dqn_ErrorSink *error)
{
    Dqn_OSFile result = {};
    if (!Dqn_Str8_HasData(path) || path.size <= 0)
        return result;

    if ((access & ~Dqn_OSFileAccess_All) || ((access & Dqn_OSFileAccess_All) == 0)) {
        DQN_INVALID_CODE_PATH;
        return result;
    }

    if (access & Dqn_OSFileAccess_Execute) {
        result.error = true;
        Dqn_ErrorSink_MakeF(
            error,
            1,
            "Failed to open file '%.*s': File access flag 'execute' is not supported",
            DQN_STR_FMT(path));
        DQN_INVALID_CODE_PATH; // TODO: Not supported via fopen
        return result;
    }

    // NOTE: fopen interface is not as expressive as the Win32
    // We will fopen the file beforehand to setup the state/check for validity
    // before closing and reopening it with the correct request access
    // permissions.
    {
        FILE *handle = nullptr;
        switch (open_mode) {
            case Dqn_OSFileOpen_CreateAlways: handle = fopen(path.data, "w"); break;
            case Dqn_OSFileOpen_OpenIfExist: handle = fopen(path.data, "r"); break;
            case Dqn_OSFileOpen_OpenAlways: handle = fopen(path.data, "a"); break;
            default: DQN_INVALID_CODE_PATH; break;
        }

        if (!handle) { // TODO(doyle): FileOpen flag to string
            result.error = true;
            Dqn_ErrorSink_MakeF(error,
                                1,
                                "Failed to open file '%.*s': File could not be opened in requested "
                                "mode 'Dqn_OSFileOpen' flag %d",
                                DQN_STR_FMT(path),
                                open_mode);
            return result;
        }
        fclose(handle);
    }

    char const *fopen_mode = nullptr;
    if (access & Dqn_OSFileAccess_AppendOnly)
        fopen_mode = "a+";
    else if (access & Dqn_OSFileAccess_Write)
        fopen_mode = "w+";
    else if (access & Dqn_OSFileAccess_Read)
        fopen_mode = "r+";

    FILE *handle = fopen(path.data, fopen_mode);
    if (!handle) {
        result.error = true;
        Dqn_ErrorSink_MakeF(error,
                            1,
                            "Failed to open file '%.*s': File could not be opened with requested "
                            "access mode 'Dqn_OSFileAccess' %d",
                            DQN_STR_FMT(path),
                            fopen_mode);
        return result;
    }
    result.handle = handle;
    return result;
}

DQN_API bool Dqn_OS_FileRead(Dqn_OSFile *file, void *buffer, Dqn_usize size, Dqn_ErrorSink *error)
{
    if (!file || !file->handle || file->error || !buffer || size <= 0)
        return false;

    if (fread(buffer, size, 1, DQN_CAST(FILE *) file->handle) != 1) {
        Dqn_TLSTMem tmem = Dqn_TLS_TMem(nullptr);
        Dqn_Str8    buffer_size_str8 =
            Dqn_U64ToByteSizeStr8(tmem.arena, size, Dqn_U64ByteSizeType_Auto);
        Dqn_ErrorSink_MakeF(
            error, 1, "Failed to read %.*s from file", DQN_STR_FMT(buffer_size_str8));
        return false;
    }

    return true;
}

DQN_API bool
Dqn_OS_FileWritePtr(Dqn_OSFile *file, void const *buffer, Dqn_usize size, Dqn_ErrorSink *error)
{
    if (!file || !file->handle || file->error || !buffer || size <= 0)
        return false;
    bool result =
        fwrite(buffer, DQN_CAST(Dqn_usize) size, 1 /*count*/, DQN_CAST(FILE *) file->handle) ==
        1 /*count*/;
    if (!result) {
        Dqn_TLSTMem tmem = Dqn_TLS_TMem(nullptr);
        Dqn_Str8    buffer_size_str8 =
            Dqn_U64ToByteSizeStr8(tmem.arena, size, Dqn_U64ByteSizeType_Auto);
        Dqn_ErrorSink_MakeF(
            error, 1, "Failed to write buffer (%s) to file handle", DQN_STR_FMT(buffer_size_str8));
    }
    return result;
}

DQN_API void Dqn_OS_FileClose(Dqn_OSFile *file)
{
    if (!file || !file->handle || file->error)
        return;
    fclose(DQN_CAST(FILE *) file->handle);
    *file = {};
}
#endif // !defined(DQN_NO_OS_FILE_API)

// NOTE: [$EXEC] Dqn_OSExec ////////////////////////////////////////////////////////////////////////
DQN_API void Dqn_OS_Exit(int32_t exit_code)
{
    exit(DQN_CAST(int) exit_code);
}

enum Dqn_OSPipeType_ {
    Dqn_OSPipeType__Read,
    Dqn_OSPipeType__Write,
    Dqn_OSPipeType__Count,
};

DQN_API Dqn_OSExecResult Dqn_OS_ExecWait(Dqn_OSExecAsyncHandle handle,
                                         Dqn_Arena            *arena,
                                         Dqn_ErrorSink        *error)
{
    Dqn_OSExecResult result = {};
    if (!handle.process || handle.os_error_code || handle.exit_code) {
        if (handle.os_error_code)
            result.os_error_code = handle.os_error_code;
        else
            result.exit_code = handle.exit_code;

        DQN_ASSERT(!handle.stdout_read);
        DQN_ASSERT(!handle.stdout_write);
        DQN_ASSERT(!handle.stderr_read);
        DQN_ASSERT(!handle.stderr_write);
        return result;
    }

#if defined(DQN_PLATFORM_EMSCRIPTEN)
    DQN_INVALID_CODE_PATHF("Unsupported operation");
#endif

    static_assert(sizeof(pid_t) <= sizeof(handle.process),
                  "We store the PID opaquely in a register sized pointer");
    pid_t process = {};
    DQN_MEMCPY(&process, &handle.process, sizeof(process));
    for (;;) {
        int status = 0;
        if (waitpid(process, &status, 0) < 0) {
            result.os_error_code = errno;
            break;
        }

        if (WIFEXITED(status)) {
            result.exit_code = WEXITSTATUS(status);
            break;
        }

        if (WIFSIGNALED(status)) {
            result.os_error_code = WTERMSIG(status);
            break;
        }
    }

    int stdout_pipe[Dqn_OSPipeType__Count] = {};
    int stderr_pipe[Dqn_OSPipeType__Count] = {};
    DQN_MEMCPY(&stdout_pipe[Dqn_OSPipeType__Read],
               &handle.stdout_read,
               sizeof(stdout_pipe[Dqn_OSPipeType__Read]));
    DQN_MEMCPY(&stdout_pipe[Dqn_OSPipeType__Write],
               &handle.stdout_write,
               sizeof(stdout_pipe[Dqn_OSPipeType__Write]));
    DQN_MEMCPY(&stderr_pipe[Dqn_OSPipeType__Read],
               &handle.stderr_read,
               sizeof(stderr_pipe[Dqn_OSPipeType__Read]));
    DQN_MEMCPY(&stderr_pipe[Dqn_OSPipeType__Write],
               &handle.stderr_write,
               sizeof(stderr_pipe[Dqn_OSPipeType__Write]));

    // NOTE: Process has finished, stop the write end of the pipe
    close(stdout_pipe[Dqn_OSPipeType__Write]);
    close(stderr_pipe[Dqn_OSPipeType__Write]);

    // NOTE: Read the data from the read end of the pipe
    if (result.os_error_code == 0) {
        Dqn_TLSTMem tmem = Dqn_TLS_TMem(arena);
        if (arena && handle.stdout_read) {
            char            buffer[4096];
            Dqn_Str8Builder builder = Dqn_Str8Builder_Init(tmem.arena);
            for (;;) {
                ssize_t bytes_read =
                    read(stdout_pipe[Dqn_OSPipeType__Read], buffer, sizeof(buffer));
                if (bytes_read <= 0)
                    break;
                Dqn_Str8Builder_AddF(&builder, "%.*s", bytes_read, buffer);
            }

            result.stdout_text = Dqn_Str8Builder_Build(&builder, arena);
        }

        if (arena && handle.stderr_read) {
            char            buffer[4096];
            Dqn_Str8Builder builder = Dqn_Str8Builder_Init(tmem.arena);
            for (;;) {
                ssize_t bytes_read =
                    read(stderr_pipe[Dqn_OSPipeType__Read], buffer, sizeof(buffer));
                if (bytes_read <= 0)
                    break;
                Dqn_Str8Builder_AddF(&builder, "%.*s", bytes_read, buffer);
            }

            result.stderr_text = Dqn_Str8Builder_Build(&builder, arena);
        }
    }

    close(stdout_pipe[Dqn_OSPipeType__Read]);
    close(stderr_pipe[Dqn_OSPipeType__Read]);
    return result;
}

DQN_API Dqn_OSExecAsyncHandle Dqn_OS_ExecAsync(Dqn_Slice<Dqn_Str8> cmd_line,
                                               Dqn_Str8            working_dir,
                                               uint8_t             exec_flags,
                                               Dqn_ErrorSink      *error)
{
#if defined(DQN_PLATFORM_EMSCRIPTEN)
    DQN_INVALID_CODE_PATHF("Unsupported operation");
#endif

    Dqn_OSExecAsyncHandle result = {};
    if (cmd_line.size == 0)
        return result;

    Dqn_TLSTMem tmem         = Dqn_TLS_TMem(nullptr);
    Dqn_Str8    cmd_rendered = Dqn_Slice_Str8Render(tmem.arena, cmd_line, DQN_STR8(" "));
    int         stdout_pipe[Dqn_OSPipeType__Count] = {};
    int         stderr_pipe[Dqn_OSPipeType__Count] = {};

    // NOTE: Open stdout pipe //////////////////////////////////////////////////////////////////////
    if (Dqn_Bit_IsSet(exec_flags, Dqn_OSExecFlag_SaveStdout)) {
        if (pipe(stdout_pipe) == -1) {
            result.os_error_code = errno;
            Dqn_ErrorSink_MakeF(
                error,
                result.os_error_code,
                "Failed to create stdout pipe to redirect the output of the command '%.*s': %s",
                DQN_STR_FMT(cmd_rendered),
                strerror(result.os_error_code));
            return result;
        }
        DQN_ASSERT(stdout_pipe[Dqn_OSPipeType__Read] != 0);
        DQN_ASSERT(stdout_pipe[Dqn_OSPipeType__Write] != 0);
    }

    DQN_DEFER
    {
        if (result.os_error_code == 0 && result.exit_code == 0)
            return;
        close(stdout_pipe[Dqn_OSPipeType__Read]);
        close(stdout_pipe[Dqn_OSPipeType__Write]);
    };

    // NOTE: Open stderr pipe //////////////////////////////////////////////////////////////////////
    if (Dqn_Bit_IsSet(exec_flags, Dqn_OSExecFlag_SaveStderr)) {
        if (Dqn_Bit_IsSet(exec_flags, Dqn_OSExecFlag_MergeStderrToStdout)) {
            stderr_pipe[Dqn_OSPipeType__Read]  = stdout_pipe[Dqn_OSPipeType__Read];
            stderr_pipe[Dqn_OSPipeType__Write] = stdout_pipe[Dqn_OSPipeType__Write];
        } else if (pipe(stderr_pipe) == -1) {
            result.os_error_code = errno;
            Dqn_ErrorSink_MakeF(
                error,
                result.os_error_code,
                "Failed to create stderr pipe to redirect the output of the command '%.*s': %s",
                DQN_STR_FMT(cmd_rendered),
                strerror(result.os_error_code));
            return result;
        }
        DQN_ASSERT(stderr_pipe[Dqn_OSPipeType__Read] != 0);
        DQN_ASSERT(stderr_pipe[Dqn_OSPipeType__Write] != 0);
    }

    DQN_DEFER {
        if (result.os_error_code == 0 && result.exit_code == 0)
            return;
        close(stderr_pipe[Dqn_OSPipeType__Read]);
        close(stderr_pipe[Dqn_OSPipeType__Write]);
    };

    pid_t child_pid = fork();
    if (child_pid < 0) {
        result.os_error_code = errno;
        Dqn_ErrorSink_MakeF(
            error,
            result.os_error_code,
            "Failed to fork process to execute the command '%.*s': %s",
            DQN_STR_FMT(cmd_rendered),
            strerror(result.os_error_code));
        return result;
    }

    if (child_pid == 0) { // Child process
        if (Dqn_Bit_IsSet(exec_flags, Dqn_OSExecFlag_SaveStdout) &&
            (dup2(stdout_pipe[Dqn_OSPipeType__Write], STDOUT_FILENO) == -1)) {
            result.os_error_code = errno;
            Dqn_ErrorSink_MakeF(
                error,
                result.os_error_code,
                "Failed to redirect stdout 'write' pipe for output of command '%.*s': %s",
                DQN_STR_FMT(cmd_rendered),
                strerror(result.os_error_code));
            return result;
        }

        if (Dqn_Bit_IsSet(exec_flags, Dqn_OSExecFlag_SaveStderr) &&
            (dup2(stderr_pipe[Dqn_OSPipeType__Write], STDERR_FILENO) == -1)) {
            result.os_error_code = errno;
            Dqn_ErrorSink_MakeF(
                error,
                result.os_error_code,
                "Failed to redirect stderr 'read' pipe for output of command '%.*s': %s",
                DQN_STR_FMT(cmd_rendered),
                strerror(result.os_error_code));
            return result;
        }

        // NOTE: Convert the command into something suitable for execvp
        char **argv =
            Dqn_Arena_NewArray(tmem.arena, char *, cmd_line.size + 1 /*null*/, Dqn_ZeroMem_Yes);
        if (!argv) {
            result.exit_code = -1;
            Dqn_ErrorSink_MakeF(
                error,
                result.os_error_code,
                "Failed to create argument values from command line '%.*s': Out of memory",
                DQN_STR_FMT(cmd_rendered));
            return result;
        }

        for (Dqn_usize arg_index = 0; arg_index < cmd_line.size; arg_index++) {
            Dqn_Str8 arg    = cmd_line.data[arg_index];
            argv[arg_index] = Dqn_Str8_Copy(tmem.arena, arg).data; // NOTE: Copy string to guarantee it is null-terminated
        }

        // NOTE: Change the working directory if there is one
        char *prev_working_dir = nullptr;
        DQN_DEFER {
            if (!prev_working_dir)
                return;
            if (result.os_error_code == 0) {
                int chdir_result = chdir(prev_working_dir);
                (void)chdir_result;
            }
            free(prev_working_dir);
        };

        if (working_dir.size) {
            prev_working_dir = get_current_dir_name();
            if (chdir(working_dir.data) == -1) {
                result.os_error_code = errno;
                Dqn_ErrorSink_MakeF(
                    error,
                    result.os_error_code,
                    "Failed to create argument values from command line '%.*s': %s",
                    DQN_STR_FMT(cmd_rendered),
                    strerror(result.os_error_code));
                return result;
            }
        }

        // NOTE: Execute the command. We reuse argv because the first arg, the
        // binary to execute is guaranteed to be null-terminated.
        if (execvp(argv[0], argv) < 0) {
            result.os_error_code = errno;
            Dqn_ErrorSink_MakeF(
                error,
                result.os_error_code,
                "Failed to execute command'%.*s': %s",
                DQN_STR_FMT(cmd_rendered),
                strerror(result.os_error_code));
            return result;
        }
    }

    DQN_ASSERT(result.os_error_code == 0);
    DQN_MEMCPY(&result.stdout_read,
               &stdout_pipe[Dqn_OSPipeType__Read],
               sizeof(stdout_pipe[Dqn_OSPipeType__Read]));
    DQN_MEMCPY(&result.stdout_write,
               &stdout_pipe[Dqn_OSPipeType__Write],
               sizeof(stdout_pipe[Dqn_OSPipeType__Write]));

    if (Dqn_Bit_IsSet(exec_flags, Dqn_OSExecFlag_SaveStderr) && Dqn_Bit_IsNotSet(exec_flags, Dqn_OSExecFlag_MergeStderrToStdout)) {
        DQN_MEMCPY(&result.stderr_read,
                   &stderr_pipe[Dqn_OSPipeType__Read],
                   sizeof(stderr_pipe[Dqn_OSPipeType__Read]));
        DQN_MEMCPY(&result.stderr_write,
                   &stderr_pipe[Dqn_OSPipeType__Write],
                   sizeof(stderr_pipe[Dqn_OSPipeType__Write]));
    }
    result.exec_flags = exec_flags;
    DQN_MEMCPY(&result.process, &child_pid, sizeof(child_pid));
    return result;
}

#if !defined(DQN_NO_SEMAPHORE)
// NOTE: [$SEMA] Dqn_OSSemaphore ///////////////////////////////////////////////////////////////////
DQN_API Dqn_OSSemaphore Dqn_OS_SemaphoreInit(uint32_t initial_count)
{
    Dqn_OSSemaphore result  = {};
    int             pshared = 0; // Share the semaphore across all threads in the process
    if (sem_init(&result.posix_handle, pshared, initial_count) == 0)
        result.posix_init = true;
    return result;
}

DQN_API bool Dqn_OS_SemaphoreIsValid(Dqn_OSSemaphore *semaphore)
{
    bool result = false;
    if (semaphore)
        result = semaphore->posix_init;
    return result;
}

DQN_API void Dqn_OS_SemaphoreDeinit(Dqn_OSSemaphore *semaphore)
{
    if (!Dqn_OS_SemaphoreIsValid(semaphore))
        return;
    // TODO(doyle): Error handling?
    if (semaphore->posix_init)
        sem_destroy(&semaphore->posix_handle);
    *semaphore = {};
}

// NOTE: These functions don't need semaphore to be passed by pointer, **BUT**
// the POSIX implementation disallows copies of sem_t. In particular:
//
// Source: The Open Group Base Specifications Issue 7, 2018 edition
// https://pubs.opengroup.org/onlinepubs/9699919799/functions/V2_chap02.html#tag_15_09_09
//
// 2.9.9 Synchronization Object Copies and Alternative Mappings
//
// For barriers, condition variables, mutexes, and read-write locks, [TSH]
// [Option Start]  if the process-shared attribute is set to
// PTHREAD_PROCESS_PRIVATE, [Option End]  only the synchronization object at the
// address used to initialize it can be used for performing synchronization. The
// effect of referring to another mapping of the same object when locking,
// unlocking, or destroying the object is undefined. [...] The effect of
// referring to a copy of the object when locking, unlocking, or destroying it
// is undefined.

DQN_API void Dqn_OS_SemaphoreIncrement(Dqn_OSSemaphore *semaphore, uint32_t amount)
{
    if (!Dqn_OS_SemaphoreIsValid(semaphore))
        return;

    #if defined(DQN_OS_WIN32)
    sem_post_multiple(&semaphore->posix_handle, amount); // mingw extension
    #else
    DQN_FOR_UINDEX(index, amount)
    sem_post(&semaphore->posix_handle);
    #endif // !defined(DQN_OS_WIN32)
}

DQN_API Dqn_OSSemaphoreWaitResult Dqn_OS_SemaphoreWait(Dqn_OSSemaphore *semaphore,
                                                       uint32_t         timeout_ms)
{
    Dqn_OSSemaphoreWaitResult result = {};
    if (!Dqn_OS_SemaphoreIsValid(semaphore))
        return result;

    if (timeout_ms == DQN_OS_SEMAPHORE_INFINITE_TIMEOUT) {
        int wait_result = 0;
        do {
            wait_result = sem_wait(&semaphore->posix_handle);
        } while (wait_result == -1 && errno == EINTR);

        if (wait_result == 0)
            result = Dqn_OSSemaphoreWaitResult_Success;
    } else {
        struct timespec abs_timeout = {};
        abs_timeout.tv_sec          = timeout_ms / 1000;
        abs_timeout.tv_nsec         = (timeout_ms % 1000) * 1'000'000;
        if (sem_timedwait(&semaphore->posix_handle, &abs_timeout) == 0)
            result = Dqn_OSSemaphoreWaitResult_Success;
        else if (errno == ETIMEDOUT)
            result = Dqn_OSSemaphoreWaitResult_Timeout;
    }
    return result;
}
#endif // !defined(DQN_NO_SEMAPHORE)

#if !defined(DQN_NO_THREAD)
// NOTE: [$MUTX] Dqn_OSMutex ///////////////////////////////////////////////////////////////////////
DQN_API Dqn_OSMutex Dqn_OS_MutexInit()
{
    Dqn_OSMutex result = {};
    if (pthread_mutexattr_init(&result.posix_attribs) != 0)
        return result;
    if (pthread_mutex_init(&result.posix_handle, &result.posix_attribs) != 0)
        return result;
    return result;
}

DQN_API void Dqn_OS_MutexDeinit(Dqn_OSMutex *mutex)
{
    if (!mutex)
        return;
    pthread_mutexattr_destroy(&mutex->posix_attribs);
    pthread_mutex_destroy(&mutex->posix_handle);
}

DQN_API void Dqn_OS_MutexLock(Dqn_OSMutex *mutex)
{
    if (!mutex)
        return;
    pthread_mutex_lock(&mutex->posix_handle);
}

DQN_API void Dqn_OS_MutexUnlock(Dqn_OSMutex *mutex)
{
    if (!mutex)
        return;
    pthread_mutex_unlock(&mutex->posix_handle);
}

// NOTE: [$THRD] Dqn_OSThread /////////////////////////////////////////////////////////////////////
static void *Dqn_OS_ThreadFunc_(void *user_context)
{
    Dqn_OS_ThreadExecute_(user_context);
    return nullptr;
}

DQN_API bool Dqn_OS_ThreadInit(Dqn_OSThread *thread, Dqn_OSThreadFunc *func, void *user_context)
{
    bool result = false;
    if (!thread)
        return result;

    thread->func           = func;
    thread->user_context   = user_context;
    thread->init_semaphore = Dqn_OS_SemaphoreInit(0 /*initial_count*/);

    // TODO(doyle): Check if semaphore is valid
    // NOTE: pthread_t is essentially the thread ID. In Windows, the handle and
    // the ID are different things. For pthreads then we just duplicate the
    // thread ID to both variables
    pthread_t p_thread = {};
    static_assert(sizeof(p_thread) <= sizeof(thread->handle),
                  "We store the thread handle opaquely in our abstraction, "
                  "there must be enough bytes to store pthread's structure");
    static_assert(sizeof(p_thread) <= sizeof(thread->thread_id),
                  "We store the thread handle opaquely in our abstraction, "
                  "there must be enough bytes to store pthread's structure");

    pthread_attr_t attribs = {};
    pthread_attr_init(&attribs);
    result = pthread_create(&p_thread, &attribs, Dqn_OS_ThreadFunc_, thread) == 0;
    pthread_attr_destroy(&attribs);

    if (result) {
        DQN_MEMCPY(&thread->handle, &p_thread, sizeof(p_thread));
        DQN_MEMCPY(&thread->thread_id, &p_thread, sizeof(p_thread));
    }

    if (result) {
        Dqn_OS_SemaphoreIncrement(&thread->init_semaphore, 1);
    } else {
        Dqn_OS_SemaphoreDeinit(&thread->init_semaphore);
        *thread = {};
    }

    return result;
}

DQN_API void Dqn_OS_ThreadDeinit(Dqn_OSThread *thread)
{
    if (!thread || !thread->handle)
        return;

    pthread_t thread_id = {};
    DQN_MEMCPY(&thread_id, &thread->thread_id, sizeof(thread_id));

    void *return_val = nullptr;
    pthread_join(thread_id, &return_val);
    thread->handle    = {};
    thread->thread_id = {};
}

DQN_API uint32_t Dqn_OS_ThreadID()
{
    pid_t result = gettid();
    DQN_ASSERT(gettid() >= 0);
    return DQN_CAST(uint32_t) result;
}
#endif // !defined(DQN_NO_THREAD)

// NOTE: [$HTTP] Dqn_OSHttp ////////////////////////////////////////////////////////////////////////
#if 0 // TODO(doyle): Implement websockets for Windows and Emscripten
static EM_BOOL EMWebSocketOnOpenCallback(int type, const EmscriptenWebSocketOpenEvent *event, void *user_context)
{
    (void)user_context;
    (void)type;
    (void)event;
    // EMSCRIPTEN_RESULT result = emscripten_websocket_send_utf8_text(event->socket, R"({"jsonrpc":"2.0","id":1,"method": "eth_subscribe","params":["newHeads"]})");
    // if (result)
    //     Dqn_Log_InfoF("Failed to emscripten_websocket_send_utf8_text(): %d\n", result);
    return EM_TRUE;
}

static EM_BOOL EMWebSocketOnMsgCallback(int type, const EmscriptenWebSocketMessageEvent *event __attribute__((nonnull)), void *user_context)
{
    (void)type;
    (void)user_context;
    (void)event;
    if (event->isText) {
        Dqn_Log_InfoF("Received: %.*s", event->numBytes, event->data);
    } else {
        Dqn_Log_InfoF("Received: %d bytes", event->numBytes);
    }
    return EM_TRUE;
}

static EM_BOOL EMWebSocketOnErrorCallback(int type, const EmscriptenWebSocketErrorEvent *event, void *user_context)
{
    (void)user_context;
    (void)type;
    (void)event;
    return EM_TRUE;
}

static EM_BOOL EMWebSocketOnCloseCallback(int type, const EmscriptenWebSocketCloseEvent *event, void *user_context)
{
    (void)user_context;
    (void)type;
    (void)event;
    return EM_TRUE;
}
#endif

#if defined(DQN_PLATFORM_EMSCRIPTEN)
static void Dqn_OS_HttpRequestEMFetchOnSuccessCallback(emscripten_fetch_t *fetch)
{
    Dqn_OSHttpResponse *response = DQN_CAST(Dqn_OSHttpResponse *) fetch->userData;
    if (!DQN_CHECK(response))
        return;

    response->http_status = DQN_CAST(uint32_t) fetch->status;
    response->body        = Dqn_Str8_Alloc(response->arena, fetch->numBytes, Dqn_ZeroMem_No);
    if (response->body.data)
        DQN_MEMCPY(response->body.data, fetch->data, fetch->numBytes);

    Dqn_OS_SemaphoreIncrement(&response->on_complete_semaphore, 1);
    Dqn_Atomic_AddU32(&response->done, 1);
}

static void Dqn_OS_HttpRequestEMFetchOnErrorCallback(emscripten_fetch_t *fetch)
{
    Dqn_OSHttpResponse *response = DQN_CAST(Dqn_OSHttpResponse *) fetch->userData;
    if (!DQN_CHECK(response))
        return;

    response->http_status = DQN_CAST(uint32_t) fetch->status;
    response->body        = Dqn_Str8_Alloc(response->arena, fetch->numBytes, Dqn_ZeroMem_No);
    if (response->body.size)
        DQN_MEMCPY(response->body.data, fetch->data, fetch->numBytes);

    Dqn_OS_SemaphoreIncrement(&response->on_complete_semaphore, 1);
    Dqn_Atomic_AddU32(&response->done, 1);
}
#endif

DQN_API void Dqn_OS_HttpRequestAsync(Dqn_OSHttpResponse     *response,
                                     Dqn_Arena              *arena,
                                     Dqn_Str8                host,
                                     Dqn_Str8                path,
                                     Dqn_OSHttpRequestSecure secure,
                                     Dqn_Str8                method,
                                     Dqn_Str8                body,
                                     Dqn_Str8                headers)
{
    if (!response || !arena)
        return;

    response->arena = arena;
    response->builder.arena =
        response->tmem_arena ? response->tmem_arena : &response->tmp_arena;

    Dqn_Arena  *tmem  = response->tmem_arena;
    Dqn_TLSTMem tmem_ = Dqn_TLS_TMem(arena);
    if (!tmem)
        tmem = tmem_.arena;

#if defined(DQN_PLATFORM_EMSCRIPTEN)
    emscripten_fetch_attr_t fetch_attribs = {};
    emscripten_fetch_attr_init(&fetch_attribs);

    if (method.size >= sizeof(fetch_attribs.requestMethod)) {
        response->error_msg =
            Dqn_Str8_InitF(arena,
                           "Request method in EM has a size limit of 31 characters, method was "
                           "'%.*s' which is %zu characters long",
                           DQN_STR_FMT(method),
                           method.size);
        DQN_CHECKF(method.size < sizeof(fetch_attribs.requestMethod),
                   "%.*s",
                   DQN_STR_FMT(response->error_msg));
        response->error_code = DQN_CAST(uint32_t) - 1;
        Dqn_Atomic_AddU32(&response->done, 1);
        return;
    }

    DQN_MEMCPY(fetch_attribs.requestMethod, method.data, method.size);

    fetch_attribs.requestData     = body.data;
    fetch_attribs.requestDataSize = body.size;
    fetch_attribs.attributes      = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    fetch_attribs.onsuccess       = Dqn_OS_HttpRequestEMFetchOnSuccessCallback;
    fetch_attribs.onerror         = Dqn_OS_HttpRequestEMFetchOnErrorCallback;
    fetch_attribs.userData        = response;

    Dqn_Str8 url = Dqn_Str8_InitF(scratch_arena, "%.*s%.*s", DQN_STR_FMT(host), DQN_STR_FMT(path));
    Dqn_Log_InfoF("Initiating HTTP '%s' request to '%.*s' with payload '%.*s'",
                  fetch_attribs.requestMethod,
                  DQN_STR_FMT(url),
                  DQN_STR_FMT(body));
    response->on_complete_semaphore = Dqn_OS_SemaphoreInit(0);
    response->em_handle             = emscripten_fetch(&fetch_attribs, url.data);
#else // #elif defined(DQN_OS_WIN32)
    DQN_INVALID_CODE_PATHF("Unimplemented function");
#endif
}

DQN_API void Dqn_OS_HttpRequestFree(Dqn_OSHttpResponse *response)
{
// NOTE: Cleanup
#if defined(DQN_PLATFORM_EMSCRIPTEN)
    if (response->em_handle) {
        emscripten_fetch_close(response->em_handle);
        response->em_handle = nullptr;
    }
#endif // #elif defined(DQN_OS_WIN32)

    Dqn_Arena_Deinit(&response->tmp_arena);
    if (Dqn_OS_SemaphoreIsValid(&response->on_complete_semaphore))
        Dqn_OS_SemaphoreDeinit(&response->on_complete_semaphore);
    *response = {};
}
