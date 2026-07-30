#include "dn_seshc.h"

struct DN_SHMsgSendAsyncContext_
{
  bool             send_success;
  DN_Arena*        arena;
  DN_SHCLIArgs     args;
  DN_SHMsgSendData send;
  DN_Str8          msg;
  bool             detached;
};

DN_SHCLIArgs DN_SH_CLIArgsCopy(DN_SHCLIArgs const *src, DN_Arena *arena)
{
  DN_SHCLIArgs result   = {};
  result                = *src;
  result.exe_path       = DN_Str8FromStr8Arena(src->exe_path, arena);
  result.ini_path       = DN_Str8FromStr8Arena(src->ini_path, arena);
  result.seed_node_url  = DN_Str8FromStr8Arena(src->seed_node_url, arena);
  return result;
}

DN_Str8 DN_SH_CLIOptionsStr8()
{
  DN_Str8 result = DN_Str8Lit(
"SESHC OPTIONS (note all options must be set to activate Seshc):\n"
"  --seshc-exe-path <path>              Path to the Seshc EXE file to invoke for sending messages\n"
"  --seshc-ini-path <path>              Path to the .ini file to load Seshc arguments from\n"
"                                       (such as account secrets, display name, recipient...)\n"
"  --seshc-seed-node <url>              URL to the Session seed node for determining the nodes to\n"
"                                       send the message to\n"
  );
  return result;
}

DN_SHCLIEatArgResult DN_SH_CLIEatArg(DN_SHCLIArgs *args, char const **arg_ptr, int arg_count)
{
  DN_SHCLIEatArgResult result = {};
  result.arg_ptr           = arg_ptr;
  result.arg_count         = arg_count;

  DN_Str8 arg = {};
  if (arg_count)
    arg = DN_Str8FromCStr8(*result.arg_ptr);

  // NOTE: Try handling the argument if we recognise it
  {
    if (!result.eaten && DN_Str8EqInsensitive(arg, DN_Str8Lit("--seshc-exe-path"))) {
      (void)DN_ArgShift(result.arg_ptr, result.arg_count);
      result.eaten   = true;
      args->exe_path = DN_Str8FromCStr8(DN_ArgShift(result.arg_ptr, result.arg_count));

      if (DN_OS_PathInfo(args->exe_path).type != DN_OSPathInfoType_File) {
        result.error_msg = DN_Str8Lit("Seshc executable path does not point to a file");
        result.error_arg = args->exe_path;
        args->exe_path   = {};
      }
    }

    if (!result.eaten && DN_Str8EqInsensitive(arg, DN_Str8Lit("--seshc-ini-path"))) {
      (void)DN_ArgShift(result.arg_ptr, result.arg_count);
      result.eaten   = true;
      args->ini_path = DN_Str8FromCStr8(DN_ArgShift(result.arg_ptr, result.arg_count));

      if (DN_OS_PathInfo(args->ini_path).type != DN_OSPathInfoType_File) {
        result.error_msg = DN_Str8Lit("Seshc ini path does not point to a file");
        result.error_arg = args->ini_path;
        args->ini_path   = {};
      }
    }

    if (!result.eaten && DN_Str8EqInsensitive(arg, DN_Str8Lit("--seshc-seed-node"))) {
      (void)DN_ArgShift(result.arg_ptr, result.arg_count);
      result.eaten        = true;
      args->seed_node_url = DN_Str8FromCStr8(DN_ArgShift(result.arg_ptr, result.arg_count));
    }
  }

  if (args->seed_node_url.count && args->exe_path.count && args->ini_path.count)
    args->valid = true;
  return result;
}

