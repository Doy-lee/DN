#if !defined(DN_CONTAINERS_H)
#define DN_CONTAINERS_H
// Containers that are imlpemented using primarily macros for operating on data structures that are
// embedded into a C style struct or from a set of defined variables from the available scope. Keep
// it stupid simple, structs and functions. Minimal amount of container types with flexible
// construction leads to less duplicated container code and less template meta-programming.
//
// PArray => Pointer (to) Array
// LArray => Literal Array
//   Define a C array and size. (P) array macros take a pointer to the aray, its size and its max
//   capacity. The (L) array macros take the literal array and derives the max capacity
//   automatically using DN_ArrayCountU(l_array).
//
//     MyStruct buffer[TB_ASType_Count] = {};
//     DN_USize size                    = 0;
//     MyStruct *item_0                 = DN_PArrayMake(buffer, &size, DN_ArrayCountU(buffer), DN_ZMem_No);
//     MyStruct *item_1                 = DN_LArrayMake(buffer, &size, DN_ZMem_No);
//
// IArray => Intrusive Array
//   Define a struct with the members `data`, `size` and `max`:
//
//     struct MyArray {
//       MyStruct *data;
//       DN_USize  size;
//       DN_USize  max;
//     } my_array = {};
//     MyStruct *item = DN_IArrayMake(&my_array, DN_ZMem_No);
//
// ISinglyLL => Intrusive Singly Linked List
//   Define a struct with the members `next`:
//
//     struct MyLinkItem {
//       int         data;
//       MyLinkItem *next;
//     } my_link = {};
//
//     MyLinkItem *first_item = DN_ISinglyLLDetach(&my_link, MyLinkItem);
//
// DoublyLL => Doubly Linked List
//   Define a struct with the members `next` and `prev`. This list has null pointers for head->prev
//   and tail->next.
//
//     struct MyLinkItem {
//       int         data;
//       MyLinkItem *next;
//       MyLinkItem *prev;
//     } my_link = {};
//
//     MyLinkItem first_item = {}, second_item = {};
//     DN_DoublyLLAppend(&first_item, &second_item); // first_item -> second_item
//
// SentinelDoublyLL => Sentinel Doubly Linked List
//   Uses a sentinel/dummy node as the list head. The sentinel points to itself when empty.
//   Define a struct with the members `next` and `prev`:
//
//     struct MyLinkItem {
//       int         data;
//       MyLinkItem *next;
//       MyLinkItem *prev;
//     } my_list = {};
//
//     DN_SentinelDoublyLLInit(&my_list);
//     DN_SentinelDoublyLLAppend(&my_list, &new_item);
//     DN_SentinelDoublyLLForEach(it, &my_list) { /* ... */ }

#if defined(_CLANGD)
  #include "../dn.h"
#endif

struct DN_Ring
{
  DN_U64 size;
  char  *base;
  DN_U64 write_pos;
  DN_U64 read_pos;
};

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

#define DN_ISinglyLLDetach(list) (decltype(list))DN_SinglyLLDetach((void **)&(list), (void **)&(list)->next)

#define DN_SentinelDoublyLLInit(list)             (list)->next = (list)->prev = (list)
#define DN_SentinelDoublyLLIsSentinel(list, item) ((list) == (item))
#define DN_SentinelDoublyLLIsEmpty(list)          (!(list) || ((list) == (list)->next))
#define DN_SentinelDoublyLLIsInit(list)           ((list)->next && (list)->prev)
#define DN_SentinelDoublyLLHasItems(list)         ((list) && ((list) != (list)->next))
#define DN_SentinelDoublyLLForEach(it, list)      auto *it = (list)->next; (it) != (list); (it) = (it)->next

#define DN_SentinelDoublyLLInitArena(list, T, arena) \
  do {                                               \
    (list) = DN_ArenaNew(arena, T, DN_ZMem_Yes);     \
    DN_SentinelDoublyLLInit(list);                   \
  } while (0)

#define DN_SentinelDoublyLLInitPool(list, T, pool) \
  do {                                             \
    (list) = DN_PoolNew(pool, T);                  \
    DN_SentinelDoublyLLInit(list);                 \
  } while (0)

#define DN_SentinelDoublyLLDetach(item)  \
  do {                                   \
    if (item) {                          \
      (item)->prev->next = (item)->next; \
      (item)->next->prev = (item)->prev; \
      (item)->next       = nullptr;      \
      (item)->prev       = nullptr;      \
    }                                    \
  } while (0)

