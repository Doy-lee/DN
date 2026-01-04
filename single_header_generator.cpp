#define USE_SINGLE_HEADER 1

#define DN_H_WITH_OS 1
#define DN_H_WITH_CORE 1
#if USE_SINGLE_HEADER
#include "Single-Header/dn_single_header.h"
#else
#include "Source/dn.h"
#endif

#if USE_SINGLE_HEADER
#include "Single-Header/dn_single_header.cpp"
#else
#include "Source/dn.cpp"
#endif

enum FileType
{
  FileType_Header,
  FileType_Impl,
  FileType_Count
};

struct File
{
  FileType type;
  DN_Str8  file_name;
};

static void AppendCppFileLineByLine(DN_Str8Builder *dest, DN_Str8 cpp_path)
{
  DN_OSErrSink *err    = DN_OS_ErrSinkBeginDefault();
  DN_Str8       buffer = DN_OS_FileReadAllTLS(cpp_path, err);
  DN_OS_ErrSinkEndAndExitIfErrorF(err, -1, "Failed to load file from '%S' for appending", cpp_path);

  for (DN_Str8 inc_walker = buffer;;) {
    DN_Str8BSplitResult split = DN_Str8BSplit(inc_walker, DN_Str8Lit("\n"));
    if (split.lhs.size == 0)
      break;
    inc_walker = split.rhs;

    // NOTE: Trim the whitespace, mainly for windows, the file we read will have \r\n whereas we just want to emit \n
    DN_Str8 line = DN_Str8TrimTailWhitespace(split.lhs);

    // NOTE: Comment out any #include "../dn_.*" matches if we encounter one
    DN_Str8FindResult find = DN_Str8FindStr8(line, DN_Str8Lit("#include \"../dn_"), DN_Str8EqCase_Sensitive);
    {
      if (find.found) {
        line = DN_Str8FromTLSF("%S// DN: Single header generator commented out this header => %S", find.start_to_before_match, DN_Str8TrimWhitespaceAround(find.match_to_end_of_buffer));

        // The only time we use '../dn_.*' is for LSP purposes, so we
        // don't care about inlining it, hence we don't set 'include_file'
      }
    }

    // NOTE: Inline any other relative includes if we encounter one
    // (Right now DN only includes stb_sprintf with a relative path)
    DN_Str8 extra_include_path = {};
    if (!find.found) {
      find = DN_Str8FindStr8(line, DN_Str8Lit("#include \""), DN_Str8EqCase_Sensitive);
      if (find.found) {
        line                     = DN_Str8FromTLSF("%S// DN: Single header generator commented out this header => %S", find.start_to_before_match, DN_Str8TrimWhitespaceAround(find.match_to_end_of_buffer));
        DN_Str8 rel_include_path = DN_Str8TrimWhitespaceAround(find.after_match_to_end_of_buffer);
        DN_Str8 root_dir         = DN_Str8FileDirectoryFromPath(cpp_path);
        extra_include_path       = DN_OS_PathFFromTLS("%S/%S", root_dir, DN_Str8TrimSuffix(rel_include_path, DN_Str8Lit("\"")));
      }
    }

    DN_Str8BuilderAppendRef(dest, line);
    DN_Str8BuilderAppendRef(dest, DN_Str8Lit("\n"));

    if (extra_include_path.size)
      AppendCppFileLineByLine(dest, extra_include_path);
  }
}

