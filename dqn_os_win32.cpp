#pragma once
#include "dqn.h"

/*
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//    $$$$$$\   $$$$$$\        $$\      $$\ $$$$$$\ $$\   $$\  $$$$$$\   $$$$$$\
//   $$  __$$\ $$  __$$\       $$ | $\  $$ |\_$$  _|$$$\  $$ |$$ ___$$\ $$  __$$\
//   $$ /  $$ |$$ /  \__|      $$ |$$$\ $$ |  $$ |  $$$$\ $$ |\_/   $$ |\__/  $$ |
//   $$ |  $$ |\$$$$$$\        $$ $$ $$\$$ |  $$ |  $$ $$\$$ |  $$$$$ /  $$$$$$  |
//   $$ |  $$ | \____$$\       $$$$  _$$$$ |  $$ |  $$ \$$$$ |  \___$$\ $$  ____/
//   $$ |  $$ |$$\   $$ |      $$$  / \$$$ |  $$ |  $$ |\$$$ |$$\   $$ |$$ |
//    $$$$$$  |\$$$$$$  |      $$  /   \$$ |$$$$$$\ $$ | \$$ |\$$$$$$  |$$$$$$$$\
//    \______/  \______/       \__/     \__|\______|\__|  \__| \______/ \________|
//
//   dqn_os_win32.cpp
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

// NOTE: [$VMEM] Dqn_OSMem /////////////////////////////////////////////////////////////////////////
static uint32_t Dqn_OS_MemConvertPageToOSFlags_(uint32_t protect)
{
    DQN_ASSERT((protect & ~Dqn_OSMemPage_All) == 0);
    DQN_ASSERT(protect != 0);
    uint32_t result = 0;

    if (protect & Dqn_OSMemPage_NoAccess) {
        result = PAGE_NOACCESS;
    } else {
        if (protect & Dqn_OSMemPage_ReadWrite) {
            result = PAGE_READWRITE;
        }  else if (protect & Dqn_OSMemPage_Read) {
            result = PAGE_READONLY;
        } else if (protect & Dqn_OSMemPage_Write) {
            Dqn_Log_WarningF("Windows does not support write-only pages, granting read+write access");
            result = PAGE_READWRITE;
        }
    }

    if (protect & Dqn_OSMemPage_Guard)
        result |= PAGE_GUARD;

    DQN_ASSERTF(result != PAGE_GUARD, "Page guard is a modifier, you must also specify a page permission like read or/and write");
    return result;
}

DQN_API void *Dqn_OS_MemReserve(Dqn_usize size, Dqn_OSMemCommit commit, uint32_t page_flags)
{
    unsigned long os_page_flags = Dqn_OS_MemConvertPageToOSFlags_(page_flags);
    unsigned long flags         = MEM_RESERVE | (commit == Dqn_OSMemCommit_Yes ? MEM_COMMIT : 0);
    void         *result        = VirtualAlloc(nullptr, size, flags, os_page_flags);
    return result;
}

DQN_API bool Dqn_OS_MemCommit(void *ptr, Dqn_usize size, uint32_t page_flags)
{
    bool result = false;
    if (!ptr || size == 0)
        return false;
    unsigned long os_page_flags = Dqn_OS_MemConvertPageToOSFlags_(page_flags);
    result                      = VirtualAlloc(ptr, size, MEM_COMMIT, os_page_flags) != nullptr;
    return result;
}

DQN_API void Dqn_OS_MemDecommit(void *ptr, Dqn_usize size)
{
    // NOTE: This is a decommit call, which is explicitly saying to free the
    // pages but not the address space, you would use OS_MemRelease to release
    // everything.
    DQN_MSVC_WARNING_PUSH
    DQN_MSVC_WARNING_DISABLE(6250) // Calling 'VirtualFree' without the MEM_RELEASE flag might free memory but not address descriptors (VADs).  This causes address space leaks.
    VirtualFree(ptr, size, MEM_DECOMMIT);
    DQN_MSVC_WARNING_POP
}

DQN_API void Dqn_OS_MemRelease(void *ptr, Dqn_usize size)
{
    (void)size;
    VirtualFree(ptr, 0, MEM_RELEASE);
}

DQN_API int Dqn_OS_MemProtect(void *ptr, Dqn_usize size, uint32_t page_flags)
{
    if (!ptr || size == 0)
        return 0;

    static Dqn_Str8 const ALIGNMENT_ERROR_MSG =
        DQN_STR8("Page protection requires pointers to be page aligned because we "
                 "can only guard memory at a multiple of the page boundary.");
    DQN_ASSERTF(Dqn_IsPowerOfTwoAligned(DQN_CAST(uintptr_t)ptr, g_dqn_library->os_page_size), "%s", ALIGNMENT_ERROR_MSG.data);
    DQN_ASSERTF(Dqn_IsPowerOfTwoAligned(size,                   g_dqn_library->os_page_size), "%s", ALIGNMENT_ERROR_MSG.data);

    unsigned long os_page_flags = Dqn_OS_MemConvertPageToOSFlags_(page_flags);
    unsigned long prev_flags    = 0;
    int           result        = VirtualProtect(ptr, size, os_page_flags, &prev_flags);

    (void)prev_flags;
    if (result == 0)
        DQN_ASSERTF(result, "VirtualProtect failed");
    return result;
}

// NOTE: [$DATE] Date //////////////////////////////////////////////////////////////////////////////
DQN_API Dqn_OSDateTime Dqn_OS_DateLocalTimeNow()
{
    SYSTEMTIME sys_time;
    GetLocalTime(&sys_time);

    Dqn_OSDateTime result = {};
    result.hour           = DQN_CAST(uint8_t) sys_time.wHour;
    result.minutes        = DQN_CAST(uint8_t) sys_time.wMinute;
    result.seconds        = DQN_CAST(uint8_t) sys_time.wSecond;
    result.day            = DQN_CAST(uint8_t) sys_time.wDay;
    result.month          = DQN_CAST(uint8_t) sys_time.wMonth;
    result.year           = DQN_CAST(int16_t) sys_time.wYear;
    return result;
}

const uint64_t DQN_OS_WIN32_UNIX_TIME_START            = 0x019DB1DED53E8000; // January 1, 1970 (start of Unix epoch) in "ticks"
const uint64_t DQN_OS_WIN32_FILE_TIME_TICKS_PER_SECOND = 10'000'000;         // Filetime returned is in intervals of 100 nanoseconds

DQN_API uint64_t Dqn_OS_DateUnixTime()
{
    FILETIME file_time;
    GetSystemTimeAsFileTime(&file_time);

    LARGE_INTEGER date_time;
    date_time.u.LowPart  = file_time.dwLowDateTime;
    date_time.u.HighPart = file_time.dwHighDateTime;
    uint64_t result      = (date_time.QuadPart - DQN_OS_WIN32_UNIX_TIME_START) / DQN_OS_WIN32_FILE_TIME_TICKS_PER_SECOND;
    return result;
}

DQN_API Dqn_OSDateTime Dqn_OS_DateUnixTimeToDate(uint64_t time)
{
    // NOTE: Windows epoch time starts from Jan 1, 1601 and counts in
    // 100-nanoseconds intervals.
    //
    // See: https://devblogs.microsoft.com/oldnewthing/20090306-00/?p=18913

    uint64_t   win32_time    = 116'444'736'000'000'000 + (time * 10'000'000);
    SYSTEMTIME sys_time      = {};
    FILETIME   file_time     = {};
    file_time.dwLowDateTime  = (DWORD)win32_time;
    file_time.dwHighDateTime = win32_time >> 32;
    FileTimeToSystemTime(&file_time, &sys_time);

    Dqn_OSDateTime result = {};
    result.year           = DQN_CAST(uint16_t)sys_time.wYear;
    result.month          = DQN_CAST(uint8_t)sys_time.wMonth;
    result.day            = DQN_CAST(uint8_t)sys_time.wDay;
    result.hour           = DQN_CAST(uint8_t)sys_time.wHour;
    result.minutes        = DQN_CAST(uint8_t)sys_time.wMinute;
    result.seconds        = DQN_CAST(uint8_t)sys_time.wSecond;
    return result;
}

DQN_API uint64_t Dqn_OS_DateToUnixTime(Dqn_OSDateTime date)
{
    DQN_ASSERT(Dqn_OS_DateIsValid(date));

    SYSTEMTIME sys_time = {};
    sys_time.wYear      = date.year;
    sys_time.wMonth     = date.month;
    sys_time.wDay       = date.day;
    sys_time.wHour      = date.hour;
    sys_time.wMinute    = date.minutes;
    sys_time.wSecond    = date.seconds;

    FILETIME file_time = {};
    SystemTimeToFileTime(&sys_time, &file_time);

    LARGE_INTEGER date_time;
    date_time.u.LowPart  = file_time.dwLowDateTime;
    date_time.u.HighPart = file_time.dwHighDateTime;

    uint64_t result = (date_time.QuadPart - DQN_OS_WIN32_UNIX_TIME_START) / DQN_OS_WIN32_FILE_TIME_TICKS_PER_SECOND;
    return result;
}

DQN_API bool Dqn_OS_SecureRNGBytes(void *buffer, uint32_t size)
{
    if (!buffer || size < 0)
        return false;

    if (size == 0)
        return true;

    bool init = true;
    Dqn_TicketMutex_Begin(&g_dqn_library->win32_bcrypt_rng_mutex);
    if (!g_dqn_library->win32_bcrypt_rng_handle)
    {
        wchar_t const BCRYPT_ALGORITHM[] = L"RNG";
        long /*NTSTATUS*/ init_status = BCryptOpenAlgorithmProvider(&g_dqn_library->win32_bcrypt_rng_handle, BCRYPT_ALGORITHM, nullptr /*implementation*/, 0 /*flags*/);
        if (!g_dqn_library->win32_bcrypt_rng_handle || init_status != 0)
        {
            Dqn_Log_ErrorF("Failed to initialise random number generator, error: %d", init_status);
            init = false;
        }
    }
    Dqn_TicketMutex_End(&g_dqn_library->win32_bcrypt_rng_mutex);

    if (!init)
        return false;

    long gen_status = BCryptGenRandom(g_dqn_library->win32_bcrypt_rng_handle, DQN_CAST(unsigned char *)buffer, size, 0 /*flags*/);
    if (gen_status != 0)
    {
        Dqn_Log_ErrorF("Failed to generate random bytes: %d", gen_status);
        return false;
    }

    return true;
}