#define DN_SentinelDoublyLLDequeue(list, dest_ptr) \
  if (DN_SentinelDoublyLLHasItems(list)) {         \
    dest_ptr = (list)->next;                       \
    DN_SentinelDoublyLLDetach(dest_ptr);           \
  }

#define DN_SentinelDoublyLLAppend(list, item) \
  do {                                        \
    if (item) {                               \
      if ((item)->next)                       \
        DN_SentinelDoublyLLDetach(item);      \
      (item)->next       = (list)->next;      \
      (item)->prev       = (list);            \
      (item)->next->prev = (item);            \
      (item)->prev->next = (item);            \
    }                                         \
  } while (0)

#define DN_SentinelDoublyLLPrepend(list, item) \
  do {                                         \
    if (item) {                                \
      if ((item)->next)                        \
        DN_SentinelDoublyLLDetach(item);       \
      (item)->next       = (list);             \
      (item)->prev       = (list)->prev;       \
      (item)->next->prev = (item);             \
      (item)->prev->next = (item);             \
    }                                          \
  } while (0)

// DoublyLL => Non-intrusive Doubly Linked List
//   A simple doubly linked list where each node has `next` and `prev` pointers.
//   The head is passed as a pointer-to-pointer to allow head updates.
//
//     struct MyLinkItem {
//       int         data;
//       MyLinkItem *next;
//       MyLinkItem *prev;
//     } *head = nullptr;
//     DN_DoublyLLAppend(&head, new_item);
//     for (MyLinkItem *it = head; it; it = it->next) { /* ... */ }

#define DN_DoublyLLDetach(head, ptr)     \
  do {                                   \
    if ((head) && (head) == (ptr))       \
      (head) = (head)->next;             \
    if ((ptr)) {                         \
      if ((ptr)->next)                   \
        (ptr)->next->prev = (ptr)->prev; \
      if ((ptr)->prev)                   \
        (ptr)->prev->next = (ptr)->next; \
      (ptr)->prev = (ptr)->next = 0;     \
    }                                    \
  } while (0)

#define DN_DoublyLLAppend(head, ptr)                                                \
  do {                                                                              \
    if ((ptr)) {                                                                    \
      DN_AssertF((ptr)->prev == 0 && (ptr)->next == 0, "Detach the pointer first"); \
      (ptr)->prev = (head);                                                         \
      (ptr)->next = 0;                                                              \
      if ((head)) {                                                                 \
        (ptr)->next  = (head)->next;                                                \
        (head)->next = (ptr);                                                       \
      } else {                                                                      \
        (head) = (ptr);                                                             \
      }                                                                             \
    }                                                                               \
  } while (0)

#define DN_DoublyLLPrepend(head, ptr)                                               \
  do {                                                                              \
    if ((ptr)) {                                                                    \
      DN_AssertF((ptr)->prev == 0 && (ptr)->next == 0, "Detach the pointer first"); \
      (ptr)->prev = nullptr;                                                        \
      (ptr)->next = (head);                                                         \
      if ((head)) {                                                                 \
        (ptr)->prev  = (head)->prev;                                                \
        (head)->prev = (ptr);                                                       \
      } else {                                                                      \
        (head) = (ptr);                                                             \
      }                                                                             \
    }                                                                               \
  } while (0)

// NOTE: For C++ we need to cast the void* returned in these functions to the concrete type. In C,
// no cast is needed.
#if defined(__cplusplus)
  #define DN_CppDeclType(x) decltype(x)
#else
  #define DN_CppDeclType
#endif

