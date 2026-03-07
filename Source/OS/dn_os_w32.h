#if !defined(DN_OS_W32_H)
#define DN_OS_W32_H

#if defined(_CLANGD)
  #define DN_H_WITH_OS 1
  #include "../dn.h"
  #include "dn_os_windows.h"
#endif

struct DN_OSW32Error
{
  unsigned long code;
  DN_Str8       msg;
};

struct DN_OSW32FolderIteratorW
{
  void    *handle;
  DN_Str16 file_name;
  wchar_t  file_name_buf[512];
};

enum DN_OSW32SyncPrimitiveType
{
  DN_OSW32SyncPrimitiveType_Semaphore,
  DN_OSW32SyncPrimitiveType_Mutex,
  DN_OSW32SyncPrimitiveType_ConditionVariable,
  DN_OSW32SyncPrimitiveType_Barrier,
};

struct DN_OSW32SyncPrimitive
{
  union
  {
    void                   *sem;
    CRITICAL_SECTION        mutex;
    CONDITION_VARIABLE      cv;
    SYNCHRONIZATION_BARRIER barrier;
  };

  DN_OSW32SyncPrimitive *next;
};

typedef HRESULT DN_OSW32SetThreadDescriptionFunc(HANDLE hThread, PWSTR const lpThreadDescription);
struct DN_OSW32Core
{
  DN_OSW32SetThreadDescriptionFunc *set_thread_description;
  LARGE_INTEGER                   qpc_frequency;
  void                           *bcrypt_rng_handle;
  bool                            bcrypt_init_success;
  bool                            sym_initialised;

  CRITICAL_SECTION                sync_primitive_free_list_mutex;
  DN_OSW32SyncPrimitive            *sync_primitive_free_list;
};

DN_API DN_OSW32Core* DN_OS_W32GetCore              ();
DN_API void          DN_OS_W32ThreadSetName        (DN_Str8 name);

DN_API DN_Str16      DN_OS_W32ErrorCodeToMsg16Alloc(DN_U32 error_code);
DN_API DN_OSW32Error DN_OS_W32ErrorCodeToMsg       (DN_Arena *arena, DN_U32 error_code);
DN_API DN_OSW32Error DN_OS_W32ErrorCodeToMsgAlloc  (DN_U32 error_code);
DN_API DN_OSW32Error DN_OS_W32LastError            (DN_Arena *arena);
DN_API DN_OSW32Error DN_OS_W32LastErrorAlloc       ();
DN_API void          DN_OS_W32MakeProcessDPIAware  ();

// NOTE: Windows Str8 <-> Str16
DN_API DN_Str16      DN_OS_W32Str8ToStr16          (DN_Arena *arena, DN_Str8 src);
DN_API int           DN_OS_W32Str8ToStr16Buffer    (DN_Str16 src, char *dest, int dest_size);
DN_API DN_Str8       DN_OS_W32Str16ToStr8          (DN_Arena *arena, DN_Str16 src);
DN_API int           DN_OS_W32Str16ToStr8Buffer    (DN_Str16 src, char *dest, int dest_size);
DN_API DN_Str8       DN_OS_W32Str16ToStr8FromHeap  (DN_Str16 src);

// NOTE: Path navigation
DN_API DN_Str16      DN_OS_W32EXEPathW             (DN_Arena *arena);
DN_API DN_Str16      DN_OS_W32EXEDirW              (DN_Arena *arena);
DN_API DN_Str8       DN_OS_W32WorkingDir           (DN_Arena *arena, DN_Str8 suffix);
DN_API DN_Str16      DN_OS_W32WorkingDirW          (DN_Arena *arena, DN_Str16 suffix);
DN_API bool          DN_OS_W32DirWIterate          (DN_Str16 path, DN_OSW32FolderIteratorW *it);
#endif // !defined(DN_OS_W32_H)
