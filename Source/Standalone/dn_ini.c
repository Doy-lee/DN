#include "dn_ini.h"

#if !defined(__cplusplus__)
  #include <stdbool.h>
#endif

typedef struct DN_INIStr8BSplit DN_INIStr8BSplit;
struct DN_INIStr8BSplit {
  DN_INIStr8 lhs;
  DN_INIStr8 rhs;
};

typedef struct DN_INIStr8Builder DN_INIStr8Builder;
struct DN_INIStr8Builder
{
  char  *data;
  size_t used;
  size_t max;
  size_t size_req;
};

void *DN_INI_ArenaAlloc(DN_INIArena *arena, size_t size)
{
  size_t new_used = arena->used + size;
  void  *result   = 0;
  if (new_used <= arena->max) {
    result      = arena->base + arena->used;
    arena->used = new_used;
    DN_INI_Memset(result, 0, size);
  }
  return result;
}

static bool DN_INI_CharIsWhitespace_(char ch)
{
  bool result = ch == ' ' || ch == '\r' || ch == '\n' || ch == '\t';
  return result;
}

DN_INIStr8 DN_INI_Str8FromPtr(char const *data, size_t count)
{
  DN_INIStr8 result = {};
  result.data       = (char *)data;
  result.size       = count;
  return result;
}

static bool DN_INI_Str8Eq(DN_INIStr8 lhs, DN_INIStr8 rhs)
{
  bool result = lhs.size == rhs.size && DN_INI_Memcmp(lhs.data, rhs.data, lhs.size) == 0;
  return result;
}

static DN_INIStr8 DN_INI_Str8Slice(DN_INIStr8 slice, size_t offset, size_t size)
{
  DN_INIStr8 result = {};
  if (slice.data) {
    size_t max_offset   = slice.size;
    size_t final_offset = offset <= max_offset ? offset : max_offset;
    size_t max_size     = slice.size - final_offset;
    result.data           = slice.data + final_offset;
    result.size           = size <= max_size ? size : max_size;
  }
  return result;
}

static DN_INIStr8BSplit DN_INI_Str8BSplit(DN_INIStr8 str8, DN_INIStr8 find)
{
  DN_INIStr8BSplit result = {};
  if (find.size > str8.size)
    return result;

  for (size_t index = 0; index < (str8.size - find.size) + 1; index++) {
    DN_INIStr8 slice = DN_INI_Str8FromPtr(str8.data + index, find.size);
    if (DN_INI_Str8Eq(slice, find)) {
      result.lhs        = DN_INI_Str8FromPtr(str8.data, (size_t)index);
      size_t rhs_size = (size_t)(str8.size - (index + 1));
      DN_INI_Assert(rhs_size < str8.size);
      result.rhs = DN_INI_Str8FromPtr(str8.data + index + 1, rhs_size);
      break;
    }
  }

  if (!result.lhs.data)
    result.lhs = str8;
  return result;
}

static DN_INIStr8BSplit DN_INI_Str8BSplitReverse(DN_INIStr8 str8, DN_INIStr8 find)
{
  DN_INIStr8BSplit result = {};
  if (find.size > str8.size)
    return result;

  for (size_t index = str8.size - find.size; index > 0; index--) {
    DN_INIStr8 slice = DN_INI_Str8FromPtr(str8.data + index, find.size);
    if (DN_INI_Str8Eq(slice, find)) {
      result.lhs        = DN_INI_Str8FromPtr(str8.data, (size_t)index);
      size_t rhs_size = (size_t)(str8.size - index - find.size);
      DN_INI_Assert(rhs_size < str8.size);
      result.rhs = DN_INI_Str8FromPtr(str8.data + index + find.size, rhs_size);
      break;
    }
  }

  if (!result.lhs.data)
    result.lhs = str8;
  return result;
}

DN_INITokeniser DN_INI_TokeniserFromPtr(char const *buf, size_t count)
{
  DN_INITokeniser result = {};
  result.data            = (char *)buf;
  result.count           = count;
  return result;
}

