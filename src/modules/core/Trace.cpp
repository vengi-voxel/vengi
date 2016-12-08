/**
 * @file
 */

#include "Trace.h"
#include "core/Var.h"
#include "core/command/Command.h"

#if USE_EMTRACE
#include <emscripten/trace.h>
#endif

#define RMT_ENABLED 0
#if RMT_ENABLED
#define RMT_USE_POSIX_THREADNAMES 1
// disable remotery via -DRMT_ENABLED=0
#include "trace/Remotery.h"
#include "core/command/CommandHandler.h"
#endif

#define MICROPROFILE_EMABLED 0
#if MICROPROFILE_EMABLED
#include "trace/microprofile.h"
#include "trace/microprofile.cpp"
#endif

namespace core {

#if RMT_ENABLED
static Remotery* _remotery = nullptr;

static void rmtInputHandler(const char* text, void* context) {
	core::executeCommands(text);
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
		rmt_UnbindOpenGL();
		rmt_DestroyGlobalInstance(_remotery);
		_remotery = nullptr;
	}
#elif USE_EMTRACE
	emscripten_trace_close();
#elif MICROPROFILE_EMABLED
	MicroProfileShutdown();
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
#if RMT_ENABLED
	Log::info("Remotery active");
#elif USE_EMTRACE
	Log::info("emtrace active");
#elif MICROPROFILE_ENABLED
	MicroProfileInit();
	MicroProfileSetEnableAllGroups(true);
	MicroProfileSetForceMetaCounters(true);
	MicroProfileStartContextSwitchTrace();
#endif
}

void traceGLInit() {
#if RMT_ENABLED
	rmt_BindOpenGL();
#endif
}

void traceShutdown() {
}

void traceGLShutdown() {
#if RMT_ENABLED
	rmt_UnbindOpenGL();
#elif MICROPROFILE_ENABLED
	MicroProfileGpuShutdown();
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
#if MICROPROFILE_EMABLED
	MicroProfileFlip(nullptr);
#endif
#endif
}

void traceBegin(const char* name) {
#if RMT_ENABLED
	_rmt_BeginCPUSample(name, 0, nullptr);
#elif USE_EMTRACE
	emscripten_trace_enter_context(name);
#elif MICROPROFILE_EMABLED
	MICROPROFILE_ENTERI(name, name, 0xffffffff);
#endif
}

void traceEnd() {
#if RMT_ENABLED
	rmt_EndCPUSample();
#elif USE_EMTRACE
	emscripten_trace_exit_context();
#elif MICROPROFILE_EMABLED
	MICROPROFILE_LEAVE();
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
#if RMT_ENABLED
	rmt_EndOpenGLSample();
#else
	traceEnd();
#endif
}

void traceMessage(const char* message) {
	if (message == nullptr) {
		return;
	}
#if RMT_ENABLED
	rmt_LogText(message);
#else
	Log::trace("%s", message);
#endif
}

void traceThread(const char* name) {
#if RMT_ENABLED
	rmt_SetCurrentThreadName(name);
#elif MICROPROFILE_EMABLED
	MicroProfileOnThreadCreate(name);
#else
	traceMessage(name);
#endif
}

}
