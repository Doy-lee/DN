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
//   dqn_allocator.cpp
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

// NOTE: [$AREN] DN_Arena /////////////////////////////////////////////////////////////////////////
DN_API DN_ArenaBlock *DN_Arena_BlockInit(DN_U64 reserve, DN_U64 commit, bool track_alloc, bool alloc_can_leak)
{
    DN_USize const page_size    = g_dn_core->os_page_size;
    DN_U64         real_reserve = reserve ? reserve : DN_ARENA_RESERVE_SIZE;
    DN_U64         real_commit  = commit  ? commit  : DN_ARENA_COMMIT_SIZE;
    real_reserve                = DN_AlignUpPowerOfTwo(real_reserve, page_size);
    real_commit                 = DN_MIN(DN_AlignUpPowerOfTwo(real_commit, page_size), real_reserve);

    DN_ASSERTF(DN_ARENA_HEADER_SIZE < real_commit && real_commit <= real_reserve, "%I64u < %I64u <= %I64u", DN_ARENA_HEADER_SIZE, real_commit, real_reserve);
    DN_ASSERTF(page_size, "Call DN_Library_Init() to initialise the known page size");

    DN_OSMemCommit  mem_commit = real_reserve == real_commit ? DN_OSMemCommit_Yes : DN_OSMemCommit_No;
    DN_ArenaBlock *result      = DN_CAST(DN_ArenaBlock *)DN_OS_MemReserve(real_reserve, mem_commit, DN_OSMemPage_ReadWrite);
    if (!result)
        return result;

    if (mem_commit == DN_OSMemCommit_No && !DN_OS_MemCommit(result, real_commit, DN_OSMemPage_ReadWrite)) {
        DN_OS_MemRelease(result, real_reserve);
        return result;
    }

    result->used    = DN_ARENA_HEADER_SIZE;
    result->commit  = real_commit;
    result->reserve = real_reserve;

    if (track_alloc)
        DN_Debug_TrackAlloc(result, result->reserve, alloc_can_leak);
    return result;
}

DN_API DN_ArenaBlock *DN_Arena_BlockInitFlags(DN_U64 reserve, DN_U64 commit, DN_ArenaFlags flags)
{
    bool            track_alloc    = (flags & DN_ArenaFlags_NoAllocTrack) == 0;
    bool            alloc_can_leak = flags & DN_ArenaFlags_AllocCanLeak;
    DN_ArenaBlock *result          = DN_Arena_BlockInit(reserve, commit, track_alloc, alloc_can_leak);
    if (result && ((flags & DN_ArenaFlags_NoPoison) == 0))
        DN_ASAN_PoisonMemoryRegion(DN_CAST(char *)result + DN_ARENA_HEADER_SIZE, result->commit - DN_ARENA_HEADER_SIZE);
    return result;
}

static void DN_Arena_UpdateStatsOnNewBlock_(DN_Arena *arena, DN_ArenaBlock const *block)
{
    DN_ASSERT(arena);
    if (block) {
        arena->stats.info.used     += block->used;
        arena->stats.info.commit   += block->commit;
        arena->stats.info.reserve  += block->reserve;
        arena->stats.info.blocks   += 1;

        arena->stats.hwm.used       = DN_MAX(arena->stats.hwm.used,    arena->stats.info.used);
        arena->stats.hwm.commit     = DN_MAX(arena->stats.hwm.commit,  arena->stats.info.commit);
        arena->stats.hwm.reserve    = DN_MAX(arena->stats.hwm.reserve, arena->stats.info.reserve);
        arena->stats.hwm.blocks     = DN_MAX(arena->stats.hwm.blocks,  arena->stats.info.blocks);
    }
}

