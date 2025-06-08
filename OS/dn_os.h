#if !defined(DN_OS_H)
#define DN_OS_H

#include <new> // operator new

#if defined(DN_PLATFORM_EMSCRIPTEN) || defined(DN_PLATFORM_POSIX) || defined(DN_PLATFORM_ARM64)
  #include "dn_os_posix.h"
#elif defined(DN_PLATFORM_WIN32)
  #include "dn_os_windows.h"
  #include "dn_os_w32.h"
#else
  #error Please define a platform e.g. 'DN_PLATFORM_WIN32' to enable the correct implementation for platform APIs
#endif


#if !defined(DN_OS_WIN32) || defined(DN_OS_WIN32_USE_PTHREADS)
  #include <pthread.h>
  #include <semaphore.h>
#endif

#if defined(DN_PLATFORM_POSIX) || defined(DN_PLATFORM_EMSCRIPTEN)
  #include <errno.h>      // errno
  #include <fcntl.h>      // O_RDONLY ... etc
  #include <sys/ioctl.h>  // ioctl
  #include <sys/mman.h>   // mmap
  #include <sys/random.h> // getrandom
  #include <sys/stat.h>   // stat
  #include <sys/types.h>  // pid_t
  #include <sys/wait.h>   // waitpid
  #include <time.h>       // clock_gettime, nanosleep
  #include <unistd.h>     // access, gettid, write

  #if !defined(DN_PLATFORM_EMSCRIPTEN)
    #include <linux/fs.h>     // FICLONE
    #include <sys/sendfile.h> // sendfile
  #endif
#endif

#if defined(DN_PLATFORM_EMSCRIPTEN)
  #include <emscripten/fetch.h> // emscripten_fetch (for DN_OSHttpResponse)
#endif

// NOTE: DN_OSDate /////////////////////////////////////////////////////////////////////////////////
struct DN_OSDateTimeStr8
{
  char  date[DN_ArrayCountU("YYYY-MM-SS")];
  DN_U8 date_size;
  char  hms[DN_ArrayCountU("HH:MM:SS")];
  DN_U8 hms_size;
};

struct DN_OSDateTime
{
  DN_U8  day;
  DN_U8  month;
  DN_U16 year;
  DN_U8  hour;
  DN_U8  minutes;
  DN_U8  seconds;
};

struct DN_OSTimer /// Record time between two time-points using the OS's performance counter.
{
  DN_U64 start;
  DN_U64 end;
};

#if !defined(DN_NO_OS_FILE_API)
// NOTE: DN_OSFile /////////////////////////////////////////////////////////////////////////////////
enum DN_OSPathInfoType
{
  DN_OSPathInfoType_Unknown,
  DN_OSPathInfoType_Directory,
  DN_OSPathInfoType_File,
};

struct DN_OSPathInfo
{
  bool              exists;
  DN_OSPathInfoType type;
  DN_U64            create_time_in_s;
  DN_U64            last_write_time_in_s;
  DN_U64            last_access_time_in_s;
  DN_U64            size;
};

struct DN_OSDirIterator
{
  void   *handle;
  DN_Str8 file_name;
  char    buffer[512];
};

// NOTE: R/W Stream API ////////////////////////////////////////////////////////////////////////////
struct DN_OSFileRead
{
  bool     success;
  DN_USize bytes_read;
};

struct DN_OSFile
{
  bool  error;
  void *handle;
};

enum DN_OSFileOpen
{
  DN_OSFileOpen_CreateAlways, // Create file if it does not exist, otherwise, zero out the file and open
  DN_OSFileOpen_OpenIfExist,  // Open file at path only if it exists
  DN_OSFileOpen_OpenAlways,   // Open file at path, create file if it does not exist
};

typedef DN_U32 DN_OSFileAccess;

