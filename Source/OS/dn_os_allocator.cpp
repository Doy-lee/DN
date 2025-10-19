#define DN_OS_ALLOCATOR_CPP

#include "../dn_base_inc.h"
#include "../dn_os_inc.h"

static void *DN_ArenaBasicAllocFromOSHeap(DN_USize size)
{
  void *result = DN_OS_MemAlloc(size, DN_ZMem_Yes);
  return result;
}

DN_API DN_Arena DN_ArenaFromHeap(DN_U64 size, DN_ArenaFlags flags)
{
  DN_ArenaMemFuncs mem_funcs = {};
  mem_funcs.type             = DN_ArenaMemFuncType_Basic;
  mem_funcs.basic_alloc      = DN_ArenaBasicAllocFromOSHeap;
  mem_funcs.basic_dealloc    = DN_OS_MemDealloc;
  DN_Arena result            = DN_ArenaFromMemFuncs(size, size, flags, mem_funcs);
  return result;
}

DN_API DN_Arena DN_ArenaFromVMem(DN_U64 reserve, DN_U64 commit, DN_ArenaFlags flags)
{
  DN_ArenaMemFuncs mem_funcs = {};
  mem_funcs.type             = DN_ArenaMemFuncType_VMem;
  mem_funcs.vmem_page_size   = g_dn_os_core_->page_size;
  mem_funcs.vmem_reserve     = DN_OS_MemReserve;
  mem_funcs.vmem_commit      = DN_OS_MemCommit;
  mem_funcs.vmem_release     = DN_OS_MemRelease;
  DN_Arena result            = DN_ArenaFromMemFuncs(reserve, commit, flags, mem_funcs);
  return result;
}

