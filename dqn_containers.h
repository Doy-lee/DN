#pragma once
#include "dqn.h"

/*
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//    $$$$$$\   $$$$$$\  $$\   $$\ $$$$$$$$\  $$$$$$\  $$$$$$\ $$\   $$\ $$$$$$$$\ $$$$$$$\   $$$$$$\
//   $$  __$$\ $$  __$$\ $$$\  $$ |\__$$  __|$$  __$$\ \_$$  _|$$$\  $$ |$$  _____|$$  __$$\ $$  __$$\
//   $$ /  \__|$$ /  $$ |$$$$\ $$ |   $$ |   $$ /  $$ |  $$ |  $$$$\ $$ |$$ |      $$ |  $$ |$$ /  \__|
//   $$ |      $$ |  $$ |$$ $$\$$ |   $$ |   $$$$$$$$ |  $$ |  $$ $$\$$ |$$$$$\    $$$$$$$  |\$$$$$$\
//   $$ |      $$ |  $$ |$$ \$$$$ |   $$ |   $$  __$$ |  $$ |  $$ \$$$$ |$$  __|   $$  __$$<  \____$$\
//   $$ |  $$\ $$ |  $$ |$$ |\$$$ |   $$ |   $$ |  $$ |  $$ |  $$ |\$$$ |$$ |      $$ |  $$ |$$\   $$ |
//   \$$$$$$  | $$$$$$  |$$ | \$$ |   $$ |   $$ |  $$ |$$$$$$\ $$ | \$$ |$$$$$$$$\ $$ |  $$ |\$$$$$$  |
//    \______/  \______/ \__|  \__|   \__|   \__|  \__|\______|\__|  \__|\________|\__|  \__| \______/
//
//   dqn_containers.h -- Data structures for storing data (e.g. arrays and hash tables)
//
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// [$CARR] Dqn_CArray --            -- Basic operations on C arrays for VArray/SArray/FArray to reuse
// [$VARR] Dqn_VArray -- DQN_VARRAY -- Array backed by virtual memory arena
// [$SARR] Dqn_SArray -- DQN_SARRAY -- Array that are allocated but cannot resize
// [$FARR] Dqn_FArray -- DQN_FARRAY -- Fixed-size arrays
// [$SLIC] Dqn_Slice  --            -- Pointe and length container of data
// [$DMAP] Dqn_DSMap  -- DQN_DSMAP  -- Hashtable, 70% max load, PoT size, linear probe, chain repair
// [$LIST] Dqn_List   -- DQN_LIST   -- Chunked linked lists, append only
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

// NOTE: [$CARR] Dqn_CArray ////////////////////////////////////////////////////////////////////////
enum Dqn_ArrayErase
{
    Dqn_ArrayErase_Unstable,
    Dqn_ArrayErase_Stable,
};

struct Dqn_ArrayEraseResult
{
    // The next index your for-index should be set to such that you can continue
    // to iterate the remainder of the array, e.g:
    //
    // for (Dqn_usize index = 0; index < array.size; index++) {
    //     if (erase)
    //          index = Dqn_FArray_EraseRange(&array, index, -3, Dqn_ArrayErase_Unstable);
    // }
    Dqn_usize it_index;
    Dqn_usize items_erased; // The number of items erased
};

template <typename T> struct Dqn_ArrayFindResult
{
    T        *data;  // Pointer to the value if a match is found, null pointer otherwise
    Dqn_usize index; // Index to the value if a match is found, 0 otherwise
};

#if !defined(DQN_NO_VARRAY)
// NOTE: [$VARR] Dqn_VArray ////////////////////////////////////////////////////////////////////////
// TODO(doyle): Add an API for shrinking the array by decomitting pages back to 
// the OS.
template <typename T> struct Dqn_VArray
{
    Dqn_Arena  arena; // Allocator for the array
    T         *data;  // Pointer to the start of the array items in the block of memory
    Dqn_usize  size;  // Number of items currently in the array
    Dqn_usize  max;   // Maximum number of items this array can store

    T       *begin()       { return data; }
    T       *end  ()       { return data + size; }
    T const *begin() const { return data; }
    T const *end  () const { return data + size; }
};
#endif // !defined(DQN_NO_VARRAY)

#if !defined(DQN_NO_SARRAY)
// NOTE: [$SARR] Dqn_SArray ////////////////////////////////////////////////////////////////////////
template <typename T> struct Dqn_SArray
{
    T        *data; // Pointer to the start of the array items in the block of memory
    Dqn_usize size; // Number of items currently in the array
    Dqn_usize max;  // Maximum number of items this array can store

    T       *begin()       { return data; }
    T       *end  ()       { return data + size; }
    T const *begin() const { return data; }
    T const *end  () const { return data + size; }
};
#endif // !defined(DQN_NO_SARRAY)

#if !defined(DQN_NO_FARRAY)
// NOTE: [$FARR] Dqn_FArray ////////////////////////////////////////////////////////////////////////
template <typename T, Dqn_usize N> struct Dqn_FArray
{
    T         data[N]; // Pointer to the start of the array items in the block of memory
    Dqn_usize size;    // Number of items currently in the array

    T       *begin()       { return data; }
    T       *end  ()       { return data + size; }
    T const *begin() const { return data; }
    T const *end  () const { return data + size; }
};

template <typename T> using Dqn_FArray8  = Dqn_FArray<T, 8>;
template <typename T> using Dqn_FArray16 = Dqn_FArray<T, 16>;
template <typename T> using Dqn_FArray32 = Dqn_FArray<T, 32>;
template <typename T> using Dqn_FArray64 = Dqn_FArray<T, 64>;
#endif // !defined(DQN_NO_FARRAY)

#if !defined(DQN_NO_DSMAP)
// NOTE: [$DMAP] Dqn_DSMap /////////////////////////////////////////////////////////////////////////
enum Dqn_DSMapKeyType
{
                                        // Key    | Key Hash         | Map Index
    Dqn_DSMapKeyType_Invalid,
    Dqn_DSMapKeyType_U64,               // U64    | Hash(U64)        | Hash(U64)        % map_size
    Dqn_DSMapKeyType_U64NoHash,         // U64    | U64              | U64              % map_size
    Dqn_DSMapKeyType_Buffer,            // Buffer | Hash(buffer)     | Hash(buffer)     % map_size
    Dqn_DSMapKeyType_BufferAsU64NoHash, // Buffer | U64(buffer[0:4]) | U64(buffer[0:4]) % map_size
};

struct Dqn_DSMapKey
{
    Dqn_DSMapKeyType type;
    uint32_t         hash;           // Hash to lookup in the map. If it equals, we check that the original key payload matches
    void const      *buffer_data;
    uint32_t         buffer_size;
    uint64_t         u64;
    bool             no_copy_buffer;
};

template <typename T>
struct Dqn_DSMapSlot
{
    Dqn_DSMapKey key;   ///< Hash table lookup key
    T            value; ///< Hash table value
};

typedef uint32_t Dqn_DSMapFlags;
enum Dqn_DSMapFlags_
{
    Dqn_DSMapFlags_Nil                   = 0,
    Dqn_DSMapFlags_DontFreeArenaOnResize = 1 << 0,
};

using Dqn_DSMapHashFunction = uint32_t(Dqn_DSMapKey key, uint32_t seed);
template <typename T> struct Dqn_DSMap
{
    uint32_t              *hash_to_slot;  // Mapping from hash to a index in the slots array
    Dqn_DSMapSlot<T>      *slots;         // Values of the array stored contiguously, non-sorted order
    uint32_t               size;          // Total capacity of the map and is a power of two
    uint32_t               occupied;      // Number of slots used in the hash table
    Dqn_Arena             *arena;         // Backing arena for the hash table
    uint32_t               initial_size;  // Initial map size, map cannot shrink on erase below this size
    Dqn_DSMapHashFunction *hash_function; // Custom hashing function to use if field is set
    uint32_t               hash_seed;     // Seed for the hashing function, when 0, DQN_DS_MAP_DEFAULT_HASH_SEED is used
    Dqn_DSMapFlags         flags;
};

template <typename T> struct Dqn_DSMapResult
{
    bool              found;
    Dqn_DSMapSlot<T> *slot;
    T                *value;
};
#endif // !defined(DQN_NO_DSMAP)

#if !defined(DQN_NO_LIST)
// NOTE: [$LIST] Dqn_List //////////////////////////////////////////////////////////////////////////
template <typename T> struct Dqn_ListChunk
{
    T                *data;
    Dqn_usize         size;
    Dqn_usize         count;
    Dqn_ListChunk<T> *next;
    Dqn_ListChunk<T> *prev;
};

template <typename T> struct Dqn_ListIterator
{
    Dqn_b32           init;             // True if Dqn_List_Iterate has been called at-least once on this iterator
    Dqn_ListChunk<T> *chunk;            // The chunk that the iterator is reading from
    Dqn_usize         chunk_data_index; // The index in the chunk the iterator is referencing
    Dqn_usize         index;            // The index of the item in the list as if it was flat array
    T                *data;             // Pointer to the data the iterator is referencing. Nullptr if invalid.
};

template <typename T> struct Dqn_List
{
    Dqn_usize         count;      // Cumulative count of all items made across all list chunks
    Dqn_usize         chunk_size; // When new ListChunk's are required, the minimum 'data' entries to allocate for that node.
    Dqn_ListChunk<T> *head;
    Dqn_ListChunk<T> *tail;
};
#endif // !defined(DQN_NO_LIST)

template <typename T>                           Dqn_ArrayEraseResult   Dqn_CArray_EraseRange              (T *data, Dqn_usize *size, Dqn_usize begin_index, Dqn_isize count, Dqn_ArrayErase erase);
template <typename T>                           T *                    Dqn_CArray_MakeArray               (T *data, Dqn_usize *size, Dqn_usize max, Dqn_usize count, Dqn_ZeroMem zero_mem);
template <typename T>                           T *                    Dqn_CArray_InsertArray             (T *data, Dqn_usize *size, Dqn_usize max, Dqn_usize index, T const *items, Dqn_usize count);
template <typename T>                           T                      Dqn_CArray_PopFront                (T *data, Dqn_usize *size, Dqn_usize count);
template <typename T>                           T                      Dqn_CArray_PopBack                 (T *data, Dqn_usize *size, Dqn_usize count);
template <typename T>                           Dqn_ArrayFindResult<T> Dqn_CArray_Find                    (T *data, Dqn_usize size, T const &value);

#if !defined(DQN_NO_VARRAY)
template <typename T>                           Dqn_VArray<T>          Dqn_VArray_InitByteSize            (Dqn_usize byte_size, uint8_t arena_flags);
template <typename T>                           Dqn_VArray<T>          Dqn_VArray_Init                    (Dqn_usize max, uint8_t arena_flags);
template <typename T>                           Dqn_VArray<T>          Dqn_VArray_InitSlice               (Dqn_Slice<T> slice, Dqn_usize max, uint8_t arena_flags);
template <typename T, Dqn_usize N>              Dqn_VArray<T>          Dqn_VArray_InitCArray              (T const (&items)[N], Dqn_usize max, uint8_t arena_flags);
template <typename T>                           void                   Dqn_VArray_Deinit                  (Dqn_VArray<T> *array);
template <typename T>                           bool                   Dqn_VArray_IsValid                 (Dqn_VArray<T> const *array);
template <typename T>                           Dqn_Slice<T>           Dqn_VArray_Slice                   (Dqn_VArray<T> const *array);
template <typename T>                           bool                   Dqn_VArray_Reserve                 (Dqn_VArray<T> *array, Dqn_usize count);
template <typename T>                           T *                    Dqn_VArray_AddArray                (Dqn_VArray<T> *array, T const *items, Dqn_usize count);
template <typename T, Dqn_usize N>              T *                    Dqn_VArray_AddCArray               (Dqn_VArray<T> *array, T const (&items)[N]);
template <typename T>                           T *                    Dqn_VArray_Add                     (Dqn_VArray<T> *array, T const &item);
#define                                                                Dqn_VArray_AddArrayAssert(...)     DQN_HARD_ASSERT(Dqn_VArray_AddArray(__VA_ARGS__))
#define                                                                Dqn_VArray_AddCArrayAssert(...)    DQN_HARD_ASSERT(Dqn_VArray_AddCArray(__VA_ARGS__))
#define                                                                Dqn_VArray_AddAssert(...)          DQN_HARD_ASSERT(Dqn_VArray_Add(__VA_ARGS__))
template <typename T>                           T *                    Dqn_VArray_MakeArray               (Dqn_VArray<T> *array, Dqn_usize count, Dqn_ZeroMem zero_mem);
template <typename T>                           T *                    Dqn_VArray_Make                    (Dqn_VArray<T> *array, Dqn_ZeroMem zero_mem);
#define                                                                Dqn_VArray_MakeArrayAssert(...)    DQN_HARD_ASSERT(Dqn_VArray_MakeArray(__VA_ARGS__))
#define                                                                Dqn_VArray_MakeAssert(...)         DQN_HARD_ASSERT(Dqn_VArray_Make(__VA_ARGS__))
template <typename T>                           T *                    Dqn_VArray_InsertArray             (Dqn_VArray<T> *array, Dqn_usize index, T const *items, Dqn_usize count);
template <typename T, Dqn_usize N>              T *                    Dqn_VArray_InsertCArray            (Dqn_VArray<T> *array, Dqn_usize index, T const (&items)[N]);
template <typename T>                           T *                    Dqn_VArray_Insert                  (Dqn_VArray<T> *array, Dqn_usize index, T const &item);
#define                                                                Dqn_VArray_InsertArrayAssert(...)  DQN_HARD_ASSERT(Dqn_VArray_InsertArray(__VA_ARGS__))
#define                                                                Dqn_VArray_InsertCArrayAssert(...) DQN_HARD_ASSERT(Dqn_VArray_InsertCArray(__VA_ARGS__))
#define                                                                Dqn_VArray_InsertAssert(...)       DQN_HARD_ASSERT(Dqn_VArray_Insert(__VA_ARGS__))
template <typename T>                           T                      Dqn_VArray_PopFront                (Dqn_VArray<T> *array, Dqn_usize count);
template <typename T>                           T                      Dqn_VArray_PopBack                 (Dqn_VArray<T> *array, Dqn_usize count);
template <typename T>                           Dqn_ArrayEraseResult   Dqn_VArray_EraseRange              (Dqn_VArray<T> *array, Dqn_usize begin_index, Dqn_isize count, Dqn_ArrayErase erase);
template <typename T>                           void                   Dqn_VArray_Clear                   (Dqn_VArray<T> *array, Dqn_ZeroMem zero_mem);
#endif // !defined(DQN_NO_VARRAY)
// NOTE: [$SARR] Dqn_SArray ////////////////////////////////////////////////////////////////////////
#if !defined(DQN_NO_SARRAY)
template <typename T>                           Dqn_SArray<T>          Dqn_SArray_Init                    (Dqn_Arena *arena, Dqn_usize size, Dqn_ZeroMem zero_mem);
template <typename T>                           Dqn_SArray<T>          Dqn_SArray_InitSlice               (Dqn_Arena *arena, Dqn_Slice<T> slice, Dqn_usize size, Dqn_ZeroMem zero_mem);
template <typename T, size_t N>                 Dqn_SArray<T>          Dqn_SArray_InitCArray              (Dqn_Arena *arena, T const (&array)[N], Dqn_usize size, Dqn_ZeroMem);
template <typename T>                           bool                   Dqn_SArray_IsValid                 (Dqn_SArray<T> const *array);
template <typename T>                           Dqn_Slice<T>           Dqn_SArray_Slice                   (Dqn_SArray<T> const *array);
template <typename T>                           T *                    Dqn_SArray_AddArray                (Dqn_SArray<T> *array, T const *items, Dqn_usize count);
template <typename T, Dqn_usize N>              T *                    Dqn_SArray_AddCArray               (Dqn_SArray<T> *array, T const (&items)[N]);
template <typename T>                           T *                    Dqn_SArray_Add                     (Dqn_SArray<T> *array, T const &item);
#define                                                                Dqn_SArray_AddArrayAssert(...)     DQN_HARD_ASSERT(Dqn_SArray_AddArray(__VA_ARGS__))
#define                                                                Dqn_SArray_AddCArrayAssert(...)    DQN_HARD_ASSERT(Dqn_SArray_AddCArray(__VA_ARGS__))
#define                                                                Dqn_SArray_AddAssert(...)          DQN_HARD_ASSERT(Dqn_SArray_Add(__VA_ARGS__))
template <typename T>                           T *                    Dqn_SArray_MakeArray               (Dqn_SArray<T> *array, Dqn_usize count, Dqn_ZeroMem zero_mem);
template <typename T>                           T *                    Dqn_SArray_Make                    (Dqn_SArray<T> *array, Dqn_ZeroMem zero_mem);
#define                                                                Dqn_SArray_MakeArrayAssert(...)    DQN_HARD_ASSERT(Dqn_SArray_MakeArray(__VA_ARGS__))
#define                                                                Dqn_SArray_MakeAssert(...)         DQN_HARD_ASSERT(Dqn_SArray_Make(__VA_ARGS__))
template <typename T>                           T *                    Dqn_SArray_InsertArray             (Dqn_SArray<T> *array, Dqn_usize index, T const *items, Dqn_usize count);
template <typename T, Dqn_usize N>              T *                    Dqn_SArray_InsertCArray            (Dqn_SArray<T> *array, Dqn_usize index, T const (&items)[N]);
template <typename T>                           T *                    Dqn_SArray_Insert                  (Dqn_SArray<T> *array, Dqn_usize index, T const &item);
#define                                                                Dqn_SArray_InsertArrayAssert(...)  DQN_HARD_ASSERT(Dqn_SArray_InsertArray(__VA_ARGS__))
#define                                                                Dqn_SArray_InsertCArrayAssert(...) DQN_HARD_ASSERT(Dqn_SArray_InsertCArray(__VA_ARGS__))
#define                                                                Dqn_SArray_InsertAssert(...)       DQN_HARD_ASSERT(Dqn_SArray_Insert(__VA_ARGS__))
template <typename T>                           T                      Dqn_SArray_PopFront                (Dqn_SArray<T> *array, Dqn_usize count);
template <typename T>                           T                      Dqn_SArray_PopBack                 (Dqn_SArray<T> *array, Dqn_usize count);
template <typename T>                           Dqn_ArrayEraseResult   Dqn_SArray_EraseRange              (Dqn_SArray<T> *array, Dqn_usize begin_index, Dqn_isize count, Dqn_ArrayErase erase);
template <typename T>                           void                   Dqn_SArray_Clear                   (Dqn_SArray<T> *array);
#endif // !defined(DQN_NO_SARRAY)
#if !defined(DQN_NO_FARRAY)
template <typename T, Dqn_usize N>              Dqn_FArray<T, N>       Dqn_FArray_Init                    (T const *array, Dqn_usize count);
#define                                                                Dqn_FArray_HasData(array)          ((array).data && (array).size)
template <typename T, Dqn_usize N>              Dqn_FArray<T, N>       Dqn_FArray_InitSlice               (Dqn_Slice<T> slice);
template <typename T, Dqn_usize N, Dqn_usize K> Dqn_FArray<T, N>       Dqn_FArray_InitCArray              (T const (&items)[K]);
template <typename T, Dqn_usize N>              bool                   Dqn_FArray_IsValid                 (Dqn_FArray<T, N> const *array);
template <typename T, Dqn_usize N>              Dqn_usize              Dqn_FArray_Max                     (Dqn_FArray<T, N> const *) { return N; }
template <typename T, Dqn_usize N>              Dqn_Slice<T>           Dqn_FArray_Slice                   (Dqn_FArray<T, N> const *array);
template <typename T, Dqn_usize N>              T *                    Dqn_FArray_AddArray                (Dqn_FArray<T, N> *array, T const *items, Dqn_usize count);
template <typename T, Dqn_usize N, Dqn_usize K> T *                    Dqn_FArray_AddCArray               (Dqn_FArray<T, N> *array, T const (&items)[K]);
template <typename T, Dqn_usize N>              T *                    Dqn_FArray_Add                     (Dqn_FArray<T, N> *array, T const &item);
#define                                                                Dqn_FArray_AddArrayAssert(...)     DQN_HARD_ASSERT(Dqn_FArray_AddArray(__VA_ARGS__))
#define                                                                Dqn_FArray_AddCArrayAssert(...)    DQN_HARD_ASSERT(Dqn_FArray_AddCArray(__VA_ARGS__))
#define                                                                Dqn_FArray_AddAssert(...)          DQN_HARD_ASSERT(Dqn_FArray_Add(__VA_ARGS__))
template <typename T, Dqn_usize N>              T *                    Dqn_FArray_MakeArray               (Dqn_FArray<T, N> *array, Dqn_usize count, Dqn_ZeroMem zero_mem);
template <typename T, Dqn_usize N>              T *                    Dqn_FArray_Make                    (Dqn_FArray<T, N> *array, Dqn_ZeroMem zero_mem);
#define                                                                Dqn_FArray_MakeArrayAssert(...)    DQN_HARD_ASSERT(Dqn_FArray_MakeArray(__VA_ARGS__))
#define                                                                Dqn_FArray_MakeAssert(...)         DQN_HARD_ASSERT(Dqn_FArray_Make(__VA_ARGS__))
template <typename T, Dqn_usize N>              T *                    Dqn_FArray_InsertArray             (Dqn_FArray<T, N> *array, T const &item, Dqn_usize index);
template <typename T, Dqn_usize N, Dqn_usize K> T *                    Dqn_FArray_InsertCArray            (Dqn_FArray<T, N> *array, Dqn_usize index, T const (&items)[K]);
template <typename T, Dqn_usize N>              T *                    Dqn_FArray_Insert                  (Dqn_FArray<T, N> *array, Dqn_usize index, T const &item);
#define                                                                Dqn_FArray_InsertArrayAssert(...)  DQN_HARD_ASSERT(Dqn_FArray_InsertArray(__VA_ARGS__))
#define                                                                Dqn_FArray_InsertAssert(...)       DQN_HARD_ASSERT(Dqn_FArray_Insert(__VA_ARGS__))
template <typename T, Dqn_usize N>              T                      Dqn_FArray_PopFront                (Dqn_FArray<T, N> *array, Dqn_usize count);
template <typename T, Dqn_usize N>              T                      Dqn_FArray_PopBack                 (Dqn_FArray<T, N> *array, Dqn_usize count);
template <typename T, Dqn_usize N>              Dqn_ArrayFindResult<T> Dqn_FArray_Find                    (Dqn_FArray<T, N> *array, T const &find);
template <typename T, Dqn_usize N>              Dqn_ArrayEraseResult   Dqn_FArray_EraseRange              (Dqn_FArray<T, N> *array, Dqn_usize begin_index, Dqn_isize count, Dqn_ArrayErase erase);
template <typename T, Dqn_usize N>              void                   Dqn_FArray_Clear                   (Dqn_FArray<T, N> *array);
#endif // !defined(DQN_NO_FARRAY)
#if !defined(DQN_NO_SLICE)
#define                                                                DQN_TO_SLICE(val)                  Dqn_Slice_Init((val)->data, (val)->size)
template <typename T>                           Dqn_Slice<T>           Dqn_Slice_Init                     (T* const data, Dqn_usize size);
template <typename T, Dqn_usize N>              Dqn_Slice<T>           Dqn_Slice_InitCArray               (Dqn_Arena *arena, T const (&array)[N]);
template <typename T>                           Dqn_Slice<T>           Dqn_Slice_Copy                     (Dqn_Arena *arena, Dqn_Slice<T> slice);
template <typename T>                           Dqn_Slice<T>           Dqn_Slice_CopyPtr                  (Dqn_Arena *arena, T* const data, Dqn_usize size);
template <typename T>                           Dqn_Slice<T>           Dqn_Slice_Alloc                    (Dqn_Arena *arena, Dqn_usize size, Dqn_ZeroMem zero_mem);
                                                Dqn_Str8               Dqn_Slice_Str8Render               (Dqn_Arena *arena, Dqn_Slice<Dqn_Str8> array, Dqn_Str8 separator);
                                                Dqn_Str8               Dqn_Slice_Str8RenderSpaceSeparated (Dqn_Arena *arena, Dqn_Slice<Dqn_Str8> array);
                                                Dqn_Str16              Dqn_Slice_Str16Render              (Dqn_Arena *arena, Dqn_Slice<Dqn_Str16> array, Dqn_Str16 separator);
                                                Dqn_Str16              Dqn_Slice_Str16RenderSpaceSeparated(Dqn_Arena *arena, Dqn_Slice<Dqn_Str16> array);
#endif // !defined(DQN_NO_SLICE)
#if !defined(DQN_NO_DSMAP)
template <typename T>                           Dqn_DSMap<T>           Dqn_DSMap_Init                     (Dqn_Arena *arena, uint32_t size, Dqn_DSMapFlags flags);
template <typename T>                           void                   Dqn_DSMap_Deinit                   (Dqn_DSMap<T> *map, Dqn_ZeroMem zero_mem);
template <typename T>                           bool                   Dqn_DSMap_IsValid                  (Dqn_DSMap<T> const *map);
template <typename T>                           uint32_t               Dqn_DSMap_Hash                     (Dqn_DSMap<T> const *map, Dqn_DSMapKey key);
template <typename T>                           uint32_t               Dqn_DSMap_HashToSlotIndex          (Dqn_DSMap<T> const *map, Dqn_DSMapKey key);
template <typename T>                           Dqn_DSMapResult<T>     Dqn_DSMap_Find                     (Dqn_DSMap<T> const *map, Dqn_DSMapKey key);
template <typename T>                           Dqn_DSMapResult<T>     Dqn_DSMap_Make                     (Dqn_DSMap<T> *map, Dqn_DSMapKey key);
template <typename T>                           Dqn_DSMapResult<T>     Dqn_DSMap_Set                      (Dqn_DSMap<T> *map, Dqn_DSMapKey key, T const &value);
template <typename T>                           Dqn_DSMapResult<T>     Dqn_DSMap_FindKeyU64               (Dqn_DSMap<T> const *map, uint64_t key);
template <typename T>                           Dqn_DSMapResult<T>     Dqn_DSMap_MakeKeyU64               (Dqn_DSMap<T> *map,       uint64_t key);
template <typename T>                           Dqn_DSMapResult<T>     Dqn_DSMap_SetKeyU64                (Dqn_DSMap<T> *map,       uint64_t key, T const &value);
template <typename T>                           Dqn_DSMapResult<T>     Dqn_DSMap_FindKeyStr8              (Dqn_DSMap<T> const *map, Dqn_Str8 key);
template <typename T>                           Dqn_DSMapResult<T>     Dqn_DSMap_MakeKeyStr8              (Dqn_DSMap<T> *map,       Dqn_Str8 key);
template <typename T>                           Dqn_DSMapResult<T>     Dqn_DSMap_SetKeyStr8               (Dqn_DSMap<T> *map,       Dqn_Str8 key, T const &value);
template <typename T>                           bool                   Dqn_DSMap_Resize                   (Dqn_DSMap<T> *map, uint32_t size);
template <typename T>                           bool                   Dqn_DSMap_Erase                    (Dqn_DSMap<T> *map, Dqn_DSMapKey key);
template <typename T>                           Dqn_DSMapKey           Dqn_DSMap_KeyBuffer                (Dqn_DSMap<T> const *map, void const *data, uint32_t size);
template <typename T>                           Dqn_DSMapKey           Dqn_DSMap_KeyBufferAsU64NoHash     (Dqn_DSMap<T> const *map, void const *data, uint32_t size);
template <typename T>                           Dqn_DSMapKey           Dqn_DSMap_KeyU64                   (Dqn_DSMap<T> const *map, uint64_t u64);
template <typename T>                           Dqn_DSMapKey           Dqn_DSMap_KeyStr8                  (Dqn_DSMap<T> const *map, Dqn_Str8 string);
#define                                                                Dqn_DSMap_KeyCStr8(map, string)    Dqn_DSMap_KeyBuffer(map, string, sizeof((string))/sizeof((string)[0]) - 1)
DQN_API                                         Dqn_DSMapKey           Dqn_DSMap_KeyU64NoHash             (uint64_t u64);
DQN_API                                         bool                   Dqn_DSMap_KeyEquals                (Dqn_DSMapKey lhs, Dqn_DSMapKey rhs);
DQN_API                                         bool                   operator==                         (Dqn_DSMapKey lhs, Dqn_DSMapKey rhs);
#endif // !defined(DQN_NO_DSMAP)
#if !defined(DQN_NO_LIST)
template <typename T>                           Dqn_List<T>            Dqn_List_Init                      (Dqn_usize chunk_size);
template <typename T, size_t N>                 Dqn_List<T>            Dqn_List_InitCArray                (Dqn_Arena *arena, Dqn_usize chunk_size, T const (&array)[N]);
template <typename T>                           T *                    Dqn_List_At                        (Dqn_List<T> *list, Dqn_usize index, Dqn_ListChunk<T> *at_chunk);
template <typename T>                           bool                   Dqn_List_Iterate                   (Dqn_List<T> *list, Dqn_ListIterator<T> *it, Dqn_usize start_index);
template <typename T>                           T *                    Dqn_List_MakeArena                 (Dqn_List<T> *list, Dqn_Arena *arena, Dqn_usize count);
template <typename T>                           T *                    Dqn_List_MakePool                  (Dqn_List<T> *list, Dqn_ChunkPool *pool, Dqn_usize count);
template <typename T>                           T *                    Dqn_List_AddArena                  (Dqn_List<T> *list, Dqn_Arena *arena, T const &value);
template <typename T>                           T *                    Dqn_List_AddPool                   (Dqn_List<T> *list, Dqn_ChunkPool *pool, T const &value);
template <typename T>                           void                   Dqn_List_AddListArena              (Dqn_List<T> *list, Dqn_Arena *arena, Dqn_List<T> other);
template <typename T>                           void                   Dqn_List_AddListArena              (Dqn_List<T> *list, Dqn_ChunkPool *pool, Dqn_List<T> other);
template <typename T>                           Dqn_Slice<T>           Dqn_List_ToSliceCopy               (Dqn_List<T> const *list, Dqn_Arena* arena);
#endif // !defined(DQN_NO_LIST)

// NOTE: [$CARR] Dqn_CArray ////////////////////////////////////////////////////////////////////////
template <typename T> Dqn_ArrayEraseResult Dqn_CArray_EraseRange(T* data, Dqn_usize *size, Dqn_usize begin_index, Dqn_isize count, Dqn_ArrayErase erase)
{
    Dqn_ArrayEraseResult result = {};
    if (!data || !size || *size == 0 || count == 0)
        return result;

    DQN_ASSERTF(count != -1, "There's a bug with negative element erases, see the Dqn_VArray section in dqn_docs.cpp");

    // NOTE: Caculate the end index of the erase range
    Dqn_isize abs_count = DQN_ABS(count);
    Dqn_usize end_index = 0;
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
        Dqn_usize tmp = begin_index;
        begin_index   = end_index;
        end_index     = tmp;
    }

    // NOTE: Ensure indexes are within valid bounds
    begin_index = DQN_MIN(begin_index, *size);
    end_index   = DQN_MIN(end_index,   *size - 1);

    // NOTE: Erase the items in the range [begin_index, one_past_end_index)
    Dqn_usize one_past_end_index = end_index + 1;
    Dqn_usize erase_count        = one_past_end_index - begin_index;
    if (erase_count) {
        T *end  = data + *size;
        T *dest = data + begin_index;
        if (erase == Dqn_ArrayErase_Stable) {
            T *src = dest + erase_count;
            DQN_MEMMOVE(dest, src, (end - src) * sizeof(T));
        } else {
            T *src = end - erase_count;
            DQN_MEMCPY(dest, src, (end - src) * sizeof(T));
        }
        *size -= erase_count;
    }

    result.items_erased = erase_count;
    result.it_index     = begin_index;
    return result;
}

template <typename T> T *Dqn_CArray_MakeArray(T* data, Dqn_usize *size, Dqn_usize max, Dqn_usize count, Dqn_ZeroMem zero_mem)
{
    if (!data || !size || count == 0)
        return nullptr;

    if (!DQN_CHECKF((*size + count) <= max, "Array is out of space (user requested +%zu items, array has %zu/%zu items)", count, *size, max))
        return nullptr;

    // TODO: Use placement new? Why doesn't this work?
    T *result  = data + *size;
    *size     += count;
    if (zero_mem == Dqn_ZeroMem_Yes)
        DQN_MEMSET(result, 0, sizeof(*result) * count);
    return result;
}

template <typename T> T *Dqn_CArray_InsertArray(T *data, Dqn_usize *size, Dqn_usize max, Dqn_usize index, T const *items, Dqn_usize count)
{
    T *result = nullptr;
    if (!data || !size || !items || count <= 0 || ((*size + count) > max))
        return result;

    Dqn_usize clamped_index = DQN_MIN(index, *size);
    if (clamped_index != *size) {
        char const *src           = DQN_CAST(char *)(data + clamped_index);
        char const *dest          = DQN_CAST(char *)(data + (clamped_index + count));
        char const *end           = DQN_CAST(char *)(data + (*size));
        Dqn_usize   bytes_to_move = end - src;
        DQN_MEMMOVE(DQN_CAST(void *) dest, src, bytes_to_move);
    }

    result = data + clamped_index;
    DQN_MEMCPY(result, items, sizeof(T) * count);
    *size += count;
    return result;
}

template <typename T> T Dqn_CArray_PopFront(T* data, Dqn_usize *size, Dqn_usize count)
{
    T result = {};
    if (!data || !size || *size <= 0)
        return result;

    result              = data[0];
    Dqn_usize pop_count = DQN_MIN(count, *size);
    DQN_MEMMOVE(data, data + pop_count, (*size - pop_count) * sizeof(T));
    *size              -= pop_count;
    return result;
}

template <typename T> T Dqn_CArray_PopBack(T* data, Dqn_usize *size, Dqn_usize count)
{
    T result = {};
    if (!data || !size || *size <= 0)
        return result;

    Dqn_usize pop_count = DQN_MIN(count, *size);
    result              = data[(*size - 1)];
    *size              -= pop_count;
    return result;
}

template <typename T> Dqn_ArrayFindResult<T> Dqn_CArray_Find(T *data, Dqn_usize size, T const &value)
{
    Dqn_ArrayFindResult<T> result = {};
    if (!data || size <= 0)
        return result;

    for (Dqn_usize index = 0; !result.data && index < size; index++) {
        T *item = data + index;
        if (*item == value) {
            result.data = item;
            result.index = index;
        }
    }

    return result;
}

#if !defined(DQN_NO_VARRAY)
// NOTE: [$VARR] Dqn_VArray ////////////////////////////////////////////////////////////////////////
template <typename T> Dqn_VArray<T> Dqn_VArray_InitByteSize(Dqn_usize byte_size, uint8_t arena_flags)
{
    Dqn_VArray<T> result = {};
    result.arena         = Dqn_Arena_InitSize(DQN_ARENA_HEADER_SIZE + byte_size, 0 /*commit*/, arena_flags | Dqn_ArenaFlag_NoGrow | Dqn_ArenaFlag_NoPoison);
    if (result.arena.curr) {
        result.data = DQN_CAST(T *)(DQN_CAST(char *)result.arena.curr + result.arena.curr->used);
        result.max  = (result.arena.curr->reserve - result.arena.curr->used) / sizeof(T);
    }
    return result;
}

