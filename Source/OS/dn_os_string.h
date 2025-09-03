#if !defined(DN_OS_STRING_H)
#define DN_OS_STRING_H

#include "../dn_base_inc.h"
#include "../dn_os_inc.h"

// NOTE: DN_Str8 ///////////////////////////////////////////////////////////////////////////////////

DN_API DN_Str8                  DN_Str8_FromFrameFV                   (DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API DN_Str8                  DN_Str8_FromFrameF                    (DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_Str8                  DN_Str8_FromFrame                     (DN_USize size, DN_ZeroMem zero_mem);
DN_API DN_Str8                  DN_Str8_FromHeapF                     (DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_Str8                  DN_Str8_FromHeap                      (DN_USize size, DN_ZeroMem zero_mem);
DN_API DN_Str8                  DN_Str8_FromTLSFV                     (DN_FMT_ATTRIB char const *fmt, va_list args);
DN_API DN_Str8                  DN_Str8_FromTLSF                      (DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_Str8                  DN_Str8_FromTLS                       (DN_USize size, DN_ZeroMem zero_mem);
DN_API DN_Str8                  DN_Str8_FromStr8Frame                 (DN_Str8 string);
DN_API DN_Str8                  DN_Str8_FromStr8TLS                   (DN_Str8 string);

DN_API DN_Slice<DN_Str8>        DN_Str8_SplitFromFrame                (DN_Str8 string, DN_Str8 delimiter, DN_Str8SplitIncludeEmptyStrings mode);
DN_API DN_Slice<DN_Str8>        DN_Str8_SplitFromTLS                  (DN_Str8 string, DN_Str8 delimiter, DN_Str8SplitIncludeEmptyStrings mode);

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

DN_API  DN_Str8Builder          DN_Str8Builder_FromArena              ()                                                 { return DN_Str8Builder_FromArena(DN_OS_TLSGet()->frame_arena); }
DN_API  DN_Str8Builder          DN_Str8Builder_FromTLS                ()                                                 { return DN_Str8Builder_FromArena(DN_OS_TLSTopArena()); }

DN_API  DN_Str8Builder          DN_Str8Builder_FromStr8PtrRefFrame    (DN_Str8 const *strings, DN_USize size)            { return DN_Str8Builder_FromStr8PtrRef(DN_OS_TLSGet()->frame_arena, strings, size); }
DN_API  DN_Str8Builder          DN_Str8Builder_FromStr8PtrRefTLS      (DN_Str8 const *strings, DN_USize size)            { return DN_Str8Builder_FromStr8PtrRef(DN_OS_TLSTopArena(), strings, size); }
DN_API  DN_Str8Builder          DN_Str8Builder_FromStr8PtrCopyFrame   (DN_Str8 const *strings, DN_USize size)            { return DN_Str8Builder_FromStr8PtrCopy(DN_OS_TLSGet()->frame_arena, strings, size); }
DN_API  DN_Str8Builder          DN_Str8Builder_FromStr8PtrCopyTLS     (DN_Str8 const *strings, DN_USize size)            { return DN_Str8Builder_FromStr8PtrCopy(DN_OS_TLSTopArena(), strings, size); }

DN_API  DN_Str8Builder          DN_Str8Builder_FromBuilderFrame       (DN_Str8Builder const *builder)                    { return DN_Str8Builder_FromBuilder(DN_OS_TLSGet()->frame_arena, builder); }
DN_API  DN_Str8Builder          DN_Str8Builder_FromBuilderTLS         (DN_Str8Builder const *builder)                    { return DN_Str8Builder_FromBuilder(DN_OS_TLSTopArena(), builder); }

DN_API  DN_Str8                 DN_Str8Builder_BuildFromFrame         (DN_Str8Builder const *builder)                    { return DN_Str8Builder_Build(builder, DN_OS_TLSGet()->frame_arena); }
DN_API  DN_Str8                 DN_Str8Builder_BuildFromHeap          (DN_Str8Builder const *builder, DN_Arena *arena);
DN_API  DN_Str8                 DN_Str8Builder_BuildFromTLS           (DN_Str8Builder const *builder)                    { return DN_Str8Builder_Build(builder, DN_OS_TLSTopArena()); }

DN_API  DN_Str8                 DN_Str8Builder_BuildDelimitedFromFrame(DN_Str8Builder const *builder, DN_Str8 delimiter) { return DN_Str8Builder_BuildDelimited(builder, delimiter, DN_OS_TLSGet()->frame_arena); }
DN_API  DN_Str8                 DN_Str8Builder_BuildDelimitedFromTLS  (DN_Str8Builder const *builder, DN_Str8 delimiter) { return DN_Str8Builder_BuildDelimited(builder, delimiter, DN_OS_TLSTopArena()); }

DN_API  DN_Slice<DN_Str8>       DN_Str8Builder_BuildSliceFromFrame    (DN_Str8Builder const *builder)                    { return DN_Str8Builder_BuildSlice(builder, DN_OS_TLSGet()->frame_arena); }
DN_API  DN_Slice<DN_Str8>       DN_Str8Builder_BuildSliceFromTLS      (DN_Str8Builder const *builder)                    { return DN_Str8Builder_BuildSlice(builder, DN_OS_TLSTopArena()); }

#endif // !defined(DN_OS_STRING_H)
