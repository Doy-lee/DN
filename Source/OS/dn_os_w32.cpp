#define DN_OS_WIN32_CPP

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
//   dn_os_w32.cpp
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

// NOTE: DN_Mem ///////////////////////////////////////////////////////////////////////////
static DN_U32 DN_OS_MemConvertPageToOSFlags_(DN_U32 protect)
{
  DN_Assert((protect & ~DN_MemPage_All) == 0);
  DN_Assert(protect != 0);
  DN_U32 result = 0;

  if (protect & DN_MemPage_NoAccess) {
    result = PAGE_NOACCESS;
  } else if (protect & DN_MemPage_ReadWrite) {
    result = PAGE_READWRITE;
  } else if (protect & DN_MemPage_Read) {
    result = PAGE_READONLY;
  } else if (protect & DN_MemPage_Write) {
    DN_LOG_WarningF("Windows does not support write-only pages, granting read+write access");
    result = PAGE_READWRITE;
  }

  if (protect & DN_MemPage_Guard)
    result |= PAGE_GUARD;

  DN_AssertF(result != PAGE_GUARD, "Page guard is a modifier, you must also specify a page permission like read or/and write");
  return result;
}

DN_API void *DN_OS_MemReserve(DN_USize size, DN_MemCommit commit, DN_U32 page_flags)
{
  unsigned long os_page_flags = DN_OS_MemConvertPageToOSFlags_(page_flags);
  unsigned long flags         = MEM_RESERVE;
  if (commit == DN_MemCommit_Yes)
      flags |= MEM_COMMIT;

  void *result = VirtualAlloc(nullptr, size, flags, os_page_flags);
  if (flags & MEM_COMMIT) {
    DN_Assert(g_dn_os_core_);
    DN_Atomic_AddU64(&g_dn_os_core_->vmem_allocs_total, 1);
    DN_Atomic_AddU64(&g_dn_os_core_->vmem_allocs_frame, 1);
  }
  return result;
}

DN_API bool DN_OS_MemCommit(void *ptr, DN_USize size, DN_U32 page_flags)
{
  bool result = false;
  if (!ptr || size == 0)
    return false;
  unsigned long os_page_flags = DN_OS_MemConvertPageToOSFlags_(page_flags);
  result                      = VirtualAlloc(ptr, size, MEM_COMMIT, os_page_flags) != nullptr;
  DN_Assert(g_dn_os_core_);
  DN_Atomic_AddU64(&g_dn_os_core_->vmem_allocs_total, 1);
  DN_Atomic_AddU64(&g_dn_os_core_->vmem_allocs_frame, 1);
  return result;
}

DN_API void DN_OS_MemDecommit(void *ptr, DN_USize size)
{
  // NOTE: This is a decommit call, which is explicitly saying to free the
  // pages but not the address space, you would use OS_MemRelease to release
  // everything.
  DN_MSVC_WARNING_PUSH
  DN_MSVC_WARNING_DISABLE(6250) // Calling 'VirtualFree' without the MEM_RELEASE flag might free memory but not address descriptors (VADs).  This causes address space leaks.
  VirtualFree(ptr, size, MEM_DECOMMIT);
  DN_MSVC_WARNING_POP
}

DN_API void DN_OS_MemRelease(void *ptr, DN_USize size)
{
  (void)size;
  VirtualFree(ptr, 0, MEM_RELEASE);
}

DN_API int DN_OS_MemProtect(void *ptr, DN_USize size, DN_U32 page_flags)
{
  if (!ptr || size == 0)
    return 0;

  static DN_Str8 const ALIGNMENT_ERROR_MSG =
      DN_STR8("Page protection requires pointers to be page aligned because we can only guard memory at a multiple of the page boundary.");
  DN_AssertF(DN_IsPowerOfTwoAligned(DN_CAST(uintptr_t) ptr, g_dn_os_core_->page_size), "%s", ALIGNMENT_ERROR_MSG.data);
  DN_AssertF(DN_IsPowerOfTwoAligned(size, g_dn_os_core_->page_size), "%s", ALIGNMENT_ERROR_MSG.data);

  unsigned long os_page_flags = DN_OS_MemConvertPageToOSFlags_(page_flags);
  unsigned long prev_flags    = 0;
  int           result        = VirtualProtect(ptr, size, os_page_flags, &prev_flags);

  (void)prev_flags;
  if (result == 0)
    DN_AssertF(result, "VirtualProtect failed");
  return result;
}

DN_API void *DN_OS_MemAlloc(DN_USize size, DN_ZeroMem zero_mem)
{
  DN_U32 flags = zero_mem == DN_ZeroMem_Yes ? HEAP_ZERO_MEMORY : 0;
  DN_Assert(size <= DN_CAST(DWORD)(-1));
  void *result = HeapAlloc(GetProcessHeap(), flags, DN_CAST(DWORD) size);
  DN_Assert(g_dn_os_core_);
  DN_Atomic_AddU64(&g_dn_os_core_->mem_allocs_total, 1);
  DN_Atomic_AddU64(&g_dn_os_core_->mem_allocs_frame, 1);
  return result;
}

DN_API void DN_OS_MemDealloc(void *ptr)
{
  HeapFree(GetProcessHeap(), 0, ptr);
}

// NOTE: Date //////////////////////////////////////////////////////////////////////////////////////
DN_API DN_OSDateTime DN_OS_DateLocalTimeNow()
{
  SYSTEMTIME sys_time;
  GetLocalTime(&sys_time);

  DN_OSDateTime result = {};
  result.hour          = DN_CAST(uint8_t) sys_time.wHour;
  result.minutes       = DN_CAST(uint8_t) sys_time.wMinute;
  result.seconds       = DN_CAST(uint8_t) sys_time.wSecond;
  result.day           = DN_CAST(uint8_t) sys_time.wDay;
  result.month         = DN_CAST(uint8_t) sys_time.wMonth;
  result.year          = DN_CAST(int16_t) sys_time.wYear;
  return result;
}

const DN_U64 DN_OS_WIN32_UNIX_TIME_START            = 0x019DB1DED53E8000; // January 1, 1970 (start of Unix epoch) in "ticks"
const DN_U64 DN_OS_WIN32_FILE_TIME_TICKS_PER_SECOND = 10'000'000;         // Filetime returned is in intervals of 100 nanoseconds

DN_API DN_U64 DN_OS_DateUnixTimeNs()
{
  FILETIME file_time;
  GetSystemTimeAsFileTime(&file_time);

  // NOTE: Filetime returned is in intervals of 100 nanoeseconds so we
  // multiply by 100 to get nanoseconds.
  LARGE_INTEGER date_time;
  date_time.u.LowPart  = file_time.dwLowDateTime;
  date_time.u.HighPart = file_time.dwHighDateTime;
  DN_U64 result      = (date_time.QuadPart - DN_OS_WIN32_UNIX_TIME_START) * 100;
  return result;
}

static SYSTEMTIME DN_OS_DateToSystemTime_(DN_OSDateTime date)
{
  SYSTEMTIME result = {};
  result.wYear      = date.year;
  result.wMonth     = date.month;
  result.wDay       = date.day;
  result.wHour      = date.hour;
  result.wMinute    = date.minutes;
  result.wSecond    = date.seconds;
  return result;
}

static DN_U64 DN_OS_SystemTimeToUnixTimeS_(SYSTEMTIME *sys_time)
{
  FILETIME file_time = {};
  SystemTimeToFileTime(sys_time, &file_time);

  LARGE_INTEGER date_time;
  date_time.u.LowPart  = file_time.dwLowDateTime;
  date_time.u.HighPart = file_time.dwHighDateTime;
  DN_U64 result      = (date_time.QuadPart - DN_OS_WIN32_UNIX_TIME_START) / DN_OS_WIN32_FILE_TIME_TICKS_PER_SECOND;
  return result;
}

DN_API DN_U64 DN_OS_DateLocalToUnixTimeS(DN_OSDateTime date)
{
  SYSTEMTIME local_time = DN_OS_DateToSystemTime_(date);
  SYSTEMTIME sys_time   = {};
  TzSpecificLocalTimeToSystemTime(nullptr, &local_time, &sys_time);
  DN_U64 result = DN_OS_SystemTimeToUnixTimeS_(&sys_time);
  return result;
}

DN_API DN_U64 DN_OS_DateToUnixTimeS(DN_OSDateTime date)
{
  DN_Assert(DN_OS_DateIsValid(date));

  SYSTEMTIME sys_time = DN_OS_DateToSystemTime_(date);
  DN_U64   result   = DN_OS_SystemTimeToUnixTimeS_(&sys_time);
  return result;
}

DN_API DN_OSDateTime DN_OS_DateUnixTimeSToDate(DN_U64 time)
{
  // NOTE: Windows epoch time starts from Jan 1, 1601 and counts in
  // 100-nanoseconds intervals.
  //
  // See: https://devblogs.microsoft.com/oldnewthing/20090306-00/?p=18913

  DN_U64     w32_time      = 116'444'736'000'000'000 + (time * 10'000'000);
  SYSTEMTIME sys_time      = {};
  FILETIME   file_time     = {};
  file_time.dwLowDateTime  = (DWORD)w32_time;
  file_time.dwHighDateTime = w32_time >> 32;
  FileTimeToSystemTime(&file_time, &sys_time);

  DN_OSDateTime result = {};
  result.year          = DN_CAST(uint16_t) sys_time.wYear;
  result.month         = DN_CAST(uint8_t) sys_time.wMonth;
  result.day           = DN_CAST(uint8_t) sys_time.wDay;
  result.hour          = DN_CAST(uint8_t) sys_time.wHour;
  result.minutes       = DN_CAST(uint8_t) sys_time.wMinute;
  result.seconds       = DN_CAST(uint8_t) sys_time.wSecond;
  return result;
}

