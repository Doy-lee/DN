#if !defined(DN_CSV_H)
#define DN_CSV_H

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

#endif // !defined(DN_CSV_H)
