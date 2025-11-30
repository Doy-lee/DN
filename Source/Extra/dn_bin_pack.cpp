#define DN_BIN_PACK_CPP

#if defined(_CLANGD)
  #include "dn_bin_pack.h"
#endif

DN_API void DN_BinPackU64(DN_BinPack *pack, DN_BinPackMode mode, DN_U64 *item)
{
  DN_U64 const VALUE_MASK   = 0b0111'1111;
  DN_U8 const  CONTINUE_BIT = 0b1000'0000;

  if (mode == DN_BinPackMode_Serialise) {
    DN_U64 it = *item;
    do {
      DN_U8 write_value = DN_Cast(DN_U8)(it & VALUE_MASK);
      it >>= 7;
      if (it)
        write_value |= CONTINUE_BIT;
      DN_Str8BuilderAppendBytesCopy(&pack->writer, &write_value, sizeof(write_value));
    } while (it);
  } else {
    *item              = 0;
    DN_USize bits_read = 0;
    for (DN_U8 src = CONTINUE_BIT; (src & CONTINUE_BIT) && bits_read < 64; bits_read += 7) {
      src              = pack->read.data[pack->read_index++];
      DN_U8 masked_src = src & VALUE_MASK;
      *item |= (DN_Cast(DN_U64) masked_src << bits_read);
    }
  }
}

DN_API void DN_BinPackVarInt_(DN_BinPack *pack, DN_BinPackMode mode, void *item, DN_USize size)
{
  DN_U64 value = 0;
  DN_AssertF(size <= sizeof(value),
             "An item larger than 64 bits (%zu) is trying to be packed as a variable integer which is not supported",
             size * 8);

  if (mode == DN_BinPackMode_Serialise) // Read `item` into U64 `value`
    DN_Memcpy(&value, item, size);

  DN_BinPackU64(pack, mode, &value);

  if (mode == DN_BinPackMode_Deserialise) // Write U64 `value` into `item`
    DN_Memcpy(item, &value, size);
}

DN_API bool DN_BinPackIsEndOfReadStream(DN_BinPack const *pack)
{
  bool result = pack->read_index == pack->read.size;
  return result;
}

DN_API void DN_BinPackUSize(DN_BinPack *pack, DN_BinPackMode mode, DN_USize *item)
{
  DN_BinPackVarInt_(pack, mode, item, sizeof(*item));
}

DN_API void DN_BinPackU32(DN_BinPack *pack, DN_BinPackMode mode, DN_U32 *item)
{
  DN_BinPackVarInt_(pack, mode, item, sizeof(*item));
}

DN_API void DN_BinPackU16(DN_BinPack *pack, DN_BinPackMode mode, DN_U16 *item)
{
  DN_BinPackVarInt_(pack, mode, item, sizeof(*item));
}

DN_API void DN_BinPackU8(DN_BinPack *pack, DN_BinPackMode mode, DN_U8 *item)
{
  DN_BinPackVarInt_(pack, mode, item, sizeof(*item));
}

DN_API void DN_BinPackI64(DN_BinPack *pack, DN_BinPackMode mode, DN_I64 *item)
{
  DN_BinPackVarInt_(pack, mode, item, sizeof(*item));
}

DN_API void DN_BinPackI32(DN_BinPack *pack, DN_BinPackMode mode, DN_I32 *item)
{
  DN_BinPackVarInt_(pack, mode, item, sizeof(*item));
}

DN_API void DN_BinPackI16(DN_BinPack *pack, DN_BinPackMode mode, DN_I16 *item)
{
  DN_BinPackVarInt_(pack, mode, item, sizeof(*item));
}

DN_API void DN_BinPackI8(DN_BinPack *pack, DN_BinPackMode mode, DN_I8 *item)
{
  DN_BinPackVarInt_(pack, mode, item, sizeof(*item));
}

DN_API void DN_BinPackF64(DN_BinPack *pack, DN_BinPackMode mode, DN_F64 *item)
{
  DN_BinPackVarInt_(pack, mode, item, sizeof(*item));
}

DN_API void DN_BinPackF32(DN_BinPack *pack, DN_BinPackMode mode, DN_F32 *item)
{
  DN_BinPackVarInt_(pack, mode, item, sizeof(*item));
}

#if defined(DN_MATH_H)
DN_API void DN_BinPackV2(DN_BinPack *pack, DN_BinPackMode mode, DN_V2F32 *item)
{
  DN_BinPackF32(pack, mode, &item->x);
  DN_BinPackF32(pack, mode, &item->y);
}

