#if !defined(DN_CONTAINERS_H)
#define DN_CONTAINERS_H

#include "../dn_base_inc.h"

struct DN_Ring
{
  DN_U64 size;
  char  *base;
  DN_U64 write_pos;
  DN_U64 read_pos;
};

// NOTE: DN_CArray /////////////////////////////////////////////////////////////////////////////////
enum DN_ArrayErase
{
  DN_ArrayErase_Unstable,
  DN_ArrayErase_Stable,
};

enum DN_ArrayAdd
{
  DN_ArrayAdd_Append,
  DN_ArrayAdd_Prepend,
};

struct DN_ArrayEraseResult
{
  // The next index your for-index should be set to such that you can continue
  // to iterate the remainder of the array, e.g:
  //
  // for (DN_USize index = 0; index < array.size; index++) {
  //     if (erase)
  //          index = DN_FArray_EraseRange(&array, index, -3, DN_ArrayErase_Unstable);
  // }
  DN_USize it_index;
  DN_USize items_erased; // The number of items erased
};

template <typename T> struct DN_ArrayFindResult
{
  T        *data;  // Pointer to the value if a match is found, null pointer otherwise
  DN_USize index; // Index to the value if a match is found, 0 otherwise
};

#if !defined(DN_NO_SARRAY)
// NOTE: DN_SArray /////////////////////////////////////////////////////////////////////////////////
template <typename T> struct DN_SArray
{
  T       *data; // Pointer to the start of the array items in the block of memory
  DN_USize size; // Number of items currently in the array
  DN_USize max;  // Maximum number of items this array can store

  T       *begin()       { return data; }
  T       *end  ()       { return data + size; }
  T const *begin() const { return data; }
  T const *end  () const { return data + size; }
};
#endif // !defined(DN_NO_SARRAY)

#if !defined(DN_NO_FARRAY)
// NOTE: DN_FArray /////////////////////////////////////////////////////////////////////////////////
template <typename T, DN_USize N> struct DN_FArray
{
  T        data[N]; // Pointer to the start of the array items in the block of memory
  DN_USize size;    // Number of items currently in the array

  T       *begin()       { return data; }
  T       *end  ()       { return data + size; }
  T const *begin() const { return data; }
  T const *end  () const { return data + size; }
};

template <typename T> using DN_FArray8  = DN_FArray<T, 8>;
template <typename T> using DN_FArray16 = DN_FArray<T, 16>;
template <typename T> using DN_FArray32 = DN_FArray<T, 32>;
template <typename T> using DN_FArray64 = DN_FArray<T, 64>;
#endif // !defined(DN_NO_FARRAY)

#if !defined(DN_NO_DSMAP)
// NOTE: DN_DSMap //////////////////////////////////////////////////////////////////////////////////
enum DN_DSMapKeyType
{
                                     // Key    | Key Hash         | Map Index
  DN_DSMapKeyType_Invalid,
  DN_DSMapKeyType_U64,               // U64    | Hash(U64)        | Hash(U64)        % map_size
  DN_DSMapKeyType_U64NoHash,         // U64    | U64              | U64              % map_size
  DN_DSMapKeyType_Buffer,            // Buffer | Hash(buffer)     | Hash(buffer)     % map_size
  DN_DSMapKeyType_BufferAsU64NoHash, // Buffer | U64(buffer[0:4]) | U64(buffer[0:4]) % map_size
};

struct DN_DSMapKey
{
  DN_DSMapKeyType  type;
  DN_U32           hash; // Hash to lookup in the map. If it equals, we check that the original key payload matches
  void const      *buffer_data;
  DN_U32           buffer_size;
  DN_U64           u64;
  bool             no_copy_buffer;
};

template <typename T>
struct DN_DSMapSlot
{
  DN_DSMapKey key;   // Hash table lookup key
  T           value; // Hash table value
};

typedef DN_U32 DN_DSMapFlags;
enum DN_DSMapFlags_
{
  DN_DSMapFlags_Nil                   = 0,
  DN_DSMapFlags_DontFreeArenaOnResize = 1 << 0,
};

