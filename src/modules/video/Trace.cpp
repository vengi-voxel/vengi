/**
 * @file
 */

#include "video/Trace.h"

namespace video {

VideoTraceScoped::VideoTraceScoped(const char* name, const char *msg) {
	traceVideoBegin(name);
	core::traceMessage(msg);
}

VideoTraceScoped::~VideoTraceScoped() {
	traceVideoEnd();
}

void traceVideoFrameEnd() {
#ifdef TRACY_ENABLE
	TracyGpuCollect;
#endif
}

void traceVideoInit() {
#ifdef TRACY_ENABLE
	TracyGpuContext;
#endif
}

void traceVideoShutdown() {
}

}