DN_API bool DN_OS_SecureRNGBytes(void *buffer, DN_U32 size)
{
  DN_Assert(g_dn_os_core_);
  DN_W32Core *w32 = DN_CAST(DN_W32Core *) g_dn_os_core_->platform_context;

  if (!buffer || size < 0 || !w32->bcrypt_init_success)
    return false;

  if (size == 0)
    return true;

  long gen_status = BCryptGenRandom(w32->bcrypt_rng_handle, DN_CAST(unsigned char *) buffer, size, 0 /*flags*/);
  if (gen_status != 0) {
    DN_LOG_ErrorF("Failed to generate random bytes: %d", gen_status);
    return false;
  }

  return true;
}

DN_API DN_OSDiskSpace DN_OS_DiskSpace(DN_Str8 path)
{
  DN_OSTLSTMem   tmem   = DN_OS_TLSPushTMem(nullptr);
  DN_OSDiskSpace result = {};
  DN_Str16       path16 = DN_W32_Str8ToStr16(tmem.arena, path);

  ULARGE_INTEGER free_bytes_avail_to_caller;
  ULARGE_INTEGER total_number_of_bytes;
  ULARGE_INTEGER total_number_of_free_bytes;
  if (!GetDiskFreeSpaceExW(path16.data,
                           &free_bytes_avail_to_caller,
                           &total_number_of_bytes,
                           &total_number_of_free_bytes))
    return result;

  result.success = true;
  result.avail   = free_bytes_avail_to_caller.QuadPart;
  result.size    = total_number_of_bytes.QuadPart;
  return result;
}

DN_API bool DN_OS_SetEnvVar(DN_Str8 name, DN_Str8 value)
{
  DN_OSTLSTMem tmem    = DN_OS_TLSPushTMem(nullptr);
  DN_Str16     name16  = DN_W32_Str8ToStr16(tmem.arena, name);
  DN_Str16     value16 = DN_W32_Str8ToStr16(tmem.arena, value);
  bool         result  = SetEnvironmentVariableW(name16.data, value16.data) != 0;
  return result;
}

DN_API DN_Str8 DN_OS_EXEPath(DN_Arena *arena)
{
  DN_Str8 result = {};
  if (!arena)
    return result;
  DN_OSTLSTMem tmem      = DN_OS_TLSTMem(arena);
  DN_Str16     exe_dir16 = DN_W32_EXEPathW(tmem.arena);
  result                 = DN_W32_Str16ToStr8(arena, exe_dir16);
  return result;
}

DN_API void DN_OS_SleepMs(DN_UInt milliseconds)
{
  Sleep(milliseconds);
}

DN_API DN_U64 DN_OS_PerfCounterFrequency()
{
  DN_Assert(g_dn_os_core_);
  DN_W32Core *w32 = DN_CAST(DN_W32Core *) g_dn_os_core_->platform_context;
  DN_Assert(w32->qpc_frequency.QuadPart);
  DN_U64 result = w32->qpc_frequency.QuadPart;
  return result;
}

DN_API DN_U64 DN_OS_PerfCounterNow()
{
  LARGE_INTEGER integer = {};
  QueryPerformanceCounter(&integer);
  DN_U64 result = integer.QuadPart;
  return result;
}

#if !defined(DN_NO_OS_FILE_API)
static DN_U64 DN_W32_FileTimeToSeconds_(FILETIME const *time)
{
  ULARGE_INTEGER time_large_int = {};
  time_large_int.u.LowPart      = time->dwLowDateTime;
  time_large_int.u.HighPart     = time->dwHighDateTime;
  DN_U64 result                 = (time_large_int.QuadPart / 10000000ULL) - 11644473600ULL;
  return result;
}

