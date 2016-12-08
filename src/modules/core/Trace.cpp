/**
 * @file
 */

#include "Trace.h"
#include "Var.h"
#include "Command.h"

#if USE_EMTRACE
#include <emscripten/trace.h>
#endif

#define RMT_ENABLED 0
#define RMT_USE_POSIX_THREADNAMES 1
// disable remotery via -DRMT_ENABLED=0
#include "Remotery.h"

namespace core {

#if RMT_ENABLED
static Remotery* _remotery = nullptr;

static void rmtInputHandler(const char* text, void* context) {
	Log::info("typed '%s' to console", text);
	if (core::Command::execute(text) <= 0) {
		const VarPtr& var = core::Var::get(text);
		if (var) {
			core_trace_msg(var->strVal().c_str());
			return;
		}
		core_trace_msg("Could not handle input");
	}
}
#endif

Trace::Trace(uint16_t port) {
#if RMT_ENABLED
	rmtSettings* settings = rmt_Settings();
	if (settings != nullptr) {
		settings->port = port;
		settings->input_handler = rmtInputHandler;
		settings->input_handler_context = nullptr;
#if 0
		settings->malloc = SDL_malloc;
		settings->free = SDL_free;
		settings->realloc = SDL_realloc;
#endif
	}
	const rmtError rmtInstError = rmt_CreateGlobalInstance(&_remotery);
	if (rmtInstError != RMT_ERROR_NONE) {
		Log::error("Failed to init remotery");
	}
#elif USE_EMTRACE
	emscripten_trace_configure("http://localhost:5000/", "Engine");
#endif
	traceThread("MainThread");
}

Trace::~Trace() {
#if RMT_ENABLED
	if (_remotery != nullptr) {
		rmt_DestroyGlobalInstance(_remotery);
		_remotery = nullptr;
	}
#elif USE_EMTRACE
	emscripten_trace_close();
#endif
}

TraceScoped::TraceScoped(const char* name, const char *msg) {
	traceBegin(name);
	traceMessage(msg);
}

TraceScoped::~TraceScoped() {
	traceEnd();
}

TraceGLScoped::TraceGLScoped(const char* name, const char *msg) {
	traceGLBegin(name);
	traceMessage(msg);
}

TraceGLScoped::~TraceGLScoped() {
	traceGLEnd();
}

void traceInit() {
#if RMT_ENABLED > 0
	Log::info("Remotery active");
#elif USE_EMTRACE
	Log::info("emtrace active");
#endif
}

void traceBeginFrame() {
#if USE_EMTRACE
	emscripten_trace_record_frame_start();
#else
	traceBegin("Frame");
#endif
}

void traceEndFrame() {
#if USE_EMTRACE
	emscripten_trace_record_frame_end();
#else
	traceEnd();
#endif
}

void traceBegin(const char* name) {
#if RMT_ENABLED > 0
	rmtU32 rmt_sample_hash = 0;
	_rmt_BeginCPUSample(name, 0, &rmt_sample_hash);
#elif USE_EMTRACE
	emscripten_trace_enter_context(name);
#endif
}

void traceEnd() {
#if RMT_ENABLED > 0
	rmt_EndCPUSample();
#elif USE_EMTRACE
	emscripten_trace_exit_context();
#endif
}

void traceGLBegin(const char* name) {
#if RMT_ENABLED && RMT_USE_OPENGL
	rmtU32 rmt_sample_hash = 0;
	_rmt_BeginOpenGLSample(name, 0, &rmt_sample_hash);
	//_rmt_BeginOpenGLSampleDynamic(name, 0, &rmt_sample_hash);
#else
	traceBegin(name);
#endif
}

void traceGLEnd() {
#if RMT_ENABLED > 0
	rmt_EndOpenGLSample();
#else
	traceEnd();
#endif
}

void traceMessage(const char* message) {
	if (message == nullptr) {
		return;
	}
#if RMT_ENABLED > 0
	rmt_LogText(message);
#else
	Log::trace("%s", message);
#endif
}

void traceThread(const char* name) {
#if RMT_ENABLED > 0
	rmt_SetCurrentThreadName(name);
#else
	traceMessage(name);
#endif
}

}
