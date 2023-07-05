/**
 * @file
 */

#include "PathTracer.h"
#include "core/Color.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include "image/Image.h"
#include "io/Stream.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/ChunkMesh.h"
#include "voxel/SurfaceExtractor.h"
#include "voxel/Mesh.h"
#include "voxel/Palette.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VolumeVisitor.h"
#include "yocto_scene.h"
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
	stop();
	delete _state;
}

bool PathTracer::createScene(const voxel::Palette &palette, const voxel::Mesh &mesh) {
	// TODO: in order to use glowing, we have to use materials here and support multiple shapes
	yocto::shape_data shape;
	const voxel::IndexArray &indices = mesh.getIndexVector();
	const voxel::VertexArray &vertices = mesh.getVertexVector();
	const voxel::NormalArray &normals = mesh.getNormalVector();
	const bool useNormals = normals.size() == vertices.size();
	shape.colors.resize(vertices.size());
	shape.positions.resize(vertices.size());
	shape.normals.resize(normals.size());
	for (int i = 0; i < (int)vertices.size(); ++i) {
		const auto &vertex = vertices[i];
		const core::RGBA rgba = palette.color(vertex.colorIndex);
		const glm::vec4 &color = core::Color::fromRGBA(rgba);
		shape.colors[i] = {color[0], color[1], color[2], color[3]};
		shape.positions[i] = {vertex.position[0], vertex.position[1], vertex.position[2]};
		if (useNormals) {
			shape.normals[i] = {normals[i][0], normals[i][1], normals[i][2]};
		}
	}

	const int tris = (int)indices.size() / 3;
	shape.triangles.resize(tris);
	for (int i = 0; i < tris; i++) {
		shape.triangles[i] = {i * 3 + 0, i * 3 + 1, i * 3 + 2};
	}
	_state->scene.shapes.push_back(shape);
	// TODO: create proper material
	_state->scene.materials.push_back({});

	const glm::vec3 &mins = mesh.getOffset();
	yocto::instance_data instance_data;
	instance_data.frame = yocto::translation_frame({mins[0], mins[1], mins[2]}),
	instance_data.shape = (int)_state->scene.shapes.size() - 1;
	instance_data.material = (int)_state->scene.materials.size() - 1;
	_state->scene.instances.push_back(instance_data);
	return true;
}

bool PathTracer::createScene(const scenegraph::SceneGraph &sceneGraph) {
	_state->scene = {};
	_state->lights = {};
	const bool marchingCubes = core::Var::getSafe(cfg::VoxelMeshMode)->intVal() == 1;
	// TODO: support references
	for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		const voxel::RawVolume *v = node.volume();
		if (v == nullptr) {
			continue;
		}

		voxel::ChunkMesh mesh(65536, 65536, true);
		voxel::Region region = v->region();

		voxel::SurfaceExtractionContext ctx =
			marchingCubes ? voxel::buildMarchingCubesContext(v, region, mesh, node.palette())
						  : voxel::buildCubicContext(v, region, mesh, v->region().getLowerCorner());
		voxel::extractSurface(ctx);

		if (!createScene(node.palette(), mesh.mesh[0])) {
			return false;
		}
		if (!createScene(node.palette(), mesh.mesh[1])) {
			return false;
		}
	}

	core_assert(_state->scene.cameras.empty());
	yocto::add_camera(_state->scene);
	core_assert(_state->scene.environments.empty());
	yocto::add_sky(_state->scene);

	_state->bvh = make_trace_bvh(_state->scene, _state->params);
	_state->lights = make_trace_lights(_state->scene, _state->params);
	return true;
}

bool PathTracer::start(const scenegraph::SceneGraph &sceneGraph, int dimensions, int samples) {
	createScene(sceneGraph);
	_state->params.resolution = dimensions;
	_state->params.samples = samples;
	_state->state = make_trace_state(_state->scene, _state->params);
	trace_start(_state->context, _state->state, _state->scene, _state->bvh, _state->lights, _state->params);
	return true;
}

bool PathTracer::stop() {
	yocto::trace_cancel(_state->context);
	return true;
}

bool PathTracer::update() {
	return yocto::trace_done(_state->context);
}

image::ImagePtr PathTracer::image() const {
	yocto::image_data image;
	if (!yocto::trace_done(_state->context)) {
		image = yocto::make_image(_state->state.width, _state->state.height, true);
		trace_preview(image, _state->context, _state->state, _state->scene, _state->bvh, _state->lights,
					  _state->params);
	} else {
		image = yocto::get_image(_state->state);
	}

	priv::YoctoImageReadStream stream(image);
	image::ImagePtr i = image::createEmptyImage("pathtracer");
	if (!i->loadRGBA(stream, image.width, image.height)) {
		return {};
	}
	return i;
}

} // namespace voxelpathtracer