DN_API DN_OSPathInfo DN_OS_PathInfo(DN_Str8 path)
{
  DN_OSPathInfo result = {};
  if (!DN_Str8_HasData(path))
    return result;

  DN_OSTLSTMem tmem   = DN_OS_TLSTMem(nullptr);
  DN_Str16     path16 = DN_W32_Str8ToStr16(tmem.arena, path);

  WIN32_FILE_ATTRIBUTE_DATA attrib_data = {};
  if (!GetFileAttributesExW(path16.data, GetFileExInfoStandard, &attrib_data))
    return result;

  result.exists                = true;
  result.create_time_in_s      = DN_W32_FileTimeToSeconds_(&attrib_data.ftCreationTime);
  result.last_access_time_in_s = DN_W32_FileTimeToSeconds_(&attrib_data.ftLastAccessTime);
  result.last_write_time_in_s  = DN_W32_FileTimeToSeconds_(&attrib_data.ftLastWriteTime);

  LARGE_INTEGER large_int = {};
  large_int.u.HighPart    = DN_CAST(int32_t) attrib_data.nFileSizeHigh;
  large_int.u.LowPart     = attrib_data.nFileSizeLow;
  result.size             = (DN_U64)large_int.QuadPart;

  if (attrib_data.dwFileAttributes != INVALID_FILE_ATTRIBUTES) {
    if (attrib_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      result.type = DN_OSPathInfoType_Directory;
    else
      result.type = DN_OSPathInfoType_File;
  }

  return result;
}

DN_API bool DN_OS_PathDelete(DN_Str8 path)
{
  bool result = false;
  if (!DN_Str8_HasData(path))
    return result;

  DN_OSTLSTMem tmem   = DN_OS_TLSTMem(nullptr);
  DN_Str16   path16 = DN_W32_Str8ToStr16(tmem.arena, path);
  if (path16.size) {
    result = DeleteFileW(path16.data);
    if (!result)
      result = RemoveDirectoryW(path16.data);
  }
  return result;
}

DN_API bool DN_OS_FileExists(DN_Str8 path)
{
  bool result = false;
  if (!DN_Str8_HasData(path))
    return result;

  DN_OSTLSTMem tmem   = DN_OS_TLSTMem(nullptr);
  DN_Str16   path16 = DN_W32_Str8ToStr16(tmem.arena, path);
  if (path16.size) {
    WIN32_FILE_ATTRIBUTE_DATA attrib_data = {};
    if (GetFileAttributesExW(path16.data, GetFileExInfoStandard, &attrib_data))
      result = (attrib_data.dwFileAttributes != INVALID_FILE_ATTRIBUTES) &&
               !(attrib_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
  }
  return result;
}

DN_API bool DN_OS_CopyFile(DN_Str8 src, DN_Str8 dest, bool overwrite, DN_OSErrSink *err)
{
  bool       result = false;
  DN_OSTLSTMem tmem   = DN_OS_TLSTMem(nullptr);
  DN_Str16   src16  = DN_W32_Str8ToStr16(tmem.arena, src);
  DN_Str16   dest16 = DN_W32_Str8ToStr16(tmem.arena, dest);

  int fail_if_exists = overwrite == false;
  result             = CopyFileW(src16.data, dest16.data, fail_if_exists) != 0;

  if (!result) {
    DN_W32Error win_error = DN_W32_LastError(tmem.arena);
    DN_OS_ErrSinkAppendF(err,
                       win_error.code,
                       "Failed to copy file '%.*s' to '%.*s': (%u) %.*s",
                       DN_STR_FMT(src),
                       DN_STR_FMT(dest),
                       win_error.code,
                       DN_STR_FMT(win_error.msg));
  }
  return result;
}

DN_API bool DN_OS_MoveFile(DN_Str8 src, DN_Str8 dest, bool overwrite, DN_OSErrSink *err)
{
  bool       result = false;
  DN_OSTLSTMem tmem   = DN_OS_TLSTMem(nullptr);
  DN_Str16   src16  = DN_W32_Str8ToStr16(tmem.arena, src);
  DN_Str16   dest16 = DN_W32_Str8ToStr16(tmem.arena, dest);

  unsigned long flags = MOVEFILE_COPY_ALLOWED;
  if (overwrite)
    flags |= MOVEFILE_REPLACE_EXISTING;

  result = MoveFileExW(src16.data, dest16.data, flags) != 0;
  if (!result) {
    DN_W32Error win_error = DN_W32_LastError(tmem.arena);
    DN_OS_ErrSinkAppendF(err,
                       win_error.code,
                       "Failed to move file '%.*s' to '%.*s': (%u) %.*s",
                       DN_STR_FMT(src),
                       DN_STR_FMT(dest),
                       win_error.code,
                       DN_STR_FMT(win_error.msg));
  }
  return result;
}

DN_API bool DN_OS_MakeDir(DN_Str8 path)
{
  bool       result = true;
  DN_OSTLSTMem tmem   = DN_OS_TLSTMem(nullptr);
  DN_Str16   path16 = DN_W32_Str8ToStr16(tmem.arena, path);

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
  for (DN_USize index = 0; index < path16.size; index++) {
    bool    first_char = index == (path16.size - 1);
    wchar_t ch         = path16.data[index];
    if (ch == '/' || ch == '\\' || first_char) {
      wchar_t temp = path16.data[index];
      if (!first_char)
        path16.data[index] = 0; // Temporarily null terminate it

      WIN32_FILE_ATTRIBUTE_DATA attrib_data = {};
      bool                      successful  = GetFileAttributesExW(path16.data, GetFileExInfoStandard, &attrib_data); // Check

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

DN_API bool DN_OS_DirExists(DN_Str8 path)
{
  bool result = false;
  if (!DN_Str8_HasData(path))
    return result;

  DN_OSTLSTMem tmem   = DN_OS_TLSTMem(nullptr);
  DN_Str16   path16 = DN_W32_Str8ToStr16(tmem.arena, path);
  if (path16.size) {
    WIN32_FILE_ATTRIBUTE_DATA attrib_data = {};
    if (GetFileAttributesExW(path16.data, GetFileExInfoStandard, &attrib_data))
      result = (attrib_data.dwFileAttributes != INVALID_FILE_ATTRIBUTES) &&
               (attrib_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
  }

  return result;
}

DN_API bool DN_OS_DirIterate(DN_Str8 path, DN_OSDirIterator *it)
{
  if (!DN_Str8_HasData(path) || !it || path.size <= 0)
    return false;

  DN_OSTLSTMem             tmem    = DN_OS_TLSTMem(nullptr);
  DN_W32FolderIteratorW wide_it = {};
  DN_Str16               path16  = {};
  if (it->handle) {
    wide_it.handle = it->handle;
  } else {
    bool needs_asterisks = DN_Str8_EndsWith(path, DN_STR8("\\")) ||
                           DN_Str8_EndsWith(path, DN_STR8("/"));
    bool has_glob = DN_Str8_EndsWith(path, DN_STR8("\\*")) ||
                    DN_Str8_EndsWith(path, DN_STR8("/*"));

    DN_Str8 adjusted_path = path;
    if (!has_glob) {
      // NOTE: We are missing the glob for enumerating the files, we will
      // add those characters in this branch, so overwrite the null
      // character, add the glob and re-null terminate the buffer.
      if (needs_asterisks)
        adjusted_path = DN_OS_PathF(tmem.arena, "%.*s*", DN_STR_FMT(path));
      else
        adjusted_path = DN_OS_PathF(tmem.arena, "%.*s/*", DN_STR_FMT(path));
    }

    path16 = DN_W32_Str8ToStr16(tmem.arena, adjusted_path);
    if (path16.size <= 0) // Conversion error
      return false;
  }

  bool result = DN_W32_DirWIterate(path16, &wide_it);
  it->handle  = wide_it.handle;
  if (result) {
    int size      = DN_W32_Str16ToStr8Buffer(wide_it.file_name, it->buffer, DN_ArrayCountU(it->buffer));
    it->file_name = DN_Str8_Init(it->buffer, size);
  }

  return result;
}

// NOTE: R/W Stream API ////////////////////////////////////////////////////////////////////////////
DN_API DN_OSFile DN_OS_FileOpen(DN_Str8 path, DN_OSFileOpen open_mode, DN_U32 access, DN_OSErrSink *err)
{
  DN_OSFile result = {};
  if (!DN_Str8_HasData(path) || path.size <= 0)
    return result;

  if ((access & ~DN_OSFileAccess_All) || ((access & DN_OSFileAccess_All) == 0)) {
    DN_InvalidCodePath;
    return result;
  }

  unsigned long create_flag = 0;
  switch (open_mode) {
    case DN_OSFileOpen_CreateAlways: create_flag = CREATE_ALWAYS; break;
    case DN_OSFileOpen_OpenIfExist:  create_flag = OPEN_EXISTING; break;
    case DN_OSFileOpen_OpenAlways:   create_flag = OPEN_ALWAYS; break;
    default: DN_InvalidCodePath;     return result;
  }

  unsigned long access_mode = 0;
  if (access & DN_OSFileAccess_AppendOnly) {
    DN_AssertF((access & ~DN_OSFileAccess_AppendOnly) == 0,
               "Append can only be applied exclusively to the file, other access modes not permitted");
    access_mode = FILE_APPEND_DATA;
  } else {
    if (access & DN_OSFileAccess_Read)
      access_mode |= GENERIC_READ;
    if (access & DN_OSFileAccess_Write)
      access_mode |= GENERIC_WRITE;
    if (access & DN_OSFileAccess_Execute)
      access_mode |= GENERIC_EXECUTE;
  }

  DN_OSTLSTMem tmem   = DN_OS_TLSTMem(nullptr);
  DN_Str16   path16 = DN_W32_Str8ToStr16(tmem.arena, path);
  void      *handle = CreateFileW(/*LPCWSTR               lpFileName*/ path16.data,
                             /*DWORD                 dwDesiredAccess*/ access_mode,
                             /*DWORD                 dwShareMode*/ FILE_SHARE_READ | FILE_SHARE_WRITE,
                             /*LPSECURITY_ATTRIBUTES lpSecurityAttributes*/ nullptr,
                             /*DWORD                 dwCreationDisposition*/ create_flag,
                             /*DWORD                 dwFlagsAndAttributes*/ FILE_ATTRIBUTE_NORMAL,
                             /*HANDLE                hTemplateFile*/ nullptr);

  if (handle == INVALID_HANDLE_VALUE) {
    DN_W32Error win_error = DN_W32_LastError(tmem.arena);
    result.error          = true;
    DN_OS_ErrSinkAppendF(err, win_error.code, "Failed to open file at '%.*s': '%.*s'", DN_STR_FMT(path), DN_STR_FMT(win_error.msg));
    return result;
  }

  result.handle = handle;
  return result;
}

DN_API DN_OSFileRead DN_OS_FileRead(DN_OSFile *file, void *buffer, DN_USize size, DN_OSErrSink *err)
{
  DN_OSFileRead result = {};
  if (!file || !file->handle || file->error || !buffer || size <= 0)
    return result;

  DN_OSTLSTMem tmem = DN_OS_TLSTMem(nullptr);
  if (!DN_Check(size <= (unsigned long)-1)) {
    DN_Str8 buffer_size_str8 = DN_CVT_U64ToByteSizeStr8(tmem.arena, size, DN_CVTU64ByteSizeType_Auto);
    DN_OS_ErrSinkAppendF(
        err,
        1 /*error_code*/,
        "Current implementation doesn't support reading >4GiB file (requested %.*s), implement Win32 overlapped IO",
        DN_STR_FMT(buffer_size_str8));
    return result;
  }

  unsigned long bytes_read  = 0;
  unsigned long read_result = ReadFile(/*HANDLE       hFile*/ file->handle,
                                       /*LPVOID       lpBuffer*/ buffer,
                                       /*DWORD        nNumberOfBytesToRead*/ DN_CAST(unsigned long) size,
                                       /*LPDWORD      lpNumberOfByesRead*/ &bytes_read,
                                       /*LPOVERLAPPED lpOverlapped*/ nullptr);
  if (read_result == 0) {
    DN_W32Error win_error = DN_W32_LastError(tmem.arena);
    DN_OS_ErrSinkAppendF(err, win_error.code, "Failed to read data from file: (%u) %.*s", win_error.code, DN_STR_FMT(win_error.msg));
    return result;
  }

  if (bytes_read != size) {
    DN_W32Error win_error = DN_W32_LastError(tmem.arena);
    DN_OS_ErrSinkAppendF(
        err,
        win_error.code,
        "Failed to read the desired number of bytes from file, we read %uB but we expected %uB: (%u) %.*s",
        bytes_read,
        DN_CAST(unsigned long) size,
        win_error.code,
        DN_STR_FMT(win_error.msg));
    return result;
  }

  result.bytes_read = bytes_read;
  result.success    = true;
  return result;
}

DN_API bool DN_OS_FileWritePtr(DN_OSFile *file, void const *buffer, DN_USize size, DN_OSErrSink *err)
{
  if (!file || !file->handle || file->error || !buffer || size <= 0)
    return false;

  bool        result = true;
  char const *end    = DN_CAST(char *) buffer + size;
  for (char const *ptr = DN_CAST(char const *) buffer; result && ptr != end;) {
    unsigned long write_size    = DN_CAST(unsigned long) DN_Min((unsigned long)-1, end - ptr);
    unsigned long bytes_written = 0;
    result                      = WriteFile(file->handle, ptr, write_size, &bytes_written, nullptr /*lpOverlapped*/) != 0;
    ptr += bytes_written;
  }

  if (!result) {
    DN_OSTLSTMem  tmem             = DN_OS_TLSTMem(nullptr);
    DN_W32Error win_error        = DN_W32_LastError(tmem.arena);
    DN_Str8     buffer_size_str8 = DN_CVT_U64ToByteSizeStr8(tmem.arena, size, DN_CVTU64ByteSizeType_Auto);
    DN_OS_ErrSinkAppendF(err, win_error.code, "Failed to write buffer (%.*s) to file handle: %.*s", DN_STR_FMT(buffer_size_str8), DN_STR_FMT(win_error.msg));
  }
  return result;
}

DN_API bool DN_OS_FileFlush(DN_OSFile *file, DN_OSErrSink *err)
{
  if (!file || !file->handle || file->error)
    return false;

  BOOL result = FlushFileBuffers(DN_CAST(HANDLE) file->handle);
  if (!result) {
    DN_OSTLSTMem  tmem      = DN_OS_TLSTMem(nullptr);
    DN_W32Error win_error = DN_W32_LastError(tmem.arena);
    DN_OS_ErrSinkAppendF(err, win_error.code, "Failed to flush file buffer to disk: %.*s", DN_STR_FMT(win_error.msg));
  }

  return DN_CAST(bool) result;
}

DN_API void DN_OS_FileClose(DN_OSFile *file)
{
  if (!file || !file->handle || file->error)
    return;
  CloseHandle(file->handle);
  *file = {};
}
#endif // !defined(DN_NO_OS_FILE_API)

// NOTE: DN_OSExec /////////////////////////////////////////////////////////////////////////////////
DN_API void DN_OS_Exit(int32_t exit_code)
{
  ExitProcess(DN_CAST(UINT) exit_code);
}

DN_API DN_OSExecResult DN_OS_ExecPump(DN_OSExecAsyncHandle handle,
                                      char                *stdout_buffer,
                                      size_t              *stdout_size,
                                      char                *stderr_buffer,
                                      size_t              *stderr_size,
                                      DN_U32             timeout_ms,
                                      DN_OSErrSink          *err)
{
  DN_OSExecResult result             = {};
  size_t          stdout_buffer_size = 0;
  size_t          stderr_buffer_size = 0;
  if (stdout_size) {
    stdout_buffer_size = *stdout_size;
    *stdout_size       = 0;
  }

  if (stderr_size) {
    stderr_buffer_size = *stderr_size;
    *stderr_size       = 0;
  }

  if (!handle.process || handle.os_error_code || handle.exit_code) {
    if (handle.os_error_code)
      result.os_error_code = handle.os_error_code;
    else
      result.exit_code = handle.exit_code;

    DN_Assert(!handle.stdout_read);
    DN_Assert(!handle.stdout_write);
    DN_Assert(!handle.stderr_read);
    DN_Assert(!handle.stderr_write);
    DN_Assert(!handle.process);
    return result;
  }

  DN_OSTLSTMem tmem                   = DN_OS_TLSTMem(nullptr);
  DWORD      stdout_bytes_available = 0;
  DWORD      stderr_bytes_available = 0;
  PeekNamedPipe(handle.stdout_read, nullptr, 0, nullptr, &stdout_bytes_available, nullptr);
  PeekNamedPipe(handle.stderr_read, nullptr, 0, nullptr, &stderr_bytes_available, nullptr);

  DWORD exec_result = WAIT_TIMEOUT;
  if (stdout_bytes_available == 0 && stderr_bytes_available == 0)
    exec_result = WaitForSingleObject(handle.process, timeout_ms);

  if (exec_result == WAIT_FAILED) {
    DN_W32Error win_error = DN_W32_LastError(tmem.arena);
    result.os_error_code  = win_error.code;
    DN_OS_ErrSinkAppendF(err, result.os_error_code, "Executed command failed to terminate: %.*s", DN_STR_FMT(win_error.msg));
  } else if (DN_Check(exec_result == WAIT_TIMEOUT || exec_result == WAIT_OBJECT_0)) {
    // NOTE: Read stdout from process //////////////////////////////////////////////////////
    // If the pipes are full, the process will block. We periodically
    // flush the pipes to make sure this doesn't happen
    char sink[DN_Kilobytes(8)];
    stdout_bytes_available = 0;
    if (PeekNamedPipe(handle.stdout_read, nullptr, 0, nullptr, &stdout_bytes_available, nullptr)) {
      if (stdout_bytes_available) {
        DWORD  bytes_read  = 0;
        char  *dest_buffer = handle.stdout_write && stdout_buffer ? stdout_buffer : sink;
        size_t dest_size   = handle.stdout_write && stdout_buffer ? stdout_buffer_size : DN_ArrayCountU(sink);
        BOOL   success     = ReadFile(handle.stdout_read, dest_buffer, DN_CAST(DWORD) dest_size, &bytes_read, NULL);
        (void)success; // TODO:
        if (stdout_size)
          *stdout_size = bytes_read;
      }
    }

    // NOTE: Read stderr from process //////////////////////////////////////////////////////
    stderr_bytes_available = 0;
    if (PeekNamedPipe(handle.stderr_read, nullptr, 0, nullptr, &stderr_bytes_available, nullptr)) {
      if (stderr_bytes_available) {
        char  *dest_buffer = handle.stderr_write && stderr_buffer ? stderr_buffer : sink;
        size_t dest_size   = handle.stderr_write && stderr_buffer ? stderr_buffer_size : DN_ArrayCountU(sink);
        DWORD  bytes_read  = 0;
        BOOL   success     = ReadFile(handle.stderr_read, dest_buffer, DN_CAST(DWORD) dest_size, &bytes_read, NULL);
        (void)success; // TODO:
        if (stderr_size)
          *stderr_size = bytes_read;
      }
    }
  }

  result.finished = exec_result == WAIT_OBJECT_0 || exec_result == WAIT_FAILED;
  if (exec_result == WAIT_OBJECT_0) {
    DWORD exit_status;
    if (GetExitCodeProcess(handle.process, &exit_status)) {
      result.exit_code = exit_status;
    } else {
      DN_W32Error win_error = DN_W32_LastError(tmem.arena);
      result.os_error_code  = win_error.code;
      DN_OS_ErrSinkAppendF(err,
                         result.os_error_code,
                         "Failed to retrieve command exit code: %.*s",
                         DN_STR_FMT(win_error.msg));
    }

    // NOTE: Cleanup ///////////////////////////////////////////////////////////////////////////////
    CloseHandle(handle.stdout_write);
    CloseHandle(handle.stderr_write);
    CloseHandle(handle.stdout_read);
    CloseHandle(handle.stderr_read);
    CloseHandle(handle.process);
  }

  result.stdout_text = DN_Str8_Init(stdout_buffer, stdout_size ? *stdout_size : 0);
  result.stderr_text = DN_Str8_Init(stderr_buffer, stderr_size ? *stderr_size : 0);
  return result;
}

DN_API DN_OSExecResult DN_OS_ExecWait(DN_OSExecAsyncHandle handle, DN_Arena *arena, DN_OSErrSink *err)
{
  DN_OSExecResult result = {};
  if (!handle.process || handle.os_error_code || handle.exit_code) {
    result.finished = true;
    if (handle.os_error_code)
      result.os_error_code = handle.os_error_code;
    else
      result.exit_code = handle.exit_code;

    DN_Assert(!handle.stdout_read);
    DN_Assert(!handle.stdout_write);
    DN_Assert(!handle.stderr_read);
    DN_Assert(!handle.stderr_write);
    DN_Assert(!handle.process);
    return result;
  }

  DN_OSTLSTMem     tmem           = DN_OS_TLSTMem(arena);
  DN_Str8Builder stdout_builder = {};
  DN_Str8Builder stderr_builder = {};
  if (arena) {
    stdout_builder.arena = tmem.arena;
    stderr_builder.arena = tmem.arena;
  }

  DN_U32 const SLOW_WAIT_TIME_MS = 100;
  DN_U32 const FAST_WAIT_TIME_MS = 20;
  DN_U32       wait_ms           = FAST_WAIT_TIME_MS;
  while (!result.finished) {
    size_t stdout_size   = DN_Kilobytes(8);
    size_t stderr_size   = DN_Kilobytes(8);
    char  *stdout_buffer = DN_Arena_NewArray(tmem.arena, char, stdout_size, DN_ZeroMem_No);
    char  *stderr_buffer = DN_Arena_NewArray(tmem.arena, char, stderr_size, DN_ZeroMem_No);
    result               = DN_OS_ExecPump(handle, stdout_buffer, &stdout_size, stderr_buffer, &stderr_size, wait_ms, err);
    DN_Str8Builder_AppendCopy(&stdout_builder, result.stdout_text);
    DN_Str8Builder_AppendCopy(&stderr_builder, result.stderr_text);
    wait_ms = (DN_Str8_HasData(result.stdout_text) || DN_Str8_HasData(result.stderr_text)) ? FAST_WAIT_TIME_MS : SLOW_WAIT_TIME_MS;
  }

  // NOTE: Get stdout/stderr. If no arena is passed this is a no-op //////////////////////////////
  result.stdout_text = DN_Str8Builder_Build(&stdout_builder, arena);
  result.stderr_text = DN_Str8Builder_Build(&stderr_builder, arena);
  return result;
}

DN_API DN_OSExecAsyncHandle DN_OS_ExecAsync(DN_Slice<DN_Str8> cmd_line, DN_OSExecArgs *args, DN_OSErrSink *err)
{
  // NOTE: Pre-amble /////////////////////////////////////////////////////////////////////////////
  DN_OSExecAsyncHandle result = {};
  if (cmd_line.size == 0)
    return result;

  DN_OSTLSTMem tmem          = DN_OS_TLSTMem(nullptr);
  DN_Str8    cmd_rendered  = DN_Slice_Str8Render(tmem.arena, cmd_line, DN_STR8(" "));
  DN_Str16   cmd16         = DN_W32_Str8ToStr16(tmem.arena, cmd_rendered);
  DN_Str16   working_dir16 = DN_W32_Str8ToStr16(tmem.arena, args->working_dir);

  DN_Str8Builder env_builder = DN_Str8Builder_InitFromTLS();
  DN_Str8Builder_AppendArrayRef(&env_builder, args->environment.data, args->environment.size);
  if (env_builder.string_size)
    DN_Str8Builder_AppendRef(&env_builder, DN_STR8("\0"));

  DN_Str8  env_block8  = DN_Str8Builder_BuildDelimitedFromTLS(&env_builder, DN_STR8("\0"));
  DN_Str16 env_block16 = {};
  if (env_block8.size)
    env_block16 = DN_W32_Str8ToStr16(tmem.arena, env_block8);

  // NOTE: Stdout/err security attributes ////////////////////////////////////////////////////////
  SECURITY_ATTRIBUTES save_std_security_attribs = {};
  save_std_security_attribs.nLength             = sizeof(save_std_security_attribs);
  save_std_security_attribs.bInheritHandle      = true;

  // NOTE: Redirect stdout ///////////////////////////////////////////////////////////////////////
  HANDLE stdout_read  = {};
  HANDLE stdout_write = {};
  DN_DEFER
  {
    if (result.os_error_code || result.exit_code) {
      CloseHandle(stdout_read);
      CloseHandle(stdout_write);
    }
  };

  if (DN_Bit_IsSet(args->flags, DN_OSExecFlags_SaveStdout)) {
    if (!CreatePipe(&stdout_read, &stdout_write, &save_std_security_attribs, /*nSize*/ 0)) {
      DN_W32Error win_error = DN_W32_LastError(tmem.arena);
      result.os_error_code  = win_error.code;
      DN_OS_ErrSinkAppendF(
          err,
          result.os_error_code,
          "Failed to create stdout pipe to redirect the output of the command '%.*s': %.*s",
          DN_STR_FMT(cmd_rendered),
          DN_STR_FMT(win_error.msg));
      return result;
    }

    if (!SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0)) {
      DN_W32Error win_error = DN_W32_LastError(tmem.arena);
      result.os_error_code  = win_error.code;
      DN_OS_ErrSinkAppendF(err,
                         result.os_error_code,
                         "Failed to make stdout 'read' pipe non-inheritable when trying to "
                         "execute command '%.*s': %.*s",
                         DN_STR_FMT(cmd_rendered),
                         DN_STR_FMT(win_error.msg));
      return result;
    }
  }

  // NOTE: Redirect stderr ///////////////////////////////////////////////////////////////////////
  HANDLE stderr_read  = {};
  HANDLE stderr_write = {};
  DN_DEFER
  {
    if (result.os_error_code || result.exit_code) {
      CloseHandle(stderr_read);
      CloseHandle(stderr_write);
    }
  };

  if (DN_Bit_IsSet(args->flags, DN_OSExecFlags_SaveStderr)) {
    if (DN_Bit_IsSet(args->flags, DN_OSExecFlags_MergeStderrToStdout)) {
      stderr_read  = stdout_read;
      stderr_write = stdout_write;
    } else {
      if (!CreatePipe(&stderr_read, &stderr_write, &save_std_security_attribs, /*nSize*/ 0)) {
        DN_W32Error win_error = DN_W32_LastError(tmem.arena);
        result.os_error_code  = win_error.code;
        DN_OS_ErrSinkAppendF(
            err,
            result.os_error_code,
            "Failed to create stderr pipe to redirect the output of the command '%.*s': %.*s",
            DN_STR_FMT(cmd_rendered),
            DN_STR_FMT(win_error.msg));
        return result;
      }

      if (!SetHandleInformation(stderr_read, HANDLE_FLAG_INHERIT, 0)) {
        DN_W32Error win_error = DN_W32_LastError(tmem.arena);
        result.os_error_code  = win_error.code;
        DN_OS_ErrSinkAppendF(err,
                           result.os_error_code,
                           "Failed to make stderr 'read' pipe non-inheritable when trying to "
                           "execute command '%.*s': %.*s",
                           DN_STR_FMT(cmd_rendered),
                           DN_STR_FMT(win_error.msg));
        return result;
      }
    }
  }

  // NOTE: Execute command ///////////////////////////////////////////////////////////////////////
  PROCESS_INFORMATION proc_info    = {};
  STARTUPINFOW        startup_info = {};
  startup_info.cb                  = sizeof(STARTUPINFOW);
  startup_info.hStdError           = stderr_write ? stderr_write : GetStdHandle(STD_ERROR_HANDLE);
  startup_info.hStdOutput          = stdout_write ? stdout_write : GetStdHandle(STD_OUTPUT_HANDLE);
  startup_info.hStdInput           = GetStdHandle(STD_INPUT_HANDLE);
  startup_info.dwFlags |= STARTF_USESTDHANDLES;
  BOOL create_result = CreateProcessW(nullptr,
                                      cmd16.data,
                                      nullptr,
                                      nullptr,
                                      true,
                                      CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT,
                                      env_block16.data,
                                      working_dir16.data,
                                      &startup_info,
                                      &proc_info);
  if (!create_result) {
    DN_W32Error win_error = DN_W32_LastError(tmem.arena);
    result.os_error_code  = win_error.code;
    DN_OS_ErrSinkAppendF(err,
                       result.os_error_code,
                       "Failed to execute command '%.*s': %.*s",
                       DN_STR_FMT(cmd_rendered),
                       DN_STR_FMT(win_error.msg));
    return result;
  }

  // NOTE: Post-amble ////////////////////////////////////////////////////////////////////////////
  CloseHandle(proc_info.hThread);
  result.process      = proc_info.hProcess;
  result.stdout_read  = stdout_read;
  result.stdout_write = stdout_write;
  if (DN_Bit_IsSet(args->flags, DN_OSExecFlags_SaveStderr) && DN_Bit_IsNotSet(args->flags, DN_OSExecFlags_MergeStderrToStdout)) {
    result.stderr_read  = stderr_read;
    result.stderr_write = stderr_write;
  }
  result.exec_flags = args->flags;
  return result;
}

static DN_W32Core *DN_OS_GetW32Core_()
{
  DN_Assert(g_dn_os_core_ && g_dn_os_core_->platform_context);
  DN_W32Core *result = DN_CAST(DN_W32Core *)g_dn_os_core_->platform_context;
  return result;
}

static DN_W32SyncPrimitive *DN_OS_U64ToW32SyncPrimitive_(DN_U64 u64)
{
  DN_W32SyncPrimitive *result = nullptr;
  DN_Memcpy(&result, &u64, sizeof(u64));
  return result;
}

static DN_U64 DN_W32_SyncPrimitiveToU64(DN_W32SyncPrimitive *primitive)
{
  DN_U64 result = 0;
  static_assert(sizeof(result) == sizeof(primitive), "Pointer size mis-match");
  DN_Memcpy(&result, &primitive, sizeof(result));
  return result;
}

static DN_W32SyncPrimitive *DN_W32_AllocSyncPrimitive_()
{
  DN_W32Core          *w32    = DN_OS_GetW32Core_();
  DN_W32SyncPrimitive *result = nullptr;
  EnterCriticalSection(&w32->sync_primitive_free_list_mutex);
  {
    if (w32->sync_primitive_free_list) {
      result                        = w32->sync_primitive_free_list;
      w32->sync_primitive_free_list = w32->sync_primitive_free_list->next;
      result->next                  = nullptr;
    } else {
      DN_OSCore *os = g_dn_os_core_;
      result        = DN_Arena_New(&os->arena, DN_W32SyncPrimitive, DN_ZeroMem_Yes);
    }
  }
  LeaveCriticalSection(&w32->sync_primitive_free_list_mutex);
  return result;
}

static void DN_W32_DeallocSyncPrimitive_(DN_W32SyncPrimitive *primitive)
{
  if (primitive) {
    DN_W32Core *w32               = DN_OS_GetW32Core_();
    EnterCriticalSection(&w32->sync_primitive_free_list_mutex);
    primitive->next               = w32->sync_primitive_free_list;
    w32->sync_primitive_free_list = primitive;
    LeaveCriticalSection(&w32->sync_primitive_free_list_mutex);
  }
}

// NOTE: DN_OSSemaphore ////////////////////////////////////////////////////////////////////////////
DN_API DN_OSSemaphore DN_OS_SemaphoreInit(DN_U32 initial_count)
{
  DN_OSSemaphore       result    = {};
  DN_W32SyncPrimitive *primitive = DN_W32_AllocSyncPrimitive_();
  if (primitive) {
    SECURITY_ATTRIBUTES security_attribs = {};
    primitive->sem                       = CreateSemaphoreA(&security_attribs, initial_count, INT32_MAX, nullptr /*name*/);
    if (primitive->sem)
      result.handle = DN_W32_SyncPrimitiveToU64(primitive);
    if (!primitive->sem)
      DN_W32_DeallocSyncPrimitive_(primitive);
  }
  return result;
}

DN_API void DN_OS_SemaphoreDeinit(DN_OSSemaphore *semaphore)
{
  if (semaphore && semaphore->handle != 0) {
    DN_W32SyncPrimitive *primitive = DN_OS_U64ToW32SyncPrimitive_(semaphore->handle);
    CloseHandle(primitive->sem);
    DN_W32_DeallocSyncPrimitive_(primitive);
    *semaphore = {};
  }
}

DN_API void DN_OS_SemaphoreIncrement(DN_OSSemaphore *semaphore, DN_U32 amount)
{
  if (semaphore && semaphore->handle != 0) {
    DN_W32SyncPrimitive *primitive  = DN_OS_U64ToW32SyncPrimitive_(semaphore->handle);
    LONG                 prev_count = 0;
    ReleaseSemaphore(primitive->sem, amount, &prev_count);
  }
}

DN_API DN_OSSemaphoreWaitResult DN_OS_SemaphoreWait(DN_OSSemaphore *semaphore, DN_U32 timeout_ms)
{
  DN_OSSemaphoreWaitResult result = {};
  if (semaphore && semaphore->handle != 0) {
    DN_W32SyncPrimitive *primitive   = DN_OS_U64ToW32SyncPrimitive_(semaphore->handle);
    DWORD                wait_result = WaitForSingleObject(primitive->sem, timeout_ms == DN_OS_SEMAPHORE_INFINITE_TIMEOUT ? INFINITE : timeout_ms);
    if (wait_result == WAIT_TIMEOUT)
      result = DN_OSSemaphoreWaitResult_Timeout;
    else if (wait_result == WAIT_OBJECT_0)
      result = DN_OSSemaphoreWaitResult_Success;
  }
  return result;
}

// NOTE: DN_OSMutex ////////////////////////////////////////////////////////////////////////////////
DN_API DN_OSMutex DN_OS_MutexInit()
{
  DN_W32SyncPrimitive *primitive = DN_W32_AllocSyncPrimitive_();
  if (primitive)
    InitializeCriticalSection(&primitive->mutex);
  DN_OSMutex result = {};
  result.handle     = DN_W32_SyncPrimitiveToU64(primitive);
  return result;
}

DN_API void DN_OS_MutexDeinit(DN_OSMutex *mutex)
{
  if (mutex && mutex->handle != 0) {
    DN_W32SyncPrimitive *primitive = DN_OS_U64ToW32SyncPrimitive_(mutex->handle);
    DeleteCriticalSection(&primitive->mutex);
    DN_W32_DeallocSyncPrimitive_(primitive);
    *mutex = {};
  }
}

DN_API void DN_OS_MutexLock(DN_OSMutex *mutex)
{
  if (mutex && mutex->handle != 0) {
    DN_W32SyncPrimitive *primitive = DN_OS_U64ToW32SyncPrimitive_(mutex->handle);
    EnterCriticalSection(&primitive->mutex);
  }
}

DN_API void DN_OS_MutexUnlock(DN_OSMutex *mutex)
{
  if (mutex && mutex->handle != 0) {
    DN_W32SyncPrimitive *primitive = DN_OS_U64ToW32SyncPrimitive_(mutex->handle);
    LeaveCriticalSection(&primitive->mutex);
  }
}

// NOTE: DN_OSConditionVariable ////////////////////////////////////////////////////////////////////
DN_API DN_OSConditionVariable DN_OS_ConditionVariableInit()
{
  DN_W32SyncPrimitive *primitive = DN_W32_AllocSyncPrimitive_();
  if (primitive)
    InitializeConditionVariable(&primitive->cv);
  DN_OSConditionVariable result = {};
  result.handle                 = DN_W32_SyncPrimitiveToU64(primitive);
  return result;
}

DN_API void DN_OS_ConditionVariableDeinit(DN_OSConditionVariable *cv)
{
  if (cv && cv->handle != 0) {
    DN_W32SyncPrimitive *primitive = DN_OS_U64ToW32SyncPrimitive_(cv->handle);
    DN_W32_DeallocSyncPrimitive_(primitive);
    *cv = {};
  }
}

DN_API bool DN_OS_ConditionVariableWaitUntil(DN_OSConditionVariable *cv, DN_OSMutex *mutex, DN_U64 end_ts_ms)
{
  bool   result = false;
  DN_U64 now_ms = DN_OS_DateUnixTimeNs() / (1000 * 1000);
  if (now_ms < end_ts_ms) {
    DN_U64 sleep_ms = end_ts_ms - now_ms;
    result          = DN_OS_ConditionVariableWait(cv, mutex, sleep_ms);
  }
  return result;
}

DN_API bool DN_OS_ConditionVariableWait(DN_OSConditionVariable *cv, DN_OSMutex *mutex, DN_U64 sleep_ms)
{
  bool result = false;
  if (mutex && cv && mutex->handle != 0 && cv->handle != 0 && sleep_ms > 0) {
    DN_W32SyncPrimitive *mutex_primitive = DN_OS_U64ToW32SyncPrimitive_(mutex->handle);
    DN_W32SyncPrimitive *cv_primitive    = DN_OS_U64ToW32SyncPrimitive_(cv->handle);
    result                               = SleepConditionVariableCS(&cv_primitive->cv, &mutex_primitive->mutex, DN_CAST(DWORD) sleep_ms);
  }
  return result;
}

DN_API void DN_OS_ConditionVariableSignal(DN_OSConditionVariable *cv)
{
  if (cv && cv->handle != 0) {
    DN_W32SyncPrimitive *primitive = DN_OS_U64ToW32SyncPrimitive_(cv->handle);
    WakeConditionVariable(&primitive->cv);
  }
}

DN_API void DN_OS_ConditionVariableBroadcast(DN_OSConditionVariable *cv)
{
  if (cv && cv->handle != 0) {
    DN_W32SyncPrimitive *primitive = DN_OS_U64ToW32SyncPrimitive_(cv->handle);
    WakeAllConditionVariable(&primitive->cv);
  }
}

// NOTE: DN_OSThread ///////////////////////////////////////////////////////////////////////////////
static DWORD __stdcall DN_OS_ThreadFunc_(void *user_context)
{
  DN_OS_ThreadExecute_(user_context);
  return 0;
}

DN_API bool DN_OS_ThreadInit(DN_OSThread *thread, DN_OSThreadFunc *func, void *user_context)
{
  bool result = false;
  if (!thread)
    return result;

  thread->func           = func;
  thread->user_context   = user_context;
  thread->init_semaphore = DN_OS_SemaphoreInit(0 /*initial_count*/);

  // TODO(doyle): Check if semaphore is valid
  DWORD               thread_id        = 0;
  SECURITY_ATTRIBUTES security_attribs = {};
  thread->handle                       = CreateThread(&security_attribs,
                                                      0 /*stack_size*/,
                                                      DN_OS_ThreadFunc_,
                                                      thread,
                                                      0 /*creation_flags*/,
                                                      &thread_id);

  result = thread->handle != INVALID_HANDLE_VALUE;
  if (result)
    thread->thread_id = thread_id;

  // NOTE: Ensure that thread_id is set before 'thread->func' is called.
  if (result) {
    DN_OS_SemaphoreIncrement(&thread->init_semaphore, 1);
  } else {
    DN_OS_SemaphoreDeinit(&thread->init_semaphore);
    *thread = {};
  }

  return result;
}

DN_API void DN_OS_ThreadDeinit(DN_OSThread *thread)
{
  if (!thread || !thread->handle)
    return;

  WaitForSingleObject(thread->handle, INFINITE);
  CloseHandle(thread->handle);
  thread->handle    = INVALID_HANDLE_VALUE;
  thread->thread_id = {};
  DN_OS_TLSDeinit(&thread->tls);
}

DN_API DN_U32 DN_OS_ThreadID()
{
  unsigned long result = GetCurrentThreadId();
  return result;
}

DN_API void DN_W32_ThreadSetName(DN_Str8 name)
{
  DN_OSTLS       *tls  = DN_OS_TLSGet();
  DN_ArenaTempMem tmem = DN_Arena_TempMemBegin(tls->arenas + DN_OSTLSArena_TMem0);

  // NOTE: SetThreadDescription is only available in
  // Windows Server 2016, Windows 10 LTSB 2016 and Windows 10 version 1607
  //
  // See: https://learn.microsoft.com/en-us/windows/w32/api/processthreadsapi/nf-processthreadsapi-setthreaddescription
  DN_W32Core *w32 = DN_OS_GetW32Core_();
  if (w32->set_thread_description) {
    DN_Str16 name16 = DN_W32_Str8ToStr16(tmem.arena, name);
    w32->set_thread_description(GetCurrentThread(), (WCHAR *)name16.data);
    DN_Arena_TempMemEnd(tmem);
    return;
  }

  // NOTE: Fallback to throw-exception method to set thread name
  #pragma pack(push, 8)

  struct DN_W32ThreadNameInfo
  {
    DN_U32 dwType;
    char    *szName;
    DN_U32 dwThreadID;
    DN_U32 dwFlags;
  };

  #pragma pack(pop)

  DN_Str8              copy = DN_Str8_Copy(tmem.arena, name);
  DN_W32ThreadNameInfo info = {};
  info.dwType               = 0x1000;
  info.szName               = (char *)copy.data;
  info.dwThreadID           = DN_OS_ThreadID();

  // TODO: Review warning 6320
  DN_MSVC_WARNING_PUSH
  DN_MSVC_WARNING_DISABLE(6320) // Exception-filter expression is the constant EXCEPTION_EXECUTE_HANDLER. This might mask exceptions that were not intended to be handled
  DN_MSVC_WARNING_DISABLE(6322) // Empty _except block
  __try {
    RaiseException(0x406D1388, 0, sizeof(info) / sizeof(void *), (const ULONG_PTR *)&info);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
  DN_MSVC_WARNING_POP

  DN_Arena_TempMemEnd(tmem);
}

// NOTE: DN_OSHttp /////////////////////////////////////////////////////////////////////////////////
void DN_OS_HttpRequestWin32Callback(HINTERNET session, DWORD *dwContext, DWORD dwInternetStatus, VOID *lpvStatusInformation, DWORD dwStatusInformationLength)
{
  (void)session;
  (void)dwStatusInformationLength;

  DN_OSHttpResponse *response         = DN_CAST(DN_OSHttpResponse *) dwContext;
  HINTERNET          request          = DN_CAST(HINTERNET) response->w32_request_handle;
  DN_W32Error        error            = {};
  DWORD const        READ_BUFFER_SIZE = DN_Megabytes(1);

  if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_RESOLVING_NAME) {
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
      response->http_status = DN_CAST(uint16_t) status;

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
      void *buffer = DN_Arena_Alloc(response->builder.arena, READ_BUFFER_SIZE, 1 /*align*/, DN_ZeroMem_No);
      if (!WinHttpReadData(request, buffer, READ_BUFFER_SIZE, nullptr))
        error = DN_W32_LastError(&response->tmp_arena);
    } else {
      error = DN_W32_LastError(&response->tmp_arena);
    }
  } else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE) {
  } else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_READ_COMPLETE) {
    DWORD bytes_read = dwStatusInformationLength;
    if (bytes_read) {
      DN_Str8 prev_buffer = DN_Str8_Init(DN_CAST(char *) lpvStatusInformation, bytes_read);
      DN_Str8Builder_AppendRef(&response->builder, prev_buffer);

      void *buffer = DN_Arena_Alloc(response->builder.arena, READ_BUFFER_SIZE, 1 /*align*/, DN_ZeroMem_No);
      if (!WinHttpReadData(request, buffer, READ_BUFFER_SIZE, nullptr))
        error = DN_W32_LastError(&response->tmp_arena);
    }
  } else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE) {
  } else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_REQUEST_ERROR) {
    WINHTTP_ASYNC_RESULT *async_result = DN_CAST(WINHTTP_ASYNC_RESULT *) lpvStatusInformation;
    error                              = DN_W32_ErrorCodeToMsg(&response->tmp_arena, DN_CAST(DN_U32) async_result->dwError);
  } else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE) {
    if (!WinHttpReceiveResponse(request, 0))
      error = DN_W32_LastError(&response->tmp_arena);
  }

  // NOTE: If the request handle is missing, then, the response has been freed.
  // MSDN says that this callback can still be called after closing the handle
  // and trigger the WINHTTP_CALLBACK_STATUS_REQUEST_ERROR.
  if (request) {
    bool read_complete = dwInternetStatus == WINHTTP_CALLBACK_STATUS_READ_COMPLETE && dwStatusInformationLength == 0;
    if (read_complete)
      response->body = DN_Str8Builder_Build(&response->builder, response->arena);

    if (read_complete || dwInternetStatus == WINHTTP_CALLBACK_STATUS_REQUEST_ERROR || error.code) {
      DN_OS_SemaphoreIncrement(&response->on_complete_semaphore, 1);
      DN_Atomic_AddU32(&response->done, 1);
    }

    if (error.code) {
      response->error_code = error.code;
      response->error_msg  = error.msg;
    }
  }
}