DN_API DN_Arena DN_Arena_InitBuffer(void *buffer, DN_USize size, DN_ArenaFlags flags)
{
    DN_ASSERT(buffer);
    DN_ASSERTF(DN_ARENA_HEADER_SIZE < size, "Buffer (%zu bytes) too small, need atleast %zu bytes to store arena metadata", size, DN_ARENA_HEADER_SIZE);
    DN_ASSERTF(DN_IsPowerOfTwo(size), "Buffer (%zu bytes) must be a power-of-two", size);

    // NOTE: Init block
    DN_ArenaBlock *block = DN_CAST(DN_ArenaBlock *) buffer;
    block->commit         = size;
    block->reserve        = size;
    block->used           = DN_ARENA_HEADER_SIZE;
    if (block && ((flags & DN_ArenaFlags_NoPoison) == 0)) {
        DN_ASAN_PoisonMemoryRegion(DN_CAST(char *)block + DN_ARENA_HEADER_SIZE, block->commit - DN_ARENA_HEADER_SIZE);
    }

    DN_Arena result = {};
    result.flags     = flags | DN_ArenaFlags_NoGrow | DN_ArenaFlags_NoAllocTrack | DN_ArenaFlags_AllocCanLeak | DN_ArenaFlags_UserBuffer;
    result.curr      = block;
    DN_Arena_UpdateStatsOnNewBlock_(&result, result.curr);
    return result;
}

DN_API DN_Arena DN_Arena_InitSize(DN_U64 reserve, DN_U64 commit, DN_ArenaFlags flags)
{
    DN_Arena result = {};
    result.flags     = flags;
    result.curr      = DN_Arena_BlockInitFlags(reserve, commit, flags);
    DN_Arena_UpdateStatsOnNewBlock_(&result, result.curr);
    return result;
}

static void DN_Arena_BlockDeinit_(DN_Arena const *arena, DN_ArenaBlock *block)
{
    DN_USize release_size = block->reserve;
    if (DN_Bit_IsNotSet(arena->flags, DN_ArenaFlags_NoAllocTrack))
        DN_Debug_TrackDealloc(block);
    DN_ASAN_UnpoisonMemoryRegion(block, block->commit);
    DN_OS_MemRelease(block, release_size);
}

DN_API void DN_Arena_Deinit(DN_Arena *arena)
{
    for (DN_ArenaBlock *block = arena ? arena->curr : nullptr; block; ) {
        DN_ArenaBlock *block_to_free = block;
        block                         = block->prev;
        DN_Arena_BlockDeinit_(arena, block_to_free);
    }
    if (arena)
        *arena = {};
}

DN_API bool DN_Arena_CommitTo(DN_Arena *arena, DN_U64 pos)
{
    if (!arena || !arena->curr)
        return false;

    DN_ArenaBlock *curr = arena->curr;
    if (pos <= curr->commit)
        return true;

    DN_U64 real_pos = pos;
    if (!DN_CHECK(pos <= curr->reserve))
        real_pos = curr->reserve;

    DN_USize end_commit  = DN_AlignUpPowerOfTwo(real_pos, g_dn_core->os_page_size);
    DN_USize commit_size = end_commit - curr->commit;
    char    *commit_ptr  = DN_CAST(char *) curr + curr->commit;
    if (!DN_OS_MemCommit(commit_ptr, commit_size, DN_OSMemPage_ReadWrite))
        return false;

    bool poison = DN_ASAN_POISON && ((arena->flags & DN_ArenaFlags_NoPoison) == 0);
    if (poison)
        DN_ASAN_PoisonMemoryRegion(commit_ptr, commit_size);

    curr->commit = end_commit;
    return true;
}

DN_API bool DN_Arena_Commit(DN_Arena *arena, DN_U64 size)
{
    if (!arena || !arena->curr)
        return false;
    DN_U64 pos    = arena->curr->commit + size;
    bool     result = DN_Arena_CommitTo(arena, pos);
    return result;
}

DN_API bool DN_Arena_Grow(DN_Arena *arena, DN_U64 reserve, DN_U64 commit)
{
    if (arena->flags & (DN_ArenaFlags_NoGrow | DN_ArenaFlags_UserBuffer))
        return false;

    DN_ArenaBlock *new_block = DN_Arena_BlockInitFlags(reserve, commit, arena->flags);
    if (new_block) {
        new_block->prev        = arena->curr;
        arena->curr            = new_block;
        new_block->reserve_sum = new_block->prev->reserve_sum + new_block->prev->reserve;
        DN_Arena_UpdateStatsOnNewBlock_(arena, arena->curr);
    }
    bool result = new_block;
    return result;
}

