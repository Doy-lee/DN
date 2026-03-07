#define DN_BASE_CONTAINERS_CPP
#if defined(_CLANGD)
#include "../dn.h"
#endif

DN_API void *DN_SliceAllocArena(void **data, DN_USize *slice_size_field, DN_USize size, DN_USize elem_size, DN_U8 align, DN_ZMem zmem, DN_Arena *arena)
{
  void *result = *data;
  *data        = DN_ArenaAlloc(arena, size * elem_size, align, zmem);
  if (*data) {
    *slice_size_field = size;
  }
  return result;
}

DN_API void *DN_CArrayInsertArray(void *data, DN_USize *size, DN_USize max, DN_USize elem_size, DN_USize index, void const *items, DN_USize count)
{
  void *result = nullptr;
  if (!data || !size || !items || count <= 0 || ((*size + count) > max))
    return result;

  DN_USize clamped_index = DN_Min(index, *size);
  if (clamped_index != *size) {
    char const *src           = DN_Cast(char *)data + (clamped_index * elem_size);
    char const *dest          = DN_Cast(char *)data + ((clamped_index + count) * elem_size);
    char const *end           = DN_Cast(char *)data + (size[0] * elem_size);
    DN_USize    bytes_to_move = end - src;
    DN_Memmove(DN_Cast(void *) dest, src, bytes_to_move);
  }

  result = DN_Cast(char *)data + (clamped_index * elem_size);
  DN_Memcpy(result, items, elem_size * count);
  *size += count;
  return result;
}

DN_API void *DN_CArrayPopFront(void *data, DN_USize *size, DN_USize elem_size, DN_USize count)
{
  if (!data || !size || *size == 0 || count == 0)
    return nullptr;

  DN_USize pop_count = DN_Min(count, *size);
  void *result = data;

  if (pop_count < *size) {
    char *src = DN_Cast(char *)data + (pop_count * elem_size);
    char *dest = DN_Cast(char *)data;
    DN_USize bytes_to_move = (*size - pop_count) * elem_size;
    DN_Memmove(dest, src, bytes_to_move);
  }

  *size -= pop_count;
  return result;
}

DN_API void *DN_CArrayPopBack(void *data, DN_USize *size, DN_USize elem_size, DN_USize count)
{
  if (!data || !size || *size == 0 || count == 0)
    return nullptr;

  DN_USize pop_count = DN_Min(count, *size);
  *size -= pop_count;

  return DN_Cast(char *)data + (*size * elem_size);
}

DN_API DN_ArrayEraseResult DN_CArrayEraseRange(void *data, DN_USize *size, DN_USize elem_size, DN_USize begin_index, DN_ISize count, DN_ArrayErase erase)
{
  DN_ArrayEraseResult result = {};
  if (!data || !size || *size == 0 || count == 0)
    return result;

  // Compute the range to erase
  DN_USize start = 0, end = 0;
  if (count < 0) {
    // Erase backwards from begin_index, inclusive of begin_index
    // Range: [begin_index + count + 1, begin_index + 1)
    // Which is: [begin_index - abs(count) + 1, begin_index + 1)
    DN_USize abs_count = DN_Abs(count);
    start = (begin_index + 1 > abs_count) ? (begin_index + 1 - abs_count) : 0;
    end   = begin_index + 1;
  } else {
    start = begin_index;
    end   = begin_index + count;
  }

  // Clamp indices to valid bounds
  start = DN_Min(start, *size);
  end   = DN_Min(end,   *size);

  // Erase the range [start, end)
  DN_USize erase_count = end > start ? end - start : 0;
  if (erase_count) {
    char    *dest      = (char *)data + (elem_size * start);
    char    *array_end = (char *)data + (elem_size * *size);
    char    *src       = dest + (elem_size * erase_count);
    if (erase == DN_ArrayErase_Stable) {
      DN_USize move_size = array_end - src;
      DN_Memmove(dest, src, move_size);
    } else {
      char    *unstable_src = array_end - (elem_size * erase_count);
      DN_USize move_size    = array_end - unstable_src;
      DN_Memcpy(dest, unstable_src, move_size);
    }
    *size -= erase_count;
  }

  result.items_erased = erase_count;
  result.it_index     = start;
  return result;
}

