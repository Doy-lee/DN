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
//   dn_cgen.cpp
//
////////////////////////////////////////////////////////////////////////////////////////////////////
*/

DN_CGenMapNodeToEnum const DN_CGEN_TABLE_KEY_LIST[] = {
  {DN_CGenTableKeyType_Name, DN_Str8Lit("name")},
  {DN_CGenTableKeyType_Type, DN_Str8Lit("type")},
};

DN_CGenMapNodeToEnum const DN_CGEN_TABLE_TYPE_LIST[] = {
  {DN_CGenTableType_Data,                DN_Str8Lit("data")                  },
  {DN_CGenTableType_CodeGenBuiltinTypes, DN_Str8Lit("code_gen_builtin_types")},
  {DN_CGenTableType_CodeGenStruct,       DN_Str8Lit("code_gen_struct")       },
  {DN_CGenTableType_CodeGenEnum,         DN_Str8Lit("code_gen_enum")         },
};

DN_CGenMapNodeToEnum const DN_CGEN_TABLE_ROW_TAG_LIST[] = {
  {DN_CGenTableRowTagType_CommentDivider, DN_Str8Lit("comment_divider")},
  {DN_CGenTableRowTagType_EmptyLine,      DN_Str8Lit("empty_line")     },
};

DN_CGenMapNodeToEnum const DN_CGEN_TABLE_ROW_TAG_COMMENT_DIVIDER_KEY_LIST[] = {
  {DN_CGenTableRowTagCommentDivider_Label, DN_Str8Lit("label")},
};

DN_CGenTableHeaderType const DN_CGEN_TABLE_CODE_GEN_STRUCT_HEADER_LIST[] = {
  DN_CGenTableHeaderType_Name,
  DN_CGenTableHeaderType_Table,
  DN_CGenTableHeaderType_CppType,
  DN_CGenTableHeaderType_CppName,
  DN_CGenTableHeaderType_CppIsPtr,
  DN_CGenTableHeaderType_CppOpEquals,
  DN_CGenTableHeaderType_CppArraySize,
  DN_CGenTableHeaderType_CppArraySizeField,
  DN_CGenTableHeaderType_CppLabel,
  DN_CGenTableHeaderType_GenTypeInfo,
};

DN_CGenTableHeaderType const DN_CGEN_TABLE_CODE_GEN_ENUM_HEADER_LIST[] = {
  DN_CGenTableHeaderType_Name,
  DN_CGenTableHeaderType_Table,
  DN_CGenTableHeaderType_CppName,
  DN_CGenTableHeaderType_CppValue,
  DN_CGenTableHeaderType_CppLabel,
  DN_CGenTableHeaderType_GenTypeInfo,
  DN_CGenTableHeaderType_GenEnumCount,
};

DN_CGenTableHeaderType const DN_CGEN_TABLE_CODE_GEN_BUILTIN_TYPES_HEADER_LIST[] = {
  DN_CGenTableHeaderType_Name,
};

static bool DN_CGen_GatherTables_(DN_CGen *cgen, DN_ErrSink *err)
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

      DN_CGenTable *table = MD_PushArray(cgen->arena, DN_CGenTable, 1);
      table->node         = node;
      table->name         = DN_CGen_MDToDNStr8(table_tag->first_child->first_child->string);
      MD_MapInsert(cgen->arena, &cgen->table_map, MD_MapKeyStr(DN_CGen_DNToMDStr8(table->name)), table);
      MD_QueuePush(cgen->first_table, cgen->last_table, table);

      for (MD_EachNode(key, table_tag->first_child)) {
        DN_CGenMapNodeToEnum key_mapping = DN_CGen_MapNodeToEnumOrExit(key,
                                                                       DN_CGEN_TABLE_KEY_LIST,
                                                                       DN_ArrayCountU(DN_CGEN_TABLE_KEY_LIST),
                                                                       "Table specified invalid key");
        switch (DN_Cast(DN_CGenTableKeyType) key_mapping.enum_val) {
          case DN_CGenTableKeyType_Nil: DN_InvalidCodePath;

          case DN_CGenTableKeyType_Name: {
            table->name = DN_CGen_MDToDNStr8(key->first_child->string);
          } break;

          case DN_CGenTableKeyType_Type: {
            MD_Node             *table_type_value     = key->first_child;
            DN_CGenMapNodeToEnum table_type_validator = DN_CGen_MapNodeToEnumOrExit(table_type_value,
                                                                                    DN_CGEN_TABLE_TYPE_LIST,
                                                                                    DN_ArrayCountU(DN_CGEN_TABLE_TYPE_LIST),
                                                                                    "Table 'type' specified invalid value");
            table->type                               = DN_Cast(DN_CGenTableType) table_type_validator.enum_val;

            DN_Assert(table->type <= DN_CGenTableType_Count);
            cgen->table_counts[table->type]++;
          } break;
        }
      }
    }
  }

  // NOTE: Parse the tables //////////////////////////////////////////////////////////////////////
  DN_USize const BEGIN_COLUMN_INDEX = 1; /*Reserve 0th slot for nil-entry*/
  for (DN_CGenTable *table = cgen->first_table; table; table = table->next) {
    table->column_count = BEGIN_COLUMN_INDEX;
    table->headers_node = table->node->first_child;
    for (MD_EachNode(column_node, table->headers_node->first_child))
      table->column_count++;

    if (table->column_count == BEGIN_COLUMN_INDEX)
      continue;

    MD_Node *row_it = table->headers_node->next;
    for (MD_EachNode(row_node, row_it))
      table->row_count++;

    table->rows    = MD_PushArray(cgen->arena, DN_CGenTableRow, table->row_count);
    table->headers = MD_PushArray(cgen->arena, DN_CGenTableHeader, table->column_count);
    for (DN_USize index = 0; index < table->row_count; index++) {
      DN_CGenTableRow *row = table->rows + index;
      row->columns         = MD_PushArray(cgen->arena, DN_CGenTableColumn, table->column_count);
    }

    // NOTE: Collect table headers /////////////////////////////////////////////////////////////
    table->headers_map    = MD_MapMake(cgen->arena);
    DN_USize column_index = BEGIN_COLUMN_INDEX;
    for (MD_EachNode(header_column, table->headers_node->first_child)) {
      DN_Assert(column_index < table->column_count);

      // NOTE: Detect builtin headers and cache the index for that table /////////////////////
      for (DN_USize enum_index = 0; enum_index < DN_CGenTableHeaderType_Count; enum_index++) {
        DN_Str8 decl_str8 = DN_CGen_TableHeaderTypeToDeclStr8(DN_Cast(DN_CGenTableHeaderType) enum_index);
        if (!DN_Str8Eq(decl_str8, DN_Str8FromPtr(header_column->string.str, header_column->string.size)))
          continue;
        table->column_indexes[enum_index] = column_index;
        break;
      }

      MD_MapInsert(cgen->arena, &table->headers_map, MD_MapKeyStr(header_column->string), DN_Cast(void *) column_index);
      table->headers[column_index++].name = header_column->string;
    }

    // NOTE: Validate table headers ////////////////////////////////////////////////////////////
    switch (table->type) {
      case DN_CGenTableType_Nil: DN_InvalidCodePath;
      case DN_CGenTableType_Count: DN_InvalidCodePath;

      case DN_CGenTableType_Data: {
      } break;

      case DN_CGenTableType_CodeGenStruct: {
        for (DN_CGenTableHeaderType enum_val : DN_CGEN_TABLE_CODE_GEN_STRUCT_HEADER_LIST) {
          if (table->column_indexes[enum_val] == 0) {
            DN_Str8 expected_value = DN_CGen_TableHeaderTypeToDeclStr8(enum_val);
            DN_CGen_LogF(MD_MessageKind_Error, table->headers_node, err, "Struct code generation table is missing column '%.*s'", DN_Str8PrintFmt(expected_value));
            return false;
          }
        }
      } break;

      case DN_CGenTableType_CodeGenEnum: {
        for (DN_CGenTableHeaderType enum_val : DN_CGEN_TABLE_CODE_GEN_ENUM_HEADER_LIST) {
          if (table->column_indexes[enum_val] == 0) {
            DN_Str8 expected_value = DN_CGen_TableHeaderTypeToDeclStr8(enum_val);
            DN_CGen_LogF(MD_MessageKind_Error, table->headers_node, err, "Enum code generation table is missing column '%.*s'", DN_Str8PrintFmt(expected_value));
            return false;
          }
        }
      } break;

      case DN_CGenTableType_CodeGenBuiltinTypes: {
        for (DN_CGenTableHeaderType enum_val : DN_CGEN_TABLE_CODE_GEN_BUILTIN_TYPES_HEADER_LIST) {
          if (table->column_indexes[enum_val] == 0) {
            DN_Str8 expected_value = DN_CGen_TableHeaderTypeToDeclStr8(enum_val);
            DN_CGen_LogF(MD_MessageKind_Error, table->headers_node, err, "Enum code generation table is missing column '%.*s'", DN_Str8PrintFmt(expected_value));
            return false;
          }
        }
      } break;
    }

    // NOTE: Parse each row in table ///////////////////////////////////////////////////////////
    DN_USize row_index = 0;
    for (MD_EachNode(row_node, row_it)) {
      column_index = BEGIN_COLUMN_INDEX;
      DN_Assert(row_index < table->row_count);

      // NOTE: Parse any tags set on the row /////////////////////////////////////////////////
      DN_CGenTableRow *row = table->rows + row_index++;
      for (MD_EachNode(row_tag, row_node->first_tag)) {
        DN_CGenMapNodeToEnum row_mapping = DN_CGen_MapNodeToEnumOrExit(row_tag,
                                                                       DN_CGEN_TABLE_ROW_TAG_LIST,
                                                                       DN_ArrayCountU(DN_CGEN_TABLE_ROW_TAG_LIST),
                                                                       "Table specified invalid row tag");
        DN_CGenTableRowTag  *tag         = MD_PushArray(cgen->arena, DN_CGenTableRowTag, 1);
        tag->type                        = DN_Cast(DN_CGenTableRowTagType) row_mapping.enum_val;
        MD_QueuePush(row->first_tag, row->last_tag, tag);

        switch (tag->type) {
          case DN_CGenTableRowTagType_Nil: DN_InvalidCodePath;

          case DN_CGenTableRowTagType_CommentDivider: {
            for (MD_EachNode(tag_key, row_tag->first_child)) {
              DN_CGenMapNodeToEnum tag_mapping = DN_CGen_MapNodeToEnumOrExit(tag_key,
                                                                             DN_CGEN_TABLE_ROW_TAG_COMMENT_DIVIDER_KEY_LIST,
                                                                             DN_ArrayCountU(DN_CGEN_TABLE_ROW_TAG_COMMENT_DIVIDER_KEY_LIST),
                                                                             "Table specified invalid row tag");
              switch (DN_Cast(DN_CGenTableRowTagCommentDivider) tag_mapping.enum_val) {
                case DN_CGenTableRowTagCommentDivider_Nil: DN_InvalidCodePath;
                case DN_CGenTableRowTagCommentDivider_Label: {
                  tag->comment = tag_key->first_child->string;
                } break;
              }
            }
          } break;

          case DN_CGenTableRowTagType_EmptyLine: break;
        }
      }

      for (MD_EachNode(column_node, row_node->first_child)) {
        table->headers[column_index].longest_string = DN_Max(table->headers[column_index].longest_string, DN_Cast(int) column_node->string.size);
        row->columns[column_index].string           = DN_CGen_MDToDNStr8(column_node->string);
        row->columns[column_index].node             = column_node;
        column_index++;
      }
    }
  }

  // NOTE: Validate codegen //////////////////////////////////////////////////////////////////////
  DN_CGenTableHeaderType const CHECK_COLUMN_LINKS[] = {
      DN_CGenTableHeaderType_CppName,
      DN_CGenTableHeaderType_CppType,
      DN_CGenTableHeaderType_CppValue,
      DN_CGenTableHeaderType_CppIsPtr,
      DN_CGenTableHeaderType_CppArraySize,
      DN_CGenTableHeaderType_CppArraySizeField,
  };

  result = true;
  for (DN_CGenTable *table = cgen->first_table; table; table = table->next) {
    for (DN_CGenLookupTableIterator it = {}; DN_CGen_LookupNextTableInCodeGenTable(cgen, table, &it);) {
      for (DN_CGenTableHeaderType check_column : CHECK_COLUMN_LINKS) {
        DN_CGenTableColumn column = it.cgen_table_row->columns[table->column_indexes[check_column]];
        if (column.string.size == 0) {
          // NOTE: The code generation table did not bind a code generation parameter to
          // a column in the target table. We skip it.
          continue;
        }

        // NOTE: Check if the column to bind to exists in the target table.
        if (!MD_MapLookup(&it.table->headers_map, MD_MapKeyStr(DN_CGen_DNToMDStr8(column.string)))) {
          result                   = false;
          DN_Str8 header_type_str8 = DN_CGen_TableHeaderTypeToDeclStr8(check_column);
          DN_CGen_LogF(MD_MessageKind_Error, column.node, err,
                       "Code generation table binds '%.*s' to '%.*s', but the column '%.*s' does not exist in table '%.*s'\n"
                       "NOTE: If you want '%.*s' to omit the column '%.*s' you can bind to the empty string `` to skip it, otherwise, please ensure the table '%.*s' has the column '%.*s'",
                       DN_Str8PrintFmt(column.string),
                       DN_Str8PrintFmt(header_type_str8),
                       DN_Str8PrintFmt(column.string),
                       DN_Str8PrintFmt(it.table->name),
                       DN_Str8PrintFmt(it.table->name),
                       DN_Str8PrintFmt(header_type_str8),
                       DN_Str8PrintFmt(it.table->name),
                       DN_Str8PrintFmt(header_type_str8));
        }
      }
    }
  }

  return result;
}

