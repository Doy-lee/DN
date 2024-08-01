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

// NOTE: [$AREN] Dqn_Arena /////////////////////////////////////////////////////////////////////////
DQN_API Dqn_ArenaBlock *Dqn_Arena_BlockInit(uint64_t reserve, uint64_t commit, bool track_alloc, bool alloc_can_leak)
{
    Dqn_usize const page_size    = g_dqn_library->os_page_size;
    uint64_t        real_reserve = reserve ? reserve : DQN_ARENA_RESERVE_SIZE;
    uint64_t        real_commit  = commit  ? commit  : DQN_ARENA_COMMIT_SIZE;
    real_reserve                 =         Dqn_AlignUpPowerOfTwo(real_reserve, page_size);
    real_commit                  = DQN_MIN(Dqn_AlignUpPowerOfTwo(real_commit, page_size), real_reserve);

    DQN_ASSERTF(DQN_ARENA_HEADER_SIZE < real_commit && real_commit <= real_reserve, "%I64u < %I64u <= %I64u", DQN_ARENA_HEADER_SIZE, real_commit, real_reserve);
    DQN_ASSERTF(page_size, "Call Dqn_Library_Init() to initialise the known page size");

    Dqn_OSMemCommit  mem_commit = real_reserve == real_commit ? Dqn_OSMemCommit_Yes : Dqn_OSMemCommit_No;
    Dqn_ArenaBlock *result      = DQN_CAST(Dqn_ArenaBlock *)Dqn_OS_MemReserve(real_reserve, mem_commit, Dqn_OSMemPage_ReadWrite);
    if (!result)
        return result;

    if (mem_commit == Dqn_OSMemCommit_No && !Dqn_OS_MemCommit(result, real_commit, Dqn_OSMemPage_ReadWrite)) {
        Dqn_OS_MemRelease(result, real_reserve);
        return result;
    }

    result->used    = DQN_ARENA_HEADER_SIZE;
    result->commit  = real_commit;
    result->reserve = real_reserve;

    if (track_alloc)
        Dqn_Debug_TrackAlloc(result, result->reserve, alloc_can_leak);
    return result;
}

DQN_API Dqn_ArenaBlock *Dqn_Arena_BlockInitFlags(uint64_t reserve, uint64_t commit, uint8_t arena_flags)
{
    bool            track_alloc    = (arena_flags & Dqn_ArenaFlag_NoAllocTrack) == 0;
    bool            alloc_can_leak = arena_flags & Dqn_ArenaFlag_AllocCanLeak;
    Dqn_ArenaBlock *result         = Dqn_Arena_BlockInit(reserve, commit, track_alloc, alloc_can_leak);
    if (result && ((arena_flags & Dqn_ArenaFlag_NoPoison) == 0))
        Dqn_ASAN_PoisonMemoryRegion(DQN_CAST(char *)result + DQN_ARENA_HEADER_SIZE, result->commit - DQN_ARENA_HEADER_SIZE);
    return result;
}

static void Dqn_Arena_UpdateStatsOnNewBlock_(Dqn_Arena *arena, Dqn_ArenaBlock const *block)
{
    DQN_ASSERT(arena);
    if (block) {
        arena->stats.info.used     += block->used;
        arena->stats.info.commit   += block->commit;
        arena->stats.info.reserve  += block->reserve;
        arena->stats.info.blocks   += 1;

        arena->stats.hwm.used       = DQN_MAX(arena->stats.hwm.used,    arena->stats.info.used);
        arena->stats.hwm.commit     = DQN_MAX(arena->stats.hwm.commit,  arena->stats.info.commit);
        arena->stats.hwm.reserve    = DQN_MAX(arena->stats.hwm.reserve, arena->stats.info.reserve);
        arena->stats.hwm.blocks     = DQN_MAX(arena->stats.hwm.blocks,  arena->stats.info.blocks);
    }
}

DQN_API Dqn_Arena Dqn_Arena_InitSize(uint64_t reserve, uint64_t commit, uint8_t flags)
{
    Dqn_Arena result = {};
    result.flags     = flags;
    result.curr      = Dqn_Arena_BlockInitFlags(reserve, commit, flags);
    Dqn_Arena_UpdateStatsOnNewBlock_(&result, result.curr);
    return result;
}

