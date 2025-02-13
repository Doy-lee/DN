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
// [$OMEM] DN_OSMem           --               -- Memory allocation (typically virtual memory if supported)
// [$DATE] DN_OSDate          --               -- Date time APIs
// [$FILE] DN_OSPathInfo/File --               -- File path info/reading/writing
// [$PATH] DN_OSPath          --               -- Construct native OS paths helpers
// [$EXEC] DN_OSExec          --               -- Execute programs programatically
// [$SEMA] DN_OSSemaphore     -- DN_SEMAPHORE --
// [$MUTX] DN_OSMutex         --               --
// [$THRD] DN_OSThread        -- DN_THREAD    --
// [$HTTP] DN_OSHttp          --               --
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

// NOTE: [$OMEM] DN_OSMem //////////////////////////////////////////////////////////////////////////
enum DN_OSMemCommit
{
    DN_OSMemCommit_No,
    DN_OSMemCommit_Yes,
};

enum DN_OSMemPage
{
    // Exception on read/write with a page. This flag overrides the read/write 
    // access.
    DN_OSMemPage_NoAccess  = 1 << 0,

    DN_OSMemPage_Read      = 1 << 1, // Only read permitted on the page.

    // Only write permitted on the page. On Windows this is not supported and
    // will be promoted to read+write permissions.
    DN_OSMemPage_Write     = 1 << 2,

    DN_OSMemPage_ReadWrite = DN_OSMemPage_Read | DN_OSMemPage_Write,

    // Modifier used in conjunction with previous flags. Raises exception on
    // first access to the page, then, the underlying protection flags are
    // active. This is supported on Windows, on other OS's using this flag will
    // set the OS equivalent of DN_OSMemPage_NoAccess.
    // This flag must only be used in DN_OSMem_Protect
    DN_OSMemPage_Guard     = 1 << 3,

    // If leak tracing is enabled, this flag will allow the allocation recorded
    // from the reserve call to be leaked, e.g. not printed when leaks are
    // dumped to the console.
    DN_OSMemPage_AllocRecordLeakPermitted = 1 << 4,

    // If leak tracing is enabled this flag will prevent any allocation record
    // from being created in the allocation table at all. If this flag is
    // enabled, 'OSMemPage_AllocRecordLeakPermitted' has no effect since the
    // record will never be created.
    DN_OSMemPage_NoAllocRecordEntry = 1 << 5,

    // [INTERNAL] Do not use. All flags together do not constitute a correct
    // configuration of pages.
    DN_OSMemPage_All                = DN_OSMemPage_NoAccess |
                                      DN_OSMemPage_ReadWrite |
                                      DN_OSMemPage_Guard |
                                      DN_OSMemPage_AllocRecordLeakPermitted |
                                      DN_OSMemPage_NoAllocRecordEntry,
};

// NOTE: [$DATE] DN_OSDate ////////////////////////////////////////////////////////////////////////
struct DN_OSDateTimeStr8
{
    char    date[DN_ARRAY_UCOUNT("YYYY-MM-SS")];
    uint8_t date_size;
    char    hms[DN_ARRAY_UCOUNT("HH:MM:SS")];
    uint8_t hms_size;
};

struct DN_OSDateTime
{
    uint8_t  day;
    uint8_t  month;
    uint16_t year;
    uint8_t  hour;
    uint8_t  minutes;
    uint8_t  seconds;
};

struct DN_OSTimer /// Record time between two time-points using the OS's performance counter.
{
    uint64_t start;
    uint64_t end;
};

#if !defined(DN_NO_OS_FILE_API)
// NOTE: [$FSYS] DN_OSFile ////////////////////////////////////////////////////////////////////////
enum DN_OSPathInfoType
{
    DN_OSPathInfoType_Unknown,
    DN_OSPathInfoType_Directory,
    DN_OSPathInfoType_File,
};

struct DN_OSPathInfo
{
    bool               exists;
    DN_OSPathInfoType type;
    uint64_t           create_time_in_s;
    uint64_t           last_write_time_in_s;
    uint64_t           last_access_time_in_s;
    uint64_t           size;
};

struct DN_OSDirIterator
{
    void    *handle;
    DN_Str8 file_name;
    char     buffer[512];
};

