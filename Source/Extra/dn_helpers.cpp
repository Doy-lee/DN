#define DN_HELPERS_CPP

/*
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   $$\   $$\ $$$$$$$$\ $$\       $$$$$$$\  $$$$$$$$\ $$$$$$$\   $$$$$$\
//   $$ |  $$ |$$  _____|$$ |      $$  __$$\ $$  _____|$$  __$$\ $$  __$$\
//   $$ |  $$ |$$ |      $$ |      $$ |  $$ |$$ |      $$ |  $$ |$$ /  \__|
//   $$$$$$$$ |$$$$$\    $$ |      $$$$$$$  |$$$$$\    $$$$$$$  |\$$$$$$\
//   $$  __$$ |$$  __|   $$ |      $$  ____/ $$  __|   $$  __$$<  \____$$\
//   $$ |  $$ |$$ |      $$ |      $$ |      $$ |      $$ |  $$ |$$\   $$ |
//   $$ |  $$ |$$$$$$$$\ $$$$$$$$\ $$ |      $$$$$$$$\ $$ |  $$ |\$$$$$$  |
//   \__|  \__|\________|\________|\__|      \________|\__|  \__| \______/
//
//   dn_helpers.cpp
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

// NOTE: DN_PCG32 //////////////////////////////////////////////////////////////////////////////////
#define DN_PCG_DEFAULT_MULTIPLIER_64 6364136223846793005ULL
#define DN_PCG_DEFAULT_INCREMENT_64  1442695040888963407ULL

DN_API DN_PCG32 DN_PCG32_Init(uint64_t seed)
{
  DN_PCG32 result = {};
  DN_PCG32_Next(&result);
  result.state += seed;
  DN_PCG32_Next(&result);
  return result;
}

DN_API uint32_t DN_PCG32_Next(DN_PCG32 *rng)
{
  uint64_t state = rng->state;
  rng->state     = state * DN_PCG_DEFAULT_MULTIPLIER_64 + DN_PCG_DEFAULT_INCREMENT_64;

  // XSH-RR
  uint32_t value = (uint32_t)((state ^ (state >> 18)) >> 27);
  int      rot   = state >> 59;
  return rot ? (value >> rot) | (value << (32 - rot)) : value;
}

DN_API uint64_t DN_PCG32_Next64(DN_PCG32 *rng)
{
  uint64_t value = DN_PCG32_Next(rng);
  value <<= 32;
  value |= DN_PCG32_Next(rng);
  return value;
}

DN_API uint32_t DN_PCG32_Range(DN_PCG32 *rng, uint32_t low, uint32_t high)
{
  uint32_t bound     = high - low;
  uint32_t threshold = -(int32_t)bound % bound;

  for (;;) {
    uint32_t r = DN_PCG32_Next(rng);
    if (r >= threshold)
      return low + (r % bound);
  }
}

DN_API float DN_PCG32_NextF32(DN_PCG32 *rng)
{
  uint32_t x = DN_PCG32_Next(rng);
  return (float)(int32_t)(x >> 8) * 0x1.0p-24f;
}

DN_API double DN_PCG32_NextF64(DN_PCG32 *rng)
{
  uint64_t x = DN_PCG32_Next64(rng);
  return (double)(int64_t)(x >> 11) * 0x1.0p-53;
}

DN_API void DN_PCG32_Advance(DN_PCG32 *rng, uint64_t delta)
{
  uint64_t cur_mult = DN_PCG_DEFAULT_MULTIPLIER_64;
  uint64_t cur_plus = DN_PCG_DEFAULT_INCREMENT_64;

  uint64_t acc_mult = 1;
  uint64_t acc_plus = 0;

  while (delta != 0) {
    if (delta & 1) {
      acc_mult *= cur_mult;
      acc_plus = acc_plus * cur_mult + cur_plus;
    }
    cur_plus = (cur_mult + 1) * cur_plus;
    cur_mult *= cur_mult;
    delta >>= 1;
  }

  rng->state = acc_mult * rng->state + acc_plus;
}

#if !defined(DN_NO_JSON_BUILDER)
// NOTE: DN_JSONBuilder ////////////////////////////////////////////////////////////////////////////
DN_API DN_JSONBuilder DN_JSONBuilder_Init(DN_Arena *arena, int spaces_per_indent)
{
  DN_JSONBuilder result       = {};
  result.spaces_per_indent    = spaces_per_indent;
  result.string_builder.arena = arena;
  return result;
}

DN_API DN_Str8 DN_JSONBuilder_Build(DN_JSONBuilder const *builder, DN_Arena *arena)
{
  DN_Str8 result = DN_Str8Builder_Build(&builder->string_builder, arena);
  return result;
}

DN_API void DN_JSONBuilder_KeyValue(DN_JSONBuilder *builder, DN_Str8 key, DN_Str8 value)
{
  if (key.size == 0 && value.size == 0)
    return;

  DN_JSONBuilderItem item = DN_JSONBuilderItem_KeyValue;
  if (value.size >= 1) {
    if (value.data[0] == '{' || value.data[0] == '[')
      item = DN_JSONBuilderItem_OpenContainer;
    else if (value.data[0] == '}' || value.data[0] == ']')
      item = DN_JSONBuilderItem_CloseContainer;
  }

  bool adding_to_container_with_items =
      item != DN_JSONBuilderItem_CloseContainer && (builder->last_item == DN_JSONBuilderItem_KeyValue ||
                                                    builder->last_item == DN_JSONBuilderItem_CloseContainer);

  uint8_t prefix_size = 0;
  char    prefix[2]   = {0};
  if (adding_to_container_with_items)
    prefix[prefix_size++] = ',';

  if (builder->last_item != DN_JSONBuilderItem_Empty)
    prefix[prefix_size++] = '\n';

  if (item == DN_JSONBuilderItem_CloseContainer)
    builder->indent_level--;

  int spaces_per_indent = builder->spaces_per_indent ? builder->spaces_per_indent : 2;
  int spaces            = builder->indent_level * spaces_per_indent;

  if (key.size)
    DN_Str8Builder_AppendF(&builder->string_builder,
                           "%.*s%*c\"%.*s\": %.*s",
                           prefix_size,
                           prefix,
                           spaces,
                           ' ',
                           DN_STR_FMT(key),
                           DN_STR_FMT(value));
  else if (spaces == 0)
    DN_Str8Builder_AppendF(&builder->string_builder, "%.*s%.*s", prefix_size, prefix, DN_STR_FMT(value));
  else
    DN_Str8Builder_AppendF(&builder->string_builder, "%.*s%*c%.*s", prefix_size, prefix, spaces, ' ', DN_STR_FMT(value));

  if (item == DN_JSONBuilderItem_OpenContainer)
    builder->indent_level++;

  builder->last_item = item;
}

DN_API void DN_JSONBuilder_KeyValueFV(DN_JSONBuilder *builder, DN_Str8 key, char const *value_fmt, va_list args)
{
  DN_OSTLSTMem tmem  = DN_OS_TLSTMem(builder->string_builder.arena);
  DN_Str8    value = DN_Str8_FromFV(tmem.arena, value_fmt, args);
  DN_JSONBuilder_KeyValue(builder, key, value);
}

DN_API void DN_JSONBuilder_KeyValueF(DN_JSONBuilder *builder, DN_Str8 key, char const *value_fmt, ...)
{
  va_list args;
  va_start(args, value_fmt);
  DN_JSONBuilder_KeyValueFV(builder, key, value_fmt, args);
  va_end(args);
}

DN_API void DN_JSONBuilder_ObjectBeginNamed(DN_JSONBuilder *builder, DN_Str8 name)
{
  DN_JSONBuilder_KeyValue(builder, name, DN_STR8("{"));
}

DN_API void DN_JSONBuilder_ObjectEnd(DN_JSONBuilder *builder)
{
  DN_JSONBuilder_KeyValue(builder, DN_STR8(""), DN_STR8("}"));
}

DN_API void DN_JSONBuilder_ArrayBeginNamed(DN_JSONBuilder *builder, DN_Str8 name)
{
  DN_JSONBuilder_KeyValue(builder, name, DN_STR8("["));
}

DN_API void DN_JSONBuilder_ArrayEnd(DN_JSONBuilder *builder)
{
  DN_JSONBuilder_KeyValue(builder, DN_STR8(""), DN_STR8("]"));
}

DN_API void DN_JSONBuilder_Str8Named(DN_JSONBuilder *builder, DN_Str8 key, DN_Str8 value)
{
  DN_JSONBuilder_KeyValueF(builder, key, "\"%.*s\"", value.size, value.data);
}

DN_API void DN_JSONBuilder_LiteralNamed(DN_JSONBuilder *builder, DN_Str8 key, DN_Str8 value)
{
  DN_JSONBuilder_KeyValueF(builder, key, "%.*s", value.size, value.data);
}

DN_API void DN_JSONBuilder_U64Named(DN_JSONBuilder *builder, DN_Str8 key, uint64_t value)
{
  DN_JSONBuilder_KeyValueF(builder, key, "%I64u", value);
}

DN_API void DN_JSONBuilder_I64Named(DN_JSONBuilder *builder, DN_Str8 key, int64_t value)
{
  DN_JSONBuilder_KeyValueF(builder, key, "%I64d", value);
}

DN_API void DN_JSONBuilder_F64Named(DN_JSONBuilder *builder, DN_Str8 key, double value, int decimal_places)
{
  if (!builder)
    return;

  if (decimal_places >= 16)
    decimal_places = 16;

  // NOTE: Generate the format string for the float, depending on how many
  // decimals places it wants.
  char float_fmt[16];
  if (decimal_places > 0) {
    // NOTE: Emit the format string "%.<decimal_places>f" i.e. %.1f
    DN_SNPrintF(float_fmt, sizeof(float_fmt), "%%.%df", decimal_places);
  } else {
    // NOTE: Emit the format string "%f"
    DN_SNPrintF(float_fmt, sizeof(float_fmt), "%%f");
  }
  DN_JSONBuilder_KeyValueF(builder, key, float_fmt, value);
}

DN_API void DN_JSONBuilder_BoolNamed(DN_JSONBuilder *builder, DN_Str8 key, bool value)
{
  DN_Str8 value_string = value ? DN_STR8("true") : DN_STR8("false");
  DN_JSONBuilder_KeyValueF(builder, key, "%.*s", value_string.size, value_string.data);
}
#endif // !defined(DN_NO_JSON_BUILDER)

// NOTE: DN_JobQueue ///////////////////////////////////////////////////////////////////////////////
DN_API DN_JobQueueSPMC DN_OS_JobQueueSPMCInit()
{
  DN_JobQueueSPMC result                = {};
  result.thread_wait_for_job_semaphore  = DN_OS_SemaphoreInit(0 /*initial_count*/);
  result.wait_for_completion_semaphore  = DN_OS_SemaphoreInit(0 /*initial_count*/);
  result.complete_queue_write_semaphore = DN_OS_SemaphoreInit(DN_ArrayCountU(result.complete_queue));
  result.mutex                          = DN_OS_MutexInit();
  return result;
}

