#if !defined(DN_OS_POSIX_H)
#define DN_OS_POSIX_H

#if defined(_CLANGD)
  #include "../dn.h"
#endif

#include <pthread.h>
#include <semaphore.h>

struct DN_OSPosixProcSelfStatus
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
enum DN_OSPosixSyncPrimitiveType
{
  DN_OSPosixSyncPrimitiveType_Semaphore,
  DN_OSPosixSyncPrimitiveType_Mutex,
  DN_OSPosixSyncPrimitiveType_ConditionVariable,
};

struct DN_OSPosixSyncPrimitive
{
  union
  {
    sem_t           sem;
    pthread_mutex_t mutex;
    pthread_cond_t  cv;
  };
  DN_OSPosixSyncPrimitive *next;
};

struct DN_OSPosixCore
{
  DN_OSPosixSyncPrimitive *sync_primitive_free_list;
  pthread_mutex_t          sync_primitive_free_list_mutex;
  bool                     clock_monotonic_raw;
};

DN_API void                     DN_OS_PosixInit          (DN_OSPosixCore *posix);
DN_API void                     DN_OS_PosixThreadSetName (DN_Str8 name);
DN_API DN_OSPosixProcSelfStatus DN_OS_PosixProcSelfStatus();
#endif // !defined(DN_OS_POSIX_H)
