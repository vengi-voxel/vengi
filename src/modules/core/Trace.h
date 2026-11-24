/**
 * @file
 */

#pragma once

#include "core/concurrent/Lock.h"
#include <stdint.h>

// Zone macros that need Tracy.hpp live in core/TraceScoped.h so this header stays cheap to parse.

namespace core {

// a singleton - available via app::App
class Trace {
public:
	Trace();
	~Trace();
};

extern void traceInit();
extern void traceShutdown();
extern void traceBeginFrame();
extern void traceEndFrame();
extern void traceBegin(const char* name);
extern void traceEnd();
extern void traceMessage(const char* name);
extern void traceThread(const char* name);
/** Identify the process in the Tracy connect dialog (no-op without Tracy). */
extern void traceAppInfo(const char *name);

class TraceScoped {
public:
	inline TraceScoped(const char *name, const char *msg = nullptr) {
		traceBegin(name);
		traceMessage(msg);
	}
	inline ~TraceScoped() {
		traceEnd();
	}
};

#ifdef TRACY_ENABLE
#define core_trace_init() core::traceInit()
#define core_trace_shutdown() core::traceShutdown()
#define core_trace_app_info(name) core::traceAppInfo(name)
#define core_trace_begin_frame(name)
#define core_trace_begin(name)
#define core_trace_end()
#elif USE_EMTRACE
#define core_trace_value_scoped(name, x)
#define core_trace_plot(name, x)
#define core_trace_plot_config(name, step)
#define core_trace_init() core::traceInit()
#define core_trace_shutdown() core::traceShutdown()
#define core_trace_app_info(name) do { } while (0)
#define core_trace_msg(message) core::traceMessage(message)
#define core_trace_thread(name) core::traceThread(name)

#define core_trace_begin_frame(name) core::traceBeginFrame()
#define core_trace_end_frame(name) core::traceEndFrame()
#define core_trace_begin(name) core::traceBegin(#name)
#define core_trace_end() core::traceEnd()
#define core_trace_scoped(name) core::TraceScoped __trace__##name(#name)
#else // USE_EMTRACE

/* "while (0,0)" fools Microsoft's compiler's /W4 warning level into thinking
	this condition isn't constant. And looks like an owl's face! */
#ifdef _MSC_VER /* stupid /W4 warnings. */
#define TRACE_NULL_WHILE_LOOP_CONDITION (0, 0)
#else
#define TRACE_NULL_WHILE_LOOP_CONDITION (0)
#endif
#define core_trace_value_scoped(name, x) do { } while (TRACE_NULL_WHILE_LOOP_CONDITION)
#define core_trace_plot(name, x) do { } while (TRACE_NULL_WHILE_LOOP_CONDITION)
#define core_trace_plot_config(name, step) do { } while (TRACE_NULL_WHILE_LOOP_CONDITION)
#define core_trace_init() do { } while (TRACE_NULL_WHILE_LOOP_CONDITION)
#define core_trace_shutdown() do { } while (TRACE_NULL_WHILE_LOOP_CONDITION)
#define core_trace_app_info(name) do { } while (TRACE_NULL_WHILE_LOOP_CONDITION)
#define core_trace_msg(message) do { } while (TRACE_NULL_WHILE_LOOP_CONDITION)
#define core_trace_thread(name) do { } while (TRACE_NULL_WHILE_LOOP_CONDITION)

#define core_trace_begin_frame(name) do { } while (TRACE_NULL_WHILE_LOOP_CONDITION)
#define core_trace_end_frame(name) do { } while (TRACE_NULL_WHILE_LOOP_CONDITION)
#define core_trace_begin(name) do { } while (TRACE_NULL_WHILE_LOOP_CONDITION)
#define core_trace_end() do { } while (TRACE_NULL_WHILE_LOOP_CONDITION)
#define core_trace_scoped(name) do { } while (TRACE_NULL_WHILE_LOOP_CONDITION)
#endif

}
