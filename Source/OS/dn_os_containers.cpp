#define DN_OS_CONTAINERS_CPP

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
//   dn_containers.cpp
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

// NOTE: DN_VArray /////////////////////////////////////////////////////////////////////////////////
template <typename T>
DN_VArray<T> DN_VArray_InitByteSize(DN_USize byte_size)
{
  DN_VArray<T> result = {};
  result.data         = DN_Cast(T *) DN_OS_MemReserve(byte_size, DN_MemCommit_No, DN_MemPage_ReadWrite);
  if (result.data)
    result.max = byte_size / sizeof(T);
  return result;
}

template <typename T>
DN_VArray<T> DN_VArray_Init(DN_USize max)
{
  DN_VArray<T> result = DN_VArray_InitByteSize<T>(max * sizeof(T));
  DN_Assert(result.max >= max);
  return result;
}

template <typename T>
DN_VArray<T> DN_VArray_InitSlice(DN_Slice<T> slice, DN_USize max)
{
  DN_USize     real_max = DN_Max(slice.size, max);
  DN_VArray<T> result   = DN_VArray_Init<T>(real_max);
  if (DN_VArray_IsValid(&result))
    DN_VArray_AddArray(&result, slice.data, slice.size);
  return result;
}

template <typename T, DN_USize N>
DN_VArray<T> DN_VArray_InitCArray(T const (&items)[N], DN_USize max)
{
  DN_USize     real_max = DN_Max(N, max);
  DN_VArray<T> result   = DN_VArray_InitSlice(DN_Slice_Init(items, N), real_max);
  return result;
}

template <typename T>
void DN_VArray_Deinit(DN_VArray<T> *array)
{
  DN_OS_MemRelease(array->data, array->max * sizeof(T));
  *array = {};
}

template <typename T>
bool DN_VArray_IsValid(DN_VArray<T> const *array)
{
  bool result = array->data && array->size <= array->max;
  return result;
}

template <typename T>
DN_Slice<T> DN_VArray_Slice(DN_VArray<T> const *array)
{
  DN_Slice<T> result = {};
  if (array)
    result = DN_Slice_Init<T>(array->data, array->size);
  return result;
}

template <typename T>
T *DN_VArray_AddArray(DN_VArray<T> *array, T const *items, DN_USize count)
{
  T *result = DN_VArray_MakeArray(array, count, DN_ZMem_No);
  if (result)
    DN_Memcpy(result, items, count * sizeof(T));
  return result;
}

template <typename T, DN_USize N>
T *DN_VArray_AddCArray(DN_VArray<T> *array, T const (&items)[N])
{
  T *result = DN_VArray_AddArray(array, items, N);
  return result;
}

template <typename T>
T *DN_VArray_Add(DN_VArray<T> *array, T const &item)
{
  T *result = DN_VArray_AddArray(array, &item, 1);
  return result;
}

template <typename T>
T *DN_VArray_MakeArray(DN_VArray<T> *array, DN_USize count, DN_ZMem z_mem)
{
  if (!DN_VArray_IsValid(array))
    return nullptr;

  if (!DN_CheckF((array->size + count) < array->max, "Array is out of space (user requested +%zu items, array has %zu/%zu items)", count, array->size, array->max))
    return nullptr;

  if (!DN_VArray_Reserve(array, count))
    return nullptr;

  // TODO: Use placement new
  T *result = array->data + array->size;
  array->size += count;
  if (z_mem == DN_ZMem_Yes)
    DN_Memset(result, 0, count * sizeof(T));
  return result;
}

template <typename T>
T *DN_VArray_Make(DN_VArray<T> *array, DN_ZMem z_mem)
{
  T *result = DN_VArray_MakeArray(array, 1, z_mem);
  return result;
}

template <typename T>
T *DN_VArray_InsertArray(DN_VArray<T> *array, DN_USize index, T const *items, DN_USize count)
{
  T *result = nullptr;
  if (!DN_VArray_IsValid(array))
    return result;
  if (DN_VArray_Reserve(array, array->size + count))
    result = DN_CArray_InsertArray(array->data, &array->size, array->max, index, items, count);
  return result;
}

template <typename T, DN_USize N>
T *DN_VArray_InsertCArray(DN_VArray<T> *array, DN_USize index, T const (&items)[N])
{
  T *result = DN_VArray_InsertArray(array, index, items, N);
  return result;
}

template <typename T>
T *DN_VArray_Insert(DN_VArray<T> *array, DN_USize index, T const &item)
{
  T *result = DN_VArray_InsertArray(array, index, &item, 1);
  return result;
}

template <typename T>
T *DN_VArray_PopFront(DN_VArray<T> *array, DN_USize count)
{
  T *result = DN_CArray_PopFront(array->data, &array->size, count);
  return result;
}

template <typename T>
T *DN_VArray_PopBack(DN_VArray<T> *array, DN_USize count)
{
  T *result = DN_CArray_PopBack(array->data, &array->size, count);
  return result;
}

template <typename T>
DN_ArrayEraseResult DN_VArray_EraseRange(DN_VArray<T> *array, DN_USize begin_index, DN_ISize count, DN_ArrayErase erase)
{
  DN_ArrayEraseResult result = {};
  if (!DN_VArray_IsValid(array))
    return result;
  result = DN_CArray_EraseRange<T>(array->data, &array->size, begin_index, count, erase);
  return result;
}

template <typename T>
void DN_VArray_Clear(DN_VArray<T> *array, DN_ZMem z_mem)
{
  if (array) {
    if (z_mem == DN_ZMem_Yes)
      DN_Memset(array->data, 0, array->size * sizeof(T));
    array->size = 0;
  }
}

template <typename T>
bool DN_VArray_Reserve(DN_VArray<T> *array, DN_USize count)
{
  if (!DN_VArray_IsValid(array) || count == 0)
    return false;

  DN_USize real_commit    = (array->size + count) * sizeof(T);
  DN_USize aligned_commit = DN_AlignUpPowerOfTwo(real_commit, g_dn_->os.page_size);
  if (array->commit >= aligned_commit)
    return true;
  bool result   = DN_OS_MemCommit(array->data, aligned_commit, DN_MemPage_ReadWrite);
  array->commit = aligned_commit;
  return result;
}
