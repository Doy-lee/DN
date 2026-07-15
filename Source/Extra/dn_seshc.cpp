#include "dn_seshc.h"

struct DN_SHSendDMThreadContext_
{
  DN_Arena     arena;
  DN_SHCLIArgs args;
  DN_Str8      sesh_pkey_hex;
  DN_Str8      msg;
};

void DN_SH_LogBroadcast(DN_LogType type, DN_SHCLIArgs *cli_args, DN_Str8 short_desc, char const *fmt, ...)
{
  DN_TCScratch scratch = DN_TCScratchBeginArena(nullptr, 0);
  va_list args;
  va_start(args, fmt);
  DN_Str8 msg = DN_Str8FromFmtVArena(&scratch.arena, fmt, args);
  va_end(args);
  DN_Str8 prefix = {};
  switch (type) {
    case DN_LogType_Debug:   prefix = DN_Str8Lit("🐞 "); break;
    case DN_LogType_Info:    prefix = DN_Str8Lit("ℹ️ "); break;
    case DN_LogType_Warning: prefix = DN_Str8Lit("⚠️ "); break;
    case DN_LogType_Error:   prefix = DN_Str8Lit("🚨 "); break;
    case DN_LogType_Count:   break;
  }
  DN_LogF(type, "%.*s%.*s", DN_Str8PrintFmt(prefix), DN_Str8PrintFmt(msg));

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
  DN_SH_CLIArgsSendDMDetachedFmt(cli_args,
                                 cli_args->recipient_pkey,
                                 "%.*s %.*s\n"
                                 "%.*s %u-%02u-%02u %u:%02u:%02u %s\n"
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
                                 DN_Str8PrintFmt(msg));
  DN_TCScratchEnd(&scratch);
}

DN_SHCLIArgs DN_SH_CLIArgsCopy(DN_SHCLIArgs const *src, DN_Arena *arena)
{
  DN_SHCLIArgs result   = {};
  result                = *src;
  result.exe_path       = DN_Str8FromStr8Arena(src->exe_path, arena);
  result.account_secret = DN_Str8FromStr8Arena(src->account_secret, arena);
  result.seed_node_url  = DN_Str8FromStr8Arena(src->seed_node_url, arena);
  result.display_name   = DN_Str8FromStr8Arena(src->display_name, arena);
  result.recipient_pkey = DN_Str8FromStr8Arena(src->recipient_pkey, arena);
  return result;
}

DN_Str8 DN_SH_CLIOptionsStr8()
{
  DN_Str8 result = DN_Str8Lit(
"SESHC OPTIONS (note all options must be set to activate Seshc):\n"
"  --seshc-exe-path <path>          Path to the Seshc EXE file to invoke for sending messages\n"
"  --seshc-account-secret <secret>  Specify the secret of the Session account to send a message\n"
"                                   as (see --help on the Seshc EXE for more info)\n"
"  --seshc-seed-node <url>          URL to the Session seed node for determining the nodes to\n"
"                                   send the message to\n"
"  --seshc-display-name <name>      Display name to send the message as\n"
"  --seshc-recipient-pkey <name>    Recipient to send the message as (must be 0x05 prefixed, 0x\n"
"                                   is optional)"
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

    if (!result.eaten && DN_Str8EqInsensitive(arg, DN_Str8Lit("--seshc-account-secret"))) {
      (void)DN_ArgShift(result.arg_ptr, result.arg_count);
      result.eaten         = true;
      args->account_secret = DN_Str8FromCStr8(DN_ArgShift(result.arg_ptr, result.arg_count));
    }

    if (!result.eaten && DN_Str8EqInsensitive(arg, DN_Str8Lit("--seshc-seed-node"))) {
      (void)DN_ArgShift(result.arg_ptr, result.arg_count);
      result.eaten        = true;
      args->seed_node_url = DN_Str8FromCStr8(DN_ArgShift(result.arg_ptr, result.arg_count));
    }

    if (!result.eaten && DN_Str8EqInsensitive(arg, DN_Str8Lit("--seshc-display-name"))) {
      (void)DN_ArgShift(result.arg_ptr, result.arg_count);
      result.eaten       = true;
      args->display_name = DN_Str8FromCStr8(DN_ArgShift(result.arg_ptr, result.arg_count));
    }

    if (!result.eaten && DN_Str8EqInsensitive(arg, DN_Str8Lit("--seshc-recipient-pkey"))) {
      (void)DN_ArgShift(result.arg_ptr, result.arg_count);
      result.eaten         = true;
      args->recipient_pkey = DN_Str8FromCStr8(DN_ArgShift(result.arg_ptr, result.arg_count));

      DN_Str8 check = DN_Str8TrimHexPrefix(args->recipient_pkey);
      if (!DN_Str8StartsWithSensitive(check, DN_Str8Lit("05"))) {
        result.error_msg     = DN_Str8Lit("Seshc recipient public key does not have a 05 prefix");
        result.error_arg     = args->recipient_pkey;
        args->recipient_pkey = {};
      }
    }
  }

  if (args->seed_node_url.count && args->account_secret.count && args->exe_path.count && args->display_name.count && args->recipient_pkey.count)
    args->valid = true;
  return result;
}

