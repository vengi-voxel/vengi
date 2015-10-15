#pragma once

#ifdef __EMSCRIPTEN__
#define USE_EMTRACE 1
#include <emscripten/trace.h>
#else
#endif

#include "Var.h"

namespace core {

// a singleton - available via core::App
class Trace {
public:
	Trace(uint16_t port = 0x4597) {
#if USE_EMTRACE
		emscripten_trace_configure("http://localhost:5000/", "Engine");
#endif
	}
	~Trace() {
	}

	static inline bool enabled() {
#if USE_EMTRACE
		return true;
#else
		return false;
#endif
	}
};

#if USE_EMTRACE
#define core_trace_begin_frame() if (core::Trace::enabled()) emscripten_trace_record_frame_start()
#define core_trace_end_frame() if (core::Trace::enabled()) emscripten_trace_record_frame_end()
#define core_trace_begin(name) if (core::Trace::enabled()) emscripten_trace_enter_context(#name)
#define core_trace_end(name) if (core::Trace::enabled()) emscripten_trace_exit_context()
#else
#define core_trace_begin_frame()
#define core_trace_end_frame()
#define core_trace_begin(name)
#define core_trace_end()
#endif

class TraceScoped {
private:
	const bool enabled;
public:
	TraceScoped(const char* name) :
			enabled(Trace::enabled()) {
		if (enabled) {
#if USE_EMTRACE
			emscripten_trace_enter_context(name);
#endif
		}
	}

	~TraceScoped() {
		if (enabled) {
#if USE_EMTRACE
			emscripten_trace_exit_context();
#endif
		}
	}
};

#define core_trace_scoped(name) core::TraceScoped(name)
#if USE_EMTRACE
#define core_trace_msg(name)
#else
#define core_trace_msg(message)
#endif

}
