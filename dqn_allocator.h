#pragma once
#include "dqn.h"

/*
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//    $$$$$$\  $$\       $$\       $$$$$$\   $$$$$$\   $$$$$$\ $$$$$$$$\  $$$$$$\  $$$$$$$\
//   $$  __$$\ $$ |      $$ |     $$  __$$\ $$  __$$\ $$  __$$\\__$$  __|$$  __$$\ $$  __$$\
//   $$ /  $$ |$$ |      $$ |     $$ /  $$ |$$ /  \__|$$ /  $$ |  $$ |   $$ /  $$ |$$ |  $$ |
//   $$$$$$$$ |$$ |      $$ |     $$ |  $$ |$$ |      $$$$$$$$ |  $$ |   $$ |  $$ |$$$$$$$  |
//   $$  __$$ |$$ |      $$ |     $$ |  $$ |$$ |      $$  __$$ |  $$ |   $$ |  $$ |$$  __$$<
//   $$ |  $$ |$$ |      $$ |     $$ |  $$ |$$ |  $$\ $$ |  $$ |  $$ |   $$ |  $$ |$$ |  $$ |
//   $$ |  $$ |$$$$$$$$\ $$$$$$$$\ $$$$$$  |\$$$$$$  |$$ |  $$ |  $$ |    $$$$$$  |$$ |  $$ |
//   \__|  \__|\________|\________|\______/  \______/ \__|  \__|  \__|    \______/ \__|  \__|
//
//   dqn_allocator.h -- Custom memory allocators
//
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// [$AREN] DN_Arena        -- Growing bump allocator
// [$CHUN] DN_Pool         -- Allocates reusable, free-able memory in PoT chunks
// [$POOL] DN_Pool         -- TODO
// [$ACAT] DN_ArenaCatalog -- Collate, create & manage arenas in a catalog
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

// NOTE: [$AREN] DN_Arena /////////////////////////////////////////////////////////////////////////
#if !defined(DN_ARENA_RESERVE_SIZE)
    #define DN_ARENA_RESERVE_SIZE DN_MEGABYTES(64)
#endif
#if !defined(DN_ARENA_COMMIT_SIZE)
    #define DN_ARENA_COMMIT_SIZE DN_KILOBYTES(64)
#endif

struct DN_ArenaBlock
{
    DN_ArenaBlock *prev;
    DN_U64        used;
    DN_U64        commit;
    DN_U64        reserve;
    DN_U64        reserve_sum;
};

typedef uint32_t DN_ArenaFlags;
enum DN_ArenaFlags_
{
    DN_ArenaFlags_Nil          = 0,
    DN_ArenaFlags_NoGrow       = 1 << 0,
    DN_ArenaFlags_NoPoison     = 1 << 1,
    DN_ArenaFlags_NoAllocTrack = 1 << 2,
    DN_ArenaFlags_AllocCanLeak = 1 << 3,
    DN_ArenaFlags_UserBuffer   = 1 << 4,
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

struct DN_Arena
{
    DN_ArenaBlock *curr;
    DN_ArenaStats  stats;
    DN_ArenaFlags  flags;
    DN_Str8        label;
    DN_Arena      *prev, *next;
};

struct DN_ArenaTempMem
{
    DN_Arena *arena;
    DN_U64   used_sum;
};

struct DN_ArenaTempMemScope
{
    DN_ArenaTempMemScope(DN_Arena *arena);
    ~DN_ArenaTempMemScope();
    DN_ArenaTempMem mem;
};

DN_USize const DN_ARENA_HEADER_SIZE = DN_AlignUpPowerOfTwo(sizeof(DN_Arena), 64);

// NOTE: [$CHUN] DN_Pool /////////////////////////////////////////////////////////////////////
#if !defined(DN_POOL_DEFAULT_ALIGN)
    #define DN_POOL_DEFAULT_ALIGN 16
#endif

struct DN_PoolSlot
{
    void         *data;
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
    uint8_t       align;
};

// NOTE: [$ACAT] DN_ArenaCatalog //////////////////////////////////////////////////////////////////
struct DN_ArenaCatalogItem
{
    DN_Arena            *arena;
    DN_Str8              label;
    bool                  arena_pool_allocated;
    DN_ArenaCatalogItem *next;
    DN_ArenaCatalogItem *prev;
};

struct DN_ArenaCatalog
{
    DN_TicketMutex      ticket_mutex; // Mutex for adding to the linked list of arenas
    DN_Pool            *pool;
    DN_ArenaCatalogItem sentinel;
    uint16_t             arena_count;
};

enum DN_ArenaCatalogFreeArena
{
    DN_ArenaCatalogFreeArena_No,
    DN_ArenaCatalogFreeArena_Yes,
};

// NOTE: [$AREN] DN_Arena /////////////////////////////////////////////////////////////////////////
DN_API DN_Arena             DN_Arena_InitBuffer                          (void *buffer, DN_USize size, DN_ArenaFlags flags);
DN_API DN_Arena             DN_Arena_InitSize                            (DN_U64 reserve, DN_U64 commit, DN_ArenaFlags flags);
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
#define                     DN_Arena_New_Frame(T, zero_mem)              (T *)DN_Arena_Alloc(DN_TLS_Get()->frame_arena, sizeof(T), alignof(T), zero_mem)
#define                     DN_Arena_New(arena, T, zero_mem)             (T *)DN_Arena_Alloc(arena,                      sizeof(T),            alignof(T), zero_mem)
#define                     DN_Arena_NewArray(arena, T, count, zero_mem) (T *)DN_Arena_Alloc(arena,                      sizeof(T)  * (count), alignof(T), zero_mem)
#define                     DN_Arena_NewCopy(arena, T, src)              (T *)DN_Arena_Copy (arena, (src),               sizeof(T),            alignof(T))
#define                     DN_Arena_NewArrayCopy(arena, T, src, count)  (T *)DN_Arena_Copy (arena, (src),               sizeof(T)  * (count), alignof(T))

// NOTE: [$CHUN] DN_Pool /////////////////////////////////////////////////////////////////////
DN_API DN_Pool              DN_Pool_Init                                 (DN_Arena *arena, uint8_t align);
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

// NOTE: [$ACAT] DN_ArenaCatalog //////////////////////////////////////////////////////////////////
DN_API void                 DN_ArenaCatalog_Init                         (DN_ArenaCatalog *catalog, DN_Pool *pool);
DN_API DN_ArenaCatalogItem *DN_ArenaCatalog_Find                         (DN_ArenaCatalog *catalog, DN_Str8 label);
DN_API void                 DN_ArenaCatalog_AddF                         (DN_ArenaCatalog *catalog, DN_Arena *arena, DN_FMT_ATTRIB char const *fmt, ...);
DN_API void                 DN_ArenaCatalog_AddFV                        (DN_ArenaCatalog *catalog, DN_Arena *arena, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API DN_Arena *           DN_ArenaCatalog_AllocFV                      (DN_ArenaCatalog *catalog, DN_USize reserve, DN_USize commit, uint8_t arena_flags, DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API DN_Arena *           DN_ArenaCatalog_AllocF                       (DN_ArenaCatalog *catalog, DN_USize reserve, DN_USize commit, uint8_t arena_flags, DN_FMT_ATTRIB char const *fmt, ...);
DN_API bool                 DN_ArenaCatalog_Erase                        (DN_ArenaCatalog *catalog, DN_Arena *arena, DN_ArenaCatalogFreeArena free_arena);
