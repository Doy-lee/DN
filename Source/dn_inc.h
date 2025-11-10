#if !defined(DN_INC_H)
#define DN_INC_H

struct DN_InitArgs
{
  DN_U64 os_tls_reserve;
  DN_U64 os_tls_commit;
  DN_U64 os_tls_err_sink_reserve;
  DN_U64 os_tls_err_sink_commit;
};

typedef DN_USize DN_InitFlags;
enum DN_InitFlags_
{
  DN_InitFlags_Nil            = 0,
  DN_InitFlags_OS             = 1 << 0,
  DN_InitFlags_OSLeakTracker  = 1 << 1,
  DN_InitFlags_LogLibFeatures = 1 << 2,
  DN_InitFlags_LogCPUFeatures = 1 << 3,
  DN_InitFlags_LogAllFeatures = DN_InitFlags_LogLibFeatures | DN_InitFlags_LogCPUFeatures,
};

struct DN_Core
{
  DN_InitFlags   init_flags;
  DN_USize       mem_allocs_frame;
  DN_LeakTracker leak;
  #if defined(DN_OS_H)
  DN_OSCore os;
  #endif
};

extern DN_Core *g_dn_;

DN_API void DN_Init(DN_Core *dn, DN_InitFlags flags, DN_InitArgs *args);
DN_API void DN_BeginFrame();

#endif // !defined(DN_INC_H)
