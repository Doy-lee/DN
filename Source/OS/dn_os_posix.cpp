#define DN_OS_POSIX_CPP

#if defined(_CLANGD)
  #define DN_H_WITH_OS 1
  #include "../dn.h"
  #include "dn_os_posix.h"
#endif

#include <dirent.h> // readdir, opendir, closedir
#include <sys/statvfs.h>
#include <sys/mman.h>

// NOTE: DN_OSMem
static DN_U32 DN_OS_MemConvertPageToOSFlags_(DN_U32 protect)
{
  DN_Assert((protect & ~DN_MemPage_All) == 0);
  DN_Assert(protect != 0);
  DN_U32 result = 0;

  if (protect & (DN_MemPage_NoAccess | DN_MemPage_Guard)) {
    result = PROT_NONE;
  } else {
    if (protect & DN_MemPage_Read)
      result = PROT_READ;
    if (protect & DN_MemPage_Write)
      result = PROT_WRITE;
  }
  return result;
}

DN_API void *DN_OS_MemReserve(DN_USize size, DN_MemCommit commit, DN_U32 page_flags)
{
  #if defined(DN_PLATFORM_EMSCRIPTEN)
  DN_AssertInvalidCodePathF("Emscripten does not support virtual memory, you should use DN_OS_MemAlloc");
  #endif

  unsigned long os_page_flags = DN_OS_MemConvertPageToOSFlags_(page_flags);

  if (commit == DN_MemCommit_Yes)
    os_page_flags |= (PROT_READ | PROT_WRITE);

  void *result = mmap(nullptr, size, os_page_flags, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  DN_AtomicAddU64(&g_dn_->os.mem_allocs_total, 1);
  DN_AtomicAddU64(&g_dn_->os.mem_allocs_frame, 1);
  if (result == MAP_FAILED)
    result = nullptr;
  return result;
}

DN_API bool DN_OS_MemCommit(void *ptr, DN_USize size, DN_U32 page_flags)
{
  #if defined(DN_PLATFORM_EMSCRIPTEN)
  DN_AssertInvalidCodePathF("Emscripten does not support virtual memory");
  #endif
  bool result = false;
  if (!ptr || size == 0)
    return false;

  unsigned long os_page_flags = DN_OS_MemConvertPageToOSFlags_(page_flags);
  result                      = mprotect(ptr, size, os_page_flags) == 0;
  DN_AtomicAddU64(&g_dn_->os.mem_allocs_total, 1);
  DN_AtomicAddU64(&g_dn_->os.mem_allocs_frame, 1);
  return result;
}

DN_API void DN_OS_MemDecommit(void *ptr, DN_USize size)
{
  #if defined(DN_PLATFORM_EMSCRIPTEN)
  DN_AssertInvalidCodePathF("Emscripten does not support virtual memory");
  #endif
  mprotect(ptr, size, PROT_NONE);
  madvise(ptr, size, MADV_FREE);
}

DN_API void DN_OS_MemRelease(void *ptr, DN_USize size)
{
  #if defined(DN_PLATFORM_EMSCRIPTEN)
  DN_AssertInvalidCodePathF("Emscripten does not support virtual memory");
  #endif
  munmap(ptr, size);
}

DN_API int DN_OS_MemProtect(void *ptr, DN_USize size, DN_U32 page_flags)
{
  #if defined(DN_PLATFORM_EMSCRIPTEN)
  DN_AssertInvalidCodePathF("Emscripten does not support virtual memory");
  #endif
  if (!ptr || size == 0)
    return 0;

  static DN_Str8 const ALIGNMENT_ERROR_MSG = DN_Str8Lit(
      "Page protection requires pointers to be page aligned because we "
      "can only guard memory at a multiple of the page boundary.");
  DN_AssertF(DN_IsPowerOfTwoAligned(DN_Cast(uintptr_t) ptr, g_dn_->os.page_size),
             "%s",
             ALIGNMENT_ERROR_MSG.data);
  DN_AssertF(
      DN_IsPowerOfTwoAligned(size, g_dn_->os.page_size), "%s", ALIGNMENT_ERROR_MSG.data);

  unsigned long os_page_flags = DN_OS_MemConvertPageToOSFlags_(page_flags);
  int           result        = mprotect(ptr, size, os_page_flags);
  DN_AssertF(result == 0, "mprotect failed (%d)", errno);
  return result;
}

DN_API void *DN_OS_MemAlloc(DN_USize size, DN_ZMem z_mem)
{
  void *result = z_mem == DN_ZMem_Yes ? calloc(1, size) : malloc(size);
  return result;
}

DN_API void DN_OS_MemDealloc(void *ptr)
{
  free(ptr);
}

// NOTE: Date
DN_API DN_Date DN_OS_DateLocalTimeNow()
{
  DN_Date         result = {};
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

  result.hour         = time.tm_hour;
  result.minutes      = time.tm_min;
  result.seconds      = time.tm_sec;
  result.milliseconds = ts.tv_nsec / (1000 * 1000); // TODO: Verify that getting the milliseconds like this is correct

  result.day          = DN_Cast(DN_U8) time.tm_mday;
  result.month        = DN_Cast(DN_U8) time.tm_mon + 1;
  result.year         = 1900 + DN_Cast(DN_U16) time.tm_year;
  return result;
}

DN_API DN_U64 DN_OS_DateUnixTimeNs()
{
  struct timespec ts = {};
  clock_gettime(CLOCK_REALTIME, &ts);
  DN_U64 result = (ts.tv_sec * 1000 /*ms*/ * 1000 /*us*/ * 1000 /*ns*/) + ts.tv_nsec;
  return result;
}

DN_API DN_U64 DN_OS_DateUnixTimeSFromLocalDate(DN_Date date)
{
  struct tm tm_time = {0};
  tm_time.tm_year   = (int)date.year - 1900;
  tm_time.tm_mon    = (int)date.month - 1; // month is 1-12 in your struct
  tm_time.tm_mday   = (int)date.day;       // day of month 1-31
  tm_time.tm_hour   = (int)date.hour;
  tm_time.tm_min    = (int)date.minutes;
  tm_time.tm_sec    = (int)date.seconds;
  tm_time.tm_isdst  = -1; // tm_isdst = -1 lets mktime() determine whether DST is in effect
  time_t unix_time  = mktime(&tm_time);
  DN_U64 result     = DN_Cast(DN_U64) unix_time;
  return result;
}

DN_API DN_U64 DN_OS_DateLocalUnixTimeSFromUnixTimeS(DN_U64 unix_ts_s)
{
  struct tm tm_local;
  time_t    unix_ts = unix_ts_s;
  void     *ret     = localtime_r(&unix_ts, &tm_local);
  DN_Assert(ret);

  long   local_offset_seconds = tm_local.tm_gmtoff;
  DN_U64 result               = unix_ts_s;
  if (local_offset_seconds > 0)
    result += local_offset_seconds;
  else
    result -= local_offset_seconds;
  return result;
}

DN_API DN_Date DN_OS_DateUnixTimeSToDate(DN_U64 time)
{
  time_t        posix_time = DN_Cast(time_t) time;
  struct tm     posix_date = *gmtime(&posix_time);
  DN_Date       result     = {};
  result.year              = posix_date.tm_year + 1900;
  result.month             = posix_date.tm_mon + 1;
  result.day               = posix_date.tm_mday;
  result.hour              = posix_date.tm_hour;
  result.minutes           = posix_date.tm_min;
  result.seconds           = posix_date.tm_sec;
  return result;
}

DN_API void DN_OS_GenBytesSecure(void *buffer, DN_U32 size)
{
#if defined(DN_PLATFORM_EMSCRIPTEN)
  DN_AssertInvalidCodePath;
  (void)buffer;
  (void)size;
#else
  DN_Assert(buffer && size);
  DN_USize bytes_written = 0;
  while (bytes_written < size) {
    DN_USize bytes_remaining = size - bytes_written;
    DN_USize need_amount     = DN_Min(bytes_remaining, 32);
    DN_USize bytes_read      = 0;
    do {
      bytes_read = getrandom((DN_U8 *)buffer + bytes_written, need_amount, 0);
    } while (bytes_read != need_amount || errno == EAGAIN || errno == EINTR);
    bytes_written += bytes_read;
  }
#endif
}

DN_API bool DN_OS_SetEnvVar(DN_Str8 name, DN_Str8 value)
{
  DN_VerifyWarningF(false, "Unimplemented function");
  (void)name;
  (void)value;
  bool result = false;
  return result;
}

DN_API DN_OSDiskSpace DN_OS_DiskSpace(DN_Str8 path)
{
  DN_TCScratch   scratch           = DN_TCScratchBeginArena(nullptr, 0);
  DN_OSDiskSpace result            = {};
  DN_Str8        path_z_terminated = DN_Str8FromStr8Arena(path, &scratch.arena);
  struct statvfs info              = {};
  if (statvfs(path_z_terminated.data, &info) == 0) {
    result.success = true;
    result.avail   = info.f_bavail * info.f_frsize;
    result.size    = info.f_blocks * info.f_frsize;
  }
  DN_TCScratchEnd(&scratch);
  return result;
}

DN_API DN_Str8 DN_OS_EXEPath(DN_Arena *arena)
{
  DN_Str8 result = {};
  if (!arena)
    return result;

  DN_U64 mem_p                            = DN_MemListPos(arena->mem);
  int    required_size_wo_null_terminator = 0;
  for (int try_size = 128;; try_size *= 2) {
    char    *try_buf       = DN_ArenaNewArray(arena, char, try_size, DN_ZMem_No);
    int      bytes_written = readlink("/proc/self/exe", try_buf, try_size);
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

      // TODO(dn): It'd be nice if there's some way of keeping this
      // try_buf around, memcopy the byte and trash the try_buf from the
      // arena. Instead we just get the size and redo the call one last
      // time after this "calculate" step.
      DN_AssertF(bytes_written < try_size,
                 "bytes_written can never be greater than the try size, function writes at "
                 "most try_size");
      required_size_wo_null_terminator = bytes_written;
      break;
    }
  }
  DN_MemListPopTo(arena->mem, mem_p);

  if (required_size_wo_null_terminator) {
    mem_p                                      = DN_MemListPos(arena->mem);
    char *exe_path                             = DN_ArenaNewArray(arena, char, required_size_wo_null_terminator + 1, DN_ZMem_No);
    exe_path[required_size_wo_null_terminator] = 0;

    int bytes_written = readlink("/proc/self/exe", exe_path, required_size_wo_null_terminator);
    if (bytes_written == -1) {
      // Note that if read-link fails again can be because there's
      // a potential race condition here, our exe or directory could have
      // been deleted since the last call, so we need to be careful.
      DN_MemListPopTo(arena->mem, mem_p);
    } else {
      result = DN_Str8FromPtr(exe_path, required_size_wo_null_terminator);
    }
  }
  return result;
}

