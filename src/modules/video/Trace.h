/**
 * @file
 */

#pragma once

#include "core/Trace.h"
#ifdef TRACY_ENABLE
#include "video/gl/GLRenderer.h"
#include "core/tracy/TracyOpenGL.hpp"
#endif

namespace video {

class TraceGLScoped {
public:
	TraceGLScoped(const char* name, const char *msg = nullptr);
	~TraceGLScoped();
};

extern void traceGLInit();
extern void traceGLShutdown();
extern void traceGLBegin(const char* name);
extern void traceGLEnd();
extern void traceGLFrameEnd();

#define video_trace_init() video::traceGLInit()
#define video_trace_shutdown() video::traceGLShutdown()
#define video_trace_frame_end() video::traceGLFrameEnd()
#ifdef TRACY_ENABLE
#define video_trace_begin(name)
#define video_trace_begin_dynamic(name)
#define video_trace_end()
#define video_trace_scoped(name) TracyGpuNamedZone(__tracy_scoped_##name, #name, true)
#else
#define video_trace_begin(name) video::traceGLBegin(#name)
#define video_trace_begin_dynamic(name) video::traceGLBegin(#name)
#define video_trace_end() video::traceGLEnd()
#define video_trace_scoped(name) video::TraceGLScoped __trace__##name(#name)
#endif

}