static void Dqn_Arena_BlockDeinit_(Dqn_Arena const *arena, Dqn_ArenaBlock *block)
{
    Dqn_usize release_size = block->reserve;
    if (Dqn_Bit_IsNotSet(arena->flags, Dqn_ArenaFlag_NoAllocTrack))
        Dqn_Debug_TrackDealloc(block);
    Dqn_ASAN_UnpoisonMemoryRegion(block, block->commit);
    Dqn_OS_MemRelease(block, release_size);
}

DQN_API void Dqn_Arena_Deinit(Dqn_Arena *arena)
{
    for (Dqn_ArenaBlock *block = arena ? arena->curr : nullptr; block; ) {
        Dqn_ArenaBlock *block_to_free = block;
        block                         = block->prev;
        Dqn_Arena_BlockDeinit_(arena, block_to_free);
    }
}

DQN_API bool Dqn_Arena_CommitTo(Dqn_Arena *arena, uint64_t pos)
{
    if (!arena || !arena->curr)
        return false;

    Dqn_ArenaBlock *curr = arena->curr;
    if (pos <= curr->commit)
        return true;

    uint64_t real_pos = pos;
    if (!DQN_CHECK(pos <= curr->reserve))
        real_pos = curr->reserve;

    Dqn_usize end_commit  = Dqn_AlignUpPowerOfTwo(real_pos, g_dqn_library->os_page_size);
    Dqn_usize commit_size = end_commit - curr->commit;
    char     *commit_ptr  = DQN_CAST(char *) curr + curr->commit;
    if (!Dqn_OS_MemCommit(commit_ptr, commit_size, Dqn_OSMemPage_ReadWrite))
        return false;

    bool poison = DQN_ASAN_POISON && ((arena->flags & Dqn_ArenaFlag_NoPoison) == 0);
    if (poison)
        Dqn_ASAN_PoisonMemoryRegion(commit_ptr, commit_size);

    curr->commit = end_commit;
    return true;
}

DQN_API bool Dqn_Arena_Commit(Dqn_Arena *arena, uint64_t size)
{
    if (!arena || !arena->curr)
        return false;
    uint64_t pos    = arena->curr->commit + size;
    bool     result = Dqn_Arena_CommitTo(arena, pos);
    return result;
}

DQN_API void *Dqn_Arena_Alloc(Dqn_Arena *arena, uint64_t size, uint8_t align, Dqn_ZeroMem zero_mem)
{
    if (!arena)
        return nullptr;

    if (!arena->curr) {
        arena->curr = Dqn_Arena_BlockInitFlags(DQN_ARENA_RESERVE_SIZE, DQN_ARENA_COMMIT_SIZE, arena->flags);
        Dqn_Arena_UpdateStatsOnNewBlock_(arena, arena->curr);
    }

    if (!arena->curr)
        return nullptr;

    try_alloc_again:
    Dqn_ArenaBlock *curr       = arena->curr;
    bool poison                = DQN_ASAN_POISON && ((arena->flags & Dqn_ArenaFlag_NoPoison) == 0);
    uint8_t         real_align = poison ? DQN_MAX(align, DQN_ASAN_POISON_ALIGNMENT) : align;
    uint64_t        offset_pos = Dqn_AlignUpPowerOfTwo(curr->used, real_align) + (poison ? DQN_ASAN_POISON_GUARD_SIZE : 0);
    uint64_t        end_pos    = offset_pos + size;

    if (end_pos > curr->reserve) {
        if (arena->flags & Dqn_ArenaFlag_NoGrow)
            return nullptr;
        Dqn_usize       new_reserve = DQN_MAX(DQN_ARENA_HEADER_SIZE + size, DQN_ARENA_RESERVE_SIZE);
        Dqn_usize       new_commit  = DQN_MAX(DQN_ARENA_HEADER_SIZE + size, DQN_ARENA_COMMIT_SIZE);
        Dqn_ArenaBlock *new_block   = Dqn_Arena_BlockInitFlags(new_reserve, new_commit, arena->flags);
        if (!new_block)
            return nullptr;
        new_block->prev        = arena->curr;
        arena->curr            = new_block;
        new_block->reserve_sum = new_block->prev->reserve_sum + new_block->prev->reserve;
        Dqn_Arena_UpdateStatsOnNewBlock_(arena, arena->curr);
        goto try_alloc_again;
    }

    Dqn_usize prev_arena_commit = curr->commit;
    if (end_pos > curr->commit) {
        Dqn_usize end_commit  = Dqn_AlignUpPowerOfTwo(end_pos, g_dqn_library->os_page_size);
        Dqn_usize commit_size = end_commit - curr->commit;
        char     *commit_ptr  = DQN_CAST(char *)curr + curr->commit;
        if (!Dqn_OS_MemCommit(commit_ptr, commit_size, Dqn_OSMemPage_ReadWrite))
            return nullptr;
        if (poison)
            Dqn_ASAN_PoisonMemoryRegion(commit_ptr, commit_size);
        curr->commit              = end_commit;
        arena->stats.info.commit += commit_size;
        arena->stats.hwm.commit   = DQN_MAX(arena->stats.hwm.commit, arena->stats.info.commit);
    }

    void *result            = DQN_CAST(char *) curr + offset_pos;
    Dqn_usize alloc_size    = end_pos - curr->used;
    curr->used             += alloc_size;
    arena->stats.info.used += alloc_size;
    arena->stats.hwm.used   = DQN_MAX(arena->stats.hwm.used, arena->stats.info.used);
    Dqn_ASAN_UnpoisonMemoryRegion(result, size);

    if (zero_mem == Dqn_ZeroMem_Yes) {
        Dqn_usize reused_bytes = DQN_MIN(prev_arena_commit - offset_pos, size);
        DQN_MEMSET(result, 0, reused_bytes);
    }

    DQN_ASSERT(arena->stats.hwm.used    >= arena->stats.info.used);
    DQN_ASSERT(arena->stats.hwm.commit  >= arena->stats.info.commit);
    DQN_ASSERT(arena->stats.hwm.reserve >= arena->stats.info.reserve);
    DQN_ASSERT(arena->stats.hwm.blocks  >= arena->stats.info.blocks);
    return result;
}