#define                     DN_PArrayResizeFromPool(ptr, size, max, pool, new_max)         DN_CArrayResizeFromPool((void **)&(ptr), size, max, sizeof((ptr)[0]), pool, new_max)
#define                     DN_PArrayGrowFromPool(ptr, size, max, pool, new_max)           DN_CArrayGrowFromPool((void **)&(ptr), size, max, sizeof((ptr)[0]), pool, new_max)
#define                     DN_PArrayGrowIfNeededFromPool(ptr, size, max, pool, add_count) DN_CArrayGrowIfNeededFromPool((void **)(ptr), size, max, sizeof((ptr)[0]), pool, add_count)
#define                     DN_PArrayMakeArray(ptr, size, max, count, z_mem)               (DN_CppDeclType(&(ptr)[0]))DN_CArrayMakeArray(ptr, size, max, sizeof((ptr)[0]), count, z_mem)
#define                     DN_PArrayMakeArrayZ(ptr, size, max, count)                     (DN_CppDeclType(&(ptr)[0]))DN_CArrayMakeArray(ptr, size, max, sizeof((ptr)[0]), count, DN_ZMem_Yes)
#define                     DN_PArrayMake(ptr, size, max, z_mem)                           (DN_CppDeclType(&(ptr)[0]))DN_CArrayMakeArray(ptr, size, max, sizeof((ptr)[0]), 1, z_mem)
#define                     DN_PArrayMakeZ(ptr, size, max)                                 (DN_CppDeclType(&(ptr)[0]))DN_CArrayMakeArray(ptr, size, max, sizeof((ptr)[0]), 1, DN_ZMem_Yes)
#define                     DN_PArrayAddArray(ptr, size, max, items, count, add)           (DN_CppDeclType(&(ptr)[0]))DN_CArrayAddArray(ptr, size, max, sizeof((ptr)[0]), items, count, add)
#define                     DN_PArrayAdd(ptr, size, max, item, add)                        (DN_CppDeclType(&(ptr)[0]))DN_CArrayAddArray(ptr, size, max, sizeof((ptr)[0]), &item, 1, add)
#define                     DN_PArrayAppendArray(ptr, size, max, items, count)             (DN_CppDeclType(&(ptr)[0]))DN_CArrayAddArray(ptr, size, max, sizeof((ptr)[0]), items, count, DN_ArrayAdd_Append)
#define                     DN_PArrayAppend(ptr, size, max, item)                          (DN_CppDeclType(&(ptr)[0]))DN_CArrayAddArray(ptr, size, max, sizeof((ptr)[0]), &item, 1, DN_ArrayAdd_Append)
#define                     DN_PArrayPrependArray(ptr, size, max, items, count)            (DN_CppDeclType(&(ptr)[0]))DN_CArrayAddArray(ptr, size, max, sizeof((ptr)[0]), items, count, DN_ArrayAdd_Prepend)
#define                     DN_PArrayPrepend(ptr, size, max, item)                         (DN_CppDeclType(&(ptr)[0]))DN_CArrayAddArray(ptr, size, max, sizeof((ptr)[0]), &item, 1, DN_ArrayAdd_Prepend)
#define                     DN_PArrayEraseRange(ptr, size, begin_index, count, erase)      DN_CArrayEraseRange(ptr, size, sizeof((ptr)[0]), begin_index, count, erase)
#define                     DN_PArrayErase(ptr, size, index, erase)                        DN_CArrayEraseRange(ptr, size, sizeof((ptr)[0]), index, 1, erase)
#define                     DN_PArrayInsertArray(ptr, size, max, index, items, count)      (DN_CppDeclType(&(ptr)[0]))DN_CArrayInsertArray(ptr, size, max, sizeof((ptr)[0]), index, items, count)
#define                     DN_PArrayInsert(ptr, size, max, index, item)                   (DN_CppDeclType(&(ptr)[0]))DN_CArrayInsertArray(ptr, size, max, sizeof((ptr)[0]), index, &item, 1)
#define                     DN_PArrayPopFront(ptr, size, max, count)                       (DN_CppDeclType(&(ptr)[0]))DN_CArrayPopFront(ptr, size, sizeof((ptr)[0]), count)
#define                     DN_PArrayPopBack(ptr, size, max, count)                        (DN_CppDeclType(&(ptr)[0]))DN_CArrayPopBack(ptr, size, sizeof((ptr)[0]), count)