DN_API void *DN_CArrayMakeArray(void *data, DN_USize *size, DN_USize max, DN_USize data_size, DN_USize make_size, DN_ZMem z_mem)
{
  void *result = nullptr;
  DN_USize new_size = *size + make_size;
  if (new_size <= max) {
    result = DN_Cast(char *) data + (data_size * size[0]);
    *size  = new_size;
    if (z_mem == DN_ZMem_Yes)
      DN_Memset(result, 0, data_size * make_size);
  }

  return result;
}

DN_API void *DN_CArrayAddArray(void *data, DN_USize *size, DN_USize max, DN_USize data_size, void const *elems, DN_USize elems_count, DN_ArrayAdd add)
{
  void *result = DN_CArrayMakeArray(data, size, max, data_size, elems_count, DN_ZMem_No);
  if (result) {
    if (add == DN_ArrayAdd_Append) {
      DN_Memcpy(result, elems, elems_count * data_size);
    } else {
      char *move_dest = DN_Cast(char *)data + (elems_count * data_size); // Shift elements forward
      char *move_src  = DN_Cast(char *)data;
      DN_Memmove(move_dest, move_src, data_size * size[0]);
      DN_Memcpy(data, elems, data_size * elems_count);
    }
  }
  return result;
}

DN_API bool DN_CArrayResizeFromPool(void **data, DN_USize *size, DN_USize *max, DN_USize data_size, DN_Pool *pool, DN_USize new_max)
{
  bool result = true;
  if (new_max != *max) {
    DN_USize bytes_to_alloc = data_size * new_max;
    void    *buffer         = DN_PoolNewArray(pool, DN_U8, bytes_to_alloc);
    if (buffer) {
      DN_USize bytes_to_copy = data_size * DN_Min(*size, new_max);
      DN_Memcpy(buffer, *data, bytes_to_copy);
      DN_PoolDealloc(pool, *data);
      *data = buffer;
      *max  = new_max;
      *size = DN_Min(*size, new_max);
    } else {
      result = false;
    }
  }

  return result;
}

DN_API bool DN_CArrayGrowFromPool(void **data, DN_USize size, DN_USize *max, DN_USize data_size, DN_Pool *pool, DN_USize new_max)
{
  bool result = true;
  if (new_max > *max)
    result = DN_CArrayResizeFromPool(data, &size, max, data_size, pool, new_max);
  return result;
}

DN_API bool DN_CArrayGrowIfNeededFromPool(void **data, DN_USize size, DN_USize *max, DN_USize data_size, DN_Pool *pool, DN_USize add_count)
{
  bool     result   = true;
  DN_USize new_size = size + add_count;
  if (new_size > *max) {
    DN_USize new_max = DN_Max(DN_Max(*max * 2, new_size), 8);
    result           = DN_CArrayResizeFromPool(data, &size, max, data_size, pool, new_max);
  }
  return result;
}

DN_API void *DN_SinglyLLDetach(void **link, void **next)
{
  void *result = *link;
  if (*link) {
    *link = *next;
    *next = nullptr;
  }
  return result;
}

DN_API bool DN_RingHasSpace(DN_Ring const *ring, DN_U64 size)
{
  DN_U64 avail  = ring->write_pos - ring->read_pos;
  DN_U64 space  = ring->size - avail;
  bool   result = space >= size;
  return result;
}

DN_API bool DN_RingHasData(DN_Ring const *ring, DN_U64 size)
{
  DN_U64 data   = ring->write_pos - ring->read_pos;
  bool   result = data >= size;
  return result;
}

DN_API void DN_RingWrite(DN_Ring *ring, void const *src, DN_U64 src_size)
{
  DN_Assert(src_size <= ring->size);
  DN_U64 offset               = ring->write_pos % ring->size;
  DN_U64 bytes_before_split   = ring->size - offset;
  DN_U64 pre_split_bytes      = DN_Min(bytes_before_split, src_size);
  DN_U64 post_split_bytes     = src_size - pre_split_bytes;
  void const *pre_split_data  = src;
  void const *post_split_data = ((char *)src + pre_split_bytes);
  DN_Memcpy(ring->base + offset, pre_split_data,  pre_split_bytes);
  DN_Memcpy(ring->base,          post_split_data, post_split_bytes);
  ring->write_pos += src_size;
}