using DN_DSMapHashFunction = DN_U32(DN_DSMapKey key, DN_U32 seed);
template <typename T> struct DN_DSMap
{
  DN_U32               *hash_to_slot;  // Mapping from hash to a index in the slots array
  DN_DSMapSlot<T>      *slots;         // Values of the array stored contiguously, non-sorted order
  DN_U32                size;          // Total capacity of the map and is a power of two
  DN_U32                occupied;      // Number of slots used in the hash table
  DN_Arena             *arena;         // Backing arena for the hash table
  DN_Pool               pool;          // Allocator for keys that are variable-sized buffers
  DN_U32                initial_size;  // Initial map size, map cannot shrink on erase below this size
  DN_DSMapHashFunction *hash_function; // Custom hashing function to use if field is set
  DN_U32                hash_seed;     // Seed for the hashing function, when 0, DN_DS_MAP_DEFAULT_HASH_SEED is used
  DN_DSMapFlags         flags;
};

template <typename T> struct DN_DSMapResult
{
  bool             found;
  DN_DSMapSlot<T> *slot;
  T               *value;
};
#endif // !defined(DN_NO_DSMAP)

#if !defined(DN_NO_LIST)
// NOTE: DN_List ///////////////////////////////////////////////////////////////////////////////////
template <typename T> struct DN_ListChunk
{
  T               *data;
  DN_USize         size;
  DN_USize         count;
  DN_ListChunk<T> *next;
  DN_ListChunk<T> *prev;
};

template <typename T> struct DN_ListIterator
{
  DN_B32           init;             // True if DN_List_Iterate has been called at-least once on this iterator
  DN_ListChunk<T> *chunk;            // The chunk that the iterator is reading from
  DN_USize         chunk_data_index; // The index in the chunk the iterator is referencing
  DN_USize         index;            // The index of the item in the list as if it was flat array
  T               *data;             // Pointer to the data the iterator is referencing. Nullptr if invalid.
};

template <typename T> struct DN_List
{
  DN_USize         count;      // Cumulative count of all items made across all list chunks
  DN_USize         chunk_size; // When new ListChunk's are required, the minimum 'data' entries to allocate for that node.
  DN_ListChunk<T> *head;
  DN_ListChunk<T> *tail;
};
#endif // !defined(DN_NO_LIST)

// NOTE: Macros for operating on data structures that are embedded into a C style struct or from a
// set of defined variables from the available scope. Keep it stupid simple, structs and functions.
// Minimal amount of container types with flexible construction leads to less duplicated container
// code and less template meta-programming.
//
// LArray => Literal Array
//   Define a C array and size:
//
//   ```
//   MyStruct buffer[TB_ASType_Count] = {};
//   DN_USize size                    = 0;
//   MyStruct *item                   = DN_LArray_Make(buffer, size, DN_ArrayCountU(buffer), DN_ZMem_No);
//   ```
//
// IArray => Intrusive Array
//   Define a struct with the members 'data', 'size' and 'max':
//
//   ```
//   struct MyArray {
//     MyStruct *data;
//     DN_USize  size;
//     DN_USize  max;
//   } my_array = {};
//
//   MyStruct *item = DN_IArray_Make(&my_array, MyArray, DN_ZMem_No);
//   ```
// ISLList => Intrusive Singly Linked List
//   Define a struct with the members 'next':
//
//   ```
//   struct MyLinkItem {
//     int         data;
//     MyLinkItem *next;
//   } my_link = {};
//
//   MyLinkItem *first_item = DN_ISLList_Detach(&my_link, MyLinkItem);
//   ```

#define DN_DLList_Init(list) \
  (list)->next = (list)->prev = (list)

#define DN_DLList_IsSentinel(list, item) ((list) == (item))

#define DN_DLList_InitArena(list, T, arena)          \
  do {                                               \
    (list) = DN_ArenaNew(arena, T, DN_ZMem_Yes); \
    DN_DLList_Init(list);                            \
  } while (0)

#define DN_DLList_InitPool(list, T, pool) \
  do {                                    \
    (list) = DN_PoolNew(pool, T);        \
    DN_DLList_Init(list);                 \
  } while (0)

#define DN_DLList_Detach(item)      \
  do {                                   \
    if (item) {                          \
      (item)->prev->next = (item)->next; \
      (item)->next->prev = (item)->prev; \
      (item)->next       = nullptr;      \
      (item)->prev       = nullptr;      \
    }                                    \
  } while (0)

#define DN_DLList_Dequeue(list, dest_ptr) \
  if (DN_DLList_HasItems(list)) {         \
    dest_ptr = (list)->next;              \
    DN_DLList_Detach(dest_ptr);           \
  }

#define DN_DLList_Append(list, item) \
  do {                                    \
    if (item) {                           \
      if ((item)->next)                   \
        DN_DLList_Detach(item);           \
      (item)->next       = (list)->next;  \
      (item)->prev       = (list);        \
      (item)->next->prev = (item);        \
      (item)->prev->next = (item);        \
    }                                     \
  } while (0)

