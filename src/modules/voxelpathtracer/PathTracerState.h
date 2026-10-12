/**
 * @file
 */

#pragma once

#include <yocto_scene.h>
#include <yocto_trace.h>

namespace voxelpathtracer {

using PathTracerParams = yocto::trace_params;

struct PathTracerState {
	yocto::trace_context context;
	yocto::scene_data scene;
	yocto::trace_bvh bvh;
	PathTracerParams params;
	yocto::trace_lights lights;
	yocto::trace_state state;
	bool started = false;
	float aperture = 0.0f;
	float sunIntensity = 1.0f;
	float sunArea = 1.0f;
	float sunElevation = yocto::pif / 4.0f;
	float sunAzimuth = yocto::pif / 4.0f;
	bool sunDisk = false;
	float exposure = 0.0f;
	bool filmic = false;

	PathTracerState() : context(yocto::make_trace_context({})) {
	}
};

} // namespace voxelpathtracer