// NOTE: R/W Stream API ////////////////////////////////////////////////////////////////////////////
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

typedef uint32_t DN_OSFileAccess;
enum DN_OSFileAccess_
{
    DN_OSFileAccess_Read       = 1 << 0,
    DN_OSFileAccess_Write      = 1 << 1,
    DN_OSFileAccess_Execute    = 1 << 2,
    DN_OSFileAccess_AppendOnly = 1 << 3, // This flag cannot be combined with any other access mode
    DN_OSFileAccess_ReadWrite  = DN_OSFileAccess_Read      | DN_OSFileAccess_Write,
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
    bool            has_prefix_path_separator;
    DN_OSPathLink *head;
    DN_OSPathLink *tail;
    DN_USize       string_size;
    uint16_t        links_size;
};

// NOTE: [$EXEC] DN_OSExec ////////////////////////////////////////////////////////////////////////
typedef uint32_t DN_OSExecFlags;
enum DN_OSExecFlags_
{
    DN_OSExecFlags_Nil                 = 0,
    DN_OSExecFlags_SaveStdout          = 1 << 0,
    DN_OSExecFlags_SaveStderr          = 1 << 1,
    DN_OSExecFlags_SaveOutput          = DN_OSExecFlags_SaveStdout | DN_OSExecFlags_SaveStderr,
    DN_OSExecFlags_MergeStderrToStdout = 1 << 2                     | DN_OSExecFlags_SaveOutput,
};

struct DN_OSExecAsyncHandle
{
    DN_OSExecFlags exec_flags;
    uint32_t        os_error_code;
    uint32_t        exit_code;
    void           *process;
    void           *stdout_read;
    void           *stdout_write;
    void           *stderr_read;
    void           *stderr_write;
};

struct DN_OSExecResult
{
    bool     finished;
    DN_Str8 stdout_text;
    DN_Str8 stderr_text;
    uint32_t os_error_code;
    uint32_t exit_code;
};

struct DN_OSExecArgs
{
    DN_OSExecFlags     flags;
    DN_Str8            working_dir;
    DN_Slice<DN_Str8> environment;
};

#if !defined(DN_NO_SEMAPHORE)
// NOTE: [$SEMA] DN_OSSemaphore ///////////////////////////////////////////////////////////////////
uint32_t const DN_OS_SEMAPHORE_INFINITE_TIMEOUT = UINT32_MAX;

struct DN_OSSemaphore
{
    #if defined(DN_OS_WIN32) && !defined(DN_OS_WIN32_USE_PTHREADS)
    void *win32_handle;
    #else
    sem_t posix_handle;
    bool  posix_init;
    #endif
};

enum DN_OSSemaphoreWaitResult
{
    DN_OSSemaphoreWaitResult_Failed,
    DN_OSSemaphoreWaitResult_Success,
    DN_OSSemaphoreWaitResult_Timeout,
};
#endif // !defined(DN_NO_SEMAPHORE)

// NOTE: [$THRD] DN_OSThread /////////////////////////////////////////////////////////////////////
#if !defined(DN_NO_THREAD) && !defined(DN_NO_SEMAPHORE)
typedef int32_t (DN_OSThreadFunc)(struct DN_OSThread*);

struct DN_OSThread
{
    DN_FStr8<64>     name;
    DN_TLS           tls;
    void             *handle;
    uint64_t          thread_id;
    void             *user_context;
    DN_OSThreadFunc *func;
    DN_OSSemaphore   init_semaphore;
};
#endif // !defined(DN_NO_THREAD)

// NOTE: [$HTTP] DN_OSHttp ////////////////////////////////////////////////////////////////////////
enum DN_OSHttpRequestSecure
{
    DN_OSHttpRequestSecure_No,
    DN_OSHttpRequestSecure_Yes,
};

struct DN_OSHttpResponse
{
    // NOTE: Response data
    uint32_t   error_code;
    DN_Str8   error_msg;
    uint16_t   http_status;
    DN_Str8   body;
    DN_B32    done;

    // NOTE: Book-keeping
    DN_Arena *arena; // Allocates memory for the response

