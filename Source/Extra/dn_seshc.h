#if !defined(DN_SESHC_H)
#define DN_SESHC_H

#if defined(_CLANGD)
  #define DN_WITH_OS 1
  #include "../dn.h"
#endif

struct DN_SHCLIArgs
{
  bool    valid;
  DN_Str8 exe_path;
  DN_Str8 ini_path;
  DN_Str8 seed_node_url;
};

struct DN_SHCLIEatArgResult
{
  bool         eaten;
  DN_Str8      error_msg; // Parsing error message. String is immutably stored in the data-segment w/ program lifetime.
  DN_Str8      error_arg; // Argument that caused the parsing error
  char const **arg_ptr;   // Next argument pointer that that the caller should proceed from
  DN_USize     arg_count; // Next argument count that that the caller should assume
};

struct DN_SHMsgSendData
{
  DN_Str8 recipient;      // Either `ini:<section.key>` or "05-prefix session account ID"
  DN_Str8 account_secret; // Seshc EXE --account-secret format
  DN_Str8 display_name;
};

struct DN_SHMsgSendAsyncHandle
{
  DN_OSThread thread;
};

enum DN_SHLogMode
{
  DN_SHLogMode_Blocking,
  DN_SHLogMode_Detached,
};

enum DN_SHMsgSendAsyncStatus
{
  DN_SHMsgSendAsyncStatus_Timeout,
  DN_SHMsgSendAsyncStatus_Success,
  DN_SHMsgSendAsyncStatus_Failed,
};


// NOTE: Seshc
// Overview
//   Helper library that provides some useful APIs for programatically calling the Seshc CLI
//   executable to send messages from your C/C++ application.

// API
//   DN_SH_MsgSendDetachedFmt
//     Send a 1-on-1 message with a background thread that is spawned on demand and detached. If the
//     application exits before this thread has completed sending the message, the message will not
//     be sent.

//   DN_SH_MsgSendBlockingFmt
//     Send a 1-on-1 message using the current executing thread, blocking control-flow until the
//     message has been sent. Returns true if the send was successful, false otherwise.

//   DN_SH_MsgSendAsyncFmt
//     Send a 1-on-1 message with a background thread that is spawned on demand. A handle to the
//     thread is returned that can be waited on.

DN_SHCLIArgs            DN_SH_CLIArgsCopy             (DN_SHCLIArgs const *src, DN_Arena *arena);
DN_Str8                 DN_SH_CLIOptionsStr8          ();
DN_SHCLIEatArgResult    DN_SH_CLIEatArg               (DN_SHCLIArgs *args, char const **arg_ptr, int arg_count);
bool                    DN_SH_MsgSendBlockingFmt      (DN_SHMsgSendData send, DN_SHCLIArgs *cli_args, char const *fmt, ...);
void                    DN_SH_MsgSendDetachedFmt      (DN_SHMsgSendData send, DN_SHCLIArgs *cli_args, char const *fmt, ...);
DN_SHMsgSendAsyncHandle DN_SH_MsgSendAsyncFmt         (DN_SHMsgSendData send, DN_SHCLIArgs *cli_args, DN_Arena *arena, char const *fmt, ...);
DN_SHMsgSendAsyncStatus DN_SH_MsgSendAsyncWait        (DN_SHMsgSendAsyncHandle *handle, DN_U32 timeout_ms);
void                    DN_SH_LogBroadcastFmtV        (DN_SHLogMode mode, DN_LogType type, DN_SHMsgSendData send, DN_SHCLIArgs *cli_args, DN_Str8 short_desc, char const *fmt, va_list args);
void                    DN_SH_LogBroadcastBlockingFmt (DN_LogType type, DN_SHMsgSendData send, DN_SHCLIArgs *cli_args, DN_Str8 short_desc, char const *fmt, ...);
void                    DN_SH_LogBroadcastDetachedFmt (DN_LogType type, DN_SHMsgSendData send, DN_SHCLIArgs *cli_args, DN_Str8 short_desc, char const *fmt, ...);

#endif // #if !defined(DN_SESHC_H)
