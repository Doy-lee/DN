#if !defined(DN_JSON_BUILDER_H)
#define DN_JSON_BUILDER_H

#include <stdarg.h>

#if !defined(__cplusplus)
  #include <stdbool.h>
#endif

#if !defined(DN_JSB_VSNPrintF)
  #include <stdio.h>
  #define DN_JSB_VSNPrintF(buf, size, fmt, args) vsnprintf(buf, size, fmt, args)
#endif

typedef enum DN_JSBResult {
  DN_JSBResult_Success,
  DN_JSBResult_BadAPIUsage,
  DN_JSBResult_DepthArrayOutOfMem,
  DN_JSBResult_BufOutOfMem,
} DN_JSBResult;

typedef enum DN_JSBPrettify {
  DN_JSBPrettify_No,
  DN_JSBPrettify_Yes,
} DN_JSBPrettify;

typedef enum DN_JSBDepthType {
  DN_JSBDepthType_Nil,
  DN_JSBDepthType_Object,
  DN_JSBDepthType_Array,
} DN_JSBDepthType;

typedef enum DN_JSBAction {
  DN_JSBAction_Nil,
  DN_JSBAction_PushObject,
  DN_JSBAction_PopObject,
  DN_JSBAction_PushArray,
  DN_JSBAction_PopArray,
  DN_JSBAction_Key,
  DN_JSBAction_Value,
} DN_JSBAction;

typedef struct DN_JSBDepth {
  DN_JSBDepthType type;
  DN_JSBAction    last_action;
} DN_JSBDepth;

typedef struct DN_JSBBuilder {
  DN_JSBDepth*  depth;
  size_t         depth_count;
  size_t         depth_max;
  size_t         p; // Size required not incl- the null-terminator (e.g. To build, pass in `p + 1` bytes)
  DN_JSBResult   error;
  DN_JSBPrettify pretty;
} DN_JSBBuilder;

DN_JSBResult  DN_JSB_PushObject       (DN_JSBBuilder *builder, char *buf, size_t buf_size);
DN_JSBResult  DN_JSB_PopObject        (DN_JSBBuilder *builder, char *buf, size_t buf_size);
DN_JSBResult  DN_JSB_PushArray        (DN_JSBBuilder *builder, char *buf, size_t buf_size);
DN_JSBResult  DN_JSB_PopArray         (DN_JSBBuilder *builder, char *buf, size_t buf_size);
DN_JSBResult  DN_JSB_KeyF             (DN_JSBBuilder *builder, char *buf, size_t buf_size, char const *fmt, ...);
DN_JSBResult  DN_JSB_ValueStringF     (DN_JSBBuilder *builder, char *buf, size_t buf_size, char const *fmt, ...);
DN_JSBResult  DN_JSB_ValueBool        (DN_JSBBuilder *builder, char *buf, size_t buf_size, bool flag);
DN_JSBResult  DN_JSB_ValueNull        (DN_JSBBuilder *builder, char *buf, size_t buf_size);
DN_JSBResult  DN_JSB_ValueNumber      (DN_JSBBuilder *builder, char *buf, size_t buf_size, double number);

DN_JSBDepth * DN_JSB_PeekDepth_       (DN_JSBBuilder *builder, size_t offset);
bool          DN_JSB_PushDepth_       (DN_JSBBuilder *builder, DN_JSBDepthType type);
DN_JSBResult  DN_JSB_AppendFV_        (DN_JSBBuilder *builder, char *buf, size_t buf_size, char const *fmt, va_list args);
DN_JSBResult  DN_JSB_AppendF_         (DN_JSBBuilder *builder, char *buf, size_t buf_size, char const *fmt, ...);
DN_JSBResult  DN_JSB_BeginWriteValue_ (DN_JSBBuilder *builder, char *buf, size_t buf_size);
void          DN_JSB_FinishWriteValue_(DN_JSBBuilder *builder, DN_JSBResult last_value_raw_str_result);
#endif // !defined(DN_JSON_BUILDER_H)