template <typename T> Dqn_VArray<T> Dqn_VArray_Init(Dqn_usize max, uint8_t arena_flags)
{
    Dqn_VArray<T> result = Dqn_VArray_InitByteSize<T>(max * sizeof(T), arena_flags);
    DQN_ASSERT(result.max >= max);
    return result;
}

template <typename T> Dqn_VArray<T> Dqn_VArray_InitSlice(Dqn_Slice<T> slice, Dqn_usize max, uint8_t arena_flags)
{
    Dqn_usize     real_max = DQN_MAX(slice.size, max);
    Dqn_VArray<T> result   = Dqn_VArray_Init<T>(real_max, arena_flags);
    if (Dqn_VArray_IsValid(&result))
        Dqn_VArray_AddArray(&result, slice.data, slice.size);
    return result;
}

template <typename T, Dqn_usize N> Dqn_VArray<T> Dqn_VArray_InitCArray(T const (&items)[N], Dqn_usize max, uint8_t arena_flags)
{
    Dqn_usize     real_max = DQN_MAX(N, max);
    Dqn_VArray<T> result   = Dqn_VArray_InitSlice(Dqn_Slice_Init(items, N), real_max, arena_flags);
    return result;
}

template <typename T> void Dqn_VArray_Deinit(Dqn_VArray<T> *array)
{
    Dqn_Arena_Deinit(&array->arena);
    *array = {};
}

