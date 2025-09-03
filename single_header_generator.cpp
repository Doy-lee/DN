#define USE_SINGLE_HEADER 1

#if USE_SINGLE_HEADER
#include "Single_Header/dn_single_header.h"
#else
#include "Source/dn_base_inc.h"
#include "Source/dn_os_inc.h"
#include "Source/dn_core_inc.h"
#endif

#if USE_SINGLE_HEADER
#include "Single_Header/dn_single_header.cpp"
#else
#include "Source/dn_base_inc.cpp"
#include "Source/dn_os_inc.cpp"
#include "Source/dn_core_inc.cpp"
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
  DN_Str8       buffer = DN_OS_ReadAllFromTLS(cpp_path, err);
  DN_OS_ErrSinkEndAndExitIfErrorF(err, -1, "Failed to load file from '%S' for appending", cpp_path);

  for (DN_Str8 inc_walker = buffer;;) {
    DN_Str8BSplitResult split = DN_Str8_BSplit(inc_walker, DN_STR8("\n"));
    if (!DN_Str8_HasData(split.lhs))
      break;
    inc_walker = split.rhs;

    // NOTE: Trim the whitespace, mainly for windows, the file we read will have \r\n whereas we just want to emit \n
    DN_Str8 line = DN_Str8_TrimTailWhitespace(split.lhs);

    // NOTE: Comment out any #include "../dn_.*" matches if we encounter one
    DN_Str8FindResult find = DN_Str8_FindStr8(line, DN_STR8("#include \"../dn_"), DN_Str8EqCase_Sensitive);
    {
      if (find.found) {
        line = DN_Str8_FromTLSF("%S// DN: Single header generator commented out this header => %S", find.start_to_before_match, DN_Str8_TrimWhitespaceAround(find.match_to_end_of_buffer));

        // The only time we use '../dn_.*' is for LSP purposes, so we
        // don't care about inlining it, hence we don't set 'include_file'
      }
    }

    // NOTE: Inline any other relative includes if we encounter one
    // (Right now DN only includes stb_sprintf with a relative path)
    DN_Str8 extra_include_path = {};
    if (!find.found) {
      find = DN_Str8_FindStr8(line, DN_STR8("#include \""), DN_Str8EqCase_Sensitive);
      if (find.found) {
        line                     = DN_Str8_FromTLSF("%S// DN: Single header generator commented out this header => %S", find.start_to_before_match, DN_Str8_TrimWhitespaceAround(find.match_to_end_of_buffer));
        DN_Str8 rel_include_path = DN_Str8_TrimWhitespaceAround(find.after_match_to_end_of_buffer);
        DN_Str8 root_dir         = DN_Str8_FileDirectoryFromPath(cpp_path);
        extra_include_path       = DN_OS_PathFFromTLS("%S/%S", root_dir, DN_Str8_TrimSuffix(rel_include_path, DN_STR8("\"")));
      }
    }

    DN_Str8Builder_AppendRef(dest, line);
    DN_Str8Builder_AppendRef(dest, DN_STR8("\n"));

    if (extra_include_path.size)
      AppendCppFileLineByLine(dest, extra_include_path);
  }
}

