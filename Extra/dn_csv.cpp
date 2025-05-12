#include "dn_csv.h"

static DN_CSVTokeniser DN_CSV_TokeniserInit(DN_Str8 string, char delimiter)
{
  DN_CSVTokeniser result = {};
  result.string          = string;
  result.delimiter       = delimiter;
  return result;
}

static bool DN_CSV_TokeniserValid(DN_CSVTokeniser *tokeniser)
{
  bool result = tokeniser && !tokeniser->bad;
  return result;
}

static bool DN_CSV_TokeniserNextRow(DN_CSVTokeniser *tokeniser)
{
  bool result = false;
  if (DN_CSV_TokeniserValid(tokeniser) && DN_Str8_HasData(tokeniser->string)) {
    // NOTE: First time querying row iterator is nil, let tokeniser advance
    if (tokeniser->it) {
      // NOTE: Only advance the tokeniser if we're at the end of the line and
      // there's more to tokenise.
      char const *end = tokeniser->string.data + tokeniser->string.size;
      if (tokeniser->it != end && tokeniser->end_of_line) {
        tokeniser->end_of_line = false;
        result                 = true;
      }
    }
  }

  return result;
}

static DN_Str8 DN_CSV_TokeniserNextField(DN_CSVTokeniser *tokeniser)
{
  DN_Str8 result = {};
  if (!DN_CSV_TokeniserValid(tokeniser))
    return result;

  if (!DN_Str8_HasData(tokeniser->string)) {
    tokeniser->bad = true;
    return result;
  }

  // NOTE: First time tokeniser is invoked with a string, set up initial state.
  char const *string_end = tokeniser->string.data + tokeniser->string.size;
  if (!tokeniser->it) {
    tokeniser->it = tokeniser->string.data;
    // NOTE: Skip any leading new lines
    while (tokeniser->it[0] == '\n' || tokeniser->it[0] == '\r')
      if (++tokeniser->it == string_end)
        break;
  }

  // NOTE: Tokeniser pointing at end, no more valid data to parse.
  if (tokeniser->it == string_end)
    return result;

  // NOTE: Scan forward until the next control character.
  // 1. '"'                   Double quoted field,  extract everything between the quotes.
  // 2. tokeniser->delimiter  End of the field,     extract everything leading up to the delimiter.
  // 3. '\n'                  Last field in record, extract everything leading up the the new line.
  char const *begin = tokeniser->it;
  while (tokeniser->it != string_end && (tokeniser->it[0] != '"' &&
                                                tokeniser->it[0] != tokeniser->delimiter &&
                                                tokeniser->it[0] != '\n'))
    tokeniser->it++;

  bool quoted_field = (tokeniser->it != string_end) && tokeniser->it[0] == '"';
  if (quoted_field) {
    begin = ++tokeniser->it; // Begin after the quote

  // NOTE: Scan forward until the next '"' which marks the end
  // of the field unless it is escaped by another '"'.
  find_next_quote:
    while (tokeniser->it != string_end && tokeniser->it[0] != '"')
      tokeniser->it++;

    // NOTE: If we encounter a '"' right after, the quotes were escaped
    // and we need to skip to the next instance of a '"'.
    if (tokeniser->it != string_end && tokeniser->it + 1 != string_end && tokeniser->it[1] == '"') {
      tokeniser->it += 2;
      goto find_next_quote;
    }
  }

  // NOTE: Mark the end of the field
  char const *end        = tokeniser->it;
  tokeniser->end_of_line = tokeniser->it == string_end || end[0] == '\n';

  // NOTE: In files with \r\n style new lines ensure that we don't include
  // the \r byte in the CSV field we produce.
  if (end != string_end && end[0] == '\n') {
    DN_Assert((uintptr_t)(end - 1) > (uintptr_t)tokeniser->string.data &&
               "Internal error: The string iterator is pointing behind the start of the string we're reading");
    if (end[-1] == '\r')
      end = end - 1;
  }

  // NOTE: Quoted fields may have whitespace after the closing quote, we skip
  // until we reach the field terminator.
  if (quoted_field)
    while (tokeniser->it != string_end && (tokeniser->it[0] != tokeniser->delimiter &&
                                                  tokeniser->it[0] != '\n'))
      tokeniser->it++;

  // NOTE: Advance the tokeniser past the field terminator.
  if (tokeniser->it != string_end)
    tokeniser->it++;

  // NOTE: Generate the record
  result.data = DN_CAST(char *) begin;
  result.size = DN_CAST(int)(end - begin);
  return result;
}

