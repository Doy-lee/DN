#include "dn_json_builder.h"

static void DN_JSB_Indent_(DN_JSBBuilder *builder, char *buf, size_t buf_size, size_t depth)
{
  for (size_t i = 0; i < depth; i++) {
    DN_JSB_AppendF_(builder, buf, buf_size, "  ");
  }
}

DN_JSBDepth *DN_JSB_PeekDepth_(DN_JSBBuilder *builder, size_t offset)
{
  DN_JSBDepth *result = 0;
  if (builder->depth_count > offset)
    result = &builder->depth[builder->depth_count - (1 + offset)];
  return result;
}

bool DN_JSB_PushDepth_(DN_JSBBuilder *builder, DN_JSBDepthType type)
{
  bool result = false;
  if (builder->depth_count < builder->depth_max) {
    DN_JSBDepth *depth = &builder->depth[builder->depth_count++];
    depth->last_action = DN_JSBAction_Nil;
    depth->type        = type;
    result             = true;
  }
  return result;
}

DN_JSBResult DN_JSB_PushObject(DN_JSBBuilder *builder, char *buf, size_t buf_size)
{
  DN_JSBDepth *top_depth = DN_JSB_PeekDepth_(builder, 0);
  if (top_depth && top_depth->type != DN_JSBDepthType_Array) {
    if ((top_depth->last_action == DN_JSBAction_Value || top_depth->last_action == DN_JSBAction_PopArray || top_depth->last_action == DN_JSBAction_PushObject))
      return DN_JSBResult_BadAPIUsage;
  }

  if (!DN_JSB_PushDepth_(builder, DN_JSBDepthType_Object))
    return DN_JSBResult_DepthArrayOutOfMem;

  bool         leading_comma = top_depth && (top_depth->last_action != DN_JSBAction_Nil && top_depth->last_action != DN_JSBAction_Key);
  if (builder->pretty && top_depth && top_depth->type == DN_JSBDepthType_Array) {
    if (leading_comma) {
      DN_JSB_AppendF_(builder, buf, buf_size, ",\n");
    } else {
      DN_JSB_AppendF_(builder, buf, buf_size, "\n");
    }
    DN_JSB_Indent_(builder, buf, buf_size, builder->depth_count - 1);
    leading_comma = false;
  }
  DN_JSBResult result = DN_JSB_AppendF_(builder, buf, buf_size, "%s{", leading_comma ? "," : "");
  if (top_depth)
    top_depth->last_action = DN_JSBAction_PushObject; // Record the push on the now, previous depth

  if (builder->error == DN_JSBResult_Success)
    builder->error = result;
  return result;
}

DN_JSBResult DN_JSB_PopObject(DN_JSBBuilder *builder, char *buf, size_t buf_size)
{
  DN_JSBDepth *top_depth = DN_JSB_PeekDepth_(builder, 0);
  if (!top_depth || top_depth->type != DN_JSBDepthType_Object)
    return DN_JSBResult_BadAPIUsage;

  bool had_children = top_depth->last_action != DN_JSBAction_Nil;
  builder->depth_count--; // Remove the depth entry from the stack

  DN_JSBDepth *new_top = DN_JSB_PeekDepth_(builder, 0);
  if (new_top)
    new_top->last_action = DN_JSBAction_PopObject;

  if (builder->pretty && had_children) {
    DN_JSB_AppendF_(builder, buf, buf_size, "\n");
    DN_JSB_Indent_(builder, buf, buf_size, builder->depth_count);
  }
  DN_JSBResult result = DN_JSB_AppendF_(builder, buf, buf_size, "}");
  if (builder->error == DN_JSBResult_Success)
    builder->error = result;
  return result;
}

DN_JSBResult DN_JSB_PushArray(DN_JSBBuilder *builder, char *buf, size_t buf_size)
{
  DN_JSBDepth *top_depth = DN_JSB_PeekDepth_(builder, 0);
  if (top_depth && top_depth->type != DN_JSBDepthType_Array) {
    if (top_depth->last_action == DN_JSBAction_Value || top_depth->last_action == DN_JSBAction_PopArray || top_depth->last_action == DN_JSBAction_PushObject || top_depth->last_action == DN_JSBAction_PopObject)
      return DN_JSBResult_BadAPIUsage;
  }

  if (!DN_JSB_PushDepth_(builder, DN_JSBDepthType_Array))
    return DN_JSBResult_DepthArrayOutOfMem;

  bool         leading_comma = top_depth && (top_depth->last_action != DN_JSBAction_Nil && top_depth->last_action != DN_JSBAction_Key);
  if (builder->pretty && top_depth && top_depth->type == DN_JSBDepthType_Array) {
    if (leading_comma) {
      DN_JSB_AppendF_(builder, buf, buf_size, ",\n");
    } else {
      DN_JSB_AppendF_(builder, buf, buf_size, "\n");
    }
    DN_JSB_Indent_(builder, buf, buf_size, builder->depth_count - 1);
    leading_comma = false;
  }
  DN_JSBResult result = DN_JSB_AppendF_(builder, buf, buf_size, "%s[", leading_comma ? "," : "");
  if (top_depth)
    top_depth->last_action = DN_JSBAction_PushArray; // Record the push on the now, previous depth
  if (builder->error == DN_JSBResult_Success)
    builder->error = result;
  return result;
}