#define DN_DLList_Prepend(list, item) \
  do {                                     \
    if (item) {                            \
      if ((item)->next)                    \
        DN_DLList_Detach(item);            \
      (item)->next       = (list);         \
      (item)->prev       = (list)->prev;   \
      (item)->next->prev = (item);         \
      (item)->prev->next = (item);         \
    }                                      \
  } while (0)

#define DN_DLList_IsEmpty(list) \
  (!(list) || ((list) == (list)->next))

#define DN_DLList_IsInit(list) \
  ((list)->next && (list)->prev)

#define DN_DLList_HasItems(list) \
  ((list) && ((list) != (list)->next))

#define DN_DLList_ForEach(it, list) \
  auto *it = (list)->next; (it) != (list); (it) = (it)->next


#define                      DN_ISLList_Detach(list)                                             (decltype(list)) DN_CSLList_Detach((void **)&(list), (void **)&(list)->next)

#define                      DN_LArray_ResizeFromPool(c_array, size, max, pool, new_max)         DN_CArray2_ResizeFromPool((void **)&(c_array), size, max, sizeof((c_array)[0]), pool, new_max)
#define                      DN_LArray_GrowFromPool(c_array, size, max, pool, new_max)           DN_CArray2_GrowFromPool((void **)&(c_array), size, max, sizeof((c_array)[0]), pool, new_max)
#define                      DN_LArray_GrowIfNeededFromPool(c_array, size, max, pool, add_count) DN_CArray2_GrowIfNeededFromPool((void **)(c_array), size, max, sizeof((c_array)[0]), pool, add_count)
#define                      DN_LArray_MakeArray(c_array, size, max, count, z_mem)            (decltype(&(c_array)[0])) DN_CArray2_MakeArray(c_array, size, max, sizeof((c_array)[0]), count, z_mem)
#define                      DN_LArray_MakeArrayZ(c_array, size, max, count)                     (decltype(&(c_array)[0])) DN_CArray2_MakeArray(c_array, size, max, sizeof((c_array)[0]), count, DN_ZMem_Yes)
#define                      DN_LArray_Make(c_array, size, max, z_mem)                        (decltype(&(c_array)[0])) DN_CArray2_MakeArray(c_array, size, max, sizeof((c_array)[0]), 1,     z_mem)
#define                      DN_LArray_MakeZ(c_array, size, max)                                 (decltype(&(c_array)[0])) DN_CArray2_MakeArray(c_array, size, max, sizeof((c_array)[0]), 1,     DN_ZMem_Yes)
#define                      DN_LArray_AddArray(c_array, size, max, items, count, add)           (decltype(&(c_array)[0])) DN_CArray2_AddArray (c_array, size, max, sizeof((c_array)[0]), items, count, add)
#define                      DN_LArray_Add(c_array, size, max, item, add)                        (decltype(&(c_array)[0])) DN_CArray2_AddArray (c_array, size, max, sizeof((c_array)[0]), &item, 1,     add)
#define                      DN_LArray_AppendArray(c_array, size, max, items, count)             (decltype(&(c_array)[0])) DN_CArray2_AddArray (c_array, size, max, sizeof((c_array)[0]), items, count, DN_ArrayAdd_Append)
#define                      DN_LArray_Append(c_array, size, max, item)                          (decltype(&(c_array)[0])) DN_CArray2_AddArray (c_array, size, max, sizeof((c_array)[0]), &item, 1,     DN_ArrayAdd_Append)
#define                      DN_LArray_PrependArray(c_array, size, max, items, count)            (decltype(&(c_array)[0])) DN_CArray2_AddArray (c_array, size, max, sizeof((c_array)[0]), items, count, DN_ArrayAdd_Prepend)
#define                      DN_LArray_Prepend(c_array, size, max, item)                         (decltype(&(c_array)[0])) DN_CArray2_AddArray (c_array, size, max, sizeof((c_array)[0]), &item, 1,     DN_ArrayAdd_Prepend)
#define                      DN_LArray_EraseRange(c_array, size, begin_index, count, erase)      DN_CArray2_EraseRange(c_array, size, sizeof((c_array)[0]), begin_index, count, erase)
#define                      DN_LArray_Erase(c_array, size, index, erase)                        DN_CArray2_EraseRange(c_array, size, sizeof((c_array)[0]), index,       1,     erase)
#define                      DN_LArray_InsertArray(c_array, size, max, index, items, count)      (decltype(&(c_array)[0])) DN_CArray2_InsertArray(c_array, size, max, sizeof((c_array)[0]), index, items, count)
#define                      DN_LArray_Insert(c_array, size, max, index, item)                   (decltype(&(c_array)[0])) DN_CArray2_InsertArray(c_array, size, max, sizeof((c_array)[0]), index, &item, 1)

