#if !defined(DN_BIN_PACK_H)
#define DN_BIN_PACK_H

#if defined(_CLANGD)
  #include "../dn_base_inc.h"
#endif

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

DN_API bool    DN_BinPackIsEndOfReadStream(DN_BinPack const *pack);
DN_API void    DN_BinPackUSize            (DN_BinPack *pack, DN_BinPackMode mode, DN_USize *item);
DN_API void    DN_BinPackU64              (DN_BinPack *pack, DN_BinPackMode mode, DN_U64 *item);
DN_API void    DN_BinPackU32              (DN_BinPack *pack, DN_BinPackMode mode, DN_U32 *item);
DN_API void    DN_BinPackU16              (DN_BinPack *pack, DN_BinPackMode mode, DN_U16 *item);
DN_API void    DN_BinPackU8               (DN_BinPack *pack, DN_BinPackMode mode, DN_U8 *item);
DN_API void    DN_BinPackI64              (DN_BinPack *pack, DN_BinPackMode mode, DN_I64 *item);
DN_API void    DN_BinPackI32              (DN_BinPack *pack, DN_BinPackMode mode, DN_I32 *item);
DN_API void    DN_BinPackI16              (DN_BinPack *pack, DN_BinPackMode mode, DN_I16 *item);
DN_API void    DN_BinPackI8               (DN_BinPack *pack, DN_BinPackMode mode, DN_I8 *item);
DN_API void    DN_BinPackF64              (DN_BinPack *pack, DN_BinPackMode mode, DN_F64 *item);
DN_API void    DN_BinPackF32              (DN_BinPack *pack, DN_BinPackMode mode, DN_F32 *item);
#if defined                               (DN_MATH_H)
DN_API void    DN_BinPackV2               (DN_BinPack *pack, DN_BinPackMode mode, DN_V2F32 *item);
DN_API void    DN_BinPackV4               (DN_BinPack *pack, DN_BinPackMode mode, DN_V4F32 *item);
#endif
DN_API void    DN_BinPackBool             (DN_BinPack *pack, DN_BinPackMode mode, bool *item);
DN_API void    DN_BinPackStr8FromArena    (DN_BinPack *pack, DN_Arena *arena, DN_BinPackMode mode, DN_Str8 *string);
DN_API void    DN_BinPackStr8FromPool     (DN_BinPack *pack, DN_Pool *pool, DN_BinPackMode mode, DN_Str8 *string);
DN_API DN_Str8 DN_BinPackStr8FromBuffer   (DN_BinPack *pack, DN_BinPackMode mode, char *ptr, DN_USize *size, DN_USize max);
DN_API void    DN_BinPackBytesFromArena   (DN_BinPack *pack, DN_Arena *arena, DN_BinPackMode mode, void **ptr, DN_USize *size);
DN_API void    DN_BinPackBytesFromPool    (DN_BinPack *pack, DN_Pool *pool, DN_BinPackMode mode, void **ptr, DN_USize *size);
DN_API void    DN_BinPackCArray           (DN_BinPack *pack, DN_BinPackMode mode, void *ptr, DN_USize size);
DN_API void    DN_BinPackCBuffer          (DN_BinPack *pack, DN_BinPackMode mode, char *ptr, DN_USize *size, DN_USize max);
DN_API DN_Str8 DN_BinPackBuild            (DN_BinPack const *pack, DN_Arena *arena);

#endif // !defined(DN_BIN_PACK_H)
