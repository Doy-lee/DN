#if !defined(DN_INI_H)
#define DN_INI_H

#include <stddef.h> // size_t
#if !defined(DN_INI_Assert)
  #include <assert.h>
  #define DN_INI_Assert(expr) assert(expr)
#endif

typedef enum DN_INITokenType {
  DN_INITokenType_Nil,
  DN_INITokenType_Section,
  DN_INITokenType_Key,
  DN_INITokenType_KeyValueSeparator,
  DN_INITokenType_Value,
  DN_INITokenType_Comment,
  DN_INITokenType_EndOfStream,
  DN_INITokenType_Error,
} DN_INITokenType;

typedef struct DN_INIStr8 {
  char  *data;
  size_t size;
} DN_INIStr8;
#define DN_INIStr8Lit(str) DN_INIStr8{(char *)str, sizeof(str)/sizeof(str[0]) - 1}

typedef struct DN_INIToken {
  DN_INITokenType type;
  char           *data;
  size_t          count;
  size_t          next_p;

  // NOTE: Line metadata
  DN_INIStr8      error;
  size_t          line;
  size_t          column;
  char           *line_start;
} DN_INIToken;

typedef struct DN_INITokeniser {
  char           *data;
  char           *line_start;
  size_t          count;
  size_t          pos;
  DN_INITokenType prev_token;
  size_t          line;
  size_t          column;
} DN_INITokeniser;

typedef struct DN_INIKeyValue {
  DN_INIStr8 key;
  DN_INIStr8 value;
} DN_INIKeyValue;

typedef struct DN_INISection {
  DN_INIStr8      name;
  DN_INIKeyValue *key_values;
  size_t          key_values_count;
  DN_INIToken     token;
} DN_INISection;

typedef struct DN_INIParse {
  DN_INISection *sections;
  size_t         sections_count;
  size_t         key_values_count;
  DN_INIToken    error_token;
} DN_INIParse;

DN_INITokeniser DN_INI_TokeniserFromPtr(char const *buf, size_t count);
DN_INIToken     DN_INI_NextToken       (DN_INITokeniser const *tokeniser);
void            DN_INI_EatToken        (DN_INITokeniser *tokeniser, DN_INIToken token);
DN_INIParse     DN_INI_ParseFromBuffer (char const *buf, size_t count, DN_INISection *sections, size_t sections_count, DN_INIKeyValue *key_values, size_t key_values_count);

#if defined(DN_INI_WITH_UNIT_TESTS)
void            DN_INI_UnitTests       ();
#endif
#endif // !defined(DN_INI_H)
