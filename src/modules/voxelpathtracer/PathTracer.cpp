/**
 * @file
 */

#include "PathTracer.h"
#include "image/Image.h"
#include "io/Stream.h"
#include "scenegraph/SceneGraph.h"
#include <yocto_trace.h>

namespace voxelpathtracer {

namespace priv {

/**
 * Simplified read stream that knows how image::Image::loadRGBA() works.
 *
 * @code
 * YoctoImageReadStream stream(img);
 * target.loadRGBA(stream, img.width, img.height);
 * @endcode
 */
class YoctoImageReadStream : public io::ReadStream {
private:
	const yocto::image_data &_img;
	bool _eos = false;

public:
	YoctoImageReadStream(const yocto::image_data &img) : _img(img) {
	}

	// the complete image is read with one call!!
	int read(void *dataPtr, size_t dataSize) override {
		if ((int)dataSize != _img.width * _img.height * 4) {
			return -1;
		}
		uint8_t *buf = (uint8_t *)dataPtr;
		for (int i = 0; i < _img.height; i++) {
			for (int j = 0; j < _img.width; j++) {
				const yocto::vec4b &v = yocto::float_to_byte(_img[{j, i}]);
				memcpy(&buf[(size_t)(i * _img.width + j) * 4], &v, 4);
			}
		}
		_eos = true;
		return (int)dataSize;
	}

	bool eos() const override {
		return _eos;
	}
};

} // namespace priv

struct PathTracerState {
	yocto::trace_context context;
	yocto::scene_data scene;
	yocto::trace_bvh bvh;
	yocto::trace_params params;
	yocto::trace_lights lights;
	yocto::trace_state state;

	PathTracerState() : context(yocto::make_trace_context({})) {
	}
};

PathTracer::PathTracer() : _state(new PathTracerState()) {
}

PathTracer::~PathTracer() {
	delete _state;
}

bool PathTracer::createScene(const scenegraph::SceneGraph &sceneGraph) {
	_state->scene = {};
	_state->lights = {};
	// TODO: create mesh
	// TODO: create yocto shape from mesh
	// TODO: create yocto scene by adding shapes
	// _state->scene.shapes.push_back(shape);
	// const glm::vec3 mins = node->region().getLowerCornerf();
	// yocto::instance_data instance_data;
	// instance_data.frame = yocto::translation_frame({mins[0], mins[1], mins[2]}),
	// instance_data.shape = (int)_state->scene.shapes.size() - 1;
	// instance_data.material = (int)_state->scene.materials.size() - 1;
	// _state->scene.instances.push_back(instance_data);
	return false;
}

bool PathTracer::start(const scenegraph::SceneGraph &sceneGraph, int dimensions, int samples) {
	createScene(sceneGraph);
	_state->params.resolution = dimensions;
	_state->params.samples = samples;
	_state->state = make_trace_state(_state->scene, _state->params);
	trace_start(_state->context, _state->state, _state->scene, _state->bvh, _state->lights, _state->params);

	return false;
}

bool PathTracer::stop() {
	yocto::trace_cancel(_state->context);
	return true;
}

bool PathTracer::update() {
	// TODO: implement
	return false;
}

image::ImagePtr PathTracer::image() const {
	if (!_state->context.stop && !_state->context.done) {
		return {};
	}
	// yocto::image_data imageData = yocto::get_image(_state->state);
	// TODO: implement
	return {};
}

} // namespace voxelpathtracer
