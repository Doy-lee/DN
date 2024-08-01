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
//   dqn_cgen.cpp
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

Dqn_CGenMapNodeToEnum const DQN_CGEN_TABLE_KEY_LIST[] =
{
    {Dqn_CGenTableKeyType_Name, DQN_STR8("name")},
    {Dqn_CGenTableKeyType_Type, DQN_STR8("type")},
};

Dqn_CGenMapNodeToEnum const DQN_CGEN_TABLE_TYPE_LIST[] =
{
    {Dqn_CGenTableType_Data,                DQN_STR8("data")                  },
    {Dqn_CGenTableType_CodeGenBuiltinTypes, DQN_STR8("code_gen_builtin_types")},
    {Dqn_CGenTableType_CodeGenStruct,       DQN_STR8("code_gen_struct")       },
    {Dqn_CGenTableType_CodeGenEnum,         DQN_STR8("code_gen_enum")         },
};

Dqn_CGenMapNodeToEnum const DQN_CGEN_TABLE_ROW_TAG_LIST[] =
{
    {Dqn_CGenTableRowTagType_CommentDivider, DQN_STR8("comment_divider")},
    {Dqn_CGenTableRowTagType_EmptyLine,      DQN_STR8("empty_line")},
};

Dqn_CGenMapNodeToEnum const DQN_CGEN_TABLE_ROW_TAG_COMMENT_DIVIDER_KEY_LIST[] =
{
    {Dqn_CGenTableRowTagCommentDivider_Label, DQN_STR8("label")},
};

Dqn_CGenTableHeaderType const DQN_CGEN_TABLE_CODE_GEN_STRUCT_HEADER_LIST[] =
{
    Dqn_CGenTableHeaderType_Name,
    Dqn_CGenTableHeaderType_Table,
    Dqn_CGenTableHeaderType_CppType,
    Dqn_CGenTableHeaderType_CppName,
    Dqn_CGenTableHeaderType_CppIsPtr,
    Dqn_CGenTableHeaderType_CppOpEquals,
    Dqn_CGenTableHeaderType_CppArraySize,
    Dqn_CGenTableHeaderType_CppArraySizeField,
    Dqn_CGenTableHeaderType_CppLabel,
    Dqn_CGenTableHeaderType_GenTypeInfo,
};

Dqn_CGenTableHeaderType const DQN_CGEN_TABLE_CODE_GEN_ENUM_HEADER_LIST[] =
{
    Dqn_CGenTableHeaderType_Name,
    Dqn_CGenTableHeaderType_Table,
    Dqn_CGenTableHeaderType_CppName,
    Dqn_CGenTableHeaderType_CppValue,
    Dqn_CGenTableHeaderType_CppLabel,
    Dqn_CGenTableHeaderType_GenTypeInfo,
    Dqn_CGenTableHeaderType_GenEnumCount,
};

Dqn_CGenTableHeaderType const DQN_CGEN_TABLE_CODE_GEN_BUILTIN_TYPES_HEADER_LIST[] =
{
    Dqn_CGenTableHeaderType_Name,
};

