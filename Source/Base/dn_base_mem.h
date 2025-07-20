#if !defined(DN_BASE_MEM_H)
#define DN_BASE_MEM_H

#include "../dn_clangd.h"

enum DN_MemCommit
{
  DN_MemCommit_No,
  DN_MemCommit_Yes,
};

typedef DN_U32 DN_MemPage;
enum DN_MemPage_
{
  // Exception on read/write with a page. This flag overrides the read/write
  // access.
  DN_MemPage_NoAccess = 1 << 0,

  DN_MemPage_Read = 1 << 1, // Only read permitted on the page.

  // Only write permitted on the page. On Windows this is not supported and
  // will be promoted to read+write permissions.
  DN_MemPage_Write = 1 << 2,

  DN_MemPage_ReadWrite = DN_MemPage_Read | DN_MemPage_Write,

  // Modifier used in conjunction with previous flags. Raises exception on
  // first access to the page, then, the underlying protection flags are
  // active. This is supported on Windows, on other OS's using this flag will
  // set the OS equivalent of DN_MemPage_NoAccess.
  // This flag must only be used in DN_Mem_Protect
  DN_MemPage_Guard = 1 << 3,

  // If leak tracing is enabled, this flag will allow the allocation recorded
  // from the reserve call to be leaked, e.g. not printed when leaks are
  // dumped to the console.
  DN_MemPage_AllocRecordLeakPermitted = 1 << 4,

  // If leak tracing is enabled this flag will prevent any allocation record
  // from being created in the allocation table at all. If this flag is
  // enabled, 'OSMemPage_AllocRecordLeakPermitted' has no effect since the
  // record will never be created.
  DN_MemPage_NoAllocRecordEntry = 1 << 5,

  // [INTERNAL] Do not use. All flags together do not constitute a correct
  // configuration of pages.
  DN_MemPage_All = DN_MemPage_NoAccess |
                     DN_MemPage_ReadWrite |
                     DN_MemPage_Guard |
                     DN_MemPage_AllocRecordLeakPermitted |
                     DN_MemPage_NoAllocRecordEntry,
};

#if !defined(DN_ARENA_RESERVE_SIZE)
  #define DN_ARENA_RESERVE_SIZE DN_Megabytes(64)
#endif

#if !defined(DN_ARENA_COMMIT_SIZE)
  #define DN_ARENA_COMMIT_SIZE DN_Kilobytes(64)
#endif

struct DN_ArenaBlock
{
  DN_ArenaBlock *prev;
  DN_U64         used;
  DN_U64         commit;
  DN_U64         reserve;
  DN_U64         reserve_sum;
};

typedef uint32_t DN_ArenaFlags;

enum DN_ArenaFlags_
{
  DN_ArenaFlags_Nil          = 0,
  DN_ArenaFlags_NoGrow       = 1 << 0,
  DN_ArenaFlags_NoPoison     = 1 << 1,
  DN_ArenaFlags_NoAllocTrack = 1 << 2,
  DN_ArenaFlags_AllocCanLeak = 1 << 3,

  // NOTE: Internal flags. Do not use
  DN_ArenaFlags_UserBuffer   = 1 << 4,
  DN_ArenaFlags_MemFuncs     = 1 << 5,
};

struct DN_ArenaInfo
{
  DN_U64 used;
  DN_U64 commit;
  DN_U64 reserve;
  DN_U64 blocks;
};

struct DN_ArenaStats
{
  DN_ArenaInfo info;
  DN_ArenaInfo hwm;
};

enum DN_ArenaMemFuncType
{
  DN_ArenaMemFuncType_Nil,
  DN_ArenaMemFuncType_Basic,
  DN_ArenaMemFuncType_VMem,
};

typedef void *(DN_ArenaMemBasicAllocFunc)(DN_USize size);
typedef void  (DN_ArenaMemBasicDeallocFunc)(void *ptr);
typedef void *(DN_ArenaMemVMemReserveFunc)(DN_USize size, DN_MemCommit commit, DN_MemPage page_flags);
typedef bool  (DN_ArenaMemVMemCommitFunc)(void *ptr, DN_USize size, DN_U32 page_flags);
typedef void  (DN_ArenaMemVMemReleaseFunc)(void *ptr, DN_USize size);
struct DN_ArenaMemFuncs
{
  DN_ArenaMemFuncType          type;
  DN_ArenaMemBasicAllocFunc   *basic_alloc;
  DN_ArenaMemBasicDeallocFunc *basic_dealloc;

