#pragma once

#include "Log.h"
#include <SDL_platform.h>

#if USE_EMTRACE
#include <emscripten/trace.h>
#endif

#define RMT_ENABLED 0

// disable remotery via -DRMT_ENABLED=0
#include "Remotery.h"

namespace core {

// a singleton - available via core::App
class Trace {
private:
	Remotery* _remotery;
public:
	Trace(uint16_t port = 17815);
	~Trace();
};

class TraceScoped {
public:
	TraceScoped(const char* name, const char *msg = nullptr);
	~TraceScoped();
};

#if RMT_ENABLED > 0
#define core_trace_init() Log::info("Remotery active")
#define core_trace_begin_frame() core_trace_begin("Frame")
#define core_trace_end_frame() core_trace_end()
#define core_trace_begin(name) rmt_BeginCPUSample(name, 0)
#define core_trace_end() rmt_EndCPUSample()
#define core_trace_gl_begin(name) rmt_BeginOpenGLSample(name)
#define core_trace_gl_begin_dynamic(name) rmt_BeginOpenGLSampleDynamic(name)
#define core_trace_gl_end() rmt_EndOpenGLSample()
#define core_trace_msg(message) do { if (message != nullptr) rmt_LogText(message); } while (0)
#define core_trace_thread(name) rmt_SetCurrentThreadName(name)
#if RMT_USE_OPENGL
#define core_trace_gl_scoped(name) rmt_ScopedOpenGLSample(name)
#else
#define core_trace_gl_scoped(name) rmt_ScopedCPUSample(name, 0)
#endif
#define core_trace_scoped(name) rmt_ScopedCPUSample(name, 0)
#elif USE_EMTRACE
#define core_trace_init() Log::info("emtrace active")
#define core_trace_begin_frame() emscripten_trace_record_frame_start()
#define core_trace_end_frame() emscripten_trace_record_frame_end()
#define core_trace_begin(name) emscripten_trace_enter_context(#name)
#define core_trace_end() emscripten_trace_exit_context()
#define core_trace_gl_begin(name)
#define core_trace_gl_begin_dynamic(name)
#define core_trace_gl_end()
#define core_trace_msg(message)
#define core_trace_thread(name)
#define core_trace_gl_scoped(name)
#define core_trace_scoped(name) core::TraceScoped(#name)
#else
#define core_trace_init() Log::info("Tracing disabled")
#define core_trace_begin_frame()
#define core_trace_end_frame()
#define core_trace_begin(name)
#define core_trace_end()
#define core_trace_gl_begin(name)
#define core_trace_gl_begin_dynamic(name)
#define core_trace_gl_end()
#define core_trace_msg(message)
#define core_trace_thread(name)
#define core_trace_gl_scoped(name)
#define core_trace_scoped(name) core::TraceScoped(#name)
#endif


}