bool DN_SH_MsgSendBlockingFmt(DN_SHMsgSendData send, DN_SHCLIArgs *cli_args, char const *fmt, ...)
{
  bool result = false;
  if (cli_args->valid) {
    DN_TcScratch scratch = DN_TcScratchBeginArena(nullptr, 0);

    va_list args;
    va_start(args, fmt);
    DN_Str8 msg = DN_Str8FmtVArena(&scratch.arena, fmt, args);
    va_end(args);

    DN_Str8 account_secret_arg = DN_Str8Lit("--account-secret");
    DN_Str8 seed_node_url_arg  = DN_Str8Lit("--seed-node-url");
    DN_Str8 display_name_arg   = DN_Str8Lit("--display-name");
    DN_Str8 ini_path_arg       = DN_Str8Lit("--ini-path");
    DN_Str8 send_arg           = DN_Str8Lit("send");

    DN_Str8  cmds[16]   = {};
    DN_USize cmds_count = 0;
    DN_LArrayAppend(cmds, &cmds_count, cli_args->exe_path);

    DN_LArrayAppend(cmds, &cmds_count, account_secret_arg);
    DN_LArrayAppend(cmds, &cmds_count, send.account_secret);

    DN_LArrayAppend(cmds, &cmds_count, seed_node_url_arg);
    DN_LArrayAppend(cmds, &cmds_count, cli_args->seed_node_url);

    if (send.display_name.count) {
      DN_LArrayAppend(cmds, &cmds_count, display_name_arg);
      DN_LArrayAppend(cmds, &cmds_count, send.display_name);
    }

    DN_LArrayAppend(cmds, &cmds_count, ini_path_arg);
    DN_LArrayAppend(cmds, &cmds_count, cli_args->ini_path);

    DN_LArrayAppend(cmds, &cmds_count, send_arg);
    DN_LArrayAppend(cmds, &cmds_count, send.recipient);

    DN_LArrayAppend(cmds, &cmds_count, msg);

    DN_Str8Slice cmd_line = {};
    cmd_line.data         = cmds;
    cmd_line.count        = cmds_count;
    DN_OSExecResult exec  = DN_OS_Exec(cmd_line, DN_OS_ExecArgsDefault(), &scratch.arena, nullptr);
    result                = exec.os_error_code == 0 && exec.exit_code == 0;
    if (!result) {
      DN_LogErrorF("Error sending message to %.*s (exit code: %d, os: %d): %.*s",
                   DN_Str8PrintFmt(send.recipient),
                   exec.exit_code,
                   exec.os_error_code,
                   DN_Str8PrintFmt(exec.stderr_text));
    }
    DN_TcScratchEnd(&scratch);

  }
  return result;
}

static int DN_SH_MsgSendDetachedFmtThreadFunc_(DN_OSThread *thread)
{
  DN_SHMsgSendAsyncContext_ *context = DN_Cast(DN_SHMsgSendAsyncContext_ *) thread->user_context;
  context->send_success              = DN_SH_MsgSendBlockingFmt(context->send, &context->args, "%.*s", DN_Str8PrintFmt(context->msg));
  if (context->detached)
    DN_ArenaDeinit(context->arena);
  return 0;
}

static DN_SHMsgSendAsyncHandle DN_SH_MsgSendAsyncFmtVMaybeDetached_(DN_SHMsgSendData send, DN_SHCLIArgs *cli_args, DN_Arena *arena, bool detached, char const *fmt, va_list args)
{
  DN_SHMsgSendAsyncHandle result = {};
  if (!cli_args->valid)
    return result;

  // NOTE: Setup context
  DN_SHMsgSendAsyncContext_ *context = DN_ArenaNewZ(arena, DN_SHMsgSendAsyncContext_);
  context->args                      = DN_SH_CLIArgsCopy(cli_args, arena);
  context->send                      = send;
  context->arena                     = arena;
  context->msg                       = DN_Str8FmtVArena(arena, fmt, args);
  context->detached                  = detached;

  // NOTE: Execute and forget
  DN_OSThreadInitArgs thread_args = DN_OS_ThreadInitArgsDefault();
  if (detached)
    thread_args.flags |= DN_OSThreadFlags_Detached;

  DN_OS_ThreadInit(&result.thread, DN_SH_MsgSendDetachedFmtThreadFunc_, thread_args, context);
  return result;
}

void DN_SH_MsgSendDetachedFmt(DN_SHMsgSendData send, DN_SHCLIArgs *cli_args, char const *fmt, ...)
{
  DN_Arena  arena_stack = DN_OS_ArenaFromHeapVirtual(0, 0, DN_MemFlags_Nil);
  DN_Arena *arena       = DN_ArenaNewZ(&arena_stack, DN_Arena);
  *arena                = arena_stack;

  va_list args;
  va_start(args, fmt);
  DN_SH_MsgSendAsyncFmtVMaybeDetached_(send, cli_args, arena, /*detached=*/ true, fmt, args);
  va_end(args);
}

DN_SHMsgSendAsyncHandle DN_SH_MsgSendAsyncFmt(DN_SHMsgSendData send, DN_SHCLIArgs *cli_args, DN_Arena *arena, char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_SHMsgSendAsyncHandle result = DN_SH_MsgSendAsyncFmtVMaybeDetached_(send, cli_args, arena, /*detached=*/ false, fmt, args);
  va_end(args);
  return result;
}

