/**
 * @file
 */

#pragma once

#include "core/Trace.h"
#include "engine-config.h"
#if defined(TRACY_ENABLE) && defined(USE_GL_RENDERER)
#include "video/gl/flextGL.h"
#include "core/tracy/public/tracy/TracyOpenGL.hpp"
#endif

namespace video {

class VideoTraceScoped {
public:
	VideoTraceScoped(const char* name, const char *msg = nullptr);
	~VideoTraceScoped();
};

extern void traceVideoInit();
extern void traceVideoShutdown();
extern void traceVideoBegin(const char* name);
extern void traceVideoEnd();
extern void traceVideoFrameEnd();

#define video_trace_init() video::traceVideoInit()
#define video_trace_shutdown() video::traceVideoShutdown()
#define video_trace_frame_end() video::traceVideoFrameEnd()
#if defined(TRACY_ENABLE) && defined(USE_GL_RENDERER)
#define video_trace_begin(name)
#define video_trace_begin_dynamic(name)
#define video_trace_end()
#define video_trace_scoped(name) TracyGpuNamedZone(__tracy_scoped_##name, #name, tracy::GetGpuCtx().ptr != nullptr)
#else
#define video_trace_begin(name) video::traceVideoBegin(#name)
#define video_trace_begin_dynamic(name) video::traceVideoBegin(#name)
#define video_trace_end() video::traceVideoEnd()
#define video_trace_scoped(name) video::VideoTraceScoped __trace__##name(#name)
#endif

}