DN_API void DN_OS_SleepMs(DN_UInt milliseconds)
{
  struct timespec ts;
  ts.tv_sec  = milliseconds / 1000;
  ts.tv_nsec = (milliseconds % 1000) * 1'000'000; // Convert remaining milliseconds to nanoseconds
  // nanosleep can fail if interrupted by a signal, so we loop until the full sleep time has passed
  while (nanosleep(&ts, &ts) == -1 && errno == EINTR)
    ;
}

DN_API DN_U64 DN_OS_PerfCounterFrequency()
{
  // NOTE: On Linux we use clock_gettime(CLOCK_MONOTONIC_RAW) (or CLOCK_MONOTONIC) which
  // increments at nanosecond granularity.
  DN_U64 result = 1'000'000'000;
  return result;
}

static DN_OSPosixCore *DN_OS_PosixGetCore()
{
  DN_Core *dn = DN_Get();
  DN_Assert(dn && dn->os_init);
  DN_OSPosixCore *result = DN_Cast(DN_OSPosixCore *)dn->os.platform_context;
  return result;
}

DN_API DN_U64 DN_OS_PerfCounterNow()
{
  DN_OSPosixCore *posix = DN_OS_PosixGetCore();
  struct timespec ts;
  clock_gettime(posix->clock_monotonic_raw ? CLOCK_MONOTONIC_RAW : CLOCK_MONOTONIC, &ts);
  DN_U64 result = DN_Cast(DN_U64) ts.tv_sec * 1'000'000'000 + DN_Cast(DN_U64) ts.tv_nsec;
  return result;
}

DN_API bool DN_OS_FileCopy(DN_Str8 src, DN_Str8 dest, bool overwrite, DN_ErrSink *error)
{
  bool result = false;
  #if defined(DN_PLATFORM_EMSCRIPTEN)
  DN_ErrSinkAppendF(error, 1, "Unsupported on Emscripten because of their VFS model");
  #else
  int src_fd = open(src.data, O_RDONLY);
  if (src_fd == -1) {
    int error_code = errno;
    DN_ErrSinkAppendF(error,
                      error_code,
                      "Failed to open file '%.*s' for copying: (%d) %s",
                      DN_Str8PrintFmt(src),
                      error_code,
                      strerror(error_code));
    return result;
  }
  DN_DEFER
  {
    close(src_fd);
  };

  // NOTE: File permission is set to read/write by owner, read by others
  int dest_fd = open(dest.data, O_WRONLY | O_CREAT | (overwrite ? O_TRUNC : 0), 0644);
  if (dest_fd == -1) {
    int error_code = errno;
    DN_ErrSinkAppendF(error,
                      error_code,
                      "Failed to open file destination '%.*s' for copying to: (%d) %s",
                      DN_Str8PrintFmt(src),
                      error_code,
                      strerror(error_code));
    return result;
  }
  DN_DEFER
  {
    close(dest_fd);
  };

  struct stat stat_existing;
  int         fstat_result = fstat(src_fd, &stat_existing);
  if (fstat_result == -1) {
    int error_code = errno;
    DN_ErrSinkAppendF(error,
                      error_code,
                      "Failed to query file size of '%.*s' for copying: (%d) %s",
                      DN_Str8PrintFmt(src),
                      error_code,
                      strerror(error_code));
    return result;
  }

  ssize_t bytes_written = sendfile64(dest_fd, src_fd, 0, stat_existing.st_size);
  result                = (bytes_written == stat_existing.st_size);
  if (!result) {
    int        error_code = errno;
    DN_TCScratch scratch               = DN_TCScratchBeginArena(nullptr, 0);
    DN_Str8      file_size_str8     = DN_Str8FromByteCount(scratch.arena, stat_existing.st_size, DN_ByteCountType_Auto);
    DN_Str8      bytes_written_str8 = DN_Str8FromByteCount(scratch.arena, bytes_written, DN_ByteCountType_Auto);
    DN_rrSinkAppendF(error,
                     error_code,
                     "Failed to copy file '%.*s' to '%.*s', we copied %.*s but the file "
                     "size is %.*s: (%d) %s",
                     DN_Str8PrintFmt(src),
                     DN_Str8PrintFmt(dest),
                     DN_Str8PrintFmt(bytes_written_str8),
                     DN_Str8PrintFmt(file_size_str8),
                     error_code,
                     strerror(error_code));
    DN_TCScratchEnd(&scratch);
  }

  #endif
  return result;
}

