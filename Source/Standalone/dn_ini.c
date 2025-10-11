#include "dn_ini.h"

#if !defined(__cplusplus__)
  #include <stdbool.h>
#endif

#include <stdio.h>

typedef struct DN_INIArena {
  char  *base;
  size_t used, max;
} DN_INIArena;

typedef struct DN_INIStr8BSplit {
  DN_INIStr8 lhs;
  DN_INIStr8 rhs;
} DN_INIStr8BSplit;

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

static DN_INIStr8 DN_INI_Str8FromPtr(char const *data, uint32_t count)
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

static DN_INIStr8 DN_INI_Str8Slice(DN_INIStr8 slice, uint32_t offset, uint32_t size)
{
  DN_INIStr8 result = {};
  if (slice.data) {
    uint32_t max_offset   = slice.size;
    uint32_t final_offset = offset <= max_offset ? offset : max_offset;
    uint32_t max_size     = slice.size - final_offset;
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
      result.lhs        = DN_INI_Str8FromPtr(str8.data, (uint32_t)index);
      uint32_t rhs_size = (uint32_t)(str8.size - (index + 1));
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
      result.lhs        = DN_INI_Str8FromPtr(str8.data, (uint32_t)index);
      uint32_t rhs_size = (uint32_t)(str8.size - index - find.size);
      DN_INI_Assert(rhs_size < str8.size);
      result.rhs = DN_INI_Str8FromPtr(str8.data + index + find.size, rhs_size);
      break;
    }
  }

  if (!result.lhs.data)
    result.lhs = str8;
  return result;
}

DN_INITokeniser DN_INI_TokeniserFromPtr(char const *buf, uint32_t count)
{
  DN_INITokeniser result = {};
  result.data            = (char *)buf;
  result.count           = count;
  return result;
}

DN_INIToken DN_INI_NextToken(DN_INITokeniser const *tokeniser)
{
  uint32_t    pos    = tokeniser->pos;
  DN_INIToken result = {};
  result.line        = tokeniser->line;
  result.line_start  = tokeniser->line_start ? tokeniser->line_start : tokeniser->data;
  result.next_p      = tokeniser->count;

  while (result.type == DN_INITokenType_Nil) {
    if (tokeniser->pos < tokeniser->count && DN_INI_CharIsWhitespace_(tokeniser->data[pos])) {
      if (tokeniser->data[pos++] == '\n') {
        result.line++;
        result.line_start = tokeniser->data + pos;
        result.new_line   = true;
      }
      continue;
    }

    if (pos >= tokeniser->count) {
      if (tokeniser->prev_token.type == DN_INITokenType_Nil               ||
          tokeniser->prev_token.type == DN_INITokenType_Value             ||
          tokeniser->prev_token.type == DN_INITokenType_Comment           ||
          tokeniser->prev_token.type == DN_INITokenType_Value             ||
          tokeniser->prev_token.type == DN_INITokenType_KeyValueSeparator ||
          tokeniser->prev_token.type == DN_INITokenType_MultilineValue    ||
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
          result.count  = (uint32_t)((tokeniser->data + pos) - result.data);
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
              result.count = (uint32_t)((tokeniser->data + pos) - result.data);
              result.next_p = pos;
            }
          }
        }
      } break;

      case '=': {
        if (tokeniser->prev_token.type == DN_INITokenType_Key) {
          result.type = DN_INITokenType_KeyValueSeparator;
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

          if (end_of_stream ||
              DN_INI_CharIsWhitespace_(tokeniser->data[pos]) ||
              tokeniser->data[pos] == '#' ||
              tokeniser->data[pos] == '\\' ||
              tokeniser->data[pos] == '=' ||
              end_of_quote) {

            uint32_t next_p = pos;
            if (end_of_quote) {
              next_p = pos + 1;
              DN_INI_Assert(!end_of_stream);
              DN_INI_Assert(tokeniser->data[pos] == '"');
            }

            if (!end_of_stream && tokeniser->data[pos] == '\\') {
              if (tokeniser->prev_token.type != DN_INITokenType_KeyValueSeparator &&
                  tokeniser->prev_token.type != DN_INITokenType_Value &&
                  tokeniser->prev_token.type != DN_INITokenType_MultilineValue) {
                result.type   = DN_INITokenType_Error;
                result.error  = DN_INIStr8Lit("Invalid unquoted string, escape character '\\' is only allowed in INI values");
                result.count  = (uint32_t)((tokeniser->data + pos) - result.data);
                result.next_p = next_p;
                break;
              }

              DN_INIStr8 esc_str8 = DN_INI_Str8Slice(DN_INI_Str8FromPtr(tokeniser->data, tokeniser->count), pos + 1, 1);
              if (DN_INI_Str8Eq(esc_str8, DN_INIStr8Lit("\n")))
                next_p += 2;
              else
                next_p += 1;
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
            bool multiline_value = tokeniser->prev_token.type == DN_INITokenType_MultilineValue && !result.new_line;

            if (tokeniser->prev_token.type == DN_INITokenType_KeyValueSeparator || multiline_value || tokeniser->prev_token.type == DN_INITokenType_Value) {
              if (tokeniser->data[pos] == ' ') // Value can have spaces in it without quotes
                continue;
              result.type = tokeniser->prev_token.type == DN_INITokenType_KeyValueSeparator ? DN_INITokenType_Value : DN_INITokenType_MultilineValue;
            } else if (tokeniser->prev_token.type == DN_INITokenType_Key) {
              result.type  = DN_INITokenType_Error;
              result.error = DN_INIStr8Lit("Invalid unquoted string, multiple consecutive keys encountered");
            } else {
              result.type = DN_INITokenType_Key;
            }

            result.count  = (uint32_t)((tokeniser->data + pos) - result.data);
            result.next_p = next_p;

            if (result.type == DN_INITokenType_Value && tokeniser->data[pos] == '#')
              while (result.count && DN_INI_CharIsWhitespace_(result.data[result.count - 1]))
                result.count--;
          }
        }
      } break;
    }
  }

  result.column = (uint32_t)(result.data - result.line_start);
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

