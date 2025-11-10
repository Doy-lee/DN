#define DN_OS_STRING_CPP

#if defined(_CLANGD)
  #include "../dn_base_inc.h"
  #include "../dn_os_inc.h"
#endif

// NOTE: DN_Str8
DN_API  DN_Str8 DN_Str8FromFmtArenaFrame(DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Arena *frame_arena = DN_OS_TLSGet()->frame_arena;
  DN_Str8   result      = DN_Str8FromFmtVArena(frame_arena, fmt, args);
  va_end(args);
  return result;
}

DN_API  DN_Str8 DN_Str8FromFmtVArenaFrame(DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_Arena *frame_arena = DN_OS_TLSGet()->frame_arena;
  DN_Str8   result      = DN_Str8FromFmtVArena(frame_arena, fmt, args);
  return result;
}

DN_API DN_Str8 DN_Str8FromArenaFrame(DN_USize size, DN_ZMem z_mem)
{
  DN_Arena *frame_arena = DN_OS_TLSGet()->frame_arena;
  DN_Str8   result      = DN_Str8FromArena(frame_arena, size, z_mem);
  return result;
}

DN_API DN_Str8 DN_Str8FromHeapF(DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_USize size   = DN_FmtVSize(fmt, args);
  DN_Str8  result = DN_Str8FromHeap(size, DN_ZMem_No);
  DN_VSNPrintF(result.data, DN_Cast(int)(result.size + 1), fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8 DN_Str8FromHeap(DN_USize size, DN_ZMem z_mem)
{
  DN_Str8 result = {};
  result.data    = DN_Cast(char *)DN_OS_MemAlloc(size + 1, z_mem);
  if (result.data) {
    result.size              = size;
    result.data[result.size] = 0;
  }
  return result;
}

DN_API DN_Str8 DN_Str8FromTLSFV(DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_Str8 result = DN_Str8FromFmtVArena(DN_OS_TLSTopArena(), fmt, args);
  return result;
}


DN_API DN_Str8 DN_Str8FromTLSF(DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8 result = DN_Str8FromFmtVArena(DN_OS_TLSTopArena(), fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8 DN_Str8FromTLS(DN_USize size, DN_ZMem z_mem)
{
  DN_Str8 result = DN_Str8FromArena(DN_OS_TLSTopArena(), size, z_mem);
  return result;
}

DN_API DN_Str8 DN_Str8FromStr8Frame(DN_Str8 string)
{
  DN_Str8 result = DN_Str8FromStr8Arena(DN_OS_TLSGet()->frame_arena, string);
  return result;
}

DN_API DN_Str8 DN_Str8FromStr8TLS(DN_Str8 string)
{
  DN_Str8 result = DN_Str8FromStr8Arena(DN_OS_TLSTopArena(), string);
  return result;
}

DN_API DN_Str8SplitResult DN_Str8SplitFromFrame(DN_Str8 string, DN_Str8 delimiter, DN_Str8SplitIncludeEmptyStrings mode)
{
  DN_Str8SplitResult result = DN_Str8SplitArena(DN_OS_TLSGet()->frame_arena, string, delimiter, mode);
  return result;
}

DN_API DN_Str8SplitResult DN_Str8SplitFromTLS(DN_Str8 string, DN_Str8 delimiter, DN_Str8SplitIncludeEmptyStrings mode)
{
  DN_Str8SplitResult result = DN_Str8SplitArena(DN_OS_TLSTopArena(), string, delimiter, mode);
  return result;
}

DN_API DN_Str8 DN_Str8SegmentFromFrame(DN_Str8 src, DN_USize segment_size, char segment_char)
{
  DN_Str8 result = DN_Str8Segment(DN_OS_TLSGet()->frame_arena, src, segment_size, segment_char);
  return result;
}

DN_API DN_Str8 DN_Str8SegmentFromTLS(DN_Str8 src, DN_USize segment_size, char segment_char)
{
  DN_Str8 result = DN_Str8Segment(DN_OS_TLSTopArena(), src, segment_size, segment_char);
  return result;
}

DN_API DN_Str8 DN_Str8ReverseSegmentFromFrame(DN_Str8 src, DN_USize segment_size, char segment_char)
{
  DN_Str8 result = DN_Str8ReverseSegment(DN_OS_TLSGet()->frame_arena, src, segment_size, segment_char);
  return result;
}

DN_API DN_Str8 DN_Str8ReverseSegmentFromTLS(DN_Str8 src, DN_USize segment_size, char segment_char)
{
  DN_Str8 result = DN_Str8ReverseSegment(DN_OS_TLSTopArena(), src, segment_size, segment_char);
  return result;
}

DN_API DN_Str8 DN_Str8AppendFFromFrame(DN_Str8 string, char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8 result = DN_Str8AppendFV(DN_OS_TLSGet()->frame_arena, string, fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8 DN_Str8AppendFFromTLS(DN_Str8 string, char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8 result = DN_Str8AppendFV(DN_OS_TLSTopArena(), string, fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8 DN_Str8FillFFromFrame(DN_USize count, char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8 result = DN_Str8FillFV(DN_OS_TLSGet()->frame_arena, count, fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8 DN_Str8FillFFromTLS(DN_USize count, char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_Str8 result = DN_Str8FillFV(DN_OS_TLSTopArena(), count, fmt, args);
  va_end(args);
  return result;
}

DN_API DN_Str8TruncateResult DN_Str8TruncateMiddleArenaFrame(DN_Str8 str8, uint32_t side_size, DN_Str8 truncator)
{
  DN_Str8TruncateResult result = DN_Str8TruncateMiddle(DN_OS_TLSGet()->frame_arena, str8, side_size, truncator);
  return result;
}

DN_API DN_Str8TruncateResult DN_Str8TruncateMiddleArenaTLS(DN_Str8 str8, uint32_t side_size, DN_Str8 truncator)
{
  DN_Str8TruncateResult result = DN_Str8TruncateMiddle(DN_OS_TLSTopArena(), str8, side_size, truncator);
  return result;
}

DN_API DN_Str8 DN_Str8PadNewLines(DN_Arena *arena, DN_Str8 src, DN_Str8 pad)
{
  // TODO: Implement this without requiring TLS so it can go into base strings
  DN_OSTLSTMem   tmem    = DN_OS_TLSPushTMem(arena);
  DN_Str8Builder builder = DN_Str8BuilderFromTLS();

  DN_Str8BSplitResult split = DN_Str8BSplit(src, DN_Str8Lit("\n"));
  while (split.lhs.size) {
    DN_Str8BuilderAppendRef(&builder, pad);
    DN_Str8BuilderAppendRef(&builder, split.lhs);
    split = DN_Str8BSplit(split.rhs, DN_Str8Lit("\n"));
    if (split.lhs.size)
      DN_Str8BuilderAppendRef(&builder, DN_Str8Lit("\n"));
  }

  DN_Str8 result = DN_Str8BuilderBuild(&builder, arena);
  return result;
}

DN_API DN_Str8 DN_Str8PadNewLinesFromFrame(DN_Str8 src, DN_Str8 pad)
{
  DN_Str8 result = DN_Str8PadNewLines(DN_OS_TLSGet()->frame_arena, src, pad);
  return result;
}

DN_API DN_Str8 DN_Str8PadNewLinesFromTLS(DN_Str8 src, DN_Str8 pad)
{
  DN_Str8 result = DN_Str8PadNewLines(DN_OS_TLSTopArena(), src, pad);
  return result;
}

DN_API DN_Str8 DN_Str8UpperFromFrame(DN_Str8 string)
{
  DN_Str8 result = DN_Str8Upper(DN_OS_TLSGet()->frame_arena, string);
  return result;
}

DN_API DN_Str8 DN_Str8UpperFromTLS(DN_Str8 string)
{
  DN_Str8 result = DN_Str8Upper(DN_OS_TLSTopArena(), string);
  return result;
}

DN_API DN_Str8 DN_Str8LowerFromFrame(DN_Str8 string)
{
  DN_Str8 result = DN_Str8Lower(DN_OS_TLSGet()->frame_arena, string);
  return result;
}

DN_API DN_Str8 DN_Str8LowerFromTLS(DN_Str8 string)
{
  DN_Str8 result = DN_Str8Lower(DN_OS_TLSTopArena(), string);
  return result;
}

DN_API DN_Str8 DN_Str8Replace(DN_Str8       string,
                               DN_Str8       find,
                               DN_Str8       replace,
                               DN_USize      start_index,
                               DN_Arena     *arena,
                               DN_Str8EqCase eq_case)
{
  // TODO: Implement this without requiring TLS so it can go into base strings
  DN_Str8 result = {};
  if (string.size == 0 || find.size == 0 || find.size > string.size || find.size == 0 || string.size == 0) {
    result = DN_Str8FromStr8Arena(arena, string);
    return result;
  }

  DN_OSTLSTMem   tmem           = DN_OS_TLSTMem(arena);
  DN_Str8Builder string_builder = DN_Str8BuilderFromArena(tmem.arena);
  DN_USize       max            = string.size - find.size;
  DN_USize       head           = start_index;

  for (DN_USize tail = head; tail <= max; tail++) {
    DN_Str8 check = DN_Str8Slice(string, tail, find.size);
    if (!DN_Str8Eq(check, find, eq_case))
      continue;

    if (start_index > 0 && string_builder.string_size == 0) {
      // User provided a hint in the string to start searching from, we
      // need to add the string up to the hint. We only do this if there's
      // a replacement action, otherwise we have a special case for no
      // replacements, where the entire string gets copied.
      DN_Str8 slice = DN_Str8FromPtr(string.data, head);
      DN_Str8BuilderAppendRef(&string_builder, slice);
    }

    DN_Str8 range = DN_Str8Slice(string, head, (tail - head));
    DN_Str8BuilderAppendRef(&string_builder, range);
    DN_Str8BuilderAppendRef(&string_builder, replace);
    head = tail + find.size;
    tail += find.size - 1; // NOTE: -1 since the for loop will post increment us past the end of the find string
  }

  if (string_builder.string_size == 0) {
    // NOTE: No replacement possible, so we just do a full-copy
    result = DN_Str8FromStr8Arena(arena, string);
  } else {
    DN_Str8 remainder = DN_Str8FromPtr(string.data + head, string.size - head);
    DN_Str8BuilderAppendRef(&string_builder, remainder);
    result = DN_Str8BuilderBuild(&string_builder, arena);
  }

  return result;
}

DN_API DN_Str8 DN_Str8ReplaceInsensitive(DN_Str8 string, DN_Str8 find, DN_Str8 replace, DN_USize start_index, DN_Arena *arena)
{
  DN_Str8 result = DN_Str8Replace(string, find, replace, start_index, arena, DN_Str8EqCase_Insensitive);
  return result;
}

// NOTE: DN_Str8Builder ////////////////////////////////////////////////////////////////////////////
DN_API DN_Str8 DN_Str8BuilderBuildFromOSHeap(DN_Str8Builder const *builder)
{
  DN_Str8 result = DN_ZeroInit;
  if (!builder || builder->string_size <= 0 || builder->count <= 0)
    return result;

  result.data = DN_Cast(char *) DN_OS_MemAlloc(builder->string_size + 1, DN_ZMem_No);
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
