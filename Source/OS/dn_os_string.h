#if !defined(DN_OS_STRING_H)
#define DN_OS_STRING_H

// NOTE: DN_Str8 ///////////////////////////////////////////////////////////////////////////////////

DN_API  DN_Str8                 DN_Str8_InitFFromFrame                (DN_FMT_ATTRIB char const *fmt, ...);
DN_API  DN_Str8                 DN_Str8_InitFFromOSHeap               (DN_FMT_ATTRIB char const *fmt, ...);
DN_API  DN_Str8                 DN_Str8_InitFFromTLS                  (DN_FMT_ATTRIB char const *fmt, ...);

DN_API  DN_Str8                 DN_Str8_InitFVFromFrame               (DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API  DN_Str8                 DN_Str8_InitFVFromTLS                 (DN_FMT_ATTRIB char const *fmt, va_list args);

DN_API  DN_Str8                 DN_Str8_AllocFromFrame                (DN_USize size, DN_ZeroMem zero_mem);
DN_API  DN_Str8                 DN_Str8_AllocFromOSHeap               (DN_USize size, DN_ZeroMem zero_mem);
DN_API  DN_Str8                 DN_Str8_AllocFromTLS                  (DN_USize size, DN_ZeroMem zero_mem);

DN_API DN_Str8                  DN_Str8_CopyFromFrame                 (DN_Arena *arena, DN_Str8 string);
DN_API DN_Str8                  DN_Str8_CopyFromTLS                   (DN_Arena *arena, DN_Str8 string);

DN_API DN_Slice<DN_Str8>        DN_Str8_SplitAllocFromFrame           (DN_Str8 string, DN_Str8 delimiter, DN_Str8SplitIncludeEmptyStrings mode);
DN_API DN_Slice<DN_Str8>        DN_Str8_SplitAllocFromTLS             (DN_Str8 string, DN_Str8 delimiter, DN_Str8SplitIncludeEmptyStrings mode);

DN_API DN_Str8                  DN_Str8_SegmentFromFrame              (DN_Str8 src, DN_USize segment_size, char segment_char);
DN_API DN_Str8                  DN_Str8_SegmentFromTLS                (DN_Str8 src, DN_USize segment_size, char segment_char);

DN_API DN_Str8                  DN_Str8_ReverseSegmentFromFrame       (DN_Str8 src, DN_USize segment_size, char segment_char);
DN_API DN_Str8                  DN_Str8_ReverseSegmentFromTLS         (DN_Str8 src, DN_USize segment_size, char segment_char);

DN_API DN_Str8                  DN_Str8_AppendFFromFrame              (DN_Str8 string, char const *fmt, ...);
DN_API DN_Str8                  DN_Str8_AppendFFromTLS                (DN_Str8 string, char const *fmt, ...);

DN_API DN_Str8                  DN_Str8_FillFFromFrame                (DN_Str8 string, char const *fmt, ...);
DN_API DN_Str8                  DN_Str8_FillFFromTLS                  (DN_Str8 string, char const *fmt, ...);

DN_API DN_Str8DotTruncateResult DN_Str8_DotTruncateMiddleFromFrame    (DN_Str8 str8, uint32_t side_size, DN_Str8 truncator);
DN_API DN_Str8DotTruncateResult DN_Str8_DotTruncateMiddleFromTLS      (DN_Str8 str8, uint32_t side_size, DN_Str8 truncator);

DN_API DN_Str8                  DN_Str8_PadNewLines                   (DN_Arena *arena, DN_Str8 src, DN_Str8 pad);
DN_API DN_Str8                  DN_Str8_PadNewLinesFromFrame          (DN_Str8 src, DN_Str8 pad);
DN_API DN_Str8                  DN_Str8_PadNewLinesFromTLS            (DN_Str8 src, DN_Str8 pad);

DN_API DN_Str8                  DN_Str8_UpperFromFrame                (DN_Str8 string);
DN_API DN_Str8                  DN_Str8_UpperFromTLS                  (DN_Str8 string);

DN_API DN_Str8                  DN_Str8_LowerFromFrame                (DN_Str8 string);
DN_API DN_Str8                  DN_Str8_LowerFromTLS                  (DN_Str8 string);

DN_API DN_Str8                  DN_Str8_Replace                       (DN_Str8 string, DN_Str8 find, DN_Str8 replace, DN_USize start_index, DN_Arena *arena, DN_Str8EqCase eq_case = DN_Str8EqCase_Sensitive);
DN_API DN_Str8                  DN_Str8_ReplaceInsensitive            (DN_Str8 string, DN_Str8 find, DN_Str8 replace, DN_USize start_index, DN_Arena *arena);

// NOTE: DN_Str8Builder ////////////////////////////////////////////////////////////////////////////

DN_API  DN_Str8Builder          DN_Str8Builder_InitFromFrame          ()                                                 { return DN_Str8Builder_Init(DN_OS_TLSGet()->frame_arena); }
DN_API  DN_Str8Builder          DN_Str8Builder_InitFromTLS            ()                                                 { return DN_Str8Builder_Init(DN_OS_TLSTopArena()); }

DN_API  DN_Str8Builder          DN_Str8Builder_InitArrayRefFromFrame  (DN_Str8 const *strings, DN_USize size)            { return DN_Str8Builder_InitArrayRef(DN_OS_TLSGet()->frame_arena, strings, size); }
DN_API  DN_Str8Builder          DN_Str8Builder_InitArrayRefFromTLS    (DN_Str8 const *strings, DN_USize size)            { return DN_Str8Builder_InitArrayRef(DN_OS_TLSTopArena(), strings, size); }
DN_API  DN_Str8Builder          DN_Str8Builder_InitArrayCopyFromFrame (DN_Str8 const *strings, DN_USize size)            { return DN_Str8Builder_InitArrayCopy(DN_OS_TLSGet()->frame_arena, strings, size); }
DN_API  DN_Str8Builder          DN_Str8Builder_InitArrayCopyFromTLS   (DN_Str8 const *strings, DN_USize size)            { return DN_Str8Builder_InitArrayCopy(DN_OS_TLSTopArena(), strings, size); }

DN_API  DN_Str8Builder          DN_Str8Builder_CopyFromFrame          (DN_Str8Builder const *builder)                    { return DN_Str8Builder_Copy(DN_OS_TLSGet()->frame_arena, builder); }
DN_API  DN_Str8Builder          DN_Str8Builder_CopyFromTLS            (DN_Str8Builder const *builder)                    { return DN_Str8Builder_Copy(DN_OS_TLSTopArena(), builder); }

DN_API  DN_Str8                 DN_Str8Builder_BuildFromFrame         (DN_Str8Builder const *builder)                    { return DN_Str8Builder_Build(builder, DN_OS_TLSGet()->frame_arena); }
DN_API  DN_Slice<DN_Str8>       DN_Str8Builder_BuildFromOSHeap        (DN_Str8Builder const *builder, DN_Arena *arena);
DN_API  DN_Str8                 DN_Str8Builder_BuildFromTLS           (DN_Str8Builder const *builder)                    { return DN_Str8Builder_Build(builder, DN_OS_TLSTopArena()); }

DN_API  DN_Str8                 DN_Str8Builder_BuildDelimitedFromFrame(DN_Str8Builder const *builder, DN_Str8 delimiter) { return DN_Str8Builder_BuildDelimited(builder, delimiter, DN_OS_TLSGet()->frame_arena); }
DN_API  DN_Str8                 DN_Str8Builder_BuildDelimitedFromTLS  (DN_Str8Builder const *builder, DN_Str8 delimiter) { return DN_Str8Builder_BuildDelimited(builder, delimiter, DN_OS_TLSTopArena()); }

DN_API  DN_Slice<DN_Str8>       DN_Str8Builder_BuildSliceFromFrame    (DN_Str8Builder const *builder)                    { return DN_Str8Builder_BuildSlice(builder, DN_OS_TLSGet()->frame_arena); }
DN_API  DN_Slice<DN_Str8>       DN_Str8Builder_BuildSliceFromTLS      (DN_Str8Builder const *builder)                    { return DN_Str8Builder_BuildSlice(builder, DN_OS_TLSTopArena()); }

#endif // !defined(DN_OS_STRING_H)
