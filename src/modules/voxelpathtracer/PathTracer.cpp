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
#include "video/Camera.h"
#include "voxel/ChunkMesh.h"
#include "voxel/Mesh.h"
#include "voxel/Palette.h"
#include "voxel/RawVolume.h"
#include "voxel/SurfaceExtractor.h"
#include "voxelrender/SceneGraphRenderer.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelpathtracer {

namespace priv {

static inline yocto::vec3f toVec3f(const glm::vec3 &in) {
	return yocto::vec3f{in.x, in.y, in.z};
}

static inline yocto::vec4f toColor(const glm::vec4 &color, uint8_t ao) {
	// matches voxel.frag values
	static const float aoValues[]{0.15f, 0.6f, 0.8f, 1.0f};
	return yocto::vec4f{color.r * aoValues[ao], color.g * aoValues[ao], color.b * aoValues[ao], color.a};
}

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
							 const voxel::Mesh &mesh, bool opaque) {
	const voxel::Palette &palette = node.palette();
	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	const scenegraph::SceneGraphTransform &transform = node.transform(keyFrameIdx);
	const voxel::IndexArray &indices = mesh.getIndexVector();
	if (indices.empty()) {
		return true;
	}
	core_assert((int)indices.size() % 3 == 0);
	const int tris = (int)indices.size() / 3;
	yocto::shape_data nonGlow;
	yocto::shape_data glow;
	const voxel::VertexArray &vertices = mesh.getVertexVector();
	const voxel::NormalArray &normals = mesh.getNormalVector();
	const bool useNormals = normals.size() == vertices.size();
	nonGlow.colors.reserve(vertices.size());
	nonGlow.positions.reserve(vertices.size());
	nonGlow.texcoords.reserve(vertices.size());
	nonGlow.triangles.reserve(tris);
	glow.colors.reserve(vertices.size());
	glow.positions.reserve(vertices.size());
	glow.texcoords.reserve(vertices.size());
	glow.triangles.reserve(tris);
	if (useNormals) {
		nonGlow.normals.reserve(normals.size());
		glow.normals.reserve(normals.size());
	}

	const glm::vec3 size = glm::vec3(sceneGraph.resolveRegion(node).getDimensionsInVoxels());
	const glm::vec3 pivot = sceneGraph.resolvePivot(node);

	for (int i = 0; i < tris; i++) {
		const voxel::VoxelVertex &vertex0 = vertices[indices[i * 3 + 0]];
		const voxel::VoxelVertex &vertex1 = vertices[indices[i * 3 + 1]];
		const voxel::VoxelVertex &vertex2 = vertices[indices[i * 3 + 2]];

		const glm::vec3 pos0 = transform.apply(vertex0.position, pivot * size);
		const glm::vec3 pos1 = transform.apply(vertex1.position, pivot * size);
		const glm::vec3 pos2 = transform.apply(vertex2.position, pivot * size);
		// uv is the same for all three vertices
		const glm::vec2 &uv = image::Image::uv(vertex0.colorIndex, 0, voxel::PaletteMaxColors, 1);
		yocto::shape_data *shape;
		// color is the same for all three vertices
		core::RGBA rgba;
		if (palette.hasGlow(vertex0.colorIndex)) {
			rgba = palette.glowColor(vertex0.colorIndex);
			shape = &glow;
		} else {
			rgba = palette.color(vertex0.colorIndex);
			shape = &nonGlow;
		}

		const glm::vec4 &color = core::Color::fromRGBA(rgba);
		shape->colors.push_back(priv::toColor(color, vertex0.ambientOcclusion));
		shape->colors.push_back(priv::toColor(color, vertex1.ambientOcclusion));
		shape->colors.push_back(priv::toColor(color, vertex2.ambientOcclusion));
		shape->positions.push_back(priv::toVec3f(pos0));
		shape->positions.push_back(priv::toVec3f(pos1));
		shape->positions.push_back(priv::toVec3f(pos2));
		shape->texcoords.push_back({uv[0], uv[1]});
		shape->texcoords.push_back({uv[0], uv[1]});
		shape->texcoords.push_back({uv[0], uv[1]});
		if (useNormals) {
			shape->normals.push_back(priv::toVec3f(normals[indices[i * 3 + 0]]));
			shape->normals.push_back(priv::toVec3f(normals[indices[i * 3 + 1]]));
			shape->normals.push_back(priv::toVec3f(normals[indices[i * 3 + 2]]));
		}

		const int offsetStart = (int)shape->triangles.size() * 3;
		const yocto::vec3i vidx{offsetStart + 0, offsetStart + 1, offsetStart + 2};
		shape->triangles.push_back(vidx);
	}
	const glm::vec3 &mins = mesh.getOffset();
	yocto::instance_data instance_data;
	instance_data.frame = yocto::translation_frame(priv::toVec3f(mins));

	if (!nonGlow.triangles.empty()) {
		_state.scene.shapes.push_back(nonGlow);
		instance_data.material = opaque ? _state.materialOpaque : _state.materialTransparent;
		instance_data.shape = (int)_state.scene.shapes.size() - 1;
		_state.scene.instances.push_back(instance_data);
	}
	if (!glow.triangles.empty()) {
		_state.scene.shapes.push_back(glow);
		instance_data.material = _state.materialGlow;
		instance_data.shape = (int)_state.scene.shapes.size() - 1;
		_state.scene.instances.push_back(instance_data);
	}

	return true;
}

