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

DN_API int               DN_CVT_FmtBuffer3DotTruncate                 (char *buffer, int size, DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_CVTU64Str8     DN_CVT_U64ToStr8                             (DN_U64 val, char separator);
DN_API DN_CVTU64Bytes    DN_CVT_U64ToBytes                            (DN_U64 bytes, DN_CVTBytesType type);
#define                  DN_CVT_U64ToBytesAuto(bytes)                 DN_CVT_U64ToBytes(bytes, DN_CVTBytesType_Auto)
DN_API DN_Str8           DN_CVT_U64ToBytesStr8                        (DN_Arena *arena, DN_U64 bytes, DN_CVTBytesType type);
#define                  DN_CVT_U64ToBytesStr8Auto(arena, bytes)      DN_CVT_U64ToBytesStr8(arena,                 bytes, DN_CVTBytesType_Auto)
#define                  DN_CVT_U64ToBytesStr8FromTLS(bytes, type)    DN_CVT_U64ToBytesStr8(DN_OS_TLSTopArena(),   bytes, type)
#define                  DN_CVT_U64ToBytesStr8AutoFromTLS(bytes)      DN_CVT_U64ToBytesStr8(DN_OS_TLSTopArena(),   bytes, DN_CVTBytesType_Auto)
#define                  DN_CVT_U64ToBytesStr8FromFrame(bytes, type)  DN_CVT_U64ToBytesStr8(DN_OS_TLSFrameArena(), bytes, type)
#define                  DN_CVT_U64ToBytesStr8AutoFromFrame(bytes)    DN_CVT_U64ToBytesStr8(DN_OS_TLSFrameArena(), bytes, DN_CVTBytesType_Auto)
DN_API DN_Str8           DN_CVT_BytesTypeToStr8                       (DN_CVTBytesType type);
DN_API DN_Str8           DN_CVT_U64ToAge                              (DN_Arena *arena, DN_U64 age_s, DN_CVTU64AgeUnit unit);
DN_API DN_Str8           DN_CVT_F64ToAge                              (DN_Arena *arena, DN_F64 age_s, DN_CVTU64AgeUnit unit);

DN_API DN_U64            DN_CVT_HexToU64                              (DN_Str8 hex);
DN_API DN_Str8           DN_CVT_U64ToHex                              (DN_Arena *arena, DN_U64 number, DN_CVTU64HexStrFlags flags);
#define                  DN_CVT_U64ToHexFromFrame(number, flags)      DN_CVT_U64ToHex(DN_OS_TLSFrameArena(), number, flags)
DN_API DN_CVTU64HexStr   DN_CVT_U64ToHexStr                           (DN_U64 number, DN_CVTU64HexStrFlags flags);

DN_API bool              DN_CVT_BytesToHexPtr                         (void const *src, DN_USize src_size, char *dest, DN_USize dest_size);
DN_API DN_Str8           DN_CVT_BytesToHex                            (DN_Arena *arena, void const *src, DN_USize size);
#define                  DN_CVT_BytesToHexFromTLS(...)                DN_CVT_BytesToHex(DN_OS_TLSTopArena(), __VA_ARGS__)
#define                  DN_CVT_BytesToHexFromFrame(...)              DN_CVT_BytesToHex(DN_OS_TLSFrameArena(), __VA_ARGS__)

DN_API DN_USize          DN_CVT_HexToBytesPtrUnchecked                (DN_Str8 hex, void *dest, DN_USize dest_size);
DN_API DN_USize          DN_CVT_HexToBytesPtr                         (DN_Str8 hex, void *dest, DN_USize dest_size);
DN_API DN_Str8           DN_CVT_HexToBytesUnchecked                   (DN_Arena *arena, DN_Str8 hex);
#define                  DN_CVT_HexToBytesUncheckedFromTLS(...)       DN_CVT_HexToBytesUnchecked(DN_OS_TLSTopArena(), __VA_ARGS__)
DN_API DN_Str8           DN_CVT_HexToBytes                            (DN_Arena *arena, DN_Str8 hex);
#define                  DN_CVT_HexToBytesFromFrame(...)              DN_CVT_HexToBytes(DN_OS_TLSFrameArena(), __VA_ARGS__)
#define                  DN_CVT_HexToBytesFromTLS(...)                DN_CVT_HexToBytes(DN_OS_TLSTopArena(), __VA_ARGS__)
#endif // defined(DN_BASE_CONVERT_H)