DN_API DN_CGen DN_CGen_FromFilesArgV(int argc, char const **argv, DN_ErrSink *err)
{
  DN_CGen result   = {};
  result.arena     = MD_ArenaAlloc();
  result.file_list = MD_MakeList(result.arena);
  result.table_map = MD_MapMake(result.arena);

  bool has_error = false;
  for (DN_ISize arg_index = 0; arg_index < argc; arg_index++) {
    MD_String8     file_name    = MD_S8CString(DN_Cast(char *) argv[arg_index]);
    MD_ParseResult parse_result = MD_ParseWholeFile(result.arena, file_name);
    for (MD_Message *message = parse_result.errors.first; message != 0; message = message->next) {
      has_error = true;
      DN_CGen_LogF(message->kind, message->node, err, "%.*s", MD_S8VArg(message->string));
    }
    MD_PushNewReference(result.arena, result.file_list, parse_result.node);
  }

  if (!has_error)
    DN_CGen_GatherTables_(&result, err);
  return result;
}

DN_API DN_Str8 DN_CGen_TableHeaderTypeToDeclStr8(DN_CGenTableHeaderType type)
{
  DN_Str8 result = {};
  switch (type) {
    case DN_CGenTableHeaderType_Name: result = DN_Str8Lit("name"); break;
    case DN_CGenTableHeaderType_Table: result = DN_Str8Lit("table"); break;
    case DN_CGenTableHeaderType_CppType: result = DN_Str8Lit("cpp_type"); break;
    case DN_CGenTableHeaderType_CppName: result = DN_Str8Lit("cpp_name"); break;
    case DN_CGenTableHeaderType_CppValue: result = DN_Str8Lit("cpp_value"); break;
    case DN_CGenTableHeaderType_CppIsPtr: result = DN_Str8Lit("cpp_is_ptr"); break;
    case DN_CGenTableHeaderType_CppOpEquals: result = DN_Str8Lit("cpp_op_equals"); break;
    case DN_CGenTableHeaderType_CppArraySize: result = DN_Str8Lit("cpp_array_size"); break;
    case DN_CGenTableHeaderType_CppArraySizeField: result = DN_Str8Lit("cpp_array_size_field"); break;
    case DN_CGenTableHeaderType_CppLabel: result = DN_Str8Lit("cpp_label"); break;
    case DN_CGenTableHeaderType_GenTypeInfo: result = DN_Str8Lit("gen_type_info"); break;
    case DN_CGenTableHeaderType_GenEnumCount: result = DN_Str8Lit("gen_enum_count"); break;
    case DN_CGenTableHeaderType_Count: result = DN_Str8Lit("XX BAD ENUM VALUE XX"); break;
    default: result = DN_Str8Lit("XX INVALID ENUM VALUE XX"); break;
  }
  return result;
}

