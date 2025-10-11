#if !defined(DN_INI_H)
#define DN_INI_H

#include <stdint.h> // uint32_t

#if !defined(DN_INI_Assert)
  #include <assert.h>
  #define DN_INI_Assert(expr) assert(expr)
#endif

#if !defined(DN_INI_Memset) || !defined(DN_INI_Memcmp) || !defined(DN_INI_Memcpy)
  #include <string.h>
  #if !defined(DN_INI_Memset)
  #define DN_INI_Memset(ptr, val, size) memset(ptr, val, size)
  #endif

  #if !defined(DN_INI_Memcmp)
  #define DN_INI_Memcmp(dest, src, size) memcmp(dest, src, size)
  #endif

  #if !defined(DN_INI_Memcpy)
  #define DN_INI_Memcpy(dest, src, size) memcpy(dest, src, size)
  #endif
#endif

typedef enum DN_INITokenType {
  DN_INITokenType_Nil,
  DN_INITokenType_Section,
  DN_INITokenType_Key,
  DN_INITokenType_KeyValueSeparator,
  DN_INITokenType_MultilineValue,
  DN_INITokenType_Value,
  DN_INITokenType_Comment,
  DN_INITokenType_EndOfStream,
  DN_INITokenType_Error,
} DN_INITokenType;

typedef struct DN_INIStr8 {
  char    *data;
  uint32_t size;
} DN_INIStr8;

#if defined(__cplusplus)
  #define DN_INIStr8Lit(str) DN_INIStr8{(char *)str, sizeof(str)/sizeof(str[0]) - 1}
#else
  #define DN_INIStr8Lit(str) (DN_INIStr8){(char *)str, sizeof(str)/sizeof(str[0]) - 1}
#endif

typedef struct DN_INIToken {
  char           *data;
  DN_INITokenType type;
  uint32_t        count;
  uint32_t        next_p;
  bool            new_line;

  // NOTE: Line metadata
  DN_INIStr8      error;
  uint32_t        line;
  uint32_t        column;
  char           *line_start;
} DN_INIToken;

typedef struct DN_INITokeniser {
  char           *data;
  char           *line_start;
  uint32_t        count;
  uint32_t        pos;
  DN_INIToken     prev_token;
  uint32_t        line;
  uint32_t        column;
} DN_INITokeniser;

typedef struct DN_INIKeyValue DN_INIKeyValue;
struct DN_INIKeyValue {
  DN_INIStr8      key;
  DN_INIStr8      value;
  DN_INIKeyValue *next;
};

typedef struct DN_INISection DN_INISection;
struct DN_INISection {
  DN_INIStr8      name;
  DN_INIKeyValue *first_key_value;
  DN_INIKeyValue *last_key_value;
  uint32_t        key_values_count;
  DN_INIToken     token;
  DN_INISection  *next, *parent;
  DN_INISection  *child_first, *child_last;
};

typedef struct DN_INIParse {
  DN_INISection first_section;
  uint32_t      total_sections_count;
  uint32_t      total_key_values_count;
  DN_INIToken   error_token;
  uint32_t      memory_required;
} DN_INIParse;

DN_INITokeniser DN_INI_TokeniserFromPtr  (char const *buf, uint32_t count);
DN_INIToken     DN_INI_NextToken         (DN_INITokeniser const *tokeniser);
void            DN_INI_EatToken          (DN_INITokeniser *tokeniser, DN_INIToken token);
DN_INISection * DN_INI_FindSectionStr8   (DN_INISection *section, DN_INIStr8 str8);
DN_INISection * DN_INI_FindSection       (DN_INISection *section, char const *name, uint32_t name_size);
DN_INIKeyValue *DN_INI_KeyFromSectionStr8(DN_INISection *section, DN_INIStr8 str8);
DN_INIKeyValue *DN_INI_KeyFromSection    (DN_INISection *section, char const *key, uint32_t key_size);
DN_INIParse     DN_INI_ParseFromPtr      (char const *buf, uint32_t count, char *base, uint32_t base_count);

#if defined(DN_INI_WITH_UNIT_TESTS)
void            DN_INI_UnitTests       ();
#endif
#endif // !defined(DN_INI_H)