template <typename T> bool Dqn_VArray_IsValid(Dqn_VArray<T> const *array)
{
    bool result = array && array->data && array->size <= array->max && array->arena.curr;
    return result;
}

template <typename T> Dqn_Slice<T> Dqn_VArray_Slice(Dqn_VArray<T> const *array)
{
    Dqn_Slice<T> result = {};
    if (array)
        result = Dqn_Slice_Init<T>(array->data, array->size);
    return result;
}

template <typename T> T *Dqn_VArray_AddArray(Dqn_VArray<T> *array, T const *items, Dqn_usize count)
{
    T *result = Dqn_VArray_MakeArray(array, count, Dqn_ZeroMem_No);
    if (result)
        DQN_MEMCPY(result, items, count * sizeof(T));
    return result;
}

template <typename T, Dqn_usize N> T *Dqn_VArray_AddCArray(Dqn_VArray<T> *array, T const (&items)[N])
{
    T *result = Dqn_VArray_AddArray(array, items, N);
    return result;
}

template <typename T> T *Dqn_VArray_Add(Dqn_VArray<T> *array, T const &item)
{
    T *result = Dqn_VArray_AddArray(array, &item, 1);
    return result;
}

template <typename T> T *Dqn_VArray_MakeArray(Dqn_VArray<T> *array, Dqn_usize count, Dqn_ZeroMem zero_mem)
{
    if (!Dqn_VArray_IsValid(array))
        return nullptr;

    if (!DQN_CHECKF((array->size + count) < array->max, "Array is out of space (user requested +%zu items, array has %zu/%zu items)", count, array->size, array->max))
        return nullptr;

    // TODO: Use placement new? Why doesn't this work?

    uint8_t prev_flags  = array->arena.flags;
    array->arena.flags |= (Dqn_ArenaFlag_NoGrow | Dqn_ArenaFlag_NoPoison);
    T *result           = Dqn_Arena_NewArray(&array->arena, T, count, zero_mem);
    array->arena.flags  = prev_flags;
    if (result)
        array->size += count;
    return result;
}