DN_API DN_CGenMapNodeToEnum DN_CGen_MapNodeToEnumOrExit(MD_Node const *node, DN_CGenMapNodeToEnum const *valid_keys, DN_USize valid_keys_size, char const *fmt, ...)
{
  DN_CGenMapNodeToEnum result = {};
  for (DN_USize index = 0; index < valid_keys_size; index++) {
    DN_CGenMapNodeToEnum const *validator = valid_keys + index;
    if (DN_Str8Eq(DN_Str8FromPtr(node->string.str, node->string.size), validator->node_string)) {
      result = *validator;
      break;
    }
  }

  if (result.enum_val == 0) {
    MD_CodeLoc loc  = MD_CodeLocFromNode(DN_Cast(MD_Node *) node);
    DN_TCScratch tmem = DN_TCScratchBegin(nullptr, 0);
    va_list    args;
    va_start(args, fmt);
    DN_Str8 user_msg = DN_Str8FromFmtVArena(tmem.arena, fmt, args);
    va_end(args);

    DN_Str8Builder builder = {};
    builder.arena          = tmem.arena;

    DN_Str8BuilderAppendF(&builder, "%.*s: '%.*s' is not recognised, the supported values are ", DN_Str8PrintFmt(user_msg), MD_S8VArg(node->string));
    for (DN_USize index = 0; index < valid_keys_size; index++) {
      DN_CGenMapNodeToEnum const *validator = valid_keys + index;
      DN_Str8BuilderAppendF(&builder, DN_Cast(char *) "%s'%.*s'", index ? ", " : "", DN_Str8PrintFmt(validator->node_string));
    }

    DN_Str8 error_msg = DN_Str8BuilderBuild(&builder, tmem.arena);
    DN_TCScratchEnd(&tmem);
    MD_PrintMessageFmt(stderr, loc, MD_MessageKind_Error, DN_Cast(char *) "%.*s", DN_Str8PrintFmt(error_msg));
    DN_OS_Exit(DN_Cast(uint32_t) - 1);
  }
  return result;
}

DN_API DN_USize DN_CGen_NodeChildrenCount(MD_Node const *node)
{
  DN_USize result = 0;
  for (MD_EachNode(item, node->first_child))
    result++;
  return result;
}

DN_API void DN_CGen_LogF(MD_MessageKind kind, MD_Node *node, DN_ErrSink *err, char const *fmt, ...)
{
  if (!err)
    return;

  DN_TCScratch   tmem    = DN_TCScratchBegin(nullptr, 0);
  DN_Str8Builder builder = {};
  builder.arena = tmem.arena;

  MD_String8 kind_string = MD_StringFromMessageKind(kind);
  MD_CodeLoc loc         = MD_CodeLocFromNode(node);
  DN_Str8BuilderAppendF(&builder, "" MD_FmtCodeLoc " %.*s: ", MD_CodeLocVArg(loc), MD_S8VArg(kind_string));

  va_list args;
  va_start(args, fmt);
  DN_Str8BuilderAppendFV(&builder, fmt, args);
  va_end(args);

  DN_Str8 msg = DN_Str8BuilderBuild(&builder, err->arena);
  DN_TCScratchEnd(&tmem);
  DN_OS_ErrSinkAppendF(err, DN_Cast(uint32_t) - 1, "%.*s", DN_Str8PrintFmt(msg));
}

DN_API bool DN_CGen_TableHasHeaders(DN_CGenTable const *table, DN_Str8 const *headers, DN_USize header_count, DN_ErrSink *err)
{
  bool           result  = true;
  DN_TCScratch     tmem    = DN_TCScratchBegin(nullptr, 0);
  DN_Str8Builder builder = {};
  builder.arena          = tmem.arena;

  for (DN_USize index = 0; index < header_count; index++) {
    DN_Str8     header    = headers[index];
    MD_String8  header_md = {DN_Cast(MD_u8 *) header.data, header.size};
    MD_MapSlot *slot      = MD_MapLookup(DN_Cast(MD_Map *) & table->headers_map, MD_MapKeyStr(header_md));
    if (!slot) {
      result = false;
      DN_Str8BuilderAppendF(&builder, "%s%.*s", builder.count ? ", " : "", DN_Str8PrintFmt(header));
    }
  }

  if (!result) {
    DN_Str8 missing_headers = DN_Str8BuilderBuild(&builder, tmem.arena);
    DN_CGen_LogF(MD_MessageKind_Error,
                 table->headers_node,
                 err,
                 "Table '%.*s' is missing the header(s): %.*s",
                 DN_Str8PrintFmt(table->name),
                 DN_Str8PrintFmt(missing_headers));
  }

  DN_TCScratchEnd(&tmem);
  return result;
}

DN_API DN_CGenLookupColumnAtHeader DN_CGen_LookupColumnAtHeader(DN_CGenTable *table, DN_Str8 header, DN_CGenTableRow const *row)
{
  DN_CGenLookupColumnAtHeader result = {};
  if (!table || !row)
    return result;

  MD_String8  header_md = {DN_Cast(MD_u8 *) header.data, header.size};
  MD_MapSlot *slot      = MD_MapLookup(&table->headers_map, MD_MapKeyStr(header_md));
  if (!slot)
    return result;

  DN_USize column_index = DN_Cast(DN_USize) slot->val;
  DN_Assert(column_index < table->column_count);
  {
    DN_USize begin = DN_Cast(uintptr_t)(table->rows);
    DN_USize end   = DN_Cast(uintptr_t)(table->rows + table->row_count);
    DN_USize ptr   = DN_Cast(uintptr_t) row;
    DN_AssertF(ptr >= begin && ptr <= end, "The row to lookup does not belong to the table passed in");
  }

  result.index  = column_index;
  result.column = row->columns[column_index];
  result.header = table->headers[column_index];
  return result;
}

DN_API bool DN_CGen_LookupNextTableInCodeGenTable(DN_CGen *cgen, DN_CGenTable *cgen_table, DN_CGenLookupTableIterator *it)
{
  if (!cgen_table)
    return false;

  if (it->row_index >= cgen_table->row_count)
    return false;

  if (cgen_table->type != DN_CGenTableType_CodeGenEnum && cgen_table->type != DN_CGenTableType_CodeGenStruct && cgen_table->type != DN_CGenTableType_CodeGenBuiltinTypes)
    return false;

  // NOTE: Lookup the table in this row that we will code generate from. Not
  // applicable when we are doing builtin types as the types are just put
  // in-line into the code generation table itself.
  it->cgen_table     = cgen_table;
  it->cgen_table_row = cgen_table->rows + it->row_index++;
  if (cgen_table->type != DN_CGenTableType_CodeGenBuiltinTypes) {
    DN_CGenTableColumn cgen_table_column = it->cgen_table_row->columns[cgen_table->column_indexes[DN_CGenTableHeaderType_Table]];
    MD_String8         key               = {DN_Cast(MD_u8 *) cgen_table_column.string.data, cgen_table_column.string.size};
    MD_MapSlot        *table_slot        = MD_MapLookup(&cgen->table_map, MD_MapKeyStr(key));
    if (!table_slot) {
      MD_CodeLoc loc = MD_CodeLocFromNode(cgen_table_column.node);
      MD_PrintMessageFmt(stderr,
                         loc,
                         MD_MessageKind_Warning,
                         DN_Cast(char *) "Code generation table references non-existent table '%.*s'",
                         DN_Str8PrintFmt(cgen_table_column.string));
      return false;
    }
    it->table = DN_Cast(DN_CGenTable *) table_slot->val;
  }

  for (DN_USize type = 0; type < DN_CGenTableHeaderType_Count; type++)
    it->cgen_table_column[type] = it->cgen_table_row->columns[cgen_table->column_indexes[type]];
  return true;
}

DN_API bool DN_CGen_WillCodeGenTypeName(DN_CGen const *cgen, DN_Str8 name)
{
  if (name.size == 0)
    return false;

  for (DN_CGenTable *table = cgen->first_table; table != 0; table = table->next) {
    if (table->type != DN_CGenTableType_CodeGenStruct && table->type != DN_CGenTableType_CodeGenEnum)
      continue;

    for (DN_USize row_index = 0; row_index < table->row_count; row_index++) {
      DN_CGenTableRow const    *row    = table->rows + row_index;
      DN_CGenTableColumn const *column = row->columns + table->column_indexes[DN_CGenTableHeaderType_Name];
      if (DN_Str8Eq(column->string, name))
        return true;
    }
  }

  return false;
}

static void DN_CGen_EmitRowWhitespace_(DN_CGenTableRow const *row, DN_CppFile *cpp)
{
  for (DN_CGenTableRowTag *tag = row->first_tag; tag; tag = tag->next) {
    switch (tag->type) {
      case DN_CGenTableRowTagType_Nil: DN_InvalidCodePath;

      case DN_CGenTableRowTagType_CommentDivider: {
        if (tag->comment.size <= 0)
          break;

        DN_TCScratch tmem         = DN_TCScratchBegin(nullptr, 0);
        DN_Str8    prefix       = DN_Str8FromFmtArena(tmem.arena, "// NOTE: %.*s ", MD_S8VArg(tag->comment));
        int        line_padding = DN_Max(100 - (DN_Cast(int) prefix.size + (DN_CppSpacePerIndent(cpp) * cpp->indent)), 0);
        DN_CppPrint(cpp, "%.*s", DN_Str8PrintFmt(prefix));
        for (int index = 0; index < line_padding; index++)
          DN_CppAppend(cpp, "/");
        DN_CppAppend(cpp, "\n");
        DN_TCScratchEnd(&tmem);
      } break;

      case DN_CGenTableRowTagType_EmptyLine: {
        DN_CppAppend(cpp, "\n");
      } break;
    }
  }
}