DN_API void DN_OS_HttpRequestAsync(DN_OSHttpResponse     *response,
                                   DN_Arena              *arena,
                                   DN_Str8                host,
                                   DN_Str8                path,
                                   DN_OSHttpRequestSecure secure,
                                   DN_Str8                method,
                                   DN_Str8                body,
                                   DN_Str8                headers)
{
  if (!response || !arena)
    return;

  response->arena         = arena;
  response->builder.arena = response->tmem_arena ? response->tmem_arena : &response->tmp_arena;

  DN_Arena  *tmem_arena = response->tmem_arena;
  DN_OSTLSTMem tmem_      = DN_OS_TLSTMem(arena);
  if (!tmem_arena)
    tmem_arena = tmem_.arena;

  DN_W32Error error = {};
  DN_DEFER
  {
    response->error_msg  = error.msg;
    response->error_code = error.code;
    if (error.code) {
      // NOTE: 'Wait' handles failures gracefully, skipping the wait and
      // cleans up the request
      DN_OS_HttpRequestWait(response);
      DN_Atomic_AddU32(&response->done, 1);
    }
  };

  response->w32_request_session = WinHttpOpen(nullptr /*user agent*/, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC);
  if (!response->w32_request_session) {
    error = DN_W32_LastError(&response->tmp_arena);
    return;
  }

  DWORD callback_flags = WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE |
                         WINHTTP_CALLBACK_STATUS_READ_COMPLETE |
                         WINHTTP_CALLBACK_STATUS_REQUEST_ERROR |
                         WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE;
  if (WinHttpSetStatusCallback(response->w32_request_session,
                               DN_CAST(WINHTTP_STATUS_CALLBACK) DN_OS_HttpRequestWin32Callback,
                               callback_flags,
                               DN_CAST(DWORD_PTR) nullptr /*dwReserved*/) == WINHTTP_INVALID_STATUS_CALLBACK) {
    error = DN_W32_LastError(&response->tmp_arena);
    return;
  }

  DN_Str16 host16                    = DN_W32_Str8ToStr16(tmem_arena, host);
  response->w32_request_connection = WinHttpConnect(response->w32_request_session, host16.data, secure ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, 0 /*reserved*/);
  if (!response->w32_request_connection) {
    error = DN_W32_LastError(&response->tmp_arena);
    return;
  }

  DN_Str16 method16              = DN_W32_Str8ToStr16(tmem_arena, method);
  DN_Str16 path16                = DN_W32_Str8ToStr16(tmem_arena, path);
  response->w32_request_handle = WinHttpOpenRequest(response->w32_request_connection,
                                                      method16.data,
                                                      path16.data,
                                                      nullptr /*version*/,
                                                      nullptr /*referrer*/,
                                                      nullptr /*accept types*/,
                                                      secure ? WINHTTP_FLAG_SECURE : 0);
  if (!response->w32_request_handle) {
    error = DN_W32_LastError(&response->tmp_arena);
    return;
  }

  DN_Str16 headers16              = DN_W32_Str8ToStr16(tmem_arena, headers);
  response->on_complete_semaphore = DN_OS_SemaphoreInit(0);
  if (!WinHttpSendRequest(response->w32_request_handle,
                          headers16.data,
                          DN_CAST(DWORD) headers16.size,
                          body.data /*optional data*/,
                          DN_CAST(DWORD) body.size /*optional length*/,
                          DN_CAST(DWORD) body.size /*total content length*/,
                          DN_CAST(DWORD_PTR) response)) {
    error = DN_W32_LastError(&response->tmp_arena);
    return;
  }
}

