#define DN_ASYNC_CPP

#include "../dn_base_inc.h"
#include "../dn_os_inc.h"
#include "dn_async.h"

static DN_I32 DN_ASYNC_ThreadEntryPoint_(DN_OSThread *thread)
{
  DN_OS_ThreadSetName(DN_FStr8_ToStr8(&thread->name));
  DN_ASYNCCore *async = DN_CAST(DN_ASYNCCore *) thread->user_context;
  DN_Ring      *ring  = &async->ring;
  for (;;) {
    DN_OS_SemaphoreWait(&async->worker_sem, UINT32_MAX);
    if (async->join_threads)
        break;

    DN_ASYNCJob job = {};
    for (DN_OS_MutexScope(&async->ring_mutex)) {
      if (DN_Ring_HasData(ring, sizeof(job))) {
        DN_Ring_Read(ring, &job, sizeof(job));
        break;
      }
    }

    if (job.work.func) {
      DN_OS_ConditionVariableBroadcast(&async->ring_write_cv); // Resume any blocked ring write(s)

      DN_Atomic_AddU32(&async->busy_threads, 1);
      job.work.func(job.work.input);
      DN_Atomic_SubU32(&async->busy_threads, 1);

      if (job.completion_sem.handle != 0)
        DN_OS_SemaphoreIncrement(&job.completion_sem, 1);
    }
  }

  return 0;
}

DN_API void DN_ASYNC_Init(DN_ASYNCCore *async, char *base, DN_USize base_size, DN_OSThread *threads, DN_U32 threads_size)
{
  DN_Assert(async);
  async->ring.size     = base_size;
  async->ring.base     = base;
  async->ring_mutex    = DN_OS_MutexInit();
  async->ring_write_cv = DN_OS_ConditionVariableInit();
  async->worker_sem    = DN_OS_SemaphoreInit(0);
  async->thread_count  = threads_size;
  async->threads       = threads;
  for (DN_ForIndexU(index, async->thread_count)) {
    DN_OSThread *thread = async->threads + index;
    thread->name        = DN_FStr8_InitF<64>("ASYNC W%zu", index);
    DN_OS_ThreadInit(thread, DN_ASYNC_ThreadEntryPoint_, async);
  }
}

DN_API void DN_ASYNC_Deinit(DN_ASYNCCore *async)
{
  DN_Assert(async);
  DN_Atomic_SetValue32(&async->join_threads, true);
  DN_OS_SemaphoreIncrement(&async->worker_sem, async->thread_count);
  for (DN_ForItSize(it, DN_OSThread, async->threads, async->thread_count))
    DN_OS_ThreadDeinit(it.data);
}


static bool DN_ASYNC_QueueJob_(DN_ASYNCCore *async, DN_ASYNCJob const *job, DN_U64 wait_time_ms) {
  DN_U64 end_time_ms = DN_OS_DateUnixTimeMs() + wait_time_ms;
  bool result = false;
  for (DN_OS_MutexScope(&async->ring_mutex)) {
    for (;;) {
      if (DN_Ring_HasSpace(&async->ring, sizeof(*job))) {
        DN_Ring_WriteStruct(&async->ring, job);
        result = true;
        break;
      }
      DN_OS_ConditionVariableWaitUntil(&async->ring_write_cv, &async->ring_mutex, end_time_ms);
      if (DN_OS_DateUnixTimeMs() >= end_time_ms)
        break;
    }
  }

  if (result)
    DN_OS_SemaphoreIncrement(&async->worker_sem, 1); // Flag that a job is available

  return result;
}

DN_API bool DN_ASYNC_QueueWork(DN_ASYNCCore *async, DN_ASYNCWorkFunc *func, void *input, DN_U64 wait_time_ms)
{
  DN_ASYNCJob job = {};
  job.work.func   = func;
  job.work.input  = input;
  bool result     = DN_ASYNC_QueueJob_(async, &job, wait_time_ms);
  return result;
}

DN_API DN_OSSemaphore DN_ASYNC_QueueTask(DN_ASYNCCore *async, DN_ASYNCWorkFunc *func, void *input, DN_U64 wait_time_ms)
{
  DN_OSSemaphore result = DN_OS_SemaphoreInit(0);
  DN_ASYNCJob    job    = {};
  job.work.func         = func;
  job.work.input        = input;
  job.completion_sem    = result;
  DN_ASYNC_QueueJob_(async, &job, wait_time_ms);
  return result;
}

DN_API void DN_ASYNC_WaitTask(DN_OSSemaphore *sem, DN_U32 timeout_ms)
{
  DN_OS_SemaphoreWait(sem, timeout_ms);
  DN_OS_SemaphoreDeinit(sem);
}