DQN_API Dqn_Str8 Dqn_OS_EXEPath(Dqn_Arena *arena)
{
    Dqn_Str8 result = {};
    if (!arena)
        return result;
    Dqn_TLSTMem t_mem      = Dqn_TLS_TMem(arena);
    Dqn_Str16   exe_dir16 = Dqn_Win_EXEPathW(t_mem.arena);
    result                = Dqn_Win_Str16ToStr8(arena, exe_dir16);
    return result;
}

DQN_API void Dqn_OS_SleepMs(Dqn_uint milliseconds)
{
    Sleep(milliseconds);
}

DQN_API uint64_t Dqn_OS_PerfCounterFrequency()
{
    uint64_t result = g_dqn_library->win32_qpc_frequency.QuadPart;
    DQN_ASSERTF(result, "Initialise the library with Dqn_Library_Init() to get a valid QPC frequency value");
    return result;
}

DQN_API uint64_t Dqn_OS_PerfCounterNow()
{
    LARGE_INTEGER integer = {};
    QueryPerformanceCounter(&integer);
    uint64_t result = integer.QuadPart;
    return result;
}

#if !defined(DQN_NO_OS_FILE_API)
static uint64_t Dqn_Win_FileTimeToSeconds_(FILETIME const *time)
{
    ULARGE_INTEGER time_large_int = {};
    time_large_int.u.LowPart      = time->dwLowDateTime;
    time_large_int.u.HighPart     = time->dwHighDateTime;
    uint64_t result                = (time_large_int.QuadPart / 10000000ULL) - 11644473600ULL;
    return result;
}

