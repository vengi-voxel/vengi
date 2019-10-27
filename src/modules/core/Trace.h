/**
 * @file
 */

#pragma once

#include <stdint.h>

namespace core {

// a singleton - available via core::App
class Trace {
public:
	Trace();
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

class TraceCallback {
public:
	virtual ~TraceCallback() {}
	virtual void traceBeginFrame(const char *threadName) {}
	virtual void traceBegin(const char *threadName, const char *name) = 0;
	virtual void traceEnd(const char *threadName) = 0;
	virtual void traceEndFrame(const char *threadName) {}
};

extern TraceCallback* traceSet(TraceCallback* callback);
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

#define core_trace_set(x) core::traceSet(x)
#define core_trace_init() core::traceInit()
#define core_trace_gl_init() core::traceGLInit()
#define core_trace_shutdown() core::traceShutdown()
#define core_trace_gl_shutdown() core::traceGLShutdown()
#define core_trace_msg(message) core::traceMessage(message)
#define core_trace_thread(name) core::traceThread(name)
#define core_trace_mutex(type, name) type name

#define core_trace_begin_frame() core::traceBeginFrame()
#define core_trace_end_frame() core::traceEndFrame()
#define core_trace_begin(name) core::traceBegin(#name)
#define core_trace_end() core::traceEnd()
#define core_trace_gl_begin(name) core::traceGLBegin(#name)
#define core_trace_gl_begin_dynamic(name) core::traceGLBegin(#name)
#define core_trace_gl_end() core::traceGLEnd()
#define core_trace_gl_scoped(name) core::TraceGLScoped __trace__##name(#name)
#define core_trace_scoped(name) core::TraceScoped __trace__##name(#name)

}
