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
#ifdef USE_GL_RENDERER
	TracyGpuCollect
#endif
#endif
}

void traceVideoInit() {
#ifdef TRACY_ENABLE
#ifdef USE_GL_RENDERER
	TracyGpuContext
#endif
	// USE_VK_RENDERER: Tracy Vulkan context init requires VkDevice/VkQueue/VkCommandBuffer.
	// Deferred until per-frame command buffer infrastructure is wired through.
#endif
}

void traceVideoShutdown() {
}

}
