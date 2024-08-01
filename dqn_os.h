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
//   dqn_os.h -- Common APIs/services provided by the operating system/platform layer
//
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// [$OMEM] Dqn_OSMem           --               -- Memory allocation (typically virtual memory if supported)
// [$DATE] Dqn_OSDate          --               -- Date time APIs
// [$FILE] Dqn_OSPathInfo/File --               -- File path info/reading/writing
// [$PATH] Dqn_OSPath          --               -- Construct native OS paths helpers
// [$EXEC] Dqn_OSExec          --               -- Execute programs programatically
// [$SEMA] Dqn_OSSemaphore     -- DQN_SEMAPHORE --
// [$MUTX] Dqn_OSMutex         --               --
// [$THRD] Dqn_OSThread        -- DQN_THREAD    --
// [$HTTP] Dqn_OSHttp          --               --
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

// NOTE: [$OMEM] Dqn_OSMem //////////////////////////////////////////////////////////////////////////
enum Dqn_OSMemCommit
{
    Dqn_OSMemCommit_No,
    Dqn_OSMemCommit_Yes,
};

enum Dqn_OSMemPage
{
    // Exception on read/write with a page. This flag overrides the read/write 
    // access.
    Dqn_OSMemPage_NoAccess  = 1 << 0,

    Dqn_OSMemPage_Read      = 1 << 1, // Only read permitted on the page.

    // Only write permitted on the page. On Windows this is not supported and
    // will be promoted to read+write permissions.
    Dqn_OSMemPage_Write     = 1 << 2,

    Dqn_OSMemPage_ReadWrite = Dqn_OSMemPage_Read | Dqn_OSMemPage_Write,

    // Modifier used in conjunction with previous flags. Raises exception on
    // first access to the page, then, the underlying protection flags are
    // active. This is supported on Windows, on other OS's using this flag will
    // set the OS equivalent of Dqn_OSMemPage_NoAccess.
    // This flag must only be used in Dqn_OSMem_Protect
    Dqn_OSMemPage_Guard     = 1 << 3,

    // If leak tracing is enabled, this flag will allow the allocation recorded
    // from the reserve call to be leaked, e.g. not printed when leaks are
    // dumped to the console.
    Dqn_OSMemPage_AllocRecordLeakPermitted = 1 << 4,

    // If leak tracing is enabled this flag will prevent any allocation record
    // from being created in the allocation table at all. If this flag is
    // enabled, 'OSMemPage_AllocRecordLeakPermitted' has no effect since the
    // record will never be created.
    Dqn_OSMemPage_NoAllocRecordEntry = 1 << 5,

    // [INTERNAL] Do not use. All flags together do not constitute a correct
    // configuration of pages.
    Dqn_OSMemPage_All                = Dqn_OSMemPage_NoAccess |
                                      Dqn_OSMemPage_ReadWrite |
                                      Dqn_OSMemPage_Guard |
                                      Dqn_OSMemPage_AllocRecordLeakPermitted |
                                      Dqn_OSMemPage_NoAllocRecordEntry,
};

// NOTE: [$DATE] Dqn_OSDate ////////////////////////////////////////////////////////////////////////
struct Dqn_OSDateTimeStr8
{
    char    date[DQN_ARRAY_UCOUNT("YYYY-MM-SS")];
    uint8_t date_size;
    char    hms[DQN_ARRAY_UCOUNT("HH:MM:SS")];
    uint8_t hms_size;
};

struct Dqn_OSDateTime
{
    uint8_t  day;
    uint8_t  month;
    uint16_t year;
    uint8_t  hour;
    uint8_t  minutes;
    uint8_t  seconds;
};

struct Dqn_OSTimer /// Record time between two time-points using the OS's performance counter.
{
    uint64_t start;
    uint64_t end;
};

#if !defined(DQN_NO_OS_FILE_API)
// NOTE: [$FSYS] Dqn_OSFile ////////////////////////////////////////////////////////////////////////
enum Dqn_OSPathInfoType
{
    Dqn_OSPathInfoType_Unknown,
    Dqn_OSPathInfoType_Directory,
    Dqn_OSPathInfoType_File,
};

struct Dqn_OSPathInfo
{
    bool               exists;
    Dqn_OSPathInfoType type;
    uint64_t           create_time_in_s;
    uint64_t           last_write_time_in_s;
    uint64_t           last_access_time_in_s;
    uint64_t           size;
};