DN_API void DN_RingRead(DN_Ring *ring, void *dest, DN_U64 dest_size)
{
  DN_Assert(dest_size <= ring->size);
  DN_U64 offset             = ring->read_pos % ring->size;
  DN_U64 bytes_before_split = ring->size - offset;
  DN_U64 pre_split_bytes    = DN_Min(bytes_before_split, dest_size);
  DN_U64 post_split_bytes   = dest_size - pre_split_bytes;
  DN_Memcpy(dest,                           ring->base + offset, pre_split_bytes);
  DN_Memcpy((char *)dest + pre_split_bytes, ring->base,          post_split_bytes);
  ring->read_pos += dest_size;
}

DN_U32 const DN_DS_MAP_DEFAULT_HASH_SEED = 0x8a1ced49;
DN_U32 const DN_DS_MAP_SENTINEL_SLOT     = 0;

template <typename T>
DN_DSMap<T> DN_DSMapInit(DN_Arena *arena, DN_U32 size, DN_DSMapFlags flags)
{
  DN_DSMap<T> result = {};
  if (!DN_CheckF(DN_IsPowerOfTwo(size), "Power-of-two size required, given size was '%u'", size))
    return result;
  if (size <= 0)
    return result;
  if (!DN_Check(arena))
    return result;
  result.arena        = arena;
  result.pool         = DN_PoolFromArena(arena, DN_POOL_DEFAULT_ALIGN);
  result.hash_to_slot = DN_ArenaNewArray(result.arena, DN_U32, size, DN_ZMem_Yes);
  result.slots        = DN_ArenaNewArray(result.arena, DN_DSMapSlot<T>, size, DN_ZMem_Yes);
  result.occupied     = 1; // For sentinel
  result.size         = size;
  result.initial_size = size;
  result.flags        = flags;
  DN_AssertF(result.hash_to_slot && result.slots, "We pre-allocated a block of memory sufficient in size for the 2 arrays. Maybe the pointers needed extra space because of natural alignment?");
  return result;
}

template <typename T>
void DN_DSMapDeinit(DN_DSMap<T> *map, DN_ZMem z_mem)
{
  if (!map)
    return;
  // TODO(doyle): Use z_mem
  (void)z_mem;
  DN_ArenaDeinit(map->arena);
  *map = {};
}

template <typename T>
bool DN_DSMapIsValid(DN_DSMap<T> const *map)
{
  bool result = map &&
                map->arena &&
                map->hash_to_slot &&                  // Hash to slot mapping array must be allocated
                map->slots &&                         // Slots array must be allocated
                (map->size & (map->size - 1)) == 0 && // Must be power of two size
                map->occupied >= 1;                   // DN_DS_MAP_SENTINEL_SLOT takes up one slot
  return result;
}

template <typename T>
DN_U32 DN_DSMapHash(DN_DSMap<T> const *map, DN_DSMapKey key)
{
  DN_U32 result = 0;
  if (!map)
    return result;

  if (key.type == DN_DSMapKeyType_U64NoHash) {
    result = DN_Cast(DN_U32) key.u64;
    return result;
  }

  if (key.type == DN_DSMapKeyType_BufferAsU64NoHash) {
    result = key.hash;
    return result;
  }

  DN_U32 seed = map->hash_seed ? map->hash_seed : DN_DS_MAP_DEFAULT_HASH_SEED;
  if (map->hash_function) {
    map->hash_function(key, seed);
  } else {
    // NOTE: Courtesy of Demetri Spanos (which this hash table was inspired
    // from), the following is a hashing function snippet provided for
    // reliable, quick and simple quality hashing functions for hash table
    // use.
    // Source: https://github.com/demetri/scribbles/blob/c475464756c104c91bab83ed4e14badefef12ab5/hashing/ub_aware_hash_functions.c

    char const *key_ptr = nullptr;
    DN_U32      len     = 0;
    DN_U32      h       = seed;
    switch (key.type) {
      case DN_DSMapKeyType_BufferAsU64NoHash: /*FALLTHRU*/
      case DN_DSMapKeyType_U64NoHash:         DN_InvalidCodePath; /*FALLTHRU*/
      case DN_DSMapKeyType_Invalid:           break;

      case DN_DSMapKeyType_Buffer:
        key_ptr = DN_Cast(char const *) key.buffer_data;
        len     = key.buffer_size;
        break;

      case DN_DSMapKeyType_U64:
        key_ptr = DN_Cast(char const *) & key.u64;
        len     = sizeof(key.u64);
        break;
    }

    // Murmur3 32-bit without UB unaligned accesses
    // DN_U32 mur3_32_no_UB(const void *key, int len, DN_U32 h)

    // main body, work on 32-bit blocks at a time
    for (DN_U32 i = 0; i < len / 4; i++) {
      DN_U32 k;
      memcpy(&k, &key_ptr[i * 4], sizeof(k));

      k *= 0xcc9e2d51;
      k = ((k << 15) | (k >> 17)) * 0x1b873593;
      h = (((h ^ k) << 13) | ((h ^ k) >> 19)) * 5 + 0xe6546b64;
    }

    // load/mix up to 3 remaining tail bytes into a tail block
    DN_U32   t    = 0;
    uint8_t *tail = ((uint8_t *)key_ptr) + 4 * (len / 4);
    switch (len & 3) {
      case 3: t ^= tail[2] << 16;
      case 2: t ^= tail[1] << 8;
      case 1: {
        t ^= tail[0] << 0;
        h ^= ((0xcc9e2d51 * t << 15) | (0xcc9e2d51 * t >> 17)) * 0x1b873593;
      }
    }

    // finalization mix, including key length
    h      = ((h ^ len) ^ ((h ^ len) >> 16)) * 0x85ebca6b;
    h      = (h ^ (h >> 13)) * 0xc2b2ae35;
    result = h ^ (h >> 16);
  }
  return result;
}