DN_API void *DN_Arena_Alloc(DN_Arena *arena, DN_U64 size, uint8_t align, DN_ZeroMem zero_mem)
{
    if (!arena)
        return nullptr;

    if (!arena->curr) {
        arena->curr = DN_Arena_BlockInitFlags(DN_ARENA_RESERVE_SIZE, DN_ARENA_COMMIT_SIZE, arena->flags);
        DN_Arena_UpdateStatsOnNewBlock_(arena, arena->curr);
    }

    if (!arena->curr)
        return nullptr;

    try_alloc_again:
    DN_ArenaBlock *curr       = arena->curr;
    bool           poison     = DN_ASAN_POISON && ((arena->flags & DN_ArenaFlags_NoPoison) == 0);
    uint8_t        real_align = poison ? DN_MAX(align, DN_ASAN_POISON_ALIGNMENT) : align;
    DN_U64         offset_pos = DN_AlignUpPowerOfTwo(curr->used, real_align) + (poison ? DN_ASAN_POISON_GUARD_SIZE : 0);
    DN_U64         end_pos    = offset_pos + size;
    DN_U64         alloc_size = end_pos - curr->used;

    if (end_pos > curr->reserve) {
        if (arena->flags & (DN_ArenaFlags_NoGrow | DN_ArenaFlags_UserBuffer))
            return nullptr;
        DN_USize new_reserve = DN_MAX(DN_ARENA_HEADER_SIZE + alloc_size, DN_ARENA_RESERVE_SIZE);
        DN_USize new_commit  = DN_MAX(DN_ARENA_HEADER_SIZE + alloc_size, DN_ARENA_COMMIT_SIZE);
        if (!DN_Arena_Grow(arena, new_reserve, new_commit))
            return nullptr;
        goto try_alloc_again;
    }

    DN_USize prev_arena_commit = curr->commit;
    if (end_pos > curr->commit) {
        DN_ASSERT((arena->flags & DN_ArenaFlags_UserBuffer) == 0);
        DN_USize end_commit  = DN_AlignUpPowerOfTwo(end_pos, g_dn_core->os_page_size);
        DN_USize commit_size = end_commit - curr->commit;
        char    *commit_ptr  = DN_CAST(char *) curr + curr->commit;
        if (!DN_OS_MemCommit(commit_ptr, commit_size, DN_OSMemPage_ReadWrite))
            return nullptr;
        if (poison)
            DN_ASAN_PoisonMemoryRegion(commit_ptr, commit_size);
        curr->commit              = end_commit;
        arena->stats.info.commit += commit_size;
        arena->stats.hwm.commit   = DN_MAX(arena->stats.hwm.commit, arena->stats.info.commit);
    }

    void *result            = DN_CAST(char *) curr + offset_pos;
    curr->used             += alloc_size;
    arena->stats.info.used += alloc_size;
    arena->stats.hwm.used   = DN_MAX(arena->stats.hwm.used, arena->stats.info.used);
    DN_ASAN_UnpoisonMemoryRegion(result, size);

    if (zero_mem == DN_ZeroMem_Yes) {
        DN_USize reused_bytes = DN_MIN(prev_arena_commit - offset_pos, size);
        DN_MEMSET(result, 0, reused_bytes);
    }

    DN_ASSERT(arena->stats.hwm.used    >= arena->stats.info.used);
    DN_ASSERT(arena->stats.hwm.commit  >= arena->stats.info.commit);
    DN_ASSERT(arena->stats.hwm.reserve >= arena->stats.info.reserve);
    DN_ASSERT(arena->stats.hwm.blocks  >= arena->stats.info.blocks);
    return result;
}

DN_API void *DN_Arena_AllocContiguous(DN_Arena *arena, DN_U64 size, uint8_t align, DN_ZeroMem zero_mem)
{
    DN_ArenaFlags prev_flags  = arena->flags;
    arena->flags             |= (DN_ArenaFlags_NoGrow | DN_ArenaFlags_NoPoison);
    void *memory              = DN_Arena_Alloc(arena, size, align, zero_mem);
    arena->flags              = prev_flags;
    return memory;
}

DN_API void *DN_Arena_Copy(DN_Arena *arena, void const *data, DN_U64 size, uint8_t align)
{
    if (!arena || !data || size == 0)
        return nullptr;
    void *result = DN_Arena_Alloc(arena, size, align, DN_ZeroMem_No);
    if (result)
        DN_MEMCPY(result, data, size);
    return result;
}

