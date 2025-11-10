#if !defined(DN_OS_PRINT_H)
#define DN_OS_PRINT_H

enum DN_OSPrintDest
{
  DN_OSPrintDest_Out,
  DN_OSPrintDest_Err,
};

// NOTE: Print Macros
#define DN_OS_PrintOut(string)                       DN_OS_Print(DN_OSPrintDest_Out, string)
#define DN_OS_PrintOutF(fmt, ...)                    DN_OS_PrintF(DN_OSPrintDest_Out, fmt, ##__VA_ARGS__)
#define DN_OS_PrintOutFV(fmt, args)                  DN_OS_PrintFV(DN_OSPrintDest_Out, fmt, args)

#define DN_OS_PrintOutStyle(style, string)           DN_OS_PrintStyle(DN_OSPrintDest_Out, style, string)
#define DN_OS_PrintOutFStyle(style, fmt, ...)        DN_OS_PrintFStyle(DN_OSPrintDest_Out, style, fmt, ##__VA_ARGS__)
#define DN_OS_PrintOutFVStyle(style, fmt, args, ...) DN_OS_PrintFVStyle(DN_OSPrintDest_Out, style, fmt, args)

#define DN_OS_PrintOutLn(string)                     DN_OS_PrintLn(DN_OSPrintDest_Out, string)
#define DN_OS_PrintOutLnF(fmt, ...)                  DN_OS_PrintLnF(DN_OSPrintDest_Out, fmt, ##__VA_ARGS__)
#define DN_OS_PrintOutLnFV(fmt, args)                DN_OS_PrintLnFV(DN_OSPrintDest_Out, fmt, args)

#define DN_OS_PrintOutLnStyle(style, string)         DN_OS_PrintLnStyle(DN_OSPrintDest_Out, style, string);
#define DN_OS_PrintOutLnFStyle(style, fmt, ...)      DN_OS_PrintLnFStyle(DN_OSPrintDest_Out, style, fmt, ##__VA_ARGS__)
#define DN_OS_PrintOutLnFVStyle(style, fmt, args)    DN_OS_PrintLnFVStyle(DN_OSPrintDest_Out, style, fmt, args);

#define DN_OS_PrintErr(string)                       DN_OS_Print(DN_OSPrintDest_Err, string)
#define DN_OS_PrintErrF(fmt, ...)                    DN_OS_PrintF(DN_OSPrintDest_Err, fmt, ##__VA_ARGS__)
#define DN_OS_PrintErrFV(fmt, args)                  DN_OS_PrintFV(DN_OSPrintDest_Err, fmt, args)

#define DN_OS_PrintErrStyle(style, string)           DN_OS_PrintStyle(DN_OSPrintDest_Err, style, string)
#define DN_OS_PrintErrFStyle(style, fmt, ...)        DN_OS_PrintFStyle(DN_OSPrintDest_Err, style, fmt, ##__VA_ARGS__)
#define DN_OS_PrintErrFVStyle(style, fmt, args, ...) DN_OS_PrintFVStyle(DN_OSPrintDest_Err, style, fmt, args)

#define DN_OS_PrintErrLn(string)                     DN_OS_PrintLn(DN_OSPrintDest_Err, string)
#define DN_OS_PrintErrLnF(fmt, ...)                  DN_OS_PrintLnF(DN_OSPrintDest_Err, fmt, ##__VA_ARGS__)
#define DN_OS_PrintErrLnFV(fmt, args)                DN_OS_PrintLnFV(DN_OSPrintDest_Err, fmt, args)

#define DN_OS_PrintErrLnStyle(style, string)         DN_OS_PrintLnStyle(DN_OSPrintDest_Err, style, string);
#define DN_OS_PrintErrLnFStyle(style, fmt, ...)      DN_OS_PrintLnFStyle(DN_OSPrintDest_Err, style, fmt, ##__VA_ARGS__)
#define DN_OS_PrintErrLnFVStyle(style, fmt, args)    DN_OS_PrintLnFVStyle(DN_OSPrintDest_Err, style, fmt, args);

// NOTE: Print
DN_API void DN_OS_Print                (DN_OSPrintDest dest, DN_Str8 string);
DN_API void DN_OS_PrintF               (DN_OSPrintDest dest, DN_FMT_ATTRIB char const *fmt, ...);
DN_API void DN_OS_PrintFV              (DN_OSPrintDest dest, DN_FMT_ATTRIB char const *fmt, va_list args);

DN_API void DN_OS_PrintStyle           (DN_OSPrintDest dest, DN_LOGStyle style, DN_Str8 string);
DN_API void DN_OS_PrintFStyle          (DN_OSPrintDest dest, DN_LOGStyle style, DN_FMT_ATTRIB char const *fmt, ...);
DN_API void DN_OS_PrintFVStyle         (DN_OSPrintDest dest, DN_LOGStyle style, DN_FMT_ATTRIB char const *fmt, va_list args);

DN_API void DN_OS_PrintLn              (DN_OSPrintDest dest, DN_Str8 string);
DN_API void DN_OS_PrintLnF             (DN_OSPrintDest dest, DN_FMT_ATTRIB char const *fmt, ...);
DN_API void DN_OS_PrintLnFV            (DN_OSPrintDest dest, DN_FMT_ATTRIB char const *fmt, va_list args);

DN_API void DN_OS_PrintLnStyle         (DN_OSPrintDest dest, DN_LOGStyle style, DN_Str8 string);
DN_API void DN_OS_PrintLnFStyle        (DN_OSPrintDest dest, DN_LOGStyle style, DN_FMT_ATTRIB char const *fmt, ...);
DN_API void DN_OS_PrintLnFVStyle       (DN_OSPrintDest dest, DN_LOGStyle style, DN_FMT_ATTRIB char const *fmt, va_list args);
#endif // !defined(DN_OS_PRINT_H)