DQN_API Dqn_OSPathInfo Dqn_OS_PathInfo(Dqn_Str8 path)
{
    Dqn_OSPathInfo result = {};
    if (!Dqn_Str8_HasData(path))
        return result;

    Dqn_TLSTMem t_mem   = Dqn_TLS_TMem(nullptr);
    Dqn_Str16   path16 = Dqn_Win_Str8ToStr16(t_mem.arena, path);

    WIN32_FILE_ATTRIBUTE_DATA attrib_data = {};
    if (!GetFileAttributesExW(path16.data, GetFileExInfoStandard, &attrib_data))
        return result;

    result.exists                = true;
    result.create_time_in_s      = Dqn_Win_FileTimeToSeconds_(&attrib_data.ftCreationTime);
    result.last_access_time_in_s = Dqn_Win_FileTimeToSeconds_(&attrib_data.ftLastAccessTime);
    result.last_write_time_in_s  = Dqn_Win_FileTimeToSeconds_(&attrib_data.ftLastWriteTime);

    LARGE_INTEGER large_int = {};
    large_int.u.HighPart    = DQN_CAST(int32_t)attrib_data.nFileSizeHigh;
    large_int.u.LowPart     = attrib_data.nFileSizeLow;
    result.size             = (uint64_t)large_int.QuadPart;

    if (attrib_data.dwFileAttributes != INVALID_FILE_ATTRIBUTES) {
        if (attrib_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            result.type = Dqn_OSPathInfoType_Directory;
        else
            result.type = Dqn_OSPathInfoType_File;
    }

    return result;
}

DQN_API bool Dqn_OS_PathDelete(Dqn_Str8 path)
{
    bool result = false;
    if (!Dqn_Str8_HasData(path))
        return result;

    Dqn_TLSTMem t_mem = Dqn_TLS_TMem(nullptr);
    Dqn_Str16   path16  = Dqn_Win_Str8ToStr16(t_mem.arena, path);
    if (path16.size) {
        result = DeleteFileW(path16.data);
        if (!result)
            result = RemoveDirectoryW(path16.data);
    }
    return result;
}

DQN_API bool Dqn_OS_FileExists(Dqn_Str8 path)
{
    bool result = false;
    if (!Dqn_Str8_HasData(path))
        return result;

    Dqn_TLSTMem t_mem = Dqn_TLS_TMem(nullptr);
    Dqn_Str16   path16  = Dqn_Win_Str8ToStr16(t_mem.arena, path);
    if (path16.size) {
        WIN32_FILE_ATTRIBUTE_DATA attrib_data = {};
        if (GetFileAttributesExW(path16.data, GetFileExInfoStandard, &attrib_data)) {
            result = (attrib_data.dwFileAttributes != INVALID_FILE_ATTRIBUTES) &&
                     !(attrib_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
        }
    }
    return result;
}

DQN_API bool Dqn_OS_CopyFile(Dqn_Str8 src, Dqn_Str8 dest, bool overwrite, Dqn_ErrorSink *error)
{
    bool         result  = false;
    Dqn_TLSTMem  t_mem = Dqn_TLS_TMem(nullptr);
    Dqn_Str16    src16   = Dqn_Win_Str8ToStr16(t_mem.arena, src);
    Dqn_Str16    dest16  = Dqn_Win_Str8ToStr16(t_mem.arena, dest);

    int fail_if_exists = overwrite == false;
    result             = CopyFileW(src16.data, dest16.data, fail_if_exists) != 0;

    if (!result) {
        Dqn_WinError win_error = Dqn_Win_LastError(t_mem.arena);
        Dqn_ErrorSink_MakeF(error,
                            win_error.code,
                            "Failed to copy file '%.*s' to '%.*s': (%u) %.*s",
                            DQN_STR_FMT(src),
                            DQN_STR_FMT(dest),
                            win_error.code,
                            DQN_STR_FMT(win_error.msg));
    }
    return result;
}

DQN_API bool Dqn_OS_MoveFile(Dqn_Str8 src, Dqn_Str8 dest, bool overwrite, Dqn_ErrorSink *error)
{
    bool        result  = false;
    Dqn_TLSTMem t_mem = Dqn_TLS_TMem(nullptr);
    Dqn_Str16   src16   = Dqn_Win_Str8ToStr16(t_mem.arena, src);
    Dqn_Str16   dest16  = Dqn_Win_Str8ToStr16(t_mem.arena, dest);

    unsigned long flags = MOVEFILE_COPY_ALLOWED;
    if (overwrite) {
        flags |= MOVEFILE_REPLACE_EXISTING;
    }

    result = MoveFileExW(src16.data, dest16.data, flags) != 0;
    if (!result) {
        Dqn_WinError win_error = Dqn_Win_LastError(t_mem.arena);
        Dqn_ErrorSink_MakeF(error,
                            win_error.code,
                            "Failed to move file '%.*s' to '%.*s': (%u) %.*s",
                            DQN_STR_FMT(src),
                            DQN_STR_FMT(dest),
                            win_error.code,
                            DQN_STR_FMT(win_error.msg));
    }
    return result;
}

DQN_API bool Dqn_OS_MakeDir(Dqn_Str8 path)
{
    bool        result = true;
    Dqn_TLSTMem t_mem  = Dqn_TLS_TMem(nullptr);
    Dqn_Str16   path16 = Dqn_Win_Str8ToStr16(t_mem.arena, path);

    // NOTE: Go back from the end of the string to all the directories in the
    // string, and try to create them. Since Win32 API cannot create
    // intermediate directories that don't exist in a path we need to go back
    // and record all the directories until we encounter one that exists.
    //
    // From that point onwards go forwards and make all the directories
    // inbetween by null-terminating the string temporarily, creating the
    // directory and so forth until we reach the end.
    //
    // If we find a file at some point in the path we fail out because the
    // series of directories can not be made if a file exists with the same
    // name.
    for (Dqn_usize index = 0; index < path16.size; index++) {
        bool first_char = index == (path16.size - 1);
        wchar_t ch      = path16.data[index];
        if (ch == '/' || ch == '\\' || first_char) {
            wchar_t temp = path16.data[index];
            if (!first_char)
                path16.data[index] = 0; // Temporarily null terminate it

            WIN32_FILE_ATTRIBUTE_DATA attrib_data = {};
            bool successful = GetFileAttributesExW(path16.data, GetFileExInfoStandard, &attrib_data); // Check

            if (successful) {
                if (attrib_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    // NOTE: The directory exists, continue iterating the path
                } else {
                    // NOTE: There's some kind of file that exists at the path
                    // but it's not a directory. This request to make a 
                    // directory is invalid.
                    return false;
                }
            } else {
                // NOTE: There's nothing that exists at this path, we can create
                // a directory here
                result |= (CreateDirectoryW(path16.data, nullptr) == 0);
            }

            if (!first_char)
                path16.data[index] = temp; // Undo null termination
        }
    }
    return result;
}


DQN_API bool Dqn_OS_DirExists(Dqn_Str8 path)
{
    bool result = false;
    if (!Dqn_Str8_HasData(path))
        return result;

    Dqn_TLSTMem t_mem = Dqn_TLS_TMem(nullptr);
    Dqn_Str16   path16  = Dqn_Win_Str8ToStr16(t_mem.arena, path);
    if (path16.size) {
        WIN32_FILE_ATTRIBUTE_DATA attrib_data = {};
        if (GetFileAttributesExW(path16.data, GetFileExInfoStandard, &attrib_data)) {
            result = (attrib_data.dwFileAttributes != INVALID_FILE_ATTRIBUTES) &&
                     (attrib_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
        }
    }

    return result;
}

// NOTE: R/W Stream API ////////////////////////////////////////////////////////////////////////////
DQN_API Dqn_OSFile Dqn_OS_FileOpen(Dqn_Str8 path, Dqn_OSFileOpen open_mode, uint32_t access, Dqn_ErrorSink *error)
{
    Dqn_OSFile result = {};
    if (!Dqn_Str8_HasData(path) || path.size <= 0)
        return result;

    if ((access & ~Dqn_OSFileAccess_All) || ((access & Dqn_OSFileAccess_All) == 0)) {
        DQN_INVALID_CODE_PATH;
        return result;
    }

    unsigned long create_flag = 0;
    switch (open_mode) {
        case Dqn_OSFileOpen_CreateAlways: create_flag = CREATE_ALWAYS; break;
        case Dqn_OSFileOpen_OpenIfExist:  create_flag = OPEN_EXISTING; break;
        case Dqn_OSFileOpen_OpenAlways:   create_flag = OPEN_ALWAYS;   break;
        default: DQN_INVALID_CODE_PATH; return result;
    }

    unsigned long access_mode = 0;
    if (access & Dqn_OSFileAccess_AppendOnly) {
        DQN_ASSERTF((access & ~Dqn_OSFileAccess_AppendOnly) == 0,
                    "Append can only be applied exclusively to the file, other access modes not permitted");
        access_mode = FILE_APPEND_DATA;
    } else {
        if (access & Dqn_OSFileAccess_Read)
            access_mode |= GENERIC_READ;
        if (access & Dqn_OSFileAccess_Write)
            access_mode |= GENERIC_WRITE;
        if (access & Dqn_OSFileAccess_Execute)
            access_mode |= GENERIC_EXECUTE;
    }

    Dqn_TLSTMem t_mem = Dqn_TLS_TMem(nullptr);
    Dqn_Str16   path16  = Dqn_Win_Str8ToStr16(t_mem.arena, path);
    void       *handle  = CreateFileW(/*LPCWSTR               lpFileName*/ path16.data,
                                      /*DWORD                 dwDesiredAccess*/ access_mode,
                                      /*DWORD                 dwShareMode*/ 0,
                                      /*LPSECURITY_ATTRIBUTES lpSecurityAttributes*/ nullptr,
                                      /*DWORD                 dwCreationDisposition*/ create_flag,
                                      /*DWORD                 dwFlagsAndAttributes*/ FILE_ATTRIBUTE_NORMAL,
                                      /*HANDLE                hTemplateFile*/ nullptr);

    if (handle == INVALID_HANDLE_VALUE) {
        Dqn_WinError win_error = Dqn_Win_LastError(t_mem.arena);
        result.error           = true;
        Dqn_ErrorSink_MakeF(error, win_error.code, "Failed to open file at '%.*s': '%.*s'", DQN_STR_FMT(path), DQN_STR_FMT(win_error.msg));
        return result;
    }

    result.handle = handle;
    return result;
}

DQN_API bool Dqn_OS_FileRead(Dqn_OSFile *file, void *buffer, Dqn_usize size, Dqn_ErrorSink *error)
{
    if (!file || !file->handle || file->error || !buffer || size <= 0)
        return false;

    Dqn_TLSTMem t_mem = Dqn_TLS_TMem(nullptr);
    if (!DQN_CHECK(size <= (unsigned long)-1)) {
        Dqn_Str8    buffer_size_str8 = Dqn_U64ToByteSizeStr8(t_mem.arena, size, Dqn_U64ByteSizeType_Auto);
        Dqn_ErrorSink_MakeF(
            error,
            1 /*error_code*/,
            "Current implementation doesn't support reading >4GiB file (requested %.*s), implement Win32 overlapped IO",
            DQN_STR_FMT(buffer_size_str8));
        return false;
    }

    unsigned long bytes_read  = 0;
    unsigned long read_result = ReadFile(/*HANDLE       hFile*/                 file->handle,
                                         /*LPVOID       lpBuffer*/              buffer,
                                         /*DWORD        nNumberOfBytesToRead*/  DQN_CAST(unsigned long)size,
                                         /*LPDWORD      lpNumberOfByesRead*/   &bytes_read,
                                         /*LPOVERLAPPED lpOverlapped*/          nullptr);
    if (read_result == 0) {
        Dqn_WinError win_error = Dqn_Win_LastError(t_mem.arena);
        Dqn_ErrorSink_MakeF(error, win_error.code, "Failed to read data from file: (%u) %.*s", win_error.code, DQN_STR_FMT(win_error.msg));
        return false;
    }

    if (bytes_read != size) {
        Dqn_WinError win_error = Dqn_Win_LastError(t_mem.arena);
        Dqn_ErrorSink_MakeF(
            error,
            win_error.code,
            "Failed to read the desired number of bytes from file, we read %uB but we expected %uB: (%u) %.*s",
            bytes_read,
            DQN_CAST(unsigned long)size,
            win_error.code,
            DQN_STR_FMT(win_error.msg));
        return false;
    }

    return true;
}

DQN_API bool Dqn_OS_FileWritePtr(Dqn_OSFile *file, void const *buffer, Dqn_usize size, Dqn_ErrorSink *error)
{
    if (!file || !file->handle || file->error || !buffer || size <= 0)
        return false;

    bool        result   = true;
    char const *end      = DQN_CAST(char *) buffer + size;
    for (char const *ptr = DQN_CAST(char const *) buffer; result && ptr != end;) {
        unsigned long write_size     = DQN_CAST(unsigned long)DQN_MIN((unsigned long)-1, end - ptr);
        unsigned long bytes_written  = 0;
        result                       = WriteFile(file->handle, ptr, write_size, &bytes_written, nullptr /*lpOverlapped*/) != 0;
        ptr                         += bytes_written;
    }

    if (!result) {
        Dqn_TLSTMem  t_mem          = Dqn_TLS_TMem(nullptr);
        Dqn_WinError win_error        = Dqn_Win_LastError(t_mem.arena);
        Dqn_Str8     buffer_size_str8 = Dqn_U64ToByteSizeStr8(t_mem.arena, size, Dqn_U64ByteSizeType_Auto);
        Dqn_ErrorSink_MakeF(error, win_error.code, "Failed to write buffer (%.*s) to file handle: %.*s", DQN_STR_FMT(buffer_size_str8), DQN_STR_FMT(win_error.msg));
    }
    return result;
}

DQN_API void Dqn_OS_FileClose(Dqn_OSFile *file)
{
    if (!file || !file->handle || file->error)
        return;
    CloseHandle(file->handle);
    *file = {};
}
#endif // !defined(DQN_NO_OS_FILE_API)

// NOTE: [$EXEC] Dqn_OSExec ////////////////////////////////////////////////////////////////////////
DQN_API void Dqn_OS_Exit(int32_t exit_code)
{
    ExitProcess(DQN_CAST(UINT)exit_code);
}

DQN_API Dqn_OSExecResult Dqn_OS_ExecWait(Dqn_OSExecAsyncHandle handle, Dqn_Arena *arena, Dqn_ErrorSink *error)
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
        DQN_ASSERT(!handle.process);
        return result;
    }

    Dqn_TLSTMem     t_mem          = Dqn_TLS_TMem(arena);
    Dqn_Str8Builder stdout_builder = {};
    Dqn_Str8Builder stderr_builder = {};
    if (arena) {
        stdout_builder.arena = t_mem.arena;
        stderr_builder.arena = t_mem.arena;
    }

    DWORD const SLOW_WAIT_TIME_MS = 100;
    DWORD const FAST_WAIT_TIME_MS = 20;
    DWORD       wait_ms           = FAST_WAIT_TIME_MS;
    DWORD       exec_result       = WAIT_TIMEOUT;
    while (exec_result == WAIT_TIMEOUT) {
        exec_result = WaitForSingleObject(handle.process, wait_ms);
        if (exec_result == WAIT_FAILED) {
            Dqn_WinError win_error = Dqn_Win_LastError(t_mem.arena);
            result.os_error_code   = win_error.code;
            Dqn_ErrorSink_MakeF(error, result.os_error_code, "Executed command failed to terminate: %.*s", DQN_STR_FMT(win_error.msg));
        } else if (DQN_CHECK(exec_result == WAIT_TIMEOUT || exec_result == WAIT_OBJECT_0)) {
            // NOTE: If the pipes are full, the process will block. We
            // periodically flush the pipes to make sure this doesn't happen

            // NOTE: Read stdout from process //////////////////////////////////////////////////////
            // TODO: Allow the user to pump the execution of the command w/ a user passed in buffer.
            char  output_buffer[DQN_KILOBYTES(8)];
            DWORD stdout_bytes_available = 0;
            if (PeekNamedPipe(handle.stdout_read, nullptr, 0, nullptr, &stdout_bytes_available, nullptr)) {
                if (stdout_bytes_available) {
                    for (;;) {
                        DWORD bytes_read = 0;
                        BOOL  success    = ReadFile(handle.stdout_read, output_buffer, sizeof(output_buffer), &bytes_read, NULL);
                        if (!success || bytes_read == 0)
                            break;

                        if (handle.stdout_write && arena)
                            Dqn_Str8Builder_AddF(&stdout_builder, "%.*s", bytes_read, output_buffer);

                        if (bytes_read < sizeof(output_buffer))
                            break;
                    }
                }
            }

            // NOTE: Read stderr from process //////////////////////////////////////////////////////
            DWORD stderr_bytes_available = 0;
            if (PeekNamedPipe(handle.stderr_read, nullptr, 0, nullptr, &stderr_bytes_available, nullptr)) {
                if (stderr_bytes_available) {
                    for (;;) {
                        DWORD bytes_read = 0;
                        BOOL  success    = ReadFile(handle.stderr_read, output_buffer, sizeof(output_buffer), &bytes_read, NULL);
                        if (!success || bytes_read == 0)
                            break;

                        if (handle.stderr_read && arena)
                            Dqn_Str8Builder_AddF(&stderr_builder, "%.*s", bytes_read, output_buffer);

                        if (bytes_read < sizeof(output_buffer))
                            break;
                    }
                }
            }

            // NOTE: If we produced some data, we'll sleep for a short amount of
            // time because if we have the presence of data it's typical we deal
            // with N amount of. Otherwise if we timed-out and we didn't produce
            // any data, its more likely this is a long running operation so we
            // sleep longer to avoid slamming the CPU.
            if (stdout_bytes_available || stderr_bytes_available) {
                wait_ms = FAST_WAIT_TIME_MS;
            } else {
                wait_ms = SLOW_WAIT_TIME_MS;
            }
        }
    }

    // NOTE: Get stdout/stderr. If no arena is passed this is a no-op //////////////////////////////
    result.stdout_text = Dqn_Str8Builder_Build(&stdout_builder, arena);
    result.stderr_text = Dqn_Str8Builder_Build(&stderr_builder, arena);

    // NOTE: Get exit code /////////////////////////////////////////////////////////////////////////
    if (exec_result != WAIT_FAILED) {
        DWORD exit_status;
        if (GetExitCodeProcess(handle.process, &exit_status)) {
            result.exit_code = exit_status;
        } else {
            Dqn_WinError win_error = Dqn_Win_LastError(t_mem.arena);
            result.os_error_code   = win_error.code;
            Dqn_ErrorSink_MakeF(error,
                                result.os_error_code,
                                "Failed to retrieve command exit code: %.*s",
                                DQN_STR_FMT(win_error.msg));
        }
    }

    // NOTE: Cleanup ///////////////////////////////////////////////////////////////////////////////
    CloseHandle(handle.stdout_write);
    CloseHandle(handle.stderr_write);
    CloseHandle(handle.stdout_read);
    CloseHandle(handle.stderr_read);
    CloseHandle(handle.process);
    return result;
}

DQN_API Dqn_OSExecAsyncHandle Dqn_OS_ExecAsync(Dqn_Slice<Dqn_Str8> cmd_line, Dqn_Str8 working_dir, uint8_t exec_flags, Dqn_ErrorSink *error)
{
    // NOTE: Pre-amble /////////////////////////////////////////////////////////////////////////////
    Dqn_OSExecAsyncHandle result = {};
    if (cmd_line.size == 0)
        return result;

    Dqn_TLSTMem t_mem       = Dqn_TLS_TMem(nullptr);
    Dqn_Str8    cmd_rendered  = Dqn_Slice_Str8Render(t_mem.arena, cmd_line, DQN_STR8(" "));
    Dqn_Str16   cmd16         = Dqn_Win_Str8ToStr16(t_mem.arena, cmd_rendered);
    Dqn_Str16   working_dir16 = Dqn_Win_Str8ToStr16(t_mem.arena, working_dir);

    // NOTE: Stdout/err security attributes ////////////////////////////////////////////////////////
    SECURITY_ATTRIBUTES save_std_security_attribs = {};
    save_std_security_attribs.nLength             = sizeof(save_std_security_attribs);
    save_std_security_attribs.bInheritHandle      = true;

    // NOTE: Redirect stdout ///////////////////////////////////////////////////////////////////////
    HANDLE stdout_read  = {};
    HANDLE stdout_write = {};
    DQN_DEFER {
        if (result.os_error_code || result.exit_code) {
            CloseHandle(stdout_read);
            CloseHandle(stdout_write);
        }
    };

    if (Dqn_Bit_IsSet(exec_flags, Dqn_OSExecFlag_SaveStdout)) {
        if (!CreatePipe(&stdout_read, &stdout_write, &save_std_security_attribs, /*nSize*/ 0)) {
            Dqn_WinError win_error = Dqn_Win_LastError(t_mem.arena);
            result.os_error_code   = win_error.code;
            Dqn_ErrorSink_MakeF(
                error,
                result.os_error_code,
                "Failed to create stdout pipe to redirect the output of the command '%.*s': %.*s",
                DQN_STR_FMT(cmd_rendered),
                DQN_STR_FMT(win_error.msg));
            return result;
        }

        if (!SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0)) {
            Dqn_WinError win_error = Dqn_Win_LastError(t_mem.arena);
            result.os_error_code   = win_error.code;
            Dqn_ErrorSink_MakeF(error,
                                result.os_error_code,
                                "Failed to make stdout 'read' pipe non-inheritable when trying to "
                                "execute command '%.*s': %.*s",
                                DQN_STR_FMT(cmd_rendered),
                                DQN_STR_FMT(win_error.msg));
            return result;
        }
    }

    // NOTE: Redirect stderr ///////////////////////////////////////////////////////////////////////
    HANDLE stderr_read  = {};
    HANDLE stderr_write = {};
    DQN_DEFER {
        if (result.os_error_code || result.exit_code) {
            CloseHandle(stderr_read);
            CloseHandle(stderr_write);
        }
    };

    if (Dqn_Bit_IsSet(exec_flags, Dqn_OSExecFlag_SaveStderr)) {
        if (Dqn_Bit_IsSet(exec_flags, Dqn_OSExecFlag_MergeStderrToStdout)) {
            stderr_read  = stdout_read;
            stderr_write = stdout_write;
        } else {
            if (!CreatePipe(&stderr_read, &stderr_write, &save_std_security_attribs, /*nSize*/ 0)) {
                Dqn_WinError win_error = Dqn_Win_LastError(t_mem.arena);
                result.os_error_code   = win_error.code;
                Dqn_ErrorSink_MakeF(
                    error,
                    result.os_error_code,
                    "Failed to create stderr pipe to redirect the output of the command '%.*s': %.*s",
                    DQN_STR_FMT(cmd_rendered),
                    DQN_STR_FMT(win_error.msg));
                return result;
            }

            if (!SetHandleInformation(stderr_read, HANDLE_FLAG_INHERIT, 0)) {
                Dqn_WinError win_error = Dqn_Win_LastError(t_mem.arena);
                result.os_error_code   = win_error.code;
                Dqn_ErrorSink_MakeF(error,
                                    result.os_error_code,
                                    "Failed to make stderr 'read' pipe non-inheritable when trying to "
                                    "execute command '%.*s': %.*s",
                                    DQN_STR_FMT(cmd_rendered),
                                    DQN_STR_FMT(win_error.msg));
                return result;
            }
        }
    }

    // NOTE: Execute command ///////////////////////////////////////////////////////////////////////
    PROCESS_INFORMATION proc_info = {};
    STARTUPINFOW startup_info     = {};
    startup_info.cb               = sizeof(STARTUPINFOW);
    startup_info.hStdError        = stderr_write ? stderr_write : GetStdHandle(STD_ERROR_HANDLE);
    startup_info.hStdOutput       = stdout_write ? stdout_write : GetStdHandle(STD_OUTPUT_HANDLE);
    startup_info.hStdInput        = GetStdHandle(STD_INPUT_HANDLE);
    startup_info.dwFlags         |= STARTF_USESTDHANDLES;
    BOOL create_result            = CreateProcessW(nullptr, cmd16.data, nullptr, nullptr, true, 0, nullptr, working_dir16.data, &startup_info, &proc_info);
    if (!create_result) {
        Dqn_WinError win_error = Dqn_Win_LastError(t_mem.arena);
        result.os_error_code   = win_error.code;
        Dqn_ErrorSink_MakeF(error,
                            result.os_error_code,
                            "Failed to execute command '%.*s': %.*s",
                            DQN_STR_FMT(cmd_rendered),
                            DQN_STR_FMT(win_error.msg));
        return result;
    }

    // NOTE: Post-amble ////////////////////////////////////////////////////////////////////////////
    CloseHandle(proc_info.hThread);
    result.process      = proc_info.hProcess;
    result.stdout_read  = stdout_read;
    result.stdout_write = stdout_write;
    if (Dqn_Bit_IsSet(exec_flags, Dqn_OSExecFlag_SaveStderr) && Dqn_Bit_IsNotSet(exec_flags, Dqn_OSExecFlag_MergeStderrToStdout)) {
        result.stderr_read  = stderr_read;
        result.stderr_write = stderr_write;
    }
    result.exec_flags = exec_flags;
    return result;
}