int main(int argc, char **argv)
{
  DN_Core dn = {};
  DN_Init(&dn, DN_InitFlags_OS, nullptr);

  if (argc != 3) {
    DN_OS_PrintErrF("USAGE: %s <path/to/dn/Source> <output_dir>", argv[0]);
    return -1;
  }

  DN_Str8 dn_root_dir = DN_Str8FromCStr8(argv[1]);
  DN_Str8 output_dir  = DN_Str8FromCStr8(argv[2]);
  if (!DN_OS_PathMakeDir(output_dir)) {
    DN_OS_PrintErrF("Failed to make requested output directory: %S", output_dir);
    return -1;
  }

  File const FILES[] = {
      {FileType_Header, DN_Str8Lit("dn.h")},
      {FileType_Impl, DN_Str8Lit("dn.cpp")},
  };

  for (DN_ForIndexU(type, FileType_Count)) {
    DN_OSTLSTMem   tmem    = DN_OS_TLSPushTMem(nullptr);
    DN_Str8Builder builder = DN_Str8BuilderFromTLS();
    for (DN_ForItCArray(it, File const, FILES)) {
      if (it.data->type != type)
        continue;

      // NOTE: Parse the include files in the *_inc.[h|cpp] files
      DN_Str8 path = DN_OS_PathFFromTLS("%S/%S", dn_root_dir, it.data->file_name);
      {
        DN_OSErrSink *err         = DN_OS_ErrSinkBeginDefault();
        DN_Str8       file_buffer = DN_OS_FileReadAllTLS(path, err);
        DN_OS_ErrSinkEndAndExitIfErrorF(err, -1, "Failed to load file");

        // NOTE: Walk the top-level dn_*_inc.[h|cpp] files
        for (DN_Str8 walker = file_buffer;;) {
          DN_Str8BSplitResult split = DN_Str8BSplit(walker, DN_Str8Lit("\n"));
          if (split.lhs.size == 0)
            break;

          // NOTE: Parse the line, if it was a #include, extract it into this string
          DN_Str8 include_file = {};
          {
            walker       = split.rhs;
            DN_Str8 line = DN_Str8TrimTailWhitespace(split.lhs);

            // NOTE: Comment out any #include "dn_.*" matches if we encounter one
            DN_Str8FindResult find = DN_Str8FindStr8(line, DN_Str8Lit("#include \""), DN_Str8EqCase_Sensitive);
            {
              if (find.found && DN_Str8FindStr8(line, DN_Str8Lit("dn_"), DN_Str8EqCase_Sensitive).found) {
                line         = DN_Str8FromTLSF("%S// DN: Single header generator inlined this file => %S", find.start_to_before_match, DN_Str8TrimWhitespaceAround(find.match_to_end_of_buffer));
                include_file = DN_Str8BSplit(find.after_match_to_end_of_buffer, DN_Str8Lit("\"")).lhs;
                DN_Assert(include_file.size);
              }
            }

            // NOTE: Record the line
            DN_Str8BuilderAppendRef(&builder, line);
            DN_Str8BuilderAppendRef(&builder, DN_Str8Lit("\n"));
          }

          if (include_file.size) { // NOTE: If the line was a include file, we will inline the included file
            DN_Str8 include_path = DN_OS_PathFFromTLS("%S/%S", dn_root_dir, include_file);
            AppendCppFileLineByLine(&builder, include_path);
          }
        }
      }
    }

    DN_Str8 extra_files[] = {
        DN_Str8Lit("Extra/dn_math"),
        DN_Str8Lit("Extra/dn_async"),
        DN_Str8Lit("Extra/dn_bin_pack"),
        DN_Str8Lit("Extra/dn_csv"),
        DN_Str8Lit("Extra/dn_hash"),
        DN_Str8Lit("Extra/dn_helpers"),
    };
    DN_Str8 suffix = type == FileType_Header ? DN_Str8Lit("h") : DN_Str8Lit("cpp");
    for (DN_ForItCArray(extra_it, DN_Str8, extra_files)) {
      DN_Str8 extra_path = DN_OS_PathFFromTLS("%S/%S.%S", dn_root_dir, *extra_it.data, suffix);
      AppendCppFileLineByLine(&builder, extra_path);
    }

    DN_Date date = DN_OS_DateLocalTimeNow();
    DN_Str8BuilderPrependF(&builder, "// Generated by the DN single header generator %04u-%02u-%02u %02u:%02u:%02u\n\n", date.year, date.month, date.day, date.hour, date.minutes, date.seconds);

    DN_Str8       buffer             = DN_Str8TrimWhitespaceAround(DN_Str8BuilderBuildFromTLS(&builder));
    DN_Str8       single_header_path = DN_OS_PathFFromTLS("%S/dn_single_header.%S", output_dir, suffix);
    DN_OSErrSink *err                = DN_OS_ErrSinkBeginDefault();
    DN_OS_FileWriteAllSafe(single_header_path, buffer, err);
    DN_OS_ErrSinkEndAndExitIfErrorF(err, -1, "Failed to write Single header file '%S'", single_header_path);
  }
}