    // NOTE: Async book-keeping
    // Synchronous HTTP response uses the TLS scratch arena whereas async
    // calls use their own dedicated arena.
    DN_Arena         tmp_arena;
    DN_Arena        *tmem_arena;
    DN_Str8Builder   builder;
    DN_OSSemaphore   on_complete_semaphore;

    #if defined(DN_PLATFORM_EMSCRIPTEN)
    emscripten_fetch_t  *em_handle;
    #elif defined(DN_OS_WIN32)
    HINTERNET            win32_request_session;
    HINTERNET            win32_request_connection;
    HINTERNET            win32_request_handle;
    #endif
};

DN_API void                      DN_OS_Init();

// NOTE: [$OMEM] Memory //////////////////////////////////////////////////////////////////////////
DN_API void *                    DN_OS_MemReserve (DN_USize size, DN_OSMemCommit commit, uint32_t page_flags);
DN_API bool                      DN_OS_MemCommit  (void *ptr, DN_USize size, uint32_t page_flags);
DN_API void                      DN_OS_MemDecommit(void *ptr, DN_USize size);
DN_API void                      DN_OS_MemRelease (void *ptr, DN_USize size);
DN_API int                       DN_OS_MemProtect (void *ptr, DN_USize size, uint32_t page_flags);

// NOTE: Heap
DN_API void                     *DN_OS_MemAlloc   (DN_USize size, DN_ZeroMem zero_mem);
DN_API void                      DN_OS_MemDealloc (void *ptr);

// NOTE: [$DATE] Date //////////////////////////////////////////////////////////////////////////////
DN_API DN_OSDateTime             DN_OS_DateLocalTimeNow    ();
DN_API DN_OSDateTimeStr8         DN_OS_DateLocalTimeStr8Now(char date_separator = '-', char hms_separator = ':');
DN_API DN_OSDateTimeStr8         DN_OS_DateLocalTimeStr8   (DN_OSDateTime time, char date_separator = '-', char hms_separator = ':');
DN_API uint64_t                  DN_OS_DateUnixTimeNs      ();
DN_API uint64_t                  DN_OS_DateUnixTimeS       ();
DN_API DN_OSDateTime             DN_OS_DateUnixTimeSToDate (uint64_t time);
DN_API uint64_t                  DN_OS_DateLocalToUnixTimeS(DN_OSDateTime date);
DN_API uint64_t                  DN_OS_DateToUnixTimeS     (DN_OSDateTime date);
DN_API bool                      DN_OS_DateIsValid         (DN_OSDateTime date);

// NOTE: Other /////////////////////////////////////////////////////////////////////////////////////
DN_API bool                      DN_OS_SecureRNGBytes      (void *buffer, uint32_t size);
DN_API bool                      DN_OS_SetEnvVar           (DN_Str8 name, DN_Str8 value);
DN_API DN_Str8                   DN_OS_EXEPath             (DN_Arena *arena);
DN_API DN_Str8                   DN_OS_EXEDir              (DN_Arena *arena);
#define                          DN_OS_EXEDir_TLS()        DN_OS_EXEDir(DN_TLS_TopArena())
DN_API void                      DN_OS_SleepMs             (DN_UInt milliseconds);

