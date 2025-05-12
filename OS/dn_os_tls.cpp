#define DN_OS_TLSCPP

// NOTE: DN_OSTLS ////////////////////////////////////////////////////////////////////////////////////
DN_OSTLSTMem::DN_OSTLSTMem(DN_OSTLS *tls, DN_U8 arena_index, DN_OSTLSPushTMem push_tmem)
{
  DN_Assert(arena_index == DN_OSTLSArena_TMem0 || arena_index == DN_OSTLSArena_TMem1);
  arena      = tls->arenas + arena_index;
  temp_mem   = DN_Arena_TempMemBegin(arena);
  destructed = false;
  push_arena = push_tmem;
  if (push_arena)
    DN_OS_TLSPushArena(arena);
}

DN_OSTLSTMem::~DN_OSTLSTMem()
{
  DN_Assert(destructed == false);
  DN_Arena_TempMemEnd(temp_mem);
  destructed = true;
  if (push_arena)
    DN_OS_TLSPopArena();
}

DN_API void DN_OS_TLSInit(DN_OSTLS *tls, DN_OSTLSInitArgs args)
{
  DN_Check(tls);
  if (tls->init)
    return;

  DN_U64 reserve          = args.reserve          ? args.reserve          : DN_Kilobytes(64);
  DN_U64 commit           = args.commit           ? args.commit           : DN_Kilobytes(4);
  DN_U64 err_sink_reserve = args.err_sink_reserve ? args.err_sink_reserve : DN_Kilobytes(64);
  DN_U64 err_sink_commit  = args.err_sink_commit  ? args.err_sink_commit  : DN_Kilobytes(4);

  // TODO: We shouldn't have the no alloc track flag here but the initial TLS
  // init on OS init happens before CORE init. CORE init is the one responsible
  // for setting up the alloc tracking data structures.
  DN_ForIndexU(index, DN_OSTLSArena_Count) {
    DN_Arena *arena = tls->arenas + index;
    switch (DN_CAST(DN_OSTLSArena) index) {
      default:                      *arena = DN_Arena_InitFromOSVMem(reserve, commit, DN_ArenaFlags_AllocCanLeak | DN_ArenaFlags_NoAllocTrack); break;
      case DN_OSTLSArena_ErrorSink: *arena = DN_Arena_InitFromOSVMem(err_sink_reserve, err_sink_commit, DN_ArenaFlags_AllocCanLeak | DN_ArenaFlags_NoAllocTrack); break;
      case DN_OSTLSArena_Count:     DN_InvalidCodePath; break;
    }
  }

  tls->thread_id      = DN_OS_ThreadID();
  tls->err_sink.arena = tls->arenas + DN_OSTLSArena_ErrorSink;
  tls->init           = true;
}

DN_API void DN_OS_TLSDeinit(DN_OSTLS *tls)
{
  tls->init              = false;
  tls->err_sink          = {};
  tls->arena_stack_index = {};
  DN_ForIndexU(index, DN_OSTLSArena_Count) {
    DN_Arena *arena = tls->arenas + index;
    DN_Arena_Deinit(arena);
  }
}

DN_THREAD_LOCAL DN_OSTLS *g_dn_curr_thread_tls;
DN_API void DN_OS_TLSSetCurrentThreadTLS(DN_OSTLS *tls)
{
  g_dn_curr_thread_tls = tls;
}

DN_API DN_OSTLS *DN_OS_TLSGet()
{
  DN_Assert(g_dn_curr_thread_tls &&
         "DN must be initialised (via DN_Core_Init) before calling any functions depending on "
         "TLS if this is the main thread, OR, the created thread has not called "
         "SetCurrentThreadTLS yet so the TLS data structure hasn't been assigned yet");
  return g_dn_curr_thread_tls;
}

DN_API DN_Arena *DN_OS_TLSArena()
{
  DN_OSTLS *tls    = DN_OS_TLSGet();
  DN_Arena *result = tls->arenas + DN_OSTLSArena_Main;
  return result;
}

// TODO: Is there a way to handle conflict arenas without the user needing to
// manually pass it in?
DN_API DN_OSTLSTMem DN_OS_TLSGetTMem(void const *conflict_arena, DN_OSTLSPushTMem push_tmem)
{
  DN_OSTLS *tls       = DN_OS_TLSGet();
  DN_U8     tls_index = (DN_U8)-1;
  for (DN_U8 index = DN_OSTLSArena_TMem0; index <= DN_OSTLSArena_TMem1; index++) {
    DN_Arena *arena = tls->arenas + index;
    if (!conflict_arena || arena != conflict_arena) {
      tls_index = index;
      break;
    }
  }

  DN_Assert(tls_index != (DN_U8)-1);
  return DN_OSTLSTMem(tls, tls_index, push_tmem);
}