DN_API bool DN_OS_FileMove(DN_Str8 src, DN_Str8 dest, bool overwrite, DN_ErrSink *error)
{
  // See: https://github.com/gingerBill/gb/blob/master/gb.h
  bool result     = false;
  bool file_moved = true;
  if (link(src.data, dest.data) == -1) {
    // NOTE: Link can fail if we're trying to link across different volumes
    // so we fall back to a binary directory.
    file_moved |= DN_OS_FileCopy(src, dest, overwrite, error);
  }

  if (file_moved) {
    result            = true;
    int unlink_result = unlink(src.data);
    if (unlink_result == -1) {
      int error_code = errno;
      DN_ErrSinkAppendF(
          error,
          error_code,
          "File '%.*s' was moved but failed to be unlinked from old location: (%d) %s",
          DN_Str8PrintFmt(src),
          error_code,
          strerror(error_code));
    }
  }
  return result;
}

DN_API DN_OSFile DN_OS_FileOpen(DN_Str8         path,
                                DN_OSFileOpen   open_mode,
                                DN_OSFileAccess access,
                                DN_ErrSink     *error)
{
  DN_OSFile result = {};
  if (path.count == 0 || path.count <= 0)
    return result;

  if ((access & ~(DN_OSFileAccess_All) || ((access & DN_OSFileAccess_All) == 0))) {
    DN_AssertInvalidCodePath;
    return result;
  }

  if (access & DN_OSFileAccess_Execute) {
    result.error = true;
    DN_ErrSinkAppendF(error, 1, "Failed to open file '%.*s': File access flag 'execute' is not supported", DN_Str8PrintFmt(path));
    DN_AssertInvalidCodePath; // TODO: Not supported via fopen
    return result;
  }

  // NOTE: fopen interface is not as expressive as the Win32
  // We will fopen the file beforehand to setup the state/check for validity
  // before closing and reopening it with the correct request access
  // permissions.
  {
    FILE *handle = nullptr;
    switch (open_mode) {
      case     DN_OSFileOpen_CreateAlways: handle = fopen(path.data, "w"); break;
      case     DN_OSFileOpen_OpenIfExist:  handle = fopen(path.data, "r"); break;
      case     DN_OSFileOpen_OpenAlways:   handle = fopen(path.data, "a"); break;
      default: DN_AssertInvalidCodePath;         break;
    }

    if (!handle) { // TODO(doyle): FileOpen flag to string
      result.error = true;
      DN_ErrSinkAppendF(error,
                        1,
                        "Failed to open file '%.*s': File could not be opened in requested "
                        "mode 'DN_OSFileOpen' flag %d",
                        DN_Str8PrintFmt(path),
                        open_mode);
      return result;
    }
    fclose(handle);
  }

  char const *fopen_mode = nullptr;
  if (access & DN_OSFileAccess_AppendOnly)
    fopen_mode = "a+";
  else if (access & DN_OSFileAccess_Write)
    fopen_mode = "w+";
  else if (access & DN_OSFileAccess_Read)
    fopen_mode = "r";

  FILE *handle = fopen(path.data, fopen_mode);
  if (!handle) {
    result.error = true;
    DN_ErrSinkAppendF(error,
                      1,
                      "Failed to open file '%S': File could not be opened with requested "
                      "access mode 'DN_OSFileAccess' %d",
                      path,
                      fopen_mode);
    return result;
  }
  result.handle = handle;
  return result;
}

DN_API DN_OSFileRead DN_OS_FileRead(DN_OSFile *file, void *buffer, DN_USize size, DN_ErrSink *err)
{
  DN_OSFileRead result = {};
  if (!file || !file->handle || file->error || !buffer || size <= 0)
    return result;

  result.bytes_read = fread(buffer, 1, size, DN_Cast(FILE *) file->handle);
  if (feof(DN_Cast(FILE*)file->handle)) {
    DN_TCScratch scratch          = DN_TCScratchBeginArena(nullptr, 0);
    DN_Str8x32   buffer_size_str8 = DN_Str8x32FromByteCountU64Auto(size);
    DN_ErrSinkAppendF(err, 1, "Failed to read %S from file", buffer_size_str8);
    DN_TCScratchEnd(&scratch);
    return result;
  }

  result.success = true;
  return result;
}

DN_API bool DN_OS_FileWritePtr(DN_OSFile *file, void const *buffer, DN_USize size, DN_ErrSink *err)
{
  if (!file || !file->handle || file->error || !buffer || size <= 0)
    return false;
  bool result =
      fwrite(buffer, DN_Cast(DN_USize) size, 1 /*count*/, DN_Cast(FILE *) file->handle) ==
      1 /*count*/;
  if (!result) {
    DN_TCScratch scratch          = DN_TCScratchBeginArena(nullptr, 0);
    DN_Str8x32   buffer_size_str8 = DN_Str8x32FromByteCountU64Auto(size);
    DN_ErrSinkAppendF(err, 1, "Failed to write buffer (%s) to file handle", DN_Str8PrintFmt(buffer_size_str8));
    DN_TCScratchEnd(&scratch);
  }
  return result;
}

DN_API bool DN_OS_FileFlush(DN_OSFile *file, DN_ErrSink *err)
{
  // TODO: errno is not thread safe
  int fd = fileno(DN_Cast(FILE *) file->handle);
  if (fd == -1) {
    DN_ErrSinkAppendF(err, errno, "Failed to flush file buffer to disk, file handle could not be converted to descriptor (%d): %s", fd, strerror(errno));
    return false;
  }

  int fsync_result = fsync(fd);
  if (fsync_result == -1) {
    DN_ErrSinkAppendF(err, errno, "Failed to flush file buffer to disk (%d): %s", fsync_result, strerror(errno));
    return false;
  }
  return true;
}

DN_API void DN_OS_FileClose(DN_OSFile *file)
{
  if (!file || !file->handle || file->error)
    return;
  fclose(DN_Cast(FILE *) file->handle);
  *file = {};
}