#if !defined(DQN_NO_SEMAPHORE)
// NOTE: [$SEMA] Dqn_OSSemaphore ///////////////////////////////////////////////////////////////////
DQN_API Dqn_OSSemaphore Dqn_OS_SemaphoreInit(uint32_t initial_count)
{
    Dqn_OSSemaphore result = {};
    SECURITY_ATTRIBUTES security_attribs = {};
    result.win32_handle                  = CreateSemaphoreA(&security_attribs, initial_count, INT32_MAX, nullptr /*name*/);
    return result;
}

DQN_API bool Dqn_OS_SemaphoreIsValid(Dqn_OSSemaphore *semaphore)
{
    bool result = false;
    if (semaphore) {
        result = semaphore->win32_handle;
    }
    return result;
}

DQN_API void Dqn_OS_SemaphoreDeinit(Dqn_OSSemaphore *semaphore)
{
    if (!Dqn_OS_SemaphoreIsValid(semaphore))
        return;
    CloseHandle(semaphore->win32_handle);
    *semaphore = {};
}

DQN_API void Dqn_OS_SemaphoreIncrement(Dqn_OSSemaphore *semaphore, uint32_t amount)
{
    if (!Dqn_OS_SemaphoreIsValid(semaphore))
        return;
    LONG prev_count = 0;
    ReleaseSemaphore(DQN_CAST(HANDLE *)semaphore->win32_handle, amount, &prev_count);
}