DN_API void DN_OS_TLSPushArena(DN_Arena *arena)
{
  DN_Assert(arena);
  DN_OSTLS *tls = DN_OS_TLSGet();
  DN_Assert(tls->arena_stack_index < DN_ArrayCountU(tls->arena_stack));
  tls->arena_stack[tls->arena_stack_index++] = arena;
}

DN_API void DN_OS_TLSPopArena()
{
  DN_OSTLS *tls = DN_OS_TLSGet();
  DN_Assert(tls->arena_stack_index > 0);
  tls->arena_stack_index--;
}

DN_API DN_Arena *DN_OS_TLSTopArena()
{
  DN_OSTLS   *tls    = DN_OS_TLSGet();
  DN_Arena *result = nullptr;
  if (tls->arena_stack_index)
    result = tls->arena_stack[tls->arena_stack_index - 1];
  return result;
}

DN_API void DN_OS_TLSBeginFrame(DN_Arena *frame_arena)
{
  DN_OSTLS *tls      = DN_OS_TLSGet();
  tls->frame_arena = frame_arena;
}

DN_API DN_Arena *DN_OS_TLSFrameArena()
{
  DN_OSTLS   *tls    = DN_OS_TLSGet();
  DN_Arena *result = tls->frame_arena;
  DN_AssertF(result, "Frame arena must be set by calling DN_OS_TLSBeginFrame at the beginning of the frame");
  return result;
}

// NOTE: DN_OSErrSink ////////////////////////////////////////////////////////////////////////////////
static void DN_OS_ErrSinkCheck_(DN_OSErrSink const *err)
{
  DN_AssertF(err->arena, "Arena should be assigned in TLS init");
  if (err->stack_size == 0)
    return;

  DN_OSErrSinkNode const *node = err->stack + (err->stack_size - 1);
  DN_Assert(node->mode >= DN_OSErrSinkMode_Nil && node->mode <= DN_OSErrSinkMode_ExitOnError);
  DN_Assert(node->msg_sentinel);

  // NOTE: Walk the list ensuring we eventually terminate at the sentinel (e.g. we have a
  // well formed doubly-linked-list terminated by a sentinel, or otherwise we will hit the
  // walk limit or dereference a null pointer and assert)
  size_t WALK_LIMIT = 99'999;
  size_t walk       = 0;
  for (DN_OSErrSinkMsg *it = node->msg_sentinel->next; it != node->msg_sentinel; it = it->next, walk++) {
    DN_AssertF(it, "Encountered null pointer which should not happen in a sentinel DLL");
    DN_Assert(walk < WALK_LIMIT);
  }
}

DN_API DN_OSErrSink *DN_OS_ErrSinkBegin_(DN_OSErrSinkMode mode, DN_CallSite call_site)
{
  DN_OSTLS     *tls       = DN_OS_TLSGet();
  DN_OSErrSink *err       = &tls->err_sink;
  DN_OSErrSink *result    = err;
  DN_USize    arena_pos = DN_Arena_Pos(result->arena);

  if (tls->err_sink.stack_size == DN_ArrayCountU(err->stack)) {
    DN_Str8Builder builder = DN_Str8Builder_InitFromTLS();
    DN_USize       counter = 0;
    DN_ForItSize(it, DN_OSErrSinkNode, err->stack, err->stack_size) {
      DN_MSVC_WARNING_PUSH
      DN_MSVC_WARNING_DISABLE(6284) // Object passed as _Param_(4) when a string is required in call to 'DN_Str8Builder_AppendF' Actual type: 'struct DN_Str8'.
      DN_Str8Builder_AppendF(&builder, "  [%04zu] %S:%u %S\n", counter++, it.data->call_site.file, it.data->call_site.line, it.data->call_site.function);
      DN_MSVC_WARNING_POP
    }

    DN_MSVC_WARNING_PUSH
    DN_MSVC_WARNING_DISABLE(6284) // Object passed as _Param_(6) when a string is required in call to 'DN_LOG_EmitFromType' Actual type: 'struct DN_Str8'.
    DN_AssertF(tls->err_sink.stack_size < DN_ArrayCountU(err->stack),
               "Error sink has run out of error scopes, potential leak. Scopes were\n%S", DN_Str8Builder_BuildFromTLS(&builder));
    DN_MSVC_WARNING_POP
  }

  DN_OSErrSinkNode *node = tls->err_sink.stack + tls->err_sink.stack_size++;
  node->arena_pos      = arena_pos;
  node->mode           = mode;
  node->call_site      = call_site;
  DN_DLList_InitArena(node->msg_sentinel, DN_OSErrSinkMsg, result->arena);

  // NOTE: Handle allocation error
  if (!DN_Check(node && node->msg_sentinel)) {
    DN_Arena_PopTo(result->arena, arena_pos);
    node->msg_sentinel = nullptr;
    tls->err_sink.stack_size--;
  }

  return result;
}

