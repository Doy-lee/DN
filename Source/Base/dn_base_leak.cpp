#define DN_BASE_LEAK_CPP

#if defined(_CLANGD)
#include "../dn.h"
#endif

DN_API void DN_LeakTrackAlloc_(DN_LeakTracker *leak, void *ptr, DN_USize size, bool leak_permitted)
{
  if (!ptr)
    return;

  DN_TicketMutex_Begin(&leak->alloc_table_mutex);
  DN_DEFER
  {
    DN_TicketMutex_End(&leak->alloc_table_mutex);
  };

  // NOTE: If the entry was not added, we are reusing a pointer that has been freed.
  // TODO: Add API for always making the item but exposing a var to indicate if the item was newly created or it
  // already existed.
  DN_Str8                      stack_trace = DN_StackTraceWalkStr8FromHeap(128, 3 /*skip*/);
  DN_DSMap<DN_LeakAlloc>      *alloc_table = &leak->alloc_table;
  DN_DSMapResult<DN_LeakAlloc> alloc_entry = DN_DSMap_MakeKeyU64(alloc_table, DN_Cast(DN_U64) ptr);
  DN_LeakAlloc                *alloc       = alloc_entry.value;
  if (alloc_entry.found) {
    if ((alloc->flags & DN_LeakAllocFlag_Freed) == 0) {
      DN_Str8x32 alloc_size     = DN_ByteCountStr8x32(alloc->size);
      DN_Str8x32 new_alloc_size = DN_ByteCountStr8x32(size);
      DN_HardAssertF(
          alloc->flags & DN_LeakAllocFlag_Freed,
          "This pointer is already in the leak tracker, however it has not been freed yet. This "
          "same pointer is being ask to be tracked twice in the allocation table, e.g. one if its "
          "previous free calls has not being marked freed with an equivalent call to "
          "DN_LeakTrackDealloc()\n"
          "\n"
          "The pointer (0x%p) originally allocated %.*s at:\n"
          "\n"
          "%.*s\n"
          "\n"
          "The pointer is allocating %.*s again at:\n"
          "\n"
          "%.*s\n",
          ptr,
          DN_Str8PrintFmt(alloc_size),
          DN_Str8PrintFmt(alloc->stack_trace),
          DN_Str8PrintFmt(new_alloc_size),
          DN_Str8PrintFmt(stack_trace));
    }

    // NOTE: Pointer was reused, clean up the prior entry
    leak->alloc_table_bytes_allocated_for_stack_traces -= alloc->stack_trace.size;
    leak->alloc_table_bytes_allocated_for_stack_traces -= alloc->freed_stack_trace.size;

    DN_OS_MemDealloc(alloc->stack_trace.data);
    DN_OS_MemDealloc(alloc->freed_stack_trace.data);
    *alloc = {};
  }

  alloc->ptr         = ptr;
  alloc->size        = size;
  alloc->stack_trace = stack_trace;
  alloc->flags |= leak_permitted ? DN_LeakAllocFlag_LeakPermitted : 0;
  leak->alloc_table_bytes_allocated_for_stack_traces += alloc->stack_trace.size;
}

DN_API void DN_LeakTrackDealloc_(DN_LeakTracker *leak, void *ptr)
{
  if (!ptr)
    return;

  DN_TicketMutex_Begin(&leak->alloc_table_mutex);
  DN_DEFER
  {
    DN_TicketMutex_End(&leak->alloc_table_mutex);
  };

  DN_Str8                      stack_trace = DN_StackTraceWalkStr8FromHeap(128, 3 /*skip*/);
  DN_DSMap<DN_LeakAlloc>      *alloc_table = &leak->alloc_table;
  DN_DSMapResult<DN_LeakAlloc> alloc_entry = DN_DSMap_FindKeyU64(alloc_table, DN_Cast(uintptr_t) ptr);
  DN_HardAssertF(alloc_entry.found,
                 "Allocated pointer can not be removed as it does not exist in the "
                 "allocation table. When this memory was allocated, the pointer was "
                 "not added to the allocation table [ptr=%p]",
                 ptr);

  DN_LeakAlloc *alloc = alloc_entry.value;
  if (alloc->flags & DN_LeakAllocFlag_Freed) {
    DN_Str8x32 freed_size = DN_ByteCountStr8x32(alloc->freed_size);
    DN_HardAssertF((alloc->flags & DN_LeakAllocFlag_Freed) == 0,
                   "Double free detected, pointer to free was already marked "
                   "as freed. Either the pointer was reallocated but not "
                   "traced, or, the pointer was freed twice.\n"
                   "\n"
                   "The pointer (0x%p) originally allocated %.*s at:\n"
                   "\n"
                   "%.*s\n"
                   "\n"
                   "The pointer was freed at:\n"
                   "\n"
                   "%.*s\n"
                   "\n"
                   "The pointer is being freed again at:\n"
                   "\n"
                   "%.*s\n",
                   ptr,
                   DN_Str8PrintFmt(freed_size),
                   DN_Str8PrintFmt(alloc->stack_trace),
                   DN_Str8PrintFmt(alloc->freed_stack_trace),
                   DN_Str8PrintFmt(stack_trace));
  }

  DN_Assert(alloc->freed_stack_trace.size == 0);
  alloc->flags |= DN_LeakAllocFlag_Freed;
  alloc->freed_stack_trace = stack_trace;
  leak->alloc_table_bytes_allocated_for_stack_traces += alloc->freed_stack_trace.size;
}

DN_API void DN_LeakDump_(DN_LeakTracker *leak)
{
  DN_U64 leak_count   = 0;
  DN_U64 leaked_bytes = 0;
  for (DN_USize index = 1; index < leak->alloc_table.occupied; index++) {
    DN_DSMapSlot<DN_LeakAlloc> *slot           = leak->alloc_table.slots + index;
    DN_LeakAlloc               *alloc          = &slot->value;
    bool                        alloc_leaked   = (alloc->flags & DN_LeakAllocFlag_Freed) == 0;
    bool                        leak_permitted = (alloc->flags & DN_LeakAllocFlag_LeakPermitted);
    if (alloc_leaked && !leak_permitted) {
      leaked_bytes += alloc->size;
      leak_count++;
      DN_Str8x32 alloc_size = DN_ByteCountStr8x32(alloc->size);
      DN_LOG_WarningF(
          "Pointer (0x%p) leaked %.*s at:\n"
          "%.*s",
          alloc->ptr,
          DN_Str8PrintFmt(alloc_size),
          DN_Str8PrintFmt(alloc->stack_trace));
    }
  }

  if (leak_count) {
    DN_Str8x32 leak_size = DN_ByteCountStr8x32(leaked_bytes);
    DN_LOG_WarningF("There were %I64u leaked allocations totalling %.*s", leak_count, DN_Str8PrintFmt(leak_size));
  }
}
