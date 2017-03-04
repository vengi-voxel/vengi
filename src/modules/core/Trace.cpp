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

#define EASY_PROFILER_ENABLED 1
#if EASY_PROFILER_ENABLED
#ifndef BUILD_WITH_EASY_PROFILER
#define BUILD_WITH_EASY_PROFILER
#endif
#include <easy/profiler.h>
#endif

#define MICROPROFILE_EMABLED 0
#if MICROPROFILE_EMABLED
#define MICROPROFILE_IMPL
#define MICROPROFILE_GPU_TIMERS 0
#if MICROPROFILE_GPU_TIMERS
#define MICROPROFILE_GPU_TIMERS_GL 1
#endif
#include "trace/microprofile.h"
#include "trace/microprofile.cpp"
#include <unordered_map>
thread_local std::unordered_map<const char*, MicroProfileToken> _tokens;
thread_local const char *_threadName = nullptr;
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
	} else {
		Log::info("Remotery port: %i", port);
	}
#elif USE_EMTRACE
	emscripten_trace_configure("http://localhost:5000/", "Engine");
#elif MICROPROFILE_ENABLED
	MicroProfileInit();
	MicroProfileSetEnableAllGroups(true);
	MicroProfileSetForceMetaCounters(true);
	MicroProfileStartContextSwitchTrace();
#elif EASY_PROFILER_ENABLED
	profiler::startListen(port);
	Log::info("easy_profiler %s port: %i", profiler::versionName(), (int)port);
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
#elif EASY_PROFILER_ENABLED
	profiler::stopListen();
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
	Log::info("microprofile active on port " CORE_STRINGIFY(MICROPROFILE_WEBSERVER_PORT));
#elif EASY_PROFILER_ENABLED
	Log::info("easy profiler active");
#endif
}

void traceGLInit() {
#if RMT_ENABLED && RMT_USE_OPENGL
	rmt_BindOpenGL();
#elif MICROPROFILE_ENABLED
#if MICROPROFILE_GPU_TIMERS_GL
	MicroProfileGpuInitGL();
#endif
#endif
}

void traceShutdown() {
#if EASY_PROFILER_ENABLED
	EASY_PROFILER_DISABLE;
#endif
}

void traceGLShutdown() {
#if RMT_ENABLED && RMT_USE_OPENGL
	rmt_UnbindOpenGL();
#elif MICROPROFILE_ENABLED && MICROPROFILE_GPU_TIMERS
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

#if MICROPROFILE_EMABLED
static inline MicroProfileToken getToken(MicroProfileTokenType type, const char *name, uint32_t color = 0xff00ff, const char *group = nullptr) {
	if (group == nullptr) {
		group = _threadName;
	}
	auto i = _tokens.find(name);
	MicroProfileToken token;
	if (i == _tokens.end()) {
		MicroProfileGetTokenC(&token, group, name, color, type);
		_tokens.insert(std::make_pair(name, token));
	} else {
		token = i->second;
	}
	return token;
}
#endif

void traceBegin(const char* name) {
#if RMT_ENABLED
	_rmt_BeginCPUSample(name, 0, nullptr);
#elif USE_EMTRACE
	emscripten_trace_enter_context(name);
#elif EASY_PROFILER_ENABLED
	EASY_BLOCK(name);
#elif MICROPROFILE_EMABLED
	MicroProfileEnter(getToken(MicroProfileTokenTypeCpu, name));
#endif
}

void traceEnd() {
#if RMT_ENABLED
	rmt_EndCPUSample();
#elif EASY_PROFILER_ENABLED
	EASY_END_BLOCK;
#elif USE_EMTRACE
	emscripten_trace_exit_context();
#elif MICROPROFILE_EMABLED
	MicroProfileLeave();
#endif
}

void traceGLBegin(const char* name) {
#if RMT_ENABLED && RMT_USE_OPENGL
	rmtU32 rmt_sample_hash = 0;
	_rmt_BeginOpenGLSample(name, 0, &rmt_sample_hash);
	//_rmt_BeginOpenGLSampleDynamic(name, 0, &rmt_sample_hash);
#elif MICROPROFILE_EMABLED && MICROPROFILE_GPU_TIMERS
	MicroProfileEnterGpu(getToken(MicroProfileTokenTypeGpu, name), MicroProfileGetGlobalGpuThreadLog());
#else
	traceBegin(name);
#endif
}

void traceGLEnd() {
#if RMT_ENABLED && RMT_USE_OPENGL
	rmt_EndOpenGLSample();
#elif MICROPROFILE_EMABLED && MICROPROFILE_GPU_TIMERS
	MicroProfileLeaveGpu(MicroProfileGetGlobalGpuThreadLog());
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

void traceThread(const	 char* name) {
#if RMT_ENABLED
	rmt_SetCurrentThreadName(name);
#elif MICROPROFILE_EMABLED
	_threadName = name;
	MicroProfileOnThreadCreate(name);
#elif EASY_PROFILER_ENABLED
	EASY_THREAD(name);
#else
	traceMessage(name);
#endif
}

}
