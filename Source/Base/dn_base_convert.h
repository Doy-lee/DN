#if !defined(DN_BASE_CONVERT_H)
#define DN_BASE_CONVERT_H

#include "../dn_base_inc.h"

struct DN_CVTU64Str8
{
  char  data[27 + 1]; // NOTE(dn): 27 is the maximum size of DN_U64 including a separator
  DN_U8 size;
};

enum DN_CVTBytesType
{
  DN_CVTBytesType_B,
  DN_CVTBytesType_KiB,
  DN_CVTBytesType_MiB,
  DN_CVTBytesType_GiB,
  DN_CVTBytesType_TiB,
  DN_CVTBytesType_Count,
  DN_CVTBytesType_Auto,
};

struct DN_CVTU64Bytes
{
  DN_CVTBytesType type;
  DN_Str8         suffix; // "KiB", "MiB", "GiB" .. e.t.c
  DN_F64          bytes;
};

struct DN_CVTU64HexStr
{
  char  data[2 /*0x*/ + 16 /*hex*/ + 1 /*null-terminator*/];
  DN_U8 size;
};

typedef DN_U32 DN_CVTU64HexStrFlags;
enum DN_CVTU64HexStrFlags_
{
  DN_CVTU64HexStrFlags_Nil          = 0,
  DN_CVTU64HexStrFlags_0xPrefix     = 1 << 0, /// Add the '0x' prefix from the string
  DN_CVTU64HexStrFlags_UppercaseHex = 1 << 1, /// Use uppercase ascii characters for hex
};

typedef DN_U32 DN_CVTU64AgeUnit;
enum DN_CVTU64AgeUnit_
{
  DN_CVTU64AgeUnit_Sec  = 1 << 0,
  DN_CVTU64AgeUnit_Min  = 1 << 1,
  DN_CVTU64AgeUnit_Hr   = 1 << 2,
  DN_CVTU64AgeUnit_Day  = 1 << 3,
  DN_CVTU64AgeUnit_Week = 1 << 4,
  DN_CVTU64AgeUnit_Year = 1 << 5,
  DN_CVTU64AgeUnit_HMS  = DN_CVTU64AgeUnit_Sec | DN_CVTU64AgeUnit_Min | DN_CVTU64AgeUnit_Hr,
  DN_CVTU64AgeUnit_All  = DN_CVTU64AgeUnit_HMS | DN_CVTU64AgeUnit_Day | DN_CVTU64AgeUnit_Week | DN_CVTU64AgeUnit_Year,
};

struct DN_NibbleFromU8Result
{
  char nibble0;
  char nibble1;
};

DN_API DN_NibbleFromU8Result DN_CVT_NibbleFromU8                          (DN_U8 u8);
DN_API DN_U8                 DN_CVT_U8FromHexNibble                       (char hex);

DN_API int                   DN_CVT_FmtBuffer3DotTruncate                 (char *buffer, int size, DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_CVTU64Str8         DN_CVT_Str8FromU64                           (DN_U64 val, char separator);
DN_API DN_CVTU64Bytes        DN_CVT_BytesFromU64                          (DN_U64 bytes, DN_CVTBytesType type);
#define                      DN_CVT_BytesFromU64Auto(bytes)               DN_CVT_BytesFromU64(bytes, DN_CVTBytesType_Auto)
DN_API DN_Str8               DN_CVT_BytesStr8FromU64                      (DN_Arena *arena, DN_U64 bytes, DN_CVTBytesType type);
#define                      DN_CVT_BytesStr8FromTLS(bytes, type)         DN_CVT_BytesStr8FromU64(DN_OS_TLSTopArena(),   bytes, type)
#define                      DN_CVT_BytesStr8FromU64Auto(arena, bytes)    DN_CVT_BytesStr8FromU64(arena,                 bytes, DN_CVTBytesType_Auto)
#define                      DN_CVT_BytesStr8FromU64AutoTLS(bytes)        DN_CVT_BytesStr8FromU64(DN_OS_TLSTopArena(),   bytes, DN_CVTBytesType_Auto)
#define                      DN_CVT_BytesStr8FromU64Frame(bytes, type)    DN_CVT_BytesStr8FromU64(DN_OS_TLSFrameArena(), bytes, type)
#define                      DN_CVT_BytesStr8FromU64AutoFrame(bytes)      DN_CVT_BytesStr8FromU64(DN_OS_TLSFrameArena(), bytes, DN_CVTBytesType_Auto)
DN_API DN_Str8               DN_CVT_BytesTypeToStr8                       (DN_CVTBytesType type);
DN_API DN_Str8               DN_CVT_AgeFromU64                            (DN_Arena *arena, DN_U64 age_s, DN_CVTU64AgeUnit unit);
DN_API DN_Str8               DN_CVT_AgeFromF64                            (DN_Arena *arena, DN_F64 age_s, DN_CVTU64AgeUnit unit);

DN_API DN_U64                DN_CVT_U64FromHex                            (DN_Str8 hex);
DN_API DN_Str8               DN_CVT_HexFromU64                            (DN_Arena *arena, DN_U64 number, DN_CVTU64HexStrFlags flags);
#define                      DN_CVT_HexFromU64Frame(number, flags)        DN_CVT_HexFromU64(DN_OS_TLSFrameArena(), number, flags)
DN_API DN_CVTU64HexStr       DN_CVT_HexFromU64Str                         (DN_U64 number, DN_CVTU64HexStrFlags flags);

DN_API bool                  DN_CVT_HexFromBytesPtr                       (void const *src, DN_USize src_size, char *dest, DN_USize dest_size);
DN_API DN_Str8               DN_CVT_HexFromBytes                          (DN_Arena *arena, void const *src, DN_USize size);
#define                      DN_CVT_HexFromBytesTLS(...)                  DN_CVT_HexFromBytes(DN_OS_TLSTopArena(), __VA_ARGS__)
#define                      DN_CVT_HexFromBytesFrame(...)                DN_CVT_HexFromBytes(DN_OS_TLSFrameArena(), __VA_ARGS__)

DN_API DN_USize              DN_CVT_BytesFromHexPtrUnchecked              (DN_Str8 hex, void *dest, DN_USize dest_size);
DN_API DN_USize              DN_CVT_BytesFromHexPtr                       (DN_Str8 hex, void *dest, DN_USize dest_size);
DN_API DN_Str8               DN_CVT_BytesFromHexUnchecked                 (DN_Arena *arena, DN_Str8 hex);
#define                      DN_CVT_BytesFromHexUncheckedFromTLS(...)     DN_CVT_BytesFromHexUnchecked(DN_OS_TLSTopArena(), __VA_ARGS__)
DN_API DN_Str8               DN_CVT_BytesFromHex                          (DN_Arena *arena, DN_Str8 hex);
#define                      DN_CVT_BytesFromHexFrame(...)                DN_CVT_BytesFromHex(DN_OS_TLSFrameArena(), __VA_ARGS__)
#define                      DN_CVT_BytesFromHexTLS(...)                  DN_CVT_BytesFromHex(DN_OS_TLSTopArena(), __VA_ARGS__)
#endif // defined(DN_BASE_CONVERT_H)