template <typename T> T *Dqn_VArray_Make(Dqn_VArray<T> *array, Dqn_ZeroMem zero_mem)
{
    T *result = Dqn_VArray_MakeArray(array, 1, zero_mem);
    return result;
}

template <typename T> T *Dqn_VArray_InsertArray(Dqn_VArray<T> *array, Dqn_usize index, T const *items, Dqn_usize count)
{
    T *result = nullptr;
    if (!Dqn_VArray_IsValid(array))
        return result;
    if (Dqn_VArray_Reserve(array, array->size + count))
        result = Dqn_CArray_InsertArray(array->data, &array->size, array->max, index, items, count);
    return result;
}

template <typename T, Dqn_usize N> T *Dqn_VArray_InsertCArray(Dqn_VArray<T> *array, Dqn_usize index, T const (&items)[N])
{
    T *result = Dqn_VArray_InsertArray(array, index, items, N);
    return result;
}

template <typename T> T *Dqn_VArray_Insert(Dqn_VArray<T> *array, Dqn_usize index, T const &item)
{
    T *result = Dqn_VArray_InsertArray(array, index, &item, 1);
    return result;
}

template <typename T> T *Dqn_VArray_PopFront(Dqn_VArray<T> *array, Dqn_usize count)
{
    T *result = Dqn_CArray_PopFront(array->data, &array->size, count);
    return result;
}