static bool Dqn_CGen_GatherTables_(Dqn_CGen *cgen, Dqn_ErrorSink *error)
{
    bool result = false;
    if (!cgen || !cgen->file_list || !cgen->arena)
        return result;

    // NOTE: Gather the tables /////////////////////////////////////////////////////////////////////
    for (MD_EachNode(ref, cgen->file_list->first_child)) {
        MD_Node *root = MD_ResolveNodeFromReference(ref);
        for (MD_EachNode(node, root->first_child)) {

            MD_Node *table_tag = MD_TagFromString(node, MD_S8Lit("table"), 0);
            if (MD_NodeIsNil(table_tag))
                continue;

            Dqn_CGenTable *table = MD_PushArray(cgen->arena, Dqn_CGenTable, 1);
            table->node          = node;
            table->name          = Dqn_CGen_MDToDqnStr8(table_tag->first_child->first_child->string);
            MD_MapInsert(cgen->arena, &cgen->table_map, MD_MapKeyStr(Dqn_CGen_DqnToMDStr8(table->name)), table);
            MD_QueuePush(cgen->first_table, cgen->last_table, table);

            for (MD_EachNode(key, table_tag->first_child)) {
                Dqn_CGenMapNodeToEnum key_mapping = Dqn_CGen_MapNodeToEnumOrExit(key,
                                                                                 DQN_CGEN_TABLE_KEY_LIST,
                                                                                 DQN_ARRAY_UCOUNT(DQN_CGEN_TABLE_KEY_LIST),
                                                                                 "Table specified invalid key");
                switch (DQN_CAST(Dqn_CGenTableKeyType)key_mapping.enum_val) {
                    case Dqn_CGenTableKeyType_Nil: DQN_INVALID_CODE_PATH;

                    case Dqn_CGenTableKeyType_Name: {
                        table->name = Dqn_CGen_MDToDqnStr8(key->first_child->string);
                    } break;

                    case Dqn_CGenTableKeyType_Type: {
                        MD_Node              *table_type_value     = key->first_child;
                        Dqn_CGenMapNodeToEnum table_type_validator = Dqn_CGen_MapNodeToEnumOrExit(table_type_value,
                                                                                                  DQN_CGEN_TABLE_TYPE_LIST,
                                                                                                  DQN_ARRAY_UCOUNT(DQN_CGEN_TABLE_TYPE_LIST),
                                                                                                  "Table 'type' specified invalid value");
                        table->type                                = DQN_CAST(Dqn_CGenTableType) table_type_validator.enum_val;

                        DQN_ASSERT(table->type <= Dqn_CGenTableType_Count);
                        cgen->table_counts[table->type]++;
                    } break;
                }
            }
        }
    }

    // NOTE: Parse the tables //////////////////////////////////////////////////////////////////////
    Dqn_usize const BEGIN_COLUMN_INDEX = 1; /*Reserve 0th slot for nil-entry*/
    for (Dqn_CGenTable *table = cgen->first_table; table; table = table->next) {
        table->column_count = BEGIN_COLUMN_INDEX;
        table->headers_node = table->node->first_child;
        for (MD_EachNode(column_node, table->headers_node->first_child))
            table->column_count++;

        if (table->column_count == BEGIN_COLUMN_INDEX)
            continue;

        MD_Node *row_it = table->headers_node->next;
        for (MD_EachNode(row_node, row_it))
            table->row_count++;

        table->rows    = MD_PushArray(cgen->arena, Dqn_CGenTableRow, table->row_count);
        table->headers = MD_PushArray(cgen->arena, Dqn_CGenTableHeader, table->column_count);
        for (Dqn_usize index = 0; index < table->row_count; index++) {
            Dqn_CGenTableRow *row = table->rows + index;
            row->columns          = MD_PushArray(cgen->arena, Dqn_CGenTableColumn, table->column_count);
        }

        // NOTE: Collect table headers /////////////////////////////////////////////////////////////
        table->headers_map     = MD_MapMake(cgen->arena);
        Dqn_usize column_index = BEGIN_COLUMN_INDEX;
        for (MD_EachNode(header_column, table->headers_node->first_child)) {
            DQN_ASSERT(column_index < table->column_count);

            // NOTE: Detect builtin headers and cache the index for that table /////////////////////
            for (Dqn_usize enum_index = 0; enum_index < Dqn_CGenTableHeaderType_Count; enum_index++) {
                Dqn_Str8 decl_str8 = Dqn_CGen_TableHeaderTypeToDeclStr8(DQN_CAST(Dqn_CGenTableHeaderType)enum_index);
                if (decl_str8 != Dqn_Str8_Init(header_column->string.str, header_column->string.size))
                    continue;
                table->column_indexes[enum_index] = column_index;
                break;
            }

            MD_MapInsert(cgen->arena, &table->headers_map, MD_MapKeyStr(header_column->string), DQN_CAST(void *)column_index);
            table->headers[column_index++].name = header_column->string;
        }

        // NOTE: Validate table headers ////////////////////////////////////////////////////////////
        switch (table->type) {
            case Dqn_CGenTableType_Nil: DQN_INVALID_CODE_PATH;
            case Dqn_CGenTableType_Count: DQN_INVALID_CODE_PATH;

            case Dqn_CGenTableType_Data: {
            } break;

            case Dqn_CGenTableType_CodeGenStruct: {
                for (Dqn_CGenTableHeaderType enum_val : DQN_CGEN_TABLE_CODE_GEN_STRUCT_HEADER_LIST) {
                    if (table->column_indexes[enum_val] == 0) {
                        Dqn_Str8 expected_value = Dqn_CGen_TableHeaderTypeToDeclStr8(enum_val);
                        Dqn_CGen_LogF(MD_MessageKind_Error, table->headers_node, error, "Struct code generation table is missing column '%.*s'", DQN_STR_FMT(expected_value));
                        return false;
                    }
                }
            } break;

            case Dqn_CGenTableType_CodeGenEnum: {
                for (Dqn_CGenTableHeaderType enum_val : DQN_CGEN_TABLE_CODE_GEN_ENUM_HEADER_LIST) {
                    if (table->column_indexes[enum_val] == 0) {
                        Dqn_Str8 expected_value = Dqn_CGen_TableHeaderTypeToDeclStr8(enum_val);
                        Dqn_CGen_LogF(MD_MessageKind_Error, table->headers_node, error, "Enum code generation table is missing column '%.*s'", DQN_STR_FMT(expected_value));
                        return false;
                    }
                }
            } break;

            case Dqn_CGenTableType_CodeGenBuiltinTypes: {
                for (Dqn_CGenTableHeaderType enum_val : DQN_CGEN_TABLE_CODE_GEN_BUILTIN_TYPES_HEADER_LIST) {
                    if (table->column_indexes[enum_val] == 0) {
                        Dqn_Str8 expected_value = Dqn_CGen_TableHeaderTypeToDeclStr8(enum_val);
                        Dqn_CGen_LogF(MD_MessageKind_Error, table->headers_node, error, "Enum code generation table is missing column '%.*s'", DQN_STR_FMT(expected_value));
                        return false;
                    }
                }
            } break;
        }

        // NOTE: Parse each row in table ///////////////////////////////////////////////////////////
        Dqn_usize row_index = 0;
        for (MD_EachNode(row_node, row_it)) {
            column_index = BEGIN_COLUMN_INDEX;
            DQN_ASSERT(row_index < table->row_count);

            // NOTE: Parse any tags set on the row /////////////////////////////////////////////////
            Dqn_CGenTableRow *row = table->rows + row_index++;
            for (MD_EachNode(row_tag, row_node->first_tag)) {
                Dqn_CGenMapNodeToEnum row_mapping = Dqn_CGen_MapNodeToEnumOrExit(row_tag,
                                                                                 DQN_CGEN_TABLE_ROW_TAG_LIST,
                                                                                 DQN_ARRAY_UCOUNT(DQN_CGEN_TABLE_ROW_TAG_LIST),
                                                                                 "Table specified invalid row tag");
                Dqn_CGenTableRowTag  *tag         = MD_PushArray(cgen->arena, Dqn_CGenTableRowTag, 1);
                tag->type                         = DQN_CAST(Dqn_CGenTableRowTagType) row_mapping.enum_val;
                MD_QueuePush(row->first_tag, row->last_tag, tag);

                switch (tag->type) {
                    case Dqn_CGenTableRowTagType_Nil: DQN_INVALID_CODE_PATH;

                    case Dqn_CGenTableRowTagType_CommentDivider: {
                        for (MD_EachNode(tag_key, row_tag->first_child)) {
                            Dqn_CGenMapNodeToEnum tag_mapping = Dqn_CGen_MapNodeToEnumOrExit(tag_key,
                                                                                             DQN_CGEN_TABLE_ROW_TAG_COMMENT_DIVIDER_KEY_LIST,
                                                                                             DQN_ARRAY_UCOUNT(DQN_CGEN_TABLE_ROW_TAG_COMMENT_DIVIDER_KEY_LIST),
                                                                                             "Table specified invalid row tag");
                            switch (DQN_CAST(Dqn_CGenTableRowTagCommentDivider)tag_mapping.enum_val) {
                                case Dqn_CGenTableRowTagCommentDivider_Nil: DQN_INVALID_CODE_PATH;
                                case Dqn_CGenTableRowTagCommentDivider_Label: {
                                    tag->comment = tag_key->first_child->string;
                                } break;
                            }
                        }
                    } break;

                    case Dqn_CGenTableRowTagType_EmptyLine: break;
                }
            }

            for (MD_EachNode(column_node, row_node->first_child)) {
                table->headers[column_index].longest_string = DQN_MAX(table->headers[column_index].longest_string, DQN_CAST(int) column_node->string.size);
                row->columns[column_index].string           = Dqn_CGen_MDToDqnStr8(column_node->string);
                row->columns[column_index].node             = column_node;
                column_index++;
            }
        }
    }

    // NOTE: Validate codegen //////////////////////////////////////////////////////////////////////
    Dqn_CGenTableHeaderType const CHECK_COLUMN_LINKS[] = {
        Dqn_CGenTableHeaderType_CppName,
        Dqn_CGenTableHeaderType_CppType,
        Dqn_CGenTableHeaderType_CppValue,
        Dqn_CGenTableHeaderType_CppIsPtr,
        Dqn_CGenTableHeaderType_CppArraySize,
        Dqn_CGenTableHeaderType_CppArraySizeField,
    };

    result = true;
    for (Dqn_CGenTable *table = cgen->first_table; table; table = table->next) {
        for (Dqn_CGenLookupTableIterator it = {}; Dqn_CGen_LookupNextTableInCodeGenTable(cgen, table, &it);) {
            for (Dqn_CGenTableHeaderType check_column : CHECK_COLUMN_LINKS) {
                Dqn_CGenTableColumn column = it.cgen_table_row->columns[table->column_indexes[check_column]];
                if (column.string.size == 0) {
                    // NOTE: The code generation table did not bind a code generation parameter to
                    // a column in the target table. We skip it.
                    continue;
                }

                // NOTE: Check if the column to bind to exists in the target table.
                if (!MD_MapLookup(&it.table->headers_map, MD_MapKeyStr(Dqn_CGen_DqnToMDStr8(column.string)))) {
                    result                    = false;
                    Dqn_Str8 header_type_str8 = Dqn_CGen_TableHeaderTypeToDeclStr8(check_column);
                    Dqn_CGen_LogF(MD_MessageKind_Error, column.node, error,
                                    "Code generation table binds '%.*s' to '%.*s', but the column '%.*s' does not exist in table '%.*s'\n"
                                    "NOTE: If you want '%.*s' to omit the column '%.*s' you can bind to the empty string `` to skip it, otherwise, please ensure the table '%.*s' has the column '%.*s'"
                                    ,
                                    DQN_STR_FMT(column.string),
                                    DQN_STR_FMT(header_type_str8),
                                    DQN_STR_FMT(column.string),
                                    DQN_STR_FMT(it.table->name),
                                    DQN_STR_FMT(it.table->name),
                                    DQN_STR_FMT(header_type_str8),
                                    DQN_STR_FMT(it.table->name),
                                    DQN_STR_FMT(header_type_str8));
                }
            }
        }
    }

    return result;
}

DQN_API Dqn_CGen Dqn_CGen_InitFilesArgV(int argc, char const **argv, Dqn_ErrorSink *error)
{
    Dqn_CGen result  = {};
    result.arena     = MD_ArenaAlloc();
    result.file_list = MD_MakeList(result.arena);
    result.table_map = MD_MapMake(result.arena);

    bool has_error = false;
    for (Dqn_isize arg_index = 0; arg_index < argc; arg_index++) {
        MD_String8     file_name    = MD_S8CString(DQN_CAST(char *)argv[arg_index]);
        MD_ParseResult parse_result = MD_ParseWholeFile(result.arena, file_name);
        for (MD_Message *message = parse_result.errors.first; message != 0; message = message->next) {
            has_error = true;
            Dqn_CGen_LogF(message->kind, message->node, error, "%.*s", MD_S8VArg(message->string));
        }
        MD_PushNewReference(result.arena, result.file_list, parse_result.node);
    }

    if (!has_error)
        Dqn_CGen_GatherTables_(&result, error);
    return result;
}

DQN_API Dqn_Str8 Dqn_CGen_TableHeaderTypeToDeclStr8(Dqn_CGenTableHeaderType type)
{
    Dqn_Str8 result = {};
    switch (type) {
        case Dqn_CGenTableHeaderType_Name:              result = DQN_STR8("name"); break;
        case Dqn_CGenTableHeaderType_Table:             result = DQN_STR8("table"); break;
        case Dqn_CGenTableHeaderType_CppType:           result = DQN_STR8("cpp_type"); break;
        case Dqn_CGenTableHeaderType_CppName:           result = DQN_STR8("cpp_name"); break;
        case Dqn_CGenTableHeaderType_CppValue:          result = DQN_STR8("cpp_value"); break;
        case Dqn_CGenTableHeaderType_CppIsPtr:          result = DQN_STR8("cpp_is_ptr"); break;
        case Dqn_CGenTableHeaderType_CppOpEquals:       result = DQN_STR8("cpp_op_equals"); break;
        case Dqn_CGenTableHeaderType_CppArraySize:      result = DQN_STR8("cpp_array_size"); break;
        case Dqn_CGenTableHeaderType_CppArraySizeField: result = DQN_STR8("cpp_array_size_field"); break;
        case Dqn_CGenTableHeaderType_CppLabel:          result = DQN_STR8("cpp_label"); break;
        case Dqn_CGenTableHeaderType_GenTypeInfo:       result = DQN_STR8("gen_type_info"); break;
        case Dqn_CGenTableHeaderType_GenEnumCount:      result = DQN_STR8("gen_enum_count"); break;
        case Dqn_CGenTableHeaderType_Count:             result = DQN_STR8("XX BAD ENUM VALUE XX"); break;
        default:                                        result = DQN_STR8("XX INVALID ENUM VALUE XX"); break;
    }
    return result;
}