#define                      DN_IArray_ResizeFromPool(array, pool, new_max)                      DN_CArray2_ResizeFromPool((void **)(&(array)->data), &(array)->size, &(array)->max, sizeof((array)->data[0]), pool, new_max)
#define                      DN_IArray_GrowFromPool(array, pool, new_max)                        DN_CArray2_GrowFromPool((void **)(&(array)->data), &(array)->size, &(array)->max, sizeof((array)->data[0]), pool, new_max)
#define                      DN_IArray_GrowIfNeededFromPool(array, pool, add_count)              DN_CArray2_GrowIfNeededFromPool((void **)(&(array)->data), (array)->size, &(array)->max, sizeof((array)->data[0]), pool, add_count)
#define                      DN_IArray_MakeArray(array, count, z_mem)                         (decltype(&((array)->data)[0])) DN_CArray2_MakeArray((array)->data, &(array)->size, (array)->max, sizeof(((array)->data)[0]), count, z_mem)
#define                      DN_IArray_MakeArrayZ(array, count)                                  (decltype(&((array)->data)[0])) DN_CArray2_MakeArray((array)->data, &(array)->size, (array)->max, sizeof(((array)->data)[0]), count, DN_ZMem_Yes)
#define                      DN_IArray_Make(array, z_mem)                                     (decltype(&((array)->data)[0])) DN_CArray2_MakeArray((array)->data, &(array)->size, (array)->max, sizeof(((array)->data)[0]), 1,     z_mem)
#define                      DN_IArray_MakeZ(array)                                              (decltype(&((array)->data)[0])) DN_CArray2_MakeArray((array)->data, &(array)->size, (array)->max, sizeof(((array)->data)[0]), 1,     DN_ZMem_Yes)
#define                      DN_IArray_AddArray(array, items, count, add)                        (decltype(&((array)->data)[0])) DN_CArray2_AddArray ((array)->data, &(array)->size, (array)->max, sizeof(((array)->data)[0]), items, count, add)
#define                      DN_IArray_Add(array, item, add)                                     (decltype(&((array)->data)[0])) DN_CArray2_AddArray ((array)->data, &(array)->size, (array)->max, sizeof(((array)->data)[0]), &item, 1,     add)
#define                      DN_IArray_AppendArray(array, items, count)                          (decltype(&((array)->data)[0])) DN_CArray2_AddArray ((array)->data, &(array)->size, (array)->max, sizeof(((array)->data)[0]), items, count, DN_ArrayAdd_Append)
#define                      DN_IArray_Append(array, item)                                       (decltype(&((array)->data)[0])) DN_CArray2_AddArray ((array)->data, &(array)->size, (array)->max, sizeof(((array)->data)[0]), &item, 1,     DN_ArrayAdd_Append)
#define                      DN_IArray_PrependArray(array, items, count)                         (decltype(&((array)->data)[0])) DN_CArray2_AddArray ((array)->data, &(array)->size, (array)->max, sizeof(((array)->data)[0]), items, count, DN_ArrayAdd_Prepend)
#define                      DN_IArray_Prepend(array, item)                                      (decltype(&((array)->data)[0])) DN_CArray2_AddArray ((array)->data, &(array)->size, (array)->max, sizeof(((array)->data)[0]), &item, 1,     DN_ArrayAdd_Prepend)
#define                      DN_IArray_EraseRange(array, begin_index, count, erase)              DN_CArray2_EraseRange((array)->data, &(array)->size, sizeof(((array)->data)[0]), begin_index, count, erase)
#define                      DN_IArray_Erase(array, index, erase)                                DN_CArray2_EraseRange((array)->data, &(array)->size, sizeof(((array)->data)[0]), index,           1, erase)
#define                      DN_IArray_InsertArray(array, index, items, count)                   (decltype(&((array)->data)[0])) DN_CArray2_InsertArray((array)->data, &(array)->size, (array)->max, sizeof(((array)->data)[0]), index, items, count)
#define                      DN_IArray_Insert(array, index, item, count)                         (decltype(&((array)->data)[0])) DN_CArray2_InsertArray((array)->data, &(array)->size, (array)->max, sizeof(((array)->data)[0]), index, &item, 1)