template <typename T> T *Dqn_VArray_PopBack(Dqn_VArray<T> *array, Dqn_usize count)
{
    T *result = Dqn_CArray_PopBack(array->data, &array->size, count);
    return result;
}

template <typename T> Dqn_ArrayEraseResult Dqn_VArray_EraseRange(Dqn_VArray<T> *array, Dqn_usize begin_index, Dqn_isize count, Dqn_ArrayErase erase)
{
    Dqn_ArrayEraseResult result = {};
    if (!Dqn_VArray_IsValid(array))
        return result;
    result = Dqn_CArray_EraseRange<T>(array->data, &array->size, begin_index, count, erase);
    Dqn_Arena_Pop(&array->arena, result.items_erased * sizeof(T));
    return result;
}

template <typename T> void Dqn_VArray_Clear(Dqn_VArray<T> *array, Dqn_ZeroMem zero_mem)
{
    if (array) {
        if (zero_mem == Dqn_ZeroMem_Yes)
            DQN_MEMSET(array->data, 0, array->size * sizeof(T));
        Dqn_Arena_PopTo(&array->arena, 0);
        array->size = 0;
    }
}

template <typename T> bool Dqn_VArray_Reserve(Dqn_VArray<T> *array, Dqn_usize count)
{
    if (!Dqn_VArray_IsValid(array) || count == 0)
        return false;
    bool result = Dqn_Arena_CommitTo(&array->arena, DQN_ARENA_HEADER_SIZE + (count * sizeof(T)));
    return result;
}
#endif // !defined(DQN_NO_VARRAY)

#if !defined(DQN_NO_SARRAY)
// NOTE: [$SARR] Dqn_SArray ////////////////////////////////////////////////////////////////////////
template <typename T> Dqn_SArray<T> Dqn_SArray_Init(Dqn_Arena *arena, Dqn_usize size, Dqn_ZeroMem zero_mem)
{
    Dqn_SArray<T> result = {};
    if (!arena || !size)
        return result;
    result.data = Dqn_Arena_NewArray(arena, T, size, zero_mem);
    if (result.data)
        result.max = size;
    return result;
}

template <typename T> Dqn_SArray<T> Dqn_SArray_InitSlice(Dqn_Arena *arena, Dqn_Slice<T> slice, Dqn_usize size, Dqn_ZeroMem zero_mem)
{
    Dqn_usize     max    = DQN_MAX(slice.size, size);
    Dqn_SArray<T> result = Dqn_SArray_Init<T>(arena, max, Dqn_ZeroMem_No);
    if (Dqn_SArray_IsValid(&result)) {
        Dqn_SArray_AddArray(&result, slice.data, slice.size);
        if (zero_mem == Dqn_ZeroMem_Yes)
            DQN_MEMSET(result.data + result.size, 0, (result.max - result.size) * sizeof(T));
    }
    return result;
}

template <typename T, size_t N> Dqn_SArray<T> Dqn_SArray_InitCArray(Dqn_Arena *arena, T const (&array)[N], Dqn_usize size, Dqn_ZeroMem zero_mem)
{
    Dqn_SArray<T> result = Dqn_SArray_InitSlice(arena, Dqn_Slice_Init(DQN_CAST(T *)array, N), size, zero_mem);
    return result;
}

template <typename T> bool Dqn_SArray_IsValid(Dqn_SArray<T> const *array)
{
    bool result = array && array->data && array->size <= array->max;
    return result;
}

template <typename T> Dqn_Slice<T> Dqn_SArray_Slice(Dqn_SArray<T> const *array)
{
    Dqn_Slice<T> result = {};
    if (array)
        result = Dqn_Slice_Init<T>(DQN_CAST(T *)array->data, array->size);
    return result;
}

template <typename T> T *Dqn_SArray_MakeArray(Dqn_SArray<T> *array, Dqn_usize count, Dqn_ZeroMem zero_mem)
{
    if (!Dqn_SArray_IsValid(array))
        return nullptr;
    T *result = Dqn_CArray_MakeArray(array->data, &array->size, array->max, count, zero_mem);
    return result;
}

template <typename T> T *Dqn_SArray_Make(Dqn_SArray<T> *array, Dqn_ZeroMem zero_mem)
{
    T *result = Dqn_SArray_MakeArray(array, 1, zero_mem);
    return result;
}

template <typename T> T *Dqn_SArray_AddArray(Dqn_SArray<T> *array, T const *items, Dqn_usize count)
{
    T *result = Dqn_SArray_MakeArray(array, count, Dqn_ZeroMem_No);
    if (result)
        DQN_MEMCPY(result, items, count * sizeof(T));
    return result;
}

template <typename T, Dqn_usize N> T *Dqn_SArray_AddCArray(Dqn_SArray<T> *array, T const (&items)[N])
{
    T *result = Dqn_SArray_AddArray(array, items, N);
    return result;
}

template <typename T> T *Dqn_SArray_Add(Dqn_SArray<T> *array, T const &item)
{
    T *result = Dqn_SArray_AddArray(array, &item, 1);
    return result;
}

template <typename T> T *Dqn_SArray_InsertArray(Dqn_SArray<T> *array, Dqn_usize index, T const *items, Dqn_usize count)
{
    T *result = nullptr;
    if (!Dqn_SArray_IsValid(array))
        return result;
    result = Dqn_CArray_InsertArray(array->data, &array->size, array->max, index, items, count);
    return result;
}

template <typename T, Dqn_usize N> T *Dqn_SArray_InsertCArray(Dqn_SArray<T> *array, Dqn_usize index, T const (&items)[N])
{
    T *result = Dqn_SArray_InsertArray(array, index, items, N);
    return result;
}

template <typename T> T *Dqn_SArray_Insert(Dqn_SArray<T> *array, Dqn_usize index, T const &item)
{
    T *result = Dqn_SArray_InsertArray(array, index, &item, 1);
    return result;
}

template <typename T> T Dqn_SArray_PopFront(Dqn_SArray<T> *array, Dqn_usize count)
{
    T result = Dqn_CArray_PopFront(array->data, &array->size, count);
    return result;
}

template <typename T> T Dqn_SArray_PopBack(Dqn_SArray<T> *array, Dqn_usize count)
{
    T result = Dqn_CArray_PopBack(array->data, &array->size, count);
    return result;
}

template <typename T> Dqn_ArrayEraseResult Dqn_SArray_EraseRange(Dqn_SArray<T> *array, Dqn_usize begin_index, Dqn_isize count, Dqn_ArrayErase erase)
{
    Dqn_ArrayEraseResult result = {};
    if (!Dqn_SArray_IsValid(array) || array->size == 0 || count == 0)
        return result;
    result = Dqn_CArray_EraseRange(array->data, &array->size, begin_index, count, erase);
    return result;
}

template <typename T> void Dqn_SArray_Clear(Dqn_SArray<T> *array)
{
    if (array)
        array->size = 0;
}
#endif // !defined(DQN_NO_SARRAY)

#if !defined(DQN_NO_FARRAY)
// NOTE: [$FARR] Dqn_FArray ////////////////////////////////////////////////////////////////////////
template <typename T, Dqn_usize N> Dqn_FArray<T, N> Dqn_FArray_Init(T const *array, Dqn_usize count)
{
    Dqn_FArray<T, N> result = {};
    bool added              = Dqn_FArray_AddArray(&result, array, count);
    DQN_ASSERT(added);
    return result;
}

template <typename T, Dqn_usize N> Dqn_FArray<T, N> Dqn_FArray_InitSlice(Dqn_Slice<T> slice)
{
    Dqn_FArray<T, N> result = Dqn_FArray_Init(slice.data, slice.size);
    return result;
}

template <typename T, Dqn_usize N, Dqn_usize K> Dqn_FArray<T, N> Dqn_FArray_InitCArray(T const (&items)[K])
{
    Dqn_FArray<T, N> result = Dqn_FArray_Init<T, N>(items, K);
    return result;
}

template <typename T, Dqn_usize N> bool Dqn_FArray_IsValid(Dqn_FArray<T, N> const *array)
{
    bool result = array && array->size <= DQN_ARRAY_UCOUNT(array->data);
    return result;
}

template <typename T, Dqn_usize N> Dqn_Slice<T> Dqn_FArray_Slice(Dqn_FArray<T, N> const *array)
{
    Dqn_Slice<T> result = {};
    if (array)
        result = Dqn_Slice_Init<T>(DQN_CAST(T *)array->data, array->size);
    return result;
}

template <typename T, Dqn_usize N> T *Dqn_FArray_AddArray(Dqn_FArray<T, N> *array, T const *items, Dqn_usize count)
{
    T *result = Dqn_FArray_MakeArray(array, count, Dqn_ZeroMem_No);
    if (result)
        DQN_MEMCPY(result, items, count * sizeof(T));
    return result;
}

template <typename T, Dqn_usize N, Dqn_usize K> T *Dqn_FArray_AddCArray(Dqn_FArray<T, N> *array, T const (&items)[K])
{
    T *result = Dqn_FArray_MakeArray(array, K, Dqn_ZeroMem_No);
    if (result)
        DQN_MEMCPY(result, items, K * sizeof(T));
    return result;
}