DN_API void DN_OS_HttpRequestFree(DN_OSHttpResponse *response)
{
  // NOTE: Cleanup
  // NOTE: These calls are synchronous even when the HTTP request is async.
  WinHttpCloseHandle(response->w32_request_handle);
  WinHttpCloseHandle(response->w32_request_connection);
  WinHttpCloseHandle(response->w32_request_session);

  response->w32_request_session    = nullptr;
  response->w32_request_connection = nullptr;
  response->w32_request_handle     = nullptr;
  DN_Arena_Deinit(&response->tmp_arena);
  DN_OS_SemaphoreDeinit(&response->on_complete_semaphore);

  *response = {};
}

// NOTE: DN_W32 ////////////////////////////////////////////////////////////////////////////////////
DN_API DN_Str16 DN_W32_ErrorCodeToMsg16Alloc(DN_U32 error_code)
{
  DWORD flags                     = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
  void *module_to_get_errors_from = nullptr;
  if (error_code >= 12000 && error_code <= 12175) {
    flags |= FORMAT_MESSAGE_FROM_HMODULE;
    module_to_get_errors_from = GetModuleHandleA("winhttp.dll");
  }

  wchar_t *result16 = nullptr;
  DWORD    size     = FormatMessageW(/*DWORD    dwFlags     */ flags | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                              /*LPCVOID  lpSource    */ module_to_get_errors_from,
                              /*DWORD    dwMessageId */ error_code,
                              /*DWORD    dwLanguageId*/ 0,
                              /*LPWSTR   lpBuffer    */ (LPWSTR)&result16,
                              /*DWORD    nSize       */ 0,
                              /*va_list *Arguments   */ nullptr);

  DN_Str16 result = {result16, size};
  return result;
}