DN_API  DN_ArrayEraseResult  DN_CArray2_EraseRange           (void *data, DN_USize *size, DN_USize elem_size, DN_USize begin_index, DN_ISize count, DN_ArrayErase erase);
DN_API  void                *DN_CArray2_MakeArray            (void *data, DN_USize *size, DN_USize max, DN_USize data_size, DN_USize make_size, DN_ZMem z_mem);
DN_API  void                *DN_CArray2_AddArray             (void *data, DN_USize *size, DN_USize max, DN_USize data_size, void const *elems, DN_USize elems_count, DN_ArrayAdd add);
DN_API  bool                 DN_CArray2_Resize               (void **data, DN_USize *size, DN_USize *max, DN_USize data_size, DN_Pool *pool, DN_USize new_max);
DN_API  bool                 DN_CArray2_Grow                 (void **data, DN_USize *size, DN_USize *max, DN_USize data_size, DN_Pool *pool, DN_USize new_max);
DN_API  bool                 DN_CArray2_GrowIfNeededFromPool (void **data, DN_USize size, DN_USize *max, DN_USize data_size, DN_Pool *pool);
DN_API  void                *DN_CSLList_Detach               (void **link, void **next);

DN_API  bool                 DN_Ring_HasSpace                (DN_Ring const *ring, DN_U64 size);
DN_API  bool                 DN_Ring_HasData                 (DN_Ring const *ring, DN_U64 size);
DN_API  void                 DN_Ring_Write                   (DN_Ring *ring, void const *src, DN_U64 src_size);
#define                      DN_Ring_WriteStruct(ring, item) DN_Ring_Write((ring), (item), sizeof(*(item)))
DN_API  void                 DN_Ring_Read                    (DN_Ring *ring, void *dest, DN_U64 dest_size);
#define                      DN_Ring_ReadStruct(ring, dest)  DN_Ring_Read((ring), (dest), sizeof(*(dest)))

template <typename T>                           DN_ArrayEraseResult   DN_CArray_EraseRange              (T *data, DN_USize *size, DN_USize begin_index, DN_ISize count, DN_ArrayErase erase);
template <typename T>                           T *                   DN_CArray_MakeArray               (T *data, DN_USize *size, DN_USize max, DN_USize count, DN_ZMem z_mem);
template <typename T>                           T *                   DN_CArray_InsertArray             (T *data, DN_USize *size, DN_USize max, DN_USize index, T const *items, DN_USize count);
template <typename T>                           T                     DN_CArray_PopFront                (T *data, DN_USize *size, DN_USize count);
template <typename T>                           T                     DN_CArray_PopBack                 (T *data, DN_USize *size, DN_USize count);
template <typename T>                           DN_ArrayFindResult<T> DN_CArray_Find                    (T *data, DN_USize size, T const &value);