static DN_Str8 DN_CSV_TokeniserNextColumn(DN_CSVTokeniser *tokeniser)
{
  DN_Str8 result = {};
  if (!DN_CSV_TokeniserValid(tokeniser))
    return result;

  // NOTE: End of line, the user must explicitly advance to the next row
  if (tokeniser->end_of_line)
    return result;

  // NOTE: Advance tokeniser to the next field in the row
  result = DN_CSV_TokeniserNextField(tokeniser);
  return result;
}

static void DN_CSV_TokeniserSkipLine(DN_CSVTokeniser *tokeniser)
{
  while (DN_CSV_TokeniserValid(tokeniser) && !tokeniser->end_of_line)
    DN_CSV_TokeniserNextColumn(tokeniser);
  DN_CSV_TokeniserNextRow(tokeniser);
}

static int DN_CSV_TokeniserNextN(DN_CSVTokeniser *tokeniser, DN_Str8 *fields, int fields_size, bool column_iterator)
{
  if (!DN_CSV_TokeniserValid(tokeniser) || !fields || fields_size <= 0)
    return 0;

  int result = 0;
  for (; result < fields_size; result++) {
    fields[result] = column_iterator ? DN_CSV_TokeniserNextColumn(tokeniser) : DN_CSV_TokeniserNextField(tokeniser);
    if (!DN_CSV_TokeniserValid(tokeniser) || !DN_Str8_HasData(fields[result]))
      break;
  }

  return result;
}

DN_MSVC_WARNING_PUSH
DN_MSVC_WARNING_DISABLE(4505) // 'x': unreferenced function with internal linkage has been removed
static int DN_CSV_TokeniserNextColumnN(DN_CSVTokeniser *tokeniser, DN_Str8 *fields, int fields_size)
{
  int result = DN_CSV_TokeniserNextN(tokeniser, fields, fields_size, true /*column_iterator*/);
  return result;
}

static int DN_CSV_TokeniserNextFieldN(DN_CSVTokeniser *tokeniser, DN_Str8 *fields, int fields_size)
{
  int result = DN_CSV_TokeniserNextN(tokeniser, fields, fields_size, false /*column_iterator*/);
  return result;
}

static void DN_CSV_TokeniserSkipLineN(DN_CSVTokeniser *tokeniser, int count)
{
  for (int i = 0; i < count && DN_CSV_TokeniserValid(tokeniser); i++)
    DN_CSV_TokeniserSkipLine(tokeniser);
}

static void DN_CSV_PackU64(DN_CSVPack *pack, DN_CSVSerialise serialise, DN_U64 *value)
{
  if (serialise == DN_CSVSerialise_Read) {
    DN_Str8            csv_value = DN_CSV_TokeniserNextColumn(&pack->read_tokeniser);
    DN_Str8ToU64Result to_u64    = DN_Str8_ToU64(csv_value, 0);
    DN_Assert(to_u64.success);
    *value = to_u64.value;
  } else {
    DN_Str8Builder_AppendF(&pack->write_builder, "%s%" PRIu64, pack->write_column++ ? "," : "", *value);
  }
}

static void DN_CSV_PackI64(DN_CSVPack *pack, DN_CSVSerialise serialise, DN_I64 *value)
{
  if (serialise == DN_CSVSerialise_Read) {
    DN_Str8            csv_value = DN_CSV_TokeniserNextColumn(&pack->read_tokeniser);
    DN_Str8ToI64Result to_i64    = DN_Str8_ToI64(csv_value, 0);
    DN_Assert(to_i64.success);
    *value = to_i64.value;
  } else {
    DN_Str8Builder_AppendF(&pack->write_builder, "%s%" PRIu64, pack->write_column++ ? "," : "", *value);
  }
}

