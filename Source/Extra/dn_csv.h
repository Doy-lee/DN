#if !defined(DN_CSV_H)
#define DN_CSV_H

// NOTE: Data structures to create and parse CSV files, supports Python style escaped quotes (e.g.
// Using "" to escape quotes inside a quoted string).
//
// API
//  DN_CSV_TokeniserNextN: Reads the next N consecutive fields from the parser. If `column_iterator`
//  is `false` then the read of the N consecutive fields does not proceed past the end of the
//  current CSV row. If `true` then it reads the next N fields even if reading would progress onto
//  the next row.

  #if defined(_CLANGD)
    #include "../dn.h"
  #endif

enum DN_CSVSerialise
{
  DN_CSVSerialise_Read,
  DN_CSVSerialise_Write,
};

struct DN_CSVTokeniser
{
  bool        bad;
  DN_Str8     string;
  char        delimiter;
  char const *it;
  bool        end_of_line;
};

struct DN_CSVPack
{
  DN_Str8Builder  write_builder;
  DN_USize        write_column;
  DN_CSVTokeniser read_tokeniser;
};

DN_CSVTokeniser DN_CSV_TokeniserInit       (DN_Str8 string, char delimiter);
bool            DN_CSV_TokeniserValid      (DN_CSVTokeniser *tokeniser);
bool            DN_CSV_TokeniserNextRow    (DN_CSVTokeniser *tokeniser);
DN_Str8         DN_CSV_TokeniserNextField  (DN_CSVTokeniser *tokeniser);
DN_Str8         DN_CSV_TokeniserNextColumn (DN_CSVTokeniser *tokeniser);
void            DN_CSV_TokeniserSkipLine   (DN_CSVTokeniser *tokeniser);
int             DN_CSV_TokeniserNextN      (DN_CSVTokeniser *tokeniser, DN_Str8 *fields, int fields_size, bool column_iterator);
int             DN_CSV_TokeniserNextColumnN(DN_CSVTokeniser *tokeniser, DN_Str8 *fields, int fields_size);
int             DN_CSV_TokeniserNextFieldN (DN_CSVTokeniser *tokeniser, DN_Str8 *fields, int fields_size);
void            DN_CSV_TokeniserSkipLineN  (DN_CSVTokeniser *tokeniser, int count);
void            DN_CSV_PackU64             (DN_CSVPack *pack, DN_CSVSerialise serialise, DN_U64 *value);
void            DN_CSV_PackI64             (DN_CSVPack *pack, DN_CSVSerialise serialise, DN_I64 *value);
void            DN_CSV_PackI32             (DN_CSVPack *pack, DN_CSVSerialise serialise, DN_I32 *value);
void            DN_CSV_PackI16             (DN_CSVPack *pack, DN_CSVSerialise serialise, DN_I16 *value);
void            DN_CSV_PackI8              (DN_CSVPack *pack, DN_CSVSerialise serialise, DN_I8 *value);
void            DN_CSV_PackU32             (DN_CSVPack *pack, DN_CSVSerialise serialise, DN_U32 *value);
void            DN_CSV_PackU16             (DN_CSVPack *pack, DN_CSVSerialise serialise, DN_U16 *value);
void            DN_CSV_PackBoolAsU64       (DN_CSVPack *pack, DN_CSVSerialise serialise, bool *value);
void            DN_CSV_PackStr8            (DN_CSVPack *pack, DN_CSVSerialise serialise, DN_Str8 *str8, DN_Arena *arena);
void            DN_CSV_PackBuffer          (DN_CSVPack *pack, DN_CSVSerialise serialise, void *dest, size_t *size);
void            DN_CSV_PackBufferWithMax   (DN_CSVPack *pack, DN_CSVSerialise serialise, void *dest, size_t *size, size_t max);
bool            DN_CSV_PackNewLine         (DN_CSVPack *pack, DN_CSVSerialise serialise);

#endif // !defined(DN_CSV_H)
