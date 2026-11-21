/**
 * @file
 */

#include "video/Trace.h"
#include "core/Log.h"

namespace video {

VideoTraceScoped::VideoTraceScoped(const char* name, const char *msg) {
	traceVideoBegin(name);
	core::traceMessage(msg);
}

VideoTraceScoped::~VideoTraceScoped() {
	traceVideoEnd();
}

void traceVideoFrameEnd() {
#if defined(TRACY_ENABLE) && defined(USE_GL_RENDERER)
	if (tracy::GetGpuCtx().ptr != nullptr) {
		TracyGpuCollect;
	}
#endif
}

void traceVideoInit() {
#if defined(TRACY_ENABLE) && defined(USE_GL_RENDERER)
	TracyGpuContext;
	if (tracy::GetGpuCtx().ptr == nullptr) {
		Log::warn("TracyGpuContext failed; GPU zones disabled");
	}
#endif
}

void traceVideoShutdown() {
}

}