DN_Str8 DN_CGen_ConvertTemplatesToEmittableLiterals_(DN_Arena *arena, DN_Str8 type)
{
  DN_TCScratch tmem   = DN_TCScratchBegin(&arena, 1);
  DN_Str8    result = DN_Str8TrimWhitespaceAround(type);
  result            = DN_Str8Replace(result, /*find*/ DN_Str8Lit("<"), /*replace*/ DN_Str8Lit("_"), /*start_index*/ 0, arena, DN_Str8EqCase_Sensitive);
  result            = DN_Str8Replace(result, /*find*/ DN_Str8Lit(">"), /*replace*/ DN_Str8Lit(""), /*start_index*/ 0, arena, DN_Str8EqCase_Sensitive);
  result            = DN_Str8TrimWhitespaceAround(result);
  DN_TCScratchEnd(&tmem);
  return result;
}

DN_Str8 DN_CGen_StripQualifiersOnCppType_(DN_Arena *arena, DN_Str8 type)
{
  DN_TCScratch tmem   = DN_TCScratchBegin(&arena, 1);
  DN_Str8    result = DN_Str8TrimWhitespaceAround(type);
  result            = DN_Str8Replace(result, /*find*/ DN_Str8Lit("*"), /*replace*/ DN_Str8Lit(""), /*start_index*/ 0, tmem.arena, DN_Str8EqCase_Sensitive);
  result            = DN_Str8Replace(result, /*find*/ DN_Str8Lit("constexpr"), /*replace*/ DN_Str8Lit(""), /*start_index*/ 0, tmem.arena, DN_Str8EqCase_Sensitive);
  result            = DN_Str8Replace(result, /*find*/ DN_Str8Lit("const"), /*replace*/ DN_Str8Lit(""), /*start_index*/ 0, tmem.arena, DN_Str8EqCase_Sensitive);
  result            = DN_Str8Replace(result, /*find*/ DN_Str8Lit("static"), /*replace*/ DN_Str8Lit(""), /*start_index*/ 0, tmem.arena, DN_Str8EqCase_Sensitive);
  result            = DN_Str8Replace(result, /*find*/ DN_Str8Lit(" "), /*replace*/ DN_Str8Lit(""), /*start_index*/ 0, arena, DN_Str8EqCase_Sensitive);
  result            = DN_Str8TrimWhitespaceAround(result);
  DN_TCScratchEnd(&tmem);
  return result;
}

