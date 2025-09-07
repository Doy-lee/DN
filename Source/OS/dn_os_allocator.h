#if !defined(DN_OS_ALLOCATOR_H)
#define DN_OS_ALLOCATOR_H

DN_API DN_Arena DN_Arena_FromHeap(DN_U64 size, DN_ArenaFlags flags);
DN_API DN_Arena DN_Arena_FromVMem(DN_U64 reserve, DN_U64 commit, DN_ArenaFlags flags);

#endif // !defined(DN_OS_ALLOCATOR_H)