DQN_API void *Dqn_Arena_AllocContiguous(Dqn_Arena *arena, uint64_t size, uint8_t align, Dqn_ZeroMem zero_mem)
{
    uint8_t prev_flags  = arena->flags;
    arena->flags       |= (Dqn_ArenaFlag_NoGrow | Dqn_ArenaFlag_NoPoison);
    void *memory        = Dqn_Arena_Alloc(arena, size, align, zero_mem);
    arena->flags        = prev_flags;
    return memory;
}

DQN_API void *Dqn_Arena_Copy(Dqn_Arena *arena, void const *data, uint64_t size, uint8_t align)
{
    if (!arena || !data || size == 0)
        return nullptr;
    void *result = Dqn_Arena_Alloc(arena, size, align, Dqn_ZeroMem_No);
    if (result)
        DQN_MEMCPY(result, data, size);
    return result;
}

DQN_API void Dqn_Arena_PopTo(Dqn_Arena *arena, uint64_t init_used)
{
    if (!arena || !arena->curr)
        return;
    uint64_t        used = DQN_MAX(DQN_ARENA_HEADER_SIZE, init_used);
    Dqn_ArenaBlock *curr = arena->curr;
    while (curr->reserve_sum >= used) {
        Dqn_ArenaBlock *block_to_free = curr;
        arena->stats.info.used       -= block_to_free->used;
        arena->stats.info.commit     -= block_to_free->commit;
        arena->stats.info.reserve    -= block_to_free->reserve;
        arena->stats.info.blocks     -= 1;
        curr                          = curr->prev;
        Dqn_Arena_BlockDeinit_(arena, block_to_free);
    }

    arena->stats.info.used -= curr->used;
    arena->curr             = curr;
    curr->used              = used - curr->reserve_sum;
    char     *poison_ptr    = (char *)curr + Dqn_AlignUpPowerOfTwo(curr->used, DQN_ASAN_POISON_ALIGNMENT);
    Dqn_usize poison_size   = ((char *)curr + curr->commit) - poison_ptr;
    Dqn_ASAN_PoisonMemoryRegion(poison_ptr, poison_size);
    arena->stats.info.used += curr->used;
}

DQN_API void Dqn_Arena_Pop(Dqn_Arena *arena, uint64_t amount)
{
    Dqn_ArenaBlock *curr     = arena->curr;
    Dqn_usize       used_sum = curr->reserve_sum + curr->used;
    if (!DQN_CHECK(amount <= used_sum))
        amount = used_sum;
    Dqn_usize pop_to = used_sum - amount;
    Dqn_Arena_PopTo(arena, pop_to);
}

