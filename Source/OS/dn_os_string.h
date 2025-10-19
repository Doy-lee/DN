#if !defined(DN_OS_STRING_H)
#define DN_OS_STRING_H

#include "../dn_base_inc.h"
#include "../dn_os_inc.h"

// NOTE: DN_Str8 ///////////////////////////////////////////////////////////////////////////////////

DN_API DN_Str8                  DN_Str8FromFmtVArenaFrame            (DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API DN_Str8                  DN_Str8FromFmtArenaFrame             (DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_Str8                  DN_Str8FromArenaFrame                (DN_USize size, DN_ZMem z_mem);
DN_API DN_Str8                  DN_Str8FromHeapF                     (DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_Str8                  DN_Str8FromHeap                      (DN_USize size, DN_ZMem z_mem);
DN_API DN_Str8                  DN_Str8FromTLSFV                     (DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API DN_Str8                  DN_Str8FromTLSF                      (DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_Str8                  DN_Str8FromTLS                       (DN_USize size, DN_ZMem z_mem);
DN_API DN_Str8                  DN_Str8FromStr8Frame                 (DN_Str8 string);
DN_API DN_Str8                  DN_Str8FromStr8TLS                   (DN_Str8 string);

DN_API DN_Str8SplitResult       DN_Str8SplitFromFrame                (DN_Str8 string, DN_Str8 delimiter, DN_Str8SplitIncludeEmptyStrings mode);
DN_API DN_Str8SplitResult       DN_Str8SplitFromTLS                  (DN_Str8 string, DN_Str8 delimiter, DN_Str8SplitIncludeEmptyStrings mode);

DN_API DN_Str8                  DN_Str8SegmentFromFrame              (DN_Str8 src, DN_USize segment_size, char segment_char);
DN_API DN_Str8                  DN_Str8SegmentFromTLS                (DN_Str8 src, DN_USize segment_size, char segment_char);

DN_API DN_Str8                  DN_Str8ReverseSegmentFromFrame       (DN_Str8 src, DN_USize segment_size, char segment_char);
DN_API DN_Str8                  DN_Str8ReverseSegmentFromTLS         (DN_Str8 src, DN_USize segment_size, char segment_char);

DN_API DN_Str8                  DN_Str8AppendFFromFrame              (DN_Str8 string, char const *fmt, ...);
DN_API DN_Str8                  DN_Str8AppendFFromTLS                (DN_Str8 string, char const *fmt, ...);

DN_API DN_Str8                  DN_Str8FillFFromFrame                (DN_Str8 string, char const *fmt, ...);
DN_API DN_Str8                  DN_Str8FillFFromTLS                  (DN_Str8 string, char const *fmt, ...);

DN_API DN_Str8TruncateResult    DN_Str8TruncateMiddleArenaFrame      (DN_Str8 str8, uint32_t side_size, DN_Str8 truncator);
DN_API DN_Str8TruncateResult    DN_Str8TruncateMiddleArenaTLS        (DN_Str8 str8, uint32_t side_size, DN_Str8 truncator);

DN_API DN_Str8                  DN_Str8PadNewLines                   (DN_Arena *arena, DN_Str8 src, DN_Str8 pad);
DN_API DN_Str8                  DN_Str8PadNewLinesFromFrame          (DN_Str8 src, DN_Str8 pad);
DN_API DN_Str8                  DN_Str8PadNewLinesFromTLS            (DN_Str8 src, DN_Str8 pad);

DN_API DN_Str8                  DN_Str8UpperFromFrame                (DN_Str8 string);
DN_API DN_Str8                  DN_Str8UpperFromTLS                  (DN_Str8 string);

DN_API DN_Str8                  DN_Str8LowerFromFrame                (DN_Str8 string);
DN_API DN_Str8                  DN_Str8LowerFromTLS                  (DN_Str8 string);

DN_API DN_Str8                  DN_Str8Replace                       (DN_Str8 string, DN_Str8 find, DN_Str8 replace, DN_USize start_index, DN_Arena *arena, DN_Str8EqCase eq_case = DN_Str8EqCase_Sensitive);
DN_API DN_Str8                  DN_Str8ReplaceInsensitive            (DN_Str8 string, DN_Str8 find, DN_Str8 replace, DN_USize start_index, DN_Arena *arena);

// NOTE: DN_Str8Builder ////////////////////////////////////////////////////////////////////////////

DN_API  DN_Str8Builder          DN_Str8BuilderFromArena              ()                                                 { return DN_Str8BuilderFromArena(DN_OS_TLSGet()->frame_arena); }
DN_API  DN_Str8Builder          DN_Str8BuilderFromTLS                ()                                                 { return DN_Str8BuilderFromArena(DN_OS_TLSTopArena()); }

DN_API  DN_Str8Builder          DN_Str8BuilderFromStr8PtrRefFrame    (DN_Str8 const *strings, DN_USize size)            { return DN_Str8BuilderFromStr8PtrRef(DN_OS_TLSGet()->frame_arena, strings, size); }
DN_API  DN_Str8Builder          DN_Str8BuilderFromStr8PtrRefTLS      (DN_Str8 const *strings, DN_USize size)            { return DN_Str8BuilderFromStr8PtrRef(DN_OS_TLSTopArena(), strings, size); }
DN_API  DN_Str8Builder          DN_Str8BuilderFromStr8PtrCopyFrame   (DN_Str8 const *strings, DN_USize size)            { return DN_Str8BuilderFromStr8PtrCopy(DN_OS_TLSGet()->frame_arena, strings, size); }
DN_API  DN_Str8Builder          DN_Str8BuilderFromStr8PtrCopyTLS     (DN_Str8 const *strings, DN_USize size)            { return DN_Str8BuilderFromStr8PtrCopy(DN_OS_TLSTopArena(), strings, size); }

DN_API  DN_Str8Builder          DN_Str8BuilderFromBuilderFrame       (DN_Str8Builder const *builder)                    { return DN_Str8BuilderFromBuilder(DN_OS_TLSGet()->frame_arena, builder); }
DN_API  DN_Str8Builder          DN_Str8BuilderFromBuilderTLS         (DN_Str8Builder const *builder)                    { return DN_Str8BuilderFromBuilder(DN_OS_TLSTopArena(), builder); }

DN_API  DN_Str8                 DN_Str8BuilderBuildFromFrame         (DN_Str8Builder const *builder)                    { return DN_Str8BuilderBuild(builder, DN_OS_TLSGet()->frame_arena); }
DN_API  DN_Str8                 DN_Str8BuilderBuildFromHeap          (DN_Str8Builder const *builder, DN_Arena *arena);
DN_API  DN_Str8                 DN_Str8BuilderBuildFromTLS           (DN_Str8Builder const *builder)                    { return DN_Str8BuilderBuild(builder, DN_OS_TLSTopArena()); }

DN_API  DN_Str8                 DN_Str8BuilderBuildDelimitedFromFrame(DN_Str8Builder const *builder, DN_Str8 delimiter) { return DN_Str8BuilderBuildDelimited(builder, delimiter, DN_OS_TLSGet()->frame_arena); }
DN_API  DN_Str8                 DN_Str8BuilderBuildDelimitedFromTLS  (DN_Str8Builder const *builder, DN_Str8 delimiter) { return DN_Str8BuilderBuildDelimited(builder, delimiter, DN_OS_TLSTopArena()); }

DN_API  DN_Slice<DN_Str8>       DN_Str8BuilderBuildSliceFromFrame    (DN_Str8Builder const *builder)                    { return DN_Str8BuilderBuildSlice(builder, DN_OS_TLSGet()->frame_arena); }
DN_API  DN_Slice<DN_Str8>       DN_Str8BuilderBuildSliceFromTLS      (DN_Str8Builder const *builder)                    { return DN_Str8BuilderBuildSlice(builder, DN_OS_TLSTopArena()); }

#endif // !defined(DN_OS_STRING_H)