enum DN_OSFileAccess_
{
  DN_OSFileAccess_Read       = 1 << 0,
  DN_OSFileAccess_Write      = 1 << 1,
  DN_OSFileAccess_Execute    = 1 << 2,
  DN_OSFileAccess_AppendOnly = 1 << 3, // This flag cannot be combined with any other access mode
  DN_OSFileAccess_ReadWrite  = DN_OSFileAccess_Read | DN_OSFileAccess_Write,
  DN_OSFileAccess_All        = DN_OSFileAccess_ReadWrite | DN_OSFileAccess_Execute | DN_OSFileAccess_AppendOnly,
};
#endif // DN_NO_OS_FILE_API

// NOTE: DN_OSPath ////////////////////////////////////////////////////////////////////////////////
#if !defined(DN_OSPathSeperator)
  #if defined(DN_OS_WIN32)
    #define DN_OSPathSeperator "\\"
  #else
    #define DN_OSPathSeperator "/"
  #endif
  #define DN_OSPathSeperatorString DN_STR8(DN_OSPathSeperator)
#endif

struct DN_OSPathLink
{
  DN_Str8        string;
  DN_OSPathLink *next;
  DN_OSPathLink *prev;
};

struct DN_OSPath
{
  bool           has_prefix_path_separator;
  DN_OSPathLink *head;
  DN_OSPathLink *tail;
  DN_USize       string_size;
  DN_U16       links_size;
};

// NOTE: DN_OSExec /////////////////////////////////////////////////////////////////////////////////
typedef DN_U32 DN_OSExecFlags;

enum DN_OSExecFlags_
{
  DN_OSExecFlags_Nil                 = 0,
  DN_OSExecFlags_SaveStdout          = 1 << 0,
  DN_OSExecFlags_SaveStderr          = 1 << 1,
  DN_OSExecFlags_SaveOutput          = DN_OSExecFlags_SaveStdout | DN_OSExecFlags_SaveStderr,
  DN_OSExecFlags_MergeStderrToStdout = 1 << 2 | DN_OSExecFlags_SaveOutput,
};

struct DN_OSExecAsyncHandle
{
  DN_OSExecFlags exec_flags;
  DN_U32       os_error_code;
  DN_U32       exit_code;
  void          *process;
  void          *stdout_read;
  void          *stdout_write;
  void          *stderr_read;
  void          *stderr_write;
};

struct DN_OSExecResult
{
  bool    finished;
  DN_Str8 stdout_text;
  DN_Str8 stderr_text;
  DN_U32  os_error_code;
  DN_U32  exit_code;
};

struct DN_OSExecArgs
{
  DN_OSExecFlags    flags;
  DN_Str8           working_dir;
  DN_Slice<DN_Str8> environment;
};

// NOTE: DN_OSSemaphore ////////////////////////////////////////////////////////////////////////////
DN_U32 const DN_OS_SEMAPHORE_INFINITE_TIMEOUT = UINT32_MAX;

struct DN_OSSemaphore
{
  DN_U64 handle;
};

enum DN_OSSemaphoreWaitResult
{
  DN_OSSemaphoreWaitResult_Failed,
  DN_OSSemaphoreWaitResult_Success,
  DN_OSSemaphoreWaitResult_Timeout,
};

struct DN_OSMutex
{
  DN_U64 handle;
};

struct DN_OSConditionVariable
{
  DN_U64 handle;
};

// NOTE: DN_OSThread ///////////////////////////////////////////////////////////////////////////////
typedef DN_I32(DN_OSThreadFunc)(struct DN_OSThread *);

struct DN_OSThread
{
  DN_FStr8<64>     name;
  DN_OSTLS         tls;
  DN_OSTLSInitArgs tls_init_args;
  void            *handle;
  DN_U64           thread_id;
  void            *user_context;
  DN_OSThreadFunc *func;
  DN_OSSemaphore   init_semaphore;
};

// NOTE: DN_OSHttp /////////////////////////////////////////////////////////////////////////////////
enum DN_OSHttpRequestSecure
{
  DN_OSHttpRequestSecure_No,
  DN_OSHttpRequestSecure_Yes,
};