DN_API void DN_Arena_PopTo(DN_Arena *arena, DN_U64 init_used)
{
    if (!arena || !arena->curr)
        return;
    DN_U64         used = DN_MAX(DN_ARENA_HEADER_SIZE, init_used);
    DN_ArenaBlock *curr = arena->curr;
    while (curr->reserve_sum >= used) {
        DN_ArenaBlock *block_to_free  = curr;
        arena->stats.info.used       -= block_to_free->used;
        arena->stats.info.commit     -= block_to_free->commit;
        arena->stats.info.reserve    -= block_to_free->reserve;
        arena->stats.info.blocks     -= 1;
        if (arena->flags & DN_ArenaFlags_UserBuffer)
            break;
        curr                          = curr->prev;
        DN_Arena_BlockDeinit_(arena, block_to_free);
    }

    arena->stats.info.used -= curr->used;
    arena->curr             = curr;
    curr->used              = used - curr->reserve_sum;
    char     *poison_ptr    = (char *)curr + DN_AlignUpPowerOfTwo(curr->used, DN_ASAN_POISON_ALIGNMENT);
    DN_USize poison_size    = ((char *)curr + curr->commit) - poison_ptr;
    DN_ASAN_PoisonMemoryRegion(poison_ptr, poison_size);
    arena->stats.info.used += curr->used;
}

DN_API void DN_Arena_Pop(DN_Arena *arena, DN_U64 amount)
{
    DN_ArenaBlock *curr     = arena->curr;
    DN_USize       used_sum = curr->reserve_sum + curr->used;
    if (!DN_CHECK(amount <= used_sum))
        amount = used_sum;
    DN_USize pop_to = used_sum - amount;
    DN_Arena_PopTo(arena, pop_to);
}

DN_API DN_U64 DN_Arena_Pos(DN_Arena const *arena)
{
    DN_U64 result = (arena && arena->curr) ? arena->curr->reserve_sum + arena->curr->used : 0;
    return result;
}

DN_API void DN_Arena_Clear(DN_Arena *arena)
{
    DN_Arena_PopTo(arena, 0);
}

DN_API bool DN_Arena_OwnsPtr(DN_Arena const *arena, void *ptr)
{
    bool result = false;
    uintptr_t uint_ptr = DN_CAST(uintptr_t)ptr;
    for (DN_ArenaBlock const *block = arena ? arena->curr : nullptr; !result && block; block = block->prev) {
        uintptr_t begin = DN_CAST(uintptr_t) block + DN_ARENA_HEADER_SIZE;
        uintptr_t end   = begin + block->reserve;
        result          = uint_ptr >= begin && uint_ptr <= end;
    }
    return result;
}


DN_API DN_ArenaStats DN_Arena_SumStatsArray(DN_ArenaStats const *array, DN_USize size)
{
    DN_ArenaStats result = {};
    DN_FOR_UINDEX(index, size) {
        DN_ArenaStats stats  = array[index];
        result.info.used     += stats.info.used;
        result.info.commit   += stats.info.commit;
        result.info.reserve  += stats.info.reserve;
        result.info.blocks   += stats.info.blocks;

        result.hwm.used       = DN_MAX(result.hwm.used,    result.info.used);
        result.hwm.commit     = DN_MAX(result.hwm.commit,  result.info.commit);
        result.hwm.reserve    = DN_MAX(result.hwm.reserve, result.info.reserve);
        result.hwm.blocks     = DN_MAX(result.hwm.blocks,  result.info.blocks);
    }
    return result;
}

DN_API DN_ArenaStats DN_Arena_SumStats(DN_ArenaStats lhs, DN_ArenaStats rhs)
{
    DN_ArenaStats array[] = {lhs, rhs};
    DN_ArenaStats result  = DN_Arena_SumStatsArray(array, DN_ARRAY_UCOUNT(array));
    return result;
}

DN_API DN_ArenaStats DN_Arena_SumArenaArrayToStats(DN_Arena const *array, DN_USize size)
{
    DN_ArenaStats result = {};
    for (DN_USize index = 0; index < size; index++) {
        DN_Arena const *arena = array + index;
        result                = DN_Arena_SumStats(result, arena->stats);
    }
    return result;
}

DN_API DN_ArenaTempMem DN_Arena_TempMemBegin(DN_Arena *arena)
{
    DN_ArenaTempMem result = {};
    if (arena) {
        DN_ArenaBlock *curr = arena->curr;
        result              = {arena, curr ? curr->reserve_sum + curr->used : 0};
    }
    return result;
};