DN_INIToken DN_INI_NextToken(DN_INITokeniser const *tokeniser)
{
  size_t    pos    = tokeniser->pos;
  DN_INIToken result = {};
  result.line        = tokeniser->line;
  result.line_start  = tokeniser->line_start ? tokeniser->line_start : tokeniser->data;
  result.next_p      = tokeniser->count;

  while (result.type == DN_INITokenType_Nil) {
    if (tokeniser->pos < tokeniser->count && DN_INI_CharIsWhitespace_(tokeniser->data[pos])) {
      if (tokeniser->data[pos++] == '\n') {
        result.line++;
        result.line_start          = tokeniser->data + pos;
        result.line_start_new_line = true;
      }
      continue;
    }

    if (pos >= tokeniser->count) {
      if (tokeniser->prev_token.type == DN_INITokenType_Nil            ||
          tokeniser->prev_token.type == DN_INITokenType_Value          ||
          tokeniser->prev_token.type == DN_INITokenType_Comment        ||
          tokeniser->prev_token.type == DN_INITokenType_Value          ||
          tokeniser->prev_token.type == DN_INITokenType_FieldSeparator ||
          tokeniser->prev_token.type == DN_INITokenType_MultilineValue ||
          tokeniser->prev_token.type == DN_INITokenType_Section) {
        result.type = DN_INITokenType_EndOfStream;
      } else {
        result.type  = DN_INITokenType_Error;
        result.error = DN_INIStr8Lit("Premature end of stream encountered malforming the last key-value pair");
      }
      result.data = tokeniser->data + pos;
    }

    switch (tokeniser->data[pos]) {
      case '[': {
        result.data = (char *)tokeniser->data + pos + 1;
        while (result.type == DN_INITokenType_Nil) {
          pos++;
          if (pos >= tokeniser->count) {
            result.type  = DN_INITokenType_Error;
            result.error = DN_INIStr8Lit("Invalid end-of-stream in section header");
          } else if (tokeniser->data[pos] == '\n') {
            result.type  = DN_INITokenType_Error;
            result.error = DN_INIStr8Lit("Invalid new-line in section header");
          } else if (tokeniser->data[pos] == ']') {
            result.type   = DN_INITokenType_Section;
            result.next_p = pos + 1;
          }
          result.count  = (size_t)((tokeniser->data + pos) - result.data);
        }
      } break;

      case '#': {
        if (tokeniser->prev_token.type != DN_INITokenType_Nil &&
            tokeniser->prev_token.type != DN_INITokenType_Comment &&
            tokeniser->prev_token.type != DN_INITokenType_Value &&
            tokeniser->prev_token.type != DN_INITokenType_MultilineValue &&
            tokeniser->prev_token.type != DN_INITokenType_Section) {
          result.data  = (char *)tokeniser->data + pos + 1;
          result.type  = DN_INITokenType_Error;
          result.error = DN_INIStr8Lit("Invalid comment that was not preceeded by another comment, section, value or at the start of the file");
          result.count = 1;
        } else {
          result.data = (char *)tokeniser->data + pos + 1;
          while (result.type == DN_INITokenType_Nil) {
            pos++;
            if (pos >= tokeniser->count || tokeniser->data[pos] == '\n') {
              result.type  = DN_INITokenType_Comment;
              result.count = (size_t)((tokeniser->data + pos) - result.data);
              result.next_p = pos;
            }
          }
        }
      } break;

      case '=': {
        if (tokeniser->prev_token.type == DN_INITokenType_Key) {
          result.type = DN_INITokenType_FieldSeparator;
        } else {
          result.type = DN_INITokenType_Error;
          result.error = DN_INIStr8Lit("Invalid key-value separator, '=' is not being used to separate a key-value pair");
        }
        result.data   = (char *)tokeniser->data + pos;
        result.next_p = pos + 1;
        result.count  = 1;
      } break;

      default: {
        bool quoted = tokeniser->data[pos] == '"';
        if (quoted)
            pos++;

        result.data = (char *)tokeniser->data + pos;

        for (; result.type == DN_INITokenType_Nil; pos++) {
          bool end_of_stream = pos >= tokeniser->count;
          bool end_of_quote  = !end_of_stream && quoted && tokeniser->data[pos] == '"';
          bool hash          = tokeniser->data[pos] == '#' && !quoted;
          bool backslash     = tokeniser->data[pos] == '\\' && !quoted;
          bool equals        = tokeniser->data[pos] == '=' && !quoted;

          if (end_of_stream || DN_INI_CharIsWhitespace_(tokeniser->data[pos]) || hash || backslash || equals || end_of_quote) {
            result.count  = (size_t)((tokeniser->data + pos) - result.data);
            result.next_p = pos;
            if (end_of_quote) {
              result.next_p = pos + 1;
              DN_INI_Assert(!end_of_stream);
              DN_INI_Assert(tokeniser->data[pos] == '"');
            }

            if (!end_of_stream && tokeniser->data[pos] == '\\') {
              if (tokeniser->prev_token.type != DN_INITokenType_FieldSeparator &&
                  tokeniser->prev_token.type != DN_INITokenType_Value &&
                  tokeniser->prev_token.type != DN_INITokenType_MultilineValue) {
                result.type   = DN_INITokenType_Error;
                result.error  = DN_INIStr8Lit("Invalid unquoted string, escape character '\\' is only allowed in INI values");
                result.count  = (size_t)((tokeniser->data + pos) - result.data);
                break;
              }

              DN_INIStr8 esc_str8 = DN_INI_Str8Slice(DN_INI_Str8FromPtr(tokeniser->data, tokeniser->count), pos + 1, 1);
              if (DN_INI_Str8Eq(esc_str8, DN_INIStr8Lit("\n"))) {
                result.next_p += 2;
              } else if (DN_INI_Str8Eq(esc_str8, DN_INIStr8Lit("\\"))) {
                // NOTE: Backespace is escaping a backspace
              } else {
                result.next_p += 1;
              }
            }

            // NOTE: We only have a continuation of a multiline if we didn't have a newline, e.g.:
            //
            //   foo=bar \\n
            //   baz\n
            //   next=property
            //
            // 'baz' is a multiline value that appends to 'bar '. When the tokeniser then reads
            // 'next', the previous value is a multiline-value, but, we started a new-line which
            // terminates the multi-line value. This means that we know we're starting a new
            // key-value pair so we should _not_ append the multi-line value.
            bool multiline_value = tokeniser->prev_token.type == DN_INITokenType_MultilineValue && !result.line_start_new_line;
            bool value           = tokeniser->prev_token.type == DN_INITokenType_Value          && !result.line_start_new_line;

            if (tokeniser->prev_token.type == DN_INITokenType_FieldSeparator || multiline_value || value) {
              if (tokeniser->data[pos] == ' ') // Value can have spaces in it without quotes
                continue;
              result.type = tokeniser->prev_token.type == DN_INITokenType_FieldSeparator ? DN_INITokenType_Value : DN_INITokenType_MultilineValue;
            } else if (tokeniser->prev_token.type == DN_INITokenType_Key) {
              result.type  = DN_INITokenType_Error;
              result.error = DN_INIStr8Lit("Invalid unquoted string, multiple consecutive keys encountered");
            } else {
              result.type = DN_INITokenType_Key;
            }

            if (result.type == DN_INITokenType_Value && tokeniser->data[pos] == '#')
              while (result.count && DN_INI_CharIsWhitespace_(result.data[result.count - 1]))
                result.count--;
          }
        }
      } break;
    }
  }

  result.column = (size_t)(result.data - result.line_start);
  return result;
}