DQN_API uint64_t Dqn_Arena_Pos(Dqn_Arena const *arena)
{
    uint64_t result = (arena && arena->curr) ? arena->curr->reserve_sum + arena->curr->used : 0;
    return result;
}

DQN_API void Dqn_Arena_Clear(Dqn_Arena *arena)
{
    Dqn_Arena_PopTo(arena, 0);
}

DQN_API bool Dqn_Arena_OwnsPtr(Dqn_Arena const *arena, void *ptr)
{
    bool result = false;
    uintptr_t uint_ptr = DQN_CAST(uintptr_t)ptr;
    for (Dqn_ArenaBlock const *block = arena ? arena->curr : nullptr; !result && block; block = block->prev) {
        uintptr_t begin = DQN_CAST(uintptr_t) block + DQN_ARENA_HEADER_SIZE;
        uintptr_t end   = begin + block->reserve;
        result          = uint_ptr >= begin && uint_ptr <= end;
    }
    return result;
}


DQN_API Dqn_ArenaStats Dqn_Arena_SumStatsArray(Dqn_ArenaStats const *array, Dqn_usize size)
{
    Dqn_ArenaStats result = {};
    DQN_FOR_UINDEX(index, size) {
        Dqn_ArenaStats stats  = array[index];
        result.info.used     += stats.info.used;
        result.info.commit   += stats.info.commit;
        result.info.reserve  += stats.info.reserve;
        result.info.blocks   += stats.info.blocks;

        result.hwm.used       = DQN_MAX(result.hwm.used,    result.info.used);
        result.hwm.commit     = DQN_MAX(result.hwm.commit,  result.info.commit);
        result.hwm.reserve    = DQN_MAX(result.hwm.reserve, result.info.reserve);
        result.hwm.blocks     = DQN_MAX(result.hwm.blocks,  result.info.blocks);
    }
    return result;
}

DQN_API Dqn_ArenaStats Dqn_Arena_SumStats(Dqn_ArenaStats lhs, Dqn_ArenaStats rhs)
{
    Dqn_ArenaStats array[] = {lhs, rhs};
    Dqn_ArenaStats result  = Dqn_Arena_SumStatsArray(array, DQN_ARRAY_UCOUNT(array));
    return result;
}

DQN_API Dqn_ArenaStats Dqn_Arena_SumArenaArrayToStats(Dqn_Arena const *array, Dqn_usize size)
{
    Dqn_ArenaStats result = {};
    for (Dqn_usize index = 0; index < size; index++) {
        Dqn_Arena const *arena = array + index;
        result                 = Dqn_Arena_SumStats(result, arena->stats);
    }
    return result;
}

DQN_API Dqn_ArenaTempMem Dqn_Arena_TempMemBegin(Dqn_Arena *arena)
{
    Dqn_ArenaTempMem result = {};
    if (arena) {
        Dqn_ArenaBlock *curr = arena->curr;
        result               = {arena, curr ? curr->reserve_sum + curr->used : 0};
    }
    return result;
};

DQN_API void Dqn_Arena_TempMemEnd(Dqn_ArenaTempMem mem)
{
    Dqn_Arena_PopTo(mem.arena, mem.used_sum);
};

Dqn_ArenaTempMemScope::Dqn_ArenaTempMemScope(Dqn_Arena *arena)
{
    mem = Dqn_Arena_TempMemBegin(arena);
}

Dqn_ArenaTempMemScope::~Dqn_ArenaTempMemScope()
{
    Dqn_Arena_TempMemEnd(mem);
}

// NOTE: [$CHUN] Dqn_ChunkPool /////////////////////////////////////////////////////////////////////
DQN_API Dqn_ChunkPool Dqn_ChunkPool_Init(Dqn_Arena *arena, uint8_t align)
{
    Dqn_ChunkPool result = {};
    if (arena) {
        result.arena = arena;
        result.align = align;
        if (result.align == 0)
            result.align = DQN_CHUNK_POOL_DEFAULT_ALIGN;
    }
    return result;
}

DQN_API bool Dqn_ChunkPool_IsValid(Dqn_ChunkPool const *pool)
{
    bool result = pool && pool->arena && pool->align;
    return result;
}