template <typename T>
DN_U32 DN_DSMapHashToSlotIndex(DN_DSMap<T> const *map, DN_DSMapKey key)
{
  DN_Assert(key.type != DN_DSMapKeyType_Invalid);
  DN_U32 result = DN_DS_MAP_SENTINEL_SLOT;
  if (!DN_DSMapIsValid(map))
    return result;

  result = key.hash & (map->size - 1);
  for (;;) {
    if (result == DN_DS_MAP_SENTINEL_SLOT) // Sentinel is reserved
      result++;

    if (map->hash_to_slot[result] == DN_DS_MAP_SENTINEL_SLOT) // Slot is vacant, can use
      return result;

    DN_DSMapSlot<T> *slot = map->slots + map->hash_to_slot[result];
    if (slot->key.type == DN_DSMapKeyType_Invalid || (slot->key.hash == key.hash && slot->key == key))
      return result;

    result = (result + 1) & (map->size - 1);
  }
}

template <typename T>
DN_DSMapResult<T> DN_DSMapFind(DN_DSMap<T> const *map, DN_DSMapKey key)
{
  DN_DSMapResult<T> result = {};
  if (DN_DSMapIsValid(map)) {
    DN_U32 index = DN_DSMapHashToSlotIndex(map, key);
    if (index != DN_DS_MAP_SENTINEL_SLOT && map->hash_to_slot[index] == DN_DS_MAP_SENTINEL_SLOT) {
      result.slot = map->slots; // NOTE: Set to sentinel value
    } else {
      result.slot  = map->slots + map->hash_to_slot[index];
      result.found = true;
    }
    result.value = &result.slot->value;
  }
  return result;
}

template <typename T>
DN_DSMapResult<T> DN_DSMapMake(DN_DSMap<T> *map, DN_DSMapKey key)
{
  DN_DSMapResult<T> result = {};
  if (!DN_DSMapIsValid(map))
    return result;

  DN_U32 index = DN_DSMapHashToSlotIndex(map, key);
  if (map->hash_to_slot[index] == DN_DS_MAP_SENTINEL_SLOT) {
    // NOTE: Create the slot
    if (index != DN_DS_MAP_SENTINEL_SLOT)
      map->hash_to_slot[index] = map->occupied++;

    // NOTE: Check if resize is required
    bool map_is_75pct_full = (map->occupied * 4) > (map->size * 3);
    if (map_is_75pct_full) {
      if (!DN_DSMapResize(map, map->size * 2))
        return result;
      result = DN_DSMapMake(map, key);
    } else {
      result.slot      = map->slots + map->hash_to_slot[index];
      result.slot->key = key; // NOTE: Assign key to new slot
      if ((key.type == DN_DSMapKeyType_Buffer ||
           key.type == DN_DSMapKeyType_BufferAsU64NoHash) &&
          !key.no_copy_buffer)
        result.slot->key.buffer_data = DN_PoolNewArrayCopy(&map->pool, char, key.buffer_data, key.buffer_size);
    }
  } else {
    result.slot  = map->slots + map->hash_to_slot[index];
    result.found = true;
  }

  result.value = &result.slot->value;
  DN_Assert(result.slot->key.type != DN_DSMapKeyType_Invalid);
  return result;
}