void DN_INI_EatToken(DN_INITokeniser *tokeniser, DN_INIToken token)
{
  DN_INI_Assert(token.data >= tokeniser->data && token.data <= tokeniser->data + tokeniser->count);
  DN_INI_Assert(tokeniser->pos <= tokeniser->count);
  tokeniser->pos        = token.next_p;
  tokeniser->prev_token = token;
  tokeniser->line       = token.line;
  tokeniser->column     = token.column;
  tokeniser->line_start = token.line_start;
}

static DN_INIToken DN_INI_MakeParseOutOfMemoryErrorToken_(DN_INIToken token)
{
  DN_INIToken result = token;
  result.type        = DN_INITokenType_Error;
  result.error       = DN_INIStr8Lit("Out of memory");
  return result;
}

DN_INISection *DN_INI_ChildSectionFromStr8(DN_INISection *section, DN_INIStr8 str8)
{
  DN_INIStr8     section_name = str8;
  DN_INISection *result       = section;
  DN_INISection *curr         = section;
  while (result) {
    DN_INIStr8BSplit split = DN_INI_Str8BSplit(section_name, DN_INIStr8Lit("."));
    if (split.lhs.size == 0)
      break;

    result = 0;
    for (DN_INISection *it = curr->child_first; !result && it; it = it->next) {
      if (DN_INI_Str8Eq(it->name, split.lhs)) {
        curr = result = it;
        section_name  = split.rhs;
      }
    }
  }
  return result;
}

DN_INISection *DN_INI_ChildSectionFromCStr(DN_INISection *section, char const *name, size_t name_size)
{
  DN_INISection *result = DN_INI_ChildSectionFromStr8(section, DN_INI_Str8FromPtr(name, name_size));
  return result;
}

DN_INIField *DN_INI_FieldFromSectionStr8(DN_INISection *section, DN_INIStr8 str8)
{
  DN_INIField *result = 0;
  if (section) {
    DN_INIStr8BSplit split        = DN_INI_Str8BSplitReverse(str8, DN_INIStr8Lit("."));
    DN_INIStr8       find_key     = str8;
    DN_INISection   *find_section = section;
    if (split.rhs.size) {
      find_section = DN_INI_ChildSectionFromCStr(section, split.lhs.data, split.lhs.size);
      find_key     = split.rhs;
    }

    if (find_section) {
      for (DN_INIField *it = find_section->first_field; !result && it; it = it->next)
        if (DN_INI_Str8Eq(it->key, find_key))
          result = it;
    }
  }
  return result;
}

DN_INIField *DN_INI_FieldFromSection(DN_INISection *section, char const *key, size_t key_size)
{
  DN_INIField *result = DN_INI_FieldFromSectionStr8(section, DN_INI_Str8FromPtr(key, key_size));
  return result;
}

DN_INIFieldUSize DN_INI_FieldUSizeFromSectionStr8(DN_INISection *section, DN_INIStr8 str8)
{
  DN_INIFieldUSize result = {};
  result.field              = DN_INI_FieldFromSectionStr8(section, str8);
  if (result.field) {
    if (result.field->value_type == DN_INIFieldType_USize) {
      result.value   = result.field->value_usize;
      result.success = true;
    } else {
      // NOTE: Try parse string as USize
      // NOTE: Sanitize input/output
      DN_INIStr8 value = result.field->value;
      while (value.size && DN_INI_CharIsWhitespace_(value.data[0])) {
        value.data++;
        value.size--;
      }

      // NOTE: Handle prefix '+'
      if (value.size && value.data[0] == '+') {
        value.data++;
        value.size--;
      }

      // NOTE: Convert the string number to the binary number
      size_t value_usize = 0;
      for (size_t index = 0; index < value.size; index++) {
        char     ch    = value.data[index];
        uint64_t digit = ch - '0';
        if (!(ch >= '0' && ch <= '9'))
            return result;
        if (value_usize > (SIZE_MAX / 10) - digit)
          return result; // NOTE: Overflow
        value_usize = value_usize * 10 + digit;
      }
      result.value   = value_usize;
      result.success = true;
    }
  }
  return result;
}

