#define DN_CONVERT_CPP

#include "../dn_base_inc.h"

DN_API DN_NibbleFromU8Result DN_CVT_NibbleFromU8(DN_U8 u8)
{
  static char const    *table  = "0123456789abcdef";
  DN_U8                 lhs    = (u8 >> 0) & 0xF;
  DN_U8                 rhs    = (u8 >> 4) & 0xF;
  DN_NibbleFromU8Result result = {};
  result.nibble0               = table[rhs];
  result.nibble1               = table[lhs];
  return result;
}

DN_API DN_U8 DN_CVT_U8FromHexNibble(char hex)
{
  bool  digit  = hex >= '0' && hex <= '9';
  bool  upper  = hex >= 'A' && hex <= 'F';
  bool  lower  = hex >= 'a' && hex <= 'f';
  DN_U8 result = 0xFF;
  if (digit)
    result = hex - '0';
  if (upper)
    result = hex - 'A' + 10;
  if (lower)
    result = hex - 'a' + 10;
  return result;
}

DN_API int DN_CVT_FmtBuffer3DotTruncate(char *buffer, int size, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  int size_required = DN_VSNPrintF(buffer, size, fmt, args);
  int result        = DN_Max(DN_Min(size_required, size - 1), 0);
  if (result == size - 1) {
    buffer[size - 2] = '.';
    buffer[size - 3] = '.';
  }
  va_end(args);
  return result;
}

DN_API DN_CVTU64Str8 DN_CVT_Str8FromU64(DN_U64 val, char separator)
{
  DN_CVTU64Str8 result = {};
  if (val == 0) {
    result.data[result.size++] = '0';
  } else {
    // NOTE: The number is written in reverse because we form the string by
    // dividing by 10, so we write it in, then reverse it out after all is
    // done.
    DN_CVTU64Str8 temp = {};
    for (DN_USize digit_count = 0; val > 0; digit_count++) {
      if (separator && (digit_count != 0) && (digit_count % 3 == 0))
        temp.data[temp.size++] = separator;

      auto digit             = DN_CAST(char)(val % 10);
      temp.data[temp.size++] = '0' + digit;
      val /= 10;
    }

    // NOTE: Reverse the string
    DN_MSVC_WARNING_PUSH
    DN_MSVC_WARNING_DISABLE(6293) // Ill-defined for-loop
    DN_MSVC_WARNING_DISABLE(6385) // Reading invalid data from 'temp.data' unsigned overflow is valid for loop termination
    for (DN_USize temp_index = temp.size - 1; temp_index < temp.size; temp_index--) {
      char ch                    = temp.data[temp_index];
      result.data[result.size++] = ch;
    }
    DN_MSVC_WARNING_POP
  }

  return result;
}

DN_API DN_CVTU64Bytes DN_CVT_BytesFromU64(DN_U64 bytes, DN_CVTBytesType type)
{
  DN_Assert(type != DN_CVTBytesType_Count);
  DN_CVTU64Bytes result = {};
  result.bytes          = DN_CAST(DN_F64) bytes;

  if (type == DN_CVTBytesType_Auto)
    for (; result.type < DN_CVTBytesType_Count && result.bytes >= 1024.0; result.type = DN_CAST(DN_CVTBytesType)(DN_CAST(DN_USize) result.type + 1))
      result.bytes /= 1024.0;
  else
    for (; result.type < type; result.type = DN_CAST(DN_CVTBytesType)(DN_CAST(DN_USize) result.type + 1))
      result.bytes /= 1024.0;

  result.suffix = DN_CVT_BytesTypeToStr8(result.type);
  return result;
}

DN_API DN_Str8 DN_CVT_BytesStr8FromU64(DN_Arena *arena, DN_U64 bytes, DN_CVTBytesType desired_type)
{
  DN_CVTU64Bytes byte_size = DN_CVT_BytesFromU64(bytes, desired_type);
  DN_Str8        result    = DN_Str8_FromF(arena, "%.2f%.*s", byte_size.bytes, DN_STR_FMT(byte_size.suffix));
  return result;
}