struct Dqn_OS_DirIterator
{
    void    *handle;
    Dqn_Str8 file_name;
    char     buffer[512];
};

// NOTE: R/W Stream API ////////////////////////////////////////////////////////////////////////////
struct Dqn_OSFile
{
    bool  error;
    void *handle;
};

enum Dqn_OSFileOpen
{
    Dqn_OSFileOpen_CreateAlways, // Create file if it does not exist, otherwise, zero out the file and open
    Dqn_OSFileOpen_OpenIfExist,  // Open file at path only if it exists
    Dqn_OSFileOpen_OpenAlways,   // Open file at path, create file if it does not exist
};

enum Dqn_OSFileAccess
{
    Dqn_OSFileAccess_Read       = 1 << 0,
    Dqn_OSFileAccess_Write      = 1 << 1,
    Dqn_OSFileAccess_Execute    = 1 << 2,
    Dqn_OSFileAccess_AppendOnly = 1 << 3, // This flag cannot be combined with any other access mode
    Dqn_OSFileAccess_ReadWrite  = Dqn_OSFileAccess_Read      | Dqn_OSFileAccess_Write,
    Dqn_OSFileAccess_All        = Dqn_OSFileAccess_ReadWrite | Dqn_OSFileAccess_Execute,
};
#endif // DQN_NO_OS_FILE_API

// NOTE: Dqn_OSPath ////////////////////////////////////////////////////////////////////////////////
#if !defined(Dqn_OSPathSeperator)
    #if defined(DQN_OS_WIN32)
        #define Dqn_OSPathSeperator "\\"
    #else
        #define Dqn_OSPathSeperator "/"
    #endif
    #define Dqn_OSPathSeperatorString DQN_STR8(Dqn_OSPathSeperator)
#endif

struct Dqn_OSPathLink
{
    Dqn_Str8        string;
    Dqn_OSPathLink *next;
    Dqn_OSPathLink *prev;
};

struct Dqn_OSPath
{
    bool            has_prefix_path_separator;
    Dqn_OSPathLink *head;
    Dqn_OSPathLink *tail;
    Dqn_usize       string_size;
    uint16_t        links_size;
};

// NOTE: [$EXEC] Dqn_OSExec ////////////////////////////////////////////////////////////////////////
enum Dqn_OSExecFlag
{
    Dqn_OSExecFlag_Nil                 = 0,
    Dqn_OSExecFlag_SaveStdout          = 1 << 0,
    Dqn_OSExecFlag_SaveStderr          = 1 << 1,
    Dqn_OSExecFlag_SaveOutput          = Dqn_OSExecFlag_SaveStdout | Dqn_OSExecFlag_SaveStderr,
    Dqn_OSExecFlag_MergeStderrToStdout = 1 << 2                    | Dqn_OSExecFlag_SaveOutput,
};

struct Dqn_OSExecAsyncHandle
{
    uint8_t   exec_flags;
    uint32_t  os_error_code;
    uint32_t  exit_code;
    void     *process;
    void     *stdout_read;
    void     *stdout_write;
    void     *stderr_read;
    void     *stderr_write;
};

struct Dqn_OSExecResult
{
    Dqn_Str8 stdout_text;
    Dqn_Str8 stderr_text;
    uint32_t os_error_code;
    uint32_t exit_code;
};

#if !defined(DQN_NO_SEMAPHORE)
// NOTE: [$SEMA] Dqn_OSSemaphore ///////////////////////////////////////////////////////////////////
uint32_t const DQN_OS_SEMAPHORE_INFINITE_TIMEOUT = UINT32_MAX;

struct Dqn_OSSemaphore
{
    #if defined(DQN_OS_WIN32) && !defined(DQN_OS_WIN32_USE_PTHREADS)
    void *win32_handle;
    #else
    sem_t posix_handle;
    bool  posix_init;
    #endif
};

enum Dqn_OSSemaphoreWaitResult
{
    Dqn_OSSemaphoreWaitResult_Failed,
    Dqn_OSSemaphoreWaitResult_Success,
    Dqn_OSSemaphoreWaitResult_Timeout,
};
#endif // !defined(DQN_NO_SEMAPHORE)

