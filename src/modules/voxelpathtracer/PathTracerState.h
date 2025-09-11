/**
 * @file
 */

#include <yocto_scene.h>
#include <yocto_trace.h>

namespace voxelpathtracer {

struct PathTracerState {
	yocto::trace_context context;
	yocto::scene_data scene;
	yocto::trace_bvh bvh;
	yocto::trace_params params;
	yocto::trace_lights lights;
	yocto::trace_state state;
	bool started = false;

	PathTracerState() : context(yocto::make_trace_context({})) {
	}
};

} // namespace voxelpathtracer