DQN_API Dqn_OSSemaphoreWaitResult Dqn_OS_SemaphoreWait(Dqn_OSSemaphore *semaphore, uint32_t timeout_ms)
{
    Dqn_OSSemaphoreWaitResult result = {};
    if (!Dqn_OS_SemaphoreIsValid(semaphore))
        return result;

    if (!semaphore->win32_handle)
        return result;

    DWORD wait_result = WaitForSingleObject(semaphore->win32_handle, timeout_ms == DQN_OS_SEMAPHORE_INFINITE_TIMEOUT ? INFINITE : timeout_ms);
    if (wait_result == WAIT_TIMEOUT)
        result = Dqn_OSSemaphoreWaitResult_Timeout;
    else if (wait_result == WAIT_OBJECT_0)
        result = Dqn_OSSemaphoreWaitResult_Success;
    return result;
}
#endif // !defined(DQN_NO_SEMAPHORE)

#if !defined(DQN_NO_THREAD)
// NOTE: [$MUTX] Dqn_OSMutex ///////////////////////////////////////////////////////////////////////
DQN_API Dqn_OSMutex Dqn_OS_MutexInit()
{
    Dqn_OSMutex result = {};

    CRITICAL_SECTION crit_section = {};
    InitializeCriticalSection(&crit_section);

    static_assert(sizeof(CRITICAL_SECTION) <= sizeof(result.win32_handle), "Insufficient bytes to store Win32 mutex opaquely in our abstracted Dqn_OSMutex");
    DQN_MEMCPY(result.win32_handle, &crit_section, sizeof(crit_section));
    return result;
}

DQN_API void Dqn_OS_MutexDeinit(Dqn_OSMutex *mutex)
{
    if (!mutex)
        return;
    CRITICAL_SECTION *crit_section = DQN_CAST(CRITICAL_SECTION *)mutex->win32_handle;
    DeleteCriticalSection(crit_section);
    DQN_MEMSET(mutex->win32_handle, 0, DQN_ARRAY_UCOUNT(mutex->win32_handle));
}

DQN_API void Dqn_OS_MutexLock(Dqn_OSMutex *mutex)
{
    if (!mutex)
        return;
    CRITICAL_SECTION *crit_section = DQN_CAST(CRITICAL_SECTION *)mutex->win32_handle;
    EnterCriticalSection(crit_section);
}

DQN_API void Dqn_OS_MutexUnlock(Dqn_OSMutex *mutex)
{
    if (!mutex)
        return;
    CRITICAL_SECTION *crit_section = DQN_CAST(CRITICAL_SECTION *)mutex->win32_handle;
    LeaveCriticalSection(crit_section);
}