  DN_U32                       vmem_page_size;
  DN_ArenaMemVMemReserveFunc  *vmem_reserve;
  DN_ArenaMemVMemCommitFunc   *vmem_commit;
  DN_ArenaMemVMemReleaseFunc  *vmem_release;
};

struct DN_Arena
{
  DN_ArenaMemFuncs mem_funcs;
  DN_ArenaBlock   *curr;
  DN_ArenaStats    stats;
  DN_ArenaFlags    flags;
  DN_Str8          label;
  DN_Arena        *prev, *next;
};

struct DN_ArenaTempMem
{
  DN_Arena *arena;
  DN_U64    used_sum;
};

struct DN_ArenaTempMemScope
{
  DN_ArenaTempMemScope(DN_Arena *arena);
  ~DN_ArenaTempMemScope();
  DN_ArenaTempMem mem;
};

DN_USize const DN_ARENA_HEADER_SIZE = DN_AlignUpPowerOfTwo(sizeof(DN_Arena), 64);

// NOTE: DN_Arena //////////////////////////////////////////////////////////////////////////////////
DN_API DN_Arena             DN_Arena_InitFromBuffer                      (void *buffer, DN_USize size, DN_ArenaFlags flags);
DN_API DN_Arena             DN_Arena_InitFromMemFuncs                    (DN_U64 reserve, DN_U64 commit, DN_ArenaFlags flags, DN_ArenaMemFuncs mem_funcs);
DN_API void                 DN_Arena_Deinit                              (DN_Arena *arena);
DN_API bool                 DN_Arena_Commit                              (DN_Arena *arena, DN_U64 size);
DN_API bool                 DN_Arena_CommitTo                            (DN_Arena *arena, DN_U64 pos);
DN_API bool                 DN_Arena_Grow                                (DN_Arena *arena, DN_U64 reserve, DN_U64 commit);
DN_API void *               DN_Arena_Alloc                               (DN_Arena *arena, DN_U64 size, uint8_t align, DN_ZeroMem zero_mem);
DN_API void *               DN_Arena_AllocContiguous                     (DN_Arena *arena, DN_U64 size, uint8_t align, DN_ZeroMem zero_mem);
DN_API void *               DN_Arena_Copy                                (DN_Arena *arena, void const *data, DN_U64 size, uint8_t align);
DN_API void                 DN_Arena_PopTo                               (DN_Arena *arena, DN_U64 init_used);
DN_API void                 DN_Arena_Pop                                 (DN_Arena *arena, DN_U64 amount);
DN_API DN_U64               DN_Arena_Pos                                 (DN_Arena const *arena);
DN_API void                 DN_Arena_Clear                               (DN_Arena *arena);
DN_API bool                 DN_Arena_OwnsPtr                             (DN_Arena const *arena, void *ptr);
DN_API DN_ArenaStats        DN_Arena_SumStatsArray                       (DN_ArenaStats const *array, DN_USize size);
DN_API DN_ArenaStats        DN_Arena_SumStats                            (DN_ArenaStats lhs, DN_ArenaStats rhs);
DN_API DN_ArenaStats        DN_Arena_SumArenaArrayToStats                (DN_Arena const *array, DN_USize size);
DN_API DN_ArenaTempMem      DN_Arena_TempMemBegin                        (DN_Arena *arena);
DN_API void                 DN_Arena_TempMemEnd                          (DN_ArenaTempMem mem);
#define                     DN_Arena_New_Frame(T, zero_mem)              (T *)DN_Arena_Alloc(DN_OS_TLSGet()->frame_arena, sizeof(T), alignof(T), zero_mem)
#define                     DN_Arena_New(arena, T, zero_mem)             (T *)DN_Arena_Alloc(arena,                      sizeof(T),            alignof(T), zero_mem)
#define                     DN_Arena_NewArray(arena, T, count, zero_mem) (T *)DN_Arena_Alloc(arena,                      sizeof(T)  * (count), alignof(T), zero_mem)
#define                     DN_Arena_NewCopy(arena, T, src)              (T *)DN_Arena_Copy (arena, (src),               sizeof(T),            alignof(T))
#define                     DN_Arena_NewArrayCopy(arena, T, src, count)  (T *)DN_Arena_Copy (arena, (src),               sizeof(T)  * (count), alignof(T))

