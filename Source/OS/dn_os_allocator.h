#if !defined(DN_OS_ALLOCATOR_H)
#define DN_OS_ALLOCATOR_H

DN_API DN_Arena DN_ArenaFromHeap(DN_U64 size, DN_ArenaFlags flags);
DN_API DN_Arena DN_ArenaFromVMem(DN_U64 reserve, DN_U64 commit, DN_ArenaFlags flags);

#endif // !defined(DN_OS_ALLOCATOR_H)