struct DN_OSHttpResponse
{
  // NOTE: Response data
  DN_U32   error_code;
  DN_Str8  error_msg;
  DN_U16   http_status;
  DN_Str8  body;
  DN_B32   done;

  // NOTE: Book-keeping
  DN_Arena *arena; // Allocates memory for the response

  // NOTE: Async book-keeping
  // Synchronous HTTP response uses the TLS scratch arena whereas async
  // calls use their own dedicated arena.
  DN_Arena       tmp_arena;
  DN_Arena      *tmem_arena;
  DN_Str8Builder builder;
  DN_OSSemaphore on_complete_semaphore;

  #if defined(DN_PLATFORM_EMSCRIPTEN)
  emscripten_fetch_t *em_handle;
  #elif defined(DN_PLATFORM_WIN32)
  HINTERNET w32_request_session;
  HINTERNET w32_request_connection;
  HINTERNET w32_request_handle;
  #endif
};

struct DN_OSInitArgs
{
  DN_U64 tls_reserve;
  DN_U64 tls_commit;
  DN_U64 tls_err_sink_reserve;
  DN_U64 tls_err_sink_commit;
};

struct DN_OSCore
{
  DN_CPUReport                    cpu_report;
  DN_OSTLS                        tls;                      // Thread local storage state for the main thread.

  // NOTE: Logging ///////////////////////////////////////////////////////////////////////////////
  DN_LOGEmitFromTypeFVFunc *      log_callback;             // Set this pointer to override the logging routine
  void *                          log_user_data;            // User pointer passed into 'log_callback'
  bool                            log_to_file;              // Output logs to file as well as standard out
  DN_OSFile                       log_file;                 // TODO(dn): Hmmm, how should we do this... ?
  DN_TicketMutex                  log_file_mutex;           // Is locked when instantiating the log_file for the first time
  bool                            log_no_colour;            // Disable colours in the logging output

  // NOTE: OS //////////////////////////////////////////////////////////////////////////////////////
  DN_U32                          logical_processor_count;
  DN_U32                          page_size;
  DN_U32                          alloc_granularity;

  // NOTE: Memory ////////////////////////////////////////////////////////////////////////////////
  // Total OS mem allocs in lifetime of program (e.g. malloc, VirtualAlloc, HeapAlloc ...). This
  // only includes allocations routed through the library such as the growing nature of arenas or
  // using the memory allocation routines in the library like DN_OS_MemCommit and so forth.
  DN_U64                          vmem_allocs_total;
  DN_U64                          vmem_allocs_frame;        // Total OS virtual memory allocs since the last 'DN_Core_FrameBegin' was invoked
  DN_U64                          mem_allocs_total;
  DN_U64                          mem_allocs_frame;         // Total OS heap allocs since the last 'DN_Core_FrameBegin' was invoked

  DN_Arena                        arena;
  void                           *platform_context;
};

struct DN_OSDiskSpace
{
  bool   success;
  DN_U64 avail;
  DN_U64 size;
};

DN_API void                      DN_OS_Init                        (DN_OSCore *os, DN_OSInitArgs *args);
DN_API void                      DN_OS_EmitLogsWithOSPrintFunctions(DN_OSCore *os);
DN_API void                      DN_OS_DumpThreadContextArenaStat  (DN_Str8 file_path);

// NOTE: Memory ////////////////////////////////////////////////////////////////////////////////////
DN_API void *                    DN_OS_MemReserve (DN_USize size, DN_MemCommit commit, DN_MemPage page_flags);
DN_API bool                      DN_OS_MemCommit  (void *ptr, DN_USize size, DN_U32 page_flags);
DN_API void                      DN_OS_MemDecommit(void *ptr, DN_USize size);
DN_API void                      DN_OS_MemRelease (void *ptr, DN_USize size);
DN_API int                       DN_OS_MemProtect (void *ptr, DN_USize size, DN_U32 page_flags);

// NOTE: Heap
DN_API void *                    DN_OS_MemAlloc   (DN_USize size, DN_ZeroMem zero_mem);
DN_API void                      DN_OS_MemDealloc (void *ptr);