DN_INIFieldStr8 DN_INI_FieldStr8FromSectionStr8(DN_INISection *section, DN_INIStr8 str8)
{
  DN_INIFieldStr8 result = {};
  result.field           = DN_INI_FieldFromSectionStr8(section, str8);
  if (result.field) {
    result.value   = result.field->value;
    result.success = true;
  }
  return result;
}

DN_INIFieldBool DN_INI_FieldBoolFromSectionStr8(DN_INISection *section, DN_INIStr8 str8)
{
  DN_INIFieldBool result = {};
  result.field              = DN_INI_FieldFromSectionStr8(section, str8);
  if (result.field) {
    if (result.field->value_type == DN_INIFieldType_Bool) {
      result.value   = result.field->value_bool;
      result.success = true;
    } else {
      if (DN_INI_Str8Eq(result.field->value, DN_INIStr8Lit("y")) ||
          DN_INI_Str8Eq(result.field->value, DN_INIStr8Lit("1")) ||
          DN_INI_Str8Eq(result.field->value, DN_INIStr8Lit("true"))) {
        result.value   = true;
        result.success = true;
      } else if (DN_INI_Str8Eq(result.field->value, DN_INIStr8Lit("m")) ||
                 DN_INI_Str8Eq(result.field->value, DN_INIStr8Lit("0")) ||
                 DN_INI_Str8Eq(result.field->value, DN_INIStr8Lit("false"))) {
        result.value   = false;
        result.success = true;
      }
    }
  }
  return result;
}

DN_INICore DN_INI_ParseFromPtr(char const *buf, size_t count, char *base, size_t base_count)
{
  DN_INITokeniser tokeniser = DN_INI_TokeniserFromPtr(buf, count);
  DN_INIArena     arena     = {};
  arena.base                = base;
  arena.max                 = base_count;

  DN_INICore     result       = {};
  DN_INISection  *curr_section = &result.first_section;
  DN_INIField *field    = 0;
  for (;;) {
    DN_INIToken token = DN_INI_NextToken(&tokeniser);
    if (token.type == DN_INITokenType_EndOfStream)
      break;

    if (token.type == DN_INITokenType_Error) {
      result.error_token = token;
      break;
    }

    switch (token.type) {
      case DN_INITokenType_EndOfStream:    /*FALLTHRU*/
      case DN_INITokenType_Error:          /*FALLTHRU*/
      case DN_INITokenType_Nil:            DN_INI_Assert(!"Invalid code path"); break;
      case DN_INITokenType_FieldSeparator: break;
      case DN_INITokenType_Comment:        break;

      case DN_INITokenType_Section: {
        DN_INISection *parent       = &result.first_section;
        DN_INIStr8     section_name = DN_INI_Str8FromPtr(token.data, token.count);
        curr_section                = &result.first_section;
        for (;;) {
          DN_INIStr8BSplit split = DN_INI_Str8BSplit(section_name, DN_INIStr8Lit("."));
          if (split.lhs.size == 0)
            break;

          DN_INISection *next_section = DN_INI_ChildSectionFromCStr(parent, split.lhs.data, split.lhs.size);
          if (!next_section) {
            result.total_sections_count++;
            next_section = (DN_INISection *)DN_INI_ArenaAlloc(&arena, sizeof(*parent));
            if (next_section) {
              if (!parent->child_first)
                parent->child_first = next_section;
              if (parent->child_last)
                parent->child_last->next = next_section;
              parent->child_last   = next_section;
              next_section->name   = split.lhs;
              next_section->token  = token;
              next_section->parent = parent;
            }
          }

          if (base && !parent) {
            result.error_token = DN_INI_MakeParseOutOfMemoryErrorToken_(token);
            return result;
          }

          section_name = split.rhs;
          curr_section = next_section;
          parent       = curr_section;
        }
      } break;

      case DN_INITokenType_Key: {
        field = DN_INI_FieldFromSection(curr_section, token.data, token.count);
        if (!field) {
          result.total_fields_count++;
          field = (DN_INIField *)DN_INI_ArenaAlloc(&arena, sizeof(*field));
          if (base && !field) {
            result.error_token = DN_INI_MakeParseOutOfMemoryErrorToken_(token);
            return result;
          }

          if (field) {
            field->key.data = token.data;
            field->key.size = token.count;
          }

          if (curr_section) {
            if (!curr_section->first_field)
              curr_section->first_field = field;
            if (curr_section->last_field)
              curr_section->last_field->next = field;
            curr_section->last_field = field;
            curr_section->fields_count++;
          }
        }
      } break;

      case DN_INITokenType_MultilineValue: {
        size_t bytes_req = token.count;
        if (tokeniser.prev_token.type == DN_INITokenType_Value) {
          // NOTE: We saw a value, then the next token was a multiline value, we will merge these
          // values into 1 stream, so we need to copy the previous string out as well.
          bytes_req += tokeniser.prev_token.count;
        }

        result.memory_required += bytes_req;
        if (curr_section && field) {
          DN_INI_Assert(curr_section->fields_count);
          DN_INI_Assert(field->key.size);

          char *string = (char *)DN_INI_ArenaAlloc(&arena, bytes_req);
          if (!string) {
            result.error_token = DN_INI_MakeParseOutOfMemoryErrorToken_(token);
            return result;
          }

          if (tokeniser.prev_token.type == DN_INITokenType_Value) {

            field->value.data = string;
            field->value.size = 0;

            for (size_t index = 0; index < tokeniser.prev_token.count; index++) {
              char ch   = tokeniser.prev_token.data[index];
              char next = index + 1 < tokeniser.prev_token.count ? tokeniser.prev_token.data[index + 1] : 0;
              if (ch == '\\' && next == 'n') {
                field->value.data[field->value.size++] = '\n';
                index++;
              } else {
                field->value.data[field->value.size++] = ch;
              }
            }
          } else {
            // NOTE: If we have a multi-line value we are accumulating onto the same key-value.
            // Invariant to this is that the arena is only being used to allocate contiguous memory
            // for the string. Essentially each time we visit this branch we're just bumping the
            // capacity of the original string we allocated at the start of the multi-line value.
          }

          for (size_t index = 0; index < token.count; index++) {
            char ch   = token.data[index];
            char next = index + 1 < token.count ? token.data[index + 1] : 0;
            if (ch == '\\' && next == 'n') {
              field->value.data[field->value.size++] = '\n';
              index++;
            } else {
              field->value.data[field->value.size++] = ch;
            }
          }
        }
      } break;

      case DN_INITokenType_Value: {
        // NOTE: Scan the string to see if it has a line break, if so we allocate memory for it and
        // and convert the line break into a single-byte newline
        size_t bytes_req = 0;
        for (size_t index = 0; index < token.count; index++) {
          char ch   = token.data[index];
          char next = index + 1 < token.count ? token.data[index + 1] : 0;
          if (ch == '\\' && next == 'n') {
            bytes_req = token.count;
            break;
          }
        }

        result.memory_required += bytes_req;
        if (curr_section && field) {
          DN_INI_Assert(curr_section->fields_count);
          DN_INI_Assert(field->key.size);
          if (bytes_req) {
            field->value.data = (char *)DN_INI_ArenaAlloc(&arena, bytes_req);
            if (!field->value.data) {
              result.error_token = DN_INI_MakeParseOutOfMemoryErrorToken_(token);
              return result;
            }

            for (size_t index = 0; index < token.count; index++) {
              char ch   = token.data[index];
              char next = index + 1 < token.count ? token.data[index + 1] : 0;
              if (ch == '\\' && next == 'n') {
                field->value.data[field->value.size++] = '\n';
                index++;
              } else {
                field->value.data[field->value.size++] = ch;
              }
            }
            DN_INI_Assert(field->value.size <= bytes_req);
          } else {
            field->value.data = token.data;
            field->value.size = token.count;
          }
        }
      } break;
    }
    DN_INI_EatToken(&tokeniser, token);
  }

  result.memory_required += (result.total_sections_count * sizeof(DN_INISection)) + (result.total_fields_count * sizeof(DN_INIField));
  return result;
}