DN_API bool DN_OS_ErrSinkHasError(DN_OSErrSink *err)
{
  bool result = false;
  if (err && err->stack_size) {
    DN_OSErrSinkNode *node = err->stack + (err->stack_size - 1);
    result               = DN_DLList_HasItems(node->msg_sentinel);
  }
  return result;
}

DN_API DN_OSErrSinkMsg *DN_OS_ErrSinkEnd(DN_Arena *arena, DN_OSErrSink *err)
{
  DN_OSErrSinkMsg *result = nullptr;
  DN_OS_ErrSinkCheck_(err);
  if (!err || err->stack_size == 0)
    return result;

  DN_AssertF(arena != err->arena,
             "You are not allowed to reuse the arena for ending the error sink because the memory would get popped and lost");
  // NOTE: Walk the list and allocate it onto the user's arena
  DN_OSErrSinkNode *node = err->stack + (err->stack_size - 1);
  DN_OSErrSinkMsg  *prev = nullptr;
  for (DN_OSErrSinkMsg *it = node->msg_sentinel->next; it != node->msg_sentinel; it = it->next) {
    DN_OSErrSinkMsg *entry = DN_Arena_New(arena, DN_OSErrSinkMsg, DN_ZeroMem_Yes);
    entry->msg           = DN_Str8_Copy(arena, it->msg);
    entry->call_site     = it->call_site;
    entry->error_code    = it->error_code;
    if (!result)
      result = entry; // Assign first entry if we haven't yet
    if (prev)
      prev->next = entry; // Link the prev message to the current one
    prev = entry;         // Update prev to latest
  }

  // NOTE: Deallocate all the memory for this scope
  err->stack_size--;
  DN_Arena_PopTo(err->arena, node->arena_pos);
  return result;
}

static void DN_OS_ErrSinkAddMsgToStr8Builder_(DN_Str8Builder *builder, DN_OSErrSinkMsg *msg, DN_OSErrSinkMsg *end)
{
  if (msg == end) // NOTE: No error messages to add
    return;

  if (msg->next == end) {
    DN_OSErrSinkMsg *it        = msg;
    DN_Str8        file_name = DN_Str8_FileNameFromPath(it->call_site.file);
    DN_Str8Builder_AppendF(builder,
                           "%.*s:%05I32u:%.*s %.*s",
                           DN_STR_FMT(file_name),
                           it->call_site.line,
                           DN_STR_FMT(it->call_site.function),
                           DN_STR_FMT(it->msg));
  } else {
    // NOTE: More than one message
    for (DN_OSErrSinkMsg *it = msg; it != end; it = it->next) {
      DN_Str8 file_name = DN_Str8_FileNameFromPath(it->call_site.file);
      DN_Str8Builder_AppendF(builder,
                             "%s  - %.*s:%05I32u:%.*s%s%.*s",
                             it == msg ? "" : "\n",
                             DN_STR_FMT(file_name),
                             it->call_site.line,
                             DN_STR_FMT(it->call_site.function),
                             DN_Str8_HasData(it->msg) ? " " : "",
                             DN_STR_FMT(it->msg));
    }
  }
}

DN_API DN_Str8 DN_OS_ErrSinkEndStr8(DN_Arena *arena, DN_OSErrSink *err)
{
  DN_Str8 result = {};
  DN_OS_ErrSinkCheck_(err);
  if (!err || err->stack_size == 0)
    return result;

  DN_AssertF(arena != err->arena,
             "You are not allowed to reuse the arena for ending the error sink because the memory would get popped and lost");

  // NOTE: Walk the list and allocate it onto the user's arena
  DN_OSTLSTMem      tmem    = DN_OS_TLSPushTMem(arena);
  DN_Str8Builder  builder = DN_Str8Builder_InitFromTLS();
  DN_OSErrSinkNode *node    = err->stack + (err->stack_size - 1);
  DN_OS_ErrSinkAddMsgToStr8Builder_(&builder, node->msg_sentinel->next, node->msg_sentinel);

  // NOTE: Deallocate all the memory for this scope
  err->stack_size--;
  DN_U64 arena_pos = node->arena_pos;
  DN_Arena_PopTo(err->arena, arena_pos);

  result = DN_Str8Builder_Build(&builder, arena);
  return result;
}