int main(int argc, char **argv)
{
  DN_Core   dn      = {};
  DN_OSCore os      = {};
  DN_OS_Init(&os, nullptr);
  DN_Core_Init(&dn, DN_CoreOnInit_Nil);

  if (argc != 3) {
    DN_OS_PrintErrF("USAGE: %s <path/to/dn/Source> <output_dir>", argv[0]);
    return -1;
  }

  DN_Str8 dn_root_dir = DN_Str8_FromCStr8(argv[1]);
  DN_Str8 output_dir  = DN_Str8_FromCStr8(argv[2]);
  if (!DN_OS_MakeDir(output_dir)) {
    DN_OS_PrintErrF("Failed to make requested output directory: %S", output_dir);
    return -1;
  }

  File const FILES[] = {
      {FileType_Header, DN_STR8("dn_base_inc.h")},
      {FileType_Header, DN_STR8("dn_os_inc.h")},
      {FileType_Header, DN_STR8("dn_core_inc.h")},
      {FileType_Impl, DN_STR8("dn_base_inc.cpp")},
      {FileType_Impl, DN_STR8("dn_os_inc.cpp")},
      {FileType_Impl, DN_STR8("dn_core_inc.cpp")},
  };

  for (DN_ForIndexU(type, FileType_Count)) {
    DN_OSTLSTMem   tmem    = DN_OS_TLSPushTMem(nullptr);
    DN_Str8Builder builder = DN_Str8Builder_FromTLS();
    for (DN_ForItCArray(it, File const, FILES)) {
      if (it.data->type != type)
        continue;

      // NOTE: Parse the include files in the *_inc.[h|cpp] files
      DN_Str8 path = DN_OS_PathFFromTLS("%S/%S", dn_root_dir, it.data->file_name);
      {
        DN_OSErrSink *err         = DN_OS_ErrSinkBeginDefault();
        DN_Str8       file_buffer = DN_OS_ReadAllFromTLS(path, err);
        DN_OS_ErrSinkEndAndExitIfErrorF(err, -1, "Failed to load file");

        // NOTE: Walk the top-level dn_*_inc.[h|cpp] files
        for (DN_Str8 walker = file_buffer;;) {
          DN_Str8BSplitResult split = DN_Str8_BSplit(walker, DN_STR8("\n"));
          if (!DN_Str8_HasData(split.lhs))
            break;

          // NOTE: Parse the line, if it was a #include, extract it into this string
          DN_Str8 include_file = {};
          {
            walker       = split.rhs;
            DN_Str8 line = DN_Str8_TrimTailWhitespace(split.lhs);

            // NOTE: Comment out any #include "dn_.*" matches if we encounter one
            DN_Str8FindResult find = DN_Str8_FindStr8(line, DN_STR8("#include \""), DN_Str8EqCase_Sensitive);
            {
              if (find.found && DN_Str8_FindStr8(line, DN_STR8("dn_"), DN_Str8EqCase_Sensitive).found) {
                line         = DN_Str8_FromTLSF("%S// DN: Single header generator inlined this file => %S", find.start_to_before_match, DN_Str8_TrimWhitespaceAround(find.match_to_end_of_buffer));
                include_file = DN_Str8_BSplit(find.after_match_to_end_of_buffer, DN_STR8("\"")).lhs;
                DN_Assert(include_file.size);
              }
            }

            // NOTE: Record the line
            DN_Str8Builder_AppendRef(&builder, line);
            DN_Str8Builder_AppendRef(&builder, DN_STR8("\n"));
          }

          if (include_file.size) { // NOTE: If the line was a include file, we will inline the included file
            DN_Str8 include_path = DN_OS_PathFFromTLS("%S/%S", dn_root_dir, include_file);
            AppendCppFileLineByLine(&builder, include_path);
          }
        }
      }
    }

    DN_Str8 extra_files[] = {
        DN_STR8("Extra/dn_math"),
        DN_STR8("Extra/dn_async"),
        DN_STR8("Extra/dn_bin_pack"),
        DN_STR8("Extra/dn_csv"),
        DN_STR8("Extra/dn_hash"),
        DN_STR8("Extra/dn_helpers"),
    };
    DN_Str8 suffix = type == FileType_Header ? DN_STR8("h") : DN_STR8("cpp");
    for (DN_ForItCArray(extra_it, DN_Str8, extra_files)) {
      DN_Str8 extra_path = DN_OS_PathFFromTLS("%S/%S.%S", dn_root_dir, *extra_it.data, suffix);
      AppendCppFileLineByLine(&builder, extra_path);
    }

    DN_OSDateTime date = DN_OS_DateLocalTimeNow();
    DN_Str8Builder_PrependF(&builder, "// Generated by the DN single header generator %04u-%02u-%02u %02u:%02u:%02u\n\n", date.year, date.month, date.day, date.hour, date.minutes, date.seconds);

    DN_Str8       buffer             = DN_Str8_TrimWhitespaceAround(DN_Str8Builder_BuildFromTLS(&builder));
    DN_Str8       single_header_path = DN_OS_PathFFromTLS("%S/dn_single_header.%S", output_dir, suffix);
    DN_OSErrSink *err                = DN_OS_ErrSinkBeginDefault();
    DN_OS_WriteAllSafe(single_header_path, buffer, err);
    DN_OS_ErrSinkEndAndExitIfErrorF(err, -1, "Failed to write Single header file '%S'", single_header_path);
  }
}
