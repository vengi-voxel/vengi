/**
 * @file
 */

#include "video/Trace.h"

namespace video {

TraceGLScoped::TraceGLScoped(const char* name, const char *msg) {
	traceGLBegin(name);
	core::traceMessage(msg);
}

TraceGLScoped::~TraceGLScoped() {
	traceGLEnd();
}

void traceGLFrameEnd() {
#ifdef TRACY_ENABLE
	TracyGpuCollect;
#endif
}

void traceGLInit() {
#ifdef TRACY_ENABLE
	TracyGpuContext;
#endif

}

void traceGLShutdown() {
}

void traceGLBegin(const char* name) {
	core::traceBegin(name);
}

void traceGLEnd() {
	core::traceEnd();
}

}