DN_API bool DN_OS_JobQueueSPMCCanAdd(DN_JobQueueSPMC const *queue, uint32_t count)
{
  uint32_t read_index  = queue->read_index;
  uint32_t write_index = queue->write_index;
  uint32_t size        = write_index - read_index;
  bool     result      = (size + count) <= DN_ArrayCountU(queue->jobs);
  return result;
}

DN_API bool DN_OS_JobQueueSPMCAddArray(DN_JobQueueSPMC *queue, DN_Job *jobs, uint32_t count)
{
  if (!queue)
    return false;

  uint32_t const pot_mask    = DN_ArrayCountU(queue->jobs) - 1;
  uint32_t       read_index  = queue->read_index;
  uint32_t       write_index = queue->write_index;
  uint32_t       size        = write_index - read_index;

  if ((size + count) > DN_ArrayCountU(queue->jobs))
    return false;

  for (size_t offset = 0; offset < count; offset++) {
    uint32_t wrapped_write_index     = (write_index + offset) & pot_mask;
    queue->jobs[wrapped_write_index] = jobs[offset];
  }

  DN_OS_MutexLock(&queue->mutex);
  queue->write_index += count;
  DN_OS_SemaphoreIncrement(&queue->thread_wait_for_job_semaphore, count);
  DN_OS_MutexUnlock(&queue->mutex);
  return true;
}