DN_JSBResult DN_JSB_PopArray(DN_JSBBuilder *builder, char *buf, size_t buf_size)
{
  DN_JSBDepth *top_depth = DN_JSB_PeekDepth_(builder, 0);
  if (!top_depth || top_depth->type != DN_JSBDepthType_Array)
    return DN_JSBResult_BadAPIUsage;

  bool had_children = top_depth->last_action != DN_JSBAction_Nil;
  builder->depth_count--; // Remove the depth entry from the stack
  DN_JSBDepth *new_top = DN_JSB_PeekDepth_(builder, 0);
  if (new_top)
    new_top->last_action = DN_JSBAction_PopArray;

  if (builder->pretty && had_children) {
    DN_JSB_AppendF_(builder, buf, buf_size, "\n");
    DN_JSB_Indent_(builder, buf, buf_size, builder->depth_count);
  }
  DN_JSBResult result = DN_JSB_AppendF_(builder, buf, buf_size, "]");
  if (builder->error == DN_JSBResult_Success)
    builder->error = result;
  return result;
}

DN_JSBResult DN_JSB_KeyF(DN_JSBBuilder *builder, char *buf, size_t buf_size, char const *fmt, ...)
{
  DN_JSBDepth *top_depth = DN_JSB_PeekDepth_(builder, 0);
  if (!top_depth || top_depth->last_action == DN_JSBAction_Key || top_depth->last_action == DN_JSBAction_PushArray)
    return DN_JSBResult_BadAPIUsage;

  va_list args;
  va_start(args, fmt);
  if (builder->pretty) {
    if (top_depth->last_action != DN_JSBAction_Nil) {
      DN_JSB_AppendF_(builder, buf, buf_size, ",\n");
    } else {
      DN_JSB_AppendF_(builder, buf, buf_size, "\n");
    }
    DN_JSB_Indent_(builder, buf, buf_size, builder->depth_count);
    DN_JSB_AppendF_(builder, buf, buf_size, "\"");
    DN_JSB_AppendFV_(builder, buf, buf_size, fmt, args);
    DN_JSBResult result = DN_JSB_AppendF_(builder, buf, buf_size, "\": ");
    va_end(args);
    if (result == DN_JSBResult_Success)
      top_depth->last_action = DN_JSBAction_Key;
    if (builder->error == DN_JSBResult_Success)
      builder->error = result;
    return result;
  }
  DN_JSB_AppendF_(builder, buf, buf_size, "%s\"", top_depth->last_action != DN_JSBAction_Nil ? "," : "");
  DN_JSB_AppendFV_(builder, buf, buf_size, fmt, args);
  DN_JSBResult result = DN_JSB_AppendF_(builder, buf, buf_size, "\":");
  va_end(args);

  if (result == DN_JSBResult_Success)
    top_depth->last_action = DN_JSBAction_Key;
  if (builder->error == DN_JSBResult_Success)
    builder->error = result;
  return result;
}

DN_JSBResult DN_JSB_ValueStringF(DN_JSBBuilder *builder, char *buf, size_t buf_size, char const *fmt, ...)
{
  DN_JSBResult result = DN_JSB_BeginWriteValue_(builder, buf, buf_size);
  if (result == DN_JSBResult_Success) {
    DN_JSB_AppendF_(builder, buf, buf_size, "\"");
    va_list args;
    va_start(args, fmt);
    DN_JSB_AppendFV_(builder, buf, buf_size, fmt, args);
    va_end(args);
    result = DN_JSB_AppendF_(builder, buf, buf_size, "\"");
    DN_JSB_FinishWriteValue_(builder, result);
  }
  if (builder->error == DN_JSBResult_Success)
    builder->error = result;
  return result;
}

