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

    DN_ASYNCTask task = {};
    for (DN_OS_MutexScope(&async->ring_mutex)) {
      if (DN_Ring_HasData(ring, sizeof(task)))
        DN_Ring_Read(ring, &task, sizeof(task));
    }

    if (task.work.func) {
      DN_OS_ConditionVariableBroadcast(&async->ring_write_cv); // Resume any blocked ring write(s)

      DN_ASYNCWorkArgs args = {};
      args.input            = task.work.input;
      args.thread           = thread;

      DN_AtomicAddU32(&async->busy_threads, 1);
      task.work.func(args);
      DN_AtomicSubU32(&async->busy_threads, 1);

      if (task.completion_sem.handle != 0)
        DN_OS_SemaphoreIncrement(&task.completion_sem, 1);
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
  DN_AtomicSetValue32(&async->join_threads, true);
  DN_OS_SemaphoreIncrement(&async->worker_sem, async->thread_count);
  for (DN_ForItSize(it, DN_OSThread, async->threads, async->thread_count))
    DN_OS_ThreadDeinit(it.data);
}

static bool DN_ASYNC_QueueTask_(DN_ASYNCCore *async, DN_ASYNCTask const *task, DN_U64 wait_time_ms) {
  DN_U64 end_time_ms = DN_OS_DateUnixTimeMs() + wait_time_ms;
  bool result = false;
  for (DN_OS_MutexScope(&async->ring_mutex)) {
    for (;;) {
      if (DN_Ring_HasSpace(&async->ring, sizeof(*task))) {
        DN_Ring_WriteStruct(&async->ring, task);
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
  DN_ASYNCTask task = {};
  task.work.func    = func;
  task.work.input   = input;
  bool result       = DN_ASYNC_QueueTask_(async, &task, wait_time_ms);
  return result;
}

DN_API DN_ASYNCTask DN_ASYNC_QueueTask(DN_ASYNCCore *async, DN_ASYNCWorkFunc *func, void *input, DN_U64 wait_time_ms)
{
  DN_ASYNCTask result   = {};
  result.work.func      = func;
  result.work.input     = input;
  result.completion_sem = DN_OS_SemaphoreInit(0);
  result.queued         = DN_ASYNC_QueueTask_(async, &result, wait_time_ms);
  if (!result.queued)
      DN_OS_SemaphoreDeinit(&result.completion_sem);
  return result;
}

DN_API bool DN_ASYNC_WaitTask(DN_ASYNCTask *task, DN_U32 timeout_ms)
{
  bool result = true;
  if (!task->queued)
    return result;

  DN_OSSemaphoreWaitResult wait = DN_OS_SemaphoreWait(&task->completion_sem, timeout_ms);
  result                        = wait == DN_OSSemaphoreWaitResult_Success;
  if (result)
    DN_OS_SemaphoreDeinit(&task->completion_sem);
  return result;
}