// NOTE: Counters //////////////////////////////////////////////////////////////////////////////////
DN_API uint64_t                  DN_OS_PerfCounterNow      ();
DN_API uint64_t                  DN_OS_PerfCounterFrequency();
DN_API DN_F64                    DN_OS_PerfCounterS        (uint64_t begin, uint64_t end);
DN_API DN_F64                    DN_OS_PerfCounterMs       (uint64_t begin, uint64_t end);
DN_API DN_F64                    DN_OS_PerfCounterUs       (uint64_t begin, uint64_t end);
DN_API DN_F64                    DN_OS_PerfCounterNs       (uint64_t begin, uint64_t end);
DN_API DN_OSTimer                DN_OS_TimerBegin          ();
DN_API void                      DN_OS_TimerEnd            (DN_OSTimer *timer);
DN_API DN_F64                    DN_OS_TimerS              (DN_OSTimer timer);
DN_API DN_F64                    DN_OS_TimerMs             (DN_OSTimer timer);
DN_API DN_F64                    DN_OS_TimerUs             (DN_OSTimer timer);
DN_API DN_F64                    DN_OS_TimerNs             (DN_OSTimer timer);
DN_API uint64_t                  DN_OS_EstimateTSCPerSecond(uint64_t duration_ms_to_gauge_tsc_frequency);
#if !defined(DN_NO_OS_FILE_API)
// NOTE: File system paths /////////////////////////////////////////////////////////////////////////
DN_API DN_OSPathInfo             DN_OS_PathInfo       (DN_Str8 path);
DN_API bool                      DN_OS_FileIsOlderThan(DN_Str8 file, DN_Str8 check_against);
DN_API bool                      DN_OS_PathDelete     (DN_Str8 path);
DN_API bool                      DN_OS_FileExists     (DN_Str8 path);
DN_API bool                      DN_OS_CopyFile       (DN_Str8 src, DN_Str8 dest, bool overwrite, DN_ErrSink *err);
DN_API bool                      DN_OS_MoveFile       (DN_Str8 src, DN_Str8 dest, bool overwrite, DN_ErrSink *err);
DN_API bool                      DN_OS_MakeDir        (DN_Str8 path);
DN_API bool                      DN_OS_DirExists      (DN_Str8 path);
DN_API bool                      DN_OS_DirIterate     (DN_Str8 path, DN_OSDirIterator *it);