// NOTE: [$THRD] Dqn_OSThread /////////////////////////////////////////////////////////////////////
#if !defined(DQN_NO_THREAD) && !defined(DQN_NO_SEMAPHORE)
typedef int32_t (Dqn_OSThreadFunc)(struct Dqn_OSThread*);

struct Dqn_OSThread
{
    Dqn_TLS           tls;
    void             *handle;
    uint64_t          thread_id;
    void             *user_context;
    Dqn_OSThreadFunc *func;
    Dqn_OSSemaphore   init_semaphore;
};
#endif // !defined(DQN_NO_THREAD)

// NOTE: [$HTTP] Dqn_OSHttp ////////////////////////////////////////////////////////////////////////
enum Dqn_OSHttpRequestSecure
{
    Dqn_OSHttpRequestSecure_No,
    Dqn_OSHttpRequestSecure_Yes,
};

struct Dqn_OSHttpResponse
{
    // NOTE: Response data
    uint32_t   error_code;
    Dqn_Str8   error_msg;
    uint16_t   http_status;
    Dqn_Str8   body;
    Dqn_b32    done;

    // NOTE: Book-keeping
    Dqn_Arena *arena; // Allocates memory for the response

    // NOTE: Async book-keeping
    // Synchronous HTTP response uses the TLS scratch arena whereas async
    // calls use their own dedicated arena.
    Dqn_Arena         tmp_arena;
    Dqn_Arena        *tmem_arena;
    Dqn_Str8Builder   builder;
    Dqn_OSSemaphore   on_complete_semaphore;

    #if defined(DQN_PLATFORM_EMSCRIPTEN)
    emscripten_fetch_t  *em_handle;
    #elif defined(DQN_OS_WIN32)
    HINTERNET            win32_request_session;
    HINTERNET            win32_request_connection;
    HINTERNET            win32_request_handle;
    #endif
};

// NOTE: [$OMEM] Memory //////////////////////////////////////////////////////////////////////////
DQN_API void *                    Dqn_OS_MemReserve (Dqn_usize size, Dqn_OSMemCommit commit, uint32_t page_flags);
DQN_API bool                      Dqn_OS_MemCommit  (void *ptr, Dqn_usize size, uint32_t page_flags);
DQN_API void                      Dqn_OS_MemDecommit(void *ptr, Dqn_usize size);
DQN_API void                      Dqn_OS_MemRelease (void *ptr, Dqn_usize size);
DQN_API int                       Dqn_OS_MemProtect (void *ptr, Dqn_usize size, uint32_t page_flags);

// NOTE: [$DATE] Date //////////////////////////////////////////////////////////////////////////////
DQN_API Dqn_OSDateTime            Dqn_OS_DateLocalTimeNow    ();
DQN_API Dqn_OSDateTimeStr8        Dqn_OS_DateLocalTimeStr8Now(char date_separator = '-', char hms_separator = ':');
DQN_API Dqn_OSDateTimeStr8        Dqn_OS_DateLocalTimeStr8   (Dqn_OSDateTime time, char date_separator = '-', char hms_separator = ':');
DQN_API uint64_t                  Dqn_OS_DateUnixTime        ();
DQN_API Dqn_OSDateTime            Dqn_OS_DateUnixTimeToDate  (uint64_t time);
DQN_API uint64_t                  Dqn_OS_DateToUnixTime      (Dqn_OSDateTime date);
DQN_API bool                      Dqn_OS_DateIsValid         (Dqn_OSDateTime date);

