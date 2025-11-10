#if !defined(DN_OS_POSIX_H)
#define DN_OS_POSIX_H

#if defined(_CLANGD)
  #include "../dn_base_inc.h"
#endif

#include <pthread.h>
#include <semaphore.h>

/*
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//    $$$$$$\   $$$$$$\        $$$$$$$\   $$$$$$\   $$$$$$\  $$$$$$\ $$\   $$\
//   $$  __$$\ $$  __$$\       $$  __$$\ $$  __$$\ $$  __$$\ \_$$  _|$$ |  $$ |
//   $$ /  $$ |$$ /  \__|      $$ |  $$ |$$ /  $$ |$$ /  \__|  $$ |  \$$\ $$  |
//   $$ |  $$ |\$$$$$$\        $$$$$$$  |$$ |  $$ |\$$$$$$\    $$ |   \$$$$  /
//   $$ |  $$ | \____$$\       $$  ____/ $$ |  $$ | \____$$\   $$ |   $$  $$<
//   $$ |  $$ |$$\   $$ |      $$ |      $$ |  $$ |$$\   $$ |  $$ |  $$  /\$$\
//    $$$$$$  |\$$$$$$  |      $$ |       $$$$$$  |\$$$$$$  |$$$$$$\ $$ /  $$ |
//    \______/  \______/       \__|       \______/  \______/ \______|\__|  \__|
//
//   dn_os_posix.h
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

struct DN_POSIXProcSelfStatus
{
  char   name[64];
  DN_U8  name_size;
  DN_U32 pid;
  DN_U64 vm_peak;
  DN_U32 vm_size;
};

// NOTE: The POSIX implementation disallows copies of synchronisation objects in
// general hence we have to dynamically allocate these primitives to maintain a
// consistent address.
//
//
// Source: The Open Group Base Specifications Issue 7, 2018 edition
// https://pubs.opengroup.org/onlinepubs/9699919799/functions/V2_chap02.html#tag_15_09_09
//
// 2.9.9 Synchronization Object Copies and Alternative Mappings
//
// For barriers, condition variables, mutexes, and read-write locks, [TSH]
// [Option Start]  if the process-shared attribute is set to
// PTHREAD_PROCESS_PRIVATE, [Option End]  only the synchronization object at the
// address used to initialize it can be used for performing synchronization. The
// effect of referring to another mapping of the same object when locking,
// unlocking, or destroying the object is undefined. [...] The effect of
// referring to a copy of the object when locking, unlocking, or destroying it
// is undefined.

enum DN_POSIXSyncPrimitiveType
{
  DN_OSPOSIXSyncPrimitiveType_Semaphore,
  DN_OSPOSIXSyncPrimitiveType_Mutex,
  DN_OSPOSIXSyncPrimitiveType_ConditionVariable,
};

struct DN_POSIXSyncPrimitive
{
  union
  {
    sem_t           sem;
    pthread_mutex_t mutex;
    pthread_cond_t  cv;
  };
  DN_POSIXSyncPrimitive *next;
};

struct DN_POSIXCore
{
  DN_POSIXSyncPrimitive *sync_primitive_free_list;
  pthread_mutex_t        sync_primitive_free_list_mutex;
  bool                   clock_monotonic_raw;
};

DN_API void                   DN_Posix_Init(DN_POSIXCore *posix);
DN_API void                   DN_Posix_ThreadSetName(DN_Str8 name);
DN_API DN_POSIXProcSelfStatus DN_Posix_ProcSelfStatus();
#endif // !defined(DN_OS_POSIX_H)