// NOTE: R/W Stream API ////////////////////////////////////////////////////////////////////////////
DN_API DN_OSFile                 DN_OS_FileOpen    (DN_Str8 path, DN_OSFileOpen open_mode, DN_OSFileAccess access, DN_ErrSink *err);
DN_API bool                      DN_OS_FileRead    (DN_OSFile *file, void *buffer, DN_USize size, DN_ErrSink *err);
DN_API bool                      DN_OS_FileWritePtr(DN_OSFile *file, void const *data, DN_USize size, DN_ErrSink *err);
DN_API bool                      DN_OS_FileWrite   (DN_OSFile *file, DN_Str8 buffer, DN_ErrSink *err);
DN_API bool                      DN_OS_FileWriteFV (DN_OSFile *file, DN_ErrSink *err, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API bool                      DN_OS_FileWriteF  (DN_OSFile *file, DN_ErrSink *err, DN_FMT_ATTRIB char const *fmt, ...);
DN_API bool                      DN_OS_FileFlush   (DN_OSFile *file, DN_ErrSink *err);
DN_API void                      DN_OS_FileClose   (DN_OSFile *file);

// NOTE: R/W Entire File ///////////////////////////////////////////////////////////////////////////
DN_API DN_Str8                   DN_OS_ReadAll          (DN_Arena *arena, DN_Str8 path, DN_ErrSink *err);
#define                          DN_OS_ReadAll_TLS(...) DN_OS_ReadAll(DN_TLS_TopArena(), ##__VA_ARGS__)
DN_API bool                      DN_OS_WriteAll         (DN_Str8 path, DN_Str8 buffer, DN_ErrSink *err);
DN_API bool                      DN_OS_WriteAllFV       (DN_Str8 path, DN_ErrSink *err, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API bool                      DN_OS_WriteAllF        (DN_Str8 path, DN_ErrSink *err, DN_FMT_ATTRIB char const *fmt, ...);
DN_API bool                      DN_OS_WriteAllSafe     (DN_Str8 path, DN_Str8 buffer, DN_ErrSink *err);
DN_API bool                      DN_OS_WriteAllSafeFV   (DN_Str8 path, DN_ErrSink *err, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API bool                      DN_OS_WriteAllSafeF    (DN_Str8 path, DN_ErrSink *err, DN_FMT_ATTRIB char const *fmt, ...);
#endif // !defined(DN_NO_OS_FILE_API)

// NOTE: File system paths /////////////////////////////////////////////////////////////////////////
DN_API bool                      DN_OS_PathAddRef                             (DN_Arena *arena, DN_OSPath *fs_path, DN_Str8 path);
#define                          DN_OS_PathAddRef_TLS(...)                    DN_OS_PathAddRef(DN_TLS_TopArena(), ##__VA_ARGS__)
#define                          DN_OS_PathAddRef_Frame(...)                  DN_OS_PathAddRef(DN_TLS_FrameArena(), ##__VA_ARGS__)
DN_API bool                      DN_OS_PathAdd                                (DN_Arena *arena, DN_OSPath *fs_path, DN_Str8 path);
#define                          DN_OS_PathAdd_TLS(...)                       DN_OS_PathAdd(DN_TLS_TopArena(), ##__VA_ARGS__)
#define                          DN_OS_PathAdd_Frame(...)                     DN_OS_PathAdd(DN_TLS_FrameArena(), ##__VA_ARGS__)
DN_API bool                      DN_OS_PathAddF                               (DN_Arena *arena, DN_OSPath *fs_path, DN_FMT_ATTRIB char const *fmt, ...);
#define                          DN_OS_PathAddF_TLS(...)                      DN_OS_PathAddF(DN_TLS_TopArena(), ##__VA_ARGS__)
#define                          DN_OS_PathAddF_Frame(...)                    DN_OS_PathAddF(DN_TLS_FrameArena(), ##__VA_ARGS__)
DN_API bool                      DN_OS_PathPop                                (DN_OSPath *fs_path);
DN_API DN_Str8                   DN_OS_PathBuildWithSeparator                 (DN_Arena *arena, DN_OSPath const *fs_path, DN_Str8 path_separator);
#define                          DN_OS_PathBuildWithSeperator_TLS(...)        DN_OS_PathBuildWithSeperator(DN_TLS_TopArena(), ##__VA_ARGS__)
#define                          DN_OS_PathBuildWithSeperator_Frame(...)      DN_OS_PathBuildWithSeperator(DN_TLS_FrameArena(), ##__VA_ARGS__)
DN_API DN_Str8                   DN_OS_PathTo                                 (DN_Arena *arena, DN_Str8 path, DN_Str8 path_separtor);
#define                          DN_OS_PathTo_TLS(...)                        DN_OS_PathTo(DN_TLS_TopArena(), ##__VA_ARGS__)
#define                          DN_OS_PathTo_Frame(...)                      DN_OS_PathTo(DN_TLS_FrameArena(), ##__VA_ARGS__)
DN_API DN_Str8                   DN_OS_PathToF                                (DN_Arena *arena, DN_Str8 path_separator, DN_FMT_ATTRIB char const *fmt, ...);
#define                          DN_OS_PathToF_TLS(...)                       DN_OS_PathToF(DN_TLS_TopArena(), ##__VA_ARGS__)
#define                          DN_OS_PathToF_Frame(...)                     DN_OS_PathToF(DN_TLS_FrameArena(), ##__VA_ARGS__)
DN_API DN_Str8                   DN_OS_Path                                   (DN_Arena *arena, DN_Str8 path);
#define                          DN_OS_Path_TLS(...)                          DN_OS_Path(DN_TLS_TopArena(), ##__VA_ARGS__)
#define                          DN_OS_Path_Frame(...)                        DN_OS_Path(DN_TLS_FrameArena(), ##__VA_ARGS__)
DN_API DN_Str8                   DN_OS_PathF                                  (DN_Arena *arena, DN_FMT_ATTRIB char const *fmt, ...);
#define                          DN_OS_PathF_TLS(...)                         DN_OS_PathF(DN_TLS_TopArena(), ##__VA_ARGS__)
#define                          DN_OS_PathF_Frame(...)                       DN_OS_PathF(DN_TLS_FrameArena(), ##__VA_ARGS__)

#define                          DN_OS_PathBuildFwdSlash(allocator, fs_path)  DN_OS_PathBuildWithSeparator(allocator, fs_path, DN_STR8("/"))
#define                          DN_OS_PathBuildBackSlash(allocator, fs_path) DN_OS_PathBuildWithSeparator(allocator, fs_path, DN_STR8("\\"))
#define                          DN_OS_PathBuild(allocator, fs_path)          DN_OS_PathBuildWithSeparator(allocator, fs_path, DN_OSPathSeparatorString)

// NOTE: [$EXEC] DN_OSExec ////////////////////////////////////////////////////////////////////////
DN_API void                      DN_OS_Exit                 (int32_t exit_code);
DN_API DN_OSExecResult           DN_OS_ExecPump             (DN_OSExecAsyncHandle handle, char *stdout_buffer, size_t *stdout_size, char *stderr_buffer, size_t *stderr_size, uint32_t timeout_ms, DN_ErrSink  *err);
DN_API DN_OSExecResult           DN_OS_ExecWait             (DN_OSExecAsyncHandle handle, DN_Arena *arena, DN_ErrSink *err);
DN_API DN_OSExecAsyncHandle      DN_OS_ExecAsync            (DN_Slice<DN_Str8> cmd_line, DN_OSExecArgs *args, DN_ErrSink *err);
DN_API DN_OSExecResult           DN_OS_Exec                 (DN_Slice<DN_Str8> cmd_line, DN_OSExecArgs *args, DN_Arena *arena, DN_ErrSink *err);
DN_API DN_OSExecResult           DN_OS_ExecOrAbort          (DN_Slice<DN_Str8> cmd_line, DN_OSExecArgs *args, DN_Arena *arena);
#define                          DN_OS_ExecOrAbort_TLS(...) DN_OS_ExecOrAbort(__VA_ARGS__, DN_TLS_TopArena())

// NOTE: [$SEMA] DN_OSSemaphore ///////////////////////////////////////////////////////////////////
#if !defined(DN_NO_SEMAPHORE)
DN_API DN_OSSemaphore            DN_OS_SemaphoreInit     (uint32_t initial_count);
DN_API bool                      DN_OS_SemaphoreIsValid  (DN_OSSemaphore *semaphore);
DN_API void                      DN_OS_SemaphoreDeinit   (DN_OSSemaphore *semaphore);
DN_API void                      DN_OS_SemaphoreIncrement(DN_OSSemaphore *semaphore, uint32_t amount);
DN_API DN_OSSemaphoreWaitResult  DN_OS_SemaphoreWait     (DN_OSSemaphore *semaphore, uint32_t timeout_ms);
#endif // !defined(DN_NO_SEMAPHORE)

// NOTE: [$MUTX] DN_OSMutex ///////////////////////////////////////////////////////////////////////
DN_API DN_OSMutex                DN_OS_MutexInit  ();
DN_API void                      DN_OS_MutexDeinit(DN_OSMutex *mutex);
DN_API void                      DN_OS_MutexLock  (DN_OSMutex *mutex);
DN_API void                      DN_OS_MutexUnlock(DN_OSMutex *mutex);
#define DN_OS_Mutex(mutex) DN_DEFER_LOOP(DN_OS_MutexLock(mutex), DN_OS_MutexUnlock(mutex))

// NOTE: [$THRD] DN_OSThread /////////////////////////////////////////////////////////////////////
#if !defined(DN_NO_THREAD) && !defined(DN_NO_SEMAPHORE)
DN_API bool                      DN_OS_ThreadInit  (DN_OSThread *thread, DN_OSThreadFunc *func, void *user_context);
DN_API void                      DN_OS_ThreadDeinit(DN_OSThread *thread);
DN_API uint32_t                  DN_OS_ThreadID    ();
DN_API void                      DN_OS_ThreadSetTLS(DN_TLS *tls);
DN_API void                      DN_OS_ThreadSetName(DN_Str8 name);
#endif // !defined(DN_NO_THREAD)

// NOTE: [$HTTP] DN_OSHttp ////////////////////////////////////////////////////////////////////////
DN_API void                      DN_OS_HttpRequestAsync(DN_OSHttpResponse *response, DN_Arena *arena, DN_Str8 host, DN_Str8 path, DN_OSHttpRequestSecure secure, DN_Str8 method, DN_Str8 body, DN_Str8 headers);
DN_API void                      DN_OS_HttpRequestWait (DN_OSHttpResponse *response);
DN_API void                      DN_OS_HttpRequestFree (DN_OSHttpResponse *response);
DN_API DN_OSHttpResponse         DN_OS_HttpRequest     (DN_Arena *arena, DN_Str8 host, DN_Str8 path, DN_OSHttpRequestSecure secure, DN_Str8 method, DN_Str8 body, DN_Str8 headers);
