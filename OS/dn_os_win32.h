#if !defined(DN_OS_WIN32_H)
#define DN_OS_WIN32_H

struct DN_WinError
{
  unsigned long code;
  DN_Str8       msg;
};

// NOTE: Windows Str8 <-> Str16 ///////////////////////////////////////////
struct DN_Win_FolderIteratorW
{
  void    *handle;
  DN_Str16 file_name;
  wchar_t  file_name_buf[512];
};

DN_API void         DN_Win_ThreadSetName      (DN_Str8 name);

// NOTE: DN_Win ////////////////////////////////////////////////////////////////////////////////////
DN_API DN_Str16    DN_Win_ErrorCodeToMsg16Alloc(uint32_t error_code);
DN_API DN_WinError DN_Win_ErrorCodeToMsg       (DN_Arena *arena, uint32_t error_code);
DN_API DN_WinError DN_Win_ErrorCodeToMsgAlloc  (uint32_t error_code);
DN_API DN_WinError DN_Win_LastError            (DN_Arena *arena);
DN_API DN_WinError DN_Win_LastErrorAlloc       ();
DN_API void        DN_Win_MakeProcessDPIAware  ();

// NOTE: Windows Str8 <-> Str16 ////////////////////////////////////////////////////////////////////
DN_API DN_Str16    DN_Win_Str8ToStr16        (DN_Arena *arena, DN_Str8 src);
DN_API int         DN_Win_Str8ToStr16Buffer  (DN_Str16 src, char *dest, int dest_size);
DN_API DN_Str8     DN_Win_Str16ToStr8        (DN_Arena *arena, DN_Str16 src);
DN_API int         DN_Win_Str16ToStr8Buffer  (DN_Str16 src, char *dest, int dest_size);
DN_API DN_Str8     DN_Win_Str16ToStr8FromHeap(DN_Str16 src);

// NOTE: Path navigation ///////////////////////////////////////////////////////////////////////////
DN_API DN_Str16    DN_Win_EXEPathW           (DN_Arena *arena);
DN_API DN_Str16    DN_Win_EXEDirW            (DN_Arena *arena);
DN_API DN_Str8     DN_Win_WorkingDir         (DN_Arena *arena, DN_Str8 suffix);
DN_API DN_Str16    DN_Win_WorkingDirW        (DN_Arena *arena, DN_Str16 suffix);
DN_API bool        DN_Win_DirWIterate        (DN_Str16 path, DN_Win_FolderIteratorW *it);
#endif // !defined(DN_OS_WIN32)