// NOTE: DN_OSDate /////////////////////////////////////////////////////////////////////////////////
DN_API DN_OSDateTime             DN_OS_DateLocalTimeNow    ();
DN_API DN_OSDateTimeStr8         DN_OS_DateLocalTimeStr8Now(char date_separator = '-', char hms_separator = ':');
DN_API DN_OSDateTimeStr8         DN_OS_DateLocalTimeStr8   (DN_OSDateTime time, char date_separator = '-', char hms_separator = ':');
DN_API DN_U64                    DN_OS_DateUnixTimeNs      ();
#define                          DN_OS_DateUnixTimeUs()    (DN_OS_DateUnixTimeNs() / 1000)
#define                          DN_OS_DateUnixTimeMs()    (DN_OS_DateUnixTimeNs() / (1000 * 1000))
#define                          DN_OS_DateUnixTimeS()     (DN_OS_DateUnixTimeNs() / (1000 * 1000 * 1000))
DN_API DN_OSDateTime             DN_OS_DateUnixTimeSToDate (DN_U64 time);
DN_API DN_U64                    DN_OS_DateLocalToUnixTimeS(DN_OSDateTime date);
DN_API DN_U64                    DN_OS_DateToUnixTimeS     (DN_OSDateTime date);
DN_API bool                      DN_OS_DateIsValid         (DN_OSDateTime date);

// NOTE: Other /////////////////////////////////////////////////////////////////////////////////////
DN_API bool                      DN_OS_SecureRNGBytes      (void *buffer, DN_U32 size);
DN_API bool                      DN_OS_SetEnvVar           (DN_Str8 name, DN_Str8 value);
DN_API DN_OSDiskSpace            DN_OS_DiskSpace           (DN_Str8 path);
DN_API DN_Str8                   DN_OS_EXEPath             (DN_Arena *arena);
DN_API DN_Str8                   DN_OS_EXEDir              (DN_Arena *arena);
#define                          DN_OS_EXEDirFromTLS()     DN_OS_EXEDir(DN_OS_TLSTopArena())
DN_API void                      DN_OS_SleepMs             (DN_UInt milliseconds);

// NOTE: Counters //////////////////////////////////////////////////////////////////////////////////
DN_API DN_U64                    DN_OS_PerfCounterNow      ();
DN_API DN_U64                    DN_OS_PerfCounterFrequency();
DN_API DN_F64                    DN_OS_PerfCounterS        (DN_U64 begin, uint64_t end);
DN_API DN_F64                    DN_OS_PerfCounterMs       (DN_U64 begin, uint64_t end);
DN_API DN_F64                    DN_OS_PerfCounterUs       (DN_U64 begin, uint64_t end);
DN_API DN_F64                    DN_OS_PerfCounterNs       (DN_U64 begin, uint64_t end);
DN_API DN_OSTimer                DN_OS_TimerBegin          ();
DN_API void                      DN_OS_TimerEnd            (DN_OSTimer *timer);
DN_API DN_F64                    DN_OS_TimerS              (DN_OSTimer timer);
DN_API DN_F64                    DN_OS_TimerMs             (DN_OSTimer timer);
DN_API DN_F64                    DN_OS_TimerUs             (DN_OSTimer timer);
DN_API DN_F64                    DN_OS_TimerNs             (DN_OSTimer timer);
DN_API DN_U64                    DN_OS_EstimateTSCPerSecond(uint64_t duration_ms_to_gauge_tsc_frequency);
#if !defined(DN_NO_OS_FILE_API)
// NOTE: File system paths /////////////////////////////////////////////////////////////////////////
DN_API DN_OSPathInfo             DN_OS_PathInfo       (DN_Str8 path);
DN_API bool                      DN_OS_FileIsOlderThan(DN_Str8 file, DN_Str8 check_against);
DN_API bool                      DN_OS_PathDelete     (DN_Str8 path);
DN_API bool                      DN_OS_FileExists     (DN_Str8 path);
DN_API bool                      DN_OS_CopyFile       (DN_Str8 src, DN_Str8 dest, bool overwrite, DN_OSErrSink *err);
DN_API bool                      DN_OS_MoveFile       (DN_Str8 src, DN_Str8 dest, bool overwrite, DN_OSErrSink *err);
DN_API bool                      DN_OS_MakeDir        (DN_Str8 path);
DN_API bool                      DN_OS_DirExists      (DN_Str8 path);
DN_API bool                      DN_OS_DirIterate     (DN_Str8 path, DN_OSDirIterator *it);