DQN_API Dqn_CGenMapNodeToEnum Dqn_CGen_MapNodeToEnumOrExit(MD_Node const *node, Dqn_CGenMapNodeToEnum const *valid_keys, Dqn_usize valid_keys_size, char const *fmt, ...)
{
    Dqn_CGenMapNodeToEnum result = {};
    for (Dqn_usize index = 0; index < valid_keys_size; index++) {
        Dqn_CGenMapNodeToEnum const *validator = valid_keys + index;
        if (Dqn_Str8_Init(node->string.str, node->string.size) == validator->node_string) {
            result = *validator;
            break;
        }
    }

    if (result.enum_val == 0) {
        MD_CodeLoc  loc     = MD_CodeLocFromNode(DQN_CAST(MD_Node *)node);
        Dqn_TLSTMem tmem = Dqn_TLS_TMem(nullptr);
        va_list     args;
        va_start(args, fmt);
        Dqn_Str8 user_msg = Dqn_Str8_InitFV(tmem.arena, fmt, args);
        va_end(args);

        Dqn_Str8Builder builder = {};
        builder.arena           = tmem.arena;

        Dqn_Str8Builder_AddF(&builder, "%.*s: '%.*s' is not recognised, the supported values are ", DQN_STR_FMT(user_msg), MD_S8VArg(node->string));
        for (Dqn_usize index = 0; index < valid_keys_size; index++) {
            Dqn_CGenMapNodeToEnum const *validator = valid_keys + index;
            Dqn_Str8Builder_AddF(&builder, DQN_CAST(char *)"%s'%.*s'", index ? ", " : "", DQN_STR_FMT(validator->node_string));
        }

        Dqn_Str8 error_msg = Dqn_Str8Builder_Build(&builder, tmem.arena);
        MD_PrintMessageFmt(stderr, loc, MD_MessageKind_Error, DQN_CAST(char *) "%.*s", DQN_STR_FMT(error_msg));
        Dqn_OS_Exit(DQN_CAST(uint32_t)-1);
    }
    return result;
}

DQN_API Dqn_usize Dqn_CGen_NodeChildrenCount(MD_Node const *node)
{
    Dqn_usize result = 0;
    for (MD_EachNode(item, node->first_child))
        result++;
    return result;
}

DQN_API void Dqn_CGen_LogF(MD_MessageKind kind, MD_Node *node, Dqn_ErrorSink *error, char const *fmt, ...)
{
    if (!error)
        return;

    Dqn_TLSTMem     tmem    = Dqn_TLS_PushTMem(nullptr);
    Dqn_Str8Builder builder = Dqn_Str8Builder_Init_TLS();

    MD_String8 kind_string = MD_StringFromMessageKind(kind);
    MD_CodeLoc loc         = MD_CodeLocFromNode(node);
    Dqn_Str8Builder_AddF(&builder, "" MD_FmtCodeLoc " %.*s: ", MD_CodeLocVArg(loc), MD_S8VArg(kind_string));

    va_list args;
    va_start(args, fmt);
    Dqn_Str8Builder_AddFV(&builder, fmt, args);
    va_end(args);

    Dqn_Str8 msg = Dqn_Str8Builder_Build(&builder, tmem.arena);
    Dqn_ErrorSink_MakeF(error, DQN_CAST(uint32_t)-1, "%.*s", DQN_STR_FMT(msg));
}

DQN_API bool Dqn_CGen_TableHasHeaders(Dqn_CGenTable const *table, Dqn_Str8 const *headers, Dqn_usize header_count, Dqn_ErrorSink *error)
{
    bool            result  = true;
    Dqn_TLSTMem     tmem    = Dqn_TLS_TMem(nullptr);
    Dqn_Str8Builder builder = {};
    builder.arena           = tmem.arena;

    for (Dqn_usize index = 0; index < header_count; index++) {
        Dqn_Str8    header    = headers[index];
        MD_String8  header_md = {DQN_CAST(MD_u8 *) header.data, header.size};
        MD_MapSlot *slot      = MD_MapLookup(DQN_CAST(MD_Map *)&table->headers_map, MD_MapKeyStr(header_md));
        if (!slot) {
            result = false;
            Dqn_Str8Builder_AddF(&builder, "%s%.*s", builder.count ? ", " : "", DQN_STR_FMT(header));
        }
    }

    if (!result) {
        Dqn_Str8 missing_headers = Dqn_Str8Builder_Build(&builder, tmem.arena);
        Dqn_CGen_LogF(MD_MessageKind_Error,
                      table->headers_node,
                      error,
                      "Table '%.*s' is missing the header(s): %.*s",
                      DQN_STR_FMT(table->name),
                      DQN_STR_FMT(missing_headers));
    }

    return result;
}

DQN_API Dqn_CGenLookupColumnAtHeader Dqn_CGen_LookupColumnAtHeader(Dqn_CGenTable *table, Dqn_Str8 header, Dqn_CGenTableRow const *row)
{
    Dqn_CGenLookupColumnAtHeader result = {};
    if (!table || !row)
        return result;

    MD_String8  header_md = {DQN_CAST(MD_u8 *) header.data, header.size};
    MD_MapSlot *slot      = MD_MapLookup(&table->headers_map, MD_MapKeyStr(header_md));
    if (!slot)
        return result;

    Dqn_usize column_index = DQN_CAST(Dqn_usize) slot->val;
    DQN_ASSERT(column_index < table->column_count);
    {
        Dqn_usize begin = DQN_CAST(uintptr_t)(table->rows);
        Dqn_usize end   = DQN_CAST(uintptr_t)(table->rows + table->row_count);
        Dqn_usize ptr   = DQN_CAST(uintptr_t)row;
        DQN_ASSERTF(ptr >= begin && ptr <= end, "The row to lookup does not belong to the table passed in");
    }

    result.index  = column_index;
    result.column = row->columns[column_index];
    result.header = table->headers[column_index];
    return result;
}

DQN_API bool Dqn_CGen_LookupNextTableInCodeGenTable(Dqn_CGen *cgen, Dqn_CGenTable *cgen_table, Dqn_CGenLookupTableIterator *it)
{
    if (!cgen_table)
        return false;

    if (it->row_index >= cgen_table->row_count)
        return false;

    if (cgen_table->type != Dqn_CGenTableType_CodeGenEnum && cgen_table->type != Dqn_CGenTableType_CodeGenStruct && cgen_table->type != Dqn_CGenTableType_CodeGenBuiltinTypes)
        return false;

    // NOTE: Lookup the table in this row that we will code generate from. Not
    // applicable when we are doing builtin types as the types are just put
    // in-line into the code generation table itself.
    it->cgen_table     = cgen_table;
    it->cgen_table_row = cgen_table->rows + it->row_index++;
    if (cgen_table->type != Dqn_CGenTableType_CodeGenBuiltinTypes) {
        Dqn_CGenTableColumn cgen_table_column = it->cgen_table_row->columns[cgen_table->column_indexes[Dqn_CGenTableHeaderType_Table]];
        MD_String8          key               = {DQN_CAST(MD_u8 *)cgen_table_column.string.data, cgen_table_column.string.size};
        MD_MapSlot         *table_slot        = MD_MapLookup(&cgen->table_map, MD_MapKeyStr(key));
        if (!table_slot) {
            MD_CodeLoc loc = MD_CodeLocFromNode(cgen_table_column.node);
            MD_PrintMessageFmt(stderr,
                               loc,
                               MD_MessageKind_Warning,
                               DQN_CAST(char *) "Code generation table references non-existent table '%.*s'",
                               DQN_STR_FMT(cgen_table_column.string));
            return false;
        }
        it->table = DQN_CAST(Dqn_CGenTable *) table_slot->val;
    }

    for (Dqn_usize type = 0; type < Dqn_CGenTableHeaderType_Count; type++)
        it->cgen_table_column[type] = it->cgen_table_row->columns[cgen_table->column_indexes[type]];
    return true;
}

DQN_API bool Dqn_CGen_WillCodeGenTypeName(Dqn_CGen const *cgen, Dqn_Str8 name)
{
    if (!Dqn_Str8_HasData(name))
        return false;

    for (Dqn_CGenTable *table = cgen->first_table; table != 0; table = table->next) {
        if (table->type != Dqn_CGenTableType_CodeGenStruct && table->type != Dqn_CGenTableType_CodeGenEnum)
            continue;

        for (Dqn_usize row_index = 0; row_index < table->row_count; row_index++) {
            Dqn_CGenTableRow const    *row    = table->rows + row_index;
            Dqn_CGenTableColumn const *column = row->columns + table->column_indexes[Dqn_CGenTableHeaderType_Name];
            if (column->string == name)
                return true;
        }
    }

    return false;
}