#if !defined(DN_POOL_DEFAULT_ALIGN)
    #define DN_POOL_DEFAULT_ALIGN 16
#endif

struct DN_PoolSlot
{
  void        *data;
  DN_PoolSlot *next;
};

enum DN_PoolSlotSize
{
  DN_PoolSlotSize_32B,
  DN_PoolSlotSize_64B,
  DN_PoolSlotSize_128B,
  DN_PoolSlotSize_256B,
  DN_PoolSlotSize_512B,
  DN_PoolSlotSize_1KiB,
  DN_PoolSlotSize_2KiB,
  DN_PoolSlotSize_4KiB,
  DN_PoolSlotSize_8KiB,
  DN_PoolSlotSize_16KiB,
  DN_PoolSlotSize_32KiB,
  DN_PoolSlotSize_64KiB,
  DN_PoolSlotSize_128KiB,
  DN_PoolSlotSize_256KiB,
  DN_PoolSlotSize_512KiB,
  DN_PoolSlotSize_1MiB,
  DN_PoolSlotSize_2MiB,
  DN_PoolSlotSize_4MiB,
  DN_PoolSlotSize_8MiB,
  DN_PoolSlotSize_16MiB,
  DN_PoolSlotSize_32MiB,
  DN_PoolSlotSize_64MiB,
  DN_PoolSlotSize_128MiB,
  DN_PoolSlotSize_256MiB,
  DN_PoolSlotSize_512MiB,
  DN_PoolSlotSize_1GiB,
  DN_PoolSlotSize_2GiB,
  DN_PoolSlotSize_4GiB,
  DN_PoolSlotSize_8GiB,
  DN_PoolSlotSize_16GiB,
  DN_PoolSlotSize_32GiB,
  DN_PoolSlotSize_Count,
};

struct DN_Pool
{
  DN_Arena    *arena;
  DN_PoolSlot *slots[DN_PoolSlotSize_Count];
  DN_U8        align;
};

// NOTE: DN_Pool ///////////////////////////////////////////////////////////////////////////////////
DN_API DN_Pool              DN_Pool_Init                                 (DN_Arena *arena, DN_U8 align);
DN_API bool                 DN_Pool_IsValid                              (DN_Pool const *pool);
DN_API void *               DN_Pool_Alloc                                (DN_Pool *pool, DN_USize size);
DN_API DN_Str8              DN_Pool_AllocStr8FV                          (DN_Pool *pool, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API DN_Str8              DN_Pool_AllocStr8F                           (DN_Pool *pool, DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_Str8              DN_Pool_AllocStr8Copy                        (DN_Pool *pool, DN_Str8 string);
DN_API void                 DN_Pool_Dealloc                              (DN_Pool *pool, void *ptr);
DN_API void *               DN_Pool_Copy                                 (DN_Pool *pool, void const *data, DN_U64 size, uint8_t align);

#define                     DN_Pool_New(pool, T)                         (T *)DN_Pool_Alloc(pool, sizeof(T))
#define                     DN_Pool_NewArray(pool, T, count)             (T *)DN_Pool_Alloc(pool, count * sizeof(T))
#define                     DN_Pool_NewCopy(arena, T, src)               (T *)DN_Pool_Copy (arena, (src), sizeof(T),            alignof(T))
#define                     DN_Pool_NewArrayCopy(arena, T, src, count)   (T *)DN_Pool_Copy (arena, (src), sizeof(T)  * (count), alignof(T))

// NOTE: DN_Debug //////////////////////////////////////////////////////////////////////////////////
#if defined(DN_LEAK_TRACKING) && !defined(DN_FREESTANDING)
DN_API void                 DN_Debug_TrackAlloc  (void *ptr, DN_USize size, bool alloc_can_leak);
DN_API void                 DN_Debug_TrackDealloc(void *ptr);
DN_API void                 DN_Debug_DumpLeaks   ();
#else
#define                     DN_Debug_TrackAlloc(ptr, size, alloc_can_leak) do { (void)ptr; (void)size; (void)alloc_can_leak; } while (0)
#define                     DN_Debug_TrackDealloc(ptr)                     do { (void)ptr;                                   } while (0)
#define                     DN_Debug_DumpLeaks()                           do {                                              } while (0)
#endif
#endif // !defined(DN_BASE_MEM_H)
