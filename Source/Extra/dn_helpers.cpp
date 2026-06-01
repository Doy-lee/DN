#define DN_HELPERS_CPP

#if defined(_CLANGD)
  #include "dn_helpers.h"
#endif

DN_API DN_JSONBuilder DN_JSONBuilder_Init(DN_Arena *arena, int spaces_per_indent)
{
  DN_JSONBuilder result       = {};
  result.spaces_per_indent    = spaces_per_indent;
  result.string_builder.arena = arena;
  return result;
}

DN_API DN_Str8 DN_JSONBuilder_Build(DN_JSONBuilder const *builder, DN_Arena *arena)
{
  DN_Str8 result = DN_Str8FromStr8BuilderArena(&builder->string_builder, arena);
  return result;
}

DN_API void DN_JSONBuilder_KeyValue(DN_JSONBuilder *builder, DN_Str8 key, DN_Str8 value)
{
  if (key.size == 0 && value.size == 0)
    return;

  DN_JSONBuilderItem item = DN_JSONBuilderItem_KeyValue;
  if (value.size >= 1) {
    if (value.data[0] == '{' || value.data[0] == '[')
      item = DN_JSONBuilderItem_OpenContainer;
    else if (value.data[0] == '}' || value.data[0] == ']')
      item = DN_JSONBuilderItem_CloseContainer;
  }

  bool adding_to_container_with_items =
      item != DN_JSONBuilderItem_CloseContainer && (builder->last_item == DN_JSONBuilderItem_KeyValue ||
                                                    builder->last_item == DN_JSONBuilderItem_CloseContainer);

  uint8_t prefix_size = 0;
  char    prefix[2]   = {0};
  if (adding_to_container_with_items)
    prefix[prefix_size++] = ',';

  if (builder->last_item != DN_JSONBuilderItem_Empty)
    prefix[prefix_size++] = '\n';

  if (item == DN_JSONBuilderItem_CloseContainer)
    builder->indent_level--;

  int spaces_per_indent = builder->spaces_per_indent ? builder->spaces_per_indent : 2;
  int spaces            = builder->indent_level * spaces_per_indent;

  if (key.size)
    DN_Str8BuilderAppendF(&builder->string_builder,
                           "%.*s%*c\"%.*s\": %.*s",
                           prefix_size,
                           prefix,
                           spaces,
                           ' ',
                           DN_Str8PrintFmt(key),
                           DN_Str8PrintFmt(value));
  else if (spaces == 0)
    DN_Str8BuilderAppendF(&builder->string_builder, "%.*s%.*s", prefix_size, prefix, DN_Str8PrintFmt(value));
  else
    DN_Str8BuilderAppendF(&builder->string_builder, "%.*s%*c%.*s", prefix_size, prefix, spaces, ' ', DN_Str8PrintFmt(value));

  if (item == DN_JSONBuilderItem_OpenContainer)
    builder->indent_level++;

  builder->last_item = item;
}

DN_API void DN_JSONBuilder_KeyValueFV(DN_JSONBuilder *builder, DN_Str8 key, char const *value_fmt, va_list args)
{
  DN_TCScratch scratch = DN_TCScratchBeginArena(&builder->string_builder.arena, 1);
  DN_Str8      value   = DN_Str8FromFmtVArena(&scratch.arena, value_fmt, args);
  DN_JSONBuilder_KeyValue(builder, key, value);
  DN_TCScratchEnd(&scratch);
}

DN_API void DN_JSONBuilder_KeyValueF(DN_JSONBuilder *builder, DN_Str8 key, char const *value_fmt, ...)
{
  va_list args;
  va_start(args, value_fmt);
  DN_JSONBuilder_KeyValueFV(builder, key, value_fmt, args);
  va_end(args);
}

DN_API void DN_JSONBuilder_ObjectBeginNamed(DN_JSONBuilder *builder, DN_Str8 name)
{
  DN_JSONBuilder_KeyValue(builder, name, DN_Str8Lit("{"));
}

DN_API void DN_JSONBuilder_ObjectEnd(DN_JSONBuilder *builder)
{
  DN_JSONBuilder_KeyValue(builder, DN_Str8Lit(""), DN_Str8Lit("}"));
}

DN_API void DN_JSONBuilder_ArrayBeginNamed(DN_JSONBuilder *builder, DN_Str8 name)
{
  DN_JSONBuilder_KeyValue(builder, name, DN_Str8Lit("["));
}

DN_API void DN_JSONBuilder_ArrayEnd(DN_JSONBuilder *builder)
{
  DN_JSONBuilder_KeyValue(builder, DN_Str8Lit(""), DN_Str8Lit("]"));
}

DN_API void DN_JSONBuilder_Str8Named(DN_JSONBuilder *builder, DN_Str8 key, DN_Str8 value)
{
  DN_JSONBuilder_KeyValueF(builder, key, "\"%.*s\"", value.size, value.data);
}

DN_API void DN_JSONBuilder_LiteralNamed(DN_JSONBuilder *builder, DN_Str8 key, DN_Str8 value)
{
  DN_JSONBuilder_KeyValueF(builder, key, "%.*s", value.size, value.data);
}

DN_API void DN_JSONBuilder_U64Named(DN_JSONBuilder *builder, DN_Str8 key, uint64_t value)
{
  DN_JSONBuilder_KeyValueF(builder, key, "%I64u", value);
}

DN_API void DN_JSONBuilder_I64Named(DN_JSONBuilder *builder, DN_Str8 key, int64_t value)
{
  DN_JSONBuilder_KeyValueF(builder, key, "%I64d", value);
}

DN_API void DN_JSONBuilder_F64Named(DN_JSONBuilder *builder, DN_Str8 key, double value, int decimal_places)
{
  if (!builder)
    return;

  if (decimal_places >= 16)
    decimal_places = 16;

  // NOTE: Generate the format string for the float, depending on how many
  // decimals places it wants.
  char float_fmt[16];
  if (decimal_places > 0) {
    // NOTE: Emit the format string "%.<decimal_places>f" i.e. %.1f
    DN_SNPrintF(float_fmt, sizeof(float_fmt), "%%.%df", decimal_places);
  } else {
    // NOTE: Emit the format string "%f"
    DN_SNPrintF(float_fmt, sizeof(float_fmt), "%%f");
  }
  DN_JSONBuilder_KeyValueF(builder, key, float_fmt, value);
}

DN_API void DN_JSONBuilder_BoolNamed(DN_JSONBuilder *builder, DN_Str8 key, bool value)
{
  DN_Str8 value_string = value ? DN_Str8Lit("true") : DN_Str8Lit("false");
  DN_JSONBuilder_KeyValueF(builder, key, "%.*s", value_string.size, value_string.data);
}
