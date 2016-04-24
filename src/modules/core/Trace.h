#pragma once

#include "Log.h"
#include <SDL_platform.h>

#ifdef __EMSCRIPTEN__
//#define USE_EMTRACE 1
#endif
#if USE_EMTRACE
#include <emscripten/trace.h>
#else
#if USE_REMOTERY
#include "Remotery.h"
#endif
#endif

namespace core {

// a singleton - available via core::App
class Trace {
private:
#if USE_REMOTERY
	Remotery* _remotery;
#endif
public:
	Trace(uint16_t port = 17815) {
#if USE_REMOTERY
		rmtSettings* settings = rmt_Settings();
		settings->port = port;
		rmt_CreateGlobalInstance(&_remotery);
		rmt_SetCurrentThreadName("MainThread");
#if USE_GL_REMOTERY
		rmt_BindOpenGL();
#endif
#elif USE_EMTRACE
		emscripten_trace_configure("http://localhost:5000/", "Engine");
#endif
	}

	~Trace() {
#if USE_REMOTERY
#if USE_GL_REMOTERY
		rmt_UnbindOpenGL();
#endif
		rmt_DestroyGlobalInstance(_remotery);
		_remotery = nullptr;
#elif USE_EMSCTRACE
		emscripten_trace_close();
#endif
	}

	static inline bool enabled() {
#if USE_REMOTERY
		return true;
#elif USE_EMTRACE
		return true;
#else
		return false;
#endif
	}
};

#if USE_REMOTERY
#define core_trace_init() if (core::Trace::enabled()) Log::info("Remotery active")
#define core_trace_begin_frame()
#define core_trace_end_frame()
#define core_trace_begin(name) if (core::Trace::enabled()) rmt_BeginCPUSample(name, 0)
#define core_trace_end() if (core::Trace::enabled()) rmt_EndCPUSample()
#define core_trace_gl_begin(name) rmt_BeginOpenGLSample(name)
#define core_trace_gl_begin_dynamic(name) rmt_BeginOpenGLSampleDynamic(name)
#define core_trace_gl_end() rmt_EndOpenGLSample()
#define core_trace_msg(message) if (message != nullptr && core::Trace::enabled()) rmt_LogText(message)
#define core_trace_thread(name) if (core::Trace::enabled()) rmt_SetCurrentThreadName(name)
#elif USE_EMTRACE
#define core_trace_init() if (core::Trace::enabled()) Log::info("emtrace active")
#define core_trace_begin_frame() if (core::Trace::enabled()) emscripten_trace_record_frame_start()
#define core_trace_end_frame() if (core::Trace::enabled()) emscripten_trace_record_frame_end()
#define core_trace_begin(name) if (core::Trace::enabled()) emscripten_trace_enter_context(#name)
#define core_trace_end() if (core::Trace::enabled()) emscripten_trace_exit_context()
#define core_trace_gl_begin(name)
#define core_trace_gl_begin_dynamic(name)
#define core_trace_gl_end()
#define core_trace_msg(message)
#define core_trace_thread(name)
#else
#define core_trace_init()
#define core_trace_begin_frame()
#define core_trace_end_frame()
#define core_trace_begin(name)
#define core_trace_end()
#define core_trace_gl_begin(name)
#define core_trace_gl_begin_dynamic(name)
#define core_trace_gl_end()
#define core_trace_msg(message)
#define core_trace_thread(name)
#endif

class TraceScoped {
public:
	TraceScoped(const char* name, const char *msg = nullptr) {
		core_trace_begin(name);
		core_trace_msg(msg);
	}

	~TraceScoped() {
		core_trace_end();
	}
};

#define core_trace_scoped(name) core::TraceScoped(name)

}