void DN_INI_AppendValue(DN_INISection *section, DN_INIField *field)
{
  if (!section->first_field)
    section->first_field = field;
  if (section->last_field)
    section->last_field->next = field;
  section->last_field = field;
}

DN_INISection *DN_INI_AppendSectionF(DN_INICore *ini, DN_INIArena *arena, DN_INISection *section, char const *fmt, ...)
{
  va_list args, args_copy;
  va_start(args, fmt);
  va_copy(args_copy, args);
  int size_req = vsnprintf(0, 0, fmt, args);
  va_end(args);

  DN_INISection *result   = 0;
  size_t         mem_req  = sizeof(*result) + (size_req + 1);
  ini->memory_required   += mem_req;
  if (arena && arena->used + mem_req <= arena->max) {
    result            = (DN_INISection *)DN_INI_ArenaAlloc(arena, sizeof(*result));
    result->name.data = (char *)DN_INI_ArenaAlloc(arena, size_req + 1);
    result->name.size = size_req;
    vsnprintf(result->name.data, result->name.size + 1, fmt, args_copy);

    if (result) {
      if (section) {
        if (!section->child_first)
          section->child_first = result;
        if (section->child_last)
          section->child_last->next = result;
        section->child_last = result;
      }
      if (result) {
        if (result->parent)
          result->parent->child_count++;
        result->parent = section;
      }
    }
  }
  va_end(args_copy);

  return result;
}

static DN_INIField *DN_INI_AllocFieldInternal(DN_INIStr8 key, DN_INIArena *arena)
{
  DN_INIField *result = 0;
  size_t mem_req         = sizeof(*result) + (key.size + 1);
  if (arena->used + mem_req <= arena->max) {
    result             = (DN_INIField *)DN_INI_ArenaAlloc(arena, sizeof(*result));
    result->key.data   = (char *)DN_INI_ArenaAlloc(arena, key.size + 1);
    result->key.size   = key.size;
    DN_INI_Memcpy(result->key.data, key.data, key.size);
    result->key.data[result->key.size] = 0;
  }
  return result;
}