DN_API void DN_BinPackV4(DN_BinPack *pack, DN_BinPackMode mode, DN_V4F32 *item)
{
  DN_BinPackF32(pack, mode, &item->x);
  DN_BinPackF32(pack, mode, &item->y);
  DN_BinPackF32(pack, mode, &item->z);
  DN_BinPackF32(pack, mode, &item->w);
}
#endif

DN_API void DN_BinPackBool(DN_BinPack *pack, DN_BinPackMode mode, bool *item)
{
  DN_BinPackVarInt_(pack, mode, item, sizeof(*item));
}

DN_API void DN_BinPackStr8FromArena(DN_BinPack *pack, DN_Arena *arena, DN_BinPackMode mode, DN_Str8 *string)
{
  DN_BinPackVarInt_(pack, mode, &string->size, sizeof(string->size));
  if (mode == DN_BinPackMode_Serialise) {
    DN_Str8BuilderAppendBytesCopy(&pack->writer, string->data, string->size);
  } else {
    DN_Str8 src = DN_Str8Slice(pack->read, pack->read_index, string->size);
    *string     = DN_Str8FromStr8Arena(arena, src);
    pack->read_index += src.size;
  }
}

DN_API void DN_BinPackStr8FromPool(DN_BinPack *pack, DN_Pool *pool, DN_BinPackMode mode, DN_Str8 *string)
{
  DN_BinPackVarInt_(pack, mode, &string->size, sizeof(string->size));
  if (mode == DN_BinPackMode_Serialise) {
    DN_Str8BuilderAppendBytesCopy(&pack->writer, string->data, string->size);
  } else {
    DN_Str8 src = DN_Str8Slice(pack->read, pack->read_index, string->size);
    *string     = DN_Str8FromStr8Pool(pool, src);
    pack->read_index += src.size;
  }
}

DN_API DN_Str8 DN_BinPackStr8FromBuffer(DN_BinPack *pack, DN_BinPackMode mode, char *ptr, DN_USize *size, DN_USize max)
{
  DN_BinPackCBuffer(pack, mode, ptr, size, max);
  DN_Str8 result = DN_Str8FromPtr(ptr, *size);
  return result;
}

DN_API void DN_BinPackBytesFromArena(DN_BinPack *pack, DN_Arena *arena, DN_BinPackMode mode, void **ptr, DN_USize *size)
{
  DN_Str8 string = DN_Str8FromPtr(*ptr, *size);
  DN_BinPackStr8FromArena(pack, arena, mode, &string);
  *ptr  = string.data;
  *size = string.size;
}

DN_API void DN_BinPackBytesFromPool(DN_BinPack *pack, DN_Pool *pool, DN_BinPackMode mode, void **ptr, DN_USize *size)
{
  DN_Str8 string = DN_Str8FromPtr(*ptr, *size);
  DN_BinPackStr8FromPool(pack, pool, mode, &string);
  *ptr  = string.data;
  *size = string.size;
}

DN_API void DN_BinPackCArray(DN_BinPack *pack, DN_BinPackMode mode, void *ptr, DN_USize size)
{
  DN_BinPackVarInt_(pack, mode, &size, sizeof(size));
  if (mode == DN_BinPackMode_Serialise) {
    DN_Str8BuilderAppendBytesCopy(&pack->writer, ptr, size);
  } else {
    DN_Str8 src = DN_Str8Slice(pack->read, pack->read_index, size);
    DN_Assert(src.size == size);
    DN_Memcpy(ptr, src.data, DN_Min(src.size, size));
    pack->read_index += src.size;
  }
}

DN_API void DN_BinPackCBuffer(DN_BinPack *pack, DN_BinPackMode mode, char *ptr, DN_USize *size, DN_USize max)
{
  if (mode == DN_BinPackMode_Serialise) {
    DN_BinPackUSize(pack, mode, size);
    DN_Str8BuilderAppendBytesCopy(&pack->writer, ptr, *size);
  } else {
    DN_U64 size_u64 = 0;
    DN_BinPackU64(pack, mode, &size_u64);
    DN_Assert(size_u64 < DN_USIZE_MAX);
    DN_Assert(size_u64 <= max);

    *size = DN_Min(size_u64, max);
    DN_Memcpy(ptr, pack->read.data + pack->read_index, *size);
    pack->read_index += size_u64;
  }
}

DN_API DN_Str8 DN_BinPackBuild(DN_BinPack const *pack, DN_Arena *arena)
{
  DN_Str8 result = DN_Str8BuilderBuild(&pack->writer, arena);
  return result;
}