#define                     DN_LArrayResizeFromPool(c_array, size, pool, new_max)          DN_PArrayResizeFromPool(c_array, size, DN_ArrayCountU(c_array), pool, new_max)
#define                     DN_LArrayGrowFromPool(c_array, size, pool, new_max)            DN_PArrayGrowFromPool(c_array, size, DN_ArrayCountU(c_array), pool, new_max)
#define                     DN_LArrayGrowIfNeededFromPool(c_array, size, pool, add_count)  DN_PArrayGrowIfNeededFromPool(c_array, size, DN_ArrayCountU(c_array), pool, add_count)
#define                     DN_LArrayMakeArray(c_array, size, count, z_mem)                DN_PArrayMakeArray(c_array, size, DN_ArrayCountU(c_array), count, z_mem)
#define                     DN_LArrayMakeArrayZ(c_array, size, count)                      DN_PArrayMakeArrayZ(c_array, size, DN_ArrayCountU(c_array), count)
#define                     DN_LArrayMake(c_array, size, z_mem)                            DN_PArrayMake(c_array, size, DN_ArrayCountU(c_array), z_mem)
#define                     DN_LArrayMakeZ(c_array, size, max)                             DN_PArrayMakeZ(c_array, size, DN_ArrayCountU(c_array), max)
#define                     DN_LArrayAddArray(c_array, size, items, count, add)            DN_PArrayAddArray(c_array, size, DN_ArrayCountU(c_array), items, count, add)
#define                     DN_LArrayAdd(c_array, size, item, add)                         DN_PArrayAdd(c_array, size, DN_ArrayCountU(c_array), item, add)
#define                     DN_LArrayAppendArray(c_array, size, items, count)              DN_PArrayAppendArray(c_array, size, DN_ArrayCountU(c_array), items, count)
#define                     DN_LArrayAppend(c_array, size, item)                           DN_PArrayAppend(c_array, size, DN_ArrayCountU(c_array), item)
#define                     DN_LArrayPrependArray(c_array, size, items, count)             DN_PArrayPrependArray(c_array, size, DN_ArrayCountU(c_array), items, count)
#define                     DN_LArrayPrepend(c_array, size, item)                          DN_PArrayPrepend(c_array, size, DN_ArrayCountU(c_array), item)
#define                     DN_LArrayEraseRange(c_array, size, begin_index, count, erase)  DN_PArrayEraseRange(c_array, size, DN_ArrayCountU(c_array), begin_index, count, erase)
#define                     DN_LArrayErase(c_array, size, index, erase)                    DN_PArrayErase(c_array, size, DN_ArrayCountU(c_array), index, erase)
#define                     DN_LArrayInsertArray(c_array, size, index, items, count)       DN_PArrayInsertArray(c_array, size, DN_ArrayCountU(c_array), index, items, count)
#define                     DN_LArrayInsert(c_array, size, index, item)                    DN_PArrayInsert(c_array, size, DN_ArrayCountU(c_array), index, item)
#define                     DN_LArrayPopFront(c_array, size, count)                        DN_PArrayPopFront(c_array, size, DN_ArrayCountU(c_array), count)
#define                     DN_LArrayPopBack(c_array, size, count)                         DN_PArrayPopBack(c_array, size, DN_ArrayCountU(c_array), count)