template <typename T, Dqn_usize N> T *Dqn_FArray_Add(Dqn_FArray<T, N> *array, T const &item)
{
    T *result = Dqn_FArray_AddArray(array, &item, 1);
    return result;
}

template <typename T, Dqn_usize N> T *Dqn_FArray_MakeArray(Dqn_FArray<T, N> *array, Dqn_usize count, Dqn_ZeroMem zero_mem)
{
    if (!Dqn_FArray_IsValid(array))
        return nullptr;
    T *result = Dqn_CArray_MakeArray(array->data, &array->size, N, count, zero_mem);
    return result;
}

template <typename T, Dqn_usize N> T *Dqn_FArray_Make(Dqn_FArray<T, N> *array, Dqn_ZeroMem zero_mem)
{
    T *result = Dqn_FArray_MakeArray(array, 1, zero_mem);
    return result;
}

template <typename T, Dqn_usize N> T *Dqn_FArray_InsertArray(Dqn_FArray<T, N> *array, Dqn_usize index, T const *items, Dqn_usize count)
{
    T *result = nullptr;
    if (!Dqn_FArray_IsValid(array))
        return result;
    result = Dqn_CArray_InsertArray(array->data, &array->size, N, index, items, count);
    return result;
}

template <typename T, Dqn_usize N, Dqn_usize K> T *Dqn_FArray_InsertCArray(Dqn_FArray<T, N> *array, Dqn_usize index, T const (&items)[K])
{
    T *result = Dqn_FArray_InsertArray(array, index, items, K);
    return result;
}

template <typename T, Dqn_usize N> T *Dqn_FArray_Insert(Dqn_FArray<T, N> *array, Dqn_usize index, T const &item)
{
    T *result = Dqn_FArray_InsertArray(array, index, &item, 1);
    return result;
}

template <typename T, Dqn_usize N> T Dqn_FArray_PopFront(Dqn_FArray<T, N> *array, Dqn_usize count)
{
    T result = Dqn_CArray_PopFront(array->data, &array->size, count);
    return result;
}

template <typename T, Dqn_usize N> T Dqn_FArray_PopBack(Dqn_FArray<T, N> *array, Dqn_usize count)
{
    T result = Dqn_CArray_PopBack(array->data, &array->size, count);
    return result;
}

template <typename T, Dqn_usize N> Dqn_ArrayFindResult<T> Dqn_FArray_Find(Dqn_FArray<T, N> *array, T const &find)
{
    Dqn_ArrayFindResult<T> result = Dqn_CArray_Find<T>(array->data, array->size, find);
    return result;
}

template <typename T, Dqn_usize N> Dqn_ArrayEraseResult Dqn_FArray_EraseRange(Dqn_FArray<T, N> *array, Dqn_usize begin_index, Dqn_isize count, Dqn_ArrayErase erase)
{
    Dqn_ArrayEraseResult result = {};
    if (!Dqn_FArray_IsValid(array) || array->size == 0 || count == 0)
        return result;
    result = Dqn_CArray_EraseRange(array->data, &array->size, begin_index, count, erase);
    return result;
}

template <typename T, Dqn_usize N> void Dqn_FArray_Clear(Dqn_FArray<T, N> *array)
{
    if (array)
        array->size = 0;
}
#endif // !defined(DQN_NO_FARRAY)

#if !defined(DQN_NO_SLICE)
template <typename T> Dqn_Slice<T> Dqn_Slice_Init(T* const data, Dqn_usize size)
{
    Dqn_Slice<T> result = {};
    if (data) {
        result.data = data;
        result.size = size;
    }
    return result;
}

template <typename T, Dqn_usize N>
Dqn_Slice<T> Dqn_Slice_InitCArray(Dqn_Arena *arena, T const (&array)[N])
{
    Dqn_Slice<T> result = Dqn_Slice_Alloc<T>(arena, N, Dqn_ZeroMem_No);
    if (result.data)
        DQN_MEMCPY(result.data, array, sizeof(T) * N);
    return result;
}

template <typename T> Dqn_Slice<T> Dqn_Slice_CopyPtr(Dqn_Arena *arena, T *const data, Dqn_usize size)
{
    T           *copy   = Dqn_Arena_NewArrayCopy(arena, T, data, size);
    Dqn_Slice<T> result = Dqn_Slice_Init(copy, copy ? size : 0);
    return result;
}

template <typename T> Dqn_Slice<T> Dqn_Slice_Copy(Dqn_Arena *arena, Dqn_Slice<T> slice)
{
    Dqn_Slice<T> result = Dqn_Slice_CopyPtr(arena, slice.data, slice.size);
    return result;
}

template <typename T> Dqn_Slice<T> Dqn_Slice_Alloc(Dqn_Arena *arena, Dqn_usize size, Dqn_ZeroMem zero_mem)
{
    Dqn_Slice<T> result = {};
    if (!arena || size == 0)
        return result;
    result.data = Dqn_Arena_NewArray(arena, T, size, zero_mem);
    if (result.data)
        result.size = size;
    return result;
}

#endif // !defined(DQN_NO_SLICE)

#if !defined(DQN_NO_DSMAP)
// NOTE: [$DMAP] Dqn_DSMap /////////////////////////////////////////////////////////////////////////
uint32_t const DQN_DS_MAP_DEFAULT_HASH_SEED = 0x8a1ced49;
uint32_t const DQN_DS_MAP_SENTINEL_SLOT     = 0;

template <typename T> Dqn_DSMap<T> Dqn_DSMap_Init(Dqn_Arena *arena, uint32_t size, Dqn_DSMapFlags flags)
{
    Dqn_DSMap<T> result = {};
    if (!DQN_CHECKF(Dqn_IsPowerOfTwo(size), "Power-of-two size required, given size was '%u'", size))
        return result;
    if (!DQN_CHECKF(size > 0, "Non-zero size must be given"))
        return result;
    if (!DQN_CHECK(arena))
        return result;
    result.arena        = arena;
    result.hash_to_slot = Dqn_Arena_NewArray(result.arena, uint32_t,         size, Dqn_ZeroMem_Yes);
    result.slots        = Dqn_Arena_NewArray(result.arena, Dqn_DSMapSlot<T>, size, Dqn_ZeroMem_Yes);
    result.occupied     = 1; // For sentinel
    result.size         = size;
    result.initial_size = size;
    result.flags        = flags;
    DQN_ASSERTF(result.hash_to_slot && result.slots, "We pre-allocated a block of memory sufficient in size for the 2 arrays. Maybe the pointers needed extra space because of natural alignment?");
    return result;
}

template <typename T>
void Dqn_DSMap_Deinit(Dqn_DSMap<T> *map, Dqn_ZeroMem zero_mem)
{
    if (!map)
        return;
    // TODO(doyle): Use zero_mem
    (void)zero_mem;
    Dqn_Arena_Deinit(map->arena);
    *map = {};
}

template <typename T>
bool Dqn_DSMap_IsValid(Dqn_DSMap<T> const *map)
{
    bool result = map &&
                  map->arena &&
                  map->hash_to_slot &&                  // Hash to slot mapping array must be allocated
                  map->slots &&                         // Slots array must be allocated
                  (map->size & (map->size - 1)) == 0 && // Must be power of two size
                  map->occupied >= 1;                   // DQN_DS_MAP_SENTINEL_SLOT takes up one slot
    return result;
}