DQN_API void *Dqn_ChunkPool_Alloc(Dqn_ChunkPool *pool, Dqn_usize size)
{
    void *result = nullptr;
    if (!Dqn_ChunkPool_IsValid(pool))
        return result;

    Dqn_usize const required_size       = sizeof(Dqn_ChunkPoolSlot) + pool->align + size;
    Dqn_usize const size_to_slot_offset = 5; // __lzcnt64(32) e.g. Dqn_ChunkPoolSlotSize_32B
    Dqn_usize       slot_index          = 0;
    if (required_size > 32) {
        // NOTE: Round up if not PoT as the low bits are set.
        Dqn_usize dist_to_next_msb = Dqn_CountLeadingZerosU64(required_size) + 1;
        dist_to_next_msb -= DQN_CAST(Dqn_usize)(!Dqn_IsPowerOfTwo(required_size));

        Dqn_usize const register_size = sizeof(Dqn_usize) * 8;
        DQN_ASSERT(register_size >= dist_to_next_msb + size_to_slot_offset);
        slot_index = register_size - dist_to_next_msb - size_to_slot_offset;
    }

    if (!DQN_CHECKF(slot_index < Dqn_ChunkPoolSlotSize_Count, "Chunk pool does not support the requested allocation size"))
        return result;

    Dqn_usize slot_size_in_bytes = 1ULL << (slot_index + size_to_slot_offset);
    DQN_ASSERT(required_size <= (slot_size_in_bytes << 0));
    DQN_ASSERT(required_size >= (slot_size_in_bytes >> 1));

    Dqn_ChunkPoolSlot *slot = nullptr;
    if (pool->slots[slot_index]) {
        slot                    = pool->slots[slot_index];
        pool->slots[slot_index] = slot->next;
        DQN_MEMSET(slot->data, 0, size);
        DQN_ASSERT(Dqn_IsPowerOfTwoAligned(slot->data, pool->align));
    } else {
        void *bytes = Dqn_Arena_Alloc(pool->arena, slot_size_in_bytes, alignof(Dqn_ChunkPoolSlot), Dqn_ZeroMem_Yes);
        slot        = DQN_CAST(Dqn_ChunkPoolSlot *) bytes;

        // NOTE: The raw pointer is round up to the next 'pool->align'-ed
        // address ensuring at least 1 byte of padding between the raw pointer
        // and the pointer given to the user and that the user pointer is
        // aligned to the pool's alignment.
        //
        // This allows us to smuggle 1 byte behind the user pointer that has
        // the offset to the original pointer.
        slot->data  = DQN_CAST(void *)Dqn_AlignDownPowerOfTwo(DQN_CAST(uintptr_t)slot + sizeof(Dqn_ChunkPoolSlot) + pool->align, pool->align);

        uintptr_t offset_to_original_ptr = DQN_CAST(uintptr_t)slot->data - DQN_CAST(uintptr_t)bytes;
        DQN_ASSERT(slot->data > bytes);
        DQN_ASSERT(offset_to_original_ptr <= sizeof(Dqn_ChunkPoolSlot) + pool->align);

        // NOTE: Store the offset to the original pointer behind the user's
        // pointer.
        char *offset_to_original_storage = DQN_CAST(char *)slot->data - 1;
        DQN_MEMCPY(offset_to_original_storage, &offset_to_original_ptr, 1);
    }

    // NOTE: Smuggle the slot type in the next pointer so that we know, when the
    // pointer gets returned which free list to return the pointer to.
    result     = slot->data;
    slot->next = DQN_CAST(Dqn_ChunkPoolSlot *)slot_index;
    return result;
}

DQN_API Dqn_Str8 Dqn_ChunkPool_AllocStr8FV(Dqn_ChunkPool *pool, DQN_FMT_ATTRIB char const *fmt, va_list args)
{
    Dqn_Str8 result = {};
    if (!Dqn_ChunkPool_IsValid(pool))
        return result;

    Dqn_usize size_required = Dqn_CStr8_FVSize(fmt, args);
    result.data             = DQN_CAST(char *) Dqn_ChunkPool_Alloc(pool, size_required + 1);
    if (result.data) {
        result.size = size_required;
        DQN_VSNPRINTF(result.data, DQN_CAST(int)(result.size + 1), fmt, args);
    }
    return result;
}

DQN_API Dqn_Str8 Dqn_ChunkPool_AllocStr8F(Dqn_ChunkPool *pool, DQN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Dqn_Str8 result = Dqn_ChunkPool_AllocStr8FV(pool, fmt, args);
    va_end(args);
    return result;
}