DN_SHMsgSendAsyncStatus DN_SH_MsgSendAsyncWait(DN_SHMsgSendAsyncHandle *handle, DN_U32 timeout_ms)
{
  DN_SHMsgSendAsyncStatus result = DN_SHMsgSendAsyncStatus_Success;
  if (handle && handle->thread.user_context) {
    bool joined = DN_OS_ThreadJoin(&handle->thread, timeout_ms, DN_TcDeinitArenas_Yes);
    if (joined) {
      DN_SHMsgSendAsyncContext_ *context = DN_Cast(DN_SHMsgSendAsyncContext_ *) handle->thread.user_context;
      result                             = context->send_success ? DN_SHMsgSendAsyncStatus_Success : DN_SHMsgSendAsyncStatus_Failed;
      *handle                            = {};
    } else {
      result = DN_SHMsgSendAsyncStatus_Timeout;
    }
  }
  return result;
}

void DN_SH_LogBroadcastFmtV(DN_SHLogMode mode, DN_LogType type, DN_SHMsgSendData send, DN_SHCLIArgs *cli_args, DN_Str8 short_desc, char const *fmt, va_list args)
{
  DN_TcScratch scratch = DN_TcScratchBeginArena(nullptr, 0);
  DN_Str8      msg     = DN_Str8FmtVArena(&scratch.arena, fmt, args);
  DN_Str8      prefix  = {};
  switch (type) {
    case DN_LogType_Debug:   prefix = DN_Str8Lit("🐞 "); break;
    case DN_LogType_Info:    prefix = DN_Str8Lit("ℹ️ "); break;
    case DN_LogType_Warning: prefix = DN_Str8Lit("⚠️ "); break;
    case DN_LogType_Error:   prefix = DN_Str8Lit("🚨 "); break;
    case DN_LogType_Count:   break;
  }
  DN_LogF(type, "%.*s%.*s. %.*s", DN_Str8PrintFmt(prefix), DN_Str8PrintFmt(short_desc), DN_Str8PrintFmt(msg));

  DN_Date date = DN_OS_DateLocalTimeNow();

  // NOTE: 12-hour conversion
  DN_U8 hour12 = date.hour % 12;
  if (hour12 == 0)
    hour12 = 12;
  char const *am_pm = date.hour < 12 ? "AM" : "PM";

  // Time-of-day emoji
  DN_Str8 time_emoji = {};
  if (date.hour >= 5 && date.hour <= 11)
    time_emoji = DN_Str8Lit("☀️");
  else if (date.hour >= 12 && date.hour <= 16)
    time_emoji = DN_Str8Lit("🌤️");
  else if (date.hour >= 17 && date.hour <= 19)
    time_emoji = DN_Str8Lit("🌅");
  else if (date.hour >= 20 && date.hour <= 23)
    time_emoji = DN_Str8Lit("✨");
  else
    time_emoji = DN_Str8Lit("🌑");

  if (mode == DN_SHLogMode_Detached) {
    DN_SH_MsgSendDetachedFmt(send,
                             cli_args,
                             "%.*s%.*s\n"
                             "%.*s %u-%02u-%02u %u:%02u:%02u %s%s"
                             "%.*s",
                             DN_Str8PrintFmt(prefix),
                             DN_Str8PrintFmt(short_desc),
                             DN_Str8PrintFmt(time_emoji),
                             date.year,
                             date.month,
                             date.day,
                             hour12,
                             date.minutes,
                             date.seconds,
                             am_pm,
                             msg.count ? "\n" : "",
                             DN_Str8PrintFmt(msg));
  } else {
    DN_Assert(mode == DN_SHLogMode_Blocking);
    DN_SH_MsgSendBlockingFmt(send,
                             cli_args,
                             "%.*s %.*s\n"
                             "%.*s %u-%02u-%02u %u:%02u:%02u %s%s"
                             "%.*s",
                             DN_Str8PrintFmt(prefix),
                             DN_Str8PrintFmt(short_desc),
                             DN_Str8PrintFmt(time_emoji),
                             date.year,
                             date.month,
                             date.day,
                             hour12,
                             date.minutes,
                             date.seconds,
                             am_pm,
                             msg.count ? "\n" : "",
                             DN_Str8PrintFmt(msg));
  }
  DN_TcScratchEnd(&scratch);
}

void DN_SH_LogBroadcastBlockingFmt(DN_LogType type, DN_SHMsgSendData send, DN_SHCLIArgs *cli_args, DN_Str8 short_desc, char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_SH_LogBroadcastFmtV(DN_SHLogMode_Blocking, type, send, cli_args, short_desc, fmt, args);
  va_end(args);
}

void DN_SH_LogBroadcastDetachedFmt(DN_LogType type, DN_SHMsgSendData send, DN_SHCLIArgs *cli_args, DN_Str8 short_desc, char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_SH_LogBroadcastFmtV(DN_SHLogMode_Detached, type, send, cli_args, short_desc, fmt, args);
  va_end(args);
}
