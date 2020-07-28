/**
 * @file
 */

#pragma once

#include "core/Trace.h"
#include "engine-config.h"
#ifdef TRACY_ENABLE
#ifdef HAVE_OPENCL
#include "cl/CL.h"
#include "core/tracy/TracyOpenCL.hpp"
#endif
#endif

namespace compute {

class TraceCLScoped {
public:
	TraceCLScoped(const char* name, const char *msg = nullptr);
	~TraceCLScoped();
};

extern void traceCLBegin(const char* name);
extern void traceCLEnd();

#if defined(TRACY_ENABLE) && defined(HAVE_OPENCL)
#define compute_trace_context tracy::OpenCLCtx
#define compute_trace_init(ctx, device) TracyCLContext(ctx, device)
#define compute_trace_shutdown(ctx) TracyCLDestroy(ctx)
#define compute_trace_begin(ctx, name)
#define compute_trace_begin_dynamic(ctx, name)
#define compute_trace_end(ctx)
#define compute_trace_scoped(ctx, name) TracyCLNamedZone(ctx, __tracy_scoped_##name, #name, true)
#define compute_trace_frame_end(ctx) TracyCLCollect(ctx)
#else
#define compute_trace_context void
#define compute_trace_init(ctx, device) nullptr
#define compute_trace_shutdown(ctx)
#define compute_trace_begin(ctx, name) compute::traceCLBegin(#name)
#define compute_trace_begin_dynamic(ctx, name) compute::traceCLBegin(#name)
#define compute_trace_end(ctx) compute::traceCLEnd()
#define compute_trace_scoped(ctx, name) compute::TraceCLScoped __trace__##name(#name)
#define compute_trace_frame_end(ctx)
#endif

}