template <typename T>
uint32_t Dqn_DSMap_Hash(Dqn_DSMap<T> const *map, Dqn_DSMapKey key)
{
    uint32_t result = 0;
    if (!map)
        return result;

    if (key.type == Dqn_DSMapKeyType_U64NoHash) {
        result = DQN_CAST(uint32_t)key.u64;
        return result;
    }

    if (key.type == Dqn_DSMapKeyType_BufferAsU64NoHash) {
        result = key.hash;
        return result;
    }

    uint32_t seed = map->hash_seed ? map->hash_seed : DQN_DS_MAP_DEFAULT_HASH_SEED;
    if (map->hash_function) {
        map->hash_function(key, seed);
    } else {
        // NOTE: Courtesy of Demetri Spanos (which this hash table was inspired
        // from), the following is a hashing function snippet provided for
        // reliable, quick and simple quality hashing functions for hash table
        // use.
        // Source: https://github.com/demetri/scribbles/blob/c475464756c104c91bab83ed4e14badefef12ab5/hashing/ub_aware_hash_functions.c

        char const *key_ptr = nullptr;
        uint32_t len        = 0;
        uint32_t h          = seed;
        switch (key.type) {
            case Dqn_DSMapKeyType_BufferAsU64NoHash: /*FALLTHRU*/
            case Dqn_DSMapKeyType_U64NoHash: DQN_INVALID_CODE_PATH; /*FALLTHRU*/
            case Dqn_DSMapKeyType_Invalid: break;

            case Dqn_DSMapKeyType_Buffer:
                key_ptr = DQN_CAST(char const *)key.buffer_data;
                len     = key.buffer_size;
                break;

            case Dqn_DSMapKeyType_U64:
                key_ptr = DQN_CAST(char const *)&key.u64;
                len     = sizeof(key.u64);
                break;
        }

        // Murmur3 32-bit without UB unaligned accesses
        // uint32_t mur3_32_no_UB(const void *key, int len, uint32_t h)

        // main body, work on 32-bit blocks at a time
        for (uint32_t i = 0; i < len / 4; i++) {
            uint32_t k;
            memcpy(&k, &key_ptr[i * 4], sizeof(k));

            k *= 0xcc9e2d51;
            k = ((k << 15) | (k >> 17)) * 0x1b873593;
            h = (((h ^ k) << 13) | ((h ^ k) >> 19)) * 5 + 0xe6546b64;
        }

        // load/mix up to 3 remaining tail bytes into a tail block
        uint32_t t    = 0;
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
uint32_t Dqn_DSMap_HashToSlotIndex(Dqn_DSMap<T> const *map, Dqn_DSMapKey key)
{
    DQN_ASSERT(key.type != Dqn_DSMapKeyType_Invalid);
    uint32_t result = DQN_DS_MAP_SENTINEL_SLOT;
    if (!Dqn_DSMap_IsValid(map))
        return result;

    result = key.hash & (map->size - 1);
    for (;;) {
        if (map->hash_to_slot[result] == DQN_DS_MAP_SENTINEL_SLOT)
            return result;

        Dqn_DSMapSlot<T> *slot = map->slots + map->hash_to_slot[result];
        if (slot->key.type == Dqn_DSMapKeyType_Invalid || (slot->key.hash == key.hash && slot->key == key))
            return result;

        result = (result + 1) & (map->size - 1);
    }
}

template <typename T>
Dqn_DSMapResult<T> Dqn_DSMap_Find(Dqn_DSMap<T> const *map, Dqn_DSMapKey key)
{
    Dqn_DSMapResult<T> result = {};
    if (Dqn_DSMap_IsValid(map)) {
        uint32_t index = Dqn_DSMap_HashToSlotIndex(map, key);
        if (map->hash_to_slot[index] != DQN_DS_MAP_SENTINEL_SLOT) {
            result.slot  = map->slots + map->hash_to_slot[index];
            result.value = &result.slot->value;
            result.found = true;
        }
    }
    return result;
}

template <typename T>
Dqn_DSMapResult<T> Dqn_DSMap_Make(Dqn_DSMap<T> *map, Dqn_DSMapKey key)
{
    Dqn_DSMapResult<T> result = {};
    if (!Dqn_DSMap_IsValid(map))
        return result;

    uint32_t index = Dqn_DSMap_HashToSlotIndex(map, key);
    if (map->hash_to_slot[index] == DQN_DS_MAP_SENTINEL_SLOT) {
        // NOTE: Create the slot
        map->hash_to_slot[index] = map->occupied++;

        // NOTE: Check if resize is required
        bool map_is_75pct_full = (map->occupied * 4) > (map->size * 3);
        if (map_is_75pct_full) {
            if (!Dqn_DSMap_Resize(map, map->size * 2))
                return result;
            result = Dqn_DSMap_Make(map, key);
        } else {
            result.slot      = map->slots + map->hash_to_slot[index];
            result.slot->key = key; // NOTE: Assign key to new slot
            if ((result.slot->key.type == Dqn_DSMapKeyType_Buffer ||
                 result->slot.key.type == Dqn_DSMapKeyType_BufferAsU64NoHash) &&
                !key.no_copy_buffer) {
                result.slot->key.buffer_data = Dqn_Arena_Copy(map->arena, key.buffer_data, key.buffer_size, 1);
            }
        }
    } else {
        result.slot  = map->slots + map->hash_to_slot[index];
        result.found = true;
    }

    result.value = &result.slot->value;
    DQN_ASSERT(result.slot->key.type != Dqn_DSMapKeyType_Invalid);
    return result;
}

template <typename T>
Dqn_DSMapResult<T> Dqn_DSMap_Set(Dqn_DSMap<T> *map, Dqn_DSMapKey key, T const &value)
{
    Dqn_DSMapResult<T> result = {};
    if (!Dqn_DSMap_IsValid(map))
        return result;

    result             = Dqn_DSMap_Make(map, key);
    result.slot->value = value;
    return result;
}

template <typename T>
Dqn_DSMapResult<T> Dqn_DSMap_FindKeyU64(Dqn_DSMap<T> const *map, uint64_t key)
{
    Dqn_DSMapKey map_key      = Dqn_DSMap_KeyU64(map, key);
    Dqn_DSMapResult<T> result = Dqn_DSMap_Find(map, map_key);
    return result;
}

template <typename T>
Dqn_DSMapResult<T> Dqn_DSMap_MakeKeyU64(Dqn_DSMap<T> *map, uint64_t key)
{
    Dqn_DSMapKey map_key      = Dqn_DSMap_KeyU64(map, key);
    Dqn_DSMapResult<T> result = Dqn_DSMap_Make(map, map_key);
    return result;
}

template <typename T>
Dqn_DSMapResult<T> Dqn_DSMap_SetKeyU64(Dqn_DSMap<T> *map, uint64_t key, T const &value)
{
    Dqn_DSMapKey map_key      = Dqn_DSMap_KeyU64(map, key);
    Dqn_DSMapResult<T> result = Dqn_DSMap_Set(map, map_key, value);
    return result;
}

template <typename T>
Dqn_DSMapResult<T> Dqn_DSMap_FindKeyStr8(Dqn_DSMap<T> const *map, Dqn_Str8 key)
{
    Dqn_DSMapKey map_key      = Dqn_DSMap_KeyStr8(map, key);
    Dqn_DSMapResult<T> result = Dqn_DSMap_Find(map, map_key);
    return result;
}

template <typename T>
Dqn_DSMapResult<T> Dqn_DSMap_MakeKeyStr8(Dqn_DSMap<T> *map, Dqn_Str8 key)
{
    Dqn_DSMapKey map_key      = Dqn_DSMap_KeyStr8(map, key);
    Dqn_DSMapResult<T> result = Dqn_DSMap_Make(map, map_key);
    return result;
}

template <typename T>
Dqn_DSMapResult<T> Dqn_DSMap_SetKeyStr8(Dqn_DSMap<T> *map, Dqn_Str8 key, T const &value)
{
    Dqn_DSMapKey map_key      = Dqn_DSMap_KeyStr8(map, key);
    Dqn_DSMapResult<T> result = Dqn_DSMap_Set(map, map_key);
    return result;
}

template <typename T>
bool Dqn_DSMap_Resize(Dqn_DSMap<T> *map, uint32_t size)
{
    if (!Dqn_DSMap_IsValid(map) || size < map->occupied || size < map->initial_size)
        return false;

    Dqn_Arena *prev_arena = map->arena;
    Dqn_Arena  new_arena  = {};
    new_arena.flags       = prev_arena->flags;

    Dqn_DSMap<T> new_map = Dqn_DSMap_Init<T>(&new_arena, size, map->flags);
    if (!Dqn_DSMap_IsValid(&new_map))
        return false;

    new_map.initial_size = map->initial_size;
    for (uint32_t old_index = 1 /*Sentinel*/; old_index < map->occupied; old_index++) {
        Dqn_DSMapSlot<T> *old_slot = map->slots + old_index;
        Dqn_DSMapKey      old_key  = old_slot->key;
        if (old_key.type == Dqn_DSMapKeyType_Invalid)
            continue;
        Dqn_DSMap_Set(&new_map, old_key, old_slot->value);
    }

    if ((map->flags & Dqn_DSMapFlags_DontFreeArenaOnResize) == 0)
        Dqn_DSMap_Deinit(map, Dqn_ZeroMem_No);
    *map        = new_map;    // Update the map inplace
    map->arena  = prev_arena; // Restore the previous arena pointer, it's been de-init-ed
    *map->arena = new_arena;  // Re-init the old arena with the new data
    return true;
}

template <typename T>
bool Dqn_DSMap_Erase(Dqn_DSMap<T> *map, Dqn_DSMapKey key)
{
    if (!Dqn_DSMap_IsValid(map))
        return false;

    uint32_t index      = Dqn_DSMap_HashToSlotIndex(map, key);
    uint32_t slot_index = map->hash_to_slot[index];
    if (slot_index == DQN_DS_MAP_SENTINEL_SLOT)
        return false;

    // NOTE: Mark the slot as unoccupied
    map->hash_to_slot[index] = DQN_DS_MAP_SENTINEL_SLOT;
    map->slots[slot_index]   = {}; // TODO: Optional?

    if (map->occupied > 1 /*Sentinel*/) {
        // NOTE: Repair the hash chain, e.g. rehash all the items after the removed
        // element and reposition them if necessary.
        for (uint32_t probe_index = index;;) {
            probe_index = (probe_index + 1) & (map->size - 1);
            if (map->hash_to_slot[probe_index] == DQN_DS_MAP_SENTINEL_SLOT)
                break;

            Dqn_DSMapSlot<T> *probe = map->slots + map->hash_to_slot[probe_index];
            uint32_t new_index      = probe->key.hash & (map->size - 1);
            if (index <= probe_index) {
                if (index < new_index && new_index <= probe_index)
                    continue;
            } else {
                if (index < new_index || new_index <= probe_index)
                    continue;
            }

            map->hash_to_slot[index]       = map->hash_to_slot[probe_index];
            map->hash_to_slot[probe_index] = DQN_DS_MAP_SENTINEL_SLOT;
            index                          = probe_index;
        }

        // NOTE: We have erased a slot from the hash table, this leaves a gap
        // in our contiguous array. After repairing the chain, the hash mapping
        // is correct.
        // We will now fill in the vacant spot that we erased using the last 
        // element in the slot list.
        if (map->occupied >= 3 /*Ignoring sentinel, at least 2 other elements to unstable erase*/) {
            uint32_t last_index = map->occupied - 1;
            if (last_index != slot_index) {
                // NOTE: Copy in last slot to the erase slot
                Dqn_DSMapSlot<T> *last_slot = map->slots + last_index;
                map->slots[slot_index]      = *last_slot;

                // NOTE: Update the hash-to-slot mapping for the value that was copied in
                uint32_t hash_to_slot_index           = Dqn_DSMap_HashToSlotIndex(map, last_slot->key);
                map->hash_to_slot[hash_to_slot_index] = slot_index;
                *last_slot = {}; // TODO: Optional?
            }
        }
    }

    map->occupied--;
    bool map_is_below_25pct_full = (map->occupied * 4) < (map->size * 1);
    if (map_is_below_25pct_full && (map->size / 2) >= map->initial_size)
        Dqn_DSMap_Resize(map, map->size / 2);

    return true;
}

template <typename T>
Dqn_DSMapKey Dqn_DSMap_KeyBuffer(Dqn_DSMap<T> const *map, void const *data, Dqn_usize size)
{
    DQN_ASSERT(size > 0 && size <= UINT32_MAX);
    Dqn_DSMapKey result = {};
    result.type         = Dqn_DSMapKeyType_Buffer;
    result.buffer_data  = data;
    result.buffer_size  = DQN_CAST(uint32_t) size;
    result.hash         = Dqn_DSMap_Hash(map, result);
    return result;
}

template <typename T>
Dqn_DSMapKey Dqn_DSMap_KeyBufferAsU64NoHash(Dqn_DSMap<T> const *map, void const *data, uint32_t size)
{
    Dqn_DSMapKey result = {};
    result.type         = Dqn_DSMapKeyType_BufferAsU64NoHash;
    result.buffer_data  = data;
    result.buffer_size  = DQN_CAST(uint32_t) size;
    DQN_ASSERT(size >= sizeof(result.hash));
    DQN_MEMCPY(&result.hash, data, sizeof(result.hash));
    return result;
}

template <typename T>
Dqn_DSMapKey Dqn_DSMap_KeyU64(Dqn_DSMap<T> const *map, uint64_t u64)
{
    Dqn_DSMapKey result = {};
    result.type         = Dqn_DSMapKeyType_U64;
    result.u64          = u64;
    result.hash         = Dqn_DSMap_Hash(map, result);
    return result;
}

template <typename T>
Dqn_DSMapKey Dqn_DSMap_KeyStr8(Dqn_DSMap<T> const *map, Dqn_Str8 string)
{
    Dqn_DSMapKey result = Dqn_DSMap_KeyBuffer(map, string.data, string.size);
    return result;
}
#endif // !defined(DQN_NO_DSMAP)

#if !defined(DQN_NO_LIST)
// NOTE: [$LIST] Dqn_List //////////////////////////////////////////////////////////////////////////
template <typename T> Dqn_List<T> Dqn_List_Init(Dqn_usize chunk_size)
{
    Dqn_List<T> result = {};
    result.chunk_size  = chunk_size;
    return result;
}

template <typename T, size_t N> Dqn_List<T> Dqn_List_InitCArray(Dqn_Arena *arena, Dqn_usize chunk_size, T const (&array)[N])
{
    Dqn_List<T> result = Dqn_List_Init<T>(arena, chunk_size);
    DQN_FOR_UINDEX (index, N)
        Dqn_List_Add(&result, array[index]);
    return result;
}

template <typename T> Dqn_List<T> Dqn_List_InitSliceCopy(Dqn_Arena *arena, Dqn_usize chunk_size, Dqn_Slice<T> slice)
{
    Dqn_List<T> result = Dqn_List_Init<T>(arena, chunk_size);
    DQN_FOR_UINDEX (index, slice.size)
        Dqn_List_Add(&result, slice.data[index]);
    return result;
}

template <typename T> DQN_API bool Dqn_List_AttachTail_(Dqn_List<T> *list, Dqn_ListChunk<T> *tail)
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

template <typename T> DQN_API Dqn_ListChunk<T> *Dqn_List_AllocArena_(Dqn_List<T> *list, Dqn_Arena *arena, Dqn_usize count)
{
    auto            *result = Dqn_Arena_New(arena, Dqn_ListChunk<T>, Dqn_ZeroMem_Yes);
    Dqn_ArenaTempMem tmem   = Dqn_Arena_TempMemBegin(arena);
    if (!result)
        return nullptr;

    Dqn_usize items = DQN_MAX(list->chunk_size, count);
    result->data    = Dqn_Arena_NewArray(arena, T, items, Dqn_ZeroMem_Yes);
    result->size    = items;
    if (!result->data) {
        Dqn_Arena_TempMemEnd(tmem);
        result = nullptr;
    }

    Dqn_List_AttachTail_(list, result);
    return result;
}

template <typename T> DQN_API Dqn_ListChunk<T> *Dqn_List_AllocPool_(Dqn_List<T> *list, Dqn_ChunkPool *pool, Dqn_usize count)
{
    auto *result = Dqn_ChunkPool_New(pool, Dqn_ListChunk<T>);
    if (!result)
        return nullptr;

    Dqn_usize items = DQN_MAX(list->chunk_size, count);
    result->data    = Dqn_ChunkPool_NewArray(pool, T, items);
    result->size    = items;
    if (!result->data) {
        Dqn_ChunkPool_Dealloc(result);
        result = nullptr;
    }

    Dqn_List_AttachTail_(list, result);
    return result;
}

template <typename T> DQN_API T *Dqn_List_MakeArena(Dqn_List<T> *list, Dqn_Arena *arena, Dqn_usize count)
{
    if (list->chunk_size == 0)
        list->chunk_size = 128;

    if (!list->tail || (list->tail->count + count) > list->tail->size) {
        if (!Dqn_List_AllocArena_(list, arena, count))
            return nullptr;
    }

    T *result = list->tail->data + list->tail->count;
    list->tail->count += count;
    list->count       += count;
    return result;
}

template <typename T> DQN_API T *Dqn_List_MakePool(Dqn_List<T> *list, Dqn_ChunkPool *pool, Dqn_usize count)
{
    if (list->chunk_size == 0)
        list->chunk_size = 128;

    if (!list->tail || (list->tail->count + count) > list->tail->size) {
        if (!Dqn_List_AllocPool_(list, pool, count))
            return nullptr;
    }

    T *result = list->tail->data + list->tail->count;
    list->tail->count += count;
    list->count       += count;
    return result;
}

template <typename T> DQN_API T *Dqn_List_AddArena(Dqn_List<T> *list, Dqn_Arena *arena, T const &value)
{
    T *result = Dqn_List_MakeArena(list, arena, 1);
    *result   = value;
    return result;
}

template <typename T> DQN_API T *Dqn_List_AddPool(Dqn_List<T> *list, Dqn_ChunkPool *pool, T const &value)
{
    T *result = Dqn_List_MakePool(list, pool, 1);
    *result   = value;
    return result;
}

template <typename T, size_t N> DQN_API bool Dqn_List_AddCArray(Dqn_List<T> *list, T const (&array)[N])
{
    if (!list || list->chunk_size <= 0)
        return false;
    for (T const &item : array) {
        if (!Dqn_List_Add(list, item))
            return false;
    }
    return true;
}

template <typename T> DQN_API void Dqn_List_AddListArena(Dqn_List<T> *list, Dqn_Arena *arena, Dqn_List<T> other)
{
    if (!list || list->chunk_size <= 0)
        return;
    // TODO(doyle): Copy chunk by chunk
    for (Dqn_ListIterator<T> it = {}; Dqn_List_Iterate(&other, &it, 0 /*start_index*/); )
        Dqn_List_AddArena(list, arena, *it.data);
}

template <typename T> DQN_API void Dqn_List_AddListPool(Dqn_List<T> *list, Dqn_ChunkPool *pool, Dqn_List<T> other)
{
    if (!list || list->chunk_size <= 0)
        return;
    // TODO(doyle): Copy chunk by chunk
    for (Dqn_ListIterator<T> it = {}; Dqn_List_Iterate(&other, &it, 0 /*start_index*/); )
        Dqn_List_AddPool(list, pool, *it.data);
}

template <typename T> DQN_API bool Dqn_List_Iterate(Dqn_List<T> *list, Dqn_ListIterator<T> *it, Dqn_usize start_index)
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
            Dqn_List_At(list, start_index, &it->chunk);
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
            result         = true;
        }
    }

    if (!it->chunk)
        DQN_ASSERT(result == false);
    return result;
}