DN_API bool DN_OS_JobQueueSPMCAdd(DN_JobQueueSPMC *queue, DN_Job job)
{
  bool result = DN_OS_JobQueueSPMCAddArray(queue, &job, 1);
  return result;
}

DN_API int32_t DN_OS_JobQueueSPMCThread(DN_OSThread *thread)
{
  DN_JobQueueSPMC *queue    = DN_CAST(DN_JobQueueSPMC *) thread->user_context;
  uint32_t const   pot_mask = DN_ArrayCountU(queue->jobs) - 1;
  static_assert(DN_ArrayCountU(queue->jobs) == DN_ArrayCountU(queue->complete_queue), "PoT mask is used to mask access to both arrays");

  for (;;) {
    DN_OS_SemaphoreWait(&queue->thread_wait_for_job_semaphore, DN_OS_SEMAPHORE_INFINITE_TIMEOUT);
    if (queue->quit)
      break;

    DN_Assert(queue->read_index != queue->write_index);

    DN_OS_MutexLock(&queue->mutex);
    uint32_t wrapped_read_index = queue->read_index & pot_mask;
    DN_Job   job                = queue->jobs[wrapped_read_index];
    queue->read_index += 1;
    DN_OS_MutexUnlock(&queue->mutex);

    job.elapsed_tsc -= DN_CPUGetTSC();
    job.func(thread, job.user_context);
    job.elapsed_tsc += DN_CPUGetTSC();

    if (job.add_to_completion_queue) {
      DN_OS_SemaphoreWait(&queue->complete_queue_write_semaphore, DN_OS_SEMAPHORE_INFINITE_TIMEOUT);
      DN_OS_MutexLock(&queue->mutex);
      queue->complete_queue[(queue->complete_write_index++ & pot_mask)] = job;
      DN_OS_MutexUnlock(&queue->mutex);
      DN_OS_SemaphoreIncrement(&queue->complete_queue_write_semaphore, 1);
    }

    // NOTE: Update finish counter
    DN_OS_MutexLock(&queue->mutex);
    queue->finish_index += 1;

    // NOTE: If all jobs are finished and we have another thread who is
    // blocked via `WaitForCompletion` for this job queue, we will go
    // release the semaphore to wake them all up.
    bool all_jobs_finished = queue->finish_index == queue->write_index;
    if (all_jobs_finished && queue->threads_waiting_for_completion) {
      DN_OS_SemaphoreIncrement(&queue->wait_for_completion_semaphore,
                               queue->threads_waiting_for_completion);
      queue->threads_waiting_for_completion = 0;
    }
    DN_OS_MutexUnlock(&queue->mutex);
  }

  return queue->quit_exit_code;
}

DN_API void DN_OS_JobQueueSPMCWaitForCompletion(DN_JobQueueSPMC *queue)
{
  DN_OS_MutexLock(&queue->mutex);
  if (queue->finish_index == queue->write_index) {
    DN_OS_MutexUnlock(&queue->mutex);
    return;
  }
  queue->threads_waiting_for_completion++;
  DN_OS_MutexUnlock(&queue->mutex);

  DN_OS_SemaphoreWait(&queue->wait_for_completion_semaphore, DN_OS_SEMAPHORE_INFINITE_TIMEOUT);
}

DN_API DN_USize DN_OS_JobQueueSPMCGetFinishedJobs(DN_JobQueueSPMC *queue, DN_Job *jobs, DN_USize jobs_size)
{
  DN_USize result = 0;
  if (!queue || !jobs || jobs_size <= 0)
    return result;

  uint32_t const pot_mask = DN_ArrayCountU(queue->jobs) - 1;
  DN_OS_MutexLock(&queue->mutex);
  while (queue->complete_read_index < queue->complete_write_index && result < jobs_size)
    jobs[result++] = queue->complete_queue[(queue->complete_read_index++ & pot_mask)];
  DN_OS_MutexUnlock(&queue->mutex);

  return result;
}
