#define DN_BASE_LOG_CPP

#include "../dn_base_inc.h"

static DN_LOGEmitFromTypeFVFunc *g_dn_base_log_emit_from_type_fv_func_;
static void                     *g_dn_base_log_emit_from_type_fv_user_context_;

DN_API DN_Str8 DN_LOG_ColourEscapeCodeStr8FromRGB(DN_LOGColourType colour, DN_U8 r, DN_U8 g, DN_U8 b)
{
  DN_THREAD_LOCAL char buffer[32];
  buffer[0]      = 0;
  DN_Str8 result = {};
  result.size    = DN_SNPrintF(buffer,
                            DN_ArrayCountU(buffer),
                            "\x1b[%d;2;%u;%u;%um",
                            colour == DN_LOGColourType_Fg ? 38 : 48,
                            r,
                            g,
                            b);
  result.data    = buffer;
  return result;
}

DN_API DN_Str8 DN_LOG_ColourEscapeCodeStr8FromU32(DN_LOGColourType colour, DN_U32 value)
{
  DN_U8   r      = DN_CAST(DN_U8)(value >> 24);
  DN_U8   g      = DN_CAST(DN_U8)(value >> 16);
  DN_U8   b      = DN_CAST(DN_U8)(value >> 8);
  DN_Str8 result = DN_LOG_ColourEscapeCodeStr8FromRGB(colour, r, g, b);
  return result;
}

DN_API DN_LOGPrefixSize DN_LOG_MakePrefix(DN_LOGStyle style, DN_LOGTypeParam type, DN_CallSite call_site, DN_LOGDate date, char *dest, DN_USize dest_size)
{
  DN_Str8 type_str8 = type.str8;
  if (type.is_u32_enum) {
    switch (type.u32) {
      case DN_LOGType_Debug:   type_str8 = DN_STR8("DEBUG"); break;
      case DN_LOGType_Info:    type_str8 = DN_STR8("INFO "); break;
      case DN_LOGType_Warning: type_str8 = DN_STR8("WARN");  break;
      case DN_LOGType_Error:   type_str8 = DN_STR8("ERROR"); break;
      case DN_LOGType_Count:   type_str8 = DN_STR8("BADXX"); break;
    }
  }

  static DN_USize max_type_length = 0;
  max_type_length                 = DN_Max(max_type_length, type_str8.size);
  int type_padding                = DN_CAST(int)(max_type_length - type_str8.size);

  DN_Str8 colour_esc = {};
  DN_Str8 bold_esc   = {};
  DN_Str8 reset_esc  = {};
  if (style.colour) {
    bold_esc   = DN_STR8(DN_LOG_BoldEscapeCode);
    reset_esc  = DN_STR8(DN_LOG_ResetEscapeCode);
    colour_esc = DN_LOG_ColourEscapeCodeStr8FromRGB(DN_LOGColourType_Fg, style.r, style.g, style.b);
  }

  DN_Str8 file_name = DN_Str8_FileNameFromPath(call_site.file);
  DN_GCC_WARNING_PUSH
  DN_GCC_WARNING_DISABLE(-Wformat)
  DN_GCC_WARNING_DISABLE(-Wformat-extra-args)
  DN_MSVC_WARNING_PUSH
  DN_MSVC_WARNING_DISABLE(4477)
  int     size      = DN_SNPrintF(dest,
                         DN_CAST(int)dest_size,
                         "%04u-%02u-%02uT%02u:%02u:%02u" // date
                         "%S"                            // colour
                         "%S"                            // bold
                         " %S"                           // type
                         "%.*s"                          // type padding
                         "%S"                            // reset
                         " %S"                           // file name
                         ":%05I32u "                     // line number
                         ,
                         date.year,
                         date.month,
                         date.day,
                         date.hour,
                         date.minute,
                         date.second,
                         colour_esc,      // colour
                         bold_esc,        // bold
                         type_str8,       // type
                         DN_CAST(int) type_padding,
                         "",              // type padding
                         reset_esc,       // reset
                         file_name,       // file name
                         call_site.line); // line number
  DN_MSVC_WARNING_POP // '%S' requires an argument of type 'wchar_t *', but variadic argument 7 has type 'DN_Str8'
  DN_GCC_WARNING_POP

  static DN_USize max_header_length  = 0;
  DN_USize        size_no_ansi_codes = size - colour_esc.size - reset_esc.size - bold_esc.size;
  max_header_length                  = DN_Max(max_header_length, size_no_ansi_codes);
  DN_USize header_padding            = max_header_length - size_no_ansi_codes;

  DN_LOGPrefixSize result = {};
  result.size             = size;
  result.padding          = header_padding;
  return result;
}

DN_API void DN_LOG_SetEmitFromTypeFVFunc(DN_LOGEmitFromTypeFVFunc *print_func, void *user_data)
{
  g_dn_base_log_emit_from_type_fv_func_         = print_func;
  g_dn_base_log_emit_from_type_fv_user_context_ = user_data;
}

DN_API void DN_LOG_EmitFromType(DN_LOGTypeParam type, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, ...)
{
  DN_LOGEmitFromTypeFVFunc *func         = g_dn_base_log_emit_from_type_fv_func_;
  void                     *user_context = g_dn_base_log_emit_from_type_fv_user_context_;
  if (func) {
    va_list args;
    va_start(args, fmt);
    func(type, user_context, call_site, fmt, args);
    va_end(args);
  }
}

DN_API DN_LOGTypeParam DN_LOG_MakeU32LogTypeParam(DN_LOGType type)
{
  DN_LOGTypeParam result = {};
  result.is_u32_enum     = true;
  result.u32             = type;
  return result;
}