// NOTE: DN_SArray /////////////////////////////////////////////////////////////////////////////////
#if !defined(DN_NO_SARRAY)
template <typename T>                           DN_SArray<T>          DN_SArray_Init                    (DN_Arena *arena, DN_USize size, DN_ZMem z_mem);
template <typename T>                           DN_SArray<T>          DN_SArray_InitSlice               (DN_Arena *arena, DN_Slice<T> slice, DN_USize size, DN_ZMem z_mem);
template <typename T, size_t N>                 DN_SArray<T>          DN_SArray_InitCArray              (DN_Arena *arena, T const (&array)[N], DN_USize size, DN_ZMem);
template <typename T>                           DN_SArray<T>          DN_SArray_InitBuffer              (T* buffer, DN_USize size);
template <typename T>                           bool                  DN_SArray_IsValid                 (DN_SArray<T> const *array);
template <typename T>                           DN_Slice<T>           DN_SArray_Slice                   (DN_SArray<T> const *array);
template <typename T>                           T *                   DN_SArray_AddArray                (DN_SArray<T> *array, T const *items, DN_USize count);
template <typename T, DN_USize N>               T *                   DN_SArray_AddCArray               (DN_SArray<T> *array, T const (&items)[N]);
template <typename T>                           T *                   DN_SArray_Add                     (DN_SArray<T> *array, T const &item);
#define                                                               DN_SArray_AddArrayAssert(...)     DN_HardAssert(DN_SArray_AddArray(__VA_ARGS__))
#define                                                               DN_SArray_AddCArrayAssert(...)    DN_HardAssert(DN_SArray_AddCArray(__VA_ARGS__))
#define                                                               DN_SArray_AddAssert(...)          DN_HardAssert(DN_SArray_Add(__VA_ARGS__))
template <typename T>                           T *                   DN_SArray_MakeArray               (DN_SArray<T> *array, DN_USize count, DN_ZMem z_mem);
template <typename T>                           T *                   DN_SArray_Make                    (DN_SArray<T> *array, DN_ZMem z_mem);
#define                                                               DN_SArray_MakeArrayAssert(...)    DN_HardAssert(DN_SArray_MakeArray(__VA_ARGS__))
#define                                                               DN_SArray_MakeAssert(...)         DN_HardAssert(DN_SArray_Make(__VA_ARGS__))
template <typename T>                           T *                   DN_SArray_InsertArray             (DN_SArray<T> *array, DN_USize index, T const *items, DN_USize count);
template <typename T, DN_USize N>               T *                   DN_SArray_InsertCArray            (DN_SArray<T> *array, DN_USize index, T const (&items)[N]);
template <typename T>                           T *                   DN_SArray_Insert                  (DN_SArray<T> *array, DN_USize index, T const &item);
#define                                                               DN_SArray_InsertArrayAssert(...)  DN_HardAssert(DN_SArray_InsertArray(__VA_ARGS__))
#define                                                               DN_SArray_InsertCArrayAssert(...) DN_HardAssert(DN_SArray_InsertCArray(__VA_ARGS__))
#define                                                               DN_SArray_InsertAssert(...)       DN_HardAssert(DN_SArray_Insert(__VA_ARGS__))
template <typename T>                           T                     DN_SArray_PopFront                (DN_SArray<T> *array, DN_USize count);
template <typename T>                           T                     DN_SArray_PopBack                 (DN_SArray<T> *array, DN_USize count);
template <typename T>                           DN_ArrayEraseResult   DN_SArray_EraseRange              (DN_SArray<T> *array, DN_USize begin_index, DN_ISize count, DN_ArrayErase erase);
template <typename T>                           void                  DN_SArray_Clear                   (DN_SArray<T> *array);
#endif // !defined(DN_NO_SARRAY)

#if !defined(DN_NO_FARRAY)
#define                                                               DN_FArray_ToSArray(array)          DN_SArray_InitBuffer((array)->data, DN_ArrayCountU((array)->data))
template <typename T, DN_USize N>               DN_FArray<T, N>       DN_FArray_Init                     (T const *array, DN_USize count);
#define                                                               DN_FArray_HasData(array)           ((array).data && (array).size)
template <typename T, DN_USize N>               DN_FArray<T, N>       DN_FArray_InitSlice                (DN_Slice<T> slice);
template <typename T, DN_USize N, DN_USize K>   DN_FArray<T, N>       DN_FArray_InitCArray               (T const (&items)[K]);
template <typename T, DN_USize N>               bool                  DN_FArray_IsValid                  (DN_FArray<T, N> const *array);
template <typename T, DN_USize N>               DN_USize              DN_FArray_Max                      (DN_FArray<T, N> const *) { return N; }
template <typename T, DN_USize N>               DN_Slice<T>           DN_FArray_Slice                    (DN_FArray<T, N> const *array);
template <typename T, DN_USize N>               T *                   DN_FArray_AddArray                 (DN_FArray<T, N> *array, T const *items, DN_USize count);
template <typename T, DN_USize N, DN_USize K>   T *                   DN_FArray_AddCArray                (DN_FArray<T, N> *array, T const (&items)[K]);
template <typename T, DN_USize N>               T *                   DN_FArray_Add                      (DN_FArray<T, N> *array, T const &item);
#define                                                               DN_FArray_AddArrayAssert(...)      DN_HardAssert(DN_FArray_AddArray(__VA_ARGS__))
#define                                                               DN_FArray_AddCArrayAssert(...)     DN_HardAssert(DN_FArray_AddCArray(__VA_ARGS__))
#define                                                               DN_FArray_AddAssert(...)           DN_HardAssert(DN_FArray_Add(__VA_ARGS__))
template <typename T, DN_USize N>               T *                   DN_FArray_MakeArray                (DN_FArray<T, N> *array, DN_USize count, DN_ZMem z_mem);
template <typename T, DN_USize N>               T *                   DN_FArray_Make                     (DN_FArray<T, N> *array, DN_ZMem z_mem);
#define                                                               DN_FArray_MakeArrayAssert(...)     DN_HardAssert(DN_FArray_MakeArray(__VA_ARGS__))
#define                                                               DN_FArray_MakeAssert(...)          DN_HardAssert(DN_FArray_Make(__VA_ARGS__))
template <typename T, DN_USize N>               T *                   DN_FArray_InsertArray              (DN_FArray<T, N> *array, T const &item, DN_USize index);
template <typename T, DN_USize N, DN_USize K>   T *                   DN_FArray_InsertCArray             (DN_FArray<T, N> *array, DN_USize index, T const (&items)[K]);
template <typename T, DN_USize N>               T *                   DN_FArray_Insert                   (DN_FArray<T, N> *array, DN_USize index, T const &item);
#define                                                               DN_FArray_InsertArrayAssert(...)   DN_HardAssert(DN_FArray_InsertArray(__VA_ARGS__))
#define                                                               DN_FArray_InsertAssert(...)        DN_HardAssert(DN_FArray_Insert(__VA_ARGS__))
template <typename T, DN_USize N>               T                     DN_FArray_PopFront                 (DN_FArray<T, N> *array, DN_USize count);
template <typename T, DN_USize N>               T                     DN_FArray_PopBack                  (DN_FArray<T, N> *array, DN_USize count);
template <typename T, DN_USize N>               DN_ArrayFindResult<T> DN_FArray_Find                     (DN_FArray<T, N> *array, T const &find);
template <typename T, DN_USize N>               DN_ArrayEraseResult   DN_FArray_EraseRange               (DN_FArray<T, N> *array, DN_USize begin_index, DN_ISize count, DN_ArrayErase erase);
template <typename T, DN_USize N>               void                  DN_FArray_Clear                    (DN_FArray<T, N> *array);
#endif // !defined(DN_NO_FARRAY)

