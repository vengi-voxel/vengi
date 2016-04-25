#include "Trace.h"
#include "Var.h"
#include "Command.h"

namespace core {

static void rmtInputHandler(const char* text, void* context) {
	Log::info("typed '%s' to console", text);
	if (core::Command::execute(text) <= 0) {
		const VarPtr& var = core::Var::get(text, "", CV_NOTCREATEEMPTY);
		if (var) {
			core_trace_msg(var->strVal().c_str());
			return;
		}
		core_trace_msg("Could not handle input");
	}
}

Trace::Trace(uint16_t port) :
		_remotery(nullptr) {
	rmtSettings* settings = rmt_Settings();
	if (settings != nullptr) {
		settings->port = port;
		settings->input_handler = rmtInputHandler;
		settings->input_handler_context = nullptr;
	}
	rmt_CreateGlobalInstance(&_remotery);
#if USE_EMTRACE
	emscripten_trace_configure("http://localhost:5000/", "Engine");
#endif
	core_trace_thread("MainThread");
}

Trace::~Trace() {
	rmt_DestroyGlobalInstance(_remotery);
	_remotery = nullptr;
#if USE_EMSCTRACE
	emscripten_trace_close();
#endif
}

TraceScoped::TraceScoped(const char* name, const char *msg) {
	core_trace_begin(name);
	core_trace_msg(msg);
}

TraceScoped::~TraceScoped() {
	core_trace_end();
}

}
