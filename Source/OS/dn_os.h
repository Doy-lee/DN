#if !defined(DN_OS_H)
#define DN_OS_H

#if defined(_CLANGD)
  #define DN_H_WITH_OS 1
  #include "../dn.h"
#endif

#include <new> // operator new

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

extern DN_CPUFeatureDecl g_dn_cpu_feature_decl[DN_CPUFeature_Count];

struct DN_OSTimer /// Record time between two time-points using the OS's performance counter.
{
  DN_U64 start;
  DN_U64 end;
};

// NOTE: DN_OSFile
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

// NOTE: R/W Stream API
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

// NOTE: DN_OSPath
#if !defined(DN_OSPathSeperator)
  #if defined(DN_OS_WIN32)
    #define DN_OSPathSeperator "\\"
  #else
    #define DN_OSPathSeperator "/"
  #endif
  #define DN_OSPathSeperatorString DN_Str8Lit(DN_OSPathSeperator)
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
  DN_U16         links_size;
};

// NOTE: DN_OSExec
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
  DN_OSExecFlags flags;
  DN_Str8        working_dir;
  DN_Str8Slice   environment;
};

// NOTE: DN_OSSemaphore
DN_U32 const DN_OS_SEMAPHORE_INFINITE_TIMEOUT = UINT32_MAX;

struct DN_OSSemaphore
{
  DN_U64 handle;
};

struct DN_OSBarrier
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

// NOTE: DN_OSThread
typedef DN_I32(DN_OSThreadFunc)(struct DN_OSThread *);

struct DN_OSThreadLane
{
  DN_USize     index;
  DN_USize     count;
  DN_OSBarrier barrier;
  void*        shared_mem;
};

struct DN_OSThreadLaneway
{
  DN_OSThread* threads;
  DN_USize     threads_count;
  DN_UPtr*     shared_mem;
  DN_OSBarrier barrier;
};

struct DN_OSThread
{
  DN_Str8x64       name;
  DN_TCCore        context;
  DN_OSThreadLane  lane;
  bool             is_lane_set;
  void            *handle;
  DN_U64           thread_id;
  void            *user_context;
  DN_OSThreadFunc *func;
  DN_OSSemaphore   init_semaphore;
  DN_TCInitArgs    tc_init_args;
};

// NOTE: DN_OSHttp
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
  DN_Arena       scratch_arena;
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

struct DN_OSCore
{
  DN_CPUReport           cpu_report;

  // NOTE: Logging
  bool                   log_to_file;              // Output logs to file as well as standard out
  DN_OSFile              log_file;                 // TODO(dn): Hmmm, how should we do this... ?
  DN_TicketMutex         log_file_mutex;           // Is locked when instantiating the log_file for the first time
  bool                   log_no_colour;            // Disable colours in the logging output
  DN_TicketMutex         log_mutex;

  // NOTE: OS
  DN_U32                 logical_processor_count;
  DN_U32                 page_size;
  DN_U32                 alloc_granularity;

  // NOTE: Memory
  // Total OS mem allocs in lifetime of program (e.g. malloc, VirtualAlloc, HeapAlloc ...). This
  // only includes allocations routed through the library such as the growing nature of arenas or
  // using the memory allocation routines in the library like DN_OS_MemCommit and so forth.
  DN_U64                 vmem_allocs_total;
  DN_U64                 vmem_allocs_frame;        // Total OS virtual memory allocs since the last 'DN_Core_FrameBegin' was invoked
  DN_U64                 mem_allocs_total;
  DN_U64                 mem_allocs_frame;         // Total OS heap allocs since the last 'DN_Core_FrameBegin' was invoked

  DN_MemList             mem;
  DN_Arena               arena;
  void                  *platform_context;
};

struct DN_OSDiskSpace
{
  bool   success;
  DN_U64 avail;
  DN_U64 size;
};

