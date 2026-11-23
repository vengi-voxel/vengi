/**
 * @file
 * Tracy zone macros. The C API stays off Tracy.hpp so this header does not
 * instantiate the profiler templates (thread, unique_ptr, locks).
 */

#pragma once

#include "core/Trace.h"

#ifdef TRACY_ENABLE
#include "tracy/public/tracy/TracyC.h"

namespace core {

class TraceCZone {
public:
	explicit TraceCZone(TracyCZoneCtx ctx) : _ctx(ctx) {
	}
	~TraceCZone() {
		___tracy_emit_zone_end(_ctx);
	}
	void value(uint64_t v) {
		___tracy_emit_zone_value(_ctx, v);
	}

	TraceCZone(const TraceCZone &) = delete;
	TraceCZone &operator=(const TraceCZone &) = delete;

private:
	TracyCZoneCtx _ctx;
};

}

#define core_trace_scoped(name)                                                                                        \
	static const struct ___tracy_source_location_data TracyConcat(__tracy_srcloc_, __LINE__) = {                       \
		#name, __func__, __FILE__, (uint32_t)__LINE__, 0};                                                             \
	core::TraceCZone __trace_##name(___tracy_emit_zone_begin(&TracyConcat(__tracy_srcloc_, __LINE__), 1))
#define core_trace_value_scoped(name, x)                                                                               \
	static const struct ___tracy_source_location_data TracyConcat(__tracy_srcloc_, __LINE__) = {                       \
		#name, __func__, __FILE__, (uint32_t)__LINE__, 0};                                                             \
	core::TraceCZone __tracy_scoped_##name(___tracy_emit_zone_begin(&TracyConcat(__tracy_srcloc_, __LINE__), 1));      \
	__tracy_scoped_##name.value((uint64_t)(x))
#define core_trace_plot(name, x) ___tracy_emit_plot((name), (double)(x))
#define core_trace_plot_config(name, step)                                                                             \
	___tracy_emit_plot_config((name), (int32_t)TracyPlotFormatNumber, (step) ? 1 : 0, 1, 0)
#define core_trace_msg(message) ___tracy_emit_logStringL((int8_t)TracyMessageSeverityInfo, 0, 0, (message))
#define core_trace_thread(name) ___tracy_set_thread_name(name)
#define core_trace_end_frame(name) ___tracy_emit_frame_mark(0)
#endif