bool DN_SH_CLIArgsSendDMBlockingFmt(DN_SHCLIArgs *cli_args, DN_Str8 sesh_pkey_hex, char const *fmt, ...)
{
  bool result = false;
  if (cli_args->valid) {
    DN_TCScratch scratch = DN_TCScratchBeginArena(nullptr, 0);

    va_list args;
    va_start(args, fmt);
    DN_Str8 msg = DN_Str8FromFmtVArena(&scratch.arena, fmt, args);
    va_end(args);

    // NOTE: Escape quotes and quote the input strings
    DN_Str8 display_name = cli_args->display_name;
    display_name         = DN_Str8ReplaceSensitive(display_name, DN_Str8Lit("\""), DN_Str8Lit("\\\""), 0, &scratch.arena);
    display_name         = DN_Str8FromFmtArena(&scratch.arena, "\"%.*s\"", DN_Str8PrintFmt(display_name));

    msg = DN_Str8ReplaceSensitive(msg, DN_Str8Lit("\""), DN_Str8Lit("\\\""), 0, &scratch.arena);
    msg = DN_Str8FromFmtArena(&scratch.arena, "\"%.*s\"", DN_Str8PrintFmt(msg));

    DN_Str8 cmds[] = {
        cli_args->exe_path,
        DN_Str8Lit("--account-secret"),
        cli_args->account_secret,
        DN_Str8Lit("--seed-node-url"),
        cli_args->seed_node_url,
        DN_Str8Lit("--display-name"),
        display_name,
        DN_Str8Lit("send"),
        sesh_pkey_hex,
        msg,
    };

    DN_Str8Slice cmd_line = {};
    cmd_line.data         = cmds;
    cmd_line.count        = DN_ArrayCountU(cmds);

    DN_OSExecArgs exec_args  = {};
    exec_args.flags         |= DN_OSExecFlags_SaveOutput;

    DN_OSExecResult exec = DN_OS_Exec(cmd_line, &exec_args, &scratch.arena, nullptr);
    result               = exec.os_error_code == 0 && exec.exit_code == 0;
    if (!result) {
      DN_LogErrorF("Error sending message to %.*s (exit code: %d, os exit code: %d): %.*s",
                   DN_Str8PrintFmt(cli_args->recipient_pkey),
                   exec.exit_code,
                   exec.os_error_code,
                   DN_Str8PrintFmt(exec.stderr_text));
    }
    DN_TCScratchEnd(&scratch);

  }
  return result;
}

static int DN_SH_CLIArgsSendDMDetachedFmtThreadFunc_(DN_OSThread *thread)
{
  DN_SHSendDMThreadContext_ *context = DN_Cast(DN_SHSendDMThreadContext_ *) thread->user_context;
  DN_SH_CLIArgsSendDMBlockingFmt(&context->args, context->sesh_pkey_hex, "%.*s", DN_Str8PrintFmt(context->msg));
  DN_ArenaDeinit(&context->arena);
  return 0;
}

void DN_SH_CLIArgsSendDMDetachedFmt(DN_SHCLIArgs *cli_args, DN_Str8 sesh_pkey_hex, char const *fmt, ...)
{
  if (!cli_args->valid)
    return;

  // NOTE: Setup context
  DN_Arena arena                     = DN_OS_ArenaFromHeapVirtual(0, 0, DN_MemFlags_Nil);
  DN_SHSendDMThreadContext_ *context = DN_ArenaNewZ(&arena, DN_SHSendDMThreadContext_);
  context->args                      = DN_SH_CLIArgsCopy(cli_args, &arena);
  context->sesh_pkey_hex             = DN_Str8FromStr8Arena(sesh_pkey_hex, &arena);

  va_list args;
  va_start(args, fmt);
  context->msg  = DN_Str8FromFmtVArena(&arena, fmt, args);
  va_end(args);

  // NOTE: Set the arena last after all allocations
  context->arena = arena;

  // NOTE: Setup thread to be detached
  DN_OSThread         thread       = {};
  DN_OSThreadInitArgs thread_args  = DN_OS_ThreadInitArgsDefault();
  thread_args.flags               |= DN_OSThreadFlags_Detached;

  // NOTE: Execute and forget
  DN_OS_ThreadInit(&thread, DN_SH_CLIArgsSendDMDetachedFmtThreadFunc_, thread_args, context);
}