template <typename T>
DN_DSMapResult<T> DN_DSMapSet(DN_DSMap<T> *map, DN_DSMapKey key, T const &value)
{
  DN_DSMapResult<T> result = {};
  if (!DN_DSMapIsValid(map))
    return result;

  result             = DN_DSMapMake(map, key);
  result.slot->value = value;
  return result;
}

template <typename T>
DN_DSMapResult<T> DN_DSMapFindKeyU64(DN_DSMap<T> const *map, DN_U64 key)
{
  DN_DSMapKey       map_key = DN_DSMapKeyU64(map, key);
  DN_DSMapResult<T> result  = DN_DSMapFind(map, map_key);
  return result;
}

template <typename T>
DN_DSMapResult<T> DN_DSMapMakeKeyU64(DN_DSMap<T> *map, DN_U64 key)
{
  DN_DSMapKey       map_key = DN_DSMapKeyU64(map, key);
  DN_DSMapResult<T> result  = DN_DSMapMake(map, map_key);
  return result;
}

template <typename T>
DN_DSMapResult<T> DN_DSMapSetKeyU64(DN_DSMap<T> *map, DN_U64 key, T const &value)
{
  DN_DSMapKey       map_key = DN_DSMapKeyU64(map, key);
  DN_DSMapResult<T> result  = DN_DSMapSet(map, map_key, value);
  return result;
}

template <typename T>
DN_DSMapResult<T> DN_DSMapFindKeyStr8(DN_DSMap<T> const *map, DN_Str8 key)
{
  DN_DSMapKey       map_key = DN_DSMapKeyStr8(map, key);
  DN_DSMapResult<T> result  = DN_DSMapFind(map, map_key);
  return result;
}

template <typename T>
DN_DSMapResult<T> DN_DSMapMakeKeyStr8(DN_DSMap<T> *map, DN_Str8 key)
{
  DN_DSMapKey       map_key = DN_DSMapKeyStr8(map, key);
  DN_DSMapResult<T> result  = DN_DSMapMake(map, map_key);
  return result;
}

template <typename T>
DN_DSMapResult<T> DN_DSMapSetKeyStr8(DN_DSMap<T> *map, DN_Str8 key, T const &value)
{
  DN_DSMapKey       map_key = DN_DSMapKeyStr8(map, key);
  DN_DSMapResult<T> result  = DN_DSMapSet(map, map_key);
  return result;
}

template <typename T>
bool DN_DSMapResize(DN_DSMap<T> *map, DN_U32 size)
{
  if (!DN_DSMapIsValid(map) || size < map->occupied || size < map->initial_size)
    return false;

  DN_Arena *prev_arena = map->arena;
  DN_Arena  new_arena  = {};
  new_arena.mem_funcs  = prev_arena->mem_funcs;
  new_arena.flags      = prev_arena->flags;
  new_arena.label      = prev_arena->label;
  new_arena.prev       = prev_arena->prev;
  new_arena.next       = prev_arena->next;

  DN_DSMap<T> new_map = DN_DSMapInit<T>(&new_arena, size, map->flags);
  if (!DN_DSMapIsValid(&new_map))
    return false;

  new_map.initial_size = map->initial_size;
  for (DN_U32 old_index = 1 /*Sentinel*/; old_index < map->occupied; old_index++) {
    DN_DSMapSlot<T> *old_slot = map->slots + old_index;
    DN_DSMapKey      old_key  = old_slot->key;
    if (old_key.type == DN_DSMapKeyType_Invalid)
      continue;
    DN_DSMapSet(&new_map, old_key, old_slot->value);
  }

  if ((map->flags & DN_DSMapFlags_DontFreeArenaOnResize) == 0)
    DN_DSMapDeinit(map, DN_ZMem_No);
  *map            = new_map;    // Update the map inplace
  map->arena      = prev_arena; // Restore the previous arena pointer, it's been de-init-ed
  *map->arena     = new_arena;  // Re-init the old arena with the new data
  map->pool.arena = map->arena;
  return true;
}