// NOTE: Other /////////////////////////////////////////////////////////////////////////////////////
DQN_API bool                      Dqn_OS_SecureRNGBytes      (void *buffer, uint32_t size);
DQN_API Dqn_Str8                  Dqn_OS_EXEPath             (Dqn_Arena *arena);
DQN_API Dqn_Str8                  Dqn_OS_EXEDir              (Dqn_Arena *arena);
#define                           Dqn_OS_EXEDir_TLS()        Dqn_OS_EXEDir(Dqn_TLS_TopArena())
DQN_API void                      Dqn_OS_SleepMs             (Dqn_uint milliseconds);
// NOTE: Counters //////////////////////////////////////////////////////////////////////////////////
DQN_API uint64_t                  Dqn_OS_PerfCounterNow      ();
DQN_API uint64_t                  Dqn_OS_PerfCounterFrequency();
DQN_API Dqn_f64                   Dqn_OS_PerfCounterS        (uint64_t begin, uint64_t end);
DQN_API Dqn_f64                   Dqn_OS_PerfCounterMs       (uint64_t begin, uint64_t end);
DQN_API Dqn_f64                   Dqn_OS_PerfCounterUs       (uint64_t begin, uint64_t end);
DQN_API Dqn_f64                   Dqn_OS_PerfCounterNs       (uint64_t begin, uint64_t end);
DQN_API Dqn_OSTimer               Dqn_OS_TimerBegin          ();
DQN_API void                      Dqn_OS_TimerEnd            (Dqn_OSTimer *timer);
DQN_API Dqn_f64                   Dqn_OS_TimerS              (Dqn_OSTimer timer);
DQN_API Dqn_f64                   Dqn_OS_TimerMs             (Dqn_OSTimer timer);
DQN_API Dqn_f64                   Dqn_OS_TimerUs             (Dqn_OSTimer timer);
DQN_API Dqn_f64                   Dqn_OS_TimerNs             (Dqn_OSTimer timer);
DQN_API uint64_t                  Dqn_OS_EstimateTSCPerSecond(uint64_t duration_ms_to_gauge_tsc_frequency);

#if !defined(DQN_NO_OS_FILE_API)
// NOTE: File system paths /////////////////////////////////////////////////////////////////////////
DQN_API Dqn_OSPathInfo            Dqn_OS_PathInfo       (Dqn_Str8 path);
DQN_API bool                      Dqn_OS_FileIsOlderThan(Dqn_Str8 file, Dqn_Str8 check_against);
DQN_API bool                      Dqn_OS_PathDelete     (Dqn_Str8 path);
DQN_API bool                      Dqn_OS_FileExists     (Dqn_Str8 path);
DQN_API bool                      Dqn_OS_CopyFile       (Dqn_Str8 src, Dqn_Str8 dest, bool overwrite, Dqn_ErrorSink *error);
DQN_API bool                      Dqn_OS_MoveFile       (Dqn_Str8 src, Dqn_Str8 dest, bool overwrite, Dqn_ErrorSink *error);
DQN_API bool                      Dqn_OS_MakeDir        (Dqn_Str8 path);
DQN_API bool                      Dqn_OS_DirExists      (Dqn_Str8 path);
DQN_API bool                      Dqn_OS_DirIterate     (Dqn_Str8 path, Dqn_OS_DirIterator *it);

// NOTE: R/W Stream API ////////////////////////////////////////////////////////////////////////////
DQN_API Dqn_OSFile                Dqn_OS_FileOpen    (Dqn_Str8 path, Dqn_OSFileOpen open_mode, uint32_t access, Dqn_ErrorSink *error);
DQN_API bool                      Dqn_OS_FileRead    (Dqn_OSFile *file, void *buffer, Dqn_usize size, Dqn_ErrorSink *error);
DQN_API bool                      Dqn_OS_FileWritePtr(Dqn_OSFile *file, void const *data, Dqn_usize size, Dqn_ErrorSink *error);
DQN_API bool                      Dqn_OS_FileWrite   (Dqn_OSFile *file, Dqn_Str8 buffer, Dqn_ErrorSink *error);
DQN_API bool                      Dqn_OS_FileWriteFV (Dqn_OSFile *file, Dqn_ErrorSink *error, DQN_FMT_ATTRIB char const *fmt, va_list args);
DQN_API bool                      Dqn_OS_FileWriteF  (Dqn_OSFile *file, Dqn_ErrorSink *error, DQN_FMT_ATTRIB char const *fmt, ...);
DQN_API void                      Dqn_OS_FileClose   (Dqn_OSFile *file);

