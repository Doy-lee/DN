#if !defined(DN_INI_H)
#define DN_INI_H

#include <stdint.h> // size_t

#if !defined(DN_INI_Assert)
  #include <assert.h>
  #define DN_INI_Assert(expr) assert(expr)
#endif

#include <stdarg.h>

#if !defined(DN_INI_VSNPrintF)
  #include <stdio.h>
  #define DN_INI_VSNPrintF(buffer, size, fmt, args) vsnprintf(buffer, size, fmt, args)
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
  DN_INITokenType_FieldSeparator,
  DN_INITokenType_MultilineValue,
  DN_INITokenType_Value,
  DN_INITokenType_Comment,
  DN_INITokenType_EndOfStream,
  DN_INITokenType_Error,
} DN_INITokenType;

typedef struct DN_INIStr8 {
  char    *data;
  size_t size;
} DN_INIStr8;

#if defined(__cplusplus)
  #define DN_INIStr8Lit(str) DN_INIStr8{(char *)str, sizeof(str)/sizeof(str[0]) - 1}
#else
  #define DN_INIStr8Lit(str) (DN_INIStr8){(char *)str, sizeof(str)/sizeof(str[0]) - 1}
#endif

typedef struct DN_INIToken {
  char           *data;
  DN_INITokenType type;
  size_t          count;
  size_t          next_p;
  bool            line_start_new_line;

  // NOTE: Line metadata
  DN_INIStr8      error;
  size_t          line;
  size_t          column;
  char      *     line_start;
} DN_INIToken;

typedef struct DN_INITokeniser {
  char       *data;
  char       *line_start;
  size_t      count;
  size_t      pos;
  DN_INIToken prev_token;
  size_t      line;
  size_t      column;
} DN_INITokeniser;

typedef enum DN_INIFieldType {
  DN_INIFieldType_String,
  DN_INIFieldType_Bool,
  DN_INIFieldType_USize,
} DN_INIFieldType;

typedef struct DN_INIField DN_INIField;
struct DN_INIField {
  DN_INIStr8      key;
  DN_INIFieldType value_type;
  DN_INIStr8      value;
  bool            value_bool;
  size_t          value_usize;
  DN_INIField    *next;
};

typedef struct DN_INISection DN_INISection;
struct DN_INISection {
  DN_INIStr8     name;
  DN_INIField   *first_field;
  DN_INIField   *last_field;
  size_t         fields_count;
  DN_INIToken    token;
  DN_INISection *next, *parent;
  DN_INISection *child_first, *child_last;
  size_t         child_count;
};

typedef struct DN_INICore DN_INICore;
struct DN_INICore {
  DN_INISection first_section;
  size_t        total_sections_count;
  size_t        total_fields_count;
  DN_INIToken   error_token;
  size_t        memory_required;
};

typedef struct DN_INIArena DN_INIArena;
struct DN_INIArena {
  char  *base;
  size_t used, max;
};

typedef struct DN_INIStr8FromResult DN_INIStr8FromResult;
struct DN_INIStr8FromResult {
  bool       success;
  DN_INIStr8 str8;
  size_t     size_req;
};

typedef struct DN_INIFieldUSize DN_INIFieldUSize;
struct DN_INIFieldUSize
{
  bool         success;
  DN_INIField *field;
  size_t       value;
};

typedef struct DN_INIFieldStr8 DN_INIFieldStr8;
struct DN_INIFieldStr8
{
  bool         success;
  DN_INIField *field;
  DN_INIStr8   value;
};

typedef struct DN_INIFieldBool DN_INIFieldBool;
struct DN_INIFieldBool
{
  bool         success;
  DN_INIField *field;
  bool         value;
};


// NOTE: Utilities
// Str8FromINI's `size` parameter must include space for the null-terminator, i.e:
// `DN_INIStr8FromResult.size_req + 1` bytes
int                  DN_INI_SNPrintF_                (char const *buffer, size_t size, char const *fmt, ...);
void *               DN_INI_ArenaAlloc               (DN_INIArena *arena, size_t size);
DN_INIStr8           DN_INI_Str8FromPtr              (char const *data, size_t count);
DN_INIStr8FromResult DN_INI_Str8FromINI              (DN_INICore const *ini, char *buffer, size_t size);

// NOTE: Tokeniser/Parsing
DN_INITokeniser      DN_INI_TokeniserFromPtr         (char const *buf, size_t count);
DN_INIToken          DN_INI_NextToken                (DN_INITokeniser const *tokeniser);
void                 DN_INI_EatToken                 (DN_INITokeniser *tokeniser, DN_INIToken token);

// NOTE: Lookup
DN_INISection *      DN_INI_ChildSectionFromStr8     (DN_INISection *section, DN_INIStr8 str8);
DN_INISection *      DN_INI_ChildSectionFromCStr     (DN_INISection *section, char const *name, size_t name_size);
DN_INIField *        DN_INI_FieldFromSectionStr8     (DN_INISection *section, DN_INIStr8 str8);
DN_INIField *        DN_INI_FieldFromSection         (DN_INISection *section, char const *key, size_t key_size);
DN_INIFieldUSize     DN_INI_FieldUSizeFromSectionStr8(DN_INISection *section, DN_INIStr8 str8);
DN_INIFieldStr8      DN_INI_FieldStr8FromSectionStr8 (DN_INISection *section, DN_INIStr8 str8);
DN_INIFieldBool      DN_INI_FieldBoolFromSectionStr8 (DN_INISection *section, DN_INIStr8 str8);
DN_INICore           DN_INI_ParseFromPtr             (char const *buf, size_t count, char *base, size_t base_count);

// NOTE: Building    
DN_INISection *      DN_INI_AppendSectionF           (DN_INICore *ini,        DN_INIArena *arena, DN_INISection *section, char const *fmt, ...);
DN_INIField *        DN_INI_AppendKeyBool            (DN_INICore *ini,        DN_INIArena *arena, DN_INISection *section, DN_INIStr8 key,  bool value);
DN_INIField *        DN_INI_AppendKeyPtrBool         (DN_INICore *ini,        DN_INIArena *arena, DN_INISection *section, char const *key, size_t key_size, bool value);
DN_INIField *        DN_INI_AppendKeyUSize           (DN_INICore *ini,        DN_INIArena *arena, DN_INISection *section, DN_INIStr8 key,  size_t value);
DN_INIField *        DN_INI_AppendKeyPtrUSize        (DN_INICore *ini,        DN_INIArena *arena, DN_INISection *section, char const *key, size_t key_size, size_t value);
DN_INIField *        DN_INI_AppendKeyCStr8           (DN_INICore *ini,        DN_INIArena *arena, DN_INISection *section, DN_INIStr8 key,  char const *value, size_t value_size);
DN_INIField *        DN_INI_AppendKeyF               (DN_INICore *ini,        DN_INIArena *arena, DN_INISection *section, DN_INIStr8 key,  char const *fmt, ...);
void                 DN_INI_AppendField              (DN_INISection *section, DN_INIField *field);

#if defined(DN_INI_WITH_UNIT_TESTS)
void                DN_INI_UnitTests              ();
#endif
#endif // !defined(DN_INI_H)