DN_INISection *DN_INI_FindSectionStr8(DN_INISection *section, DN_INIStr8 str8)
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

DN_INISection *DN_INI_FindSection(DN_INISection *section, char const *name, uint32_t name_size)
{
  DN_INISection *result = DN_INI_FindSectionStr8(section, DN_INI_Str8FromPtr(name, name_size));
  return result;
}

DN_INIKeyValue *DN_INI_KeyFromSectionStr8(DN_INISection *section, DN_INIStr8 str8)
{
  DN_INIKeyValue *result = 0;
  if (section) {
    DN_INIStr8BSplit split        = DN_INI_Str8BSplitReverse(str8, DN_INIStr8Lit("."));
    DN_INIStr8       find_key     = str8;
    DN_INISection   *find_section = section;
    if (split.rhs.size) {
      find_section = DN_INI_FindSection(section, split.lhs.data, split.lhs.size);
      find_key     = split.rhs;
    }

    if (find_section) {
      for (DN_INIKeyValue *it = find_section->first_key_value; !result && it; it = it->next)
        if (DN_INI_Str8Eq(it->key, find_key))
          result = it;
    }
  }
  return result;
}

DN_INIKeyValue *DN_INI_KeyFromSection(DN_INISection *section, char const *key, uint32_t key_size)
{
  DN_INIKeyValue *result = DN_INI_KeyFromSectionStr8(section, DN_INI_Str8FromPtr(key, key_size));
  return result;
}