#define                     DN_IArrayResizeFromPool(array, pool, new_max)                  DN_CArrayResizeFromPool((void **)(&(array)->data), &(array)->size, &(array)->max, sizeof((array)->data[0]), pool, new_max)
#define                     DN_IArrayGrowFromPool(array, pool, new_max)                    DN_CArrayGrowFromPool((void **)(&(array)->data), (array)->size, &(array)->max, sizeof((array)->data[0]), pool, new_max)
#define                     DN_IArrayGrowIfNeededFromPool(array, pool, add_count)          DN_CArrayGrowIfNeededFromPool((void **)(&(array)->data), (array)->size, &(array)->max, sizeof((array)->data[0]), pool, add_count)
#define                     DN_IArrayMakeArray(array, count, z_mem)                        (DN_CppDeclType(&((array)->data)[0]))DN_CArrayMakeArray((array)->data, &(array)->size, (array)->max, sizeof(((array)->data)[0]), count, z_mem)
#define                     DN_IArrayMakeArrayZ(array, count)                              (DN_CppDeclType(&((array)->data)[0]))DN_CArrayMakeArray((array)->data, &(array)->size, (array)->max, sizeof(((array)->data)[0]), count, DN_ZMem_Yes)
#define                     DN_IArrayMake(array, z_mem)                                    (DN_CppDeclType(&((array)->data)[0]))DN_CArrayMakeArray((array)->data, &(array)->size, (array)->max, sizeof(((array)->data)[0]), 1, z_mem)
#define                     DN_IArrayMakeZ(array)                                          (DN_CppDeclType(&((array)->data)[0]))DN_CArrayMakeArray((array)->data, &(array)->size, (array)->max, sizeof(((array)->data)[0]), 1, DN_ZMem_Yes)
#define                     DN_IArrayAddArray(array, items, count, add)                    (DN_CppDeclType(&((array)->data)[0]))DN_CArrayAddArray((array)->data, &(array)->size, (array)->max, sizeof(((array)->data)[0]), items, count, add)
#define                     DN_IArrayAdd(array, item, add)                                 (DN_CppDeclType(&((array)->data)[0]))DN_CArrayAddArray((array)->data, &(array)->size, (array)->max, sizeof(((array)->data)[0]), &item, 1, add)
#define                     DN_IArrayAppendArray(array, items, count)                      (DN_CppDeclType(&((array)->data)[0]))DN_CArrayAddArray((array)->data, &(array)->size, (array)->max, sizeof(((array)->data)[0]), items, count, DN_ArrayAdd_Append)
#define                     DN_IArrayAppend(array, item)                                   (DN_CppDeclType(&((array)->data)[0]))DN_CArrayAddArray((array)->data, &(array)->size, (array)->max, sizeof(((array)->data)[0]), &item, 1, DN_ArrayAdd_Append)
#define                     DN_IArrayPrependArray(array, items, count)                     (DN_CppDeclType(&((array)->data)[0]))DN_CArrayAddArray((array)->data, &(array)->size, (array)->max, sizeof(((array)->data)[0]), items, count, DN_ArrayAdd_Prepend)
#define                     DN_IArrayPrepend(array, item)                                  (DN_CppDeclType(&((array)->data)[0]))DN_CArrayAddArray((array)->data, &(array)->size, (array)->max, sizeof(((array)->data)[0]), &item, 1, DN_ArrayAdd_Prepend)
#define                     DN_IArrayEraseRange(array, begin_index, count, erase)          DN_CArrayEraseRange((array)->data, &(array)->size, sizeof(((array)->data)[0]), begin_index, count, erase)
#define                     DN_IArrayErase(array, index, erase)                            DN_CArrayEraseRange((array)->data, &(array)->size, sizeof(((array)->data)[0]), index, 1, erase)
#define                     DN_IArrayInsertArray(array, index, items, count)               (DN_CppDeclType(&((array)->data)[0]))DN_CArrayInsertArray((array)->data, &(array)->size, (array)->max, sizeof(((array)->data)[0]), index, items, count)
#define                     DN_IArrayInsert(array, index, item, count)                     (DN_CppDeclType(&((array)->data)[0]))DN_CArrayInsertArray((array)->data, &(array)->size, (array)->max, sizeof(((array)->data)[0]), index, &item, 1)
#define                     DN_IArrayPopFront(array, count)                                (DN_CppDeclType(&((array)->data)[0]))DN_CArrayPopFront((array)->data, &(array)->size, sizeof(((array)->data)[0]), count)
#define                     DN_IArrayPopBack(array, count)                                 (DN_CppDeclType(&((array)->data)[0]))DN_CArrayPopBack((array)->data, &(array)->size, sizeof(((array)->data)[0]), count)

#define                     DN_ISliceAllocArena(T, slice_ptr, count_, zmem, arena)         (T *)DN_SliceAllocArena((void **)&((slice_ptr)->data), &((slice_ptr)->count), count_, sizeof(T), alignof(T), zmem, arena)

DN_API void*                DN_SliceAllocArena                                             (void **data, DN_USize *slice_size_field, DN_USize size, DN_USize elem_size, DN_U8 align, DN_ZMem zmem, DN_Arena *arena);

DN_API void*                DN_CArrayInsertArray                                           (void *data, DN_USize *size, DN_USize max, DN_USize elem_size, DN_USize index, void const *items, DN_USize count);
DN_API void*                DN_CArrayPopFront                                              (void *data, DN_USize *size, DN_USize elem_size, DN_USize count);
DN_API void*                DN_CArrayPopBack                                               (void *data, DN_USize *size, DN_USize elem_size, DN_USize count);
DN_API DN_ArrayEraseResult  DN_CArrayEraseRange                                            (void *data, DN_USize *size, DN_USize elem_size, DN_USize begin_index, DN_ISize count, DN_ArrayErase erase);
DN_API void*                DN_CArrayMakeArray                                             (void *data, DN_USize *size, DN_USize max, DN_USize data_size, DN_USize make_size, DN_ZMem z_mem);
DN_API void*                DN_CArrayAddArray                                              (void *data, DN_USize *size, DN_USize max, DN_USize data_size, void const *elems, DN_USize elems_count, DN_ArrayAdd add);
DN_API bool                 DN_CArrayResizeFromPool                                        (void **data, DN_USize *size, DN_USize *max, DN_USize data_size, DN_Pool *pool, DN_USize new_max);
DN_API bool                 DN_CArrayGrowFromPool                                          (void **data, DN_USize size, DN_USize *max, DN_USize data_size, DN_Pool *pool, DN_USize new_max);
DN_API bool                 DN_CArrayGrowIfNeededFromPool                                  (void **data, DN_USize size, DN_USize *max, DN_USize data_size, DN_Pool *pool);