template <typename T> DQN_API T *Dqn_List_At(Dqn_List<T> *list, Dqn_usize index, Dqn_ListChunk<T> **at_chunk)
{
    if (!list || !list->chunk_size || index >= list->count)
        return nullptr;

    Dqn_usize total_chunks       = (list->count / list->chunk_size) + ((list->chunk_size % list->count) ? 1 : 0);
    Dqn_usize desired_chunk      = index / list->chunk_size;
    Dqn_usize forward_scan_dist  = desired_chunk;
    Dqn_usize backward_scan_dist = total_chunks - desired_chunk;

    // NOTE: Linearly scan forwards/backwards to the chunk we need. We don't
    // have random access to chunks
    Dqn_usize current_chunk = 0;
    Dqn_ListChunk<T> **chunk = nullptr;
    if (forward_scan_dist <= backward_scan_dist) {
        for (chunk = &list->head; *chunk && current_chunk != desired_chunk; chunk = &((*chunk)->next), current_chunk++) {
        }
    } else {
        current_chunk = total_chunks;
        for (chunk = &list->tail; *chunk && current_chunk != desired_chunk; chunk = &((*chunk)->prev), current_chunk--) {
        }
    }

    T *result = nullptr;
    if (*chunk) {
        Dqn_usize relative_index = index % list->chunk_size;
        result                   = (*chunk)->data + relative_index;
        DQN_ASSERT(relative_index < (*chunk)->count);
    }

    if (result && at_chunk)
        *at_chunk = *chunk;

    return result;
}

template <typename T> Dqn_Slice<T> Dqn_List_ToSliceCopy(Dqn_List<T> const *list, Dqn_Arena *arena)
{
    // TODO(doyle): Chunk memcopies is much faster
    Dqn_Slice<T> result = Dqn_Slice_Alloc<T>(arena, list->count, Dqn_ZeroMem_No);
    if (result.size) {
        Dqn_usize slice_index = 0;
        DQN_MSVC_WARNING_PUSH
        DQN_MSVC_WARNING_DISABLE(6011) // Dereferencing NULL pointer 'x'
        for (Dqn_ListIterator<T> it = {}; Dqn_List_Iterate<T>(DQN_CAST(Dqn_List<T> *)list, &it, 0);)
            result.data[slice_index++] = *it.data;
        DQN_MSVC_WARNING_POP
        DQN_ASSERT(slice_index == result.size);
    }
    return result;
}
#endif // !defined(DQN_NO_LIST)