// NOTE: R/W Entire File ///////////////////////////////////////////////////////////////////////////
DQN_API Dqn_Str8                  Dqn_OS_ReadAll          (Dqn_Arena *arena, Dqn_Str8 path, Dqn_ErrorSink *error);
#define                           Dqn_OS_ReadAll_TLS(...) Dqn_OS_ReadAll(Dqn_TLS_TopArena(), ##__VA_ARGS__)
DQN_API bool                      Dqn_OS_WriteAll         (Dqn_Str8 path, Dqn_Str8 buffer, Dqn_ErrorSink *error);
DQN_API bool                      Dqn_OS_WriteAllFV       (Dqn_Str8 path, Dqn_ErrorSink *error, DQN_FMT_ATTRIB char const *fmt, va_list args);
DQN_API bool                      Dqn_OS_WriteAllF        (Dqn_Str8 path, Dqn_ErrorSink *error, DQN_FMT_ATTRIB char const *fmt, ...);
DQN_API bool                      Dqn_OS_WriteAllSafe     (Dqn_Str8 path, Dqn_Str8 buffer, Dqn_ErrorSink *error);
DQN_API bool                      Dqn_OS_WriteAllSafeFV   (Dqn_Str8 path, Dqn_ErrorSink *error, DQN_FMT_ATTRIB char const *fmt, va_list args);
DQN_API bool                      Dqn_OS_WriteAllSafeF    (Dqn_Str8 path, Dqn_ErrorSink *error, DQN_FMT_ATTRIB char const *fmt, ...);
#endif // !defined(DQN_NO_OS_FILE_API)

// NOTE: File system paths /////////////////////////////////////////////////////////////////////////
DQN_API bool                      Dqn_OS_PathAddRef                             (Dqn_Arena *arena, Dqn_OSPath *fs_path, Dqn_Str8 path);
#define                           Dqn_OS_PathAddRef_TLS(...)                    Dqn_OS_PathAddRef(Dqn_TLS_TopArena(), ##__VA_ARGS__)
DQN_API bool                      Dqn_OS_PathAdd                                (Dqn_Arena *arena, Dqn_OSPath *fs_path, Dqn_Str8 path);
#define                           Dqn_OS_PathAdd_TLS(...)                       Dqn_OS_PathAdd(Dqn_TLS_TopArena(), ##__VA_ARGS__)
DQN_API bool                      Dqn_OS_PathAddF                               (Dqn_Arena *arena, Dqn_OSPath *fs_path, DQN_FMT_ATTRIB char const *fmt, ...);
#define                           Dqn_OS_PathAddF_TLS(...)                      Dqn_OS_PathAddF(Dqn_TLS_TopArena(), ##__VA_ARGS__)
DQN_API bool                      Dqn_OS_PathPop                                (Dqn_OSPath *fs_path);
DQN_API Dqn_Str8                  Dqn_OS_PathBuildWithSeparator                 (Dqn_Arena *arena, Dqn_OSPath const *fs_path, Dqn_Str8 path_separator);
#define                           Dqn_OS_PathBuildWithSeperator_TLS(...)        Dqn_OS_PathBuildWithSeperator(Dqn_TLS_TopArena(), ##__VA_ARGS__)
DQN_API Dqn_Str8                  Dqn_OS_PathTo                                 (Dqn_Arena *arena, Dqn_Str8 path, Dqn_Str8 path_separtor);
#define                           Dqn_OS_PathTo_TLS(...)                        Dqn_OS_PathTo(Dqn_TLS_TopArena(), ##__VA_ARGS__)
DQN_API Dqn_Str8                  Dqn_OS_PathToF                                (Dqn_Arena *arena, Dqn_Str8 path_separator, DQN_FMT_ATTRIB char const *fmt, ...);
#define                           Dqn_OS_PathToF_TLS(...)                       Dqn_OS_PathToF(Dqn_TLS_TopArena(), ##__VA_ARGS__)
DQN_API Dqn_Str8                  Dqn_OS_Path                                   (Dqn_Arena *arena, Dqn_Str8 path);
#define                           Dqn_OS_Path_TLS(...)                          Dqn_OS_Path(Dqn_TLS_TopArena(), ##__VA_ARGS__)
DQN_API Dqn_Str8                  Dqn_OS_PathF                                  (Dqn_Arena *arena, DQN_FMT_ATTRIB char const *fmt, ...);
#define                           Dqn_OS_PathF_TLS(...)                         Dqn_OS_PathF(Dqn_TLS_TopArena(), ##__VA_ARGS__)
#define                           Dqn_OS_PathBuildFwdSlash(allocator, fs_path)  Dqn_OS_PathBuildWithSeparator(allocator, fs_path, DQN_STR8("/"))
#define                           Dqn_OS_PathBuildBackSlash(allocator, fs_path) Dqn_OS_PathBuildWithSeparator(allocator, fs_path, DQN_STR8("\\"))
#define                           Dqn_OS_PathBuild(allocator, fs_path)          Dqn_OS_PathBuildWithSeparator(allocator, fs_path, Dqn_OSPathSeparatorString)