void PathTracer::addCamera(const scenegraph::SceneGraphNodeCamera &node) {
	addCamera(node.name().c_str(), voxelrender::toCamera({}, node));
}

void PathTracer::addCamera(const char *name, const video::Camera &cam) {
	yocto::scene_data &scene = _state.scene;
	scene.camera_names.emplace_back(name);
	yocto::camera_data &camera = scene.cameras.emplace_back();
	camera.orthographic = cam.mode() == video::CameraMode::Orthogonal;
	camera.film = 0.036f;
	camera.aspect = cam.aspect();
	camera.aperture = 0;
	camera.lens = 0.050f;
	const yocto::vec3f &from = priv::toVec3f(cam.eye());
	const yocto::vec3f &to = priv::toVec3f(cam.target());
	const yocto::vec3f &up = priv::toVec3f(cam.up());
	camera.frame = yocto::lookat_frame(from, to, up);
	camera.focus = cam.targetDistance() * 2.0f;
}

void PathTracer::setupBaseColorMaterial(const scenegraph::SceneGraphNode &node) {
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

	yocto::material_data materialOpaque;
	materialOpaque.type = yocto::material_type::matte;
	materialOpaque.color = {1.0f, 1.0f, 1.0f};
	materialOpaque.emission = priv::toVec3f(glm::clamp(_state.ambientColor + _state.diffuseColor, 0.0f, 1.0f));
	materialOpaque.color_tex = (int)_state.scene.textures.size() - 1;
	_state.scene.materials.push_back(materialOpaque);
	_state.materialOpaque = (int)_state.scene.materials.size() - 1;

	yocto::material_data materialTransparent;
	materialTransparent.type = yocto::material_type::transparent;
	materialTransparent.color = {1.0f, 1.0f, 1.0f};
	materialTransparent.emission = priv::toVec3f(glm::clamp(_state.ambientColor + _state.diffuseColor, 0.0f, 1.0f));
	materialTransparent.color_tex = (int)_state.scene.textures.size() - 1;
	_state.scene.materials.push_back(materialTransparent);
	_state.materialTransparent = (int)_state.scene.materials.size() - 1;
}

void PathTracer::setupGlowMaterial(const scenegraph::SceneGraphNode &node) {
	yocto::texture_data texture;
	texture.height = 1;
	texture.width = voxel::PaletteMaxColors;
	for (int i = 0; i < node.palette().colorCount(); ++i) {
		const core::RGBA color = node.palette().glowColor(i);
		texture.pixelsb.push_back({color.r, color.g, color.b, color.a});
	}
	for (int i = node.palette().colorCount(); i < texture.width; ++i) {
		texture.pixelsb.push_back({});
	}
	_state.scene.textures.push_back(texture);

	yocto::material_data material;
	material.type = yocto::material_type::volumetric;
	material.color = {1.0f, 1.0f, 1.0f};
	material.emission = priv::toVec3f(glm::clamp(_state.ambientColor + _state.diffuseColor, 0.0f, 1.0f));
	material.emission_tex = (int)_state.scene.textures.size() - 1;
	_state.scene.materials.push_back(material);
	_state.materialGlow = (int)_state.scene.materials.size() - 1;
}

bool PathTracer::createScene(const scenegraph::SceneGraph &sceneGraph, const video::Camera *camera) {
	_state.scene = {};
	_state.lights = {};

	const bool marchingCubes = core::Var::getSafe(cfg::VoxelMeshMode)->intVal() == 1;
	// TODO: support references
	for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		if (!node.visible()) {
			continue;
		}
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

		setupGlowMaterial(node);
		setupBaseColorMaterial(node);

		scenegraph::KeyFrameIndex keyFrameIdx = 0;
		const scenegraph::SceneGraphTransform &transform = node.transform(keyFrameIdx);
		if (!createScene(sceneGraph, node, mesh.mesh[0], true)) {
			return false;
		}
		if (!createScene(sceneGraph, node, mesh.mesh[1], false)) {
			return false;
		}
	}

	if (camera) {
		addCamera("default", *camera);
	}

	for (auto iter = sceneGraph.begin(scenegraph::SceneGraphNodeType::Camera); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		addCamera(scenegraph::toCameraNode(node));
	}

	if (_state.scene.cameras.size() <= 1) {
		yocto::add_camera(_state.scene);
	}
	yocto::add_sky(_state.scene);

	return true;
}

bool PathTracer::start(const scenegraph::SceneGraph &sceneGraph, const video::Camera *camera) {
	Log::debug("Create scene");
	createScene(sceneGraph, camera);
	_state.bvh = yocto::make_trace_bvh(_state.scene, _state.params);
	_state.lights = yocto::make_trace_lights(_state.scene, _state.params);
	_state.state = yocto::make_trace_state(_state.scene, _state.params);
	yocto::trace_start(_state.context, _state.state, _state.scene, _state.bvh, _state.lights, _state.params);
	_state.started = true;
	Log::debug("Started pathtracer");
	return true;
}

bool PathTracer::restart(const scenegraph::SceneGraph &sceneGraph, const video::Camera *camera) {
	if (!started()) {
		return false;
	}
	Log::debug("Restart pathtracer");
	stop();
	return start(sceneGraph, camera);
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
		yocto::trace_start(_state.context, _state.state, _state.scene, _state.bvh, _state.lights, _state.params);
	}
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