DN_INIField *DN_INI_AppendKeyBool(DN_INICore *ini, DN_INIArena *arena, DN_INISection *section, DN_INIStr8 key, bool value)
{
  DN_INIField *result   = 0;
  size_t          mem_req  = sizeof(*result) + key.size + sizeof(value);
  ini->memory_required    += mem_req;
  if (arena && arena->used + mem_req <= arena->max) {
    result             = DN_INI_AllocFieldInternal(key, arena);
    result->value_bool = value;
    result->value_type = DN_INIFieldType_Bool;
    DN_INI_Memcpy(result->key.data, key.data, key.size);
    DN_INI_AppendValue(section, result);
  }
  return result;
}

DN_INIField *DN_INI_AppendKeyPtrBool(DN_INICore *ini, DN_INIArena *arena, DN_INISection *section, char const *key, size_t key_size, bool value)
{
  DN_INIStr8      key_str8 = DN_INI_Str8FromPtr(key, key_size);
  DN_INIField *result   = DN_INI_AppendKeyBool(ini, arena, section, key_str8, value);
  return result;
}

DN_INIField *DN_INI_AppendKeyUSize(DN_INICore *ini, DN_INIArena *arena, DN_INISection *section, DN_INIStr8 key, size_t value)
{
  DN_INIField *result   = 0;
  size_t          mem_req  = sizeof(*result) + key.size + sizeof(value);
  ini->memory_required    += mem_req;
  if (arena && arena->used + mem_req <= arena->max) {
    result              = DN_INI_AllocFieldInternal(key, arena);
    result->value_usize = value;
    result->value_type = DN_INIFieldType_USize;
    DN_INI_Memcpy(result->key.data, key.data, key.size);
    DN_INI_AppendValue(section, result);
  }
  return result;
}

DN_INIField *DN_INI_AppendKeyPtrUSize(DN_INICore *ini, DN_INIArena *arena, DN_INISection *section, char const *key, size_t key_size, size_t value)
{
  DN_INIStr8   key_str8 = DN_INI_Str8FromPtr(key, key_size);
  DN_INIField *result   = DN_INI_AppendKeyUSize(ini, arena, section, key_str8, value);
  return result;
}

DN_INIField *DN_INI_AppendKeyCStr8(DN_INICore *ini, DN_INIArena *arena, DN_INISection *section, DN_INIStr8 key, char const *value, size_t value_size)
{
  DN_INIField *result   = 0;
  size_t          mem_req  = sizeof(*result) + (key.size + 1) + value_size;
  ini->memory_required    += mem_req;
  if (arena && arena->used + mem_req <= arena->max) {
    result             = DN_INI_AllocFieldInternal(key, arena);
    result->value.data = (char *)DN_INI_ArenaAlloc(arena, value_size);
    result->value.size = value_size;
    result->value_type = DN_INIFieldType_String;
    DN_INI_Memcpy(result->value.data, value, value_size);
    DN_INI_AppendValue(section, result);
  }
  return result;
}

DN_INIField *DN_INI_AppendKeyF(DN_INICore *ini, DN_INIArena *arena, DN_INISection *section, DN_INIStr8 key, char const *fmt, ...)
{
  va_list args, args_copy;
  va_start(args, fmt);
  va_copy(args_copy, args);
  int size_req = vsnprintf(0, 0, fmt, args);
  va_end(args);

  DN_INIField *result  = 0;
  size_t          mem_req = sizeof(*result) + (key.size + 1) + (size_req + 1);
  ini->memory_required   += mem_req;
  if (arena && arena->used + mem_req <= arena->max) {
    result             = DN_INI_AllocFieldInternal(key, arena);
    result->value.data = (char *)DN_INI_ArenaAlloc(arena, size_req + 1);
    result->value.size = size_req;
    vsnprintf(result->value.data, result->value.size + 1, fmt, args_copy);
    result->value.data[result->value.size] = 0;
    DN_INI_AppendValue(section, result);
  }
  va_end(args_copy);

  return result;
}

static void DN_INI_Str8BuilderAppend(DN_INIStr8Builder *builder, char const *fmt, ...)
{
  va_list args;
  va_list args_copy;
  va_start(args, fmt);
  va_copy(args_copy, args);
  int size_req = vsnprintf(0, 0, fmt, args);
  va_end(args);

  builder->size_req += size_req;
  if (builder->used + size_req <= builder->max) {
    vsnprintf(builder->data + builder->used, builder->max - builder->used, fmt, args_copy);
    builder->used += size_req;
  }
  va_end(args_copy);
}