DN_API void DN_CGen_EmitCodeForTables(DN_CGen *cgen, DN_CGenEmit emit, DN_CppFile *cpp, DN_Str8 emit_prefix)
{
  if (emit & DN_CGenEmit_Prototypes) {
    // NOTE: Generate type info enums //////////////////////////////////////////////////////////////
    DN_CppEnumBlock(cpp, "%.*sType", DN_Str8PrintFmt(emit_prefix))
    {
      DN_CppLine(cpp, "%.*sType_Nil,", DN_Str8PrintFmt(emit_prefix));
      for (DN_CGenTable *table = cgen->first_table; table != 0; table = table->next)
        for (DN_CGenLookupTableIterator it = {}; DN_CGen_LookupNextTableInCodeGenTable(cgen, table, &it);) {
          DN_TCScratch tmem      = DN_TCScratchBegin(nullptr, 0);
          DN_Str8    enum_name = DN_CGen_ConvertTemplatesToEmittableLiterals_(tmem.arena, it.cgen_table_column[DN_CGenTableHeaderType_Name].string);
          DN_CppLine(cpp, "%.*sType_%.*s,", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(enum_name));
          DN_TCScratchEnd(&tmem);
        }
      DN_CppLine(cpp, "%.*sType_Count,", DN_Str8PrintFmt(emit_prefix));
    }
    DN_CppNewLine(cpp);

    // NOTE: Generate structs + enums //////////////////////////////////////////////////////////////
    for (DN_CGenTable *table = cgen->first_table; table != 0; table = table->next) {
      switch (table->type) {
        case DN_CGenTableType_Nil: DN_InvalidCodePath;
        case DN_CGenTableType_Count: DN_InvalidCodePath;
        case DN_CGenTableType_CodeGenBuiltinTypes: continue;
        case DN_CGenTableType_Data: continue;

        case DN_CGenTableType_CodeGenStruct: {
          for (DN_CGenLookupTableIterator it = {}; DN_CGen_LookupNextTableInCodeGenTable(cgen, table, &it);) {
            // TODO(doyle): Verify the codegen table has the headers from the table it references
        int longest_type_name = 0;
            for (DN_USize row_index = 0; row_index < it.table->row_count; row_index++) {
              DN_TCScratch                  tmem     = DN_TCScratchBegin(nullptr, 0);
              DN_CGenTableRow const      *row      = it.table->rows + row_index;
              DN_CGenLookupColumnAtHeader cpp_type = DN_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[DN_CGenTableHeaderType_CppType].string, row);

              DN_USize length    = cpp_type.column.string.size;
              DN_Str8  find_name = DN_CGen_StripQualifiersOnCppType_(tmem.arena, cpp_type.column.string);
              if (DN_CGen_WillCodeGenTypeName(cgen, find_name))
                length += emit_prefix.size;

              longest_type_name = DN_Max(longest_type_name, DN_Cast(int) length);
              DN_TCScratchEnd(&tmem);
            }

            DN_CppStructBlock(cpp, "%.*s%.*s", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(it.cgen_table_column[DN_CGenTableHeaderType_Name].string))
            {
              for (DN_USize row_index = 0; row_index < it.table->row_count; row_index++) {
                DN_CGenTableRow const      *row            = it.table->rows + row_index;
                DN_CGenLookupColumnAtHeader cpp_name       = DN_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[DN_CGenTableHeaderType_CppName].string, row);
                DN_CGenLookupColumnAtHeader cpp_type       = DN_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[DN_CGenTableHeaderType_CppType].string, row);
                DN_CGenLookupColumnAtHeader cpp_array_size = DN_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[DN_CGenTableHeaderType_CppArraySize].string, row);
                if (cpp_name.column.string.size <= 0 || cpp_type.column.string.size <= 0)
                  continue;

            // NOTE: Generate cpp array size
                DN_TCScratch tmem       = DN_TCScratchBegin(nullptr, 0);
                DN_Str8    array_size = {};
                if (cpp_array_size.column.string.size)
                  array_size = DN_Str8FromFmtArena(tmem.arena, "[%.*s]", DN_Str8PrintFmt(cpp_array_size.column.string));

                // NOTE: Check if we're referencing a code generated type. If we
                // are, append the `emit_prefix`
                DN_Str8 emit_cpp_type = cpp_type.column.string;
                {
                  DN_Str8 find_name = DN_CGen_StripQualifiersOnCppType_(tmem.arena, emit_cpp_type);
                  if (DN_CGen_WillCodeGenTypeName(cgen, find_name))
                    emit_cpp_type = DN_Str8FromFmtArena(tmem.arena, "%.*s%.*s", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(cpp_type.column.string));
                }
                DN_TCScratchEnd(&tmem);

                int name_to_type_padding = 1 + longest_type_name - DN_Cast(int) emit_cpp_type.size;

                // NOTE: Emit decl /////////////////////////////////////////////////
                DN_CGen_EmitRowWhitespace_(row, cpp);
                DN_CppLine(cpp,
                           "%.*s%*s%.*s%.*s;",
                           DN_Str8PrintFmt(emit_cpp_type),
                           name_to_type_padding,
                           "",
                           DN_Str8PrintFmt(cpp_name.column.string),
                           DN_Str8PrintFmt(array_size));
              }
            }
            DN_CppNewLine(cpp);
            DN_CppNewLine(cpp);
          }

        } break;

        case DN_CGenTableType_CodeGenEnum: {
          for (DN_CGenLookupTableIterator it = {}; DN_CGen_LookupNextTableInCodeGenTable(cgen, table, &it);) {
            DN_CppEnumBlock(cpp, "%.*s%.*s", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(it.cgen_table_column[DN_CGenTableHeaderType_Name].string))
            {
              DN_USize enum_count = 0;
              for (DN_USize row_index = 0; row_index < it.table->row_count; row_index++) {
                DN_CGenTableRow const      *row       = it.table->rows + row_index;
                DN_CGenLookupColumnAtHeader cpp_name  = DN_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[DN_CGenTableHeaderType_CppName].string, row);
                DN_CGenLookupColumnAtHeader cpp_value = DN_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[DN_CGenTableHeaderType_CppValue].string, row);

                if (cpp_name.column.string.size <= 0)
                  continue;

                DN_CGen_EmitRowWhitespace_(row, cpp);
                if (cpp_value.column.string.size)
                  DN_CppLine(cpp,
                             "%.*s%.*s_%.*s = %.*s,",
                             DN_Str8PrintFmt(emit_prefix),
                             DN_Str8PrintFmt(it.cgen_table_column[DN_CGenTableHeaderType_Name].string),
                             DN_Str8PrintFmt(cpp_name.column.string),
                             DN_Str8PrintFmt(cpp_value.column.string));
                else
                  DN_CppLine(cpp,
                             "%.*s%.*s_%.*s = %zu,",
                             DN_Str8PrintFmt(emit_prefix),
                             DN_Str8PrintFmt(it.cgen_table_column[DN_CGenTableHeaderType_Name].string),
                             DN_Str8PrintFmt(cpp_name.column.string),
                             row_index);
                enum_count++;
              }

              DN_CGenTableColumn gen_enum_count_column = it.cgen_table_column[DN_CGenTableHeaderType_GenEnumCount];
              if (gen_enum_count_column.string.size)
                DN_CppLine(cpp,
                           "%.*s%.*s_%.*s = %zu,",
                           DN_Str8PrintFmt(emit_prefix),
                           DN_Str8PrintFmt(it.cgen_table_column[DN_CGenTableHeaderType_Name].string),
                           DN_Str8PrintFmt(gen_enum_count_column.string),
                           enum_count);
            }
            DN_CppNewLine(cpp);
          }
        } break;
      }
    }

    // NOTE: Generate enums for struct fields //////////////////////////////////////////////////
    for (DN_CGenTable *table = cgen->first_table; table != 0; table = table->next) {
      switch (table->type) {
        case DN_CGenTableType_Nil: DN_InvalidCodePath;
        case DN_CGenTableType_Count: DN_InvalidCodePath;
        case DN_CGenTableType_Data: continue;
        case DN_CGenTableType_CodeGenBuiltinTypes: continue;
        case DN_CGenTableType_CodeGenEnum: continue;

        case DN_CGenTableType_CodeGenStruct: {
          for (DN_CGenLookupTableIterator it = {}; DN_CGen_LookupNextTableInCodeGenTable(cgen, table, &it);) {
            DN_Str8 struct_name = it.cgen_table_column[DN_CGenTableHeaderType_Name].string;
            DN_CppEnumBlock(cpp, "%.*s%.*sTypeField", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(struct_name))
            {
              for (DN_USize row_index = 0; row_index < it.table->row_count; row_index++) {
                DN_CGenTableRow const      *row      = it.table->rows + row_index;
                DN_CGenLookupColumnAtHeader cpp_name = DN_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[DN_CGenTableHeaderType_CppName].string, row);
                if (cpp_name.column.string.size <= 0)
                  continue;
                DN_CGen_EmitRowWhitespace_(row, cpp);
                DN_CppLine(cpp, "%.*s%.*sTypeField_%.*s,", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(struct_name), DN_Str8PrintFmt(cpp_name.column.string));
              }
              DN_CppLine(cpp, "%.*s%.*sTypeField_Count,", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(struct_name));
            }
            DN_CppNewLine(cpp);
          }
        } break;
      }
    }

    // NOTE: Str8 to enum conversion ///////////////////////////////////////////////////////////////
    for (DN_CGenTable *table = cgen->first_table; table != 0; table = table->next) {
      if (table->type != DN_CGenTableType_CodeGenEnum)
        continue;

      for (DN_CGenLookupTableIterator it = {}; DN_CGen_LookupNextTableInCodeGenTable(cgen, table, &it);) {
        DN_Str8 type_name = it.cgen_table_column[DN_CGenTableHeaderType_Name].string;
        DN_CppStructBlock(cpp, "%.*s%.*sStr8ToEnumResult", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(type_name))
        {
          DN_CppLine(cpp, "bool success;");
          DN_CppLine(cpp, "%.*s%.*s value;", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(type_name));
        }
        DN_CppNewLine(cpp);
      }
    }

    // NOTE: Type to DN_TypeField function
    DN_CppLine(cpp,
               "DN_TypeInfo const *%.*sType_Info(%.*sType type);",
               DN_Str8PrintFmt(emit_prefix),
               DN_Str8PrintFmt(emit_prefix));

    // NOTE: Str8 <-> Enum conversion functions
    for (DN_CGenTable *table = cgen->first_table; table != 0; table = table->next) {
      if (table->type != DN_CGenTableType_CodeGenEnum)
        continue;

      for (DN_CGenLookupTableIterator it = {}; DN_CGen_LookupNextTableInCodeGenTable(cgen, table, &it);) {
        DN_Str8 type_name = it.cgen_table_column[DN_CGenTableHeaderType_Name].string;
        DN_CppLine(cpp,
                   "%.*s%.*sStr8ToEnumResult %.*s%.*s_NameStr8ToEnum(DN_Str8 string);",
                   DN_Str8PrintFmt(emit_prefix),
                   DN_Str8PrintFmt(type_name),
                   DN_Str8PrintFmt(emit_prefix),
                   DN_Str8PrintFmt(type_name));
        DN_CppLine(cpp,
                   "%.*s%.*sStr8ToEnumResult %.*s%.*s_LabelStr8ToEnum(DN_Str8 string);",
                   DN_Str8PrintFmt(emit_prefix),
                   DN_Str8PrintFmt(type_name),
                   DN_Str8PrintFmt(emit_prefix),
                   DN_Str8PrintFmt(type_name));
      }
    }


    // NOTE: Operator == and != ////////////////////////////////////////////////////////////////
    for (DN_CGenTable *table = cgen->first_table; table != 0; table = table->next) {
      if (table->type != DN_CGenTableType_CodeGenStruct)
        continue;

      for (DN_CGenLookupTableIterator it = {}; DN_CGen_LookupNextTableInCodeGenTable(cgen, table, &it);) {
        DN_Str8 cpp_op_equals = it.cgen_table_column[DN_CGenTableHeaderType_CppOpEquals].string;
        if (!DN_Str8Eq(cpp_op_equals, DN_Str8Lit("true")))
          continue;

        DN_Str8 type_name = it.cgen_table_column[DN_CGenTableHeaderType_Name].string;
        DN_CppLine(cpp,
                   "bool operator==(%.*s%.*s const &lhs, %.*s%.*s const &rhs);",
                   DN_Str8PrintFmt(emit_prefix),
                   DN_Str8PrintFmt(type_name),
                   DN_Str8PrintFmt(emit_prefix),
                   DN_Str8PrintFmt(type_name));
        DN_CppLine(cpp,
                   "bool operator!=(%.*s%.*s const &lhs, %.*s%.*s const &rhs);",
                   DN_Str8PrintFmt(emit_prefix),
                   DN_Str8PrintFmt(type_name),
                   DN_Str8PrintFmt(emit_prefix),
                   DN_Str8PrintFmt(type_name));
      }
    }
  }

  if (emit & DN_CGenEmit_Implementation) {
    // NOTE: Generate type info ////////////////////////////////////////////////////////////////////
    for (DN_CGenTable *table = cgen->first_table; table != 0; table = table->next) {
      for (DN_CGenLookupTableIterator it = {}; DN_CGen_LookupNextTableInCodeGenTable(cgen, table, &it);) {
        if (table->type == DN_CGenTableType_CodeGenBuiltinTypes)
          continue;

        DN_Str8 struct_or_enum_name = it.cgen_table_column[DN_CGenTableHeaderType_Name].string;
        DN_CppBlock(cpp, ";\n\n", "DN_TypeField const g_%.*s%.*s_type_fields[] =", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(struct_or_enum_name))
        {
          if (table->type == DN_CGenTableType_CodeGenStruct) {
            // NOTE: Construct the cpp type string first. We will prepend `emit_prefix`
            // for types that are declared in the same mdesk file. We will also
            // calculate the longest type name that we will generate for whitespace
            // padding purposes.
            DN_TCScratch tmem                  = DN_TCScratchBegin(nullptr, 0);
            DN_USize   longest_cpp_type_name = 0;
            auto       cpp_type_list         = DN_SArray_Init<DN_Str8>(tmem.arena, it.table->row_count, DN_ZMem_Yes);

            for (DN_USize row_index = 0; row_index < it.table->row_count; row_index++) {
              DN_CGenTableRow const      *row           = it.table->rows + row_index;
              DN_CGenLookupColumnAtHeader cpp_type      = DN_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[DN_CGenTableHeaderType_CppType].string, row);

              // NOTE: CHeck the length of the string after turning it into emittable code
              DN_Str8 cpp_type_name = DN_CGen_StripQualifiersOnCppType_(tmem.arena, cpp_type.column.string);
              if (DN_CGen_WillCodeGenTypeName(cgen, cpp_type_name))
                cpp_type_name = DN_Str8FromFmtArena(tmem.arena, "%.*s%.*s", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(cpp_type_name));

              DN_Str8 cpp_type_name_no_templates = DN_CGen_ConvertTemplatesToEmittableLiterals_(tmem.arena, cpp_type_name);
              longest_cpp_type_name              = DN_Max(longest_cpp_type_name, cpp_type_name_no_templates.size);

              DN_SArray_Add(&cpp_type_list, cpp_type_name);
            }

            // NOTE: Iterate each row and emit the C++ declarations ////////////////////
            for (DN_USize row_index = 0; row_index < it.table->row_count; row_index++) {
              DN_CGenTableRow const      *row                  = it.table->rows + row_index;
              DN_CGenLookupColumnAtHeader cpp_name             = DN_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[DN_CGenTableHeaderType_CppName].string, row);
              DN_CGenLookupColumnAtHeader cpp_type             = DN_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[DN_CGenTableHeaderType_CppType].string, row);
              DN_CGenLookupColumnAtHeader cpp_is_ptr           = DN_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[DN_CGenTableHeaderType_CppIsPtr].string, row);
              DN_CGenLookupColumnAtHeader cpp_array_size       = DN_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[DN_CGenTableHeaderType_CppArraySize].string, row);
              DN_CGenLookupColumnAtHeader cpp_array_size_field = DN_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[DN_CGenTableHeaderType_CppArraySizeField].string, row);
              DN_CGenLookupColumnAtHeader cpp_label            = DN_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[DN_CGenTableHeaderType_CppLabel].string, row);

              bool    cpp_is_ptr_b32      = DN_Str8Eq(cpp_is_ptr.column.string, DN_Str8Lit("true"));
              DN_Str8 cpp_array_size_str8 = cpp_array_size.column.string.size ? cpp_array_size.column.string : DN_Str8Lit("0");

              DN_CGenTableColumn struct_name               = it.cgen_table_row->columns[table->column_indexes[DN_CGenTableHeaderType_Name]];
              DN_Str8            cpp_array_size_field_str8 = DN_Str8Lit("NULL");
              if (cpp_array_size_field.column.string.size) {
                // TODO(doyle): Check that array_size_field references a valid field in the table
                // NOTE: We use a raw index for the reference because the struct might
                // not have type info being generated so we can't rely on the enum.
                DN_USize index_the_field_references = 0;
                for (DN_USize sub_row_index = 0; sub_row_index < it.table->row_count; sub_row_index++) {
                  DN_CGenTableRow const *sub_row      = it.table->rows + sub_row_index;
                  DN_CGenTableColumn     sub_cpp_name = sub_row->columns[cpp_name.index];
                  if (DN_Str8Eq(sub_cpp_name.string, cpp_array_size_field.column.string))
                    index_the_field_references = sub_row_index;
                }
                cpp_array_size_field_str8 =
                    DN_Str8FromFmtArena(tmem.arena, "&g_%.*s%.*s_type_fields[%zu]",
                                      DN_Str8PrintFmt(emit_prefix),
                                      DN_Str8PrintFmt(struct_name.string),
                                      index_the_field_references);
              }

              DN_Str8 cpp_type_name              = cpp_type_list.data[row_index];
              DN_Str8 orig_cpp_type              = DN_CGen_StripQualifiersOnCppType_(tmem.arena, cpp_type.column.string);
              DN_Str8 orig_cpp_type_no_templates = DN_CGen_ConvertTemplatesToEmittableLiterals_(tmem.arena, orig_cpp_type);

              DN_USize cpp_name_padding = 1 + it.table->headers[cpp_name.index].longest_string - cpp_name.column.string.size;
              DN_USize cpp_type_padding = 1 + longest_cpp_type_name - cpp_type_name.size;

              DN_Str8  cpp_type_enum         = DN_Str8FromFmtArena(tmem.arena, "%.*sType_%.*s", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(orig_cpp_type_no_templates));
              DN_USize cpp_type_enum_padding = cpp_type_padding + (orig_cpp_type.size - cpp_type_name.size);

              DN_Str8  cpp_label_str8         = cpp_name.column.string;
              DN_USize cpp_label_str8_padding = cpp_name_padding;
              if (cpp_label.column.string.size) {
                cpp_label_str8         = cpp_label.column.string;
                cpp_label_str8_padding = 1 + it.table->headers[cpp_label.index].longest_string - cpp_label.column.string.size;
              }

              DN_Str8Builder builder = DN_Str8BuilderFromArena(tmem.arena);

              // NOTE: row
              DN_Str8BuilderAppendF(&builder, "{%2d, ", row_index);

              // NOTE: name
              DN_Str8BuilderAppendF(&builder,
                                     "DN_Str8Lit(\"%.*s\"),%*s",
                                     DN_Str8PrintFmt(cpp_name.column.string),
                                     cpp_name_padding,
                                     "");

              // NOTE: label
              DN_Str8BuilderAppendF(&builder,
                                     "DN_Str8Lit(\"%.*s\"),%*s",
                                     DN_Str8PrintFmt(cpp_label_str8),
                                     cpp_label_str8_padding,
                                     "");

              // NOTE: value
              DN_Str8BuilderAppendF(&builder,
                                     "/*value*/ 0, ");

              // NOTE: offsetof(a, b)
              DN_Str8BuilderAppendF(&builder,
                                     "offsetof(%.*s%.*s, %.*s),%*s",
                                     DN_Str8PrintFmt(emit_prefix),
                                     DN_Str8PrintFmt(struct_or_enum_name),
                                     DN_Str8PrintFmt(cpp_name.column.string),
                                     cpp_name_padding,
                                     "");

              // NOTE: sizeof(a->b)
              DN_Str8BuilderAppendF(&builder,
                                     "sizeof(((%.*s%.*s*)0)->%.*s),%*s",
                                     DN_Str8PrintFmt(emit_prefix),
                                     DN_Str8PrintFmt(struct_or_enum_name),
                                     DN_Str8PrintFmt(cpp_name.column.string),
                                     cpp_name_padding,
                                     "");

              // NOTE: alignof(a)
              if (DN_Str8Eq(cpp_type_name, DN_Str8Lit("void"))) {
                DN_Str8  proxy_type         = DN_Str8Lit("char");
                DN_USize proxy_type_padding = 1 + longest_cpp_type_name - proxy_type.size;
                DN_Str8BuilderAppendF(&builder, "alignof(%.*s),%*s", DN_Str8PrintFmt(proxy_type), proxy_type_padding, "");
              } else {
                DN_Str8BuilderAppendF(&builder,
                                       "alignof(%.*s),%*s",
                                       DN_Str8PrintFmt(cpp_type_name),
                                       cpp_type_padding,
                                       "");
              }

              // NOTE: Type string
              DN_Str8BuilderAppendF(&builder,
                                     "DN_Str8Lit(\"%.*s\"),%*s",
                                     DN_Str8PrintFmt(cpp_type_name),
                                     cpp_type_padding,
                                     "");

              // NOTE: Type as enum
              DN_Str8BuilderAppendF(&builder,
                                     "%.*s,%*s",
                                     DN_Str8PrintFmt(cpp_type_enum),
                                     cpp_type_enum_padding,
                                     "");

              DN_Str8BuilderAppendF(&builder,
                                     "/*is_pointer*/ %s,%s /*array_size*/ %.*s, /*array_size_field*/ %.*s},",
                                     cpp_is_ptr_b32 ? "true" : "false",
                                     cpp_is_ptr_b32 ? " " : "",
                                     DN_Str8PrintFmt(cpp_array_size_str8),
                                     DN_Str8PrintFmt(cpp_array_size_field_str8));

              DN_Str8 line = DN_Str8BuilderBuild(&builder, tmem.arena);
              DN_CppLine(cpp, "%.*s", DN_Str8PrintFmt(line));
            }
          } else {
            DN_Assert(table->type == DN_CGenTableType_CodeGenEnum);
            for (DN_USize row_index = 0; row_index < it.table->row_count; row_index++) {
              DN_CGenTableRow const      *row       = it.table->rows + row_index;
              DN_CGenLookupColumnAtHeader cpp_name  = DN_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[DN_CGenTableHeaderType_CppName].string, row);
              DN_CGenLookupColumnAtHeader cpp_value = DN_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[DN_CGenTableHeaderType_CppValue].string, row);
              DN_CGenLookupColumnAtHeader cpp_label = DN_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[DN_CGenTableHeaderType_CppLabel].string, row);
              if (cpp_name.column.string.size <= 0)
                continue;

              DN_TCScratch tmem             = DN_TCScratchBegin(nullptr, 0);
              DN_USize   cpp_name_padding = 1 + it.table->headers[cpp_name.index].longest_string - cpp_name.column.string.size;
              DN_Str8    cpp_value_str8   = cpp_value.column.string.size ? cpp_value.column.string : DN_Str8FromFmtArena(tmem.arena, "%zu", row_index);
              DN_Str8    cpp_type_enum    = DN_Str8FromFmtArena(tmem.arena, "%.*sType_%.*s", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(struct_or_enum_name));

              DN_Str8  cpp_label_str8         = cpp_name.column.string;
              DN_USize cpp_label_str8_padding = cpp_name_padding;
              if (cpp_label.column.string.size) {
                cpp_label_str8         = cpp_label.column.string;
                cpp_label_str8_padding = 1 + it.table->headers[cpp_label.index].longest_string - cpp_label.column.string.size;
              }

              DN_Str8Builder builder = {};
              builder.arena = tmem.arena;
              // NOTE: row
              DN_Str8BuilderAppendF(&builder, "{%2d, ", row_index);

              // NOTE: name
              DN_Str8BuilderAppendF(&builder,
                                     "DN_Str8Lit(\"%.*s\"),%*s",
                                     DN_Str8PrintFmt(cpp_name.column.string),
                                     cpp_name_padding,
                                     "");

              // NOTE: label
              DN_Str8BuilderAppendF(&builder,
                                     "DN_Str8Lit(\"%.*s\"),%*s",
                                     DN_Str8PrintFmt(cpp_label_str8),
                                     cpp_label_str8_padding,
                                     "");

              // NOTE: value
              DN_Str8BuilderAppendF(&builder, "/*value*/ %.*s, ", DN_Str8PrintFmt(cpp_value_str8));

              // NOTE: offsetof(a, b)
              DN_Str8BuilderAppendF(&builder, "/*offsetof*/ 0, ");

              // NOTE: sizeof(a->b)
              DN_Str8BuilderAppendF(&builder,
                                     "sizeof(%.*s%.*s), ",
                                     DN_Str8PrintFmt(emit_prefix),
                                     DN_Str8PrintFmt(struct_or_enum_name));

              // NOTE: alignof(a->b)
              DN_Str8BuilderAppendF(&builder, "alignof(%.*s%.*s), ", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(struct_or_enum_name));

              // TODO: Type string
              DN_Str8BuilderAppendF(&builder, "DN_Str8Lit(\"\"), ");

              // NOTE: Type as enum
              DN_Str8BuilderAppendF(&builder, "%.*s, ", DN_Str8PrintFmt(cpp_type_enum));

              DN_Str8BuilderAppendF(&builder, "/*is_pointer*/ false, ");
              DN_Str8BuilderAppendF(&builder, "/*array_size*/ 0, ");
              DN_Str8BuilderAppendF(&builder, "/*array_size_field*/ NULL},");

              DN_Str8 line = DN_Str8BuilderBuild(&builder, tmem.arena);
              DN_TCScratchEnd(&tmem);
              DN_CppLine(cpp, "%.*s", DN_Str8PrintFmt(line));
            }
          }
          DN_TCScratchEnd(&tmem);
        }
      }
    }

        int longest_name_across_all_tables = 0;
    for (DN_CGenTable *table = cgen->first_table; table != 0; table = table->next) {
      for (DN_CGenLookupTableIterator it = {}; DN_CGen_LookupNextTableInCodeGenTable(cgen, table, &it);) {
        DN_TCScratch tmem      = DN_TCScratchBegin(nullptr, 0);
        DN_Str8    type_name = it.cgen_table_column[DN_CGenTableHeaderType_Name].string;
        if (DN_CGen_WillCodeGenTypeName(cgen, type_name))
          type_name = DN_Str8FromFmtArena(tmem.arena, "%.*s%.*s", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(type_name));

        longest_name_across_all_tables = DN_Max(longest_name_across_all_tables, DN_Cast(int) type_name.size);
        DN_TCScratchEnd(&tmem);
      }
    }

    DN_CppBlock(cpp, ";\n\n", "DN_TypeInfo const g_%.*stypes[] =", DN_Str8PrintFmt(emit_prefix))
    {
      DN_CppLine(cpp, "{DN_Str8Lit(\"\"),%*sDN_TypeKind_Nil,    0, /*fields*/ NULL, /*count*/ 0},", 1 + longest_name_across_all_tables, "");

      DN_USize longest_type_name = 0;
      for (DN_CGenTable *table = cgen->first_table; table != 0; table = table->next) {
        for (DN_CGenLookupTableIterator it = {}; DN_CGen_LookupNextTableInCodeGenTable(cgen, table, &it);) {
          DN_TCScratch tmem      = DN_TCScratchBegin(nullptr, 0);
          DN_Str8    type_name = it.cgen_table_column[DN_CGenTableHeaderType_Name].string;
          if (DN_CGen_WillCodeGenTypeName(cgen, type_name))
            type_name = DN_Str8FromFmtArena(tmem.arena, "%.*s%.*s", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(type_name));
          longest_type_name = DN_Max(longest_type_name, type_name.size);
          DN_TCScratchEnd(&tmem);
        }
      }

      for (DN_CGenTable *table = cgen->first_table; table != 0; table = table->next) {
        for (DN_CGenLookupTableIterator it = {}; DN_CGen_LookupNextTableInCodeGenTable(cgen, table, &it);) {
          DN_TCScratch tmem      = DN_TCScratchBegin(nullptr, 0);
          DN_Str8    type_name = it.cgen_table_column[DN_CGenTableHeaderType_Name].string;
          if (DN_CGen_WillCodeGenTypeName(cgen, type_name))
            type_name = DN_Str8FromFmtArena(tmem.arena, "%.*s%.*s", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(type_name));

          int         name_padding           = 1 + longest_name_across_all_tables - DN_Cast(int) type_name.size;
          DN_Str8     type_info_kind         = {};
          char const *type_info_kind_padding = "";
          if (table->type == DN_CGenTableType_CodeGenEnum) {
            type_info_kind         = DN_Str8Lit("DN_TypeKind_Enum");
            type_info_kind_padding = "  ";
          } else if (table->type == DN_CGenTableType_CodeGenStruct) {
            type_info_kind = DN_Str8Lit("DN_TypeKind_Struct");
          } else {
            DN_Assert(table->type == DN_CGenTableType_CodeGenBuiltinTypes);
            type_info_kind         = DN_Str8Lit("DN_TypeKind_Basic");
            type_info_kind_padding = " ";
          }

          DN_Str8 fields         = DN_Str8Lit("NULL");
          int     fields_padding = 1;
          if (table->type != DN_CGenTableType_CodeGenBuiltinTypes) {
            fields_padding = name_padding;
            fields         = DN_Str8FromFmtArena(tmem.arena, "g_%.*s_type_fields", DN_Str8PrintFmt(type_name));
          }

          DN_Str8 fields_count = DN_Str8Lit("0");
          if (!DN_Str8Eq(fields, DN_Str8Lit("NULL")))
            fields_count = DN_Str8FromFmtArena(tmem.arena, "sizeof(%.*s)/sizeof(%.*s[0])", DN_Str8PrintFmt(fields), DN_Str8PrintFmt(fields));

          DN_Str8Builder builder = {};
          builder.arena = tmem.arena;
          // NOTE: name
          DN_Str8BuilderAppendF(&builder, "{DN_Str8Lit(\"%.*s\"),%*s", DN_Str8PrintFmt(type_name), name_padding, "");

          // NOTE: DN_TypeKind_{Nil|Basic|Enum|Struct}
          DN_Str8BuilderAppendF(&builder, "%.*s,%s", DN_Str8PrintFmt(type_info_kind), type_info_kind_padding);

          // NOTE: sizeof(T)
          if (DN_Str8Eq(type_name, DN_Str8Lit("void")))
            DN_Str8BuilderAppendF(&builder, "0,%*s", name_padding, "");
          else
            DN_Str8BuilderAppendF(&builder, "sizeof(%.*s),%*s", DN_Str8PrintFmt(type_name), name_padding, "");

          // NOTE: Pointer to DN_TypeField[]
          DN_Str8BuilderAppendF(&builder, "/*fields*/ %.*s,%*s", DN_Str8PrintFmt(fields), fields_padding, "");

          // NOTE: DN_TypeField length
          DN_Str8BuilderAppendF(&builder, "/*count*/ %.*s},", DN_Str8PrintFmt(fields_count));

          DN_Str8 line = DN_Str8BuilderBuild(&builder, tmem.arena);
          DN_TCScratchEnd(&tmem);
          DN_CppLine(cpp, "%.*s", DN_Str8PrintFmt(line));
        }
      }
    }

    // NOTE: Type to DN_TypeField function
    DN_CppFuncBlock(cpp, "DN_TypeInfo const *%.*sType_Info(%.*sType type)", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(emit_prefix))
    {
      DN_CppLine(cpp, "DN_TypeInfo const *result = g_%.*stypes + 0;", DN_Str8PrintFmt(emit_prefix));
      DN_CppSwitchBlock(cpp, "type")
      {
        DN_CppLine(cpp, "case %.*sType_Nil: break;", DN_Str8PrintFmt(emit_prefix));
        DN_CppLine(cpp, "case %.*sType_Count: break;", DN_Str8PrintFmt(emit_prefix));
        DN_TCScratch tmem = DN_TCScratchBegin(nullptr, 0);
        for (DN_CGenTable *table = cgen->first_table; table != 0; table = table->next) {
          for (DN_CGenLookupTableIterator it = {}; DN_CGen_LookupNextTableInCodeGenTable(cgen, table, &it);) {
            DN_Str8 enum_name          = DN_CGen_ConvertTemplatesToEmittableLiterals_(tmem.arena, it.cgen_table_column[DN_CGenTableHeaderType_Name].string);
            DN_Str8 full_enum_name     = DN_Str8FromFmtArena(tmem.arena, "%.*sType_%.*s", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(enum_name));

            // NOTE: Builtin types don't have type info
            DN_Str8 full_variable_name = DN_Str8FromFmtArena(tmem.arena, "g_%.*s%.*s_type_fields", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(enum_name));
            if (it.cgen_table->type == DN_CGenTableType_CodeGenBuiltinTypes)
              full_variable_name = DN_Str8Lit("nullptr");

            DN_CppLine(cpp, "case %.*s: result = g_%.*stypes + %.*s; break;", DN_Str8PrintFmt(full_enum_name), DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(full_enum_name));
          }
        }
        DN_TCScratchEnd(&tmem);
      }
      DN_CppLine(cpp, "return result;");
    }

    // NOTE: Str8 to enum conversion ////////////////////////////////////////////////////////////
    for (DN_CGenTable *table = cgen->first_table; table != 0; table = table->next) {
      if (table->type != DN_CGenTableType_CodeGenEnum)
        continue;

      for (DN_CGenLookupTableIterator it = {}; DN_CGen_LookupNextTableInCodeGenTable(cgen, table, &it);) {
        DN_Str8 type_name = it.cgen_table_column[DN_CGenTableHeaderType_Name].string;

        DN_CppFuncBlock(cpp, "%.*s%.*sStr8ToEnumResult %.*s%.*s_NameStr8ToEnum(DN_Str8 string)", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(type_name), DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(type_name))
        {
          DN_CppLine(cpp, "%.*s%.*sStr8ToEnumResult result = {};", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(type_name));
          DN_CppForBlock(cpp, "DN_USize index = 0; !result.success && index < DN_ArrayCountU(g_%.*s%.*s_type_fields); index++", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(type_name))
          {
            DN_CppIfChain(cpp)
            {
              DN_CppLine(cpp, "DN_TypeField field = g_%.*s%.*s_type_fields[index];", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(type_name));
              DN_CppIfOrElseIfBlock(cpp, "DN_Str8EqInsensitive(string, field.name)")
              {
                DN_CppLine(cpp, "result.success = true;");
                DN_CppLine(cpp, "result.value = (%.*s%.*s)index;", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(type_name));
              }
            }
          }
          DN_CppLine(cpp, "return result;");
        }
        DN_CppNewLine(cpp);

        DN_CppFuncBlock(cpp, "%.*s%.*sStr8ToEnumResult %.*s%.*s_LabelStr8ToEnum(DN_Str8 string)", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(type_name), DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(type_name))
        {
          DN_CppLine(cpp, "%.*s%.*sStr8ToEnumResult result = {};", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(type_name));
          DN_CppForBlock(cpp, "DN_USize index = 0; !result.success && index < DN_ArrayCountU(g_%.*s%.*s_type_fields); index++", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(type_name))
          {
            DN_CppIfChain(cpp)
            {
              DN_CppLine(cpp, "DN_TypeField field = g_%.*s%.*s_type_fields[index];", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(type_name));
              DN_CppIfOrElseIfBlock(cpp, "DN_Str8EqInsensitive(string, field.label)")
              {
                DN_CppLine(cpp, "result.success = true;");
                DN_CppLine(cpp, "result.value = (%.*s%.*s)index;", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(type_name));
              }
            }
          }
          DN_CppLine(cpp, "return result;");
        }
        DN_CppNewLine(cpp);
      }
    }

    // NOTE: Operator == and != ////////////////////////////////////////////////////////////////
    for (DN_CGenTable *table = cgen->first_table; table != 0; table = table->next) {
      if (table->type != DN_CGenTableType_CodeGenStruct)
        continue;

      for (DN_CGenLookupTableIterator it = {}; DN_CGen_LookupNextTableInCodeGenTable(cgen, table, &it);) {
        DN_Str8 cpp_op_equals = it.cgen_table_column[DN_CGenTableHeaderType_CppOpEquals].string;
        if (!DN_Str8Eq(cpp_op_equals, DN_Str8Lit("true")))
          continue;

        DN_Str8 type_name = it.cgen_table_column[DN_CGenTableHeaderType_Name].string;
        DN_CppFuncBlock(cpp, "bool operator==(%.*s%.*s const &lhs, %.*s%.*s const &rhs)", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(type_name), DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(type_name))
        {
          for (DN_USize row_index = 0; row_index < it.table->row_count; row_index++) {
            DN_CGenTableRow const      *row                  = it.table->rows + row_index;
            DN_CGenLookupColumnAtHeader cpp_name             = DN_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[DN_CGenTableHeaderType_CppName].string, row);
            DN_CGenLookupColumnAtHeader cpp_is_ptr           = DN_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[DN_CGenTableHeaderType_CppIsPtr].string, row);
            DN_CGenLookupColumnAtHeader cpp_array_size       = DN_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[DN_CGenTableHeaderType_CppArraySize].string, row);
            DN_CGenLookupColumnAtHeader cpp_array_size_field = DN_CGen_LookupColumnAtHeader(it.table, it.cgen_table_column[DN_CGenTableHeaderType_CppArraySizeField].string, row);

            // TODO(doyle): Check if we're an integral type or not to double check if we
            // can use memcmp or operator==
            if (cpp_array_size_field.column.string.size) {
              DN_CppIfChain(cpp)
              {
                DN_CppIfOrElseIfBlock(cpp,
                                      "lhs.%.*s != rhs.%.*s",
                                      DN_Str8PrintFmt(cpp_array_size_field.column.string),
                                      DN_Str8PrintFmt(cpp_array_size_field.column.string))
                {
                  DN_CppLine(cpp, "return false;");
                }
              }
              DN_CppIfChain(cpp)
              {
                DN_CppIfOrElseIfBlock(cpp,
                                      "DN_Memcmp(lhs.%.*s, rhs.%.*s, lhs.%.*s) != 0",
                                      DN_Str8PrintFmt(cpp_name.column.string),
                                      DN_Str8PrintFmt(cpp_name.column.string),
                                      DN_Str8PrintFmt(cpp_array_size_field.column.string))
                {
                  DN_CppLine(cpp, "return false;");
                }
              }
            } else if (cpp_array_size.column.string.size) {
              DN_CppIfChain(cpp)
              {
                DN_CppIfOrElseIfBlock(cpp,
                                      "DN_Memcmp(lhs.%.*s, rhs.%.*s, %.*s) != 0",
                                      DN_Str8PrintFmt(cpp_name.column.string),
                                      DN_Str8PrintFmt(cpp_name.column.string),
                                      DN_Str8PrintFmt(cpp_array_size.column.string))
                {
                  DN_CppLine(cpp, "return false;");
                }
              }
            } else if (DN_Str8Eq(cpp_is_ptr.column.string, DN_Str8Lit("true"))) {
              DN_CppIfChain(cpp)
              {
                DN_CppIfOrElseIfBlock(cpp,
                                      "*lhs.%.*s != *rhs.%.*s",
                                      DN_Str8PrintFmt(cpp_name.column.string),
                                      DN_Str8PrintFmt(cpp_name.column.string))
                {
                  DN_CppLine(cpp, "return false;");
                }
              }
            } else {
              DN_CppIfChain(cpp)
              {
                DN_CppIfOrElseIfBlock(cpp,
                                      "lhs.%.*s != rhs.%.*s",
                                      DN_Str8PrintFmt(cpp_name.column.string),
                                      DN_Str8PrintFmt(cpp_name.column.string))
                {
                  DN_CppLine(cpp, "return false;");
                }
              }
            }
          }
          DN_CppLine(cpp, "return true;");
        }
        DN_CppNewLine(cpp);

        DN_CppFuncBlock(cpp, "bool operator!=(%.*s%.*s const &lhs, %.*s%.*s const &rhs)", DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(type_name), DN_Str8PrintFmt(emit_prefix), DN_Str8PrintFmt(type_name))
        {
          DN_CppLine(cpp, "bool result = !(lhs == rhs);");
          DN_CppLine(cpp, "return result;");
        }
        DN_CppNewLine(cpp);
      }
    }
  }
}
