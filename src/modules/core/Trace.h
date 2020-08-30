/**
 * @file
 */

#pragma once

#include <stdint.h>

#ifdef TRACY_ENABLE
#include "tracy/Tracy.hpp"
#endif

namespace core {

// a singleton - available via app::App
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
extern void traceShutdown();
extern void traceBeginFrame();
extern void traceEndFrame();
extern void traceBegin(const char* name);
extern void traceEnd();
extern void traceMessage(const char* name);
extern void traceThread(const char* name);

#ifdef TRACY_ENABLE
#define core_trace_value_scoped(name, x) ZoneNamedN(__tracy_scoped_##name, #name, true); ZoneValueV(__tracy_scoped_##name, (uint64_t)(x))
#define core_trace_plot(name, x) TracyPlot(name, x)
#define core_trace_set(x) core::traceSet(x)
#define core_trace_init() core::traceInit()
#define core_trace_shutdown() core::traceShutdown()
#define core_trace_msg(message) TracyMessageL(message)
#define core_trace_thread(name) tracy::SetThreadName(name)
#define core_trace_mutex(type, varname, name) type varname { tracy::SourceLocationData{ nullptr, name, __FILE__, __LINE__, 0 } }

#define core_trace_begin_frame(name)
#define core_trace_end_frame(name) FrameMark
#define core_trace_begin(name)
#define core_trace_end()
#define core_trace_scoped(name) ZoneNamedN(__tracy_scoped_##name, #name, true)
#else
#define core_trace_value_scoped(name, x)
#define core_trace_plot(name, x)
#define core_trace_set(x) core::traceSet(x)
#define core_trace_init() core::traceInit()
#define core_trace_shutdown() core::traceShutdown()
#define core_trace_msg(message) core::traceMessage(message)
#define core_trace_thread(name) core::traceThread(name)
#define core_trace_mutex(type, varname, name) type varname

#define core_trace_begin_frame(name) core::traceBeginFrame()
#define core_trace_end_frame(name) core::traceEndFrame()
#define core_trace_begin(name) core::traceBegin(#name)
#define core_trace_end() core::traceEnd()
#define core_trace_scoped(name) core::TraceScoped __trace__##name(#name)
#endif

}