DN_INIStr8FromResult DN_INI_Str8FromINI(DN_INICore const *ini, char *buffer, size_t size)
{
  DN_INIStr8FromResult result              = {};
  DN_INISection const *section_stack[64]   = {};
  size_t               section_stack_count = 0;
  if (ini->first_section.child_first)
    section_stack[section_stack_count++] = &ini->first_section;

  DN_INIStr8Builder builder = {};
  builder.data              = buffer;
  builder.max               = size;

  DN_INISection *parent_stack[32]   = {};
  size_t         parent_stack_count = 0;
  for (; section_stack_count;) {
    DN_INISection const *it = section_stack[--section_stack_count];

    if (it != &ini->first_section) {
      DN_INI_Str8BuilderAppend(&builder, "[");
      for (DN_INISection *parent = it->parent; parent; parent = parent->parent)
        if (parent->name.size)
          parent_stack[parent_stack_count++] = parent;
      for (size_t index = parent_stack_count - 1; index < parent_stack_count; index--) {
        DN_INISection *parent = parent_stack[index];
        DN_INI_Str8BuilderAppend(&builder, "%.*s.", (int)parent->name.size, parent->name.data);
      }
      parent_stack_count = 0;
      DN_INI_Str8BuilderAppend(&builder, "%.*s]\n", (int)it->name.size, it->name.data);
    }

    for (DN_INIField *field = it->first_field; field; field = field->next) {
      DN_INI_Str8BuilderAppend(&builder, "%.*s = ", (int)field->key.size, field->key.data);
      switch (field->value_type) {
        case DN_INIFieldType_String: DN_INI_Str8BuilderAppend(&builder, "%.*s\n", (int)field->value.size, field->value.data); break;
        case DN_INIFieldType_Bool:   DN_INI_Str8BuilderAppend(&builder, "%d\n", field->value_bool); break;
        case DN_INIFieldType_USize:  DN_INI_Str8BuilderAppend(&builder, "%zu\n", field->value_usize); break;
      }
    }

    if (it->next)
      section_stack[section_stack_count++] = it->next;
    if (it->child_first)
      section_stack[section_stack_count++] = it->child_first;

    if (section_stack_count)
      DN_INI_Str8BuilderAppend(&builder, "\n");
  }

  result.size_req = builder.size_req;
  if (buffer) {
    result.str8.data = builder.data;
    result.str8.size = builder.used;
    result.success   = true;
  } else {
    result.success = true;
  }

  return result;
}

