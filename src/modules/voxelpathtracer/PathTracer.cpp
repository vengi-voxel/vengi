/**
 * @file
 */

#include "PathTracer.h"
#include "core/Color.h"
#include "core/Log.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include "image/Image.h"
#include "io/Stream.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/ChunkMesh.h"
#include "voxel/Mesh.h"
#include "voxel/Palette.h"
#include "voxel/RawVolume.h"
#include "voxel/SurfaceExtractor.h"
#include "voxelutil/VolumeVisitor.h"

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

PathTracer::~PathTracer() {
	stop();
}

bool PathTracer::createScene(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
							 const voxel::Mesh &mesh) {
	const voxel::Palette &palette = node.palette();
	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	const scenegraph::SceneGraphTransform &transform = node.transform(keyFrameIdx);
	// TODO: in order to use glowing, we have to use materials here and support multiple shapes
	yocto::shape_data shape;
	const voxel::IndexArray &indices = mesh.getIndexVector();
	if (indices.empty()) {
		return true;
	}
	const voxel::VertexArray &vertices = mesh.getVertexVector();
	const voxel::NormalArray &normals = mesh.getNormalVector();
	const bool useNormals = normals.size() == vertices.size();
	shape.colors.reserve(vertices.size());
	shape.positions.reserve(vertices.size());
	shape.texcoords.reserve(vertices.size());
	if (useNormals) {
		shape.normals.reserve(normals.size());
	}

	const glm::vec3 size = glm::vec3(sceneGraph.resolveRegion(node).getDimensionsInVoxels());
	const glm::vec3 pivot = sceneGraph.resolvePivot(node);

	for (int i = 0; i < (int)vertices.size(); ++i) {
		const auto &vertex = vertices[i];
		const core::RGBA rgba = palette.color(vertex.colorIndex);
		const glm::vec4 &color = core::Color::fromRGBA(rgba);
		shape.colors.push_back({color[0], color[1], color[2], color[3]});
		const glm::vec3 pos = transform.apply(vertex.position, pivot * size);
		shape.positions.push_back({pos[0], pos[1], pos[2]});
		const glm::vec2 &uv = image::Image::uv(vertex.colorIndex, 0, voxel::PaletteMaxColors, 1);
		shape.texcoords.push_back({uv[0], uv[1]});
		if (useNormals) {
			shape.normals.push_back({normals[i][0], normals[i][1], normals[i][2]});
		}
	}

	core_assert((int)indices.size() % 3 == 0);
	const int tris = (int)indices.size() / 3;
	shape.triangles.reserve(tris);
	for (int i = 0; i < tris; i++) {
		shape.triangles.push_back({(int)indices[i * 3 + 0], (int)indices[i * 3 + 1], (int)indices[i * 3 + 2]});
	}
	_state.scene.shapes.push_back(shape);

	const glm::vec3 &mins = mesh.getOffset();
	yocto::instance_data instance_data;
	instance_data.frame = yocto::translation_frame({mins[0], mins[1], mins[2]}),
	instance_data.shape = (int)_state.scene.shapes.size() - 1;
	instance_data.material = (int)_state.scene.materials.size() - 1;
	_state.scene.instances.push_back(instance_data);
	return true;
}

bool PathTracer::createScene(const scenegraph::SceneGraph &sceneGraph) {
	_state.scene = {};
	_state.lights = {};
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

		yocto::texture_data texture;
		texture.height = 1;
		texture.width = voxel::PaletteMaxColors;
		for (int i = 0; i < node.palette().colorCount(); ++i) {
			const core::RGBA color = node.palette().color(i);
			texture.pixelsb.push_back({color.r, color.g, color.b, color.a});
		}
		for (int i = node.palette().colorCount(); i < texture.width; ++i) {
			texture.pixelsb.push_back({});
		}
		_state.scene.textures.push_back(texture);
		// TODO: create proper material
		yocto::material_data material;
		material.color = {1.0f, 1.0f, 1.0f};
		if (!_state.scene.textures.empty()) {
			material.color_tex = (int)_state.scene.textures.size() - 1;
		}
		_state.scene.materials.push_back(material);
		scenegraph::KeyFrameIndex keyFrameIdx = 0;
		const scenegraph::SceneGraphTransform &transform = node.transform(keyFrameIdx);
		if (!createScene(sceneGraph, node, mesh.mesh[0])) {
			return false;
		}
		if (!createScene(sceneGraph, node, mesh.mesh[1])) {
			return false;
		}
	}

	core_assert(_state.scene.cameras.empty());
	yocto::add_camera(_state.scene);
	core_assert(_state.scene.environments.empty());
	yocto::add_sky(_state.scene);

	return true;
}

bool PathTracer::start(const scenegraph::SceneGraph &sceneGraph) {
	createScene(sceneGraph);
	_state.bvh = yocto::make_trace_bvh(_state.scene, _state.params);
	_state.lights = yocto::make_trace_lights(_state.scene, _state.params);
	_state.state = yocto::make_trace_state(_state.scene, _state.params);
	yocto::trace_start(_state.context, _state.state, _state.scene, _state.bvh, _state.lights, _state.params);
	_state.started = true;
	return true;
}

bool PathTracer::restart(const scenegraph::SceneGraph &sceneGraph) {
	if (!started()) {
		return false;
	}
	stop();
	return start(sceneGraph);
}

bool PathTracer::stop() {
	yocto::trace_cancel(_state.context);
	_state.started = false;
	return true;
}

bool PathTracer::started() const {
	return _state.started;
}

bool PathTracer::update(int *currentSample) {
	if (!_state.started) {
		*currentSample = 0;
		return true;
	}
	if (yocto::trace_done(_state.context)) {
		if (_state.state.samples >= _state.params.samples) {
			_state.started = false;
			return true;
		}
		if (currentSample) {
			*currentSample = _state.state.samples;
		}
		Log::debug("PathTracer sample: %i", _state.state.samples);
	}
	yocto::trace_start(_state.context, _state.state, _state.scene, _state.bvh, _state.lights, _state.params);
	return false;
}

image::ImagePtr PathTracer::image() {
	yocto::image_data image;
	if (!yocto::trace_done(_state.context)) {
		image = yocto::make_image(_state.state.width, _state.state.height, true);
		yocto::trace_preview(image, _state.context, _state.state, _state.scene, _state.bvh, _state.lights,
							 _state.params);
	} else {
		image = yocto::get_image(_state.state);
	}

	priv::YoctoImageReadStream stream(image);
	image::ImagePtr i = image::createEmptyImage("pathtracer");
	if (!i->loadRGBA(stream, image.width, image.height)) {
		return {};
	}
	return i;
}

} // namespace voxelpathtracer
