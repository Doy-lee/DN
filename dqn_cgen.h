#if !defined(DN_CGEN_H)
#define DN_CGEN_H

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

// NOTE: [$CGEN] DN_CGen //////////////////////////////////////////////////////////////////////////
#if !defined(MD_H)
#error Metadesk 'md.h' must be included before 'dn_cgen.h'
#endif

#if !defined(DN_H)
#error 'dqn.h' must be included before 'dn_cgen.h'
#endif

#if !defined(DN_CPP_FILE_H)
#error 'dn_cpp_file.h' must be included before 'dn_cgen.h'
#endif

enum DN_CGenTableKeyType
{
    DN_CGenTableKeyType_Nil,
    DN_CGenTableKeyType_Name,
    DN_CGenTableKeyType_Type,
};

enum DN_CGenTableType
{
    DN_CGenTableType_Nil,
    DN_CGenTableType_Data,
    DN_CGenTableType_CodeGenBuiltinTypes,
    DN_CGenTableType_CodeGenStruct,
    DN_CGenTableType_CodeGenEnum,
    DN_CGenTableType_Count,
};

enum DN_CGenTableRowTagType
{
    DN_CGenTableRowTagType_Nil,
    DN_CGenTableRowTagType_CommentDivider,
    DN_CGenTableRowTagType_EmptyLine,
};

enum DN_CGenTableRowTagCommentDivider
{
    DN_CGenTableRowTagCommentDivider_Nil,
    DN_CGenTableRowTagCommentDivider_Label,
};

enum DN_CGenTableHeaderType
{
    DN_CGenTableHeaderType_Name,
    DN_CGenTableHeaderType_Table,
    DN_CGenTableHeaderType_CppType,
    DN_CGenTableHeaderType_CppName,
    DN_CGenTableHeaderType_CppValue,
    DN_CGenTableHeaderType_CppIsPtr,
    DN_CGenTableHeaderType_CppOpEquals,
    DN_CGenTableHeaderType_CppArraySize,
    DN_CGenTableHeaderType_CppArraySizeField,
    DN_CGenTableHeaderType_CppLabel,
    DN_CGenTableHeaderType_GenTypeInfo,
    DN_CGenTableHeaderType_GenEnumCount,
    DN_CGenTableHeaderType_Count,
};

struct DN_CGenTableHeader
{
    MD_String8 name;
    int        longest_string;
};

struct DN_CGenTableRowTag
{
    DN_CGenTableRowTagType type;
    MD_String8              comment;
    DN_CGenTableRowTag    *next;
};


struct DN_CGenTableColumn
{
    MD_Node  *node;
    DN_Str8  string;
};

struct DN_CGenTableRow
{
    DN_CGenTableRowTag *first_tag;
    DN_CGenTableRowTag *last_tag;
    DN_CGenTableColumn *columns;
};

struct DN_CGenTable
{
    DN_CGenTableType    type;
    DN_Str8             name;
    MD_Map              headers_map;
    DN_CGenTableHeader *headers;
    DN_CGenTableRow    *rows;
    size_t              column_count;
    size_t              row_count;

    MD_Node            *node;
    MD_Node            *headers_node;
    DN_USize            column_indexes[DN_CGenTableHeaderType_Count];
    DN_CGenTable       *next;
};

struct DN_CGen
{
    MD_Arena     *arena;
    MD_Node      *file_list;
    MD_Map        table_map;
    DN_CGenTable *first_table;
    DN_CGenTable *last_table;
    DN_USize      table_counts[DN_CGenTableType_Count];
};

struct DN_CGenMapNodeToEnum
{
    uint32_t enum_val;
    DN_Str8 node_string;
};

struct DN_CGenLookupTableIterator
{
    DN_CGenTable      *cgen_table;
    DN_CGenTableRow   *cgen_table_row;
    DN_CGenTableColumn cgen_table_column[DN_CGenTableHeaderType_Count];
    DN_CGenTable      *table;
    DN_USize           row_index;
};

struct DN_CGenLookupColumnAtHeader
{
    DN_USize           index;
    DN_CGenTableHeader header;
    DN_CGenTableColumn column;
};

enum DN_CGenEmit
{
    DN_CGenEmit_Prototypes     = 1 << 0,
    DN_CGenEmit_Implementation = 1 << 1,
};

// NOTE: [$CGEN] DN_CGen //////////////////////////////////////////////////////////////////////////
#define                            DN_CGen_MDToDNStr8(str8) DN_Str8_Init((str8).str, (str8).size)
#define                            DN_CGen_DNToMDStr8(str8) {DN_CAST(MD_u8 *)(str8).data, (str8).size}
DN_API DN_CGen                     DN_CGen_InitFilesArgV                (int argc, char const **argv, DN_ErrSink *err);
DN_API DN_Str8                     DN_CGen_TableHeaderTypeToDeclStr8    (DN_CGenTableHeaderType type);
DN_API DN_CGenMapNodeToEnum        DN_CGen_MapNodeToEnumOrExit          (MD_Node const *node, DN_CGenMapNodeToEnum const *valid_keys, DN_USize valid_keys_size, char const *fmt, ...);
DN_API DN_USize                    DN_CGen_NodeChildrenCount            (MD_Node const *node);
DN_API void                        DN_CGen_LogF                         (MD_MessageKind kind, MD_Node *node, DN_ErrSink *err, char const *fmt, ...);
DN_API bool                        DN_CGen_TableHasHeaders              (DN_CGenTable const *table, DN_Str8 const *headers, DN_USize header_count, DN_ErrSink *err);
DN_API DN_CGenLookupColumnAtHeader DN_CGen_LookupColumnAtHeader         (DN_CGenTable *table, DN_Str8 header, DN_CGenTableRow const *row);
DN_API bool                        DN_CGen_LookupNextTableInCodeGenTable(DN_CGen *cgen, DN_CGenTable *cgen_table, DN_CGenLookupTableIterator *it);
DN_API void                        DN_CGen_EmitCodeForTables            (DN_CGen *cgen, DN_CGenEmit emit, DN_CppFile *cpp, DN_Str8 emit_prefix);
#endif // DN_CGEN_H