template <typename T>
bool DN_DSMapErase(DN_DSMap<T> *map, DN_DSMapKey key)
{
  if (!DN_DSMapIsValid(map))
    return false;

  DN_U32 index = DN_DSMapHashToSlotIndex(map, key);
  if (index == 0)
    return true;

  DN_U32 slot_index = map->hash_to_slot[index];
  if (slot_index == DN_DS_MAP_SENTINEL_SLOT)
    return false;

  // NOTE: Mark the slot as unoccupied
  map->hash_to_slot[index] = DN_DS_MAP_SENTINEL_SLOT;

  DN_DSMapSlot<T> *slot = map->slots + slot_index;
  if (!slot->key.no_copy_buffer)
    DN_PoolDealloc(&map->pool, DN_Cast(void *) slot->key.buffer_data);
  *slot = {}; // TODO: Optional?

  if (map->occupied > 1 /*Sentinel*/) {
    // NOTE: Repair the hash chain, e.g. rehash all the items after the removed
    // element and reposition them if necessary.
    for (DN_U32 probe_index = index;;) {
      probe_index = (probe_index + 1) & (map->size - 1);
      if (map->hash_to_slot[probe_index] == DN_DS_MAP_SENTINEL_SLOT)
        break;

      DN_DSMapSlot<T> *probe     = map->slots + map->hash_to_slot[probe_index];
      DN_U32           new_index = probe->key.hash & (map->size - 1);
      if (index <= probe_index) {
        if (index < new_index && new_index <= probe_index)
          continue;
      } else {
        if (index < new_index || new_index <= probe_index)
          continue;
      }

      map->hash_to_slot[index]       = map->hash_to_slot[probe_index];
      map->hash_to_slot[probe_index] = DN_DS_MAP_SENTINEL_SLOT;
      index                          = probe_index;
    }

    // NOTE: We have erased a slot from the hash table, this leaves a gap
    // in our contiguous array. After repairing the chain, the hash mapping
    // is correct.
    // We will now fill in the vacant spot that we erased using the last
    // element in the slot list.
    if (map->occupied >= 3 /*Ignoring sentinel, at least 2 other elements to unstable erase*/) {
      DN_U32 last_index = map->occupied - 1;
      if (last_index != slot_index) {
        // NOTE: Copy in last slot to the erase slot
        DN_DSMapSlot<T> *last_slot = map->slots + last_index;
        map->slots[slot_index]     = *last_slot;

        // NOTE: Update the hash-to-slot mapping for the value that was copied in
        DN_U32 hash_to_slot_index             = DN_DSMapHashToSlotIndex(map, last_slot->key);
        map->hash_to_slot[hash_to_slot_index] = slot_index;
        *last_slot                            = {}; // TODO: Optional?
      }
    }
  }

  map->occupied--;
  bool map_is_below_25pct_full = (map->occupied * 4) < (map->size * 1);
  if (map_is_below_25pct_full && (map->size / 2) >= map->initial_size)
    DN_DSMapResize(map, map->size / 2);

  return true;
}

template <typename T>
bool DN_DSMapEraseKeyU64(DN_DSMap<T> *map, DN_U64 key)
{
  DN_DSMapKey map_key = DN_DSMapKeyU64(map, key);
  bool        result  = DN_DSMapErase(map, map_key);
  return result;
}

template <typename T>
bool DN_DSMapEraseKeyStr8(DN_DSMap<T> *map, DN_Str8 key)
{
  DN_DSMapKey map_key = DN_DSMapKeyStr8(map, key);
  bool        result  = DN_DSMapErase(map, map_key);
  return result;
}

template <typename T>
DN_DSMapKey DN_DSMapKeyBuffer(DN_DSMap<T> const *map, void const *data, DN_USize size)
{
  DN_Assert(size > 0 && size <= UINT32_MAX);
  DN_DSMapKey result = {};
  result.type        = DN_DSMapKeyType_Buffer;
  result.buffer_data = data;
  result.buffer_size = DN_Cast(DN_U32) size;
  result.hash        = DN_DSMapHash(map, result);
  return result;
}

template <typename T>
DN_DSMapKey DN_DSMapKeyBufferAsU64NoHash(DN_DSMap<T> const *map, void const *data, DN_U32 size)
{
  DN_DSMapKey result = {};
  result.type        = DN_DSMapKeyType_BufferAsU64NoHash;
  result.buffer_data = data;
  result.buffer_size = DN_Cast(DN_U32) size;
  DN_Assert(size >= sizeof(result.hash));
  DN_Memcpy(&result.hash, data, sizeof(result.hash));
  return result;
}

