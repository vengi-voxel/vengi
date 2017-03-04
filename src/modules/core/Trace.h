/**
 * @file
 */

#pragma once

#include <cstdint>

#define EASY_PROFILER_ENABLED 1
#if EASY_PROFILER_ENABLED
#ifndef BUILD_WITH_EASY_PROFILER
#define BUILD_WITH_EASY_PROFILER
#endif
#include <easy/profiler.h>
#endif

namespace core {

// a singleton - available via core::App
class Trace {
public:
	Trace(uint16_t port = 17815);
	~Trace();
};

class TraceScoped {
public:
	TraceScoped(const char* name, const char *msg = nullptr);
	~TraceScoped();
};

class TraceGLScoped {
public:
	TraceGLScoped(const char* name, const char *msg = nullptr);
	~TraceGLScoped();
};

extern void traceInit();
extern void traceGLInit();
extern void traceShutdown();
extern void traceGLShutdown();
extern void traceBeginFrame();
extern void traceEndFrame();
extern void traceBegin(const char* name);
extern void traceEnd();
extern void traceGLBegin(const char* name);
extern void traceGLEnd();
extern void traceMessage(const char* name);
extern void traceThread(const char* name);

#define core_trace_init() core::traceInit()
#define core_trace_gl_init() core::traceGLInit()
#define core_trace_shutdown() core::traceShutdown()
#define core_trace_gl_shutdown() core::traceGLShutdown()
#define core_trace_msg(message) core::traceMessage(message)
#define core_trace_thread(name) core::traceThread(name)

#if EASY_PROFILER_ENABLED
#define core_trace_begin_frame() EASY_BLOCK("Frame")
#define core_trace_end_frame()
#define core_trace_begin(name) EASY_BLOCK(#name)
#define core_trace_end() EASY_END_BLOCK
#define core_trace_gl_begin(name) core_trace_begin(name)
#define core_trace_gl_begin_dynamic(name) core_trace_begin(name)
#define core_trace_gl_end() core_trace_end()
#define core_trace_gl_scoped(name) core_trace_begin(name)
#define core_trace_scoped(name) core_trace_begin(name)
#else
#define core_trace_begin_frame() core::traceBeginFrame()
#define core_trace_end_frame() core::traceEndFrame()
#define core_trace_begin(name) core::traceBegin(#name)
#define core_trace_end() core::traceEnd()
#define core_trace_gl_begin(name) core::traceGLBegin(#name)
#define core_trace_gl_begin_dynamic(name) core::traceGLBegin(#name)
#define core_trace_gl_end() core::traceGLEnd()
#define core_trace_gl_scoped(name) core::TraceGLScoped name(#name)
#define core_trace_scoped(name) core::TraceScoped name(#name)
#endif

}