DN_API void DN_Arena_TempMemEnd(DN_ArenaTempMem mem)
{
    DN_Arena_PopTo(mem.arena, mem.used_sum);
};

DN_ArenaTempMemScope::DN_ArenaTempMemScope(DN_Arena *arena)
{
    mem = DN_Arena_TempMemBegin(arena);
}

DN_ArenaTempMemScope::~DN_ArenaTempMemScope()
{
    DN_Arena_TempMemEnd(mem);
}

// NOTE: [$POOL] DN_Pool //////////////////////////////////////////////////////////////////////////
DN_API DN_Pool DN_Pool_Init(DN_Arena *arena, uint8_t align)
{
    DN_Pool result = {};
    if (arena) {
        result.arena = arena;
        result.align = align ? align : DN_POOL_DEFAULT_ALIGN;
    }
    return result;
}

DN_API bool DN_Pool_IsValid(DN_Pool const *pool)
{
    bool result = pool && pool->arena && pool->align;
    return result;
}

DN_API void *DN_Pool_Alloc(DN_Pool *pool, DN_USize size)
{
    void *result = nullptr;
    if (!DN_Pool_IsValid(pool))
        return result;

    DN_USize const required_size       = sizeof(DN_PoolSlot) + pool->align + size;
    DN_USize const size_to_slot_offset = 5; // __lzcnt64(32) e.g. DN_PoolSlotSize_32B
    DN_USize       slot_index          = 0;
    if (required_size > 32) {
        // NOTE: Round up if not PoT as the low bits are set.
        DN_USize dist_to_next_msb = DN_CountLeadingZerosU64(required_size) + 1;
        dist_to_next_msb -= DN_CAST(DN_USize)(!DN_IsPowerOfTwo(required_size));

        DN_USize const register_size = sizeof(DN_USize) * 8;
        DN_ASSERT(register_size >= dist_to_next_msb + size_to_slot_offset);
        slot_index = register_size - dist_to_next_msb - size_to_slot_offset;
    }

    if (!DN_CHECKF(slot_index < DN_PoolSlotSize_Count, "Chunk pool does not support the requested allocation size"))
        return result;

    DN_USize slot_size_in_bytes = 1ULL << (slot_index + size_to_slot_offset);
    DN_ASSERT(required_size <= (slot_size_in_bytes << 0));
    DN_ASSERT(required_size >= (slot_size_in_bytes >> 1));

    DN_PoolSlot *slot = nullptr;
    if (pool->slots[slot_index]) {
        slot                    = pool->slots[slot_index];
        pool->slots[slot_index] = slot->next;
        DN_MEMSET(slot->data, 0, size);
        DN_ASSERT(DN_IsPowerOfTwoAligned(slot->data, pool->align));
    } else {
        void *bytes = DN_Arena_Alloc(pool->arena, slot_size_in_bytes, alignof(DN_PoolSlot), DN_ZeroMem_Yes);
        slot        = DN_CAST(DN_PoolSlot *) bytes;

        // NOTE: The raw pointer is round up to the next 'pool->align'-ed
        // address ensuring at least 1 byte of padding between the raw pointer
        // and the pointer given to the user and that the user pointer is
        // aligned to the pool's alignment.
        //
        // This allows us to smuggle 1 byte behind the user pointer that has
        // the offset to the original pointer.
        slot->data  = DN_CAST(void *)DN_AlignDownPowerOfTwo(DN_CAST(uintptr_t)slot + sizeof(DN_PoolSlot) + pool->align, pool->align);

        uintptr_t offset_to_original_ptr = DN_CAST(uintptr_t)slot->data - DN_CAST(uintptr_t)bytes;
        DN_ASSERT(slot->data > bytes);
        DN_ASSERT(offset_to_original_ptr <= sizeof(DN_PoolSlot) + pool->align);

        // NOTE: Store the offset to the original pointer behind the user's
        // pointer.
        char *offset_to_original_storage = DN_CAST(char *)slot->data - 1;
        DN_MEMCPY(offset_to_original_storage, &offset_to_original_ptr, 1);
    }

    // NOTE: Smuggle the slot type in the next pointer so that we know, when the
    // pointer gets returned which free list to return the pointer to.
    result     = slot->data;
    slot->next = DN_CAST(DN_PoolSlot *)slot_index;
    return result;
}

