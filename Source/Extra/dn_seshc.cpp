#include "dn_seshc.h"

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

bool DN_SH_CLIArgsSendDMFmt(DN_SHCLIArgs *cli_args, DN_Str8 sesh_pkey_hex, char const *fmt, ...)
{
  DN_TCScratch scratch = DN_TCScratchBeginArena(nullptr, 0);

  va_list args;
  va_start(args, fmt);
  DN_Str8 msg = DN_Str8FromFmtVArena(&scratch.arena, fmt, args);
  va_end(args);

  DN_Str8 cmds[] = {
      cli_args->exe_path,
      DN_Str8Lit("--account-secret"),
      cli_args->account_secret,
      DN_Str8Lit("--seed-node-url"),
      cli_args->seed_node_url,
      DN_Str8Lit("--display-name"),
      DN_Str8FromFmtArena(&scratch.arena, "\"%.*s\"", DN_Str8PrintFmt(cli_args->display_name)),
      DN_Str8Lit("send"),
      sesh_pkey_hex,
      msg,
  };

  DN_Str8Slice cmd_line = {};
  cmd_line.data         = cmds;
  cmd_line.count        = DN_ArrayCountU(cmds);

  DN_OSExecArgs   exec_args  = {};
  exec_args.flags           |= DN_OSExecFlags_SaveOutput;
  DN_OSExecResult exec       = DN_OS_Exec(cmd_line, &exec_args, &scratch.arena, nullptr);
  DN_LogInfoF("OUT (exit code:%zu, os: %zu): %.*s", exec.exit_code, exec.os_error_code, DN_Str8PrintFmt(exec.stdout_text));
  DN_LogInfoF("ERR: %.*s", DN_Str8PrintFmt(exec.stderr_text));
  DN_TCScratchEnd(&scratch);

  bool result = exec.os_error_code == 0 && exec.exit_code == 0;
  return result;
}