DN_API DN_OSPathInfo DN_OS_PathInfo(DN_Str8 path)
{
  DN_OSPathInfo result = {};
  if (path.count == 0)
    return result;

  struct stat file_stat;
  if (lstat(path.data, &file_stat) != -1) {
    result.exists                = true;
    result.size                  = file_stat.st_size;
    result.last_access_time_in_s = file_stat.st_atime;
    result.last_write_time_in_s  = file_stat.st_mtime;
    // TODO(dn): Seems linux does not support creation time via stat. We
    // shoddily deal with this.
    result.create_time_in_s = DN_Min(result.last_access_time_in_s, result.last_write_time_in_s);

    if (S_ISDIR(file_stat.st_mode))
      result.type = DN_OSPathInfoType_Directory;
    else if (S_ISREG(file_stat.st_mode))
      result.type = DN_OSPathInfoType_File;
  }
  return result;
}

DN_API bool DN_OS_PathDelete(DN_Str8 path)
{
  bool result = false;
  if (path.count)
    result = remove(path.data) == 0;
  return result;
}

DN_API bool DN_OS_PathIsFile(DN_Str8 path)
{
  bool result = false;
  if (path.count == 0)
    return result;

  struct stat stat_result;
  if (lstat(path.data, &stat_result) != -1)
    result = S_ISREG(stat_result.st_mode) || S_ISLNK(stat_result.st_mode);
  return result;
}

DN_API bool DN_OS_PathIsDir(DN_Str8 path)
{
  bool result = false;
  if (path.count == 0)
    return result;

  struct stat stat_result;
  if (lstat(path.data, &stat_result) != -1)
    result = S_ISDIR(stat_result.st_mode);
  return result;
}

DN_API bool DN_OS_PathMakeDir(DN_Str8 path)
{
  DN_TCScratch scratch = DN_TCScratchBeginArena(nullptr, 0);
  bool         result  = true;

  // TODO(doyle): Implement this without using the path indexes, it's not
  // necessary. See Windows implementation.
  DN_USize path_indexes_size = 0;
  uint16_t path_indexes[64]  = {};

  DN_Str8 copy = DN_Str8FromStr8Arena(path, &scratch.arena);
  for (DN_USize index = copy.count - 1; index < copy.count; index--) {
    bool first_char = index == (copy.count - 1);
    char ch         = copy.data[index];
    if (ch == '/' || first_char) {
      char temp = copy.data[index];

      if (!first_char)
        copy.data[index] = 0; // Temporarily null terminate it

      bool is_file = DN_OS_PathIsFile(copy);

      if (!first_char)
        copy.data[index] = temp; // Undo null termination

      if (is_file) {
        // NOTE: There's something that exists in at this path, but
        // it's not a directory. This request to make a directory is
        // invalid.
        DN_TCScratchEnd(&scratch);
        return false;
      } else if (DN_OS_PathIsDir(copy)) {
        // NOTE: We found a directory, we can stop here and start
        // building up all the directories that didn't exist up to
        // this point.
        break;
      } else {
        // NOTE: There's nothing that exists at this path, we can
        // create a directory here
        path_indexes[path_indexes_size++] = DN_Cast(uint16_t) index;
      }
    }
  }

  for (DN_USize index = path_indexes_size - 1; result && index < path_indexes_size; index--) {
    DN_U16 path_index = path_indexes[index];
    char   temp       = copy.data[path_index];

    if (index != 0)
      copy.data[path_index] = 0;
    result |= mkdir(copy.data, 0774) == 0;
    if (index != 0)
      copy.data[path_index] = temp;
  }
  DN_TCScratchEnd(&scratch);
  return result;
}

DN_API bool DN_OS_PathIterateDir(DN_Str8 path, DN_OSDirIterator *it)
{
  if (!it->handle) {
    it->handle = opendir(path.data);
    if (!it->handle)
      return false;
  }

  struct dirent *entry;
  for (;;) {
    entry = readdir(DN_Cast(DIR *) it->handle);
    if (entry == NULL)
      break;

    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    DN_USize name_count    = DN_CStr8Count(entry->d_name);
    DN_USize clamped_count = DN_Min(sizeof(it->buffer) - 1, name_count);
    DN_AssertF(name_count == clamped_count, "name: %s, name_size: %zu, clamped_size: %zu", entry->d_name, name_count, clamped_count);
    DN_Memcpy(it->buffer, entry->d_name, clamped_count);
    it->buffer[clamped_count] = 0;
    it->file_name             = DN_Str8FromPtr(it->buffer, clamped_count);
    return true;
  }

  closedir(DN_Cast(DIR *) it->handle);
  it->handle    = NULL;
  it->file_name = {};
  it->buffer[0] = 0;
  return false;
}

DN_API void DN_OS_Exit(int32_t exit_code)
{
  exit(DN_Cast(int) exit_code);
}

enum DN_OSPipeType_
{
  DN_OSPipeType__Read,
  DN_OSPipeType__Write,
  DN_OSPipeType__Count,
};

DN_API DN_OSExecResult DN_OS_ExecWait(DN_OSExecAsyncHandle handle,
                                      DN_Arena            *arena,
                                      DN_ErrSink          *error)
{
  DN_OSExecResult result = {};
  if (!handle.process || handle.os_error_code || handle.exit_code) {
    if (handle.os_error_code)
      result.os_error_code = handle.os_error_code;
    else
      result.exit_code = handle.exit_code;

    DN_Assert(!handle.stdout_read);
    DN_Assert(!handle.stdout_write);
    DN_Assert(!handle.stderr_read);
    DN_Assert(!handle.stderr_write);
    return result;
  }

#if defined(DN_PLATFORM_EMSCRIPTEN)
  DN_AssertInvalidCodePathF("Unsupported operation");
#endif

  static_assert(sizeof(pid_t) <= sizeof(handle.process),
                "We store the PID opaquely in a register sized pointer");
  pid_t process = {};
  DN_Memcpy(&process, &handle.process, sizeof(process));
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

  int stdout_pipe[DN_OSPipeType__Count] = {};
  int stderr_pipe[DN_OSPipeType__Count] = {};
  DN_Memcpy(&stdout_pipe[DN_OSPipeType__Read],
            &handle.stdout_read,
            sizeof(stdout_pipe[DN_OSPipeType__Read]));
  DN_Memcpy(&stdout_pipe[DN_OSPipeType__Write],
            &handle.stdout_write,
            sizeof(stdout_pipe[DN_OSPipeType__Write]));
  DN_Memcpy(&stderr_pipe[DN_OSPipeType__Read],
            &handle.stderr_read,
            sizeof(stderr_pipe[DN_OSPipeType__Read]));
  DN_Memcpy(&stderr_pipe[DN_OSPipeType__Write],
            &handle.stderr_write,
            sizeof(stderr_pipe[DN_OSPipeType__Write]));

  // NOTE: Process has finished, stop the write end of the pipe
  close(stdout_pipe[DN_OSPipeType__Write]);
  close(stderr_pipe[DN_OSPipeType__Write]);

  // NOTE: Read the data from the read end of the pipe
  if (result.os_error_code == 0) {
    DN_TCScratch scratch = DN_TCScratchBeginArena(&arena, 1);
    if (arena && handle.stdout_read) {
      char           buffer[4096];
      DN_Str8Builder builder = DN_Str8BuilderFromArena(&scratch.arena);
      for (;;) {
        ssize_t bytes_read =
            read(stdout_pipe[DN_OSPipeType__Read], buffer, sizeof(buffer));
        if (bytes_read <= 0)
          break;
        DN_Str8BuilderAppendF(&builder, "%.*s", bytes_read, buffer);
      }

      result.stdout_text = DN_Str8FromStr8BuilderArena(&builder, arena);
    }

    if (arena && handle.stderr_read) {
      char           buffer[4096];
      DN_Str8Builder builder = DN_Str8BuilderFromArena(&scratch.arena);
      for (;;) {
        ssize_t bytes_read =
            read(stderr_pipe[DN_OSPipeType__Read], buffer, sizeof(buffer));
        if (bytes_read <= 0)
          break;
        DN_Str8BuilderAppendF(&builder, "%.*s", bytes_read, buffer);
      }

      result.stderr_text = DN_Str8FromStr8BuilderArena(&builder, arena);
    }
    DN_TCScratchEnd(&scratch);
  }

  close(stdout_pipe[DN_OSPipeType__Read]);
  close(stderr_pipe[DN_OSPipeType__Read]);
  return result;
}

