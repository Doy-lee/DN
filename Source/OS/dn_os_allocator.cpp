#define DN_OS_ALLOCATOR_CPP

static void *DN_Arena_BasicAllocFromOSHeap(DN_USize size)
{
  void *result = DN_OS_MemAlloc(size, DN_ZeroMem_Yes);
  return result;
}

DN_API DN_Arena DN_Arena_InitFromOSHeap(DN_U64 size, DN_ArenaFlags flags)
{
  DN_ArenaMemFuncs mem_funcs = {};
  mem_funcs.type             = DN_ArenaMemFuncType_Basic;
  mem_funcs.basic_alloc      = DN_Arena_BasicAllocFromOSHeap;
  mem_funcs.basic_dealloc    = DN_OS_MemDealloc;
  DN_Arena result            = DN_Arena_InitFromMemFuncs(size, size, flags, mem_funcs);
  return result;
}

DN_API DN_Arena DN_Arena_InitFromOSVMem(DN_U64 reserve, DN_U64 commit, DN_ArenaFlags flags)
{
  DN_ArenaMemFuncs mem_funcs = {};
  mem_funcs.type             = DN_ArenaMemFuncType_VMem;
  mem_funcs.vmem_page_size   = g_dn_os_core_->page_size;
  mem_funcs.vmem_reserve     = DN_OS_MemReserve;
  mem_funcs.vmem_commit      = DN_OS_MemCommit;
  mem_funcs.vmem_release     = DN_OS_MemRelease;
  DN_Arena result            = DN_Arena_InitFromMemFuncs(reserve, commit, flags, mem_funcs);
  return result;
}