DN_INIParse DN_INI_ParseFromPtr(char const *buf, uint32_t count, char *base, uint32_t base_count)
{
  DN_INITokeniser tokeniser = DN_INI_TokeniserFromPtr(buf, count);
  DN_INIArena     arena     = {};
  arena.base                = base;
  arena.max                 = base_count;

  DN_INIParse     result       = {};
  DN_INISection  *curr_section = &result.first_section;
  DN_INIKeyValue *key_value    = 0;
  for (;;) {
    DN_INIToken token = DN_INI_NextToken(&tokeniser);
    if (token.type == DN_INITokenType_EndOfStream)
      break;

    if (token.type == DN_INITokenType_Error) {
      result.error_token = token;
      // fprintf(stderr, "ERROR: INI parsing failed at %zu:%zu: %.*s. String was '%.*s'\n", token.line, token.column, (int)token.error.size, token.error.data, (int)token.count, token.data);
      break;
    }

    switch (token.type) {
      case DN_INITokenType_EndOfStream:       /*FALLTHRU*/
      case DN_INITokenType_Error:             /*FALLTHRU*/
      case DN_INITokenType_Nil:               DN_INI_Assert(!"Invalid code path"); break;
      case DN_INITokenType_KeyValueSeparator: break;
      case DN_INITokenType_Comment:           break;

      case DN_INITokenType_Section: {
        DN_INISection *parent       = &result.first_section;
        DN_INIStr8     section_name = DN_INI_Str8FromPtr(token.data, token.count);
        curr_section                = &result.first_section;
        for (;;) {
          DN_INIStr8BSplit split = DN_INI_Str8BSplit(section_name, DN_INIStr8Lit("."));
          if (split.lhs.size == 0)
            break;

          DN_INISection *next_section = DN_INI_FindSection(parent, split.lhs.data, split.lhs.size);
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
        key_value = DN_INI_KeyFromSection(curr_section, token.data, token.count);
        if (!key_value) {
          result.total_key_values_count++;
          key_value = (DN_INIKeyValue *)DN_INI_ArenaAlloc(&arena, sizeof(*key_value));
          if (base && !key_value) {
            result.error_token = DN_INI_MakeParseOutOfMemoryErrorToken_(token);
            return result;
          }

          if (key_value) {
            key_value->key.data = token.data;
            key_value->key.size = token.count;
          }

          if (curr_section) {
            if (!curr_section->first_key_value)
              curr_section->first_key_value = key_value;
            if (curr_section->last_key_value)
              curr_section->last_key_value->next = key_value;
            curr_section->last_key_value = key_value;
            curr_section->key_values_count++;
          }
        }
      } break;

      case DN_INITokenType_MultilineValue: {
        uint32_t bytes_req = token.count;
        if (tokeniser.prev_token.type == DN_INITokenType_Value) {
          // NOTE: We saw a value, then the next token was a multiline value, we will merge these
          // values into 1 stream, so we need to copy the previous string out as well.
          bytes_req += tokeniser.prev_token.count;
        }

        result.memory_required += bytes_req;
        if (curr_section && key_value) {
          DN_INI_Assert(curr_section->key_values_count);
          DN_INI_Assert(key_value->key.size);

          char *string = (char *)DN_INI_ArenaAlloc(&arena, bytes_req);
          if (!string) {
            result.error_token = DN_INI_MakeParseOutOfMemoryErrorToken_(token);
            return result;
          }


          char *dest = string;
          if (tokeniser.prev_token.type == DN_INITokenType_Value) {
            DN_INI_Memcpy(dest, tokeniser.prev_token.data, tokeniser.prev_token.count);
            dest += tokeniser.prev_token.count;
            key_value->value.data = string;
            key_value->value.size = tokeniser.prev_token.count;
          } else {
            // NOTE: If we have a multi-line value we are accumulating onto the same key-value.
            // Invariant to this is that the arena is only being used to allocate contiguous memory
            // for the string. Essentially each time we visit this branch we're just bumping the
            // capacity of the original string we allocated at the start of the multi-line value.
            // This is what this assert checks, that we're expanding in place the string and there
            // hasn't been some other allocation that took place inbetween that broke continuity.
            DN_INI_Assert(key_value->value.data + key_value->value.size == string);
          }

          DN_INI_Memcpy(dest, token.data, token.count);
          key_value->value.size += token.count;
        }
      } break;

      case DN_INITokenType_Value: {
        if (curr_section && key_value) {
          DN_INI_Assert(curr_section->key_values_count);
          DN_INI_Assert(key_value->key.size);
          key_value->value.data = token.data;
          key_value->value.size = token.count;
        }
      } break;
    }
    DN_INI_EatToken(&tokeniser, token);
  }

  result.memory_required += (result.total_sections_count * sizeof(DN_INISection)) + (result.total_key_values_count * sizeof(DN_INIKeyValue));
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

    DN_INIParse parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, 0, 0);
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
    DN_INI_Assert(parse.total_sections_count == 1);
    DN_INI_Assert(parse.total_key_values_count == 2);

    char parse_memory[sizeof(DN_INIKeyValue) * 2 + sizeof(DN_INISection) * 1];
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

    DN_INIParse parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, 0, 0);
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
    DN_INI_Assert(parse.total_sections_count == 1);
    DN_INI_Assert(parse.total_key_values_count == 3);

    char parse_memory[sizeof(DN_INIKeyValue) * 3 + sizeof(DN_INISection) * 1];
    parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, parse_memory, sizeof(parse_memory));
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
  }

  // NOTE: Empty section
  {
    char const EXAMPLE[] =
        "first=hello\n"
        "[metadata]\n\n";

    DN_INIParse parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, 0, 0);
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
    DN_INI_Assert(parse.total_sections_count == 1);
    DN_INI_Assert(parse.total_key_values_count == 1);

    char parse_memory[sizeof(DN_INIKeyValue) * 1 + sizeof(DN_INISection) * 2];
    parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, parse_memory, sizeof(parse_memory));
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
  }

  // NOTE: Multiple empty sections
  {
    char const EXAMPLE[] =
        "[metadata]\n\n"
        "[metadata2]\n\n";

    DN_INIParse parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, 0, 0);
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
    DN_INI_Assert(parse.total_sections_count == 2);
    DN_INI_Assert(parse.total_key_values_count == 0);

    char parse_memory[sizeof(DN_INIKeyValue) * 0 + sizeof(DN_INISection) * 2];
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

    DN_INIParse parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, 0, 0);
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
    // NOTE: Because sections can override each other, when parsing with no memory, e.g. no context
    // we can't easily tell if a section is repeated or not without retokenising the entire file
    // every time we hit a section. Then the total section count returned in the initial pass is
    // an estimate. The same goes with the key-values
    DN_INI_Assert(parse.total_sections_count == 2);
    DN_INI_Assert(parse.total_key_values_count == 2);

    char parse_memory[sizeof(DN_INIKeyValue) * 1 + sizeof(DN_INISection) * 1];
    parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, parse_memory, sizeof(parse_memory));
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
    DN_INI_Assert(parse.total_sections_count == 1);
    DN_INI_Assert(parse.total_key_values_count == 1);
    DN_INI_Assert(parse.first_section.child_first);
    DN_INI_Assert(parse.first_section.child_first->first_key_value);
    DN_INI_Assert(parse.first_section.child_first->first_key_value == parse.first_section.child_first->last_key_value);
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->first_key_value->value, DN_INIStr8Lit("baz")));
  }

  // NOTE: Out-of-order repeated section override
  {
    char const EXAMPLE[] =
        "[metadata]\n"
        "foo=bar\n"
        "[surprise]"
        "[metadata]\n"
        "foo=baz\n";

    DN_INIParse parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, 0, 0);
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
    DN_INI_Assert(parse.total_sections_count == 3);
    DN_INI_Assert(parse.total_key_values_count == 2);

    char parse_memory[sizeof(DN_INIKeyValue) * 1 + sizeof(DN_INISection) * 2];
    parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, parse_memory, sizeof(parse_memory));
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
    DN_INI_Assert(parse.total_sections_count == 2);
    DN_INI_Assert(parse.total_key_values_count == 1);
    DN_INI_Assert(parse.first_section.child_first);
    DN_INI_Assert(parse.first_section.child_first->first_key_value);
    DN_INI_Assert(parse.first_section.child_first->first_key_value == parse.first_section.child_first->last_key_value);
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->first_key_value->value, DN_INIStr8Lit("baz")));
  }

  // NOTE: Subsection
  {
    char const EXAMPLE[] =
        "[metadata]\n"
        "foo=bar\n"
        "[metadata.test]\n"
        "hello=world\n";

    DN_INIParse parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, 0, 0);
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
    DN_INI_Assert(parse.total_sections_count == 3);
    DN_INI_Assert(parse.total_key_values_count == 2);

    char parse_memory[sizeof(DN_INIKeyValue) * 2 + sizeof(DN_INISection) * 3];
    parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, parse_memory, sizeof(parse_memory));

    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->name, DN_INIStr8Lit("metadata")));
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->first_key_value->key, DN_INIStr8Lit("foo")));
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->first_key_value->value, DN_INIStr8Lit("bar")));

    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->child_first->name, DN_INIStr8Lit("test")));
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->child_first->first_key_value->key, DN_INIStr8Lit("hello")));
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->child_first->first_key_value->value, DN_INIStr8Lit("world")));
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

    DN_INIParse parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, 0, 0);
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
    DN_INI_Assert(parse.total_sections_count == 6);
    DN_INI_Assert(parse.total_key_values_count == 4);

    char parse_memory[sizeof(DN_INIKeyValue) * 2 + sizeof(DN_INISection) * 2];
    parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, parse_memory, sizeof(parse_memory));
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);

    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->name, DN_INIStr8Lit("metadata")));
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->first_key_value->key, DN_INIStr8Lit("foo")));
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->first_key_value->value, DN_INIStr8Lit("baz")));

    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->child_first->name, DN_INIStr8Lit("test")));
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->child_first->first_key_value->key, DN_INIStr8Lit("foo")));
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->child_first->first_key_value->value, DN_INIStr8Lit("baz")));

    DN_INIKeyValue *key_value = DN_INI_KeyFromSectionStr8(&parse.first_section, DN_INIStr8Lit("metadata.test.foo"));
    DN_INI_Assert(DN_INI_Str8Eq(key_value->key,   DN_INIStr8Lit("foo")));
    DN_INI_Assert(DN_INI_Str8Eq(key_value->value, DN_INIStr8Lit("baz")));
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

    DN_INIParse parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, 0, 0);
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
    DN_INI_Assert(parse.total_sections_count == 1);
    DN_INI_Assert(parse.total_key_values_count == 2);

    char parse_memory[256];
    DN_INI_Assert(parse.memory_required <= sizeof(parse_memory));

    parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, parse_memory, sizeof(parse_memory));
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->first_key_value->key, DN_INIStr8Lit("foo")));
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->first_key_value->value, DN_INIStr8Lit("bar baz")));

    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->last_key_value->key, DN_INIStr8Lit("abc")));
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->last_key_value->value, DN_INIStr8Lit("def ghij")));
  }

  // NOTE: Multi line immediately after key-value separator
  {
    char const EXAMPLE[] =
        "[metadata]\n"
        "foo=\\\n"
        "baz\\\n"
        "j"
        ;

    DN_INIParse parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, 0, 0);
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
    DN_INI_Assert(parse.total_sections_count == 1);
    DN_INI_Assert(parse.total_key_values_count == 1);

    char parse_memory[256];
    DN_INI_Assert(parse.memory_required <= sizeof(parse_memory));

    parse = DN_INI_ParseFromPtr(EXAMPLE, sizeof(EXAMPLE) - 1, parse_memory, sizeof(parse_memory));
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->first_key_value->key, DN_INIStr8Lit("foo")));
    DN_INI_Assert(DN_INI_Str8Eq(parse.first_section.child_first->first_key_value->value, DN_INIStr8Lit("bazj")));
  }
}
#endif