DQN_API Dqn_Str8 Dqn_ChunkPool_AllocStr8Copy(Dqn_ChunkPool *pool, Dqn_Str8 string)
{
    Dqn_Str8 result = {};
    if (!Dqn_ChunkPool_IsValid(pool))
        return result;

    if (!Dqn_Str8_HasData(string))
        return result;

    char *data = DQN_CAST(char *)Dqn_ChunkPool_Alloc(pool, string.size + 1);
    if (!data)
        return result;

    DQN_MEMCPY(data, string.data, string.size);
    data[string.size] = 0;
    result            = Dqn_Str8_Init(data, string.size);
    return result;
}

DQN_API void Dqn_ChunkPool_Dealloc(Dqn_ChunkPool *pool, void *ptr)
{
    if (!Dqn_ChunkPool_IsValid(pool) || !ptr)
        return;

    DQN_ASSERT(Dqn_Arena_OwnsPtr(pool->arena, ptr));

    char const *one_byte_behind_ptr    = DQN_CAST(char *) ptr - 1;
    Dqn_usize   offset_to_original_ptr = 0;
    DQN_MEMCPY(&offset_to_original_ptr, one_byte_behind_ptr, 1);
    DQN_ASSERT(offset_to_original_ptr <= sizeof(Dqn_ChunkPoolSlot) + pool->align);

    char *original_ptr               = DQN_CAST(char *)ptr - offset_to_original_ptr;
    Dqn_ChunkPoolSlot *slot          = DQN_CAST(Dqn_ChunkPoolSlot *)original_ptr;
    Dqn_ChunkPoolSlotSize slot_index = DQN_CAST(Dqn_ChunkPoolSlotSize)(DQN_CAST(uintptr_t)slot->next);
    DQN_ASSERT(slot_index < Dqn_ChunkPoolSlotSize_Count);

    slot->next              = pool->slots[slot_index];
    pool->slots[slot_index] = slot;
}

DQN_API void *Dqn_ChunkPool_Copy(Dqn_ChunkPool *pool, void const *data, uint64_t size, uint8_t align)
{
    if (!pool || !data || size == 0)
        return nullptr;

    // TODO: Hmm should align be part of the alloc interface in general? I'm not going to worry
    // about this until we crash because of misalignment.
    DQN_ASSERT(pool->align >= align);

    void *result = Dqn_ChunkPool_Alloc(pool, size);
    if (result)
        DQN_MEMCPY(result, data, size);
    return result;
}

// NOTE: [$ACAT] Dqn_ArenaCatalog //////////////////////////////////////////////////////////////////
DQN_API void Dqn_ArenaCatalog_Init(Dqn_ArenaCatalog *catalog, Dqn_ChunkPool *pool)
{
    catalog->pool          = pool;
    catalog->sentinel.next = &catalog->sentinel;
    catalog->sentinel.prev = &catalog->sentinel;
}

DQN_API Dqn_ArenaCatalogItem *Dqn_ArenaCatalog_Find(Dqn_ArenaCatalog *catalog, Dqn_Str8 label)
{
    Dqn_TicketMutex_Begin(&catalog->ticket_mutex);
    Dqn_ArenaCatalogItem *result = &catalog->sentinel;
    for (Dqn_ArenaCatalogItem *item = catalog->sentinel.next; item != &catalog->sentinel; item = item->next) {
        if (item->label == label) {
            result = item;
            break;
        }
    }
    Dqn_TicketMutex_End(&catalog->ticket_mutex);
    return result;
}

static void Dqn_ArenaCatalog_AddInternal_(Dqn_ArenaCatalog *catalog, Dqn_Arena *arena, Dqn_Str8 label, bool arena_pool_allocated)
{
    // NOTE: We could use an atomic for appending to the sentinel but it is such
    // a rare operation to append to the catalog that we don't bother.
    Dqn_TicketMutex_Begin(&catalog->ticket_mutex);

    // NOTE: Create item in the catalog
    Dqn_ArenaCatalogItem *result = Dqn_ChunkPool_New(catalog->pool, Dqn_ArenaCatalogItem);
    if (result) {
        result->arena                = arena;
        result->label                = label;
        result->arena_pool_allocated = arena_pool_allocated;

        // NOTE: Add to the catalog (linked list)
        Dqn_ArenaCatalogItem *sentinel = &catalog->sentinel;
        result->next                   = sentinel;
        result->prev                   = sentinel->prev;
        result->next->prev             = result;
        result->prev->next             = result;
        Dqn_Atomic_AddU32(&catalog->arena_count, 1);
    }
    Dqn_TicketMutex_End(&catalog->ticket_mutex);
}