#if !defined(DN_NO_SLICE)
#define                                                               DN_TO_SLICE(val)                  DN_Slice_Init((val)->data, (val)->size)
#define                                                               DN_Slice_InitCArray(array)        DN_Slice_Init(array, DN_ArrayCountU(array))
template <typename T>                           DN_Slice<T>           DN_Slice_Init                     (T* const data, DN_USize size);
template <typename T, DN_USize N>               DN_Slice<T>           DN_Slice_InitCArrayCopy           (DN_Arena *arena, T const (&array)[N]);
template <typename T>                           DN_Slice<T>           DN_Slice_Copy                     (DN_Arena *arena, DN_Slice<T> slice);
template <typename T>                           DN_Slice<T>           DN_Slice_CopyPtr                  (DN_Arena *arena, T* const data, DN_USize size);
template <typename T>                           DN_Slice<T>           DN_Slice_Alloc                    (DN_Arena *arena, DN_USize size, DN_ZMem z_mem);
                                                DN_Str8               DN_Slice_Str8Render               (DN_Arena *arena, DN_Slice<DN_Str8> array, DN_Str8 separator);
                                                DN_Str8               DN_Slice_Str8RenderSpaceSeparated (DN_Arena *arena, DN_Slice<DN_Str8> array);
                                                DN_Str16              DN_Slice_Str16Render              (DN_Arena *arena, DN_Slice<DN_Str16> array, DN_Str16 separator);
                                                DN_Str16              DN_Slice_Str16RenderSpaceSeparated(DN_Arena *arena, DN_Slice<DN_Str16> array);
#endif // !defined(DN_NO_SLICE)