// NOTE: [$THRD] Dqn_OSThread /////////////////////////////////////////////////////////////////////
static DWORD __stdcall Dqn_OS_ThreadFunc_(void *user_context)
{
    Dqn_OS_ThreadExecute_(user_context);
    return 0;
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
    DWORD thread_id                      = 0;
    SECURITY_ATTRIBUTES security_attribs = {};
    thread->handle                       = CreateThread(&security_attribs,
                                                        0 /*stack_size*/,
                                                        Dqn_OS_ThreadFunc_,
                                                        thread,
                                                        0 /*creation_flags*/,
                                                        &thread_id);

    result = thread->handle != INVALID_HANDLE_VALUE;
    if (result) {
        thread->thread_id = thread_id;
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

    WaitForSingleObject(thread->handle, INFINITE);
    CloseHandle(thread->handle);
    thread->handle    = INVALID_HANDLE_VALUE;
    thread->thread_id = {};
}

DQN_API uint32_t Dqn_OS_ThreadID()
{
    unsigned long result = GetCurrentThreadId();
    return result;
}
#endif // !defined(DQN_NO_THREAD)

// NOTE: [$HTTP] Dqn_OSHttp ////////////////////////////////////////////////////////////////////////
void Dqn_OS_HttpRequestWin32Callback(HINTERNET session, DWORD *dwContext, DWORD dwInternetStatus, VOID *lpvStatusInformation, DWORD dwStatusInformationLength)
{
    (void)session;
    (void)dwStatusInformationLength;

    Dqn_OSHttpResponse *response = DQN_CAST(Dqn_OSHttpResponse *)dwContext;
    HINTERNET request            = DQN_CAST(HINTERNET)response->win32_request_handle;
    Dqn_WinError error           = {};
    DWORD const READ_BUFFER_SIZE = DQN_MEGABYTES(1);

    if        (dwInternetStatus == WINHTTP_CALLBACK_STATUS_RESOLVING_NAME) {
    } else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_NAME_RESOLVED) {
    } else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER) {
    } else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER) {
    } else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_SENDING_REQUEST) {
    } else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_REQUEST_SENT) {
    } else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE) {
    } else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED) {
    } else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION) {
    } else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED) {
    } else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED) {
    } else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_HANDLE_CREATED) {
    } else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING) {
    } else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_DETECTING_PROXY) {
    } else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_REDIRECT) {
    } else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_INTERMEDIATE_RESPONSE) {
    } else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_SECURE_FAILURE) {
    } else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE) {
        DWORD status      = 0;
        DWORD status_size = sizeof(status_size);
        if (WinHttpQueryHeaders(request,
                                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                WINHTTP_HEADER_NAME_BY_INDEX,
                                &status,
                                &status_size,
                                WINHTTP_NO_HEADER_INDEX)) {
            response->http_status = DQN_CAST(uint16_t)status;

            // NOTE: You can normally call into WinHttpQueryDataAvailable which means the kernel
            // will buffer the response into a single buffer and return us the full size of the
            // request.
            //
            // or
            //
            // You may call WinHttpReadData directly to write the memory into our buffer directly.
            // This is advantageous to avoid a copy from the kernel buffer into our buffer. If the
            // end user application knows the typical payload size then they can optimise for this
            // to prevent unnecessary allocation on the user side.
            void *buffer = Dqn_Arena_Alloc(response->builder.arena, READ_BUFFER_SIZE, 1 /*align*/, Dqn_ZeroMem_No);
            if (!WinHttpReadData(request, buffer, READ_BUFFER_SIZE, nullptr))
                error = Dqn_Win_LastError(&response->tmp_arena);
        } else {
            error = Dqn_Win_LastError(&response->tmp_arena);
        }
    } else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE) {
    } else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_READ_COMPLETE) {
        DWORD bytes_read = dwStatusInformationLength;
        if (bytes_read) {
            Dqn_Str8 prev_buffer = Dqn_Str8_Init(DQN_CAST(char *) lpvStatusInformation, bytes_read);
            Dqn_Str8Builder_AddRef(&response->builder, prev_buffer);

            void *buffer = Dqn_Arena_Alloc(response->builder.arena, READ_BUFFER_SIZE, 1 /*align*/, Dqn_ZeroMem_No);
            if (!WinHttpReadData(request, buffer, READ_BUFFER_SIZE, nullptr))
                error = Dqn_Win_LastError(&response->tmp_arena);
        }
    } else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE) {
    } else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_REQUEST_ERROR) {
        WINHTTP_ASYNC_RESULT *async_result = DQN_CAST(WINHTTP_ASYNC_RESULT *)lpvStatusInformation;
        error                              = Dqn_Win_ErrorCodeToMsg(&response->tmp_arena, DQN_CAST(uint32_t)async_result->dwError);
    } else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE) {
        if (!WinHttpReceiveResponse(request, 0))
            error = Dqn_Win_LastError(&response->tmp_arena);
    }

    // NOTE: If the request handle is missing, then, the response has been freed.
    // MSDN says that this callback can still be called after closing the handle
    // and trigger the WINHTTP_CALLBACK_STATUS_REQUEST_ERROR.
    if (response->win32_request_handle) {
        bool read_complete = dwInternetStatus == WINHTTP_CALLBACK_STATUS_READ_COMPLETE && dwStatusInformationLength == 0;
        if (read_complete) {
            response->body = Dqn_Str8Builder_Build(&response->builder, response->arena);
        }

        if (read_complete || dwInternetStatus == WINHTTP_CALLBACK_STATUS_REQUEST_ERROR || error.code) {
            Dqn_OS_SemaphoreIncrement(&response->on_complete_semaphore, 1);
            Dqn_Atomic_AddU32(&response->done, 1);
        }

        if (error.code) {
            response->error_code = error.code;
            response->error_msg  = error.msg;
        }
    }
}

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

    response->arena         = arena;
    response->builder.arena = response->tmem_arena ? response->tmem_arena : &response->tmp_arena;

    Dqn_Arena  *t_mem_arena = response->tmem_arena;
    Dqn_TLSTMem t_mem_      = Dqn_TLS_TMem(arena);
    if (!t_mem_arena) {
        t_mem_arena = t_mem_.arena;
    }

    Dqn_WinError error = {};
    DQN_DEFER {
        response->error_msg  = error.msg;
        response->error_code = error.code;
        if (error.code) {
            // NOTE: 'Wait' handles failures gracefully, skipping the wait and
            // cleans up the request
            Dqn_OS_HttpRequestWait(response);
            Dqn_Atomic_AddU32(&response->done, 1);
        }
    };

    response->win32_request_session = WinHttpOpen(nullptr /*user agent*/, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC);
    if (!response->win32_request_session) {
        error = Dqn_Win_LastError(&response->tmp_arena);
        return;
    }

    DWORD callback_flags = WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE |
                           WINHTTP_CALLBACK_STATUS_READ_COMPLETE |
                           WINHTTP_CALLBACK_STATUS_REQUEST_ERROR |
                           WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE;
    if (WinHttpSetStatusCallback(response->win32_request_session,
                                 DQN_CAST(WINHTTP_STATUS_CALLBACK)Dqn_OS_HttpRequestWin32Callback,
                                 callback_flags,
                                 DQN_CAST(DWORD_PTR)nullptr /*dwReserved*/) == WINHTTP_INVALID_STATUS_CALLBACK) {
        error = Dqn_Win_LastError(&response->tmp_arena);
        return;
    }

    Dqn_Str16 host16                   = Dqn_Win_Str8ToStr16(t_mem_arena, host);
    response->win32_request_connection = WinHttpConnect(response->win32_request_session, host16.data, secure ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, 0 /*reserved*/);
    if (!response->win32_request_connection) {
        error = Dqn_Win_LastError(&response->tmp_arena);
        return;
    }

    Dqn_Str16 method16 = Dqn_Win_Str8ToStr16(t_mem_arena, method);
    Dqn_Str16 path16   = Dqn_Win_Str8ToStr16(t_mem_arena, path);
    response->win32_request_handle = WinHttpOpenRequest(response->win32_request_connection,
                                                        method16.data,
                                                        path16.data,
                                                        nullptr /*version*/,
                                                        nullptr /*referrer*/,
                                                        nullptr /*accept types*/,
                                                        secure ? WINHTTP_FLAG_SECURE : 0);
    if (!response->win32_request_handle) {
        error = Dqn_Win_LastError(&response->tmp_arena);
        return;
    }

    Dqn_Str16 headers16             = Dqn_Win_Str8ToStr16(t_mem_arena, headers);
    response->on_complete_semaphore = Dqn_OS_SemaphoreInit(0);
    if (!WinHttpSendRequest(response->win32_request_handle,
                            headers16.data,
                            DQN_CAST(DWORD)headers16.size,
                            body.data /*optional data*/,
                            DQN_CAST(DWORD) body.size /*optional length*/,
                            DQN_CAST(DWORD) body.size /*total content length*/,
                            DQN_CAST(DWORD_PTR)response)) {
        error = Dqn_Win_LastError(&response->tmp_arena);
        return;
    }
}

