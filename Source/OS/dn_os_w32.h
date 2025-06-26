#if !defined(DN_OS_WIN32_H)
#define DN_OS_WIN32_H

struct DN_W32Error
{
  unsigned long code;
  DN_Str8       msg;
};

struct DN_W32FolderIteratorW
{
  void    *handle;
  DN_Str16 file_name;
  wchar_t  file_name_buf[512];
};

enum DN_W32SyncPrimitiveType
{
  DN_OSW32SyncPrimitiveType_Semaphore,
  DN_OSW32SyncPrimitiveType_Mutex,
  DN_OSW32SyncPrimitiveType_ConditionVariable,
};

struct DN_W32SyncPrimitive
{
  union
  {
    void              *sem;
    CRITICAL_SECTION   mutex;
    CONDITION_VARIABLE cv;
  };

  DN_W32SyncPrimitive *next;
};

typedef HRESULT DN_W32SetThreadDescriptionFunc(HANDLE hThread, PWSTR const lpThreadDescription);
struct DN_W32Core
{
  DN_W32SetThreadDescriptionFunc *set_thread_description;
  LARGE_INTEGER                   qpc_frequency;
  void                           *bcrypt_rng_handle;
  bool                            bcrypt_init_success;
  bool                            sym_initialised;

  CRITICAL_SECTION                sync_primitive_free_list_mutex;
  DN_W32SyncPrimitive            *sync_primitive_free_list;
};

DN_API void        DN_W32_ThreadSetName      (DN_Str8 name);

DN_API DN_Str16    DN_W32_ErrorCodeToMsg16Alloc(uint32_t error_code);
DN_API DN_W32Error DN_W32_ErrorCodeToMsg       (DN_Arena *arena, uint32_t error_code);
DN_API DN_W32Error DN_W32_ErrorCodeToMsgAlloc  (uint32_t error_code);
DN_API DN_W32Error DN_W32_LastError            (DN_Arena *arena);
DN_API DN_W32Error DN_W32_LastErrorAlloc       ();
DN_API void        DN_W32_MakeProcessDPIAware  ();

// NOTE: Windows Str8 <-> Str16 ////////////////////////////////////////////////////////////////////
DN_API DN_Str16    DN_W32_Str8ToStr16        (DN_Arena *arena, DN_Str8 src);
DN_API int         DN_W32_Str8ToStr16Buffer  (DN_Str16 src, char *dest, int dest_size);
DN_API DN_Str8     DN_W32_Str16ToStr8        (DN_Arena *arena, DN_Str16 src);
DN_API int         DN_W32_Str16ToStr8Buffer  (DN_Str16 src, char *dest, int dest_size);
DN_API DN_Str8     DN_W32_Str16ToStr8FromHeap(DN_Str16 src);

// NOTE: Path navigation ///////////////////////////////////////////////////////////////////////////
DN_API DN_Str16    DN_W32_EXEPathW           (DN_Arena *arena);
DN_API DN_Str16    DN_W32_EXEDirW            (DN_Arena *arena);
DN_API DN_Str8     DN_W32_WorkingDir         (DN_Arena *arena, DN_Str8 suffix);
DN_API DN_Str16    DN_W32_WorkingDirW        (DN_Arena *arena, DN_Str16 suffix);
DN_API bool        DN_W32_DirWIterate        (DN_Str16 path, DN_W32FolderIteratorW *it);
#endif // !defined(DN_OS_WIN32)