static void DN_CSV_PackI32(DN_CSVPack *pack, DN_CSVSerialise serialise, DN_I32 *value)
{
  DN_I64 u64 = *value;
  DN_CSV_PackI64(pack, serialise, &u64);
  if (serialise == DN_CSVSerialise_Read)
    *value = DN_SaturateCastI64ToI32(u64);
}

static void DN_CSV_PackI16(DN_CSVPack *pack, DN_CSVSerialise serialise, DN_I16 *value)
{
  DN_I64 u64 = *value;
  DN_CSV_PackI64(pack, serialise, &u64);
  if (serialise == DN_CSVSerialise_Read)
    *value = DN_SaturateCastI64ToI16(u64);
}

static void DN_CSV_PackI8(DN_CSVPack *pack, DN_CSVSerialise serialise, DN_I8 *value)
{
  DN_I64 u64 = *value;
  DN_CSV_PackI64(pack, serialise, &u64);
  if (serialise == DN_CSVSerialise_Read)
    *value = DN_SaturateCastI64ToI8(u64);
}


static void DN_CSV_PackU32(DN_CSVPack *pack, DN_CSVSerialise serialise, DN_U32 *value)
{
  DN_U64 u64 = *value;
  DN_CSV_PackU64(pack, serialise, &u64);
  if (serialise == DN_CSVSerialise_Read)
    *value = DN_SaturateCastU64ToU32(u64);
}

static void DN_CSV_PackU16(DN_CSVPack *pack, DN_CSVSerialise serialise, DN_U16 *value)
{
  DN_U64 u64 = *value;
  DN_CSV_PackU64(pack, serialise, &u64);
  if (serialise == DN_CSVSerialise_Read)
    *value = DN_SaturateCastU64ToU16(u64);
}

static void DN_CSV_PackBoolAsU64(DN_CSVPack *pack, DN_CSVSerialise serialise, bool *value)
{
  DN_U64 u64 = *value;
  DN_CSV_PackU64(pack, serialise, &u64);
  if (serialise == DN_CSVSerialise_Read)
    *value = u64 ? 1 : 0;
}

static void DN_CSV_PackStr8(DN_CSVPack *pack, DN_CSVSerialise serialise, DN_Str8 *str8, DN_Arena *arena)
{
  if (serialise == DN_CSVSerialise_Read) {
    DN_Str8 csv_value = DN_CSV_TokeniserNextColumn(&pack->read_tokeniser);
    *str8             = DN_Str8_Copy(arena, csv_value);
  } else {
    DN_Str8Builder_AppendF(&pack->write_builder, "%s%.*s", pack->write_column++ ? "," : "", DN_STR_FMT(*str8));
  }
}

static void DN_CSV_PackBuffer(DN_CSVPack *pack, DN_CSVSerialise serialise, void *dest, size_t *size)
{
  if (serialise == DN_CSVSerialise_Read) {
    DN_Str8 csv_value = DN_CSV_TokeniserNextColumn(&pack->read_tokeniser);
    *size             = DN_Min(*size, csv_value.size);
    DN_Memcpy(dest, csv_value.data, *size);
  } else {
    DN_Str8Builder_AppendF(&pack->write_builder, "%s%.*s", pack->write_column++ ? "," : "", DN_CAST(int)(*size), dest);
  }
}

static void DN_CSV_PackBufferWithMax(DN_CSVPack *pack, DN_CSVSerialise serialise, void *dest, size_t *size, size_t max)
{
  if (serialise == DN_CSVSerialise_Read)
    *size = max;
  DN_CSV_PackBuffer(pack, serialise, dest, size);
}

static bool DN_CSV_PackNewLine(DN_CSVPack *pack, DN_CSVSerialise serialise)
{
  bool result = true;
  if (serialise == DN_CSVSerialise_Read) {
    result = DN_CSV_TokeniserNextRow(&pack->read_tokeniser);
  } else {
    pack->write_column = 0;
    result             = DN_Str8Builder_AppendRef(&pack->write_builder, DN_STR8("\n"));
  }
  return result;
}
DN_MSVC_WARNING_POP