DQN_API void Dqn_OS_HttpRequestFree(Dqn_OSHttpResponse *response)
{
    // NOTE: Cleanup
    // NOTE: These calls are synchronous even when the HTTP request is async.
    WinHttpCloseHandle(response->win32_request_handle);
    WinHttpCloseHandle(response->win32_request_connection);
    WinHttpCloseHandle(response->win32_request_session);

    response->win32_request_session    = nullptr;
    response->win32_request_connection = nullptr;
    response->win32_request_handle     = nullptr;
    Dqn_Arena_Deinit(&response->tmp_arena);
    if (Dqn_OS_SemaphoreIsValid(&response->on_complete_semaphore))
        Dqn_OS_SemaphoreDeinit(&response->on_complete_semaphore);

    *response = {};
}

// NOTE: [$WIND] Dqn_Win ///////////////////////////////////////////////////////////////////////////
DQN_API Dqn_WinError Dqn_Win_ErrorCodeToMsg(Dqn_Arena *arena, uint32_t error_code)
{
    Dqn_WinError result = {};
    result.code         = error_code;
    if (!arena)
        return result;

    DWORD flags                     = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    void *module_to_get_errors_from = nullptr;

    if (result.code >= 12000 && result.code <= 12175) {
        flags |= FORMAT_MESSAGE_FROM_HMODULE;
        module_to_get_errors_from = GetModuleHandleA("winhttp.dll");
    }

    wchar_t *error16 = nullptr;
    DWORD    size    = FormatMessageW(/*DWORD    dwFlags     */ flags | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                      /*LPCVOID  lpSource    */ module_to_get_errors_from,
                                      /*DWORD    dwMessageId */ result.code,
                                      /*DWORD    dwLanguageId*/ 0,
                                      /*LPWSTR   lpBuffer    */ (LPWSTR)&error16,
                                      /*DWORD    nSize       */ 0,
                                      /*va_list *Arguments   */ nullptr);
    if (size)
        result.msg = Dqn_Win_Str16ToStr8(arena, {error16, size});
    if (error16)
        LocalFree(error16);

    return result;
}

DQN_API Dqn_WinError Dqn_Win_LastError(Dqn_Arena *arena)
{
    Dqn_WinError result = Dqn_Win_ErrorCodeToMsg(arena, GetLastError());
    return result;
}