DN_API DN_OSExecAsyncHandle DN_OS_ExecAsync(DN_Str8Slice   cmd_line,
                                            DN_OSExecArgs *args,
                                            DN_ErrSink    *error)
{
#if defined(DN_PLATFORM_EMSCRIPTEN)
  DN_AssertInvalidCodePathF("Unsupported operation");
#endif
  DN_VerifyWarningF(args->environment.count == 0, "Environment variables are unimplemented in POSIX");
  DN_OSExecAsyncHandle result = {};
  if (cmd_line.count == 0)
    return result;

  DN_TCScratch scratch = DN_TCScratchBeginArena(nullptr, 0);
  DN_DEFER { DN_TCScratchEnd(&scratch); };
  DN_Str8    cmd_rendered                      = DN_Str8SliceRender(cmd_line, DN_Str8Lit(" "), &scratch.arena);
  int        stdout_pipe[DN_OSPipeType__Count] = {};
  int        stderr_pipe[DN_OSPipeType__Count] = {};

  // NOTE: Open stdout pipe
  if (DN_BitIsSet(args->flags, DN_OSExecFlags_SaveStdout)) {
    if (pipe(stdout_pipe) == -1) {
      result.os_error_code = errno;
      DN_ErrSinkAppendF(
          error,
          result.os_error_code,
          "Failed to create stdout pipe to redirect the output of the command '%.*s': %s",
          DN_Str8PrintFmt(cmd_rendered),
          strerror(result.os_error_code));
      return result;
    }
    DN_Assert(stdout_pipe[DN_OSPipeType__Read] != 0);
    DN_Assert(stdout_pipe[DN_OSPipeType__Write] != 0);
  }

  DN_DEFER
  {
    if (result.os_error_code == 0 && result.exit_code == 0)
      return;
    close(stdout_pipe[DN_OSPipeType__Read]);
    close(stdout_pipe[DN_OSPipeType__Write]);
  };

  // NOTE: Open stderr pipe //////////////////////////////////////////////////////////////////////
  if (DN_BitIsSet(args->flags, DN_OSExecFlags_SaveStderr)) {
    if (DN_BitIsSet(args->flags, DN_OSExecFlags_MergeStderrToStdout)) {
      stderr_pipe[DN_OSPipeType__Read]  = stdout_pipe[DN_OSPipeType__Read];
      stderr_pipe[DN_OSPipeType__Write] = stdout_pipe[DN_OSPipeType__Write];
    } else if (pipe(stderr_pipe) == -1) {
      result.os_error_code = errno;
      DN_ErrSinkAppendF(
          error,
          result.os_error_code,
          "Failed to create stderr pipe to redirect the output of the command '%.*s': %s",
          DN_Str8PrintFmt(cmd_rendered),
          strerror(result.os_error_code));
      return result;
    }
    DN_Assert(stderr_pipe[DN_OSPipeType__Read] != 0);
    DN_Assert(stderr_pipe[DN_OSPipeType__Write] != 0);
  }

  DN_DEFER
  {
    if (result.os_error_code == 0 && result.exit_code == 0)
      return;
    close(stderr_pipe[DN_OSPipeType__Read]);
    close(stderr_pipe[DN_OSPipeType__Write]);
  };

  pid_t child_pid = fork();
  if (child_pid < 0) {
    result.os_error_code = errno;
    DN_ErrSinkAppendF(
        error,
        result.os_error_code,
        "Failed to fork process to execute the command '%.*s': %s",
        DN_Str8PrintFmt(cmd_rendered),
        strerror(result.os_error_code));
    return result;
  }

  if (child_pid == 0) { // Child process
    if (DN_BitIsSet(args->flags, DN_OSExecFlags_SaveStdout) &&
        (dup2(stdout_pipe[DN_OSPipeType__Write], STDOUT_FILENO) == -1)) {
      result.os_error_code = errno;
      DN_ErrSinkAppendF(
          error,
          result.os_error_code,
          "Failed to redirect stdout 'write' pipe for output of command '%.*s': %s",
          DN_Str8PrintFmt(cmd_rendered),
          strerror(result.os_error_code));
      return result;
    }

    if (DN_BitIsSet(args->flags, DN_OSExecFlags_SaveStderr) &&
        (dup2(stderr_pipe[DN_OSPipeType__Write], STDERR_FILENO) == -1)) {
      result.os_error_code = errno;
      DN_ErrSinkAppendF(
          error,
          result.os_error_code,
          "Failed to redirect stderr 'read' pipe for output of command '%.*s': %s",
          DN_Str8PrintFmt(cmd_rendered),
          strerror(result.os_error_code));
      return result;
    }

    // NOTE: Convert the command into something suitable for execvp
    char **argv =
        DN_ArenaNewArray(&scratch.arena, char *, cmd_line.count + 1 /*null*/, DN_ZMem_Yes);
    if (!argv) {
      result.exit_code = -1;
      DN_ErrSinkAppendF(
          error,
          result.os_error_code,
          "Failed to create argument values from command line '%.*s': Out of memory",
          DN_Str8PrintFmt(cmd_rendered));
      return result;
    }

    for (DN_ForIndexU(arg_index, cmd_line.count)) {
      DN_Str8 arg     = cmd_line.data[arg_index];
      argv[arg_index] = DN_Str8FromStr8Arena(arg, &scratch.arena).data; // NOTE: Copy string to guarantee it is null-terminated
    }

    // NOTE: Change the working directory if there is one
    char *prev_working_dir = nullptr;
    DN_DEFER
    {
      if (!prev_working_dir)
        return;
      if (result.os_error_code == 0) {
        int chdir_result = chdir(prev_working_dir);
        (void)chdir_result;
      }
      free(prev_working_dir);
    };

    if (args->working_dir.count) {
      prev_working_dir    = get_current_dir_name();
      DN_Str8 working_dir = DN_Str8FromStr8Arena(args->working_dir, &scratch.arena);
      if (chdir(working_dir.data) == -1) {
        result.os_error_code = errno;
        DN_ErrSinkAppendF(
            error,
            result.os_error_code,
            "Failed to create argument values from command line '%.*s': %s",
            DN_Str8PrintFmt(cmd_rendered),
            strerror(result.os_error_code));
        return result;
      }
    }

    // NOTE: Execute the command. We reuse argv because the first arg, the
    // binary to execute is guaranteed to be null-terminated.
    if (execvp(argv[0], argv) < 0) {
      result.os_error_code = errno;
      DN_ErrSinkAppendF(
          error,
          result.os_error_code,
          "Failed to execute command'%.*s': %s",
          DN_Str8PrintFmt(cmd_rendered),
          strerror(result.os_error_code));
      return result;
    }
  }

  DN_Assert(result.os_error_code == 0);
  DN_Memcpy(&result.stdout_read,
            &stdout_pipe[DN_OSPipeType__Read],
            sizeof(stdout_pipe[DN_OSPipeType__Read]));
  DN_Memcpy(&result.stdout_write,
            &stdout_pipe[DN_OSPipeType__Write],
            sizeof(stdout_pipe[DN_OSPipeType__Write]));

  if (DN_BitIsSet(args->flags, DN_OSExecFlags_SaveStderr) && DN_BitIsNotSet(args->flags, DN_OSExecFlags_MergeStderrToStdout)) {
    DN_Memcpy(&result.stderr_read,
              &stderr_pipe[DN_OSPipeType__Read],
              sizeof(stderr_pipe[DN_OSPipeType__Read]));
    DN_Memcpy(&result.stderr_write,
              &stderr_pipe[DN_OSPipeType__Write],
              sizeof(stderr_pipe[DN_OSPipeType__Write]));
  }
  result.exec_flags = args->flags;
  DN_Memcpy(&result.process, &child_pid, sizeof(child_pid));
  return result;
}

