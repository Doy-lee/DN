#include "dn_ini.h"

#if defined(__cplusplus__)
#include <stdbool.h>
#endif

static bool DN_INI_CharIsWhitespace_(char ch)
{
  bool result = ch == ' ' || ch == '\r' || ch == '\n' || ch == '\t';
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
  size_t      pos    = tokeniser->pos;
  DN_INIToken result = {};
  result.line        = tokeniser->line;
  result.line_start  = tokeniser->line_start ? tokeniser->line_start : tokeniser->data;
  result.next_p      = tokeniser->count;

  while (result.type == DN_INITokenType_Nil) {
    if (tokeniser->pos < tokeniser->count && DN_INI_CharIsWhitespace_(tokeniser->data[pos])) {
      if (tokeniser->data[pos++] == '\n') {
        result.line++;
        result.line_start = tokeniser->data + pos;
      }
      continue;
    }

    if (pos >= tokeniser->count) {
      if (tokeniser->prev_token == DN_INITokenType_Nil || tokeniser->prev_token == DN_INITokenType_Value || tokeniser->prev_token == DN_INITokenType_Comment || tokeniser->prev_token == DN_INITokenType_Value || tokeniser->prev_token == DN_INITokenType_KeyValueSeparator) {
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
          result.count  = (tokeniser->data + pos) - result.data;
        }
      } break;

      case '#': {
        if (tokeniser->prev_token != DN_INITokenType_Nil && tokeniser->prev_token != DN_INITokenType_Comment && tokeniser->prev_token != DN_INITokenType_Value && tokeniser->prev_token != DN_INITokenType_Section) {
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
              result.count = (tokeniser->data + pos) - result.data;
              result.next_p = pos;
            }
          }
        }
      } break;

      case '=': {
        if (tokeniser->prev_token == DN_INITokenType_Key) {
          result.type = DN_INITokenType_KeyValueSeparator;
        } else {
          result.type = DN_INITokenType_Error;
          result.error = DN_INIStr8Lit("Invalid key-value separator, '=' is not being used to separate a key-value pair");
        }
        result.data   = (char *)tokeniser->data + pos;
        result.next_p = pos + 1;
        result.count  = 1;
      } break;

      case '"': {
        result.data = (char *)tokeniser->data + pos + 1;
        while (result.type == DN_INITokenType_Nil) {
          pos++;
          if (pos >= tokeniser->count) {
            result.type  = DN_INITokenType_EndOfStream;
            result.count = (tokeniser->data + pos) - result.data;
          } else if (tokeniser->data[pos] == '"') {
            if (tokeniser->prev_token == DN_INITokenType_KeyValueSeparator) {
              result.type  = DN_INITokenType_Value;
            } else {
              result.type = DN_INITokenType_Error;
              result.error = DN_INIStr8Lit("Invalid quoted string, value was not preceeded by a key-value separator");
            }
            result.count = (tokeniser->data + pos) - result.data;
            result.next_p = pos + 1;
          }
        }
      } break;

      default: {
        result.data = (char *)tokeniser->data + pos;
        while (result.type == DN_INITokenType_Nil) {
          pos++;
          bool end_of_stream = pos >= tokeniser->count;
          if (end_of_stream || DN_INI_CharIsWhitespace_(tokeniser->data[pos]) || tokeniser->data[pos] == '#') {
            if (result.type == DN_INITokenType_Nil) {
              if (tokeniser->prev_token == DN_INITokenType_KeyValueSeparator) {
                if (tokeniser->data[pos] == ' ') // Value can have spaces in it without quotes
                  continue;
                result.type = DN_INITokenType_Value;
              } else if (tokeniser->prev_token == DN_INITokenType_Key) {
                result.type  = DN_INITokenType_Error;
                result.error = DN_INIStr8Lit("Invalid unquoted string, multiple consecutive keys encountered");
              } else {
                result.type = DN_INITokenType_Key;
              }
            }

            result.count  = (tokeniser->data + pos) - result.data;
            result.next_p = pos;

            if (result.type == DN_INITokenType_Value && tokeniser->data[pos] == '#') {
              while (result.count && DN_INI_CharIsWhitespace_(result.data[result.count - 1]))
                result.count--;
            }
          }
        }
      } break;
    }
  }

  result.column = result.data - result.line_start;
  return result;
}