DQN_API void Dqn_Win_MakeProcessDPIAware()
{
    typedef bool SetProcessDpiAwareProc(void);
    typedef bool SetProcessDpiAwarenessProc(DPI_AWARENESS);
    typedef bool SetProcessDpiAwarenessContextProc(void * /*DPI_AWARENESS_CONTEXT*/);

    // NOTE(doyle): Taken from cmuratori/refterm snippet on DPI awareness. It
    // appears we can make this robust by just loading user32.dll and using
    // GetProcAddress on the DPI function. If it's not there, we're on an old
    // version of windows, so we can call an older version of the API.
    void *lib_handle = LoadLibraryA("user32.dll");
    if (!lib_handle)
        return;

    if (auto *set_process_dpi_awareness_context = DQN_CAST(SetProcessDpiAwarenessContextProc *)GetProcAddress(DQN_CAST(HMODULE)lib_handle, "SetProcessDpiAwarenessContext")) {
        set_process_dpi_awareness_context(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    } else if (auto *set_process_dpi_awareness = DQN_CAST(SetProcessDpiAwarenessProc *)GetProcAddress(DQN_CAST(HMODULE)lib_handle, "SetProcessDpiAwareness")) {
        set_process_dpi_awareness(DPI_AWARENESS_PER_MONITOR_AWARE);
    } else if (auto *set_process_dpi_aware = DQN_CAST(SetProcessDpiAwareProc *)GetProcAddress(DQN_CAST(HMODULE)lib_handle, "SetProcessDpiAware")) {
        set_process_dpi_aware();
    }
}

// NOTE: Windows UTF8 to Str16 //////////////////////////////////////////////
DQN_API Dqn_Str16 Dqn_Win_Str8ToStr16(Dqn_Arena *arena, Dqn_Str8 src)
{
    Dqn_Str16 result = {};
    if (!arena || !Dqn_Str8_HasData(src))
        return result;

    int required_size = MultiByteToWideChar(CP_UTF8, 0 /*dwFlags*/, src.data, DQN_CAST(int)src.size, nullptr /*dest*/, 0 /*dest size*/);
    if (required_size <= 0)
        return result;

    wchar_t *buffer = Dqn_Arena_NewArray(arena, wchar_t, required_size + 1, Dqn_ZeroMem_No);
    if (!buffer)
        return result;

    int chars_written = MultiByteToWideChar(CP_UTF8, 0 /*dwFlags*/, src.data, DQN_CAST(int)src.size, buffer, required_size);
    if (DQN_CHECK(chars_written == required_size)) {
        result.data              = buffer;
        result.size              = chars_written;
        result.data[result.size] = 0;
    }
    return result;
}

DQN_API int Dqn_Win_Str8ToStr16Buffer(Dqn_Str8 src, wchar_t *dest, int dest_size)
{
    int result = 0;
    if (!Dqn_Str8_HasData(src))
        return result;

    result = MultiByteToWideChar(CP_UTF8, 0 /*dwFlags*/, src.data, DQN_CAST(int)src.size, nullptr /*dest*/, 0 /*dest size*/);
    if (result <= 0 || result > dest_size || !dest)
        return result;

    result = MultiByteToWideChar(CP_UTF8, 0 /*dwFlags*/, src.data, DQN_CAST(int)src.size, dest, DQN_CAST(int)dest_size);
    dest[DQN_MIN(result, dest_size - 1)] = 0;
    return result;
}

// NOTE: Windows Str16 To UTF8 //////////////////////////////////////////////////////////////////
DQN_API int Dqn_Win_Str16ToStr8Buffer(Dqn_Str16 src, char *dest, int dest_size)
{
    int result = 0;
    if (!Dqn_Str16_HasData(src))
        return result;

    int src_size = Dqn_Safe_SaturateCastISizeToInt(src.size);
    if (src_size <= 0)
        return result;

    result = WideCharToMultiByte(CP_UTF8, 0 /*dwFlags*/, src.data, src_size, nullptr /*dest*/, 0 /*dest size*/, nullptr, nullptr);
    if (result <= 0 || result > dest_size || !dest)
        return result;

    result = WideCharToMultiByte(CP_UTF8, 0 /*dwFlags*/, src.data, src_size, dest, DQN_CAST(int)dest_size, nullptr, nullptr);
    dest[DQN_MIN(result, dest_size - 1)] = 0;
    return result;
}

DQN_API Dqn_Str8 Dqn_Win_Str16ToStr8(Dqn_Arena *arena, Dqn_Str16 src)
{
    Dqn_Str8 result = {};
    if (!arena || !Dqn_Str16_HasData(src))
        return result;

    int src_size = Dqn_Safe_SaturateCastISizeToInt(src.size);
    if (src_size <= 0)
        return result;

    int required_size = WideCharToMultiByte(CP_UTF8, 0 /*dwFlags*/, src.data, src_size, nullptr /*dest*/, 0 /*dest size*/, nullptr, nullptr);
    if (required_size <= 0)
        return result;

    // NOTE: Str8 allocate ensures there's one extra byte for
    // null-termination already so no-need to +1 the required size
    Dqn_ArenaTempMemScope temp_mem = Dqn_ArenaTempMemScope(arena);
    Dqn_Str8              buffer   = Dqn_Str8_Alloc(arena, required_size, Dqn_ZeroMem_No);
    if (!Dqn_Str8_HasData(buffer))
        return result;

    int chars_written = WideCharToMultiByte(CP_UTF8, 0 /*dwFlags*/, src.data, src_size, buffer.data, DQN_CAST(int)buffer.size, nullptr, nullptr);
    if (DQN_CHECK(chars_written == required_size)) {
        result                   = buffer;
        result.data[result.size] = 0;
        temp_mem.mem             = {};
    }

    return result;
}

// NOTE: Windows Executable Directory //////////////////////////////////////////
DQN_API Dqn_Str16 Dqn_Win_EXEPathW(Dqn_Arena *arena)
{
    Dqn_TLSTMem t_mem     = Dqn_TLS_TMem(arena);
    Dqn_Str16   result      = {};
    Dqn_usize   module_size = 0;
    wchar_t    *module_path = nullptr;
    do {
        module_size += 256;
        module_path = Dqn_Arena_NewArray(t_mem.arena, wchar_t, module_size, Dqn_ZeroMem_No);
        if (!module_path)
            return result;
        module_size = DQN_CAST(Dqn_usize)GetModuleFileNameW(nullptr /*module*/, module_path, DQN_CAST(int)module_size);
    } while (GetLastError() == ERROR_INSUFFICIENT_BUFFER);

    Dqn_usize index_of_last_slash = 0;
    for (Dqn_usize index = module_size - 1; !index_of_last_slash && index < module_size; index--)
        index_of_last_slash = module_path[index] == '\\' ? index : 0;

    result.data = Dqn_Arena_NewArray(arena, wchar_t, module_size + 1, Dqn_ZeroMem_No);
    result.size = module_size;
    DQN_MEMCPY(result.data, module_path, sizeof(wchar_t) * result.size);
    result.data[result.size] = 0;
    return result;
}

DQN_API Dqn_Str16 Dqn_Win_EXEDirW(Dqn_Arena *arena)
{
    // TODO(doyle): Implement a Dqn_Str16_BinarySearchReverse
    Dqn_TLSTMem t_mem     = Dqn_TLS_TMem(arena);
    Dqn_Str16   result      = {};
    Dqn_usize   module_size = 0;
    wchar_t    *module_path = nullptr;
    do {
        module_size += 256;
        module_path = Dqn_Arena_NewArray(t_mem.arena, wchar_t, module_size, Dqn_ZeroMem_No);
        if (!module_path)
            return result;
        module_size = DQN_CAST(Dqn_usize)GetModuleFileNameW(nullptr /*module*/, module_path, DQN_CAST(int)module_size);
    } while (GetLastError() == ERROR_INSUFFICIENT_BUFFER);

    Dqn_usize index_of_last_slash = 0;
    for (Dqn_usize index = module_size - 1; !index_of_last_slash && index < module_size; index--)
        index_of_last_slash = module_path[index] == '\\' ? index : 0;

    result.data = Dqn_Arena_NewArray(arena, wchar_t, index_of_last_slash + 1, Dqn_ZeroMem_No);
    result.size = index_of_last_slash;
    DQN_MEMCPY(result.data, module_path, sizeof(wchar_t) * result.size);
    result.data[result.size] = 0;
    return result;
}

DQN_API Dqn_Str8 Dqn_Win_WorkingDir(Dqn_Arena *arena, Dqn_Str8 suffix)
{
    Dqn_TLSTMem t_mem  = Dqn_TLS_TMem(arena);
    Dqn_Str16   suffix16 = Dqn_Win_Str8ToStr16(t_mem.arena, suffix);
    Dqn_Str16   dir16    = Dqn_Win_WorkingDirW(t_mem.arena, suffix16);
    Dqn_Str8    result   = Dqn_Win_Str16ToStr8(arena, dir16);
    return result;
}

DQN_API Dqn_Str16 Dqn_Win_WorkingDirW(Dqn_Arena *arena, Dqn_Str16 suffix)
{
    DQN_ASSERT(suffix.size >= 0);
    Dqn_Str16 result = {};

    // NOTE: required_size is the size required *including* the null-terminator
    Dqn_TLSTMem   t_mem       = Dqn_TLS_TMem(arena);
    unsigned long required_size = GetCurrentDirectoryW(0, nullptr);
    unsigned long desired_size  = required_size + DQN_CAST(unsigned long) suffix.size;

    wchar_t *t_mem_w_path = Dqn_Arena_NewArray(t_mem.arena, wchar_t, desired_size, Dqn_ZeroMem_No);
    if (!t_mem_w_path)
        return result;

    unsigned long bytes_written_wo_null_terminator = GetCurrentDirectoryW(desired_size, t_mem_w_path);
    if ((bytes_written_wo_null_terminator + 1) != required_size) {
        // TODO(dqn): Error
        return result;
    }

    wchar_t *w_path = Dqn_Arena_NewArray(arena, wchar_t, desired_size, Dqn_ZeroMem_No);
    if (!w_path)
        return result;

    if (suffix.size) {
        DQN_MEMCPY(w_path, t_mem_w_path, sizeof(*t_mem_w_path) * bytes_written_wo_null_terminator);
        DQN_MEMCPY(w_path + bytes_written_wo_null_terminator, suffix.data, sizeof(suffix.data[0]) * suffix.size);
        w_path[desired_size] = 0;
    }

    result = Dqn_Str16{w_path, DQN_CAST(Dqn_usize)(desired_size - 1)};
    return result;
}

DQN_API bool Dqn_Win_FolderWIterate(Dqn_Str16 path, Dqn_Win_FolderIteratorW *it)
{
    WIN32_FIND_DATAW find_data = {};
    if (it->handle) {
        if (FindNextFileW(it->handle, &find_data) == 0)
            return false;
    } else {
        it->handle = FindFirstFileExW(path.data,             /*LPCWSTR lpFileName,*/
                                      FindExInfoStandard,    /*FINDEX_INFO_LEVELS fInfoLevelId,*/
                                      &find_data,            /*LPVOID lpFindFileData,*/
                                      FindExSearchNameMatch, /*FINDEX_SEARCH_OPS fSearchOp,*/
                                      nullptr,               /*LPVOID lpSearchFilter,*/
                                      FIND_FIRST_EX_LARGE_FETCH /*unsigned long dwAdditionalFlags)*/);

        if (it->handle == INVALID_HANDLE_VALUE)
            return false;
    }

    it->file_name_buf[0] = 0;
    it->file_name        = Dqn_Str16{it->file_name_buf, 0};

    do {
        if (find_data.cFileName[0] == '.' || (find_data.cFileName[0] == '.' && find_data.cFileName[1] == '.'))
            continue;

        it->file_name.size = Dqn_CStr16_Size(find_data.cFileName);
        DQN_ASSERT(it->file_name.size < (DQN_ARRAY_UCOUNT(it->file_name_buf) - 1));
        DQN_MEMCPY(it->file_name.data, find_data.cFileName, it->file_name.size * sizeof(wchar_t));
        it->file_name_buf[it->file_name.size] = 0;
        break;
    } while (FindNextFileW(it->handle, &find_data) != 0);

    return it->file_name.size > 0;
}

DQN_API bool Dqn_Win_FolderIterate(Dqn_Str8 path, Dqn_Win_FolderIterator *it)
{
    if (!Dqn_Str8_HasData(path) || !it || path.size <= 0)
        return false;

    Dqn_TLSTMem                t_mem    = Dqn_TLS_TMem(nullptr);
    Dqn_Win_FolderIteratorW wide_it = {};
    Dqn_Str16               path16  = {};
    if (it->handle) {
        wide_it.handle = it->handle;
    } else {
        bool needs_asterisks = Dqn_Str8_EndsWith(path, DQN_STR8("\\")) ||
                               Dqn_Str8_EndsWith(path, DQN_STR8("/"));
        bool has_glob        = Dqn_Str8_EndsWith(path, DQN_STR8("\\*")) ||
                               Dqn_Str8_EndsWith(path, DQN_STR8("/*"));

        Dqn_Str8 adjusted_path = path;
        if (!has_glob) {
            // NOTE: We are missing the glob for enumerating the files, we will
            // add those characters in this branch, so overwrite the null
            // character, add the glob and re-null terminate the buffer.
            if (needs_asterisks)
                adjusted_path = Dqn_OS_PathF(t_mem.arena, "%.*s*", DQN_STR_FMT(path));
            else
                adjusted_path = Dqn_OS_PathF(t_mem.arena, "%.*s/*", DQN_STR_FMT(path));
        }

        path16 = Dqn_Win_Str8ToStr16(t_mem.arena, adjusted_path);
        if (path16.size <= 0) // Conversion error
            return false;
    }

    bool result = Dqn_Win_FolderWIterate(path16, &wide_it);
    it->handle  = wide_it.handle;
    if (result) {
        int size      = Dqn_Win_Str16ToStr8Buffer(wide_it.file_name, it->file_name_buf, DQN_ARRAY_UCOUNT(it->file_name_buf));
        it->file_name = Dqn_Str8_Init(it->file_name_buf, size);
    }

    return result;
}