DN_API DN_W32Error DN_W32_ErrorCodeToMsgAlloc(DN_U32 error_code)
{
  DN_W32Error result = {};
  result.code        = error_code;
  DN_Str16 error16   = DN_W32_ErrorCodeToMsg16Alloc(error_code);
  if (error16.size)
    result.msg = DN_W32_Str16ToStr8FromHeap(error16);
  if (error16.data)
    LocalFree(error16.data);
  return result;
}

DN_API DN_W32Error DN_W32_ErrorCodeToMsg(DN_Arena *arena, DN_U32 error_code)
{
  DN_W32Error result = {};
  result.code        = error_code;
  if (arena) {
    DN_Str16 error16 = DN_W32_ErrorCodeToMsg16Alloc(error_code);
    if (error16.size)
      result.msg = DN_W32_Str16ToStr8(arena, error16);
    if (error16.data)
      LocalFree(error16.data);
  }
  return result;
}

DN_API DN_W32Error DN_W32_LastError(DN_Arena *arena)
{
  DN_W32Error result = DN_W32_ErrorCodeToMsg(arena, GetLastError());
  return result;
}

DN_API DN_W32Error DN_W32_LastErrorAlloc()
{
  DN_W32Error result = DN_W32_ErrorCodeToMsgAlloc(GetLastError());
  return result;
}

