DN_API void DN_BinPack_U64(DN_BinPack *pack, DN_BinPackMode mode, DN_U64 *item)
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

DN_API void DN_BinPack_VarInt_(DN_BinPack *pack, DN_BinPackMode mode, void *item, DN_USize size)
{
  DN_U64 value = 0;
  DN_AssertF(size <= sizeof(value),
             "An item larger than 64 bits (%zu) is trying to be packed as a variable integer which is not supported",
             size * 8);

  if (mode == DN_BinPackMode_Serialise) // Read `item` into U64 `value`
    DN_Memcpy(&value, item, size);

  DN_BinPack_U64(pack, mode, &value);

  if (mode == DN_BinPackMode_Deserialise) // Write U64 `value` into `item`
    DN_Memcpy(item, &value, size);
}

DN_API void DN_BinPack_U32(DN_BinPack *pack, DN_BinPackMode mode, DN_U32 *item)
{
  DN_BinPack_VarInt_(pack, mode, item, sizeof(*item));
}

DN_API void DN_BinPack_U16(DN_BinPack *pack, DN_BinPackMode mode, DN_U16 *item)
{
  DN_BinPack_VarInt_(pack, mode, item, sizeof(*item));
}

DN_API void DN_BinPack_U8(DN_BinPack *pack, DN_BinPackMode mode, DN_U8 *item)
{
  DN_BinPack_VarInt_(pack, mode, item, sizeof(*item));
}

DN_API void DN_BinPack_I64(DN_BinPack *pack, DN_BinPackMode mode, DN_I64 *item)
{
  DN_BinPack_VarInt_(pack, mode, item, sizeof(*item));
}

DN_API void DN_BinPack_I32(DN_BinPack *pack, DN_BinPackMode mode, DN_I32 *item)
{
  DN_BinPack_VarInt_(pack, mode, item, sizeof(*item));
}

DN_API void DN_BinPack_I16(DN_BinPack *pack, DN_BinPackMode mode, DN_I16 *item)
{
  DN_BinPack_VarInt_(pack, mode, item, sizeof(*item));
}

DN_API void DN_BinPack_I8(DN_BinPack *pack, DN_BinPackMode mode, DN_I8 *item)
{
  DN_BinPack_VarInt_(pack, mode, item, sizeof(*item));
}

DN_API void DN_BinPack_F64(DN_BinPack *pack, DN_BinPackMode mode, DN_F64 *item)
{
  DN_BinPack_VarInt_(pack, mode, item, sizeof(*item));
}

DN_API void DN_BinPack_F32(DN_BinPack *pack, DN_BinPackMode mode, DN_F32 *item)
{
  DN_BinPack_VarInt_(pack, mode, item, sizeof(*item));
}

#if defined(DN_MATH_H)
DN_API void DN_BinPack_V2(DN_BinPack *pack, DN_BinPackMode mode, DN_V2F32 *item)
{
  DN_BinPack_F32(pack, mode, &item->x);
  DN_BinPack_F32(pack, mode, &item->y);
}

DN_API void DN_BinPack_V4(DN_BinPack *pack, DN_BinPackMode mode, DN_V4F32 *item)
{
  DN_BinPack_F32(pack, mode, &item->x);
  DN_BinPack_F32(pack, mode, &item->y);
  DN_BinPack_F32(pack, mode, &item->z);
  DN_BinPack_F32(pack, mode, &item->w);
}
#endif

DN_API void DN_BinPack_Bool(DN_BinPack *pack, DN_BinPackMode mode, bool *item)
{
  DN_BinPack_VarInt_(pack, mode, item, sizeof(*item));
}

DN_API void DN_BinPack_Str8FromArena(DN_BinPack *pack, DN_Arena *arena, DN_BinPackMode mode, DN_Str8 *string)
{
  DN_BinPack_VarInt_(pack, mode, &string->size, sizeof(string->size));
  if (mode == DN_BinPackMode_Serialise) {
    DN_Str8BuilderAppendBytesCopy(&pack->writer, string->data, string->size);
  } else {
    DN_Str8 src = DN_Str8Slice(pack->read, pack->read_index, string->size);
    *string     = DN_Str8FromStr8Arena(arena, src);
    pack->read_index += src.size;
  }
}

DN_API void DN_BinPack_Str8FromPool(DN_BinPack *pack, DN_Pool *pool, DN_BinPackMode mode, DN_Str8 *string)
{
  DN_BinPack_VarInt_(pack, mode, &string->size, sizeof(string->size));
  if (mode == DN_BinPackMode_Serialise) {
    DN_Str8BuilderAppendBytesCopy(&pack->writer, string->data, string->size);
  } else {
    DN_Str8 src = DN_Str8Slice(pack->read, pack->read_index, string->size);
    *string     = DN_Str8FromStr8Pool(pool, src);
    pack->read_index += src.size;
  }
}

DN_API void DN_BinPack_BytesFromArena(DN_BinPack *pack, DN_Arena *arena, DN_BinPackMode mode, void **ptr, DN_USize *size)
{
  DN_Str8 string = DN_Str8FromPtr(*ptr, *size);
  DN_BinPack_Str8FromArena(pack, arena, mode, &string);
  *ptr  = string.data;
  *size = string.size;
}

DN_API void DN_BinPack_BytesFromPool(DN_BinPack *pack, DN_Pool *pool, DN_BinPackMode mode, void **ptr, DN_USize *size)
{
  DN_Str8 string = DN_Str8FromPtr(*ptr, *size);
  DN_BinPack_Str8FromPool(pack, pool, mode, &string);
  *ptr  = string.data;
  *size = string.size;
}

DN_API void DN_BinPack_CArray(DN_BinPack *pack, DN_BinPackMode mode, void *ptr, DN_USize size)
{
  DN_BinPack_VarInt_(pack, mode, &size, sizeof(size));
  if (mode == DN_BinPackMode_Serialise) {
    DN_Str8BuilderAppendBytesCopy(&pack->writer, ptr, size);
  } else {
    DN_Str8 src = DN_Str8Slice(pack->read, pack->read_index, size);
    DN_Assert(src.size == size);
    DN_Memcpy(ptr, src.data, DN_Min(src.size, size));
    pack->read_index += src.size;
  }
}

DN_API DN_Str8 DN_BinPack_Build(DN_BinPack const *pack, DN_Arena *arena)
{
  DN_Str8 result = DN_Str8BuilderBuild(&pack->writer, arena);
  return result;
}