// NOTE: R/W Stream API ////////////////////////////////////////////////////////////////////////////
DN_API DN_OSFile                 DN_OS_FileOpen    (DN_Str8 path, DN_OSFileOpen open_mode, DN_OSFileAccess access, DN_OSErrSink *err);
DN_API DN_OSFileRead             DN_OS_FileRead    (DN_OSFile *file, void *buffer, DN_USize size, DN_OSErrSink *err);
DN_API bool                      DN_OS_FileWritePtr(DN_OSFile *file, void const *data, DN_USize size, DN_OSErrSink *err);
DN_API bool                      DN_OS_FileWrite   (DN_OSFile *file, DN_Str8 buffer, DN_OSErrSink *err);
DN_API bool                      DN_OS_FileWriteFV (DN_OSFile *file, DN_OSErrSink *err, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API bool                      DN_OS_FileWriteF  (DN_OSFile *file, DN_OSErrSink *err, DN_FMT_ATTRIB char const *fmt, ...);
DN_API bool                      DN_OS_FileFlush   (DN_OSFile *file, DN_OSErrSink *err);
DN_API void                      DN_OS_FileClose   (DN_OSFile *file);

// NOTE: R/W Entire File ///////////////////////////////////////////////////////////////////////////
DN_API DN_Str8                   DN_OS_ReadAll          (DN_Arena *arena, DN_Str8 path, DN_OSErrSink *err);
#define                          DN_OS_ReadAllFromTLS(...) DN_OS_ReadAll(DN_OS_TLSTopArena(), ##__VA_ARGS__)
DN_API bool                      DN_OS_WriteAll         (DN_Str8 path, DN_Str8 buffer, DN_OSErrSink *err);
DN_API bool                      DN_OS_WriteAllFV       (DN_Str8 path, DN_OSErrSink *err, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API bool                      DN_OS_WriteAllF        (DN_Str8 path, DN_OSErrSink *err, DN_FMT_ATTRIB char const *fmt, ...);
DN_API bool                      DN_OS_WriteAllSafe     (DN_Str8 path, DN_Str8 buffer, DN_OSErrSink *err);
DN_API bool                      DN_OS_WriteAllSafeFV   (DN_Str8 path, DN_OSErrSink *err, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API bool                      DN_OS_WriteAllSafeF    (DN_Str8 path, DN_OSErrSink *err, DN_FMT_ATTRIB char const *fmt, ...);
#endif // !defined(DN_NO_OS_FILE_API)

// NOTE: File system paths /////////////////////////////////////////////////////////////////////////
DN_API bool                      DN_OS_PathAddRef                             (DN_Arena *arena, DN_OSPath *fs_path, DN_Str8 path);
#define                          DN_OS_PathAddRefFromTLS(...)                 DN_OS_PathAddRef(DN_OS_TLSTopArena(), ##__VA_ARGS__)
#define                          DN_OS_PathAddRefFromFrame(...)               DN_OS_PathAddRef(DN_OS_TLSFrameArena(), ##__VA_ARGS__)
DN_API bool                      DN_OS_PathAdd                                (DN_Arena *arena, DN_OSPath *fs_path, DN_Str8 path);
#define                          DN_OS_PathAddFromTLS(...)                    DN_OS_PathAdd(DN_OS_TLSTopArena(), ##__VA_ARGS__)
#define                          DN_OS_PathAddFromFrame(...)                  DN_OS_PathAdd(DN_OS_TLSFrameArena(), ##__VA_ARGS__)
DN_API bool                      DN_OS_PathAddF                               (DN_Arena *arena, DN_OSPath *fs_path, DN_FMT_ATTRIB char const *fmt, ...);
#define                          DN_OS_PathAddFFromTLS(...)                   DN_OS_PathAddF(DN_OS_TLSTopArena(), ##__VA_ARGS__)
#define                          DN_OS_PathAddFFromFrame(...)                 DN_OS_PathAddF(DN_OS_TLSFrameArena(), ##__VA_ARGS__)
DN_API bool                      DN_OS_PathPop                                (DN_OSPath *fs_path);
DN_API DN_Str8                   DN_OS_PathBuildWithSeparator                 (DN_Arena *arena, DN_OSPath const *fs_path, DN_Str8 path_separator);
#define                          DN_OS_PathBuildWithSeperatorFromTLS(...)     DN_OS_PathBuildWithSeperator(DN_OS_TLSTopArena(), ##__VA_ARGS__)
#define                          DN_OS_PathBuildWithSeperatorFromFrame(...)   DN_OS_PathBuildWithSeperator(DN_OS_TLSFrameArena(), ##__VA_ARGS__)
DN_API DN_Str8                   DN_OS_PathTo                                 (DN_Arena *arena, DN_Str8 path, DN_Str8 path_separtor);
#define                          DN_OS_PathToFromTLS(...)                     DN_OS_PathTo(DN_OS_TLSTopArena(), ##__VA_ARGS__)
#define                          DN_OS_PathToFromFrame(...)                   DN_OS_PathTo(DN_OS_TLSFrameArena(), ##__VA_ARGS__)
DN_API DN_Str8                   DN_OS_PathToF                                (DN_Arena *arena, DN_Str8 path_separator, DN_FMT_ATTRIB char const *fmt, ...);
#define                          DN_OS_PathToFFromTLS(...)                    DN_OS_PathToF(DN_OS_TLSTopArena(), ##__VA_ARGS__)
#define                          DN_OS_PathToFFromFrame(...)                  DN_OS_PathToF(DN_OS_TLSFrameArena(), ##__VA_ARGS__)
DN_API DN_Str8                   DN_OS_Path                                   (DN_Arena *arena, DN_Str8 path);
#define                          DN_OS_PathFromTLS(...)                       DN_OS_Path(DN_OS_TLSTopArena(), ##__VA_ARGS__)
#define                          DN_OS_PathFromFrame(...)                     DN_OS_Path(DN_OS_TLSFrameArena(), ##__VA_ARGS__)
DN_API DN_Str8                   DN_OS_PathF                                  (DN_Arena *arena, DN_FMT_ATTRIB char const *fmt, ...);
#define                          DN_OS_PathFFromTLS(...)                      DN_OS_PathF(DN_OS_TLSTopArena(), ##__VA_ARGS__)
#define                          DN_OS_PathFFromFrame(...)                    DN_OS_PathF(DN_OS_TLSFrameArena(), ##__VA_ARGS__)

#define                          DN_OS_PathBuildFwdSlash(allocator, fs_path)  DN_OS_PathBuildWithSeparator(allocator, fs_path, DN_STR8("/"))
#define                          DN_OS_PathBuildBackSlash(allocator, fs_path) DN_OS_PathBuildWithSeparator(allocator, fs_path, DN_STR8("\\"))
#define                          DN_OS_PathBuild(allocator, fs_path)          DN_OS_PathBuildWithSeparator(allocator, fs_path, DN_OSPathSeparatorString)

// NOTE: DN_OSExec /////////////////////////////////////////////////////////////////////////////////
DN_API void                      DN_OS_Exit                 (int32_t exit_code);
DN_API DN_OSExecResult           DN_OS_ExecPump             (DN_OSExecAsyncHandle handle, char *stdout_buffer, size_t *stdout_size, char *stderr_buffer, size_t *stderr_size, DN_U32 timeout_ms, DN_OSErrSink  *err);
DN_API DN_OSExecResult           DN_OS_ExecWait             (DN_OSExecAsyncHandle handle, DN_Arena *arena, DN_OSErrSink *err);
DN_API DN_OSExecAsyncHandle      DN_OS_ExecAsync            (DN_Slice<DN_Str8> cmd_line, DN_OSExecArgs *args, DN_OSErrSink *err);
DN_API DN_OSExecResult           DN_OS_Exec                 (DN_Slice<DN_Str8> cmd_line, DN_OSExecArgs *args, DN_Arena *arena, DN_OSErrSink *err);
DN_API DN_OSExecResult           DN_OS_ExecOrAbort          (DN_Slice<DN_Str8> cmd_line, DN_OSExecArgs *args, DN_Arena *arena);
#define                          DN_OS_ExecOrAbortFromTLS(...) DN_OS_ExecOrAbort(__VA_ARGS__, DN_OS_TLSTopArena())

DN_API DN_OSSemaphore            DN_OS_SemaphoreInit     (DN_U32 initial_count);
DN_API bool                      DN_OS_SemaphoreIsValid  (DN_OSSemaphore *semaphore);
DN_API void                      DN_OS_SemaphoreDeinit   (DN_OSSemaphore *semaphore);
DN_API void                      DN_OS_SemaphoreIncrement(DN_OSSemaphore *semaphore, DN_U32 amount);
DN_API DN_OSSemaphoreWaitResult  DN_OS_SemaphoreWait     (DN_OSSemaphore *semaphore, DN_U32 timeout_ms);

DN_API DN_OSMutex                DN_OS_MutexInit  ();
DN_API void                      DN_OS_MutexDeinit(DN_OSMutex *mutex);
DN_API void                      DN_OS_MutexLock  (DN_OSMutex *mutex);
DN_API void                      DN_OS_MutexUnlock(DN_OSMutex *mutex);
#define DN_OS_MutexScope(mutex)  DN_DeferLoop(DN_OS_MutexLock(mutex), DN_OS_MutexUnlock(mutex))

DN_API DN_OSConditionVariable    DN_OS_ConditionVariableInit     ();
DN_API void                      DN_OS_ConditionVariableDeinit   (DN_OSConditionVariable *cv);
DN_API bool                      DN_OS_ConditionVariableWait     (DN_OSConditionVariable *cv, DN_OSMutex *mutex, DN_U64 sleep_ms);
DN_API bool                      DN_OS_ConditionVariableWaitUntil(DN_OSConditionVariable *cv, DN_OSMutex *mutex, DN_U64 end_ts_ms);
DN_API void                      DN_OS_ConditionVariableSignal   (DN_OSConditionVariable *cv);
DN_API void                      DN_OS_ConditionVariableBroadcast(DN_OSConditionVariable *cv);

DN_API bool                      DN_OS_ThreadInit  (DN_OSThread *thread, DN_OSThreadFunc *func, void *user_context);
DN_API void                      DN_OS_ThreadDeinit(DN_OSThread *thread);
DN_API DN_U32                    DN_OS_ThreadID    ();
DN_API void                      DN_OS_ThreadSetName(DN_Str8 name);

DN_API void                      DN_OS_HttpRequestAsync(DN_OSHttpResponse *response, DN_Arena *arena, DN_Str8 host, DN_Str8 path, DN_OSHttpRequestSecure secure, DN_Str8 method, DN_Str8 body, DN_Str8 headers);
DN_API void                      DN_OS_HttpRequestWait (DN_OSHttpResponse *response);
DN_API void                      DN_OS_HttpRequestFree (DN_OSHttpResponse *response);
DN_API DN_OSHttpResponse         DN_OS_HttpRequest     (DN_Arena *arena, DN_Str8 host, DN_Str8 path, DN_OSHttpRequestSecure secure, DN_Str8 method, DN_Str8 body, DN_Str8 headers);
#endif // !defined(DN_OS_H)