#if defined(DN_INI_WITH_UNIT_TESTS) || 1
void DN_INI_UnitTests()
{
  // NOTE: Section and comments
  {
    char const EXAMPLE[] =
        "[metadata]\n"
        "  name = this8 # test\n"
        "  # continue the comment\n"
        "  version = attr: this8.__version__\n"
        "\n";

    DN_INICore parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, 0, 0);
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
    DN_INI_Assert(parse.total_sections_count == 1);
    DN_INI_Assert(parse.total_fields_count == 2);

    char parse_memory[sizeof(DN_INIField) * 2 + sizeof(DN_INISection) * 1];
    parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, parse_memory, sizeof(parse_memory));
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
  }

  // NOTE: Global section
  {
    char const EXAMPLE[] =
        "first=hello\n"
        "[metadata]\n"
        "  name = this8 # test\n"
        "  version = attr: this8.__version__\n"
        "\n";

    DN_INICore parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, 0, 0);
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
    DN_INI_Assert(parse.total_sections_count == 1);
    DN_INI_Assert(parse.total_fields_count == 3);

    char parse_memory[sizeof(DN_INIField) * 3 + sizeof(DN_INISection) * 1];
    parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, parse_memory, sizeof(parse_memory));
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
  }

  // NOTE: Empty section
  {
    char const EXAMPLE[] =
        "first=hello\n"
        "[metadata]\n\n";

    DN_INICore parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, 0, 0);
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
    DN_INI_Assert(parse.total_sections_count == 1);
    DN_INI_Assert(parse.total_fields_count == 1);

    char parse_memory[sizeof(DN_INIField) * 1 + sizeof(DN_INISection) * 2];
    parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, parse_memory, sizeof(parse_memory));
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
  }

  // NOTE: Multiple empty sections
  {
    char const EXAMPLE[] =
        "[metadata]\n\n"
        "[metadata2]\n\n";

    DN_INICore parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, 0, 0);
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
    DN_INI_Assert(parse.total_sections_count == 2);
    DN_INI_Assert(parse.total_fields_count == 0);

    char parse_memory[sizeof(DN_INIField) * 0 + sizeof(DN_INISection) * 2];
    parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, parse_memory, sizeof(parse_memory));
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
  }

  // NOTE: Repeated section override
  {
    char const EXAMPLE[] =
        "[metadata]\n"
        "foo=bar\n"
        "[metadata]\n"
        "foo=baz\n";

    DN_INICore parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, 0, 0);
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
    // NOTE: Because sections can override each other, when parsing with no memory, e.g. no context
    // we can't easily tell if a section is repeated or not without retokenising the entire file
    // every time we hit a section. Then the total section count returned in the initial pass is
    // an estimate. The same goes with the key-values
    DN_INI_Assert(parse.total_sections_count == 2);
    DN_INI_Assert(parse.total_fields_count == 2);

    char parse_memory[sizeof(DN_INIField) * 1 + sizeof(DN_INISection) * 1];
    parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, parse_memory, sizeof(parse_memory));
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
    DN_INI_Assert(parse.total_sections_count == 1);
    DN_INI_Assert(parse.total_fields_count == 1);
    DN_INI_Assert(parse.first_section.child_first);
    DN_INI_Assert(parse.first_section.child_first->first_field);
    DN_INI_Assert(parse.first_section.child_first->first_field == parse.first_section.child_first->last_field);
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->first_field->value, DN_INIStr8Lit("baz")));
  }

  // NOTE: Out-of-order repeated section override
  {
    char const EXAMPLE[] =
        "[metadata]\n"
        "foo=bar\n"
        "[surprise]"
        "[metadata]\n"
        "foo=baz\n";

    DN_INICore parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, 0, 0);
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
    DN_INI_Assert(parse.total_sections_count == 3);
    DN_INI_Assert(parse.total_fields_count == 2);

    char parse_memory[sizeof(DN_INIField) * 1 + sizeof(DN_INISection) * 2];
    parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, parse_memory, sizeof(parse_memory));
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
    DN_INI_Assert(parse.total_sections_count == 2);
    DN_INI_Assert(parse.total_fields_count == 1);
    DN_INI_Assert(parse.first_section.child_first);
    DN_INI_Assert(parse.first_section.child_first->first_field);
    DN_INI_Assert(parse.first_section.child_first->first_field == parse.first_section.child_first->last_field);
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->first_field->value, DN_INIStr8Lit("baz")));
  }

  // NOTE: Subsection
  {
    char const EXAMPLE[] =
        "[metadata]\n"
        "foo=bar\n"
        "[metadata.test]\n"
        "hello=world\n";

    DN_INICore parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, 0, 0);
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
    DN_INI_Assert(parse.total_sections_count == 3);
    DN_INI_Assert(parse.total_fields_count == 2);

    char parse_memory[sizeof(DN_INIField) * 2 + sizeof(DN_INISection) * 3];
    parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, parse_memory, sizeof(parse_memory));

    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->name, DN_INIStr8Lit("metadata")));
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->first_field->key, DN_INIStr8Lit("foo")));
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->first_field->value, DN_INIStr8Lit("bar")));

    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->child_first->name, DN_INIStr8Lit("test")));
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->child_first->first_field->key, DN_INIStr8Lit("hello")));
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->child_first->first_field->value, DN_INIStr8Lit("world")));
  }

  // NOTE: Repeated subsections
  {
    char const EXAMPLE[] =
        "[metadata]\n"
        "foo=bar\n"
        "[metadata.test]\n"
        "foo=bar\n"
        "\n"
        "[metadata]\n"
        "foo=baz\n"
        "[metadata.test]\n"
        "foo=baz\n";

    DN_INICore parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, 0, 0);
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
    DN_INI_Assert(parse.total_sections_count == 6);
    DN_INI_Assert(parse.total_fields_count == 4);

    char parse_memory[sizeof(DN_INIField) * 2 + sizeof(DN_INISection) * 2];
    parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, parse_memory, sizeof(parse_memory));
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);

    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->name, DN_INIStr8Lit("metadata")));
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->first_field->key, DN_INIStr8Lit("foo")));
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->first_field->value, DN_INIStr8Lit("baz")));

    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->child_first->name, DN_INIStr8Lit("test")));
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->child_first->first_field->key, DN_INIStr8Lit("foo")));
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->child_first->first_field->value, DN_INIStr8Lit("baz")));

    DN_INIField *field = DN_INI_FieldFromSectionStr8(&parse.first_section, DN_INIStr8Lit("metadata.test.foo"));
    DN_INI_Assert(DN_INI_Str8Eq(field->key,   DN_INIStr8Lit("foo")));
    DN_INI_Assert(DN_INI_Str8Eq(field->value, DN_INIStr8Lit("baz")));
  }

  // NOTE: Multi line value
  {
    char const EXAMPLE[] =
        "[metadata]\n"
        "foo=bar \\\n"
        "baz\n"
        "abc=def \\\n"
        "ghi\\\n"
        "j"
        ;

    DN_INICore parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, 0, 0);
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
    DN_INI_Assert(parse.total_sections_count == 1);
    DN_INI_Assert(parse.total_fields_count == 2);

    char parse_memory[300];
    DN_INI_Assert(parse.memory_required <= sizeof(parse_memory));

    parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, parse_memory, sizeof(parse_memory));
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->first_field->key, DN_INIStr8Lit("foo")));
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->first_field->value, DN_INIStr8Lit("bar baz")));

    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->last_field->key, DN_INIStr8Lit("abc")));
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->last_field->value, DN_INIStr8Lit("def ghij")));
  }

  // NOTE: Multi line immediately after key-value separator
  {
    char const EXAMPLE[] =
        "[metadata]\n"
        "foo=\\\n"
        "baz\\\n"
        "j"
        ;

    DN_INICore parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, 0, 0);
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
    DN_INI_Assert(parse.total_sections_count == 1);
    DN_INI_Assert(parse.total_fields_count == 1);

    char parse_memory[256];
    DN_INI_Assert(parse.memory_required <= sizeof(parse_memory));

    parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, parse_memory, sizeof(parse_memory));
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->first_field->key, DN_INIStr8Lit("foo")));
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->first_field->value, DN_INIStr8Lit("bazj")));
  }

  // NOTE: Empty section
  {
    char const EXAMPLE[] =
        "[metadata]\n"
        "foo=\n"
        "[empty]\n"
        ;

    DN_INICore parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, 0, 0);
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
    DN_INI_Assert(parse.total_sections_count == 2);
    DN_INI_Assert(parse.total_fields_count == 1);

    char parse_memory[400];
    DN_INI_Assert(parse.memory_required <= sizeof(parse_memory));

    parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, parse_memory, sizeof(parse_memory));
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->first_field->key, DN_INIStr8Lit("foo")));
  }
}
#endif