DN_API DN_Str8 DN_CVT_BytesTypeToStr8(DN_CVTBytesType type)
{
  DN_Str8 result = DN_STR8("");
  switch (type) {
    case DN_CVTBytesType_B:     result = DN_STR8("B"); break;
    case DN_CVTBytesType_KiB:   result = DN_STR8("KiB"); break;
    case DN_CVTBytesType_MiB:   result = DN_STR8("MiB"); break;
    case DN_CVTBytesType_GiB:   result = DN_STR8("GiB"); break;
    case DN_CVTBytesType_TiB:   result = DN_STR8("TiB"); break;
    case DN_CVTBytesType_Count: result = DN_STR8(""); break;
    case DN_CVTBytesType_Auto:  result = DN_STR8(""); break;
  }
  return result;
}

DN_API DN_Str8 DN_CVT_AgeFromU64(DN_Arena *arena, DN_U64 age_s, DN_CVTU64AgeUnit unit)
{
  DN_Str8 result = {};
  if (!arena)
    return result;

  char           buffer[512];
  DN_Arena       stack_arena = DN_Arena_FromBuffer(buffer, sizeof(buffer), DN_ArenaFlags_NoPoison);
  DN_Str8Builder builder     = DN_Str8Builder_FromArena(&stack_arena);
  DN_U64         remainder   = age_s;

  if (unit & DN_CVTU64AgeUnit_Year) {
    DN_USize value = remainder / DN_YearsToSec(1);
    remainder -= DN_YearsToSec(value);
    if (value)
      DN_Str8Builder_AppendF(&builder, "%s%zuyr", builder.string_size ? " " : "", value);
  }

  if (unit & DN_CVTU64AgeUnit_Week) {
    DN_USize value = remainder / DN_WeeksToSec(1);
    remainder -= DN_WeeksToSec(value);
    if (value)
      DN_Str8Builder_AppendF(&builder, "%s%zuw", builder.string_size ? " " : "", value);
  }

  if (unit & DN_CVTU64AgeUnit_Day) {
    DN_USize value = remainder / DN_DaysToSec(1);
    remainder -= DN_DaysToSec(value);
    if (value)
      DN_Str8Builder_AppendF(&builder, "%s%zud", builder.string_size ? " " : "", value);
  }

  if (unit & DN_CVTU64AgeUnit_Hr) {
    DN_USize value = remainder / DN_HoursToSec(1);
    remainder -= DN_HoursToSec(value);
    if (value)
      DN_Str8Builder_AppendF(&builder, "%s%zuh", builder.string_size ? " " : "", value);
  }

  if (unit & DN_CVTU64AgeUnit_Min) {
    DN_USize value = remainder / DN_MinutesToSec(1);
    remainder -= DN_MinutesToSec(value);
    if (value)
      DN_Str8Builder_AppendF(&builder, "%s%zum", builder.string_size ? " " : "", value);
  }

  if (unit & DN_CVTU64AgeUnit_Sec) {
    DN_USize value = remainder;
    if (value || builder.string_size == 0)
      DN_Str8Builder_AppendF(&builder, "%s%zus", builder.string_size ? " " : "", value);
  }

  result = DN_Str8Builder_Build(&builder, arena);
  return result;
}

