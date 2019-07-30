/**
 * @file
 */

#include "core/Trace.h"
#include "core/Var.h"
#include "core/Log.h"
#include "core/Common.h"
#include "core/command/Command.h"

#ifdef USE_EMTRACE
#include <emscripten/trace.h>
#endif

namespace core {

namespace {

static TraceCallback* _callback = nullptr;
static thread_local const char* _threadName = "Unknown";

}

Trace::Trace() {
#ifdef USE_EMTRACE
	emscripten_trace_configure("http://localhost:17000/", "Engine");
#endif
	traceThread("MainThread");
}

Trace::~Trace() {
#ifdef USE_EMTRACE
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

TraceCallback* traceSet(TraceCallback* callback) {
	TraceCallback* old = _callback;
	_callback = callback;
	return old;
}

void traceInit() {
#ifdef USE_EMTRACE
	Log::info("emtrace active");
#endif
}

void traceGLInit() {
}

void traceShutdown() {
}

void traceGLShutdown() {
}

void traceBeginFrame() {
#ifdef USE_EMTRACE
	emscripten_trace_record_frame_start();
#else
	if (_callback != nullptr) {
		_callback->traceBeginFrame(_threadName);
	} else {
		traceBegin("Frame");
	}
#endif
}

void traceEndFrame() {
#ifdef USE_EMTRACE
	emscripten_trace_record_frame_end();
#else
	if (_callback != nullptr) {
		_callback->traceEndFrame(_threadName);
	} else {
		traceEnd();
	}
#endif
}

void traceBegin(const char* name) {
#ifdef USE_EMTRACE
	emscripten_trace_enter_context(name);
#else
	if (_callback != nullptr) {
		_callback->traceBegin(_threadName, name);
	}
#endif
}

void traceEnd() {
#ifdef USE_EMTRACE
	emscripten_trace_exit_context();
#else
	if (_callback != nullptr) {
		_callback->traceEnd(_threadName);
	}
#endif
}

void traceGLBegin(const char* name) {
	traceBegin(name);
}

void traceGLEnd() {
	traceEnd();
}

void traceMessage(const char* message) {
	if (message == nullptr) {
		return;
	}
	Log::trace("%s", message);
}

void traceThread(const char* name) {
	_threadName = name;
}

}