#if !defined(DN_NO_DSMAP)
template <typename T>                           DN_DSMap<T>           DN_DSMap_Init                     (DN_Arena *arena, DN_U32 size, DN_DSMapFlags flags);
template <typename T>                           void                  DN_DSMap_Deinit                   (DN_DSMap<T> *map, DN_ZMem z_mem);
template <typename T>                           bool                  DN_DSMap_IsValid                  (DN_DSMap<T> const *map);
template <typename T>                           DN_U32                DN_DSMap_Hash                     (DN_DSMap<T> const *map, DN_DSMapKey key);
template <typename T>                           DN_U32                DN_DSMap_HashToSlotIndex          (DN_DSMap<T> const *map, DN_DSMapKey key);
template <typename T>                           DN_DSMapResult<T>     DN_DSMap_Find                     (DN_DSMap<T> const *map, DN_DSMapKey key);
template <typename T>                           DN_DSMapResult<T>     DN_DSMap_Make                     (DN_DSMap<T> *map, DN_DSMapKey key);
template <typename T>                           DN_DSMapResult<T>     DN_DSMap_Set                      (DN_DSMap<T> *map, DN_DSMapKey key, T const &value);
template <typename T>                           DN_DSMapResult<T>     DN_DSMap_FindKeyU64               (DN_DSMap<T> const *map, DN_U64 key);
template <typename T>                           DN_DSMapResult<T>     DN_DSMap_MakeKeyU64               (DN_DSMap<T> *map,       DN_U64 key);
template <typename T>                           DN_DSMapResult<T>     DN_DSMap_SetKeyU64                (DN_DSMap<T> *map,       DN_U64 key, T const &value);
template <typename T>                           DN_DSMapResult<T>     DN_DSMap_FindKeyStr8              (DN_DSMap<T> const *map, DN_Str8 key);
template <typename T>                           DN_DSMapResult<T>     DN_DSMap_MakeKeyStr8              (DN_DSMap<T> *map,       DN_Str8 key);
template <typename T>                           DN_DSMapResult<T>     DN_DSMap_SetKeyStr8               (DN_DSMap<T> *map,       DN_Str8 key, T const &value);
template <typename T>                           bool                  DN_DSMap_Resize                   (DN_DSMap<T> *map, DN_U32 size);
template <typename T>                           bool                  DN_DSMap_Erase                    (DN_DSMap<T> *map, DN_DSMapKey key);
template <typename T>                           bool                  DN_DSMap_EraseKeyU64              (DN_DSMap<T> *map, DN_U64 key);
template <typename T>                           bool                  DN_DSMap_EraseKeyStr8             (DN_DSMap<T> *map, DN_Str8 key);
template <typename T>                           DN_DSMapKey           DN_DSMap_KeyBuffer                (DN_DSMap<T> const *map, void const *data, DN_U32 size);
template <typename T>                           DN_DSMapKey           DN_DSMap_KeyBufferAsU64NoHash     (DN_DSMap<T> const *map, void const *data, DN_U32 size);
template <typename T>                           DN_DSMapKey           DN_DSMap_KeyU64                   (DN_DSMap<T> const *map, DN_U64 u64);
template <typename T>                           DN_DSMapKey           DN_DSMap_KeyStr8                  (DN_DSMap<T> const *map, DN_Str8 string);
#define                                                               DN_DSMap_KeyCStr8(map, string)    DN_DSMap_KeyBuffer(map, string, sizeof((string))/sizeof((string)[0]) - 1)
DN_API                                          DN_DSMapKey           DN_DSMap_KeyU64NoHash             (DN_U64 u64);
DN_API                                          bool                  DN_DSMap_KeyEquals                (DN_DSMapKey lhs, DN_DSMapKey rhs);
DN_API                                          bool                  operator==                        (DN_DSMapKey lhs, DN_DSMapKey rhs);
#endif // !defined(DN_NO_DSMAP)

#if !defined(DN_NO_LIST)
template <typename T>                           DN_List<T>            DN_List_Init                      (DN_USize chunk_size);
template <typename T, size_t N>                 DN_List<T>            DN_List_InitCArray                (DN_Arena *arena, DN_USize chunk_size, T const (&array)[N]);
template <typename T>                           T *                   DN_List_At                        (DN_List<T> *list, DN_USize index, DN_ListChunk<T> **at_chunk);
template <typename T>                           void                  DN_List_Clear                     (DN_List<T> *list);
template <typename T>                           bool                  DN_List_Iterate                   (DN_List<T> *list, DN_ListIterator<T> *it, DN_USize start_index);
template <typename T>                           T *                   DN_List_MakeArena                 (DN_List<T> *list, DN_Arena *arena, DN_USize count);
template <typename T>                           T *                   DN_List_MakePool                  (DN_List<T> *list, DN_Pool *pool, DN_USize count);
template <typename T>                           T *                   DN_List_AddArena                  (DN_List<T> *list, DN_Arena *arena, T const &value);
template <typename T>                           T *                   DN_List_AddPool                   (DN_List<T> *list, DN_Pool *pool, T const &value);
template <typename T>                           void                  DN_List_AddListArena              (DN_List<T> *list, DN_Arena *arena, DN_List<T> other);
template <typename T>                           void                  DN_List_AddListArena              (DN_List<T> *list, DN_Pool *pool, DN_List<T> other);
template <typename T>                           DN_Slice<T>           DN_List_ToSliceCopy               (DN_List<T> const *list, DN_Arena* arena);
#endif // !defined(DN_NO_LIST)
#endif // !defined(DN_CONTAINER_H)