DN_API DN_MemFuncs               DN_MemFuncsFromType                          (DN_MemFuncsType type);
DN_API DN_MemFuncs               DN_MemFuncsDefault                           ();
DN_API DN_MemList                DN_MemListFromHeap                           (DN_U64 size, DN_MemFlags flags);
DN_API DN_MemList                DN_MemListFromVMem                           (DN_U64 reserve, DN_U64 commit, DN_MemFlags flags);
DN_API DN_Arena                  DN_ArenaFromHeap                             (DN_U64 wize, DN_MemFlags flags);
DN_API DN_Arena                  DN_ArenaFromVMem                             (DN_U64 reserve, DN_U64 commit, DN_MemFlags flags);

DN_API DN_Str8                   DN_Str8FromHeapF                             (DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_Str8                   DN_Str8FromHeap                              (DN_USize size, DN_ZMem z_mem);
DN_API DN_Str8                   DN_Str8BuilderBuildFromHeap                  (DN_Str8Builder const *builder);

DN_API void                      DN_OS_LogPrint                               (DN_LogTypeParam type, void *user_data, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API void                      DN_OS_SetLogPrintFuncToOS                    ();

DN_API void *                    DN_OS_MemReserve                             (DN_USize size, DN_MemCommit commit, DN_MemPage page_flags);
DN_API bool                      DN_OS_MemCommit                              (void *ptr, DN_USize size, DN_U32 page_flags);
DN_API void                      DN_OS_MemDecommit                            (void *ptr, DN_USize size);
DN_API void                      DN_OS_MemRelease                             (void *ptr, DN_USize size);
DN_API int                       DN_OS_MemProtect                             (void *ptr, DN_USize size, DN_U32 page_flags);

DN_API void *                    DN_OS_MemAlloc                               (DN_USize size, DN_ZMem z_mem);
DN_API void                      DN_OS_MemDealloc                             (void *ptr);

DN_API DN_Date                   DN_OS_DateLocalTimeNow                       ();
DN_API DN_Str8x32                DN_OS_DateLocalTimeStr8Now                   (char date_separator = '-', char hms_separator = ':');
DN_API DN_Str8x32                DN_OS_DateLocalTimeStr8                      (DN_Date time, char date_separator = '-', char hms_separator = ':');
DN_API DN_U64                    DN_OS_DateUnixTimeNs                         ();
#define                          DN_OS_DateUnixTimeUs()                       (DN_OS_DateUnixTimeNs() / 1000)
#define                          DN_OS_DateUnixTimeMs()                       (DN_OS_DateUnixTimeNs() / (1000 * 1000))
#define                          DN_OS_DateUnixTimeS()                        (DN_OS_DateUnixTimeNs() / (1000 * 1000 * 1000))
DN_API DN_U64                    DN_OS_DateUnixTimeSFromLocalDate             (DN_Date date);
DN_API DN_U64                    DN_OS_DateLocalUnixTimeSFromUnixTimeS        (DN_U64 unix_ts_s);

DN_API void                      DN_OS_GenBytesSecure                         (void *buffer, DN_U32 size);
DN_API bool                      DN_OS_SetEnvVar                              (DN_Str8 name, DN_Str8 value);
DN_API DN_OSDiskSpace            DN_OS_DiskSpace                              (DN_Str8 path);
DN_API DN_Str8                   DN_OS_EXEPath                                (DN_Arena *arena);
DN_API DN_Str8                   DN_OS_EXEDir                                 (DN_Arena *arena);
DN_API void                      DN_OS_SleepMs                                (DN_UInt milliseconds);

DN_API DN_U64                    DN_OS_PerfCounterNow                         ();
DN_API DN_U64                    DN_OS_PerfCounterFrequency                   ();
DN_API DN_F64                    DN_OS_PerfCounterS                           (DN_U64 begin, uint64_t end);
DN_API DN_F64                    DN_OS_PerfCounterMs                          (DN_U64 begin, uint64_t end);
DN_API DN_F64                    DN_OS_PerfCounterUs                          (DN_U64 begin, uint64_t end);
DN_API DN_F64                    DN_OS_PerfCounterNs                          (DN_U64 begin, uint64_t end);
DN_API DN_OSTimer                DN_OS_TimerBegin                             ();
DN_API void                      DN_OS_TimerEnd                               (DN_OSTimer *timer);
DN_API DN_F64                    DN_OS_TimerS                                 (DN_OSTimer timer);
DN_API DN_F64                    DN_OS_TimerMs                                (DN_OSTimer timer);
DN_API DN_F64                    DN_OS_TimerUs                                (DN_OSTimer timer);
DN_API DN_F64                    DN_OS_TimerNs                                (DN_OSTimer timer);
DN_API DN_U64                    DN_OS_EstimateTSCPerSecond                   (uint64_t duration_ms_to_gauge_tsc_frequency);

DN_API bool                      DN_OS_FileCopy                               (DN_Str8 src, DN_Str8 dest, bool overwrite, DN_ErrSink *err);
DN_API bool                      DN_OS_FileMove                               (DN_Str8 src, DN_Str8 dest, bool overwrite, DN_ErrSink *err);

DN_API DN_OSFile                 DN_OS_FileOpen                               (DN_Str8 path, DN_OSFileOpen open_mode, DN_OSFileAccess access, DN_ErrSink *err);
DN_API DN_OSFileRead             DN_OS_FileRead                               (DN_OSFile *file, void *buffer, DN_USize size, DN_ErrSink *err);
DN_API bool                      DN_OS_FileWritePtr                           (DN_OSFile *file, void const *data, DN_USize size, DN_ErrSink *err);
DN_API bool                      DN_OS_FileWrite                              (DN_OSFile *file, DN_Str8 buffer, DN_ErrSink *err);
DN_API bool                      DN_OS_FileWriteFV                            (DN_OSFile *file, DN_ErrSink *err, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API bool                      DN_OS_FileWriteF                             (DN_OSFile *file, DN_ErrSink *err, DN_FMT_ATTRIB char const *fmt, ...);
DN_API bool                      DN_OS_FileFlush                              (DN_OSFile *file, DN_ErrSink *err);
DN_API void                      DN_OS_FileClose                              (DN_OSFile *file);

DN_API DN_Str8                   DN_OS_FileReadAll                            (DN_Allocator alloc_type, void *allocator, DN_Str8 path, DN_ErrSink *err);
DN_API DN_Str8                   DN_OS_FileReadAllArena                       (DN_Arena *arena, DN_Str8 path, DN_ErrSink *err);
DN_API DN_Str8                   DN_OS_FileReadAllPool                        (DN_Pool *pool, DN_Str8 path, DN_ErrSink *err);

DN_API bool                      DN_OS_FileWriteAll                           (DN_Str8 path, DN_Str8 buffer, DN_ErrSink *err);
DN_API bool                      DN_OS_FileWriteAllFV                         (DN_Str8 path, DN_ErrSink *err, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API bool                      DN_OS_FileWriteAllF                          (DN_Str8 path, DN_ErrSink *err, DN_FMT_ATTRIB char const *fmt, ...);
DN_API bool                      DN_OS_FileWriteAllSafe                       (DN_Str8 path, DN_Str8 buffer, DN_ErrSink *err);
DN_API bool                      DN_OS_FileWriteAllSafeFV                     (DN_Str8 path, DN_ErrSink *err, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API bool                      DN_OS_FileWriteAllSafeF                      (DN_Str8 path, DN_ErrSink *err, DN_FMT_ATTRIB char const *fmt, ...);

DN_API DN_Str8                   DN_OS_Str8FromPathInfoType                   (DN_OSPathInfoType type);
DN_API DN_OSPathInfo             DN_OS_PathInfo                               (DN_Str8 path);
DN_API bool                      DN_OS_PathIsOlderThan                        (DN_Str8 file, DN_Str8 check_against);
DN_API bool                      DN_OS_PathDelete                             (DN_Str8 path);
DN_API bool                      DN_OS_PathIsFile                             (DN_Str8 path);
DN_API bool                      DN_OS_PathIsDir                              (DN_Str8 path);
DN_API bool                      DN_OS_PathMakeDir                            (DN_Str8 path);
DN_API bool                      DN_OS_PathIterateDir                         (DN_Str8 path, DN_OSDirIterator *it);

DN_API bool                      DN_OS_PathAddRef                             (DN_Arena *arena, DN_OSPath *fs_path, DN_Str8 path);
DN_API bool                      DN_OS_PathAdd                                (DN_Arena *arena, DN_OSPath *fs_path, DN_Str8 path);
DN_API bool                      DN_OS_PathAddF                               (DN_Arena *arena, DN_OSPath *fs_path, DN_FMT_ATTRIB char const *fmt, ...);
DN_API bool                      DN_OS_PathPop                                (DN_OSPath *fs_path);
DN_API DN_Str8                   DN_OS_PathBuildWithSeparator                 (DN_Arena *arena, DN_OSPath const *fs_path, DN_Str8 path_separator);
DN_API DN_Str8                   DN_OS_PathTo                                 (DN_Arena *arena, DN_Str8 path, DN_Str8 path_separtor);
DN_API DN_Str8                   DN_OS_PathToF                                (DN_Arena *arena, DN_Str8 path_separator, DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_Str8                   DN_OS_Path                                   (DN_Arena *arena, DN_Str8 path);
DN_API DN_Str8                   DN_OS_PathF                                  (DN_Arena *arena, DN_FMT_ATTRIB char const *fmt, ...);

#define                          DN_OS_PathBuildFwdSlash(allocator, fs_path)  DN_OS_PathBuildWithSeparator(allocator, fs_path, DN_Str8Lit("/"))
#define                          DN_OS_PathBuildBackSlash(allocator, fs_path) DN_OS_PathBuildWithSeparator(allocator, fs_path, DN_Str8Lit("\\"))
#define                          DN_OS_PathBuild(allocator, fs_path)          DN_OS_PathBuildWithSeparator(allocator, fs_path, DN_OSPathSeparatorString)

DN_API void                      DN_OS_Exit                                   (int32_t exit_code);
DN_API DN_OSExecResult           DN_OS_ExecPump                               (DN_OSExecAsyncHandle handle, char *stdout_buffer, size_t *stdout_size, char *stderr_buffer, size_t *stderr_size, DN_U32 timeout_ms, DN_ErrSink  *err);
DN_API DN_OSExecResult           DN_OS_ExecWait                               (DN_OSExecAsyncHandle handle, DN_Arena *arena, DN_ErrSink *err);
DN_API DN_OSExecAsyncHandle      DN_OS_ExecAsync                              (DN_Str8Slice cmd_line, DN_OSExecArgs *args, DN_ErrSink *err);
DN_API DN_OSExecResult           DN_OS_Exec                                   (DN_Str8Slice cmd_line, DN_OSExecArgs *args, DN_Arena *arena, DN_ErrSink *err);
DN_API DN_OSExecResult           DN_OS_ExecOrAbort                            (DN_Str8Slice cmd_line, DN_OSExecArgs *args, DN_Arena *arena);

DN_API DN_OSSemaphore            DN_OS_SemaphoreInit                          (DN_U32 initial_count);
DN_API void                      DN_OS_SemaphoreDeinit                        (DN_OSSemaphore *semaphore);
DN_API void                      DN_OS_SemaphoreIncrement                     (DN_OSSemaphore *semaphore, DN_U32 amount);
DN_API DN_OSSemaphoreWaitResult  DN_OS_SemaphoreWait                          (DN_OSSemaphore *semaphore, DN_U32 timeout_ms);

DN_API DN_OSBarrier              DN_OS_BarrierInit                            (DN_U32 thread_count);
DN_API void                      DN_OS_BarrierDeinit                          (DN_OSBarrier *barrier);
DN_API void                      DN_OS_BarrierWait                            (DN_OSBarrier *barrier);

DN_API DN_OSMutex                DN_OS_MutexInit                              ();
DN_API void                      DN_OS_MutexDeinit                            (DN_OSMutex *mutex);
DN_API void                      DN_OS_MutexLock                              (DN_OSMutex *mutex);
DN_API void                      DN_OS_MutexUnlock                            (DN_OSMutex *mutex);
#define                          DN_OS_MutexScope(mutex)                      DN_DeferLoop(DN_OS_MutexLock(mutex), DN_OS_MutexUnlock(mutex))

DN_API DN_OSConditionVariable    DN_OS_ConditionVariableInit                  ();
DN_API void                      DN_OS_ConditionVariableDeinit                (DN_OSConditionVariable *cv);
DN_API bool                      DN_OS_ConditionVariableWait                  (DN_OSConditionVariable *cv, DN_OSMutex *mutex, DN_U64 sleep_ms);
DN_API bool                      DN_OS_ConditionVariableWaitUntil             (DN_OSConditionVariable *cv, DN_OSMutex *mutex, DN_U64 end_ts_ms);
DN_API void                      DN_OS_ConditionVariableSignal                (DN_OSConditionVariable *cv);
DN_API void                      DN_OS_ConditionVariableBroadcast             (DN_OSConditionVariable *cv);

DN_API bool                      DN_OS_ThreadInit                             (DN_OSThread *thread, DN_OSThreadFunc *func, DN_OSThreadLane *lane, DN_TCInitArgs tc_init_args, void *user_context);
DN_API bool                      DN_OS_ThreadJoin                             (DN_OSThread *thread, DN_TCDeinitArenas deinit_arenas);
DN_API DN_U32                    DN_OS_ThreadID                               ();
DN_API void                      DN_OS_ThreadSetNameFmt                       (char const *fmt, ...);

// NOTE: Thread lanes provide an abstraction to represent the concept of programming a CPU like a
// GPU, e.g. SIMT (Single Instruction Multiple Threads). The lane terminology is popularised by Ryan
// Fleury. SIMT is formally defined as
//
//  Threads are grouped into warps/wavefronts (typically 32 or 64 threads) that execute the same
//  instruction in lockstep, but each thread operates on different data and maintains its own state
//
// The individual threads in a wavefront on the CPU side are colloquially dubbed "lanes" and a
// thread lane here contains the necessary state to facilitate this such as the current index in the
// wavefront and synchronisation primitives to coordinate the different lanes together.
//
// The idea is to write code in a single-threaded manner (linear execution) but across multiple
// threads so that the default is all execution paths are inherently multi-threaded by default. Opt
// out of parallelism instead of opt in. This optimises for the trend of core counts increasing
// whilst clock counts remain static.
//
// A laneway is a helper function to initialise the number of requested OS threads/lanes upfront and
// setup the required synchronisation primitives. It can then be dispatched all the threads which
// start executing the `entry_point` in parallel.
//
// API
//   DN_OS_ThreadLaneSync
//     A blocking call to synchronise the program-counter of all other lanes in the laneway to this
//     function call invocation (using an OS barrier). Optionally pass in the pointer to a pointer
//     `ptr_to_share` to broadcast the pointer from one lanes to the others. The lane that wishes
//     to broadcast the pointer must have a non-null pointer, all other lanes must pass in a
//     non-null pointer. A typical use case might look like:
/*
         DN_OSThreadLane *lane = DN_OS_TCThreadLane(); // Get lane from current (t)hread (c)context

         // NOTE: Allocate buffer in lane 0
         DN_U8 *buffer         = nullptr;
         if (lane->index == 0)
           buffer = DN_ArenaNewArray(DN_TCMainArena(), DN_U8, DN_Gigabytes(1), DN_ZMem_No);

         // NOTE: Lane 0 broadcasts the `buffer` pointer to lane 1..N
         DN_OS_ThreadLaneSync(lane, &buffer);

         // NOTE: We use LaneRange to divide the buffer into equal sized chunks that each lane can
         // write into without clobbering over each other.
         DN_V2USize range = DN_OS_ThreadLaneRange(lane, DN_Gigabytes(1));
         for (DN_USize index = range.begin; index < range.end; index++) { buffer[index] = index; }
*/
//     In this example, lane 0 will allocate a 1GiB buffer pass in a `buffer` to
//     DN_OS_ThreadLaneSync` that is non-null. Lanes 1->N will skip the branch (because their lanes
//     indexes are 1..N) and invoke `DN_OS_ThreadLaneSync` with a nullptr `buffer`. After the
//     blocking call is complete, lanes 0->N will now have synchronised the `buffer` pointer and all
//     lanes point to the 1GiB range allocated in lane 0's allocator.
//
//     Additionally we demonstrate `DN_OS_ThreadLaneRange` which does math behind the scenes to
//     divide the buffer up and assign each lane their own indices in the buffer that they can work
//     on in parallel without clobbering each others work.
//
//   DN_OS_ThreadLaneRange
//     Calculates the range of values the current lane in the laneway should execute. For example if
//     you have 128 items and 16 threads each lane will receive the following `DN_V2USize` range:
//       Lane 0  => [0,   8)
//       Lane 1  => [8,   16)
//       ...
//       Lane 16 => [120, 128)
DN_API DN_OSThreadLane           DN_OS_ThreadLaneInit                         (DN_USize index, DN_USize thread_count, DN_OSBarrier barrier, DN_UPtr *share_mem);
DN_API void                      DN_OS_ThreadLaneSync                         (DN_OSThreadLane *lane, void **ptr_to_share);
DN_API DN_V2USize                DN_OS_ThreadLaneRange                        (DN_OSThreadLane const *lane, DN_USize values_count);

DN_API DN_OSThreadLaneway        DN_OS_ThreadLanewayFromArgs                  (DN_OSThread* threads, DN_USize threads_count, DN_UPtr* shared_mem);
DN_API DN_OSThreadLaneway        DN_OS_ThreadLanewayFromArena                 (DN_USize threads_count, DN_Arena* arena);
DN_API void                      DN_OS_ThreadLanewayDispatch                  (DN_OSThreadLaneway *laneway, DN_OSThreadFunc *entry_point, DN_TCInitArgs tc_init_args, void *user_context);
DN_API void                      DN_OS_ThreadLanewayJoin                      (DN_OSThreadLaneway *laneway, DN_TCDeinitArenas deinit_arenas);

DN_API DN_OSThreadLane*          DN_OS_TCThreadLane                           ();
DN_API void                      DN_OS_TCThreadLaneSync                       (void **ptr_to_share);
DN_API DN_OSThreadLane           DN_OS_TCThreadLaneEquip                      (DN_OSThreadLane lane);

DN_API void                      DN_OS_HttpRequestAsync                       (DN_OSHttpResponse *response, DN_Arena *arena, DN_Str8 host, DN_Str8 path, DN_OSHttpRequestSecure secure, DN_Str8 method, DN_Str8 body, DN_Str8 headers);
DN_API void                      DN_OS_HttpRequestWait                        (DN_OSHttpResponse *response);
DN_API void                      DN_OS_HttpRequestFree                        (DN_OSHttpResponse *response);
DN_API DN_OSHttpResponse         DN_OS_HttpRequest                            (DN_Arena *arena, DN_Str8 host, DN_Str8 path, DN_OSHttpRequestSecure secure, DN_Str8 method, DN_Str8 body, DN_Str8 headers);

// NOTE: DN_OSPrint
enum DN_OSPrintDest
{
  DN_OSPrintDest_Out,
  DN_OSPrintDest_Err,
};

// NOTE: Print Macros
#define DN_OS_PrintOut(string)                       DN_OS_Print(DN_OSPrintDest_Out, string)
#define DN_OS_PrintOutF(fmt, ...)                    DN_OS_PrintF(DN_OSPrintDest_Out, fmt, ##__VA_ARGS__)
#define DN_OS_PrintOutFV(fmt, args)                  DN_OS_PrintFV(DN_OSPrintDest_Out, fmt, args)

#define DN_OS_PrintOutStyle(style, string)           DN_OS_PrintStyle(DN_OSPrintDest_Out, style, string)
#define DN_OS_PrintOutFStyle(style, fmt, ...)        DN_OS_PrintFStyle(DN_OSPrintDest_Out, style, fmt, ##__VA_ARGS__)
#define DN_OS_PrintOutFVStyle(style, fmt, args, ...) DN_OS_PrintFVStyle(DN_OSPrintDest_Out, style, fmt, args)

#define DN_OS_PrintOutLn(string)                     DN_OS_PrintLn(DN_OSPrintDest_Out, string)
#define DN_OS_PrintOutLnF(fmt, ...)                  DN_OS_PrintLnF(DN_OSPrintDest_Out, fmt, ##__VA_ARGS__)
#define DN_OS_PrintOutLnFV(fmt, args)                DN_OS_PrintLnFV(DN_OSPrintDest_Out, fmt, args)

#define DN_OS_PrintOutLnStyle(style, string)         DN_OS_PrintLnStyle(DN_OSPrintDest_Out, style, string);
#define DN_OS_PrintOutLnFStyle(style, fmt, ...)      DN_OS_PrintLnFStyle(DN_OSPrintDest_Out, style, fmt, ##__VA_ARGS__)
#define DN_OS_PrintOutLnFVStyle(style, fmt, args)    DN_OS_PrintLnFVStyle(DN_OSPrintDest_Out, style, fmt, args);

#define DN_OS_PrintErr(string)                       DN_OS_Print(DN_OSPrintDest_Err, string)
#define DN_OS_PrintErrF(fmt, ...)                    DN_OS_PrintF(DN_OSPrintDest_Err, fmt, ##__VA_ARGS__)
#define DN_OS_PrintErrFV(fmt, args)                  DN_OS_PrintFV(DN_OSPrintDest_Err, fmt, args)

#define DN_OS_PrintErrStyle(style, string)           DN_OS_PrintStyle(DN_OSPrintDest_Err, style, string)
#define DN_OS_PrintErrFStyle(style, fmt, ...)        DN_OS_PrintFStyle(DN_OSPrintDest_Err, style, fmt, ##__VA_ARGS__)
#define DN_OS_PrintErrFVStyle(style, fmt, args, ...) DN_OS_PrintFVStyle(DN_OSPrintDest_Err, style, fmt, args)

#define DN_OS_PrintErrLn(string)                     DN_OS_PrintLn(DN_OSPrintDest_Err, string)
#define DN_OS_PrintErrLnF(fmt, ...)                  DN_OS_PrintLnF(DN_OSPrintDest_Err, fmt, ##__VA_ARGS__)
#define DN_OS_PrintErrLnFV(fmt, args)                DN_OS_PrintLnFV(DN_OSPrintDest_Err, fmt, args)

#define DN_OS_PrintErrLnStyle(style, string)         DN_OS_PrintLnStyle(DN_OSPrintDest_Err, style, string);
#define DN_OS_PrintErrLnFStyle(style, fmt, ...)      DN_OS_PrintLnFStyle(DN_OSPrintDest_Err, style, fmt, ##__VA_ARGS__)
#define DN_OS_PrintErrLnFVStyle(style, fmt, args)    DN_OS_PrintLnFVStyle(DN_OSPrintDest_Err, style, fmt, args);

// NOTE: Print
DN_API void DN_OS_Print                (DN_OSPrintDest dest, DN_Str8 string);
DN_API void DN_OS_PrintF               (DN_OSPrintDest dest, DN_FMT_ATTRIB char const *fmt, ...);
DN_API void DN_OS_PrintFV              (DN_OSPrintDest dest, DN_FMT_ATTRIB char const *fmt, va_list args);

DN_API void DN_OS_PrintStyle           (DN_OSPrintDest dest, DN_LogStyle style, DN_Str8 string);
DN_API void DN_OS_PrintFStyle          (DN_OSPrintDest dest, DN_LogStyle style, DN_FMT_ATTRIB char const *fmt, ...);
DN_API void DN_OS_PrintFVStyle         (DN_OSPrintDest dest, DN_LogStyle style, DN_FMT_ATTRIB char const *fmt, va_list args);

DN_API void DN_OS_PrintLn              (DN_OSPrintDest dest, DN_Str8 string);
DN_API void DN_OS_PrintLnF             (DN_OSPrintDest dest, DN_FMT_ATTRIB char const *fmt, ...);
DN_API void DN_OS_PrintLnFV            (DN_OSPrintDest dest, DN_FMT_ATTRIB char const *fmt, va_list args);

DN_API void DN_OS_PrintLnStyle         (DN_OSPrintDest dest, DN_LogStyle style, DN_Str8 string);
DN_API void DN_OS_PrintLnFStyle        (DN_OSPrintDest dest, DN_LogStyle style, DN_FMT_ATTRIB char const *fmt, ...);
DN_API void DN_OS_PrintLnFVStyle       (DN_OSPrintDest dest, DN_LogStyle style, DN_FMT_ATTRIB char const *fmt, va_list args);

// NOTE: DN_VArray
// TODO(doyle): Add an API for shrinking the array by decomitting pages back to the OS.
template <typename T> struct DN_VArray
{
  T       *data;   // Pointer to the start of the array items in the block of memory
  DN_USize size;   // Number of items currently in the array
  DN_USize max;    // Maximum number of items this array can store
  DN_USize commit; // Bytes committed

  T       *begin()       { return data; }
  T       *end  ()       { return data + size; }
  T const *begin() const { return data; }
  T const *end  () const { return data + size; }
};

template <typename T>                           DN_VArray<T>          DN_OS_VArrayInitByteSize          (DN_USize byte_size);
template <typename T>                           DN_VArray<T>          DN_OS_VArrayInit                  (DN_USize max);
template <typename T, DN_USize N>               DN_VArray<T>          DN_OS_VArrayInitCArray            (T const (&items)[N], DN_USize max);
template <typename T>                           void                  DN_OS_VArrayDeinit                (DN_VArray<T> *array);
template <typename T>                           bool                  DN_OS_VArrayIsValid               (DN_VArray<T> const *array);
template <typename T>                           bool                  DN_OS_VArrayReserve               (DN_VArray<T> *array, DN_USize count);
template <typename T>                           T *                   DN_OS_VArrayAddArray              (DN_VArray<T> *array, T const *items, DN_USize count);
template <typename T, DN_USize N>               T *                   DN_OS_VArrayAddCArray             (DN_VArray<T> *array, T const (&items)[N]);
template <typename T>                           T *                   DN_OS_VArrayAdd                   (DN_VArray<T> *array, T const &item);
#define                                                               DN_OS_VArrayAddArrayAssert(...)   DN_HardAssert(DN_OS_VArrayAddArray(__VA_ARGS__))
#define                                                               DN_OS_VArrayAddCArrayAssert(...)  DN_HardAssert(DN_OS_VArrayAddCArray(__VA_ARGS__))
#define                                                               DN_OS_VArrayAddAssert(...)        DN_HardAssert(DN_OS_VArrayAdd(__VA_ARGS__))
template <typename T>                           T *                   DN_OS_VArrayMakeArray             (DN_VArray<T> *array, DN_USize count, DN_ZMem z_mem);
template <typename T>                           T *                   DN_OS_VArrayMake                  (DN_VArray<T> *array, DN_ZMem z_mem);
#define                                                               DN_OS_VArrayMakeArrayAssert(...)  DN_HardAssert(DN_OS_VArrayMakeArray(__VA_ARGS__))
#define                                                               DN_OS_VArrayMakeAssert(...)       DN_HardAssert(DN_OS_VArrayMake(__VA_ARGS__))
template <typename T>                           T *                   DN_OS_VArrayInsertArray           (DN_VArray<T> *array, DN_USize index, T const *items, DN_USize count);
template <typename T, DN_USize N>               T *                   DN_OS_VArrayInsertCArray          (DN_VArray<T> *array, DN_USize index, T const (&items)[N]);
template <typename T>                           T *                   DN_OS_VArrayInsert                (DN_VArray<T> *array, DN_USize index, T const &item);
#define                                                               DN_OS_VArrayInsertArrayAssert(...) DN_HardAssert(DN_OS_VArrayInsertArray(__VA_ARGS__))
#define                                                               DN_OS_VArrayInsertCArrayAssert(...) DN_HardAssert(DN_OS_VArrayInsertCArray(__VA_ARGS__))
#define                                                               DN_OS_VArrayInsertAssert(...)     DN_HardAssert(DN_OS_VArrayInsert(__VA_ARGS__))
template <typename T>                           T                     DN_OS_VArrayPopFront              (DN_VArray<T> *array, DN_USize count);
template <typename T>                           T                     DN_OS_VArrayPopBack               (DN_VArray<T> *array, DN_USize count);
template <typename T>                           DN_ArrayEraseResult   DN_OS_VArrayEraseRange            (DN_VArray<T> *array, DN_USize begin_index, DN_ISize count, DN_ArrayErase erase);
template <typename T>                           void                  DN_OS_VArrayClear                 (DN_VArray<T> *array, DN_ZMem z_mem);
#endif // !defined(DN_OS_H)
