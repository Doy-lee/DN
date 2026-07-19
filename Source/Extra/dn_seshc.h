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
  DN_Str8 account_secret;
  DN_Str8 seed_node_url;
  DN_Str8 display_name;
  DN_Str8 recipient_pkey;
};

struct DN_SHCLIEatArgResult
{
  bool         eaten;
  DN_Str8      error_msg; // Parsing error message. String is immutably stored in the data-segment w/ program lifetime.
  DN_Str8      error_arg; // Argument that caused the parsing error
  char const **arg_ptr;   // Next argument pointer that that the caller should proceed from
  DN_USize     arg_count; // Next argument count that that the caller should assume
};

// NOTE: Seshc
// Overview
//   Helper library that provides some useful APIs for programatically calling the Seshc CLI
//   executable to send messages from your C/C++ application.
DN_Str8              DN_SH_CLIOptionsStr8  ();
DN_SHCLIEatArgResult DN_SH_CLIEatArg       (DN_SHCLIArgs *args, char const **arg_ptr, int arg_count);
bool                 DN_SH_CLIArgsSendDMFmt(DN_SHCLIArgs *cli_args, DN_Str8 sesh_pkey_hex, char const *fmt, ...);

#endif // #if !defined(DN_SESHC_H)