DN_API void DN_W32_MakeProcessDPIAware()
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

  if (auto *set_process_dpi_awareness_context = DN_CAST(SetProcessDpiAwarenessContextProc *) GetProcAddress(DN_CAST(HMODULE) lib_handle, "SetProcessDpiAwarenessContext"))
    set_process_dpi_awareness_context(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  else if (auto *set_process_dpi_awareness = DN_CAST(SetProcessDpiAwarenessProc *) GetProcAddress(DN_CAST(HMODULE) lib_handle, "SetProcessDpiAwareness"))
    set_process_dpi_awareness(DPI_AWARENESS_PER_MONITOR_AWARE);
  else if (auto *set_process_dpi_aware = DN_CAST(SetProcessDpiAwareProc *) GetProcAddress(DN_CAST(HMODULE) lib_handle, "SetProcessDpiAware"))
    set_process_dpi_aware();
}

// NOTE: Windows UTF8 to Str16 //////////////////////////////////////////////
DN_API DN_Str16 DN_W32_Str8ToStr16(DN_Arena *arena, DN_Str8 src)
{
  DN_Str16 result = {};
  if (!arena || !DN_Str8_HasData(src))
    return result;

  int required_size = MultiByteToWideChar(CP_UTF8, 0 /*dwFlags*/, src.data, DN_CAST(int) src.size, nullptr /*dest*/, 0 /*dest size*/);
  if (required_size <= 0)
    return result;

  wchar_t *buffer = DN_Arena_NewArray(arena, wchar_t, required_size + 1, DN_ZeroMem_No);
  if (!buffer)
    return result;

  int chars_written = MultiByteToWideChar(CP_UTF8, 0 /*dwFlags*/, src.data, DN_CAST(int) src.size, buffer, required_size);
  if (DN_Check(chars_written == required_size)) {
    result.data              = buffer;
    result.size              = chars_written;
    result.data[result.size] = 0;
  }
  return result;
}

