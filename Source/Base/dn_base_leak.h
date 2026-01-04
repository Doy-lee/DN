#if !defined(DN_BASE_LEAK_H)
#define DN_BASE_LEAK_H

#if defined(_CLANGD)
  #include "../dn_base_inc.h"
#endif

enum DN_LeakAllocFlag
{
  DN_LeakAllocFlag_Freed         = 1 << 0,
  DN_LeakAllocFlag_LeakPermitted = 1 << 1,
};

struct DN_LeakAlloc
{
  void     *ptr;               // 8  Pointer to the allocation being tracked
  DN_USize  size;              // 16 Size of the allocation
  DN_USize  freed_size;        // 24 Store the size of the allocation when it is freed
  DN_Str8   stack_trace;       // 40 Stack trace at the point of allocation
  DN_Str8   freed_stack_trace; // 56 Stack trace of where the allocation was freed
  DN_U16    flags;             // 72 Bit flags from `DN_LeakAllocFlag`
};

// NOTE: We aim to keep the allocation record as light as possible as memory tracking can get
// expensive. Enforce that there is no unexpected padding.
DN_StaticAssert(sizeof(DN_LeakAlloc) == 64 || sizeof(DN_LeakAlloc) == 32); // NOTE: 64 bit vs 32 bit pointers respectively

struct DN_LeakTracker
{
  DN_DSMap<DN_LeakAlloc> alloc_table;
  DN_TicketMutex         alloc_table_mutex;
  DN_Arena               alloc_table_arena;
  DN_U64                 alloc_table_bytes_allocated_for_stack_traces;
};

DN_API void DN_LeakTrackAlloc_  (DN_LeakTracker *leak, void *ptr, DN_USize size, bool alloc_can_leak);
DN_API void DN_LeakTrackDealloc_(DN_LeakTracker *leak, void *ptr);
DN_API void DN_LeakDump_        (DN_LeakTracker *leak);

#if defined(DN_LEAK_TRACKING)
#define     DN_LeakTrackAlloc(leak, ptr, size, alloc_can_leak) DN_LeakTrackAlloc_(leak, ptr, size, alloc_can_leak)
#define     DN_LeakTrackDealloc(leak, ptr)                     DN_LeakTrackDealloc_(leak, ptr)
#define     DN_LeakDump(leak)                                  DN_LeakDump_(leak)
#else
#define     DN_LeakTrackAlloc(leak, ptr, size, alloc_can_leak) do { (void)ptr; (void)size; (void)alloc_can_leak; } while (0)
#define     DN_LeakTrackDealloc(leak, ptr)                     do { (void)ptr;                                   } while (0)
#define     DN_LeakDump(leak)                                  do {                                              } while (0)
#endif
#endif // DN_BASE_LEAK_H