static void Dqn_CGen_EmitRowWhitespace_(Dqn_CGenTableRow const *row, Dqn_CppFile *cpp)
{
    for (Dqn_CGenTableRowTag *tag = row->first_tag; tag; tag = tag->next) {
        switch (tag->type) {
            case Dqn_CGenTableRowTagType_Nil: DQN_INVALID_CODE_PATH;

            case Dqn_CGenTableRowTagType_CommentDivider: {
                if (tag->comment.size <= 0)
                    break;

                Dqn_TLSTMem tmem      = Dqn_TLS_TMem(nullptr);
                Dqn_Str8    prefix       = Dqn_Str8_InitF(tmem.arena, "// NOTE: %.*s ", MD_S8VArg(tag->comment));
                int         line_padding = DQN_MAX(100 - (DQN_CAST(int) prefix.size + (Dqn_CppSpacePerIndent(cpp) * cpp->indent)), 0);
                Dqn_CppPrint(cpp, "%.*s", DQN_STR_FMT(prefix));
                for (int index = 0; index < line_padding; index++)
                    Dqn_CppAppend(cpp, "/");
                Dqn_CppAppend(cpp, "\n");
            } break;

            case Dqn_CGenTableRowTagType_EmptyLine: {
                Dqn_CppAppend(cpp, "\n");
            } break;
        }
    }
}

Dqn_Str8 Dqn_CGen_StripQualifiersOnCppType_(Dqn_Arena *arena, Dqn_Str8 type)
{
    Dqn_TLSTMem tmem = Dqn_TLS_TMem(arena);
    Dqn_Str8 result     = Dqn_Str8_TrimWhitespaceAround(type);
    result              = Dqn_Str8_Replace(result, /*find*/ DQN_STR8("*"),         /*replace*/ DQN_STR8(""), /*start_index*/ 0, tmem.arena, Dqn_Str8EqCase_Sensitive);
    result              = Dqn_Str8_Replace(result, /*find*/ DQN_STR8("constexpr"), /*replace*/ DQN_STR8(""), /*start_index*/ 0, tmem.arena, Dqn_Str8EqCase_Sensitive);
    result              = Dqn_Str8_Replace(result, /*find*/ DQN_STR8("const"),     /*replace*/ DQN_STR8(""), /*start_index*/ 0, tmem.arena, Dqn_Str8EqCase_Sensitive);
    result              = Dqn_Str8_Replace(result, /*find*/ DQN_STR8("static"),    /*replace*/ DQN_STR8(""), /*start_index*/ 0, tmem.arena, Dqn_Str8EqCase_Sensitive);
    result              = Dqn_Str8_Replace(result, /*find*/ DQN_STR8(" "),         /*replace*/ DQN_STR8(""), /*start_index*/ 0, arena,         Dqn_Str8EqCase_Sensitive);
    result              = Dqn_Str8_TrimWhitespaceAround(result);
    return result;
}

