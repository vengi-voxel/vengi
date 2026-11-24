/**
 * @file
 * Tracy zone macros. Include this only from translation units and headers that
 * actually emit zones. core/Trace.h stays free of Tracy.hpp.
 */

#pragma once

#include "core/Trace.h"

#ifdef TRACY_ENABLE
#include "tracy/public/tracy/Tracy.hpp"

#define core_trace_value_scoped(name, x)                                                                               \
	ZoneNamedN(__tracy_scoped_##name, #name, true);                                                                    \
	ZoneValueV(__tracy_scoped_##name, (uint64_t)(x))
#define core_trace_plot(name, x) TracyPlot(name, x)
#define core_trace_plot_config(name, step) TracyPlotConfig(name, tracy::PlotFormatType::Number, step, true, 0)
#define core_trace_msg(message) TracyMessageL(message)
#define core_trace_thread(name) tracy::SetThreadName(name)
#define core_trace_end_frame(name) FrameMark
#define core_trace_scoped(name) ZoneNamedN(__tracy_scoped_##name, #name, true)
#endif
