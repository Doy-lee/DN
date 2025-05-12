#if !defined(DN_BIN_PACK_H)
#define DN_BIN_PACK_H

#if !defined(DN_BASE_INC_H)
  #error dn_base_inc.h must be included before this
#endif

enum DN_BinPackMode
{
  DN_BinPackMode_Serialise,
  DN_BinPackMode_Deserialise,
};

struct DN_BinPack
{
  DN_Str8Builder writer;
  DN_Str8        read;
  DN_USize       read_index;
};

DN_API                       void    DN_BinPack_U64     (DN_BinPack *pack,       DN_BinPackMode mode, DN_U64 *item);
DN_API                       void    DN_BinPack_U32     (DN_BinPack *pack,       DN_BinPackMode mode, DN_U32 *item);
DN_API                       void    DN_BinPack_U16     (DN_BinPack *pack,       DN_BinPackMode mode, DN_U16 *item);
DN_API                       void    DN_BinPack_U8      (DN_BinPack *pack,       DN_BinPackMode mode, DN_U8 *item);
DN_API                       void    DN_BinPack_I64     (DN_BinPack *pack,       DN_BinPackMode mode, DN_I64 *item);
DN_API                       void    DN_BinPack_I32     (DN_BinPack *pack,       DN_BinPackMode mode, DN_I32 *item);
DN_API                       void    DN_BinPack_I16     (DN_BinPack *pack,       DN_BinPackMode mode, DN_I16 *item);
DN_API                       void    DN_BinPack_I8      (DN_BinPack *pack,       DN_BinPackMode mode, DN_I8 *item);
DN_API                       void    DN_BinPack_F64     (DN_BinPack *pack,       DN_BinPackMode mode, DN_F64 *item);
DN_API                       void    DN_BinPack_F32     (DN_BinPack *pack,       DN_BinPackMode mode, DN_F32 *item);
#if defined(DN_MATH_H)
DN_API                       void    DN_BinPack_V2      (DN_BinPack *pack,       DN_BinPackMode mode, DN_V2F32 *item);
DN_API                       void    DN_BinPack_V4      (DN_BinPack *pack,       DN_BinPackMode mode, DN_V4F32 *item);
#endif
DN_API                       void    DN_BinPack_Bool    (DN_BinPack *pack,       DN_BinPackMode mode, bool *item);
DN_API                       void    DN_BinPack_Str8    (DN_BinPack *pack,       DN_Arena *arena, DN_BinPackMode mode, DN_Str8 *string);
DN_API                       void    DN_BinPack_Str8Pool(DN_BinPack *pack,       DN_Pool *pool, DN_BinPackMode mode, DN_Str8 *string);
template <DN_USize N> DN_API void    DN_BinPack_FStr8   (DN_BinPack *pack,       DN_BinPackMode mode, DN_FStr8<N> *string);
DN_API                       void    DN_BinPack_Bytes   (DN_BinPack *pack,       DN_Arena *arena, DN_BinPackMode mode, void **ptr, DN_USize *size);
DN_API                       void    DN_BinPack_CArray  (DN_BinPack *pack,       DN_BinPackMode mode, void *ptr, DN_USize size);
DN_API                       DN_Str8 DN_BinPack_Build   (DN_BinPack const *pack, DN_Arena *arena);

#endif // !defined(DN_BIN_PACK_H)
