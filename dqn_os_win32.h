#pragma once
#include "dqn.h"

/*
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//    $$$$$$\   $$$$$$\        $$\      $$\ $$$$$$\ $$\   $$\  $$$$$$\   $$$$$$\
//   $$  __$$\ $$  __$$\       $$ | $\  $$ |\_$$  _|$$$\  $$ |$$ ___$$\ $$  __$$\
//   $$ /  $$ |$$ /  \__|      $$ |$$$\ $$ |  $$ |  $$$$\ $$ |\_/   $$ |\__/  $$ |
//   $$ |  $$ |\$$$$$$\        $$ $$ $$\$$ |  $$ |  $$ $$\$$ |  $$$$$ /  $$$$$$  |
//   $$ |  $$ | \____$$\       $$$$  _$$$$ |  $$ |  $$ \$$$$ |  \___$$\ $$  ____/
//   $$ |  $$ |$$\   $$ |      $$$  / \$$$ |  $$ |  $$ |\$$$ |$$\   $$ |$$ |
//    $$$$$$  |\$$$$$$  |      $$  /   \$$ |$$$$$$\ $$ | \$$ |\$$$$$$  |$$$$$$$$\
//    \______/  \______/       \__/     \__|\______|\__|  \__| \______/ \________|
//
//   dqn_os_win32.h -- Windows only functions, and, implementation of the OS layer
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

struct Dqn_WinError
{
    unsigned long code;
    Dqn_Str8 msg;
};

// NOTE: Windows Str8 <-> Str16 ///////////////////////////////////////////
struct Dqn_Win_FolderIteratorW
{
    void      *handle;
    Dqn_Str16  file_name;
    wchar_t    file_name_buf[512];
};

struct Dqn_Win_FolderIterator
{
    void     *handle;
    Dqn_Str8  file_name;
    char      file_name_buf[512];
};

// NOTE: [$WIND] Dqn_Win ///////////////////////////////////////////////////////////////////////////
DQN_API Dqn_WinError Dqn_Win_ErrorCodeToMsg     (Dqn_Arena *arena, uint32_t error_code);
DQN_API Dqn_WinError Dqn_Win_LastError          (Dqn_Arena *arena);
DQN_API void         Dqn_Win_MakeProcessDPIAware();
// NOTE: Windows Str8 <-> Str16 ////////////////////////////////////////////////////////////////////
DQN_API Dqn_Str16    Dqn_Win_Str8ToStr16        (Dqn_Arena *arena, Dqn_Str8 src);
DQN_API int          Dqn_Win_Str8ToStr16Buffer  (Dqn_Str16 src, char *dest, int dest_size);
DQN_API Dqn_Str8     Dqn_Win_Str16ToStr8        (Dqn_Arena *arena, Dqn_Str16 src);
DQN_API int          Dqn_Win_Str16ToStr8Buffer  (Dqn_Str16 src, char *dest, int dest_size);
// NOTE: Path navigation ///////////////////////////////////////////////////////////////////////////
DQN_API Dqn_Str16    Dqn_Win_EXEPathW           (Dqn_Arena *arena);
DQN_API Dqn_Str16    Dqn_Win_EXEDirW            (Dqn_Arena *arena);
DQN_API Dqn_Str8     Dqn_Win_WorkingDir         (Dqn_Arena *arena, Dqn_Str8 suffix);
DQN_API Dqn_Str16    Dqn_Win_WorkingDirW        (Dqn_Arena *arena, Dqn_Str16 suffix);
DQN_API bool         Dqn_Win_FolderIterate      (Dqn_Str8 path, Dqn_Win_FolderIterator *it);
DQN_API bool         Dqn_Win_FolderWIterate     (Dqn_Str16 path, Dqn_Win_FolderIteratorW *it);
