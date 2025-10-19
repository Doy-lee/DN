#define DN_OS_PRINT_CPP

DN_API DN_LOGStyle DN_OS_PrintStyleColour(uint8_t r, uint8_t g, uint8_t b, DN_LOGBold bold)
{
  DN_LOGStyle result = {};
  result.bold          = bold;
  result.colour        = true;
  result.r             = r;
  result.g             = g;
  result.b             = b;
  return result;
}

DN_API DN_LOGStyle DN_OS_PrintStyleColourU32(uint32_t rgb, DN_LOGBold bold)
{
  uint8_t       r      = (rgb >> 24) & 0xFF;
  uint8_t       g      = (rgb >> 16) & 0xFF;
  uint8_t       b      = (rgb >> 8) & 0xFF;
  DN_LOGStyle result = DN_OS_PrintStyleColour(r, g, b, bold);
  return result;
}

DN_API DN_LOGStyle DN_OS_PrintStyleBold()
{
  DN_LOGStyle result = {};
  result.bold          = DN_LOGBold_Yes;
  return result;
}

DN_API void DN_OS_Print(DN_OSPrintDest dest, DN_Str8 string)
{
  DN_Assert(dest == DN_OSPrintDest_Out || dest == DN_OSPrintDest_Err);

#if defined(DN_PLATFORM_WIN32)
  // NOTE: Get the output handles from kernel ////////////////////////////////////////////////////
  DN_THREAD_LOCAL void *std_out_print_handle     = nullptr;
  DN_THREAD_LOCAL void *std_err_print_handle     = nullptr;
  DN_THREAD_LOCAL bool  std_out_print_to_console = false;
  DN_THREAD_LOCAL bool  std_err_print_to_console = false;

  if (!std_out_print_handle) {
    unsigned long mode = 0;
    (void)mode;
    std_out_print_handle     = GetStdHandle(STD_OUTPUT_HANDLE);
    std_out_print_to_console = GetConsoleMode(std_out_print_handle, &mode) != 0;

    std_err_print_handle     = GetStdHandle(STD_ERROR_HANDLE);
    std_err_print_to_console = GetConsoleMode(std_err_print_handle, &mode) != 0;
  }

  // NOTE: Select the output handle //////////////////////////////////////////////////////////////
  void *print_handle     = std_out_print_handle;
  bool  print_to_console = std_out_print_to_console;
  if (dest == DN_OSPrintDest_Err) {
    print_handle     = std_err_print_handle;
    print_to_console = std_err_print_to_console;
  }

  // NOTE: Write the string //////////////////////////////////////////////////////////////////////
  DN_Assert(string.size < DN_Cast(unsigned long) - 1);
  unsigned long bytes_written = 0;
  (void)bytes_written;
  if (print_to_console)
    WriteConsoleA(print_handle, string.data, DN_Cast(unsigned long) string.size, &bytes_written, nullptr);
  else
    WriteFile(print_handle, string.data, DN_Cast(unsigned long) string.size, &bytes_written, nullptr);
#else
  fprintf(dest == DN_OSPrintDest_Out ? stdout : stderr, "%.*s", DN_Str8PrintFmt(string));
#endif
}

DN_API void DN_OS_PrintF(DN_OSPrintDest dest, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_OS_PrintFV(dest, fmt, args);
  va_end(args);
}

DN_API void DN_OS_PrintFStyle(DN_OSPrintDest dest, DN_LOGStyle style, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_OS_PrintFVStyle(dest, style, fmt, args);
  va_end(args);
}

DN_API void DN_OS_PrintStyle(DN_OSPrintDest dest, DN_LOGStyle style, DN_Str8 string)
{
  if (string.data && string.size) {
    if (style.colour)
      DN_OS_Print(dest, DN_LOG_ColourEscapeCodeStr8FromRGB(DN_LOGColourType_Fg, style.r, style.g, style.b));
    if (style.bold == DN_LOGBold_Yes)
      DN_OS_Print(dest, DN_Str8Lit(DN_LOG_BoldEscapeCode));
    DN_OS_Print(dest, string);
    if (style.colour || style.bold == DN_LOGBold_Yes)
      DN_OS_Print(dest, DN_Str8Lit(DN_LOG_ResetEscapeCode));
  }
}

static char *DN_OS_PrintVSPrintfChunker_(const char *buf, void *user, int len)
{
  DN_Str8 string = {};
  string.data    = DN_Cast(char *) buf;
  string.size    = len;

  DN_OSPrintDest dest = DN_Cast(DN_OSPrintDest) DN_Cast(uintptr_t) user;
  DN_OS_Print(dest, string);
  return (char *)buf;
}

DN_API void DN_OS_PrintFV(DN_OSPrintDest dest, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  char buffer[STB_SPRINTF_MIN];
  STB_SPRINTF_DECORATE(vsprintfcb)
  (DN_OS_PrintVSPrintfChunker_, DN_Cast(void *) DN_Cast(uintptr_t) dest, buffer, fmt, args);
}

DN_API void DN_OS_PrintFVStyle(DN_OSPrintDest dest, DN_LOGStyle style, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  if (fmt) {
    if (style.colour)
      DN_OS_Print(dest, DN_LOG_ColourEscapeCodeStr8FromRGB(DN_LOGColourType_Fg, style.r, style.g, style.b));
    if (style.bold == DN_LOGBold_Yes)
      DN_OS_Print(dest, DN_Str8Lit(DN_LOG_BoldEscapeCode));
    DN_OS_PrintFV(dest, fmt, args);
    if (style.colour || style.bold == DN_LOGBold_Yes)
      DN_OS_Print(dest, DN_Str8Lit(DN_LOG_ResetEscapeCode));
  }
}

DN_API void DN_OS_PrintLn(DN_OSPrintDest dest, DN_Str8 string)
{
  DN_OS_Print(dest, string);
  DN_OS_Print(dest, DN_Str8Lit("\n"));
}

DN_API void DN_OS_PrintLnF(DN_OSPrintDest dest, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_OS_PrintLnFV(dest, fmt, args);
  va_end(args);
}

DN_API void DN_OS_PrintLnFV(DN_OSPrintDest dest, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_OS_PrintFV(dest, fmt, args);
  DN_OS_Print(dest, DN_Str8Lit("\n"));
}

DN_API void DN_OS_PrintLnStyle(DN_OSPrintDest dest, DN_LOGStyle style, DN_Str8 string)
{
  DN_OS_PrintStyle(dest, style, string);
  DN_OS_Print(dest, DN_Str8Lit("\n"));
}

DN_API void DN_OS_PrintLnFStyle(DN_OSPrintDest dest, DN_LOGStyle style, DN_FMT_ATTRIB char const *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  DN_OS_PrintLnFVStyle(dest, style, fmt, args);
  va_end(args);
}

DN_API void DN_OS_PrintLnFVStyle(DN_OSPrintDest dest, DN_LOGStyle style, DN_FMT_ATTRIB char const *fmt, va_list args)
{
  DN_OS_PrintFVStyle(dest, style, fmt, args);
  DN_OS_Print(dest, DN_Str8Lit("\n"));
}