DN_API DN_OSExecResult DN_OS_ExecPump(DN_OSExecAsyncHandle handle,
                                      char                *stdout_buffer,
                                      size_t              *stdout_size,
                                      char                *stderr_buffer,
                                      size_t              *stderr_size,
                                      DN_U32             timeout_ms,
                                      DN_ErrSink          *err)
{
  DN_AssertInvalidCodePath;
  DN_OSExecResult result = {};
  return result;
}

static DN_OSPosixSyncPrimitive *DN_OS_PosixU64ToSyncPrimitive_(DN_U64 u64)
{
  DN_OSPosixSyncPrimitive *result = nullptr;
  DN_Memcpy(&result, &u64, sizeof(result));
  return result;
}

static DN_U64 DN_OS_PosixSyncPrimitiveToU64(DN_OSPosixSyncPrimitive *primitive)
{
  DN_U64 result = 0;
  static_assert(sizeof(result) >= sizeof(primitive), "Pointer size mis-match");
  DN_Memcpy(&result, &primitive, sizeof(result));
  return result;
}

static DN_OSPosixSyncPrimitive *DN_POSIX_AllocSyncPrimitive_()
{
  DN_OSPosixCore          *posix  = DN_OS_PosixGetCore();
  DN_OSPosixSyncPrimitive *result = nullptr;
  pthread_mutex_lock(&posix->sync_primitive_free_list_mutex);
  {
    if (posix->sync_primitive_free_list) {
      result                          = posix->sync_primitive_free_list;
      posix->sync_primitive_free_list = posix->sync_primitive_free_list->next;
      result->next                    = nullptr;
    } else {
      DN_OSCore *os = &g_dn_->os;
      result        = DN_ArenaNew(&os->arena, DN_OSPosixSyncPrimitive, DN_ZMem_Yes);
    }
  }
  pthread_mutex_unlock(&posix->sync_primitive_free_list_mutex);
  return result;
}

static void DN_OS_PosixDeallocSyncPrimitive_(DN_OSPosixSyncPrimitive *primitive)
{
  if (primitive) {
    DN_OSPosixCore *posix = DN_OS_PosixGetCore();
    pthread_mutex_lock(&posix->sync_primitive_free_list_mutex);
    primitive->next                 = posix->sync_primitive_free_list;
    posix->sync_primitive_free_list = primitive;
    pthread_mutex_unlock(&posix->sync_primitive_free_list_mutex);
  }
}

// NOTE: DN_OSSemaphore
DN_API DN_OSSemaphore DN_OS_SemaphoreInit(DN_U32 initial_count)
{
  DN_OSSemaphore         result    = {};
  DN_OSPosixSyncPrimitive *primitive = DN_POSIX_AllocSyncPrimitive_();
  if (primitive) {
    int pshared = 0; // Share the semaphore across all threads in the process
    if (sem_init(&primitive->sem, pshared, initial_count) == 0)
      result.handle = DN_OS_PosixSyncPrimitiveToU64(primitive);
    else
      DN_OS_PosixDeallocSyncPrimitive_(primitive);
  }
  return result;
}

DN_API void DN_OS_SemaphoreDeinit(DN_OSSemaphore *semaphore)
{
  if (semaphore && semaphore->handle != 0) {
    DN_OSPosixSyncPrimitive *primitive = DN_OS_PosixU64ToSyncPrimitive_(semaphore->handle);
    sem_destroy(&primitive->sem);
    DN_OS_PosixDeallocSyncPrimitive_(primitive);
    *semaphore = {};
  }
}

DN_API void DN_OS_SemaphoreIncrement(DN_OSSemaphore *semaphore, DN_U32 amount)
{
  if (semaphore && semaphore->handle != 0) {
    DN_OSPosixSyncPrimitive *primitive = DN_OS_PosixU64ToSyncPrimitive_(semaphore->handle);
    #if defined(DN_OS_WIN32)
    sem_post_multiple(&primitive->sem, amount); // mingw extension
    #else
    for (DN_ForIndexU(index, amount))
      sem_post(&primitive->sem);
    #endif // !defined(DN_OS_WIN32)
  }
}