DN_JSBResult DN_JSB_ValueNull(DN_JSBBuilder *builder, char *buf, size_t buf_size)
{
  DN_JSBResult result = DN_JSB_BeginWriteValue_(builder, buf, buf_size);
  if (result == DN_JSBResult_Success) {
    result = DN_JSB_AppendF_(builder, buf, buf_size, "null");
    DN_JSB_FinishWriteValue_(builder, result);
  }
  if (builder->error == DN_JSBResult_Success)
    builder->error = result;
  return result;
}

DN_JSBResult DN_JSB_ValueBool(DN_JSBBuilder *builder, char *buf, size_t buf_size, bool flag)
{
  DN_JSBResult result = DN_JSB_BeginWriteValue_(builder, buf, buf_size);
  if (result == DN_JSBResult_Success) {
    result = DN_JSB_AppendF_(builder, buf, buf_size, flag ? "true" : "false");
    DN_JSB_FinishWriteValue_(builder, result);
  }
  if (builder->error == DN_JSBResult_Success)
    builder->error = result;
  return result;
}

DN_JSBResult DN_JSB_ValueNumber(DN_JSBBuilder *builder, char *buf, size_t buf_size, double number)
{
  DN_JSBResult result = DN_JSB_BeginWriteValue_(builder, buf, buf_size);
  if (result == DN_JSBResult_Success) {
    result = DN_JSB_AppendF_(builder, buf, buf_size, "%.17G", number);
    DN_JSB_FinishWriteValue_(builder, result);
  }
  if (builder->error == DN_JSBResult_Success)
    builder->error = result;
  return result;
}

DN_JSBResult DN_JSB_BeginWriteValue_(DN_JSBBuilder *builder, char *buf, size_t buf_size)
{
  DN_JSBDepth *top = DN_JSB_PeekDepth_(builder, 0);
  if (!top)
    return DN_JSBResult_BadAPIUsage;

  bool is_key   = top->last_action == DN_JSBAction_Key;
  bool is_array = top->type == DN_JSBDepthType_Array;
  if (!is_key && !is_array)
    return DN_JSBResult_BadAPIUsage;

  DN_JSBResult result = DN_JSBResult_Success;
  if (top->type == DN_JSBDepthType_Array) {
    if (builder->pretty) {
      if (top->last_action != DN_JSBAction_Nil) {
        DN_JSB_AppendF_(builder, buf, buf_size, ",\n");
      } else {
        DN_JSB_AppendF_(builder, buf, buf_size, "\n");
      }
      DN_JSB_Indent_(builder, buf, buf_size, builder->depth_count);
    } else if (top->last_action != DN_JSBAction_Nil) {
      result = DN_JSB_AppendF_(builder, buf, buf_size, ",");
    }
  }
  return result;
}

void DN_JSB_FinishWriteValue_(DN_JSBBuilder *builder, DN_JSBResult last_value_raw_str_result)
{
  if (last_value_raw_str_result == DN_JSBResult_Success) {
    DN_JSBDepth *top_depth = DN_JSB_PeekDepth_(builder, 0);
    top_depth->last_action = DN_JSBAction_Value;
  }
}

DN_JSBResult DN_JSB_AppendFV_(DN_JSBBuilder *builder, char *buf, size_t buf_size, char const *fmt, va_list args)
{
  // NOTE: Calc size required
  size_t size = 0;
  {
    va_list args_copy;
    va_copy(args_copy, args);
    size = DN_JSB_VSNPrintF(0, 0, fmt, args_copy);
    va_end(args_copy);
  }

  // NOTE: Do the write
  DN_JSBResult result = DN_JSBResult_Success;
  if (buf) {
    if (builder->p > buf_size)
      return DN_JSBResult_BadAPIUsage;

    char  *dest      = buf + builder->p;
    size_t dest_size = (buf + buf_size) - dest;
    if ((size + 1) <= dest_size) { // NOTE: +1 because vsnprintf always null-terminates the stream
      DN_JSB_VSNPrintF(dest, dest_size, fmt, args);
    } else {
      result = DN_JSBResult_BufOutOfMem;
    }
  }

  if (result == DN_JSBResult_Success)
    builder->p += size;
  if (builder->error == DN_JSBResult_Success)
    builder->error = result;
  return result;
}

DN_JSBResult DN_JSB_AppendF_(DN_JSBBuilder *builder, char *buf, size_t buf_size, char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_JSBResult result = DN_JSB_AppendFV_(builder, buf, buf_size, fmt, args);
  va_end(args);
  return result;
}