void DN_INI_EatToken(DN_INITokeniser *tokeniser, DN_INIToken token)
{
  DN_INI_Assert(token.data >= tokeniser->data && token.data <= tokeniser->data + tokeniser->count);
  DN_INI_Assert(tokeniser->pos <= tokeniser->count);
  tokeniser->pos        = token.next_p;
  tokeniser->prev_token = token.type;
  tokeniser->line       = token.line;
  tokeniser->column     = token.column;
  tokeniser->line_start = token.line_start;
}

DN_INIParse DN_INI_ParseFromBuffer(char const *buf, size_t count, DN_INISection *sections, size_t sections_count, DN_INIKeyValue *key_values, size_t key_values_count)
{
  DN_INIParse result        = {};
  result.sections           = sections;
  DN_INITokeniser tokeniser = DN_INI_TokeniserFromPtr(buf, count);
  DN_INIKeyValue *next_key_value = key_values;
  for (;;) {
    DN_INIToken token = DN_INI_NextToken(&tokeniser);
    if (token.type == DN_INITokenType_EndOfStream)
      break;

    if (token.type == DN_INITokenType_Error) {
      result.error_token = token;
      // fprintf(stderr, "ERROR: INI parsing failed at %zu:%zu: %.*s. String was '%.*s'\n", token.line, token.column, (int)token.error.size, token.error.data, (int)token.count, token.data);
      break;
    }

    DN_INI_EatToken(&tokeniser, token);
    switch (token.type) {
      case DN_INITokenType_EndOfStream:       /*FALLTHRU*/
      case DN_INITokenType_Error:             /*FALLTHRU*/
      case DN_INITokenType_Nil:               DN_InvalidCodePath; break;
      case DN_INITokenType_KeyValueSeparator: break;
      case DN_INITokenType_Comment:           break;

      case DN_INITokenType_Section: {
        result.sections_count++;
        if (result.sections_count <= sections_count) {
          DN_INISection *section = sections + (result.sections_count - 1);
          section->name.data     = token.data;
          section->name.size     = token.count;
          section->token         = token;
          section->key_values    = next_key_value;
        }
      } break;

      case DN_INITokenType_Key: {
        result.key_values_count++;
        if (result.sections_count <= sections_count && result.key_values_count <= key_values_count) {
          DN_INISection  *section   = sections + result.sections_count - 1;
          DN_INIKeyValue *key_value = next_key_value++;
          key_value->key.data       = token.data;
          key_value->key.size       = token.count;
          section->key_values_count++;
        }
      } break;

      case DN_INITokenType_Value: {
        if (result.sections_count <= sections_count && result.key_values_count <= key_values_count) {
          DN_INISection  *section   = sections + result.sections_count - 1;
          DN_INIKeyValue *key_value = section->key_values + (section->key_values_count - 1);
          DN_INI_Assert(section->key_values_count);
          DN_INI_Assert(key_value->key.size);
          DN_INI_Assert(!key_value->value.data);
          key_value->value.data = token.data;
          key_value->value.size = token.count;
        }
      } break;

    }
  }
  return result;
}

#if defined(DN_INI_WITH_UNIT_TESTS) || 1
void DN_INI_UnitTests()
{
  {
    char const EXAMPLE[] =
        "[metadata]\n"
        "  name = this8 # test\n"
        "  # continue the comment\n"
        "  version = attr: this8.__version__\n"
        "\n";

    DN_INIParse parse = DN_INI_ParseFromBuffer(EXAMPLE, sizeof(EXAMPLE) - 1, 0, 0, 0, 0);
    DN_INI_Assert(parse.sections_count == 1);
    DN_INI_Assert(parse.key_values_count == 2);
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);

    DN_INISection  sections[128]   = {};
    DN_INIKeyValue key_values[128] = {};
    parse                          = DN_INI_ParseFromBuffer(EXAMPLE, sizeof(EXAMPLE) - 1, sections, sizeof(sections) / sizeof(sections[0]), key_values, sizeof(key_values) / sizeof(key_values[0]));
    DN_INI_Assert(parse.error_token.type == DN_INITokenType_Nil);
  }
}
#endif
