#define DN_CONTAINERS_CPP

#include "../dn_base_inc.h"

DN_API void *DN_CArray2_InsertArray(void *data, DN_USize *size, DN_USize max, DN_USize elem_size, DN_USize index, void const *items, DN_USize count)
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

DN_API DN_ArrayEraseResult DN_CArray2_EraseRange(void *data, DN_USize *size, DN_USize elem_size, DN_USize begin_index, DN_ISize count, DN_ArrayErase erase)
{
  DN_ArrayEraseResult result = {};
  if (!data || !size || *size == 0 || count == 0)
    return result;

  // Compute the range to erase
  DN_USize start = 0, end = 0;
  if (count < 0) {
    DN_USize abs_count = DN_Abs(count);
    start = begin_index >= abs_count ? begin_index - abs_count + 1 : 0;
    end   = begin_index >= abs_count ? begin_index + 1             : 0;
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

DN_API void *DN_CArray2_MakeArray(void *data, DN_USize *size, DN_USize max, DN_USize data_size, DN_USize make_size, DN_ZMem z_mem)
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

DN_API void *DN_CArray2_AddArray(void *data, DN_USize *size, DN_USize max, DN_USize data_size, void const *elems, DN_USize elems_count, DN_ArrayAdd add)
{
  void *result = DN_CArray2_MakeArray(data, size, max, data_size, elems_count, DN_ZMem_No);
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

DN_API bool DN_CArray2_ResizeFromPool(void **data, DN_USize *size, DN_USize *max, DN_USize data_size, DN_Pool *pool, DN_USize new_max)
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

DN_API bool DN_CArray2_GrowFromPool(void **data, DN_USize size, DN_USize *max, DN_USize data_size, DN_Pool *pool, DN_USize new_max)
{
  bool result = true;
  if (new_max > *max)
    result = DN_CArray2_ResizeFromPool(data, &size, max, data_size, pool, new_max);
  return result;
}

DN_API bool DN_CArray2_GrowIfNeededFromPool(void **data, DN_USize size, DN_USize *max, DN_USize data_size, DN_Pool *pool, DN_USize add_count)
{
  bool     result   = true;
  DN_USize new_size = size + add_count;
  if (new_size > *max) {
    DN_USize new_max = DN_Max(DN_Max(*max * 2, new_size), 8);
    result           = DN_CArray2_ResizeFromPool(data, &size, max, data_size, pool, new_max);
  }
  return result;
}

DN_API void *DN_CSLList_Detach(void **link, void **next)
{
  void *result = *link;
  if (*link) {
    *link = *next;
    *next = nullptr;
  }
  return result;
}

DN_API bool DN_Ring_HasSpace(DN_Ring const *ring, DN_U64 size)
{
  DN_U64 avail  = ring->write_pos - ring->read_pos;
  DN_U64 space  = ring->size - avail;
  bool   result = space >= size;
  return result;
}

DN_API bool DN_Ring_HasData(DN_Ring const *ring, DN_U64 size)
{
  DN_U64 data   = ring->write_pos - ring->read_pos;
  bool   result = data >= size;
  return result;
}

DN_API void DN_Ring_Write(DN_Ring *ring, void const *src, DN_U64 src_size)
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

DN_API void DN_Ring_Read(DN_Ring *ring, void *dest, DN_U64 dest_size)
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

// NOTE: DN_CArray /////////////////////////////////////////////////////////////////////////////////
template <typename T>
DN_ArrayEraseResult DN_CArray_EraseRange(T *data, DN_USize *size, DN_USize begin_index, DN_ISize count, DN_ArrayErase erase)
{
  DN_ArrayEraseResult result = {};
  if (!data || !size || *size == 0 || count == 0)
    return result;

  DN_AssertF(count != -1, "There's a bug with negative element erases, see the DN_VArray section in dn_docs.cpp");

  // NOTE: Caculate the end index of the erase range
  DN_ISize abs_count = DN_Abs(count);
  DN_USize end_index = 0;
  if (count < 0) {
    end_index = begin_index - (abs_count - 1);
    if (end_index > begin_index)
      end_index = 0;
  } else {
    end_index = begin_index + (abs_count - 1);
    if (end_index < begin_index)
      end_index = (*size) - 1;
  }

  // NOTE: Ensure begin_index < one_past_end_index
  if (end_index < begin_index) {
    DN_USize tmp = begin_index;
    begin_index  = end_index;
    end_index    = tmp;
  }

  // NOTE: Ensure indexes are within valid bounds
  begin_index = DN_Min(begin_index, *size);
  end_index   = DN_Min(end_index, *size - 1);

  // NOTE: Erase the items in the range [begin_index, one_past_end_index)
  DN_USize one_past_end_index = end_index + 1;
  DN_USize erase_count        = one_past_end_index - begin_index;
  if (erase_count) {
    T *end  = data + *size;
    T *dest = data + begin_index;
    if (erase == DN_ArrayErase_Stable) {
      T *src = dest + erase_count;
      DN_Memmove(dest, src, (end - src) * sizeof(T));
    } else {
      T *src = end - erase_count;
      DN_Memcpy(dest, src, (end - src) * sizeof(T));
    }
    *size -= erase_count;
  }

  result.items_erased = erase_count;
  result.it_index     = begin_index;
  return result;
}

template <typename T>
T *DN_CArray_MakeArray(T *data, DN_USize *size, DN_USize max, DN_USize count, DN_ZMem z_mem)
{
  if (!data || !size || count == 0)
    return nullptr;

  if (!DN_CheckF((*size + count) <= max, "Array is out of space (user requested +%zu items, array has %zu/%zu items)", count, *size, max))
    return nullptr;

  // TODO: Use placement new? Why doesn't this work?
  T *result = data + *size;
  *size += count;
  if (z_mem == DN_ZMem_Yes)
    DN_Memset(result, 0, sizeof(*result) * count);
  return result;
}

template <typename T>
T *DN_CArray_InsertArray(T *data, DN_USize *size, DN_USize max, DN_USize index, T const *items, DN_USize count)
{
  T *result = nullptr;
  if (!data || !size || !items || count <= 0 || ((*size + count) > max))
    return result;

  DN_USize clamped_index = DN_Min(index, *size);
  if (clamped_index != *size) {
    char const *src           = DN_Cast(char *)(data + clamped_index);
    char const *dest          = DN_Cast(char *)(data + (clamped_index + count));
    char const *end           = DN_Cast(char *)(data + (*size));
    DN_USize    bytes_to_move = end - src;
    DN_Memmove(DN_Cast(void *) dest, src, bytes_to_move);
  }

  result = data + clamped_index;
  DN_Memcpy(result, items, sizeof(T) * count);
  *size += count;
  return result;
}

template <typename T>
T DN_CArray_PopFront(T *data, DN_USize *size, DN_USize count)
{
  T result = {};
  if (!data || !size || *size <= 0)
    return result;

  result             = data[0];
  DN_USize pop_count = DN_Min(count, *size);
  DN_Memmove(data, data + pop_count, (*size - pop_count) * sizeof(T));
  *size -= pop_count;
  return result;
}

template <typename T>
T DN_CArray_PopBack(T *data, DN_USize *size, DN_USize count)
{
  T result = {};
  if (!data || !size || *size <= 0)
    return result;

  DN_USize pop_count = DN_Min(count, *size);
  result             = data[(*size - 1)];
  *size -= pop_count;
  return result;
}

template <typename T>
DN_ArrayFindResult<T> DN_CArray_Find(T *data, DN_USize size, T const &value)
{
  DN_ArrayFindResult<T> result = {};
  if (!data || size <= 0)
    return result;

  for (DN_USize index = 0; !result.data && index < size; index++) {
    T *item = data + index;
    if (*item == value) {
      result.data  = item;
      result.index = index;
    }
  }

  return result;
}

#if !defined(DN_NO_SARRAY)
// NOTE: DN_SArray /////////////////////////////////////////////////////////////////////////////////
template <typename T>
DN_SArray<T> DN_SArray_Init(DN_Arena *arena, DN_USize size, DN_ZMem z_mem)
{
  DN_SArray<T> result = {};
  if (!arena || !size)
    return result;
  result.data = DN_ArenaNewArray(arena, T, size, z_mem);
  if (result.data)
    result.max = size;
  return result;
}

template <typename T>
DN_SArray<T> DN_SArray_InitSlice(DN_Arena *arena, DN_Slice<T> slice, DN_USize size, DN_ZMem z_mem)
{
  DN_USize     max    = DN_Max(slice.size, size);
  DN_SArray<T> result = DN_SArray_Init<T>(arena, max, DN_ZMem_No);
  if (DN_SArray_IsValid(&result)) {
    DN_SArray_AddArray(&result, slice.data, slice.size);
    if (z_mem == DN_ZMem_Yes)
      DN_Memset(result.data + result.size, 0, (result.max - result.size) * sizeof(T));
  }
  return result;
}

template <typename T, size_t N>
DN_SArray<T> DN_SArray_InitCArray(DN_Arena *arena, T const (&array)[N], DN_USize size, DN_ZMem z_mem)
{
  DN_SArray<T> result = DN_SArray_InitSlice(arena, DN_Slice_Init(DN_Cast(T *) array, N), size, z_mem);
  return result;
}

template <typename T>
DN_SArray<T> DN_SArray_InitBuffer(T *buffer, DN_USize size)
{
  DN_SArray<T> result = {};
  result.data         = buffer;
  result.max          = size;
  return result;
}

template <typename T>
bool DN_SArray_IsValid(DN_SArray<T> const *array)
{
  bool result = array && array->data && array->size <= array->max;
  return result;
}

template <typename T>
DN_Slice<T> DN_SArray_Slice(DN_SArray<T> const *array)
{
  DN_Slice<T> result = {};
  if (array)
    result = DN_Slice_Init<T>(DN_Cast(T *) array->data, array->size);
  return result;
}

template <typename T>
T *DN_SArray_MakeArray(DN_SArray<T> *array, DN_USize count, DN_ZMem z_mem)
{
  if (!DN_SArray_IsValid(array))
    return nullptr;
  T *result = DN_CArray_MakeArray(array->data, &array->size, array->max, count, z_mem);
  return result;
}

template <typename T>
T *DN_SArray_Make(DN_SArray<T> *array, DN_ZMem z_mem)
{
  T *result = DN_SArray_MakeArray(array, 1, z_mem);
  return result;
}

template <typename T>
T *DN_SArray_AddArray(DN_SArray<T> *array, T const *items, DN_USize count)
{
  T *result = DN_SArray_MakeArray(array, count, DN_ZMem_No);
  if (result)
    DN_Memcpy(result, items, count * sizeof(T));
  return result;
}

template <typename T, DN_USize N>
T *DN_SArray_AddCArray(DN_SArray<T> *array, T const (&items)[N])
{
  T *result = DN_SArray_AddArray(array, items, N);
  return result;
}

template <typename T>
T *DN_SArray_Add(DN_SArray<T> *array, T const &item)
{
  T *result = DN_SArray_AddArray(array, &item, 1);
  return result;
}

template <typename T>
T *DN_SArray_InsertArray(DN_SArray<T> *array, DN_USize index, T const *items, DN_USize count)
{
  T *result = nullptr;
  if (!DN_SArray_IsValid(array))
    return result;
  result = DN_CArray_InsertArray(array->data, &array->size, array->max, index, items, count);
  return result;
}

template <typename T, DN_USize N>
T *DN_SArray_InsertCArray(DN_SArray<T> *array, DN_USize index, T const (&items)[N])
{
  T *result = DN_SArray_InsertArray(array, index, items, N);
  return result;
}

template <typename T>
T *DN_SArray_Insert(DN_SArray<T> *array, DN_USize index, T const &item)
{
  T *result = DN_SArray_InsertArray(array, index, &item, 1);
  return result;
}

template <typename T>
T DN_SArray_PopFront(DN_SArray<T> *array, DN_USize count)
{
  T result = DN_CArray_PopFront(array->data, &array->size, count);
  return result;
}

template <typename T>
T DN_SArray_PopBack(DN_SArray<T> *array, DN_USize count)
{
  T result = DN_CArray_PopBack(array->data, &array->size, count);
  return result;
}

template <typename T>
DN_ArrayEraseResult DN_SArray_EraseRange(DN_SArray<T> *array, DN_USize begin_index, DN_ISize count, DN_ArrayErase erase)
{
  DN_ArrayEraseResult result = {};
  if (!DN_SArray_IsValid(array) || array->size == 0 || count == 0)
    return result;
  result = DN_CArray_EraseRange(array->data, &array->size, begin_index, count, erase);
  return result;
}

template <typename T>
void DN_SArray_Clear(DN_SArray<T> *array)
{
  if (array)
    array->size = 0;
}
#endif // !defined(DN_NO_SARRAY)

#if !defined(DN_NO_FARRAY)
// NOTE: DN_FArray /////////////////////////////////////////////////////////////////////////////////
template <typename T, DN_USize N>
DN_FArray<T, N> DN_FArray_Init(T const *array, DN_USize count)
{
  DN_FArray<T, N> result = {};
  bool            added  = DN_FArray_AddArray(&result, array, count);
  DN_Assert(added);
  return result;
}

template <typename T, DN_USize N>
DN_FArray<T, N> DN_FArray_InitSlice(DN_Slice<T> slice)
{
  DN_FArray<T, N> result = DN_FArray_Init(slice.data, slice.size);
  return result;
}

template <typename T, DN_USize N, DN_USize K>
DN_FArray<T, N> DN_FArray_InitCArray(T const (&items)[K])
{
  DN_FArray<T, N> result = DN_FArray_Init<T, N>(items, K);
  return result;
}

template <typename T, DN_USize N>
bool DN_FArray_IsValid(DN_FArray<T, N> const *array)
{
  bool result = array && array->size <= DN_ArrayCountU(array->data);
  return result;
}

template <typename T, DN_USize N>
DN_Slice<T> DN_FArray_Slice(DN_FArray<T, N> const *array)
{
  DN_Slice<T> result = {};
  if (array)
    result = DN_Slice_Init<T>(DN_Cast(T *) array->data, array->size);
  return result;
}

template <typename T, DN_USize N>
T *DN_FArray_AddArray(DN_FArray<T, N> *array, T const *items, DN_USize count)
{
  T *result = DN_FArray_MakeArray(array, count, DN_ZMem_No);
  if (result)
    DN_Memcpy(result, items, count * sizeof(T));
  return result;
}

template <typename T, DN_USize N, DN_USize K>
T *DN_FArray_AddCArray(DN_FArray<T, N> *array, T const (&items)[K])
{
  T *result = DN_FArray_MakeArray(array, K, DN_ZMem_No);
  if (result)
    DN_Memcpy(result, items, K * sizeof(T));
  return result;
}

template <typename T, DN_USize N>
T *DN_FArray_Add(DN_FArray<T, N> *array, T const &item)
{
  T *result = DN_FArray_AddArray(array, &item, 1);
  return result;
}

template <typename T, DN_USize N>
T *DN_FArray_MakeArray(DN_FArray<T, N> *array, DN_USize count, DN_ZMem z_mem)
{
  if (!DN_FArray_IsValid(array))
    return nullptr;
  T *result = DN_CArray_MakeArray(array->data, &array->size, N, count, z_mem);
  return result;
}

template <typename T, DN_USize N>
T *DN_FArray_Make(DN_FArray<T, N> *array, DN_ZMem z_mem)
{
  T *result = DN_FArray_MakeArray(array, 1, z_mem);
  return result;
}

template <typename T, DN_USize N>
T *DN_FArray_InsertArray(DN_FArray<T, N> *array, DN_USize index, T const *items, DN_USize count)
{
  T *result = nullptr;
  if (!DN_FArray_IsValid(array))
    return result;
  result = DN_CArray_InsertArray(array->data, &array->size, N, index, items, count);
  return result;
}

template <typename T, DN_USize N, DN_USize K>
T *DN_FArray_InsertCArray(DN_FArray<T, N> *array, DN_USize index, T const (&items)[K])
{
  T *result = DN_FArray_InsertArray(array, index, items, K);
  return result;
}

template <typename T, DN_USize N>
T *DN_FArray_Insert(DN_FArray<T, N> *array, DN_USize index, T const &item)
{
  T *result = DN_FArray_InsertArray(array, index, &item, 1);
  return result;
}

template <typename T, DN_USize N>
T DN_FArray_PopFront(DN_FArray<T, N> *array, DN_USize count)
{
  T result = DN_CArray_PopFront(array->data, &array->size, count);
  return result;
}

template <typename T, DN_USize N>
T DN_FArray_PopBack(DN_FArray<T, N> *array, DN_USize count)
{
  T result = DN_CArray_PopBack(array->data, &array->size, count);
  return result;
}

template <typename T, DN_USize N>
DN_ArrayFindResult<T> DN_FArray_Find(DN_FArray<T, N> *array, T const &find)
{
  DN_ArrayFindResult<T> result = DN_CArray_Find<T>(array->data, array->size, find);
  return result;
}

template <typename T, DN_USize N>
DN_ArrayEraseResult DN_FArray_EraseRange(DN_FArray<T, N> *array, DN_USize begin_index, DN_ISize count, DN_ArrayErase erase)
{
  DN_ArrayEraseResult result = {};
  if (!DN_FArray_IsValid(array) || array->size == 0 || count == 0)
    return result;
  result = DN_CArray_EraseRange(array->data, &array->size, begin_index, count, erase);
  return result;
}

template <typename T, DN_USize N>
void DN_FArray_Clear(DN_FArray<T, N> *array)
{
  if (array)
    array->size = 0;
}
#endif // !defined(DN_NO_FARRAY)

#if !defined(DN_NO_SLICE)
template <typename T>
DN_Slice<T> DN_Slice_Init(T *const data, DN_USize size)
{
  DN_Slice<T> result = {};
  if (data) {
    result.data = data;
    result.size = size;
  }
  return result;
}

template <typename T, DN_USize N>
DN_Slice<T> DN_Slice_InitCArrayCopy(DN_Arena *arena, T const (&array)[N])
{
  DN_Slice<T> result = DN_Slice_Alloc<T>(arena, N, DN_ZMem_No);
  if (result.data)
    DN_Memcpy(result.data, array, sizeof(T) * N);
  return result;
}

template <typename T>
DN_Slice<T> DN_Slice_CopyPtr(DN_Arena *arena, T *const data, DN_USize size)
{
  T          *copy   = DN_ArenaNewArrayCopy(arena, T, data, size);
  DN_Slice<T> result = DN_Slice_Init(copy, copy ? size : 0);
  return result;
}

template <typename T>
DN_Slice<T> DN_Slice_Copy(DN_Arena *arena, DN_Slice<T> slice)
{
  DN_Slice<T> result = DN_Slice_CopyPtr(arena, slice.data, slice.size);
  return result;
}

template <typename T>
DN_Slice<T> DN_Slice_Alloc(DN_Arena *arena, DN_USize size, DN_ZMem z_mem)
{
  DN_Slice<T> result = {};
  if (!arena || size == 0)
    return result;
  result.data = DN_ArenaNewArray(arena, T, size, z_mem);
  if (result.data)
    result.size = size;
  return result;
}

#endif // !defined(DN_NO_SLICE)

#if !defined(DN_NO_DSMAP)
// NOTE: DN_DSMap //////////////////////////////////////////////////////////////////////////////////
DN_U32 const DN_DS_MAP_DEFAULT_HASH_SEED = 0x8a1ced49;
DN_U32 const DN_DS_MAP_SENTINEL_SLOT     = 0;

template <typename T>
DN_DSMap<T> DN_DSMap_Init(DN_Arena *arena, DN_U32 size, DN_DSMapFlags flags)
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
void DN_DSMap_Deinit(DN_DSMap<T> *map, DN_ZMem z_mem)
{
  if (!map)
    return;
  // TODO(doyle): Use z_mem
  (void)z_mem;
  DN_ArenaDeinit(map->arena);
  *map = {};
}

template <typename T>
bool DN_DSMap_IsValid(DN_DSMap<T> const *map)
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
DN_U32 DN_DSMap_Hash(DN_DSMap<T> const *map, DN_DSMapKey key)
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
DN_U32 DN_DSMap_HashToSlotIndex(DN_DSMap<T> const *map, DN_DSMapKey key)
{
  DN_Assert(key.type != DN_DSMapKeyType_Invalid);
  DN_U32 result = DN_DS_MAP_SENTINEL_SLOT;
  if (!DN_DSMap_IsValid(map))
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
DN_DSMapResult<T> DN_DSMap_Find(DN_DSMap<T> const *map, DN_DSMapKey key)
{
  DN_DSMapResult<T> result = {};
  if (DN_DSMap_IsValid(map)) {
    DN_U32 index = DN_DSMap_HashToSlotIndex(map, key);
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
DN_DSMapResult<T> DN_DSMap_Make(DN_DSMap<T> *map, DN_DSMapKey key)
{
  DN_DSMapResult<T> result = {};
  if (!DN_DSMap_IsValid(map))
    return result;

  DN_U32 index = DN_DSMap_HashToSlotIndex(map, key);
  if (map->hash_to_slot[index] == DN_DS_MAP_SENTINEL_SLOT) {
    // NOTE: Create the slot
    if (index != DN_DS_MAP_SENTINEL_SLOT)
      map->hash_to_slot[index] = map->occupied++;

    // NOTE: Check if resize is required
    bool map_is_75pct_full = (map->occupied * 4) > (map->size * 3);
    if (map_is_75pct_full) {
      if (!DN_DSMap_Resize(map, map->size * 2))
        return result;
      result = DN_DSMap_Make(map, key);
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
DN_DSMapResult<T> DN_DSMap_Set(DN_DSMap<T> *map, DN_DSMapKey key, T const &value)
{
  DN_DSMapResult<T> result = {};
  if (!DN_DSMap_IsValid(map))
    return result;

  result             = DN_DSMap_Make(map, key);
  result.slot->value = value;
  return result;
}

template <typename T>
DN_DSMapResult<T> DN_DSMap_FindKeyU64(DN_DSMap<T> const *map, DN_U64 key)
{
  DN_DSMapKey       map_key = DN_DSMap_KeyU64(map, key);
  DN_DSMapResult<T> result  = DN_DSMap_Find(map, map_key);
  return result;
}

template <typename T>
DN_DSMapResult<T> DN_DSMap_MakeKeyU64(DN_DSMap<T> *map, DN_U64 key)
{
  DN_DSMapKey       map_key = DN_DSMap_KeyU64(map, key);
  DN_DSMapResult<T> result  = DN_DSMap_Make(map, map_key);
  return result;
}

template <typename T>
DN_DSMapResult<T> DN_DSMap_SetKeyU64(DN_DSMap<T> *map, DN_U64 key, T const &value)
{
  DN_DSMapKey       map_key = DN_DSMap_KeyU64(map, key);
  DN_DSMapResult<T> result  = DN_DSMap_Set(map, map_key, value);
  return result;
}

template <typename T>
DN_DSMapResult<T> DN_DSMap_FindKeyStr8(DN_DSMap<T> const *map, DN_Str8 key)
{
  DN_DSMapKey       map_key = DN_DSMap_KeyStr8(map, key);
  DN_DSMapResult<T> result  = DN_DSMap_Find(map, map_key);
  return result;
}

template <typename T>
DN_DSMapResult<T> DN_DSMap_MakeKeyStr8(DN_DSMap<T> *map, DN_Str8 key)
{
  DN_DSMapKey       map_key = DN_DSMap_KeyStr8(map, key);
  DN_DSMapResult<T> result  = DN_DSMap_Make(map, map_key);
  return result;
}

template <typename T>
DN_DSMapResult<T> DN_DSMap_SetKeyStr8(DN_DSMap<T> *map, DN_Str8 key, T const &value)
{
  DN_DSMapKey       map_key = DN_DSMap_KeyStr8(map, key);
  DN_DSMapResult<T> result  = DN_DSMap_Set(map, map_key);
  return result;
}

template <typename T>
bool DN_DSMap_Resize(DN_DSMap<T> *map, DN_U32 size)
{
  if (!DN_DSMap_IsValid(map) || size < map->occupied || size < map->initial_size)
    return false;

  DN_Arena *prev_arena = map->arena;
  DN_Arena  new_arena  = {};
  new_arena.mem_funcs  = prev_arena->mem_funcs;
  new_arena.flags      = prev_arena->flags;
  new_arena.label      = prev_arena->label;
  new_arena.prev       = prev_arena->prev;
  new_arena.next       = prev_arena->next;

  DN_DSMap<T> new_map = DN_DSMap_Init<T>(&new_arena, size, map->flags);
  if (!DN_DSMap_IsValid(&new_map))
    return false;

  new_map.initial_size = map->initial_size;
  for (DN_U32 old_index = 1 /*Sentinel*/; old_index < map->occupied; old_index++) {
    DN_DSMapSlot<T> *old_slot = map->slots + old_index;
    DN_DSMapKey      old_key  = old_slot->key;
    if (old_key.type == DN_DSMapKeyType_Invalid)
      continue;
    DN_DSMap_Set(&new_map, old_key, old_slot->value);
  }

  if ((map->flags & DN_DSMapFlags_DontFreeArenaOnResize) == 0)
    DN_DSMap_Deinit(map, DN_ZMem_No);
  *map            = new_map;    // Update the map inplace
  map->arena      = prev_arena; // Restore the previous arena pointer, it's been de-init-ed
  *map->arena     = new_arena;  // Re-init the old arena with the new data
  map->pool.arena = map->arena;
  return true;
}

template <typename T>
bool DN_DSMap_Erase(DN_DSMap<T> *map, DN_DSMapKey key)
{
  if (!DN_DSMap_IsValid(map))
    return false;

  DN_U32 index = DN_DSMap_HashToSlotIndex(map, key);
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
        DN_U32 hash_to_slot_index             = DN_DSMap_HashToSlotIndex(map, last_slot->key);
        map->hash_to_slot[hash_to_slot_index] = slot_index;
        *last_slot                            = {}; // TODO: Optional?
      }
    }
  }

  map->occupied--;
  bool map_is_below_25pct_full = (map->occupied * 4) < (map->size * 1);
  if (map_is_below_25pct_full && (map->size / 2) >= map->initial_size)
    DN_DSMap_Resize(map, map->size / 2);

  return true;
}

template <typename T>
bool DN_DSMap_EraseKeyU64(DN_DSMap<T> *map, DN_U64 key)
{
  DN_DSMapKey map_key = DN_DSMap_KeyU64(map, key);
  bool        result  = DN_DSMap_Erase(map, map_key);
  return result;
}

template <typename T>
bool DN_DSMap_EraseKeyStr8(DN_DSMap<T> *map, DN_Str8 key)
{
  DN_DSMapKey map_key = DN_DSMap_KeyStr8(map, key);
  bool        result  = DN_DSMap_Erase(map, map_key);
  return result;
}

template <typename T>
DN_DSMapKey DN_DSMap_KeyBuffer(DN_DSMap<T> const *map, void const *data, DN_USize size)
{
  DN_Assert(size > 0 && size <= UINT32_MAX);
  DN_DSMapKey result = {};
  result.type        = DN_DSMapKeyType_Buffer;
  result.buffer_data = data;
  result.buffer_size = DN_Cast(DN_U32) size;
  result.hash        = DN_DSMap_Hash(map, result);
  return result;
}

template <typename T>
DN_DSMapKey DN_DSMap_KeyBufferAsU64NoHash(DN_DSMap<T> const *map, void const *data, DN_U32 size)
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
DN_DSMapKey DN_DSMap_KeyU64(DN_DSMap<T> const *map, DN_U64 u64)
{
  DN_DSMapKey result = {};
  result.type        = DN_DSMapKeyType_U64;
  result.u64         = u64;
  result.hash        = DN_DSMap_Hash(map, result);
  return result;
}

template <typename T>
DN_DSMapKey DN_DSMap_KeyStr8(DN_DSMap<T> const *map, DN_Str8 string)
{
  DN_DSMapKey result = DN_DSMap_KeyBuffer(map, string.data, string.size);
  return result;
}
#endif // !defined(DN_NO_DSMAP)

#if !defined(DN_NO_LIST)
// NOTE: DN_List ///////////////////////////////////////////////////////////////////////////////////
template <typename T>
DN_List<T> DN_List_Init(DN_USize chunk_size)
{
  DN_List<T> result = {};
  result.chunk_size = chunk_size;
  return result;
}

template <typename T, size_t N>
DN_List<T> DN_List_InitCArray(DN_Arena *arena, DN_USize chunk_size, T const (&array)[N])
{
  DN_List<T> result = DN_List_Init<T>(arena, chunk_size);
  for (DN_ForIndexU(index, N))
    DN_List_Add(&result, array[index]);
  return result;
}

template <typename T>
DN_List<T> DN_List_InitSliceCopy(DN_Arena *arena, DN_USize chunk_size, DN_Slice<T> slice)
{
  DN_List<T> result = DN_List_Init<T>(arena, chunk_size);
  for (DN_ForIndexU(index, slice.size))
    DN_List_Add(&result, slice.data[index]);
  return result;
}

template <typename T>
DN_API bool DN_List_AttachTail_(DN_List<T> *list, DN_ListChunk<T> *tail)
{
  if (!tail)
    return false;

  if (list->tail) {
    list->tail->next = tail;
    tail->prev       = list->tail;
  }

  list->tail = tail;

  if (!list->head)
    list->head = list->tail;
  return true;
}

template <typename T>
DN_API DN_ListChunk<T> *DN_List_AllocArena_(DN_List<T> *list, DN_Arena *arena, DN_USize count)
{
  auto           *result = DN_ArenaNew(arena, DN_ListChunk<T>, DN_ZMem_Yes);
  DN_ArenaTempMem tmem   = DN_ArenaTempMemBegin(arena);
  if (!result)
    return nullptr;

  DN_USize items = DN_Max(list->chunk_size, count);
  result->data   = DN_ArenaNewArray(arena, T, items, DN_ZMem_Yes);
  result->size   = items;
  if (!result->data) {
    DN_ArenaTempMemEnd(tmem);
    result = nullptr;
  }

  DN_List_AttachTail_(list, result);
  return result;
}

template <typename T>
DN_API DN_ListChunk<T> *DN_List_AllocPool_(DN_List<T> *list, DN_Pool *pool, DN_USize count)
{
  auto *result = DN_PoolNew(pool, DN_ListChunk<T>);
  if (!result)
    return nullptr;

  DN_USize items = DN_Max(list->chunk_size, count);
  result->data   = DN_PoolNewArray(pool, T, items);
  result->size   = items;
  if (!result->data) {
    DN_PoolDealloc(result);
    result = nullptr;
  }

  DN_List_AttachTail_(list, result);
  return result;
}

template <typename T>
DN_API T *DN_List_MakeArena(DN_List<T> *list, DN_Arena *arena, DN_USize count)
{
  if (list->chunk_size == 0)
    list->chunk_size = 128;

  if (!list->tail || (list->tail->count + count) > list->tail->size) {
    if (!DN_List_AllocArena_(list, arena, count))
      return nullptr;
  }

  T *result = list->tail->data + list->tail->count;
  list->tail->count += count;
  list->count += count;
  return result;
}

template <typename T>
DN_API T *DN_List_MakePool(DN_List<T> *list, DN_Pool *pool, DN_USize count)
{
  if (list->chunk_size == 0)
    list->chunk_size = 128;

  if (!list->tail || (list->tail->count + count) > list->tail->size) {
    if (!DN_List_AllocPool_(list, pool, count))
      return nullptr;
  }

  T *result = list->tail->data + list->tail->count;
  list->tail->count += count;
  list->count += count;
  return result;
}

template <typename T>
DN_API T *DN_List_AddArena(DN_List<T> *list, DN_Arena *arena, T const &value)
{
  T *result = DN_List_MakeArena(list, arena, 1);
  *result   = value;
  return result;
}

template <typename T>
DN_API T *DN_List_AddPool(DN_List<T> *list, DN_Pool *pool, T const &value)
{
  T *result = DN_List_MakePool(list, pool, 1);
  *result   = value;
  return result;
}

template <typename T, size_t N>
DN_API bool DN_List_AddCArray(DN_List<T> *list, T const (&array)[N])
{
  if (!list)
    return false;
  for (T const &item : array)
    if (!DN_List_Add(list, item))
      return false;
  return true;
}

template <typename T>
DN_API void DN_List_AddListArena(DN_List<T> *list, DN_Arena *arena, DN_List<T> other)
{
  if (!list)
    return;
  // TODO(doyle): Copy chunk by chunk
  for (DN_ListIterator<T> it = {}; DN_List_Iterate(&other, &it, 0 /*start_index*/);)
    DN_List_AddArena(list, arena, *it.data);
}

template <typename T>
DN_API void DN_List_AddListPool(DN_List<T> *list, DN_Pool *pool, DN_List<T> other)
{
  if (!list)
    return;
  // TODO(doyle): Copy chunk by chunk
  for (DN_ListIterator<T> it = {}; DN_List_Iterate(&other, &it, 0 /*start_index*/);)
    DN_List_AddPool(list, pool, *it.data);
}

template <typename T>
void DN_List_Clear(DN_List<T> *list)
{
  if (!list)
    return;
  list->head = list->tail = nullptr;
  list->count             = 0;
}

template <typename T>
DN_API bool DN_List_Iterate(DN_List<T> *list, DN_ListIterator<T> *it, DN_USize start_index)
{
  bool result = false;
  if (!list || !it || list->chunk_size <= 0)
    return result;

  if (it->init) {
    it->index++;
  } else {
    *it = {};
    if (start_index == 0) {
      it->chunk = list->head;
    } else {
      DN_List_At(list, start_index, &it->chunk);
      if (list->chunk_size > 0)
        it->chunk_data_index = start_index % list->chunk_size;
    }

    it->init = true;
  }

  if (it->chunk) {
    if (it->chunk_data_index >= it->chunk->count) {
      it->chunk            = it->chunk->next;
      it->chunk_data_index = 0;
    }

    if (it->chunk) {
      it->data = it->chunk->data + it->chunk_data_index++;
      result   = true;
    }
  }

  if (!it->chunk)
    DN_Assert(result == false);
  return result;
}

template <typename T>
DN_API T *DN_List_At(DN_List<T> *list, DN_USize index, DN_ListChunk<T> **at_chunk)
{
  if (!list || index >= list->count || !list->head)
    return nullptr;

  // NOTE: Scan forwards to the chunk we need. We don't have random access to chunks
  DN_ListChunk<T> **chunk         = &list->head;
  DN_USize          running_index = index;
  for (; (*chunk) && running_index >= (*chunk)->size; chunk = &((*chunk)->next))
    running_index -= (*chunk)->size;

  T *result = nullptr;
  if (*chunk)
    result = (*chunk)->data + running_index;

  if (result && at_chunk)
    *at_chunk = *chunk;

  return result;
}

template <typename T>
DN_Slice<T> DN_List_ToSliceCopy(DN_List<T> const *list, DN_Arena *arena)
{
  // TODO(doyle): Chunk memcopies is much faster
  DN_Slice<T> result = DN_Slice_Alloc<T>(arena, list->count, DN_ZMem_No);
  if (result.size) {
    DN_USize slice_index = 0;
    DN_MSVC_WARNING_PUSH
    DN_MSVC_WARNING_DISABLE(6011) // Dereferencing NULL pointer 'x'
    for (DN_ListIterator<T> it = {}; DN_List_Iterate<T>(DN_Cast(DN_List<T> *) list, &it, 0);)
      result.data[slice_index++] = *it.data;
    DN_MSVC_WARNING_POP
    DN_Assert(slice_index == result.size);
  }
  return result;
}
#endif // !defined(DN_NO_LIST)

// NOTE: DN_Slice //////////////////////////////////////////////////////////////////////////////////
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

#if !defined(DN_NO_DSMAP)
// NOTE: DN_DSMap //////////////////////////////////////////////////////////////////////////////////
DN_API DN_DSMapKey DN_DSMap_KeyU64NoHash(DN_U64 u64)
{
  DN_DSMapKey result = {};
  result.type        = DN_DSMapKeyType_U64NoHash;
  result.u64         = u64;
  result.hash        = DN_Cast(DN_U32) u64;
  return result;
}

DN_API bool DN_DSMap_KeyEquals(DN_DSMapKey lhs, DN_DSMapKey rhs)
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
  bool result = DN_DSMap_KeyEquals(lhs, rhs);
  return result;
}
#endif // !defined(DN_NO_DSMAP)