DN_API void DN_OS_ErrSinkEndAndIgnore(DN_OSErrSink *err)
{
  DN_OS_ErrSinkEnd(nullptr, err);
}

DN_API bool DN_OS_ErrSinkEndAndLogError_(DN_OSErrSink *err, DN_CallSite call_site, DN_Str8 err_msg)
{
  DN_AssertF(err->stack_size, "Begin must be called before calling end");
  DN_OSErrSinkNode *node = err->stack + (err->stack_size - 1);
  DN_AssertF(node->msg_sentinel, "Begin must be called before calling end");

  DN_OSTLSTMem     tmem = DN_OS_TLSPushTMem(nullptr);
  DN_OSErrSinkMode mode = node->mode;
  DN_OSErrSinkMsg *msg  = DN_OS_ErrSinkEnd(tmem.arena, err);
  if (!msg)
    return false;

  DN_Str8Builder builder = DN_Str8Builder_InitFromTLS();
  if (DN_Str8_HasData(err_msg)) {
    DN_Str8Builder_AppendRef(&builder, err_msg);
    DN_Str8Builder_AppendRef(&builder, DN_STR8(":"));
  } else {
    DN_Str8Builder_AppendRef(&builder, DN_STR8("Error(s) encountered:"));
  }

  if (msg->next) // NOTE: More than 1 message
    DN_Str8Builder_AppendRef(&builder, DN_STR8("\n"));
  DN_OS_ErrSinkAddMsgToStr8Builder_(&builder, msg, nullptr);

  DN_Str8 log = DN_Str8Builder_BuildFromTLS(&builder);
  DN_LOG_EmitFromType(DN_LOG_MakeU32LogTypeParam(DN_LOGType_Error), call_site, "%.*s", DN_STR_FMT(log));

  if (mode == DN_OSErrSinkMode_DebugBreakOnEndAndLog)
    DN_DebugBreak;
  return true;
}

DN_API bool DN_OS_ErrSinkEndAndLogErrorFV_(DN_OSErrSink *err, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_OSTLSTMem tmem   = DN_OS_TLSTMem(nullptr);
  DN_Str8    log    = DN_Str8_InitFV(tmem.arena, fmt, args);
  bool       result = DN_OS_ErrSinkEndAndLogError_(err, call_site, log);
  return result;
}

DN_API bool DN_OS_ErrSinkEndAndLogErrorF_(DN_OSErrSink *err, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_OSTLSTMem tmem   = DN_OS_TLSTMem(nullptr);
  DN_Str8    log    = DN_Str8_InitFV(tmem.arena, fmt, args);
  bool       result = DN_OS_ErrSinkEndAndLogError_(err, call_site, log);
  va_end(args);
  return result;
}

DN_API void DN_OS_ErrSinkEndAndExitIfErrorFV_(DN_OSErrSink *err, DN_CallSite call_site, DN_U32 exit_val, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  if (DN_OS_ErrSinkEndAndLogErrorFV_(err, call_site, fmt, args)) {
    DN_DebugBreak;
    DN_OS_Exit(exit_val);
  }
}

DN_API void DN_OS_ErrSinkEndAndExitIfErrorF_(DN_OSErrSink *err, DN_CallSite call_site, DN_U32 exit_val, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_OS_ErrSinkEndAndExitIfErrorFV_(err, call_site, exit_val, fmt, args);
  va_end(args);
}

DN_API void DN_OS_ErrSinkAppendFV_(DN_OSErrSink *err, DN_U32 error_code, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  if (!err)
    return;
  DN_Assert(err && err->stack_size);
  DN_OSErrSinkNode *node = err->stack + (err->stack_size - 1);
  DN_AssertF(node, "Error sink must be begun by calling 'Begin' before using this function.");

  DN_OSErrSinkMsg *msg = DN_Arena_New(err->arena, DN_OSErrSinkMsg, DN_ZeroMem_Yes);
  if (DN_Check(msg)) {
    msg->msg        = DN_Str8_InitFV(err->arena, fmt, args);
    msg->error_code = error_code;
    msg->call_site  = DN_OS_TLSGet()->call_site;
    DN_DLList_Prepend(node->msg_sentinel, msg);

    if (node->mode == DN_OSErrSinkMode_ExitOnError)
      DN_OS_ErrSinkEndAndExitIfErrorF_(err, msg->call_site, error_code, "Fatal error %u", error_code);
  }
}

DN_API void DN_OS_ErrSinkAppendF_(DN_OSErrSink *err, DN_U32 error_code, DN_FMT_ATTRIB char const *fmt, ...)
{
  if (!err)
    return;
  va_list args;
  va_start(args, fmt);
  DN_OS_ErrSinkAppendFV_(err, error_code, fmt, args);
  va_end(args);
}

