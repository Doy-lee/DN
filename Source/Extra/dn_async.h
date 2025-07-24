#if !defined(DN_ASYNC_H)
#define DN_ASYNC_H

#include "../dn_base_inc.h"
#include "../dn_os_inc.h"

enum DN_ASYNCPriority
{
  DN_ASYNCPriority_Low,
  DN_ASYNCPriority_High,
  DN_ASYNCPriority_Count,
};

struct DN_ASYNCCore
{
  DN_OSMutex             ring_mutex;
  DN_OSConditionVariable ring_write_cv;
  DN_OSSemaphore         worker_sem;
  DN_Ring                ring;
  DN_OSThread           *threads;
  DN_U32                 thread_count;
  DN_U32                 busy_threads;
  DN_U32                 join_threads;
};

struct DN_ASYNCWorkArgs
{
  DN_OSThread *thread;
  void        *input;
};

typedef void(DN_ASYNCWorkFunc)(DN_ASYNCWorkArgs work_args);

struct DN_ASYNCWork
{
  DN_ASYNCWorkFunc *func;
  void             *input;
  void             *output;
};

struct DN_ASYNCTask
{
  bool           queued;
  DN_ASYNCWork   work;
  DN_OSSemaphore completion_sem;
};

DN_API void         DN_ASYNC_Init     (DN_ASYNCCore *async, char *base, DN_USize base_size, DN_OSThread *threads, DN_U32 threads_size);
DN_API void         DN_ASYNC_Deinit   (DN_ASYNCCore *async);
DN_API bool         DN_ASYNC_QueueWork(DN_ASYNCCore *async, DN_ASYNCWorkFunc *func, void *input, DN_U64 wait_time_ms);
DN_API DN_ASYNCTask DN_ASYNC_QueueTask(DN_ASYNCCore *async, DN_ASYNCWorkFunc *func, void *input, DN_U64 wait_time_ms);
DN_API void         DN_ASYNC_WaitTask (DN_OSSemaphore *sem, DN_U32 timeout_ms);

#endif // DN_ASYNC_H