DN_API DN_Str8 DN_CVT_AgeFromF64(DN_Arena *arena, DN_F64 age_s, DN_CVTU64AgeUnit unit)
{
  DN_Str8 result = {};
  if (!arena)
    return result;

  char           buffer[256];
  DN_Arena       stack_arena = DN_Arena_FromBuffer(buffer, sizeof(buffer), DN_ArenaFlags_NoPoison);
  DN_Str8Builder builder     = DN_Str8Builder_FromArena(&stack_arena);
  DN_F64         remainder   = age_s;

  if (unit & DN_CVTU64AgeUnit_Year) {
    DN_F64 value = remainder / DN_CAST(DN_F64) DN_YearsToSec(1);
    if (value >= 1.0) {
      remainder -= DN_YearsToSec(value);
      DN_Str8Builder_AppendF(&builder, "%s%.1fyr", builder.string_size ? " " : "", value);
    }
  }

  if (unit & DN_CVTU64AgeUnit_Week) {
    DN_F64 value = remainder / DN_CAST(DN_F64) DN_WeeksToSec(1);
    if (value >= 1.0) {
      remainder -= DN_WeeksToSec(value);
      DN_Str8Builder_AppendF(&builder, "%s%.1fw", builder.string_size ? " " : "", value);
    }
  }

  if (unit & DN_CVTU64AgeUnit_Day) {
    DN_F64 value = remainder / DN_CAST(DN_F64) DN_DaysToSec(1);
    if (value >= 1.0) {
      remainder -= DN_WeeksToSec(value);
      DN_Str8Builder_AppendF(&builder, "%s%.1fd", builder.string_size ? " " : "", value);
    }
  }

  if (unit & DN_CVTU64AgeUnit_Hr) {
    DN_F64 value = remainder / DN_CAST(DN_F64) DN_HoursToSec(1);
    if (value >= 1.0) {
      remainder -= DN_HoursToSec(value);
      DN_Str8Builder_AppendF(&builder, "%s%.1fh", builder.string_size ? " " : "", value);
    }
  }

  if (unit & DN_CVTU64AgeUnit_Min) {
    DN_F64 value = remainder / DN_CAST(DN_F64) DN_MinutesToSec(1);
    if (value >= 1.0) {
      remainder -= DN_MinutesToSec(value);
      DN_Str8Builder_AppendF(&builder, "%s%.1fm", builder.string_size ? " " : "", value);
    }
  }

  if (unit & DN_CVTU64AgeUnit_Sec) {
    DN_F64 value = remainder;
    DN_Str8Builder_AppendF(&builder, "%s%.1fs", builder.string_size ? " " : "", value);
  }

  result = DN_Str8Builder_Build(&builder, arena);
  return result;
}

DN_API DN_U64 DN_CVT_U64FromHex(DN_Str8 hex)
{
  DN_Str8  real_hex     = DN_Str8_TrimPrefix(DN_Str8_TrimPrefix(hex, DN_STR8("0x")), DN_STR8("0X"));
  DN_USize max_hex_size = sizeof(DN_U64) * 2 /*hex chars per byte*/;
  DN_Assert(real_hex.size <= max_hex_size);

  DN_USize size   = DN_Min(max_hex_size, real_hex.size);
  DN_U64 result = 0;
  for (DN_USize index = 0; index < size; index++) {
    char  ch  = real_hex.data[index];
    DN_U8 val = DN_CVT_U8FromHexNibble(ch);
    if (val == 0xFF)
      break;
    result = (result << 4) | val;
  }
  return result;
}

DN_API DN_Str8 DN_CVT_HexFromU64(DN_Arena *arena, DN_U64 number, uint32_t flags)
{
  DN_Str8 prefix = {};
  if ((flags & DN_CVTU64HexStrFlags_0xPrefix))
    prefix = DN_STR8("0x");

  char const *fmt           = (flags & DN_CVTU64HexStrFlags_UppercaseHex) ? "%I64X" : "%I64x";
  DN_USize    required_size = DN_CStr8_FSize(fmt, number) + prefix.size;
  DN_Str8     result        = DN_Str8_Alloc(arena, required_size, DN_ZeroMem_No);

  if (DN_Str8_HasData(result)) {
    DN_Memcpy(result.data, prefix.data, prefix.size);
    int space = DN_CAST(int) DN_Max((result.size - prefix.size) + 1, 0); /*null-terminator*/
    DN_SNPrintF(result.data + prefix.size, space, fmt, number);
  }
  return result;
}

DN_API DN_CVTU64HexStr DN_CVT_HexFromU64Str8(DN_U64 number, DN_CVTU64HexStrFlags flags)
{
  DN_Str8 prefix = {};
  if (flags & DN_CVTU64HexStrFlags_0xPrefix)
    prefix = DN_STR8("0x");

  DN_CVTU64HexStr result = {};
  DN_Memcpy(result.data, prefix.data, prefix.size);
  result.size += DN_CAST(int8_t) prefix.size;

  char const *fmt  = (flags & DN_CVTU64HexStrFlags_UppercaseHex) ? "%I64X" : "%I64x";
  int         size = DN_SNPrintF(result.data + result.size, DN_ArrayCountU(result.data) - result.size, fmt, number);
  result.size += DN_CAST(uint8_t) size;
  DN_Assert(result.size < DN_ArrayCountU(result.data));

  // NOTE: snprintf returns the required size of the format string
  // irrespective of if there's space or not, but, always null terminates so
  // the last byte is wasted.
  result.size = DN_Min(result.size, DN_ArrayCountU(result.data) - 1);
  return result;
}