DQN_API void Dqn_CGen_EmitCodeForTables(Dqn_CGen *cgen, Dqn_CGenEmit emit, Dqn_CppFile *cpp, Dqn_Str8 emit_prefix)
{
    if (emit & Dqn_CGenEmit_Prototypes) {
        // NOTE: Generate type info enums //////////////////////////////////////////////////////////
        Dqn_CppEnumBlock(cpp, "%.*sType", DQN_STR_FMT(emit_prefix)) {
            Dqn_CppLine(cpp, "%.*sType_Nil,", DQN_STR_FMT(emit_prefix));
            for (Dqn_CGenTable *table = cgen->first_table; table != 0; table = table->next) {
                for (Dqn_CGenLookupTableIterator it = {}; Dqn_CGen_LookupNextTableInCodeGenTable(cgen, table, &it);)
                    Dqn_CppLine(cpp, "%.*sType_%.*s,", DQN_STR_FMT(emit_prefix), DQN_STR_FMT(it.cgen_table_column[Dqn_CGenTableHeaderType_Name].string));
            }
            Dqn_CppLine(cpp, "%.*sType_Count,", DQN_STR_FMT(emit_prefix));
        }
        Dqn_CppNewLine(cpp);

        // NOTE: Generate structs + enums //////////////////////////////////////////////////////////////
        for (Dqn_CGenTable *table = cgen->first_table; table != 0; table = table->next) {
            switch (table->type) {
                case Dqn_CGenTableType_Nil:                 DQN_INVALID_CODE_PATH;
                case Dqn_CGenTableType_Count:               DQN_INVALID_CODE_PATH;
                case Dqn_CGenTableType_CodeGenBuiltinTypes: continue;
                case Dqn_CGenTableType_Data:                continue;

                case Dqn_CGenTableType_CodeGenStruct: {

                    for (Dqn_CGenLookupTableIterator it = {}; Dqn_CGen_LookupNextTableInCodeGenTable(cgen, table, &it); ) {
                        // TODO(doyle): Verify the codegen table has the headers from the table it references
                        int longest_type_name = 0;
                        for (Dqn_usize row_index = 0; row_index < it.table->row_count; row_index++) {
                            Dqn_TLSTMem tmem                      = Dqn_TLS_TMem(nullptr);
                            Dqn_CGenTableRow const      *row      = it.table->rows + row_index;
                            Dqn_CGenLookupColumnAtHeader cpp_type = Dqn_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[Dqn_CGenTableHeaderType_CppType].string, row);

                            Dqn_usize length   = cpp_type.column.string.size;
                            Dqn_Str8 find_name = Dqn_CGen_StripQualifiersOnCppType_(tmem.arena, cpp_type.column.string);
                            if (Dqn_CGen_WillCodeGenTypeName(cgen, find_name))
                                length += emit_prefix.size;

                            longest_type_name = DQN_MAX(longest_type_name, DQN_CAST(int)length);
                        }

                        Dqn_CppStructBlock(cpp, "%.*s%.*s", DQN_STR_FMT(emit_prefix), DQN_STR_FMT(it.cgen_table_column[Dqn_CGenTableHeaderType_Name].string)) {
                            for (Dqn_usize row_index = 0; row_index < it.table->row_count; row_index++) {
                                Dqn_CGenTableRow const      *row                  = it.table->rows + row_index;
                                Dqn_CGenLookupColumnAtHeader cpp_name             = Dqn_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[Dqn_CGenTableHeaderType_CppName].string, row);
                                Dqn_CGenLookupColumnAtHeader cpp_type             = Dqn_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[Dqn_CGenTableHeaderType_CppType].string, row);
                                Dqn_CGenLookupColumnAtHeader cpp_array_size       = Dqn_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[Dqn_CGenTableHeaderType_CppArraySize].string, row);
                                if (cpp_name.column.string.size <= 0 || cpp_type.column.string.size <= 0)
                                    continue;

                                // NOTE: Generate cpp array size ///////////////////////////////////
                                Dqn_TLSTMem tmem    = Dqn_TLS_TMem(nullptr);
                                Dqn_Str8    array_size = {};
                                if (cpp_array_size.column.string.size)
                                    array_size = Dqn_Str8_InitF(tmem.arena, "[%.*s]", DQN_STR_FMT(cpp_array_size.column.string));

                                // NOTE: Check if we're referencing a code generated type. If we
                                // are, append the `emit_prefix`
                                Dqn_Str8 emit_cpp_type = cpp_type.column.string;
                                {
                                    Dqn_Str8 find_name = Dqn_CGen_StripQualifiersOnCppType_(tmem.arena, emit_cpp_type);
                                    if (Dqn_CGen_WillCodeGenTypeName(cgen, find_name))
                                        emit_cpp_type = Dqn_Str8_InitF(tmem.arena, "%.*s%.*s", DQN_STR_FMT(emit_prefix), DQN_STR_FMT(cpp_type.column.string));
                                }

                                int name_to_type_padding = 1 + longest_type_name - DQN_CAST(int) emit_cpp_type.size;

                                // NOTE: Emit decl /////////////////////////////////////////////////
                                Dqn_CGen_EmitRowWhitespace_(row, cpp);
                                Dqn_CppLine(cpp,
                                            "%.*s%*s%.*s%.*s;",
                                            DQN_STR_FMT(emit_cpp_type),
                                            name_to_type_padding,
                                            "",
                                            DQN_STR_FMT(cpp_name.column.string),
                                            DQN_STR_FMT(array_size));
                            }
                        }
                        Dqn_CppNewLine(cpp);
                        Dqn_CppNewLine(cpp);
                    }

                } break;

                case Dqn_CGenTableType_CodeGenEnum: {
                    for (Dqn_CGenLookupTableIterator it = {}; Dqn_CGen_LookupNextTableInCodeGenTable(cgen, table, &it); ) {
                        Dqn_CppEnumBlock(cpp, "%.*s%.*s", DQN_STR_FMT(emit_prefix), DQN_STR_FMT(it.cgen_table_column[Dqn_CGenTableHeaderType_Name].string)) {
                            Dqn_usize enum_count   = 0;
                            for (Dqn_usize row_index = 0; row_index < it.table->row_count; row_index++) {
                                Dqn_CGenTableRow const      *row       = it.table->rows + row_index;
                                Dqn_CGenLookupColumnAtHeader cpp_name  = Dqn_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[Dqn_CGenTableHeaderType_CppName].string, row);
                                Dqn_CGenLookupColumnAtHeader cpp_value = Dqn_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[Dqn_CGenTableHeaderType_CppValue].string, row);

                                if (cpp_name.column.string.size <= 0)
                                    continue;

                                Dqn_CGen_EmitRowWhitespace_(row, cpp);
                                if (cpp_value.column.string.size) {
                                    Dqn_CppLine(cpp,
                                                "%.*s%.*s_%.*s = %.*s,",
                                                DQN_STR_FMT(emit_prefix),
                                                DQN_STR_FMT(it.cgen_table_column[Dqn_CGenTableHeaderType_Name].string),
                                                DQN_STR_FMT(cpp_name.column.string),
                                                DQN_STR_FMT(cpp_value.column.string));
                                } else {
                                    Dqn_CppLine(cpp,
                                                "%.*s%.*s_%.*s = %zu,",
                                                DQN_STR_FMT(emit_prefix),
                                                DQN_STR_FMT(it.cgen_table_column[Dqn_CGenTableHeaderType_Name].string),
                                                DQN_STR_FMT(cpp_name.column.string),
                                                row_index);
                                }
                                enum_count++;
                            }

                            Dqn_CGenTableColumn gen_enum_count_column = it.cgen_table_column[Dqn_CGenTableHeaderType_GenEnumCount];
                            if (gen_enum_count_column.string.size)
                                Dqn_CppLine(cpp,
                                            "%.*s%.*s_%.*s = %zu,",
                                            DQN_STR_FMT(emit_prefix),
                                            DQN_STR_FMT(it.cgen_table_column[Dqn_CGenTableHeaderType_Name].string),
                                            DQN_STR_FMT(gen_enum_count_column.string),
                                            enum_count);
                        }
                        Dqn_CppNewLine(cpp);
                    }
                } break;
            }
        }

        // NOTE: Generate enums for struct fields //////////////////////////////////////////////////
        for (Dqn_CGenTable *table = cgen->first_table; table != 0; table = table->next) {
            switch (table->type) {
                case Dqn_CGenTableType_Nil:                 DQN_INVALID_CODE_PATH;
                case Dqn_CGenTableType_Count:               DQN_INVALID_CODE_PATH;
                case Dqn_CGenTableType_Data:                continue;
                case Dqn_CGenTableType_CodeGenBuiltinTypes: continue;
                case Dqn_CGenTableType_CodeGenEnum:         continue;

                case Dqn_CGenTableType_CodeGenStruct: {
                    for (Dqn_CGenLookupTableIterator it = {}; Dqn_CGen_LookupNextTableInCodeGenTable(cgen, table, &it); ) {
                        Dqn_Str8 struct_name = it.cgen_table_column[Dqn_CGenTableHeaderType_Name].string;
                        Dqn_CppEnumBlock(cpp, "%.*s%.*sTypeField", DQN_STR_FMT(emit_prefix), DQN_STR_FMT(struct_name)) {
                            for (Dqn_usize row_index = 0; row_index < it.table->row_count; row_index++) {
                                Dqn_CGenTableRow const      *row      = it.table->rows + row_index;
                                Dqn_CGenLookupColumnAtHeader cpp_name = Dqn_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[Dqn_CGenTableHeaderType_CppName].string, row);
                                if (cpp_name.column.string.size <= 0)
                                    continue;
                                Dqn_CGen_EmitRowWhitespace_(row, cpp);
                                Dqn_CppLine(cpp, "%.*s%.*sTypeField_%.*s,", DQN_STR_FMT(emit_prefix), DQN_STR_FMT(struct_name), DQN_STR_FMT(cpp_name.column.string));
                            }
                            Dqn_CppLine(cpp, "%.*s%.*sTypeField_Count,", DQN_STR_FMT(emit_prefix), DQN_STR_FMT(struct_name));
                        }
                        Dqn_CppNewLine(cpp);
                    }
                } break;
            }
        }

        // NOTE: Str8 to enum conversion ////////////////////////////////////////////////////////////
        for (Dqn_CGenTable *table = cgen->first_table; table != 0; table = table->next) {
            if (table->type != Dqn_CGenTableType_CodeGenEnum)
                continue;

            for (Dqn_CGenLookupTableIterator it = {}; Dqn_CGen_LookupNextTableInCodeGenTable(cgen, table, &it);) {
                Dqn_Str8 type_name = it.cgen_table_column[Dqn_CGenTableHeaderType_Name].string;
                Dqn_CppStructBlock(cpp, "%.*s%.*sStr8ToEnumResult", DQN_STR_FMT(emit_prefix), DQN_STR_FMT(type_name)) {
                    Dqn_CppLine(cpp, "bool success;");
                    Dqn_CppLine(cpp, "%.*s%.*s value;", DQN_STR_FMT(emit_prefix), DQN_STR_FMT(type_name));
                }
                Dqn_CppNewLine(cpp);
            }
        }

        for (Dqn_CGenTable *table = cgen->first_table; table != 0; table = table->next) {
            if (table->type != Dqn_CGenTableType_CodeGenEnum)
                continue;

            for (Dqn_CGenLookupTableIterator it = {}; Dqn_CGen_LookupNextTableInCodeGenTable(cgen, table, &it);) {
                Dqn_Str8 type_name = it.cgen_table_column[Dqn_CGenTableHeaderType_Name].string;
                Dqn_CppLine(cpp,
                            "%.*s%.*sStr8ToEnumResult %.*s%.*s_Str8ToEnum(Dqn_Str8 string);",
                            DQN_STR_FMT(emit_prefix),
                            DQN_STR_FMT(type_name),
                            DQN_STR_FMT(emit_prefix),
                            DQN_STR_FMT(type_name));
            }
        }

        // NOTE: Operator == and != ////////////////////////////////////////////////////////////////
        for (Dqn_CGenTable *table = cgen->first_table; table != 0; table = table->next) {
            if (table->type != Dqn_CGenTableType_CodeGenStruct)
                continue;

            for (Dqn_CGenLookupTableIterator it = {}; Dqn_CGen_LookupNextTableInCodeGenTable(cgen, table, &it);) {
                Dqn_Str8 cpp_op_equals = it.cgen_table_column[Dqn_CGenTableHeaderType_CppOpEquals].string;
                if (cpp_op_equals != DQN_STR8("true"))
                    continue;

                Dqn_Str8 type_name = it.cgen_table_column[Dqn_CGenTableHeaderType_Name].string;
                Dqn_CppLine(cpp,
                            "bool operator==(%.*s%.*s const &lhs, %.*s%.*s const &rhs);",
                            DQN_STR_FMT(emit_prefix),
                            DQN_STR_FMT(type_name),
                            DQN_STR_FMT(emit_prefix),
                            DQN_STR_FMT(type_name));
                Dqn_CppLine(cpp,
                            "bool operator!=(%.*s%.*s const &lhs, %.*s%.*s const &rhs);",
                            DQN_STR_FMT(emit_prefix),
                            DQN_STR_FMT(type_name),
                            DQN_STR_FMT(emit_prefix),
                            DQN_STR_FMT(type_name));
            }
        }
    }

    if (emit & Dqn_CGenEmit_Implementation) {
        // NOTE: Generate type info ////////////////////////////////////////////////////////////////////
        for (Dqn_CGenTable *table = cgen->first_table; table != 0; table = table->next) {
            for (Dqn_CGenLookupTableIterator it = {}; Dqn_CGen_LookupNextTableInCodeGenTable(cgen, table, &it);) {

                if (table->type == Dqn_CGenTableType_CodeGenBuiltinTypes)
                    continue;

                Dqn_Str8 struct_or_enum_name = it.cgen_table_column[Dqn_CGenTableHeaderType_Name].string;
                Dqn_CppBlock(cpp, ";\n\n", "Dqn_TypeField const g_%.*s%.*s_type_fields[] =", DQN_STR_FMT(emit_prefix), DQN_STR_FMT(struct_or_enum_name)) {
                    if (table->type == Dqn_CGenTableType_CodeGenStruct) {

                        // NOTE: Construct the cpp type string first. We will prepend `emit_prefix`
                        // for types that are declared in the same mdesk file. We will also
                        // calculate the longest type name that we will generate for whitespace
                        // padding purposes.
                        Dqn_TLSTMem tmem                  = Dqn_TLS_PushTMem(nullptr);
                        Dqn_usize   longest_cpp_type_name = 0;
                        auto        cpp_type_list         = Dqn_SArray_Init<Dqn_Str8>(tmem.arena, it.table->row_count, Dqn_ZeroMem_Yes);

                        for (Dqn_usize row_index = 0; row_index < it.table->row_count; row_index++) {
                            Dqn_CGenTableRow const *row           = it.table->rows + row_index;
                            Dqn_CGenLookupColumnAtHeader cpp_type = Dqn_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[Dqn_CGenTableHeaderType_CppType].string, row);
                            Dqn_Str8 cpp_type_name                = Dqn_CGen_StripQualifiersOnCppType_(tmem.arena, cpp_type.column.string);
                            if (Dqn_CGen_WillCodeGenTypeName(cgen, cpp_type_name))
                                cpp_type_name = Dqn_Str8_InitF_TLS("%.*s%.*s", DQN_STR_FMT(emit_prefix), DQN_STR_FMT(cpp_type_name));

                            longest_cpp_type_name = DQN_MAX(longest_cpp_type_name, cpp_type_name.size);
                            Dqn_SArray_Add(&cpp_type_list, cpp_type_name);
                        }

                        // NOTE: Iterate each row and emit the C++ declarations ////////////////////
                        for (Dqn_usize row_index = 0; row_index < it.table->row_count; row_index++) {
                            Dqn_CGenTableRow const *row                       = it.table->rows + row_index;
                            Dqn_CGenLookupColumnAtHeader cpp_name             = Dqn_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[Dqn_CGenTableHeaderType_CppName].string,           row);
                            Dqn_CGenLookupColumnAtHeader cpp_type             = Dqn_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[Dqn_CGenTableHeaderType_CppType].string,           row);
                            Dqn_CGenLookupColumnAtHeader cpp_is_ptr           = Dqn_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[Dqn_CGenTableHeaderType_CppIsPtr].string,          row);
                            Dqn_CGenLookupColumnAtHeader cpp_array_size       = Dqn_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[Dqn_CGenTableHeaderType_CppArraySize].string,      row);
                            Dqn_CGenLookupColumnAtHeader cpp_array_size_field = Dqn_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[Dqn_CGenTableHeaderType_CppArraySizeField].string, row);
                            Dqn_CGenLookupColumnAtHeader cpp_label            = Dqn_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[Dqn_CGenTableHeaderType_CppLabel].string,          row);

                            bool     cpp_is_ptr_b32      = cpp_is_ptr.column.string == DQN_STR8("true");
                            Dqn_Str8 cpp_array_size_str8 = Dqn_Str8_HasData(cpp_array_size.column.string) ? cpp_array_size.column.string : DQN_STR8("0");

                            Dqn_CGenTableColumn struct_name               = it.cgen_table_row->columns[table->column_indexes[Dqn_CGenTableHeaderType_Name]];
                            Dqn_Str8            cpp_array_size_field_str8 = DQN_STR8("NULL");
                            if (cpp_array_size_field.column.string.size) {

                                // TODO(doyle): Check that array_size_field references a valid field in the table
                                // NOTE: We use a raw index for the reference because the struct might
                                // not have type info being generated so we can't rely on the enum.
                                Dqn_usize index_the_field_references = 0;
                                for (Dqn_usize sub_row_index = 0; sub_row_index < it.table->row_count; sub_row_index++) {
                                    Dqn_CGenTableRow const *sub_row      = it.table->rows + sub_row_index;
                                    Dqn_CGenTableColumn     sub_cpp_name = sub_row->columns[cpp_name.index];
                                    if (sub_cpp_name.string == cpp_array_size_field.column.string)
                                        index_the_field_references = sub_row_index;
                                }
                                cpp_array_size_field_str8 =
                                    Dqn_Str8_InitF_TLS("&g_%.*s%.*s_type_fields[%zu]",
                                                       DQN_STR_FMT(emit_prefix),
                                                       DQN_STR_FMT(struct_name.string),
                                                       index_the_field_references);
                            }

                            Dqn_Str8  cpp_type_name         = cpp_type_list.data[row_index];
                            Dqn_Str8  orig_cpp_type         = Dqn_CGen_StripQualifiersOnCppType_(tmem.arena, cpp_type.column.string);

                            Dqn_usize cpp_name_padding      = 1 + it.table->headers[cpp_name.index].longest_string  - cpp_name.column.string.size;
                            Dqn_usize cpp_type_padding      = 1 + longest_cpp_type_name                             - cpp_type_name.size;

                            Dqn_Str8  cpp_type_enum         = Dqn_Str8_InitF_TLS("%.*sType_%.*s", DQN_STR_FMT(emit_prefix), DQN_STR_FMT(orig_cpp_type));
                            Dqn_usize cpp_type_enum_padding = cpp_type_padding + (orig_cpp_type.size - cpp_type_name.size);

                            Dqn_Str8  cpp_label_str8         = cpp_name.column.string;
                            Dqn_usize cpp_label_str8_padding = cpp_name_padding;
                            if (cpp_label.column.string.size) {
                                cpp_label_str8         = cpp_label.column.string;
                                cpp_label_str8_padding = 1 + it.table->headers[cpp_label.index].longest_string - cpp_label.column.string.size;
                            }

                            Dqn_Str8Builder builder = Dqn_Str8Builder_Init(tmem.arena);

                            // NOTE: row
                            Dqn_Str8Builder_AddF(&builder, "{%2d, ", row_index);

                            // NOTE: name
                            Dqn_Str8Builder_AddF(&builder,
                                                 "DQN_STR8(\"%.*s\"),%*s",
                                                 DQN_STR_FMT(cpp_name.column.string),
                                                 cpp_name_padding, "");

                            // NOTE: label
                            Dqn_Str8Builder_AddF(&builder,
                                                 "DQN_STR8(\"%.*s\"),%*s",
                                                 DQN_STR_FMT(cpp_label_str8),
                                                 cpp_label_str8_padding, "");

                            // NOTE: value
                            Dqn_Str8Builder_AddF(&builder,
                                                 "/*value*/ 0, ");


                            // NOTE: offsetof(a, b)
                            Dqn_Str8Builder_AddF(&builder,
                                                 "offsetof(%.*s%.*s, %.*s),%*s",
                                                 DQN_STR_FMT(emit_prefix),
                                                 DQN_STR_FMT(struct_or_enum_name),
                                                 DQN_STR_FMT(cpp_name.column.string),
                                                 cpp_name_padding, "");

                            // NOTE: sizeof(a->b)
                            Dqn_Str8Builder_AddF(&builder,
                                                 "sizeof(((%.*s%.*s*)0)->%.*s),%*s",
                                                 DQN_STR_FMT(emit_prefix),
                                                 DQN_STR_FMT(struct_or_enum_name),
                                                 DQN_STR_FMT(cpp_name.column.string),
                                                 cpp_name_padding, "");

                            // NOTE: alignof(a)
                            if (cpp_type_name == DQN_STR8("void")) {
                                Dqn_Str8  proxy_type         = DQN_STR8("char");
                                Dqn_usize proxy_type_padding = 1 + longest_cpp_type_name - proxy_type.size;
                                Dqn_Str8Builder_AddF(&builder, "alignof(%.*s),%*s", DQN_STR_FMT(proxy_type), proxy_type_padding, "");
                            } else {
                                Dqn_Str8Builder_AddF(&builder,
                                                     "alignof(%.*s),%*s",
                                                     DQN_STR_FMT(cpp_type_name),
                                                     cpp_type_padding, "");
                            }

                            // NOTE: Type string
                            Dqn_Str8Builder_AddF(&builder,
                                                 "DQN_STR8(\"%.*s\"),%*s",
                                                 DQN_STR_FMT(cpp_type_name),
                                                 cpp_type_padding, "");

                            // NOTE: Type as enum
                            Dqn_Str8Builder_AddF(&builder,
                                                 "%.*s,%*s",
                                                 DQN_STR_FMT(cpp_type_enum),
                                                 cpp_type_enum_padding, "");

                            Dqn_Str8Builder_AddF(&builder,
                                                 "/*is_pointer*/ %s,%s /*array_size*/ %.*s, /*array_size_field*/ %.*s},",
                                                 cpp_is_ptr_b32 ? "true" : "false",
                                                 cpp_is_ptr_b32 ? " " : "",
                                                 DQN_STR_FMT(cpp_array_size_str8),
                                                 DQN_STR_FMT(cpp_array_size_field_str8));

                            Dqn_Str8 line = Dqn_Str8Builder_Build(&builder, tmem.arena);
                            Dqn_CppLine(cpp, "%.*s", DQN_STR_FMT(line));
                        }
                    } else {
                        DQN_ASSERT(table->type == Dqn_CGenTableType_CodeGenEnum);
                        for (Dqn_usize row_index = 0; row_index < it.table->row_count; row_index++) {
                            Dqn_CGenTableRow const      *row       = it.table->rows + row_index;
                            Dqn_CGenLookupColumnAtHeader cpp_name  = Dqn_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[Dqn_CGenTableHeaderType_CppName].string, row);
                            Dqn_CGenLookupColumnAtHeader cpp_value = Dqn_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[Dqn_CGenTableHeaderType_CppValue].string, row);
                            Dqn_CGenLookupColumnAtHeader cpp_label = Dqn_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[Dqn_CGenTableHeaderType_CppLabel].string, row);
                            if (cpp_name.column.string.size <= 0)
                                continue;

                            Dqn_TLSTMem tmem             = Dqn_TLS_PushTMem(nullptr);
                            Dqn_usize   cpp_name_padding = 1 + it.table->headers[cpp_name.index].longest_string - cpp_name.column.string.size;
                            Dqn_Str8    cpp_value_str8   = Dqn_Str8_HasData(cpp_value.column.string) ? cpp_value.column.string : Dqn_Str8_InitF_TLS("%zu", row_index);
                            Dqn_Str8  cpp_type_enum      = Dqn_Str8_InitF_TLS("%.*sType_%.*s", DQN_STR_FMT(emit_prefix), DQN_STR_FMT(struct_or_enum_name));

                            Dqn_Str8  cpp_label_str8         = cpp_name.column.string;
                            Dqn_usize cpp_label_str8_padding = cpp_name_padding;
                            if (cpp_label.column.string.size) {
                                cpp_label_str8         = cpp_label.column.string;
                                cpp_label_str8_padding = 1 + it.table->headers[cpp_label.index].longest_string - cpp_label.column.string.size;
                            }

                            Dqn_Str8Builder builder = Dqn_Str8Builder_Init_TLS();
                            // NOTE: row
                            Dqn_Str8Builder_AddF(&builder, "{%2d, ", row_index);

                            // NOTE: name
                            Dqn_Str8Builder_AddF(&builder,
                                                 "DQN_STR8(\"%.*s\"),%*s",
                                                 DQN_STR_FMT(cpp_name.column.string),
                                                 cpp_name_padding, "");

                            // NOTE: label
                            Dqn_Str8Builder_AddF(&builder,
                                                 "DQN_STR8(\"%.*s\"),%*s",
                                                 DQN_STR_FMT(cpp_label_str8),
                                                 cpp_label_str8_padding, "");

                            // NOTE: value
                            Dqn_Str8Builder_AddF(&builder, "/*value*/ %.*s, ", DQN_STR_FMT(cpp_value_str8));

                            // NOTE: offsetof(a, b)
                            Dqn_Str8Builder_AddF(&builder, "/*offsetof*/ 0, ");

                            // NOTE: sizeof(a->b)
                            Dqn_Str8Builder_AddF(&builder,
                                                 "sizeof(%.*s%.*s), ",
                                                 DQN_STR_FMT(emit_prefix),
                                                 DQN_STR_FMT(struct_or_enum_name));

                            // NOTE: alignof(a->b)
                            Dqn_Str8Builder_AddF(&builder,
                                                 "alignof(%.*s%.*s), ",
                                                 DQN_STR_FMT(emit_prefix),
                                                 DQN_STR_FMT(struct_or_enum_name));

                            // TODO: Type string
                            Dqn_Str8Builder_AddF(&builder, "DQN_STR8(\"\"), ");

                            // NOTE: Type as enum
                            Dqn_Str8Builder_AddF(&builder, "%.*s, ", DQN_STR_FMT(cpp_type_enum));

                            Dqn_Str8Builder_AddF(&builder, "/*is_pointer*/ false, ");
                            Dqn_Str8Builder_AddF(&builder, "/*array_size*/ 0, ");
                            Dqn_Str8Builder_AddF(&builder, "/*array_size_field*/ NULL},");

                            Dqn_Str8 line = Dqn_Str8Builder_Build_TLS(&builder);
                            Dqn_CppLine(cpp, "%.*s", DQN_STR_FMT(line));
                        }
                    }
                }
            }
        }

        int longest_name_across_all_tables = 0;
        for (Dqn_CGenTable *table = cgen->first_table; table != 0; table = table->next) {
            for (Dqn_CGenLookupTableIterator it = {}; Dqn_CGen_LookupNextTableInCodeGenTable(cgen, table, &it);) {

                Dqn_TLSTMem tmem = Dqn_TLS_TMem(nullptr);
                Dqn_Str8 type_name  = it.cgen_table_column[Dqn_CGenTableHeaderType_Name].string;
                if (Dqn_CGen_WillCodeGenTypeName(cgen, type_name))
                    type_name = Dqn_Str8_InitF(tmem.arena, "%.*s%.*s", DQN_STR_FMT(emit_prefix), DQN_STR_FMT(type_name));

                longest_name_across_all_tables = DQN_MAX(longest_name_across_all_tables, DQN_CAST(int)type_name.size);
            }
        }

        Dqn_CppBlock(cpp, ";\n\n", "Dqn_TypeInfo const g_%.*stypes[] =", DQN_STR_FMT(emit_prefix)) {
            Dqn_CppLine(cpp, "{DQN_STR8(\"\"),%*sDqn_TypeKind_Nil,    0, /*fields*/ NULL, /*count*/ 0},", 1 + longest_name_across_all_tables, "");

            Dqn_usize longest_type_name = 0;
            for (Dqn_CGenTable *table = cgen->first_table; table != 0; table = table->next) {
                for (Dqn_CGenLookupTableIterator it = {}; Dqn_CGen_LookupNextTableInCodeGenTable(cgen, table, &it);) {
                    Dqn_TLSTMem tmem = Dqn_TLS_TMem(nullptr);
                    Dqn_Str8 type_name  = it.cgen_table_column[Dqn_CGenTableHeaderType_Name].string;
                    if (Dqn_CGen_WillCodeGenTypeName(cgen, type_name))
                        type_name = Dqn_Str8_InitF(tmem.arena, "%.*s%.*s", DQN_STR_FMT(emit_prefix), DQN_STR_FMT(type_name));
                    longest_type_name = DQN_MAX(longest_type_name, type_name.size);
                }
            }

            for (Dqn_CGenTable *table = cgen->first_table; table != 0; table = table->next) {
                for (Dqn_CGenLookupTableIterator it = {}; Dqn_CGen_LookupNextTableInCodeGenTable(cgen, table, &it);) {
                    Dqn_TLSTMem tmem   = Dqn_TLS_PushTMem(nullptr);
                    Dqn_Str8 type_name = it.cgen_table_column[Dqn_CGenTableHeaderType_Name].string;
                    if (Dqn_CGen_WillCodeGenTypeName(cgen, type_name))
                        type_name = Dqn_Str8_InitF_TLS("%.*s%.*s", DQN_STR_FMT(emit_prefix), DQN_STR_FMT(type_name));

                    int name_padding = 1 + longest_name_across_all_tables - DQN_CAST(int) type_name.size;
                    Dqn_Str8    type_info_kind         = {};
                    char const *type_info_kind_padding = "";
                    if (table->type == Dqn_CGenTableType_CodeGenEnum) {
                        type_info_kind         = DQN_STR8("Dqn_TypeKind_Enum");
                        type_info_kind_padding = "  ";
                    } else if (table->type == Dqn_CGenTableType_CodeGenStruct) {
                        type_info_kind = DQN_STR8("Dqn_TypeKind_Struct");
                    } else {
                        DQN_ASSERT(table->type == Dqn_CGenTableType_CodeGenBuiltinTypes);
                        type_info_kind         = DQN_STR8("Dqn_TypeKind_Basic");
                        type_info_kind_padding = " ";
                    }

                    Dqn_Str8 fields_count = {};
                    if (table->type == Dqn_CGenTableType_CodeGenBuiltinTypes) {
                        fields_count = DQN_STR8("0");
                    } else {
                        DQN_ASSERT(table->type == Dqn_CGenTableType_CodeGenStruct || table->type == Dqn_CGenTableType_CodeGenEnum);
                        int fields_count_int = 0;
                        for (Dqn_usize row_index = 0; row_index < it.table->row_count; row_index++) {
                            Dqn_CGenTableRow const      *row      = it.table->rows + row_index;
                            Dqn_CGenLookupColumnAtHeader cpp_name = Dqn_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[Dqn_CGenTableHeaderType_CppName].string, row);
                            fields_count_int += Dqn_Str8_HasData(cpp_name.column.string);
                        }
                        fields_count = Dqn_Str8_InitF_TLS("%d", fields_count_int);
                    }

                    Dqn_Str8 fields         = DQN_STR8("NULL");
                    int      fields_padding = 1;
                    if (table->type != Dqn_CGenTableType_CodeGenBuiltinTypes) {
                        fields_padding = name_padding;
                        fields         = Dqn_Str8_InitF(tmem.arena, "g_%.*s_type_fields", DQN_STR_FMT(type_name));
                    }

                    Dqn_Str8Builder builder = Dqn_Str8Builder_Init_TLS();

                    // NOTE: name
                    Dqn_Str8Builder_AddF(&builder,
                                         "{DQN_STR8(\"%.*s\"),%*s",
                                         DQN_STR_FMT(type_name),
                                         name_padding,
                                         "");

                    // NOTE: Dqn_TypeKind_{Nil|Basic|Enum|Struct}
                    Dqn_Str8Builder_AddF(&builder,
                                         "%.*s,%s",
                                         DQN_STR_FMT(type_info_kind),
                                         type_info_kind_padding);

                    // NOTE: sizeof(T)
                    if (type_name == DQN_STR8("void")) {
                        Dqn_Str8Builder_AddF(&builder, "0,%*s", name_padding, "");
                    } else {
                        Dqn_Str8Builder_AddF(&builder,
                                             "sizeof(%.*s),%*s",
                                             DQN_STR_FMT(type_name),
                                             name_padding,
                                             "");
                    }

                    // NOTE: Pointer to Dqn_TypeField[]
                    Dqn_Str8Builder_AddF(&builder,
                                         "/*fields*/ %.*s,%*s",
                                         DQN_STR_FMT(fields),
                                         fields_padding,
                                         "");

                    // NOTE: Dqn_TypeField length
                    Dqn_Str8Builder_AddF(&builder,
                                         "/*count*/ %.*s},",
                                         DQN_STR_FMT(fields_count));

                    Dqn_Str8 line = Dqn_Str8Builder_Build_TLS(&builder);
                    Dqn_CppLine(cpp, "%.*s", DQN_STR_FMT(line));
                }
            }
        }

        // NOTE: Str8 to enum conversion ////////////////////////////////////////////////////////////
        for (Dqn_CGenTable *table = cgen->first_table; table != 0; table = table->next) {
            if (table->type != Dqn_CGenTableType_CodeGenEnum)
                continue;

            for (Dqn_CGenLookupTableIterator it = {}; Dqn_CGen_LookupNextTableInCodeGenTable(cgen, table, &it);) {
                Dqn_Str8 type_name = it.cgen_table_column[Dqn_CGenTableHeaderType_Name].string;

                Dqn_CppFuncBlock(cpp, "%.*s%.*sStr8ToEnumResult %.*s%.*s_Str8ToEnum(Dqn_Str8 string)", DQN_STR_FMT(emit_prefix), DQN_STR_FMT(type_name), DQN_STR_FMT(emit_prefix), DQN_STR_FMT(type_name)) {
                    Dqn_CppLine(cpp, "%.*s%.*sStr8ToEnumResult result = {};", DQN_STR_FMT(emit_prefix), DQN_STR_FMT(type_name));
                    Dqn_CppForBlock(cpp, "Dqn_usize index = 0; !result.success && index < DQN_ARRAY_UCOUNT(g_%.*s%.*s_type_fields); index++", DQN_STR_FMT(emit_prefix), DQN_STR_FMT(type_name)) {
                        Dqn_CppIfChain(cpp) {
                            Dqn_CppLine(cpp, "Dqn_TypeField field = g_%.*s%.*s_type_fields[index];", DQN_STR_FMT(emit_prefix), DQN_STR_FMT(type_name));
                            Dqn_CppIfOrElseIfBlock(cpp, "string == field.name") {
                                Dqn_CppLine(cpp, "result.success = true;");
                                Dqn_CppLine(cpp, "result.value = (%.*s%.*s)index;", DQN_STR_FMT(emit_prefix), DQN_STR_FMT(type_name));
                            }
                        }
                    }
                    Dqn_CppLine(cpp, "return result;");
                }
                Dqn_CppNewLine(cpp);
            }
        }

        // NOTE: Operator == and != ////////////////////////////////////////////////////////////////
        for (Dqn_CGenTable *table = cgen->first_table; table != 0; table = table->next) {
            if (table->type != Dqn_CGenTableType_CodeGenStruct)
                continue;

            for (Dqn_CGenLookupTableIterator it = {}; Dqn_CGen_LookupNextTableInCodeGenTable(cgen, table, &it);) {
                Dqn_Str8 cpp_op_equals = it.cgen_table_column[Dqn_CGenTableHeaderType_CppOpEquals].string;
                if (cpp_op_equals != DQN_STR8("true"))
                    continue;

                Dqn_Str8 type_name = it.cgen_table_column[Dqn_CGenTableHeaderType_Name].string;
                Dqn_CppFuncBlock(cpp, "bool operator==(%.*s%.*s const &lhs, %.*s%.*s const &rhs)", DQN_STR_FMT(emit_prefix), DQN_STR_FMT(type_name), DQN_STR_FMT(emit_prefix), DQN_STR_FMT(type_name)) {
                    for (Dqn_usize row_index = 0; row_index < it.table->row_count; row_index++) {
                        Dqn_CGenTableRow const *row                       = it.table->rows + row_index;
                        Dqn_CGenLookupColumnAtHeader cpp_name             = Dqn_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[Dqn_CGenTableHeaderType_CppName].string,           row);
                        Dqn_CGenLookupColumnAtHeader cpp_is_ptr           = Dqn_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[Dqn_CGenTableHeaderType_CppIsPtr].string,          row);
                        Dqn_CGenLookupColumnAtHeader cpp_array_size       = Dqn_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[Dqn_CGenTableHeaderType_CppArraySize].string,      row);
                        Dqn_CGenLookupColumnAtHeader cpp_array_size_field = Dqn_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[Dqn_CGenTableHeaderType_CppArraySizeField].string, row);

                        // TODO(doyle): Check if we're an integral type or not to double check if we
                        // can use memcmp or operator==
                        if (Dqn_Str8_HasData(cpp_array_size_field.column.string)) {
                            Dqn_CppIfChain(cpp) {
                                Dqn_CppIfOrElseIfBlock(cpp,
                                                       "lhs.%.*s != rhs.%.*s",
                                                       DQN_STR_FMT(cpp_array_size_field.column.string),
                                                       DQN_STR_FMT(cpp_array_size_field.column.string)) {
                                    Dqn_CppLine(cpp, "return false;");
                                }
                            }
                            Dqn_CppIfChain(cpp) {
                                Dqn_CppIfOrElseIfBlock(cpp,
                                                       "DQN_MEMCMP(lhs.%.*s, rhs.%.*s, lhs.%.*s) != 0",
                                                       DQN_STR_FMT(cpp_name.column.string),
                                                       DQN_STR_FMT(cpp_name.column.string),
                                                       DQN_STR_FMT(cpp_array_size_field.column.string)) {
                                    Dqn_CppLine(cpp, "return false;");
                                }
                            }
                        } else if (Dqn_Str8_HasData(cpp_array_size.column.string)) {
                            Dqn_CppIfChain(cpp) {
                                Dqn_CppIfOrElseIfBlock(cpp,
                                                       "DQN_MEMCMP(lhs.%.*s, rhs.%.*s, %.*s) != 0",
                                                       DQN_STR_FMT(cpp_name.column.string),
                                                       DQN_STR_FMT(cpp_name.column.string),
                                                       DQN_STR_FMT(cpp_array_size.column.string)) {
                                    Dqn_CppLine(cpp, "return false;");
                                }
                            }
                        } else if (cpp_is_ptr.column.string == DQN_STR8("true")) {
                            Dqn_CppIfChain(cpp) {
                                Dqn_CppIfOrElseIfBlock(cpp,
                                            "*lhs.%.*s != *rhs.%.*s",
                                            DQN_STR_FMT(cpp_name.column.string),
                                            DQN_STR_FMT(cpp_name.column.string)) {
                                    Dqn_CppLine(cpp, "return false;");
                                }
                            }
                        } else {
                            Dqn_CppIfChain(cpp) {
                                Dqn_CppIfOrElseIfBlock(cpp,
                                            "lhs.%.*s != rhs.%.*s",
                                            DQN_STR_FMT(cpp_name.column.string),
                                            DQN_STR_FMT(cpp_name.column.string)) {
                                    Dqn_CppLine(cpp, "return false;");
                                }
                            }
                        }
                    }
                    Dqn_CppLine(cpp, "return true;");
                }
                Dqn_CppNewLine(cpp);

                Dqn_CppFuncBlock(cpp, "bool operator!=(%.*s%.*s const &lhs, %.*s%.*s const &rhs)", DQN_STR_FMT(emit_prefix), DQN_STR_FMT(type_name), DQN_STR_FMT(emit_prefix), DQN_STR_FMT(type_name)) {
                    Dqn_CppLine(cpp, "bool result = !(lhs == rhs);");
                    Dqn_CppLine(cpp, "return result;");
                }
                Dqn_CppNewLine(cpp);
            }
        }
    }
}