DN_API DN_Str8 DN_Pool_AllocStr8FV(DN_Pool *pool, DN_FMT_ATTRIB char const *fmt, va_list args)
{
    DN_Str8 result = {};
    if (!DN_Pool_IsValid(pool))
        return result;

    DN_USize size_required = DN_CStr8_FVSize(fmt, args);
    result.data             = DN_CAST(char *) DN_Pool_Alloc(pool, size_required + 1);
    if (result.data) {
        result.size = size_required;
        DN_VSNPRINTF(result.data, DN_CAST(int)(result.size + 1), fmt, args);
    }
    return result;
}

DN_API DN_Str8 DN_Pool_AllocStr8F(DN_Pool *pool, DN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    DN_Str8 result = DN_Pool_AllocStr8FV(pool, fmt, args);
    va_end(args);
    return result;
}

DN_API DN_Str8 DN_Pool_AllocStr8Copy(DN_Pool *pool, DN_Str8 string)
{
    DN_Str8 result = {};
    if (!DN_Pool_IsValid(pool))
        return result;

    if (!DN_Str8_HasData(string))
        return result;

    char *data = DN_CAST(char *)DN_Pool_Alloc(pool, string.size + 1);
    if (!data)
        return result;

    DN_MEMCPY(data, string.data, string.size);
    data[string.size] = 0;
    result            = DN_Str8_Init(data, string.size);
    return result;
}

DN_API void DN_Pool_Dealloc(DN_Pool *pool, void *ptr)
{
    if (!DN_Pool_IsValid(pool) || !ptr)
        return;

    DN_ASSERT(DN_Arena_OwnsPtr(pool->arena, ptr));

    char const *one_byte_behind_ptr    = DN_CAST(char *) ptr - 1;
    DN_USize    offset_to_original_ptr = 0;
    DN_MEMCPY(&offset_to_original_ptr, one_byte_behind_ptr, 1);
    DN_ASSERT(offset_to_original_ptr <= sizeof(DN_PoolSlot) + pool->align);

    char           *original_ptr = DN_CAST(char *) ptr - offset_to_original_ptr;
    DN_PoolSlot    *slot         = DN_CAST(DN_PoolSlot *) original_ptr;
    DN_PoolSlotSize slot_index   = DN_CAST(DN_PoolSlotSize)(DN_CAST(uintptr_t) slot->next);
    DN_ASSERT(slot_index < DN_PoolSlotSize_Count);

    slot->next              = pool->slots[slot_index];
    pool->slots[slot_index] = slot;
}

DN_API void *DN_Pool_Copy(DN_Pool *pool, void const *data, DN_U64 size, uint8_t align)
{
    if (!pool || !data || size == 0)
        return nullptr;

    // TODO: Hmm should align be part of the alloc interface in general? I'm not going to worry
    // about this until we crash because of misalignment.
    DN_ASSERT(pool->align >= align);

    void *result = DN_Pool_Alloc(pool, size);
    if (result)
        DN_MEMCPY(result, data, size);
    return result;
}

// NOTE: [$ACAT] DN_ArenaCatalog //////////////////////////////////////////////////////////////////
DN_API void DN_ArenaCatalog_Init(DN_ArenaCatalog *catalog, DN_Pool *pool)
{
    catalog->pool          = pool;
    catalog->sentinel.next = &catalog->sentinel;
    catalog->sentinel.prev = &catalog->sentinel;
}

DN_API DN_ArenaCatalogItem *DN_ArenaCatalog_Find(DN_ArenaCatalog *catalog, DN_Str8 label)
{
    DN_TicketMutex_Begin(&catalog->ticket_mutex);
    DN_ArenaCatalogItem *result = &catalog->sentinel;
    for (DN_ArenaCatalogItem *item = catalog->sentinel.next; item != &catalog->sentinel; item = item->next) {
        if (item->label == label) {
            result = item;
            break;
        }
    }
    DN_TicketMutex_End(&catalog->ticket_mutex);
    return result;
}