template <typename T>
DN_DSMapKey DN_DSMapKeyU64(DN_DSMap<T> const *map, DN_U64 u64)
{
  DN_DSMapKey result = {};
  result.type        = DN_DSMapKeyType_U64;
  result.u64         = u64;
  result.hash        = DN_DSMapHash(map, result);
  return result;
}

template <typename T>
DN_DSMapKey DN_DSMapKeyStr8(DN_DSMap<T> const *map, DN_Str8 string)
{
  DN_DSMapKey result = DN_DSMapKeyBuffer(map, string.data, string.size);
  return result;
}

DN_API DN_Str8 DN_Slice_Str8Render(DN_Arena *arena, DN_Slice<DN_Str8> array, DN_Str8 separator)
{
  DN_Str8 result = {};
  if (!arena)
    return result;

  DN_USize total_size = 0;
  for (DN_USize index = 0; index < array.size; index++) {
    if (index)
      total_size += separator.size;
    DN_Str8 item = array.data[index];
    total_size += item.size;
  }

  result = DN_Str8FromArena(arena, total_size, DN_ZMem_No);
  if (result.data) {
    DN_USize write_index = 0;
    for (DN_USize index = 0; index < array.size; index++) {
      if (index) {
        DN_Memcpy(result.data + write_index, separator.data, separator.size);
        write_index += separator.size;
      }
      DN_Str8 item = array.data[index];
      DN_Memcpy(result.data + write_index, item.data, item.size);
      write_index += item.size;
    }
  }

  return result;
}

DN_API DN_Str8 DN_Slice_Str8RenderSpaceSeparated(DN_Arena *arena, DN_Slice<DN_Str8> array)
{
  DN_Str8 result = DN_Slice_Str8Render(arena, array, DN_Str8Lit(" "));
  return result;
}

DN_API DN_Str16 DN_Slice_Str16Render(DN_Arena *arena, DN_Slice<DN_Str16> array, DN_Str16 separator)
{
  DN_Str16 result = {};
  if (!arena)
    return result;

  DN_USize total_size = 0;
  for (DN_USize index = 0; index < array.size; index++) {
    if (index)
      total_size += separator.size;
    DN_Str16 item = array.data[index];
    total_size += item.size;
  }

  result = {DN_ArenaNewArray(arena, wchar_t, total_size + 1, DN_ZMem_No), total_size};
  if (result.data) {
    DN_USize write_index = 0;
    for (DN_USize index = 0; index < array.size; index++) {
      if (index) {
        DN_Memcpy(result.data + write_index, separator.data, separator.size * sizeof(result.data[0]));
        write_index += separator.size;
      }
      DN_Str16 item = array.data[index];
      DN_Memcpy(result.data + write_index, item.data, item.size * sizeof(result.data[0]));
      write_index += item.size;
    }
  }

  result.data[total_size] = 0;
  return result;
}

DN_API DN_Str16 DN_Slice_Str16RenderSpaceSeparated(DN_Arena *arena, DN_Slice<DN_Str16> array)
{
  DN_Str16 result = DN_Slice_Str16Render(arena, array, DN_Str16Lit(L" "));
  return result;
}

// NOTE: DN_DSMap
DN_API DN_DSMapKey DN_DSMapKeyU64NoHash(DN_U64 u64)
{
  DN_DSMapKey result = {};
  result.type        = DN_DSMapKeyType_U64NoHash;
  result.u64         = u64;
  result.hash        = DN_Cast(DN_U32) u64;
  return result;
}

DN_API bool DN_DSMapKeyEquals(DN_DSMapKey lhs, DN_DSMapKey rhs)
{
  bool result = false;
  if (lhs.type == rhs.type && lhs.hash == rhs.hash) {
    switch (lhs.type) {
      case DN_DSMapKeyType_Invalid: result = true; break;
      case DN_DSMapKeyType_U64NoHash: result = true; break;
      case DN_DSMapKeyType_U64: result = lhs.u64 == rhs.u64; break;

      case DN_DSMapKeyType_BufferAsU64NoHash: /*FALLTHRU*/
      case DN_DSMapKeyType_Buffer: {
        if (lhs.buffer_size == rhs.buffer_size)
          result = DN_Memcmp(lhs.buffer_data, rhs.buffer_data, lhs.buffer_size) == 0;
      } break;
    }
  }
  return result;
}

DN_API bool operator==(DN_DSMapKey lhs, DN_DSMapKey rhs)
{
  bool result = DN_DSMapKeyEquals(lhs, rhs);
  return result;
}