// NOTE: [$EXEC] Dqn_OSExec ////////////////////////////////////////////////////////////////////////
DQN_API void                      Dqn_OS_Exit                 (int32_t exit_code);
DQN_API Dqn_OSExecResult          Dqn_OS_ExecWait             (Dqn_OSExecAsyncHandle handle, Dqn_Arena *arena, Dqn_ErrorSink *error);
DQN_API Dqn_OSExecAsyncHandle     Dqn_OS_ExecAsync            (Dqn_Slice<Dqn_Str8> cmd_line, Dqn_Str8 working_dir, uint8_t exec_flags, Dqn_ErrorSink *error);
DQN_API Dqn_OSExecResult          Dqn_OS_Exec                 (Dqn_Slice<Dqn_Str8> cmd_line, Dqn_Str8 working_dir, uint8_t exec_flags, Dqn_Arena *arena, Dqn_ErrorSink *error);
DQN_API Dqn_OSExecResult          Dqn_OS_ExecOrAbort          (Dqn_Slice<Dqn_Str8> cmd_line, Dqn_Str8 working_dir, uint8_t exec_flags, Dqn_Arena *arena);
#define                           Dqn_OS_ExecOrAbort_TLS(...) Dqn_OS_ExecOrAbort(__VA_ARGS__, Dqn_TLS_TopArena())

// NOTE: [$SEMA] Dqn_OSSemaphore ///////////////////////////////////////////////////////////////////
#if !defined(DQN_NO_SEMAPHORE)
DQN_API Dqn_OSSemaphore           Dqn_OS_SemaphoreInit     (uint32_t initial_count);
DQN_API bool                      Dqn_OS_SemaphoreIsValid  (Dqn_OSSemaphore *semaphore);
DQN_API void                      Dqn_OS_SemaphoreDeinit   (Dqn_OSSemaphore *semaphore);
DQN_API void                      Dqn_OS_SemaphoreIncrement(Dqn_OSSemaphore *semaphore, uint32_t amount);
DQN_API Dqn_OSSemaphoreWaitResult Dqn_OS_SemaphoreWait     (Dqn_OSSemaphore *semaphore, uint32_t timeout_ms);
#endif // !defined(DQN_NO_SEMAPHORE)

// NOTE: [$MUTX] Dqn_OSMutex ///////////////////////////////////////////////////////////////////////
DQN_API Dqn_OSMutex               Dqn_OS_MutexInit  ();
DQN_API void                      Dqn_OS_MutexDeinit(Dqn_OSMutex *mutex);
DQN_API void                      Dqn_OS_MutexLock  (Dqn_OSMutex *mutex);
DQN_API void                      Dqn_OS_MutexUnlock(Dqn_OSMutex *mutex);

// NOTE: [$THRD] Dqn_OSThread /////////////////////////////////////////////////////////////////////
#if !defined(DQN_NO_THREAD) && !defined(DQN_NO_SEMAPHORE)
DQN_API bool                      Dqn_OS_ThreadInit  (Dqn_OSThread *thread, Dqn_OSThreadFunc *func, void *user_context);
DQN_API void                      Dqn_OS_ThreadDeinit(Dqn_OSThread thread);
DQN_API uint32_t                  Dqn_OS_ThreadID    ();
DQN_API void                      Dqn_OS_ThreadSetTLS(Dqn_TLS *tls);
#endif // !defined(DQN_NO_THREAD)

// NOTE: [$HTTP] Dqn_OSHttp ////////////////////////////////////////////////////////////////////////
DQN_API void                      Dqn_OS_HttpRequestAsync(Dqn_OSHttpResponse *response, Dqn_Arena *arena, Dqn_Str8 host, Dqn_Str8 path, Dqn_OSHttpRequestSecure secure, Dqn_Str8 method, Dqn_Str8 body, Dqn_Str8 headers);
DQN_API void                      Dqn_OS_HttpRequestWait (Dqn_OSHttpResponse *response);
DQN_API void                      Dqn_OS_HttpRequestFree (Dqn_OSHttpResponse *response);
DQN_API Dqn_OSHttpResponse        Dqn_OS_HttpRequest     (Dqn_Arena *arena, Dqn_Str8 host, Dqn_Str8 path, Dqn_OSHttpRequestSecure secure, Dqn_Str8 method, Dqn_Str8 body, Dqn_Str8 headers);