DN_API DN_OSSemaphoreWaitResult DN_OS_SemaphoreWait(DN_OSSemaphore *semaphore,
                                                    DN_U32          timeout_ms)
{
  DN_OSSemaphoreWaitResult result = {};
  if (!semaphore || semaphore->handle == 0)
    return result;

  DN_OSPosixSyncPrimitive *primitive = DN_OS_PosixU64ToSyncPrimitive_(semaphore->handle);
  if (timeout_ms == DN_OS_SEMAPHORE_INFINITE_TIMEOUT) {
    int wait_result = 0;
    do {
      wait_result = sem_wait(&primitive->sem);
    } while (wait_result == -1 && errno == EINTR);

    if (wait_result == 0)
      result = DN_OSSemaphoreWaitResult_Success;
  } else {
    DN_U64 now_ms    = DN_OS_DateUnixTimeMs();
    DN_U64 end_ts_ms = now_ms + timeout_ms;

    struct timespec abs_timeout = {};
    abs_timeout.tv_sec          = end_ts_ms / 1'000;
    abs_timeout.tv_nsec         = 1'000'000 * (end_ts_ms - (end_ts_ms / 1'000) * 1'000);
    if (sem_timedwait(&primitive->sem, &abs_timeout) == 0)
      result = DN_OSSemaphoreWaitResult_Success;
    else if (errno == ETIMEDOUT)
      result = DN_OSSemaphoreWaitResult_Timeout;
  }
  return result;
}

DN_API DN_OSBarrier DN_OS_BarrierInit(DN_U32 thread_count)
{
  DN_OSPosixSyncPrimitive *primitive = DN_POSIX_AllocSyncPrimitive_();
  DN_OSBarrier             result    = {};
  if (primitive) {
    int init_result = pthread_barrier_init(&primitive->barrier, /*attr*/ NULL, thread_count);
    if (init_result == 0) {
      result.handle = DN_OS_PosixSyncPrimitiveToU64(primitive);
    } else {
      DN_OS_PosixDeallocSyncPrimitive_(primitive);
    }
  }
  return result;
}

DN_API void DN_OS_BarrierDeinit(DN_OSBarrier *barrier)
{
  if (barrier && barrier->handle != 0) {
    DN_OSPosixSyncPrimitive *primitive = DN_OS_PosixU64ToSyncPrimitive_(barrier->handle);
    int del_result = pthread_barrier_destroy(&primitive->barrier);
    DN_Assert(del_result == 0);
    DN_OS_PosixDeallocSyncPrimitive_(primitive);
  }
}

DN_API void DN_OS_BarrierWait(DN_OSBarrier *barrier)
{
  if (barrier && barrier->handle != 0) {
    DN_OSPosixSyncPrimitive *primitive = DN_OS_PosixU64ToSyncPrimitive_(barrier->handle);
    pthread_barrier_wait(&primitive->barrier);
  }
}

// NOTE: DN_OSMutex
DN_API DN_OSMutex DN_OS_MutexInit()
{
  DN_OSPosixSyncPrimitive *primitive = DN_POSIX_AllocSyncPrimitive_();
  DN_OSMutex               result    = {};
  if (primitive) {
    if (pthread_mutex_init(&primitive->mutex, nullptr) == 0)
      result.handle = DN_OS_PosixSyncPrimitiveToU64(primitive);
    else
      DN_OS_PosixDeallocSyncPrimitive_(primitive);
  }
  return result;
}

DN_API void DN_OS_MutexDeinit(DN_OSMutex *mutex)
{
  if (mutex && mutex->handle != 0) {
    DN_OSPosixSyncPrimitive *primitive = DN_OS_PosixU64ToSyncPrimitive_(mutex->handle);
    pthread_mutex_destroy(&primitive->mutex);
    DN_OS_PosixDeallocSyncPrimitive_(primitive);
    *mutex = {};
  }
}

DN_API void DN_OS_MutexLock(DN_OSMutex *mutex)
{
  if (mutex && mutex->handle != 0) {
    DN_OSPosixSyncPrimitive *primitive = DN_OS_PosixU64ToSyncPrimitive_(mutex->handle);
    pthread_mutex_lock(&primitive->mutex);
  }
}

DN_API void DN_OS_MutexUnlock(DN_OSMutex *mutex)
{
  if (mutex && mutex->handle != 0) {
    DN_OSPosixSyncPrimitive *primitive = DN_OS_PosixU64ToSyncPrimitive_(mutex->handle);
    pthread_mutex_unlock(&primitive->mutex);
  }
}

DN_API DN_OSConditionVariable DN_OS_ConditionVariableInit()
{
  DN_OSPosixSyncPrimitive *primitive = DN_POSIX_AllocSyncPrimitive_();
  DN_OSConditionVariable result    = {};
  if (primitive) {
    if (pthread_cond_init(&primitive->cv, nullptr) == 0)
      result.handle = DN_OS_PosixSyncPrimitiveToU64(primitive);
    else
      DN_OS_PosixDeallocSyncPrimitive_(primitive);
  }
  return result;
}

DN_API void DN_OS_ConditionVariableDeinit(DN_OSConditionVariable *cv)
{
  if (cv && cv->handle != 0) {
    DN_OSPosixSyncPrimitive *primitive = DN_OS_PosixU64ToSyncPrimitive_(cv->handle);
    pthread_cond_destroy(&primitive->cv);
    DN_OS_PosixDeallocSyncPrimitive_(primitive);
    *cv = {};
  }
}

DN_API bool DN_OS_ConditionVariableWaitUntil(DN_OSConditionVariable *cv, DN_OSMutex *mutex, DN_U64 end_ts_ms)
{
  bool result = false;
  if (cv && mutex && mutex->handle != 0 && cv->handle != 0) {
    DN_OSPosixSyncPrimitive *cv_primitive    = DN_OS_PosixU64ToSyncPrimitive_(cv->handle);
    DN_OSPosixSyncPrimitive *mutex_primitive = DN_OS_PosixU64ToSyncPrimitive_(mutex->handle);

    struct timespec time = {};
    time.tv_sec          = end_ts_ms / 1'000;
    time.tv_nsec         = 1'000'000 * (end_ts_ms - (end_ts_ms / 1'000) * 1'000);
    int wait_result      = pthread_cond_timedwait(&cv_primitive->cv, &mutex_primitive->mutex, &time);
    result               = (wait_result != ETIMEDOUT);
  }
  return result;
}

DN_API bool DN_OS_ConditionVariableWait(DN_OSConditionVariable *cv, DN_OSMutex *mutex, DN_U64 sleep_ms)
{
  DN_U64 end_ts_ms = DN_OS_DateUnixTimeMs() + sleep_ms;
  bool   result    = DN_OS_ConditionVariableWaitUntil(cv, mutex, end_ts_ms);
  return result;
}

DN_API void DN_OS_ConditionVariableSignal(DN_OSConditionVariable *cv)
{
  if (cv && cv->handle != 0) {
    DN_OSPosixSyncPrimitive *primitive = DN_OS_PosixU64ToSyncPrimitive_(cv->handle);
    pthread_cond_signal(&primitive->cv);
  }
}

DN_API void DN_OS_ConditionVariableBroadcast(DN_OSConditionVariable *cv)
{
  if (cv && cv->handle != 0) {
    DN_OSPosixSyncPrimitive *primitive = DN_OS_PosixU64ToSyncPrimitive_(cv->handle);
    pthread_cond_broadcast(&primitive->cv);
  }
}

// NOTE: DN_OSThread
static void *DN_OS_ThreadFunc_(void *user_context)
{
  DN_OS_ThreadExecute_(user_context);
  return nullptr;
}

DN_API bool DN_OS_ThreadInitLane(DN_OSThread *thread, DN_OSThreadFunc *func, DN_OSThreadLane *lane, DN_OSThreadInitArgs init_args, void *user_context)
{
  bool result = false;
  if (!thread)
    return result;

  DN_OS_ThreadPreInit_(thread, func, lane, init_args, user_context);

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
  if (init_args.stack_size)
    pthread_attr_setstacksize(&attribs, init_args.stack_size);

  result = pthread_create(&p_thread, &attribs, DN_OS_ThreadFunc_, thread) == 0;
  pthread_attr_destroy(&attribs);

  if (result) {
    DN_Memcpy(&thread->handle, &p_thread, sizeof(p_thread));
    DN_Memcpy(&thread->thread_id, &p_thread, sizeof(p_thread));
    if (thread->flags & DN_OSThreadFlags_Detached) {
      pthread_detach(p_thread);
      thread->handle = {};
    }
  }

  DN_OS_ThreadPostInit_(thread, result);
  return result;
}

DN_API bool DN_OS_ThreadJoin(DN_OSThread *thread, DN_TCDeinitArenas deinit_arenas)
{
  bool result = false;
  if (thread && thread->handle) {
    DN_AssertF(DN_BitIsNotSet(thread->flags, DN_OSThreadFlags_Detached),
               "Detached threads should have their handle immediately closed and invalidated so this branch should never hit.");
    pthread_t thread_id = {};
    DN_Memcpy(&thread_id, &thread->thread_id, sizeof(thread_id));

    void *return_val  = nullptr;
    result            = pthread_join(thread_id, &return_val) == 0;
    thread->handle    = {};
    thread->thread_id = {};
    DN_TCDeinit(&thread->context, deinit_arenas);
  }
  return result;
}

DN_API DN_U32 DN_OS_ThreadID()
{
  pid_t result = gettid();
  DN_Assert(gettid() >= 0);
  return DN_Cast(DN_U32) result;
}

DN_API void DN_OS_PosixInit(DN_OSPosixCore *posix)
{
  int mutex_init = pthread_mutex_init(&posix->sync_primitive_free_list_mutex, nullptr);
  DN_Assert(mutex_init == 0);

  struct timespec ts;
  posix->clock_monotonic_raw = clock_gettime(CLOCK_MONOTONIC_RAW, &ts) != -1;
  if (!posix->clock_monotonic_raw) {
    int get_result = clock_gettime(CLOCK_MONOTONIC, &ts);
    DN_AssertF(get_result != -1, "CLOCK_MONOTONIC_RAW and CLOCK_MONOTONIC are not supported by this platform");
  }
}

DN_API void DN_OS_PosixThreadSetName(DN_Str8 name)
{
#if defined(DN_PLATFORM_EMSCRIPTEN)
  (void)name;
#else
  DN_TCScratch scratch = DN_TCScratchBeginArena(nullptr, 0);
  DN_Str8      copy    = DN_Str8FromStr8Arena(name, &scratch.arena);
  pthread_t    thread  = pthread_self();
  pthread_setname_np(thread, (char *)copy.data);
  DN_TCScratchEnd(&scratch);
#endif
}

DN_API DN_OSPosixProcSelfStatus DN_OS_PosixProcSelfStatus()
{
  DN_OSPosixProcSelfStatus result = {};

  // NOTE: Example
  //
  //  ...
  //  VmPeak:     3352 kB
  //  VmSize:     3352 kB
  //  VmLck:         0 kB
  //  ...
  //
  // VmSize is the total virtual memory used
  DN_OSFile    file = DN_OS_FileOpen(DN_Str8Lit("/proc/self/status"), DN_OSFileOpen_OpenIfExist, DN_OSFileAccess_Read, nullptr);

  if (!file.error) {
    DN_TCScratch   scratch = DN_TCScratchBeginArena(nullptr, 0);
    char           buf[256];
    DN_Str8Builder builder = DN_Str8BuilderFromArena(&scratch.arena);
    for (;;) {
      DN_OSFileRead read = DN_OS_FileRead(&file, buf, sizeof(buf), nullptr);
      if (!read.success || read.bytes_read == 0)
        break;
      DN_Str8BuilderAppendF(&builder, "%.*s", DN_Cast(int)read.bytes_read, buf);
    }

    DN_Str8 const      NAME       = DN_Str8Lit("Name:");
    DN_Str8 const      PID        = DN_Str8Lit("Pid:");
    DN_Str8 const      VM_PEAK    = DN_Str8Lit("VmPeak:");
    DN_Str8 const      VM_SIZE    = DN_Str8Lit("VmSize:");
    DN_Str8            status_buf = DN_Str8FromStr8BuilderArena(&builder, &scratch.arena);
    DN_Str8SplitResult lines      = DN_Str8SplitArena(status_buf, DN_Str8Lit("\n"), DN_Str8SplitFlags_ExcludeEmptyStrings, &scratch.arena);

    for (DN_ForItSize(line_it, DN_Str8, lines.data, lines.count)) {
      DN_Str8 line = DN_Str8TrimWhitespaceAround(*line_it.data);
      if (DN_Str8StartsWith(line, NAME, DN_Str8EqCase_Insensitive)) {
        DN_Str8 str8     = DN_Str8TrimWhitespaceAround(DN_Str8Subset(line, NAME.count, line.count));
        result.name_size = DN_Min(str8.count, sizeof(result.name));
        DN_Memcpy(result.name, str8.data, result.name_size);
      } else if (DN_Str8StartsWith(line, PID, DN_Str8EqCase_Insensitive)) {
        DN_Str8          str8   = DN_Str8TrimWhitespaceAround(DN_Str8Subset(line, PID.count, line.count));
        DN_U64FromResult to_u64 = DN_U64FromStr8(str8);
        result.pid              = to_u64.value;
        DN_Assert(to_u64.success);
      } else if (DN_Str8StartsWith(line, VM_SIZE, DN_Str8EqCase_Insensitive)) {
        DN_Str8 size_with_kb = DN_Str8TrimWhitespaceAround(DN_Str8Subset(line, VM_SIZE.count, line.count));
        DN_Assert(DN_Str8EndsWith(size_with_kb, DN_Str8Lit("kB")));
        DN_Str8          vm_size = DN_Str8BSplit(size_with_kb, DN_Str8Lit(" ")).lhs;
        DN_U64FromResult to_u64  = DN_U64FromStr8(vm_size);
        result.vm_size           = DN_Kilobytes(to_u64.value);
        DN_Assert(to_u64.success);
      } else if (DN_Str8StartsWith(line, VM_PEAK, DN_Str8EqCase_Insensitive)) {
        DN_Str8 size_with_kb = DN_Str8TrimWhitespaceAround(DN_Str8Subset(line, VM_PEAK.count, line.count));
        DN_Assert(DN_Str8EndsWith(size_with_kb, DN_Str8Lit("kB")));
        DN_Str8          vm_size = DN_Str8BSplit(size_with_kb, DN_Str8Lit(" ")).lhs;
        DN_U64FromResult to_u64  = DN_U64FromStr8(vm_size);
        result.vm_peak           = DN_Kilobytes(to_u64.value);
        DN_Assert(to_u64.success);
      }
    }
    DN_TCScratchEnd(&scratch);
  }
  DN_OS_FileClose(&file);
  return result;
}