static void DN_ArenaCatalog_AddInternal_(DN_ArenaCatalog *catalog, DN_Arena *arena, DN_Str8 label, bool arena_pool_allocated)
{
    // NOTE: We could use an atomic for appending to the sentinel but it is such
    // a rare operation to append to the catalog that we don't bother.
    DN_TicketMutex_Begin(&catalog->ticket_mutex);

    // NOTE: Create item in the catalog
    DN_ArenaCatalogItem *result = DN_Pool_New(catalog->pool, DN_ArenaCatalogItem);
    if (result) {
        result->arena                = arena;
        result->label                = label;
        result->arena_pool_allocated = arena_pool_allocated;

        // NOTE: Add to the catalog (linked list)
        DN_ArenaCatalogItem *sentinel = &catalog->sentinel;
        result->next                   = sentinel;
        result->prev                   = sentinel->prev;
        result->next->prev             = result;
        result->prev->next             = result;
        DN_Atomic_AddU32(&catalog->arena_count, 1);
    }
    DN_TicketMutex_End(&catalog->ticket_mutex);
}

DN_API void DN_ArenaCatalog_AddF(DN_ArenaCatalog *catalog, DN_Arena *arena, DN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    DN_TicketMutex_Begin(&catalog->ticket_mutex);
    DN_Str8 label = DN_Pool_AllocStr8FV(catalog->pool, fmt, args);
    DN_TicketMutex_End(&catalog->ticket_mutex);
    va_end(args);
    DN_ArenaCatalog_AddInternal_(catalog, arena, label, false /*arena_pool_allocated*/);
}

DN_API void DN_ArenaCatalog_AddFV(DN_ArenaCatalog *catalog, DN_Arena *arena, DN_FMT_ATTRIB char const *fmt, va_list args)
{
    DN_TicketMutex_Begin(&catalog->ticket_mutex);
    DN_Str8 label = DN_Pool_AllocStr8FV(catalog->pool, fmt, args);
    DN_TicketMutex_End(&catalog->ticket_mutex);
    DN_ArenaCatalog_AddInternal_(catalog, arena, label, false /*arena_pool_allocated*/);
}

DN_API DN_Arena *DN_ArenaCatalog_AllocFV(DN_ArenaCatalog *catalog, DN_USize reserve, DN_USize commit, uint8_t arena_flags, DN_FMT_ATTRIB char const *fmt, va_list args)
{
    DN_TicketMutex_Begin(&catalog->ticket_mutex);
    DN_Str8   label  = DN_Pool_AllocStr8FV(catalog->pool, fmt, args);
    DN_Arena *result = DN_Pool_New(catalog->pool, DN_Arena);
    DN_TicketMutex_End(&catalog->ticket_mutex);

    *result = DN_Arena_InitSize(reserve, commit, arena_flags);
    DN_ArenaCatalog_AddInternal_(catalog, result, label, true /*arena_pool_allocated*/);
    return result;
}

DN_API DN_Arena *DN_ArenaCatalog_AllocF(DN_ArenaCatalog *catalog, DN_USize reserve, DN_USize commit, uint8_t arena_flags, DN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    DN_TicketMutex_Begin(&catalog->ticket_mutex);
    DN_Str8   label  = DN_Pool_AllocStr8FV(catalog->pool, fmt, args);
    DN_Arena *result = DN_Pool_New(catalog->pool, DN_Arena);
    DN_TicketMutex_End(&catalog->ticket_mutex);
    va_end(args);

    *result = DN_Arena_InitSize(reserve, commit, arena_flags);
    DN_ArenaCatalog_AddInternal_(catalog, result, label, true /*arena_pool_allocated*/);
    return result;
}

DN_API bool DN_ArenaCatalog_Erase(DN_ArenaCatalog *catalog, DN_Arena *arena, DN_ArenaCatalogFreeArena free_arena)
{
    bool result = false;
    DN_TicketMutex_Begin(&catalog->ticket_mutex);
    for (DN_ArenaCatalogItem *item = catalog->sentinel.next; item != &catalog->sentinel; item = item->next) {
        if (item->arena == arena) {
            item->next->prev = item->prev;
            item->prev->next = item->next;
            if (item->arena_pool_allocated) {
                if (free_arena == DN_ArenaCatalogFreeArena_Yes)
                    DN_Arena_Deinit(item->arena);
                DN_Pool_Dealloc(catalog->pool, item->arena);
            }
            DN_Pool_Dealloc(catalog->pool, item->label.data);
            DN_Pool_Dealloc(catalog->pool, item);
            result = true;
            break;
        }
    }
    DN_TicketMutex_End(&catalog->ticket_mutex);
    return result;
}
