#if !defined(DQN_CGEN_H)
#define DQN_CGEN_H

/*
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//    $$$$$$\   $$$$$$\  $$$$$$$$\ $$\   $$\
//   $$  __$$\ $$  __$$\ $$  _____|$$$\  $$ |
//   $$ /  \__|$$ /  \__|$$ |      $$$$\ $$ |
//   $$ |      $$ |$$$$\ $$$$$\    $$ $$\$$ |
//   $$ |      $$ |\_$$ |$$  __|   $$ \$$$$ |
//   $$ |  $$\ $$ |  $$ |$$ |      $$ |\$$$ |
//   \$$$$$$  |\$$$$$$  |$$$$$$$$\ $$ | \$$ |
//    \______/  \______/ \________|\__|  \__|
//
//   dqn_cgen.h -- C/C++ code generation from table data in Metadesk files
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

// NOTE: [$CGEN] Dqn_CGen //////////////////////////////////////////////////////////////////////////
#if !defined(MD_H)
#error Metadesk 'md.h' must be included before 'dqn_cgen.h'
#endif

#if !defined(DQN_H)
#error 'dqn.h' must be included before 'dqn_cgen.h'
#endif

#if !defined(DQN_CPP_FILE_H)
#error 'dqn_cpp_file.h' must be included before 'dqn_cgen.h'
#endif

enum Dqn_CGenTableKeyType
{
    Dqn_CGenTableKeyType_Nil,
    Dqn_CGenTableKeyType_Name,
    Dqn_CGenTableKeyType_Type,
};

enum Dqn_CGenTableType
{
    Dqn_CGenTableType_Nil,
    Dqn_CGenTableType_Data,
    Dqn_CGenTableType_CodeGenBuiltinTypes,
    Dqn_CGenTableType_CodeGenStruct,
    Dqn_CGenTableType_CodeGenEnum,
    Dqn_CGenTableType_Count,
};

enum Dqn_CGenTableRowTagType
{
    Dqn_CGenTableRowTagType_Nil,
    Dqn_CGenTableRowTagType_CommentDivider,
    Dqn_CGenTableRowTagType_EmptyLine,
};

enum Dqn_CGenTableRowTagCommentDivider
{
    Dqn_CGenTableRowTagCommentDivider_Nil,
    Dqn_CGenTableRowTagCommentDivider_Label,
};

enum Dqn_CGenTableHeaderType
{
    Dqn_CGenTableHeaderType_Name,
    Dqn_CGenTableHeaderType_Table,
    Dqn_CGenTableHeaderType_CppType,
    Dqn_CGenTableHeaderType_CppName,
    Dqn_CGenTableHeaderType_CppValue,
    Dqn_CGenTableHeaderType_CppIsPtr,
    Dqn_CGenTableHeaderType_CppOpEquals,
    Dqn_CGenTableHeaderType_CppArraySize,
    Dqn_CGenTableHeaderType_CppArraySizeField,
    Dqn_CGenTableHeaderType_CppLabel,
    Dqn_CGenTableHeaderType_GenTypeInfo,
    Dqn_CGenTableHeaderType_GenEnumCount,
    Dqn_CGenTableHeaderType_Count,
};

struct Dqn_CGenTableHeader
{
    MD_String8 name;
    int        longest_string;
};

struct Dqn_CGenTableRowTag
{
    Dqn_CGenTableRowTagType type;
    MD_String8              comment;
    Dqn_CGenTableRowTag    *next;
};


struct Dqn_CGenTableColumn
{
    MD_Node  *node;
    Dqn_Str8  string;
};

struct Dqn_CGenTableRow
{
    Dqn_CGenTableRowTag *first_tag;
    Dqn_CGenTableRowTag *last_tag;
    Dqn_CGenTableColumn *columns;
};

struct Dqn_CGenTable
{
    Dqn_CGenTableType    type;
    Dqn_Str8             name;
    MD_Map               headers_map;
    Dqn_CGenTableHeader *headers;
    Dqn_CGenTableRow    *rows;
    size_t               column_count;
    size_t               row_count;

    MD_Node             *node;
    MD_Node             *headers_node;
    Dqn_usize            column_indexes[Dqn_CGenTableHeaderType_Count];
    Dqn_CGenTable       *next;
};

struct Dqn_CGen
{
    MD_Arena      *arena;
    MD_Node       *file_list;
    MD_Map         table_map;
    Dqn_CGenTable *first_table;
    Dqn_CGenTable *last_table;
    Dqn_usize      table_counts[Dqn_CGenTableType_Count];
};

struct Dqn_CGenMapNodeToEnum
{
    uint32_t enum_val;
    Dqn_Str8 node_string;
};

struct Dqn_CGenLookupTableIterator
{
    Dqn_CGenTable      *cgen_table;
    Dqn_CGenTableRow   *cgen_table_row;
    Dqn_CGenTableColumn cgen_table_column[Dqn_CGenTableHeaderType_Count];
    Dqn_CGenTable      *table;
    Dqn_usize           row_index;
};

struct Dqn_CGenLookupColumnAtHeader
{
    Dqn_usize           index;
    Dqn_CGenTableHeader header;
    Dqn_CGenTableColumn column;
};

enum Dqn_CGenEmit
{
    Dqn_CGenEmit_Prototypes     = 1 << 0,
    Dqn_CGenEmit_Implementation = 1 << 1,
};

// NOTE: [$CGEN] Dqn_CGen //////////////////////////////////////////////////////////////////////////
#define                              Dqn_CGen_MDToDqnStr8(str8) Dqn_Str8_Init((str8).str, (str8).size)
#define                              Dqn_CGen_DqnToMDStr8(str8) {DQN_CAST(MD_u8 *)(str8).data, (str8).size}
DQN_API Dqn_CGen                     Dqn_CGen_InitFilesArgV                (int argc, char const **argv, Dqn_ErrorSink *error);
DQN_API Dqn_Str8                     Dqn_CGen_TableHeaderTypeToDeclStr8    (Dqn_CGenTableHeaderType type);
DQN_API Dqn_CGenMapNodeToEnum        Dqn_CGen_MapNodeToEnumOrExit          (MD_Node const *node, Dqn_CGenMapNodeToEnum const *valid_keys, Dqn_usize valid_keys_size, char const *fmt, ...);
DQN_API Dqn_usize                    Dqn_CGen_NodeChildrenCount            (MD_Node const *node);
DQN_API void                         Dqn_CGen_LogF                         (MD_MessageKind kind, MD_Node *node, Dqn_ErrorSink *error, char const *fmt, ...);
DQN_API bool                         Dqn_CGen_TableHasHeaders              (Dqn_CGenTable const *table, Dqn_Str8 const *headers, Dqn_usize header_count, Dqn_ErrorSink *error);
DQN_API Dqn_CGenLookupColumnAtHeader Dqn_CGen_LookupColumnAtHeader         (Dqn_CGenTable *table, Dqn_Str8 header, Dqn_CGenTableRow const *row);
DQN_API bool                         Dqn_CGen_LookupNextTableInCodeGenTable(Dqn_CGen *cgen, Dqn_CGenTable *cgen_table, Dqn_CGenLookupTableIterator *it);
DQN_API void                         Dqn_CGen_EmitCodeForTables            (Dqn_CGen *cgen, Dqn_CGenEmit emit, Dqn_CppFile *cpp, Dqn_Str8 emit_prefix);
#endif // DQN_CGEN_H
