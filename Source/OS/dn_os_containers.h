#if !defined(DN_OS_CONTAINERS_H)
#define DN_OS_CONTAINERS_H

// NOTE: DN_VArray /////////////////////////////////////////////////////////////////////////////////
// TODO(doyle): Add an API for shrinking the array by decomitting pages back to the OS.
template <typename T> struct DN_VArray
{
  T       *data;   // Pointer to the start of the array items in the block of memory
  DN_USize size;   // Number of items currently in the array
  DN_USize max;    // Maximum number of items this array can store
  DN_USize commit; // Bytes committed

  T       *begin()       { return data; }
  T       *end  ()       { return data + size; }
  T const *begin() const { return data; }
  T const *end  () const { return data + size; }
};

template <typename T>                           DN_VArray<T>          DN_VArray_InitByteSize            (DN_USize byte_size);
template <typename T>                           DN_VArray<T>          DN_VArray_Init                    (DN_USize max);
template <typename T>                           DN_VArray<T>          DN_VArray_InitSlice               (DN_Slice<T> slice, DN_USize max);
template <typename T, DN_USize N>               DN_VArray<T>          DN_VArray_InitCArray              (T const (&items)[N], DN_USize max);
template <typename T>                           void                  DN_VArray_Deinit                  (DN_VArray<T> *array);
template <typename T>                           bool                  DN_VArray_IsValid                 (DN_VArray<T> const *array);
template <typename T>                           DN_Slice<T>           DN_VArray_Slice                   (DN_VArray<T> const *array);
template <typename T>                           bool                  DN_VArray_Reserve                 (DN_VArray<T> *array, DN_USize count);
template <typename T>                           T *                   DN_VArray_AddArray                (DN_VArray<T> *array, T const *items, DN_USize count);
template <typename T, DN_USize N>               T *                   DN_VArray_AddCArray               (DN_VArray<T> *array, T const (&items)[N]);
template <typename T>                           T *                   DN_VArray_Add                     (DN_VArray<T> *array, T const &item);
#define                                                               DN_VArray_AddArrayAssert(...)     DN_HardAssert(DN_VArray_AddArray(__VA_ARGS__))
#define                                                               DN_VArray_AddCArrayAssert(...)    DN_HardAssert(DN_VArray_AddCArray(__VA_ARGS__))
#define                                                               DN_VArray_AddAssert(...)          DN_HardAssert(DN_VArray_Add(__VA_ARGS__))
template <typename T>                           T *                   DN_VArray_MakeArray               (DN_VArray<T> *array, DN_USize count, DN_ZeroMem zero_mem);
template <typename T>                           T *                   DN_VArray_Make                    (DN_VArray<T> *array, DN_ZeroMem zero_mem);
#define                                                               DN_VArray_MakeArrayAssert(...)    DN_HardAssert(DN_VArray_MakeArray(__VA_ARGS__))
#define                                                               DN_VArray_MakeAssert(...)         DN_HardAssert(DN_VArray_Make(__VA_ARGS__))
template <typename T>                           T *                   DN_VArray_InsertArray             (DN_VArray<T> *array, DN_USize index, T const *items, DN_USize count);
template <typename T, DN_USize N>               T *                   DN_VArray_InsertCArray            (DN_VArray<T> *array, DN_USize index, T const (&items)[N]);
template <typename T>                           T *                   DN_VArray_Insert                  (DN_VArray<T> *array, DN_USize index, T const &item);
#define                                                               DN_VArray_InsertArrayAssert(...)  DN_HardAssert(DN_VArray_InsertArray(__VA_ARGS__))
#define                                                               DN_VArray_InsertCArrayAssert(...) DN_HardAssert(DN_VArray_InsertCArray(__VA_ARGS__))
#define                                                               DN_VArray_InsertAssert(...)       DN_HardAssert(DN_VArray_Insert(__VA_ARGS__))
template <typename T>                           T                     DN_VArray_PopFront                (DN_VArray<T> *array, DN_USize count);
template <typename T>                           T                     DN_VArray_PopBack                 (DN_VArray<T> *array, DN_USize count);
template <typename T>                           DN_ArrayEraseResult   DN_VArray_EraseRange              (DN_VArray<T> *array, DN_USize begin_index, DN_ISize count, DN_ArrayErase erase);
template <typename T>                           void                  DN_VArray_Clear                   (DN_VArray<T> *array, DN_ZeroMem zero_mem);
#endif // !defined(DN_OS_CONTAINERS_H)