DN_API bool DN_CVT_HexFromBytesPtr(void const *src, DN_USize src_size, char *dest, DN_USize dest_size)
{
  if (!src || !dest)
    return false;

  if (!DN_Check(dest_size >= src_size * 2))
    return false;

  char const          *HEX    = "0123456789abcdef";
  unsigned char const *src_u8 = DN_CAST(unsigned char const *) src;
  for (DN_USize src_index = 0, dest_index = 0; src_index < src_size; src_index++) {
    char byte          = src_u8[src_index];
    char hex01         = (byte >> 4) & 0b1111;
    char hex02         = (byte >> 0) & 0b1111;
    dest[dest_index++] = HEX[(int)hex01];
    dest[dest_index++] = HEX[(int)hex02];
  }

  return true;
}

DN_API DN_Str8 DN_CVT_HexFromBytes(DN_Arena *arena, void const *src, DN_USize size)
{
  DN_Str8 result = {};
  if (!src || size <= 0)
    return result;

  result                   = DN_Str8_Alloc(arena, size * 2, DN_ZeroMem_No);
  result.data[result.size] = 0;
  bool converted           = DN_CVT_HexFromBytesPtr(src, size, result.data, result.size);
  DN_Assert(converted);
  return result;
}

DN_API DN_USize DN_CVT_BytesFromHexPtrUnchecked(DN_Str8 hex, void *dest, DN_USize dest_size)
{
  DN_USize       result  = 0;
  unsigned char *dest_u8 = DN_CAST(unsigned char *) dest;

  for (DN_USize hex_index = 0; hex_index < hex.size; hex_index += 2, result += 1) {
    char hex01      = hex.data[hex_index];
    char hex02      = (hex_index + 1 < hex.size) ? hex.data[hex_index + 1] : 0;
    char bit4_01    = DN_CVT_U8FromHexNibble(hex01);
    char bit4_02    = DN_CVT_U8FromHexNibble(hex02);
    char byte       = (bit4_01 << 4) | (bit4_02 << 0);
    dest_u8[result] = byte;
  }

  DN_Assert(result <= dest_size);
  return result;
}

DN_API DN_USize DN_CVT_BytesFromHexPtr(DN_Str8 hex, void *dest, DN_USize dest_size)
{
  hex = DN_Str8_TrimPrefix(hex, DN_STR8("0x"));
  hex = DN_Str8_TrimPrefix(hex, DN_STR8("0X"));

  DN_USize result = 0;
  if (!DN_Str8_HasData(hex))
    return result;

  // NOTE: Trimmed hex can be "0xf" -> "f" or "0xAB" -> "AB"
  // Either way, the size can be odd or even, hence we round up to the nearest
  // multiple of two to ensure that we calculate the min buffer size orrectly.
  DN_USize hex_size_rounded_up = hex.size + (hex.size % 2);
  DN_USize min_buffer_size     = hex_size_rounded_up / 2;
  if (hex.size <= 0 || !DN_Check(dest_size >= min_buffer_size))
    return result;

  result = DN_CVT_BytesFromHexPtrUnchecked(hex, dest, dest_size);
  return result;
}

DN_API DN_Str8 DN_CVT_BytesFromHexUnchecked(DN_Arena *arena, DN_Str8 hex)
{
  DN_USize hex_size_rounded_up = hex.size + (hex.size % 2);
  DN_Str8  result              = DN_Str8_Alloc(arena, (hex_size_rounded_up / 2), DN_ZeroMem_No);
  if (result.data) {
    DN_USize bytes_written = DN_CVT_BytesFromHexPtr(hex, result.data, result.size);
    DN_Assert(bytes_written == result.size);
  }
  return result;
}

DN_API DN_Str8 DN_CVT_BytesFromHex(DN_Arena *arena, DN_Str8 hex)
{
  hex = DN_Str8_TrimPrefix(hex, DN_STR8("0x"));
  hex = DN_Str8_TrimPrefix(hex, DN_STR8("0X"));

  DN_Str8 result = {};
  if (!DN_Str8_HasData(hex))
    return result;

  if (!DN_Check(DN_Str8_IsAll(hex, DN_Str8IsAll_Hex)))
    return result;

  result = DN_CVT_BytesFromHexUnchecked(arena, hex);
  return result;
}