DN_API void*                DN_SinglyLLDetach                                              (void **link, void **next);

DN_API bool                 DN_RingHasSpace                                                (DN_Ring const *ring, DN_U64 size);
DN_API bool                 DN_RingHasData                                                 (DN_Ring const *ring, DN_U64 size);
DN_API void                 DN_RingWrite                                                   (DN_Ring *ring, void const *src, DN_U64 src_size);
#define                     DN_RingWriteStruct(ring, item)                                 DN_RingWrite((ring), (item), sizeof(*(item)))
DN_API void                 DN_RingRead                                                    (DN_Ring *ring, void *dest, DN_U64 dest_size);
#define                     DN_RingReadStruct(ring, dest)                                  DN_RingRead((ring), (dest), sizeof(*(dest)))

template <typename T> DN_DSMap<T>        DN_DSMapInit                  (DN_Arena *arena, DN_U32 size, DN_DSMapFlags flags);
template <typename T> void               DN_DSMapDeinit                (DN_DSMap<T> *map, DN_ZMem z_mem);
template <typename T> bool               DN_DSMapIsValid               (DN_DSMap<T> const *map);
template <typename T> DN_U32             DN_DSMapHash                  (DN_DSMap<T> const *map, DN_DSMapKey key);
template <typename T> DN_U32             DN_DSMapHashToSlotIndex       (DN_DSMap<T> const *map, DN_DSMapKey key);
template <typename T> DN_DSMapResult<T>  DN_DSMapFind                  (DN_DSMap<T> const *map, DN_DSMapKey key);
template <typename T> DN_DSMapResult<T>  DN_DSMapMake                  (DN_DSMap<T> *map, DN_DSMapKey key);
template <typename T> DN_DSMapResult<T>  DN_DSMapSet                   (DN_DSMap<T> *map, DN_DSMapKey key, T const &value);
template <typename T> DN_DSMapResult<T>  DN_DSMapFindKeyU64            (DN_DSMap<T> const *map, DN_U64 key);
template <typename T> DN_DSMapResult<T>  DN_DSMapMakeKeyU64            (DN_DSMap<T> *map,       DN_U64 key);
template <typename T> DN_DSMapResult<T>  DN_DSMapSetKeyU64             (DN_DSMap<T> *map,       DN_U64 key, T const &value);
template <typename T> DN_DSMapResult<T>  DN_DSMapFindKeyStr8           (DN_DSMap<T> const *map, DN_Str8 key);
template <typename T> DN_DSMapResult<T>  DN_DSMapMakeKeyStr8           (DN_DSMap<T> *map,       DN_Str8 key);
template <typename T> DN_DSMapResult<T>  DN_DSMapSetKeyStr8            (DN_DSMap<T> *map,       DN_Str8 key, T const &value);
template <typename T> bool               DN_DSMapResize                (DN_DSMap<T> *map, DN_U32 size);
template <typename T> bool               DN_DSMapErase                 (DN_DSMap<T> *map, DN_DSMapKey key);
template <typename T> bool               DN_DSMapEraseKeyU64           (DN_DSMap<T> *map, DN_U64 key);
template <typename T> bool               DN_DSMapEraseKeyStr8          (DN_DSMap<T> *map, DN_Str8 key);
template <typename T> DN_DSMapKey        DN_DSMapKeyBuffer             (DN_DSMap<T> const *map, void const *data, DN_U32 size);
template <typename T> DN_DSMapKey        DN_DSMapKeyBufferAsU64NoHash  (DN_DSMap<T> const *map, void const *data, DN_U32 size);
template <typename T> DN_DSMapKey        DN_DSMapKeyU64                (DN_DSMap<T> const *map, DN_U64 u64);
template <typename T> DN_DSMapKey        DN_DSMapKeyStr8               (DN_DSMap<T> const *map, DN_Str8 string);
#define                                  DN_DSMapKeyCStr8(map, string) DN_DSMapKeyBuffer(map, string, sizeof((string))/sizeof((string)[0]) - 1)
DN_API                DN_DSMapKey        DN_DSMapKeyU64NoHash          (DN_U64 u64);
DN_API                bool               DN_DSMapKeyEquals             (DN_DSMapKey lhs, DN_DSMapKey rhs);
DN_API                bool               operator==                    (DN_DSMapKey lhs, DN_DSMapKey rhs);
#endif // !defined(DN_CONTAINER_H)