DN_API int DN_W32_Str8ToStr16Buffer(DN_Str8 src, wchar_t *dest, int dest_size)
{
  int result = 0;
  if (!DN_Str8_HasData(src))
    return result;

  result = MultiByteToWideChar(CP_UTF8, 0 /*dwFlags*/, src.data, DN_CAST(int) src.size, nullptr /*dest*/, 0 /*dest size*/);
  if (result <= 0 || result > dest_size || !dest)
    return result;

  result = MultiByteToWideChar(CP_UTF8, 0 /*dwFlags*/, src.data, DN_CAST(int) src.size, dest, DN_CAST(int) dest_size);
  dest[DN_Min(result, dest_size - 1)] = 0;
  return result;
}

// NOTE: Windows Str16 To UTF8 //////////////////////////////////////////////////////////////////
DN_API int DN_W32_Str16ToStr8Buffer(DN_Str16 src, char *dest, int dest_size)
{
  int result = 0;
  if (!DN_Str16_HasData(src))
    return result;

  int src_size = DN_SaturateCastISizeToInt(src.size);
  if (src_size <= 0)
    return result;

  result = WideCharToMultiByte(CP_UTF8, 0 /*dwFlags*/, src.data, src_size, nullptr /*dest*/, 0 /*dest size*/, nullptr, nullptr);
  if (result <= 0 || result > dest_size || !dest)
    return result;

  result = WideCharToMultiByte(CP_UTF8, 0 /*dwFlags*/, src.data, src_size, dest, DN_CAST(int) dest_size, nullptr, nullptr);
  dest[DN_Min(result, dest_size - 1)] = 0;
  return result;
}

DN_API DN_Str8 DN_W32_Str16ToStr8(DN_Arena *arena, DN_Str16 src)
{
  DN_Str8 result = {};
  if (!arena || !DN_Str16_HasData(src))
    return result;

  int src_size = DN_SaturateCastISizeToInt(src.size);
  if (src_size <= 0)
    return result;

  int required_size = WideCharToMultiByte(CP_UTF8, 0 /*dwFlags*/, src.data, src_size, nullptr /*dest*/, 0 /*dest size*/, nullptr, nullptr);
  if (required_size <= 0)
    return result;

  // NOTE: Str8 allocate ensures there's one extra byte for
  // null-termination already so no-need to +1 the required size
  DN_ArenaTempMemScope temp_mem = DN_ArenaTempMemScope(arena);
  DN_Str8              buffer   = DN_Str8_Alloc(arena, required_size, DN_ZeroMem_No);
  if (!DN_Str8_HasData(buffer))
    return result;

  int chars_written = WideCharToMultiByte(CP_UTF8, 0 /*dwFlags*/, src.data, src_size, buffer.data, DN_CAST(int) buffer.size, nullptr, nullptr);
  if (DN_Check(chars_written == required_size)) {
    result                   = buffer;
    result.data[result.size] = 0;
    temp_mem.mem             = {};
  }

  return result;
}

DN_API DN_Str8 DN_W32_Str16ToStr8FromHeap(DN_Str16 src)
{
  DN_Str8 result = {};
  if (!DN_Str16_HasData(src))
    return result;

  int src_size = DN_SaturateCastISizeToInt(src.size);
  if (src_size <= 0)
    return result;

  int required_size = WideCharToMultiByte(CP_UTF8, 0 /*dwFlags*/, src.data, src_size, nullptr /*dest*/, 0 /*dest size*/, nullptr, nullptr);
  if (required_size <= 0)
    return result;

  // NOTE: Str8 allocate ensures there's one extra byte for
  // null-termination already so no-need to +1 the required size
  DN_Str8 buffer = DN_Str8_AllocFromOSHeap(required_size, DN_ZeroMem_No);
  if (!DN_Str8_HasData(buffer))
    return result;

  int chars_written = WideCharToMultiByte(CP_UTF8, 0 /*dwFlags*/, src.data, src_size, buffer.data, DN_CAST(int) buffer.size, nullptr, nullptr);
  if (DN_Check(chars_written == required_size)) {
    result                   = buffer;
    result.data[result.size] = 0;
  } else {
    DN_OS_MemDealloc(buffer.data);
    buffer = {};
  }

  return result;
}

// NOTE: Windows Executable Directory //////////////////////////////////////////
DN_API DN_Str16 DN_W32_EXEPathW(DN_Arena *arena)
{
  DN_OSTLSTMem tmem        = DN_OS_TLSTMem(arena);
  DN_Str16   result      = {};
  DN_USize   module_size = 0;
  wchar_t   *module_path = nullptr;
  do {
    module_size += 256;
    module_path = DN_Arena_NewArray(tmem.arena, wchar_t, module_size, DN_ZeroMem_No);
    if (!module_path)
      return result;
    module_size = DN_CAST(DN_USize) GetModuleFileNameW(nullptr /*module*/, module_path, DN_CAST(int) module_size);
  } while (GetLastError() == ERROR_INSUFFICIENT_BUFFER);

  DN_USize index_of_last_slash = 0;
  for (DN_USize index = module_size - 1; !index_of_last_slash && index < module_size; index--)
    index_of_last_slash = module_path[index] == '\\' ? index : 0;

  result.data = DN_Arena_NewArray(arena, wchar_t, module_size + 1, DN_ZeroMem_No);
  result.size = module_size;
  DN_Memcpy(result.data, module_path, sizeof(wchar_t) * result.size);
  result.data[result.size] = 0;
  return result;
}

DN_API DN_Str16 DN_W32_EXEDirW(DN_Arena *arena)
{
  // TODO(doyle): Implement a DN_Str16_BinarySearchReverse
  DN_OSTLSTMem tmem        = DN_OS_TLSTMem(arena);
  DN_Str16   result      = {};
  DN_USize   module_size = 0;
  wchar_t   *module_path = nullptr;
  do {
    module_size += 256;
    module_path = DN_Arena_NewArray(tmem.arena, wchar_t, module_size, DN_ZeroMem_No);
    if (!module_path)
      return result;
    module_size = DN_CAST(DN_USize) GetModuleFileNameW(nullptr /*module*/, module_path, DN_CAST(int) module_size);
  } while (GetLastError() == ERROR_INSUFFICIENT_BUFFER);

  DN_USize index_of_last_slash = 0;
  for (DN_USize index = module_size - 1; !index_of_last_slash && index < module_size; index--)
    index_of_last_slash = module_path[index] == '\\' ? index : 0;

  result.data = DN_Arena_NewArray(arena, wchar_t, index_of_last_slash + 1, DN_ZeroMem_No);
  result.size = index_of_last_slash;
  DN_Memcpy(result.data, module_path, sizeof(wchar_t) * result.size);
  result.data[result.size] = 0;
  return result;
}

DN_API DN_Str8 DN_W32_WorkingDir(DN_Arena *arena, DN_Str8 suffix)
{
  DN_OSTLSTMem tmem     = DN_OS_TLSTMem(arena);
  DN_Str16   suffix16 = DN_W32_Str8ToStr16(tmem.arena, suffix);
  DN_Str16   dir16    = DN_W32_WorkingDirW(tmem.arena, suffix16);
  DN_Str8    result   = DN_W32_Str16ToStr8(arena, dir16);
  return result;
}

DN_API DN_Str16 DN_W32_WorkingDirW(DN_Arena *arena, DN_Str16 suffix)
{
  DN_Assert(suffix.size >= 0);
  DN_Str16 result = {};

  // NOTE: required_size is the size required *including* the null-terminator
  DN_OSTLSTMem    tmem          = DN_OS_TLSTMem(arena);
  unsigned long required_size = GetCurrentDirectoryW(0, nullptr);
  unsigned long desired_size  = required_size + DN_CAST(unsigned long) suffix.size;

  wchar_t *tmem_w_path = DN_Arena_NewArray(tmem.arena, wchar_t, desired_size, DN_ZeroMem_No);
  if (!tmem_w_path)
    return result;

  unsigned long bytes_written_wo_null_terminator = GetCurrentDirectoryW(desired_size, tmem_w_path);
  if ((bytes_written_wo_null_terminator + 1) != required_size) {
    // TODO(dn): Error
    return result;
  }

  wchar_t *w_path = DN_Arena_NewArray(arena, wchar_t, desired_size, DN_ZeroMem_No);
  if (!w_path)
    return result;

  if (suffix.size) {
    DN_Memcpy(w_path, tmem_w_path, sizeof(*tmem_w_path) * bytes_written_wo_null_terminator);
    DN_Memcpy(w_path + bytes_written_wo_null_terminator, suffix.data, sizeof(suffix.data[0]) * suffix.size);
    w_path[desired_size] = 0;
  }

  result = DN_Str16{w_path, DN_CAST(DN_USize)(desired_size - 1)};
  return result;
}

DN_API bool DN_W32_DirWIterate(DN_Str16 path, DN_W32FolderIteratorW *it)
{
  WIN32_FIND_DATAW find_data = {};
  if (it->handle) {
    if (FindNextFileW(it->handle, &find_data) == 0) {
      FindClose(it->handle);
      return false;
    }
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
  it->file_name        = DN_Str16{it->file_name_buf, 0};

  do {
    if (find_data.cFileName[0] == '.' || (find_data.cFileName[0] == '.' && find_data.cFileName[1] == '.'))
      continue;

    it->file_name.size = DN_CStr16_Size(find_data.cFileName);
    DN_Assert(it->file_name.size < (DN_ArrayCountU(it->file_name_buf) - 1));
    DN_Memcpy(it->file_name.data, find_data.cFileName, it->file_name.size * sizeof(wchar_t));
    it->file_name_buf[it->file_name.size] = 0;
    break;
  } while (FindNextFileW(it->handle, &find_data) != 0);

  bool result = it->file_name.size > 0;
  if (!result)
    FindClose(it->handle);
  return result;
}
