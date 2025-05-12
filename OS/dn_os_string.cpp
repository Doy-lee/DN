#define DN_OS_STRING_CPP

// NOTE: DN_Str8 ///////////////////////////////////////////////////////////////////////////////////

DN_API  DN_Str8 DN_Str8_InitFFromFrame(DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8 result = DN_Str8_InitFV(DN_OS_TLSGet()->frame_arena, fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8 DN_Str8_InitFFromOSHeap(DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  DN_Str8  result = {};
  DN_USize size   = DN_CStr8_FVSize(fmt, args);
  if (size) {
    result = DN_Str8_AllocFromOSHeap(size, DN_ZeroMem_No);
    if (DN_Str8_HasData(result))
      DN_VSNPrintF(result.data, DN_SaturateCastISizeToInt(size + 1 /*null-terminator*/), fmt, args);
  }

  va_end(args);
  return result;
}

DN_API  DN_Str8 DN_Str8_InitFFromTLS(DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8 result = DN_Str8_InitFV(DN_OS_TLSTopArena(), fmt, args);
  va_end(args);
  return result;
}

DN_API  DN_Str8 DN_Str8_InitFVFromFrame(DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_Str8 result = DN_Str8_InitFV(DN_OS_TLSGet()->frame_arena, fmt, args);
  return result;
}

DN_API  DN_Str8 DN_Str8_InitFVFromTLS(DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_Str8 result = DN_Str8_InitFV(DN_OS_TLSTopArena(), fmt, args);
  return result;
}

DN_API DN_Str8 DN_Str8_AllocFromFrame(DN_USize size, DN_ZeroMem zero_mem)
{
  DN_Str8 result = DN_Str8_Alloc(DN_OS_TLSGet()->frame_arena, size, zero_mem);
  return result;
}

DN_API DN_Str8 DN_Str8_AllocFromOSHeap(DN_USize size, DN_ZeroMem zero_mem)
{
  DN_Str8 result = {};
  result.data    = DN_CAST(char *)DN_OS_MemAlloc(size + 1, zero_mem);
  if (result.data)
    result.size = size;
  result.data[result.size] = 0;
  return result;
}

DN_API DN_Str8 DN_Str8_AllocFromTLS(DN_USize size, DN_ZeroMem zero_mem)
{
  DN_Str8 result = DN_Str8_Alloc(DN_OS_TLSTopArena(), size, zero_mem);
  return result;
}

DN_API DN_Str8 DN_Str8_CopyFromFrame(DN_Str8 string)
{
  DN_Str8 result = DN_Str8_Copy(DN_OS_TLSGet()->frame_arena, string);
  return result;
}

DN_API DN_Str8 DN_Str8_CopyFromTLS(DN_Str8 string)
{
  DN_Str8 result = DN_Str8_Copy(DN_OS_TLSTopArena(), string);
  return result;
}

DN_API DN_Slice<DN_Str8> DN_Str8_SplitAllocFromFrame(DN_Str8 string, DN_Str8 delimiter, DN_Str8SplitIncludeEmptyStrings mode)
{
  DN_Slice<DN_Str8> result = DN_Str8_SplitAlloc(DN_OS_TLSGet()->frame_arena, string, delimiter, mode);
  return result;
}

DN_API DN_Slice<DN_Str8> DN_Str8_SplitAllocFromTLS(DN_Str8 string, DN_Str8 delimiter, DN_Str8SplitIncludeEmptyStrings mode)
{
  DN_Slice<DN_Str8> result = DN_Str8_SplitAlloc(DN_OS_TLSTopArena(), string, delimiter, mode);
  return result;
}

DN_API DN_Str8 DN_Str8_SegmentFromFrame(DN_Str8 src, DN_USize segment_size, char segment_char)
{
  DN_Str8 result = DN_Str8_Segment(DN_OS_TLSGet()->frame_arena, src, segment_size, segment_char);
  return result;
}

DN_API DN_Str8 DN_Str8_SegmentFromTLS(DN_Str8 src, DN_USize segment_size, char segment_char)
{
  DN_Str8 result = DN_Str8_Segment(DN_OS_TLSTopArena(), src, segment_size, segment_char);
  return result;
}

DN_API DN_Str8 DN_Str8_ReverseSegmentFromFrame(DN_Str8 src, DN_USize segment_size, char segment_char)
{
  DN_Str8 result = DN_Str8_ReverseSegment(DN_OS_TLSGet()->frame_arena, src, segment_size, segment_char);
  return result;
}

DN_API DN_Str8 DN_Str8_ReverseSegmentFromTLS(DN_Str8 src, DN_USize segment_size, char segment_char)
{
  DN_Str8 result = DN_Str8_ReverseSegment(DN_OS_TLSTopArena(), src, segment_size, segment_char);
  return result;
}

DN_API DN_Str8 DN_Str8_AppendFFromFrame(DN_Str8 string, char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8 result = DN_Str8_AppendFV(DN_OS_TLSGet()->frame_arena, string, fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8 DN_Str8_AppendFFromTLS(DN_Str8 string, char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8 result = DN_Str8_AppendFV(DN_OS_TLSTopArena(), string, fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8 DN_Str8_FillFFromFrame(DN_USize count, char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8 result = DN_Str8_FillFV(DN_OS_TLSGet()->frame_arena, count, fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8 DN_Str8_FillFFromTLS(DN_USize count, char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8 result = DN_Str8_FillFV(DN_OS_TLSTopArena(), count, fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8DotTruncateResult DN_Str8_DotTruncateMiddleFromFrame(DN_Str8 str8, uint32_t side_size, DN_Str8 truncator)
{
  DN_Str8DotTruncateResult result = DN_Str8_DotTruncateMiddle(DN_OS_TLSGet()->frame_arena, str8, side_size, truncator);
  return result;
}

DN_API DN_Str8DotTruncateResult DN_Str8_DotTruncateMiddleFromTLS(DN_Str8 str8, uint32_t side_size, DN_Str8 truncator)
{
  DN_Str8DotTruncateResult result = DN_Str8_DotTruncateMiddle(DN_OS_TLSTopArena(), str8, side_size, truncator);
  return result;
}


DN_API DN_Str8 DN_Str8_PadNewLines(DN_Arena *arena, DN_Str8 src, DN_Str8 pad)
{
  // TODO: Implement this without requiring TLS so it can go into base strings
  DN_OSTLSTMem     tmem    = DN_OS_TLSPushTMem(arena);
  DN_Str8Builder builder = DN_Str8Builder_InitFromTLS();

  DN_Str8BinarySplitResult split = DN_Str8_BinarySplit(src, DN_STR8("\n"));
  while (split.lhs.size) {
    DN_Str8Builder_AppendRef(&builder, pad);
    DN_Str8Builder_AppendRef(&builder, split.lhs);
    split = DN_Str8_BinarySplit(split.rhs, DN_STR8("\n"));
    if (split.lhs.size)
      DN_Str8Builder_AppendRef(&builder, DN_STR8("\n"));
  }

  DN_Str8 result = DN_Str8Builder_Build(&builder, arena);
  return result;
}

DN_API DN_Str8 DN_Str8_PadNewLinesFromFrame(DN_Str8 src, DN_Str8 pad)
{
  DN_Str8 result = DN_Str8_PadNewLines(DN_OS_TLSGet()->frame_arena, src, pad);
  return result;
}

DN_API DN_Str8 DN_Str8_PadNewLinesFromTLS(DN_Str8 src, DN_Str8 pad)
{
  DN_Str8 result = DN_Str8_PadNewLines(DN_OS_TLSTopArena(), src, pad);
  return result;
}

DN_API DN_Str8 DN_Str8_UpperFromFrame(DN_Str8 string)
{
  DN_Str8 result = DN_Str8_Upper(DN_OS_TLSGet()->frame_arena, string);
  return result;
}

DN_API DN_Str8 DN_Str8_UpperFromTLS(DN_Str8 string)
{
  DN_Str8 result = DN_Str8_Upper(DN_OS_TLSTopArena(), string);
  return result;
}

DN_API DN_Str8 DN_Str8_LowerFromFrame(DN_Str8 string)
{
  DN_Str8 result = DN_Str8_Lower(DN_OS_TLSGet()->frame_arena, string);
  return result;
}

DN_API DN_Str8 DN_Str8_LowerFromTLS(DN_Str8 string)
{
  DN_Str8 result = DN_Str8_Lower(DN_OS_TLSTopArena(), string);
  return result;
}

DN_API DN_Str8 DN_Str8_Replace(DN_Str8       string,
                               DN_Str8       find,
                               DN_Str8       replace,
                               DN_USize      start_index,
                               DN_Arena     *arena,
                               DN_Str8EqCase eq_case)
{
  // TODO: Implement this without requiring TLS so it can go into base strings
  DN_Str8 result = {};
  if (!DN_Str8_HasData(string) || !DN_Str8_HasData(find) || find.size > string.size || find.size == 0 || string.size == 0) {
    result = DN_Str8_Copy(arena, string);
    return result;
  }

  DN_OSTLSTMem     tmem           = DN_OS_TLSTMem(arena);
  DN_Str8Builder string_builder = DN_Str8Builder_Init(tmem.arena);
  DN_USize       max            = string.size - find.size;
  DN_USize       head           = start_index;

  for (DN_USize tail = head; tail <= max; tail++) {
    DN_Str8 check = DN_Str8_Slice(string, tail, find.size);
    if (!DN_Str8_Eq(check, find, eq_case))
      continue;

    if (start_index > 0 && string_builder.string_size == 0) {
      // User provided a hint in the string to start searching from, we
      // need to add the string up to the hint. We only do this if there's
      // a replacement action, otherwise we have a special case for no
      // replacements, where the entire string gets copied.
      DN_Str8 slice = DN_Str8_Init(string.data, head);
      DN_Str8Builder_AppendRef(&string_builder, slice);
    }

    DN_Str8 range = DN_Str8_Slice(string, head, (tail - head));
    DN_Str8Builder_AppendRef(&string_builder, range);
    DN_Str8Builder_AppendRef(&string_builder, replace);
    head = tail + find.size;
    tail += find.size - 1; // NOTE: -1 since the for loop will post increment us past the end of the find string
  }

  if (string_builder.string_size == 0) {
    // NOTE: No replacement possible, so we just do a full-copy
    result = DN_Str8_Copy(arena, string);
  } else {
    DN_Str8 remainder = DN_Str8_Init(string.data + head, string.size - head);
    DN_Str8Builder_AppendRef(&string_builder, remainder);
    result = DN_Str8Builder_Build(&string_builder, arena);
  }

  return result;
}

DN_API DN_Str8 DN_Str8_ReplaceInsensitive(DN_Str8 string, DN_Str8 find, DN_Str8 replace, DN_USize start_index, DN_Arena *arena)
{
  DN_Str8 result = DN_Str8_Replace(string, find, replace, start_index, arena, DN_Str8EqCase_Insensitive);
  return result;
}

// NOTE: DN_Str8Builder ////////////////////////////////////////////////////////////////////////////

DN_API DN_Str8 DN_Str8Builder_BuildFromOSHeap(DN_Str8Builder const *builder)
{
  DN_Str8 result = DN_ZeroInit;
  if (!builder || builder->string_size <= 0 || builder->count <= 0)
    return result;

  result.data = DN_CAST(char *) DN_OS_MemAlloc(builder->string_size + 1, DN_ZeroMem_No);
  if (!result.data)
    return result;

  for (DN_Str8Link *link = builder->head; link; link = link->next) {
    DN_Memcpy(result.data + result.size, link->string.data, link->string.size);
    result.size += link->string.size;
  }

  result.data[result.size] = 0;
  DN_Assert(result.size == builder->string_size);
  return result;
}