DQN_API void Dqn_ArenaCatalog_AddF(Dqn_ArenaCatalog *catalog, Dqn_Arena *arena, DQN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Dqn_TicketMutex_Begin(&catalog->ticket_mutex);
    Dqn_Str8 label = Dqn_ChunkPool_AllocStr8FV(catalog->pool, fmt, args);
    Dqn_TicketMutex_End(&catalog->ticket_mutex);
    va_end(args);
    Dqn_ArenaCatalog_AddInternal_(catalog, arena, label, false /*arena_pool_allocated*/);
}

DQN_API void Dqn_ArenaCatalog_AddFV(Dqn_ArenaCatalog *catalog, Dqn_Arena *arena, DQN_FMT_ATTRIB char const *fmt, va_list args)
{
    Dqn_TicketMutex_Begin(&catalog->ticket_mutex);
    Dqn_Str8 label = Dqn_ChunkPool_AllocStr8FV(catalog->pool, fmt, args);
    Dqn_TicketMutex_End(&catalog->ticket_mutex);
    Dqn_ArenaCatalog_AddInternal_(catalog, arena, label, false /*arena_pool_allocated*/);
}

DQN_API Dqn_Arena *Dqn_ArenaCatalog_AllocFV(Dqn_ArenaCatalog *catalog, Dqn_usize reserve, Dqn_usize commit, uint8_t arena_flags, DQN_FMT_ATTRIB char const *fmt, va_list args)
{
    Dqn_TicketMutex_Begin(&catalog->ticket_mutex);
    Dqn_Str8   label  = Dqn_ChunkPool_AllocStr8FV(catalog->pool, fmt, args);
    Dqn_Arena *result = Dqn_ChunkPool_New(catalog->pool, Dqn_Arena);
    Dqn_TicketMutex_End(&catalog->ticket_mutex);

    *result = Dqn_Arena_InitSize(reserve, commit, arena_flags);
    Dqn_ArenaCatalog_AddInternal_(catalog, result, label, true /*arena_pool_allocated*/);
    return result;
}

DQN_API Dqn_Arena *Dqn_ArenaCatalog_AllocF(Dqn_ArenaCatalog *catalog, Dqn_usize reserve, Dqn_usize commit, uint8_t arena_flags, DQN_FMT_ATTRIB char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Dqn_TicketMutex_Begin(&catalog->ticket_mutex);
    Dqn_Str8   label  = Dqn_ChunkPool_AllocStr8FV(catalog->pool, fmt, args);
    Dqn_Arena *result = Dqn_ChunkPool_New(catalog->pool, Dqn_Arena);
    Dqn_TicketMutex_End(&catalog->ticket_mutex);
    va_end(args);

    *result = Dqn_Arena_InitSize(reserve, commit, arena_flags);
    Dqn_ArenaCatalog_AddInternal_(catalog, result, label, true /*arena_pool_allocated*/);
    return result;
}

DQN_API bool Dqn_ArenaCatalog_Erase(Dqn_ArenaCatalog *catalog, Dqn_Arena *arena, Dqn_ArenaCatalogFreeArena free_arena)
{
    bool result = false;
    Dqn_TicketMutex_Begin(&catalog->ticket_mutex);
    for (Dqn_ArenaCatalogItem *item = catalog->sentinel.next; item != &catalog->sentinel; item = item->next) {
        if (item->arena == arena) {
            item->next->prev = item->prev;
            item->prev->next = item->next;
            if (item->arena_pool_allocated) {
                if (free_arena == Dqn_ArenaCatalogFreeArena_Yes)
                    Dqn_Arena_Deinit(item->arena);
                Dqn_ChunkPool_Dealloc(catalog->pool, item->arena);
            }
            Dqn_ChunkPool_Dealloc(catalog->pool, item->label.data);
            Dqn_ChunkPool_Dealloc(catalog->pool, item);
            result = true;
            break;
        }
    }
    Dqn_TicketMutex_End(&catalog->ticket_mutex);
    return result;
}
