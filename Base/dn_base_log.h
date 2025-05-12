#if !defined(DN_BASE_LOG_H)
#define DN_BASE_LOG_H

enum DN_LOGType
{
  DN_LOGType_Debug,
  DN_LOGType_Info,
  DN_LOGType_Warning,
  DN_LOGType_Error,
  DN_LOGType_Count,
};

enum DN_LOGBold
{
  DN_LOGBold_No,
  DN_LOGBold_Yes,
};

struct DN_LOGStyle
{
  DN_LOGBold bold;
  bool       colour;
  DN_U8      r, g, b;
};

struct DN_LOGTypeParam
{
  bool    is_u32_enum;
  DN_U32  u32;
  DN_Str8 str8;
};

enum DN_LOGColourType
{
  DN_LOGColourType_Fg,
  DN_LOGColourType_Bg,
};

struct DN_LOGDate
{
  DN_U16 year;
  DN_U8  month;
  DN_U8  day;

  DN_U8 hour;
  DN_U8 minute;
  DN_U8 second;
};

struct DN_LOGPrefixSize
{
  DN_USize size;
  DN_USize padding;
};

typedef void DN_LOGEmitFromTypeFVFunc(DN_LOGTypeParam type, void *user_data, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, va_list args);

#define                 DN_LOG_ResetEscapeCode            "\x1b[0m"
#define                 DN_LOG_BoldEscapeCode             "\x1b[1m"
DN_API DN_Str8          DN_LOG_ColourEscapeCodeStr8FromRGB(DN_LOGColourType colour, DN_U8 r, DN_U8 g, DN_U8 b);
DN_API DN_Str8          DN_LOG_ColourEscapeCodeStr8FromU32(DN_LOGColourType colour, DN_U32 value);
DN_API DN_LOGPrefixSize DN_LOG_MakePrefix                 (DN_LOGStyle style, DN_LOGTypeParam type, DN_CallSite call_site, DN_LOGDate date, char *dest, DN_USize dest_size);
DN_API void             DN_LOG_SetEmitFromTypeFVFunc      (DN_LOGEmitFromTypeFVFunc *print_func, void *user_data);
DN_API void             DN_LOG_EmitFromType               (DN_LOGTypeParam type, DN_CallSite call_site, DN_FMT_ATTRIB char const *fmt, ...);
DN_API DN_LOGTypeParam  DN_LOG_MakeU32LogTypeParam        (DN_LOGType type);
#define                 DN_LOG_DebugF(fmt, ...)           DN_LOG_EmitFromType(DN_LOG_MakeU32LogTypeParam(DN_LOGType_Debug), DN_CALL_SITE, fmt, ##__VA_ARGS__)
#define                 DN_LOG_InfoF(fmt, ...)            DN_LOG_EmitFromType(DN_LOG_MakeU32LogTypeParam(DN_LOGType_Info), DN_CALL_SITE, fmt, ##__VA_ARGS__)
#define                 DN_LOG_WarningF(fmt, ...)         DN_LOG_EmitFromType(DN_LOG_MakeU32LogTypeParam(DN_LOGType_Warning), DN_CALL_SITE, fmt, ##__VA_ARGS__)
#define                 DN_LOG_ErrorF(fmt, ...)           DN_LOG_EmitFromType(DN_LOG_MakeU32LogTypeParam(DN_LOGType_Error), DN_CALL_SITE, fmt, ##__VA_ARGS__)


#endif // !defined(DN_BASE_LOG_H)
