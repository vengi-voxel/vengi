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
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "video/Camera.h"
#include "voxel/ChunkMesh.h"
#include "voxel/Mesh.h"
#include "voxel/RawVolume.h"
#include "voxel/SurfaceExtractor.h"
#include "voxelrender/SceneGraphRenderer.h"
#include "yocto_scene.h"

namespace voxelpathtracer {

namespace priv {

static inline yocto::vec3f toVec3f(const glm::vec3 &in) {
	return yocto::vec3f{in.x, in.y, in.z};
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
	// const palette::Palette &palette = node.palette();
	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	const scenegraph::SceneGraphTransform &transform = node.transform(keyFrameIdx);
	const voxel::IndexArray &indices = mesh.getIndexVector();
	if (indices.empty()) {
		return true;
	}
	core_assert((int)indices.size() % 3 == 0);
	const int tris = (int)indices.size() / 3;
	yocto::shape_data shapes[palette::PaletteMaxColors];
	const voxel::VertexArray &vertices = mesh.getVertexVector();
	const voxel::NormalArray &normals = mesh.getNormalVector();
	const bool useNormals = normals.size() == vertices.size();

	const voxel::Region &region = sceneGraph.resolveRegion(node);
	const glm::vec3 size = glm::vec3(region.getDimensionsInVoxels());
	const glm::vec3 pivot = node.pivot();

	for (int i = 0; i < tris; i++) {
		const voxel::VoxelVertex &vertex0 = vertices[indices[i * 3 + 0]];
		const voxel::VoxelVertex &vertex1 = vertices[indices[i * 3 + 1]];
		const voxel::VoxelVertex &vertex2 = vertices[indices[i * 3 + 2]];

		const glm::vec3 pos0 = transform.apply(vertex0.position, pivot * size);
		const glm::vec3 pos1 = transform.apply(vertex1.position, pivot * size);
		const glm::vec3 pos2 = transform.apply(vertex2.position, pivot * size);
		// uv is the same for all three vertices
		// const glm::vec2 &uv = image::Image::uv(vertex0.colorIndex, 0, palette::PaletteMaxColors, 1);
		yocto::shape_data *shape = &shapes[vertex0.colorIndex];

		// const core::RGBA rgba = palette.color(vertex0.colorIndex);
		// const glm::vec4 &color = core::Color::fromRGBA(rgba);
		// shape->colors.push_back(priv::toColor(color, vertex0.ambientOcclusion));
		// shape->colors.push_back(priv::toColor(color, vertex1.ambientOcclusion));
		// shape->colors.push_back(priv::toColor(color, vertex2.ambientOcclusion));

		shape->positions.push_back(priv::toVec3f(pos0));
		shape->positions.push_back(priv::toVec3f(pos1));
		shape->positions.push_back(priv::toVec3f(pos2));
		// shape->texcoords.push_back({uv[0], uv[1]});
		// shape->texcoords.push_back({uv[0], uv[1]});
		// shape->texcoords.push_back({uv[0], uv[1]});
		if (useNormals) {
			shape->normals.push_back(priv::toVec3f(normals[indices[i * 3 + 0]]));
			shape->normals.push_back(priv::toVec3f(normals[indices[i * 3 + 1]]));
			shape->normals.push_back(priv::toVec3f(normals[indices[i * 3 + 2]]));
		}

		const int offsetStart = (int)shape->triangles.size() * 3;
		const yocto::vec3i vidx{offsetStart + 0, offsetStart + 1, offsetStart + 2};
		shape->triangles.push_back(vidx);
	}
	// const glm::vec3 &mins = mesh.getOffset();
	_state.scene.shapes.reserve(palette::PaletteMaxColors);
	for (int i = 0; i < palette::PaletteMaxColors; ++i) {
		yocto::shape_data &shape = shapes[i];
		if (shape.triangles.empty()) {
			continue;
		}
		_state.scene.shapes.push_back(shape);

		yocto::instance_data instance_data;
		// instance_data.frame = yocto::translation_frame(priv::toVec3f(mins));
		instance_data.material = _state.scene.materials.size() + i;
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

	const yocto::vec3f &from = priv::toVec3f(cam.eye());
	const yocto::vec3f &to = priv::toVec3f(cam.target());
	const yocto::vec3f &up = priv::toVec3f(cam.up());
	camera.frame = yocto::lookat_frame(from, to, up);
	camera.aspect = cam.aspect();

	camera.orthographic = cam.mode() == video::CameraMode::Orthogonal;
	if (camera.orthographic) {
		camera.film = cam.size().x;
		if (cam.rotationType() == video::CameraRotationType::Target) {
			camera.focus = cam.targetDistance();
		} else {
			camera.focus = cam.farPlane();
		}
		camera.lens = camera.film / camera.focus;
	} else {
		camera.film = 0.036f;
		float distance = camera.film / (2.0f * glm::tan(cam.fieldOfView() / 2.0f));
		if (camera.aspect > 1.0f) {
			distance /= camera.aspect;
		}
		if (cam.rotationType() == video::CameraRotationType::Target) {
			camera.focus = cam.targetDistance();
		} else {
			camera.focus = cam.farPlane();
		}
		camera.lens = camera.focus * distance / (camera.focus + distance);
	}
}

static yocto::material_type mapMaterialType(palette::MaterialType type) {
	// https://xelatihy.github.io/yocto-gl/yocto/yocto_scene/#materials
	switch (type) {
		case palette::MaterialType::Diffuse:
			return yocto::material_type::matte;
		case palette::MaterialType::Emit:
			return yocto::material_type::volumetric;
		case palette::MaterialType::Metal:
			return yocto::material_type::reflective;
		case palette::MaterialType::Glass:
			return yocto::material_type::refractive;
		case palette::MaterialType::Blend:
			return yocto::material_type::transparent;
		case palette::MaterialType::Media:
			return yocto::material_type::subsurface;
	}
	return yocto::material_type::matte;
}

static void setupMaterial(yocto::scene_data &scene, const palette::Palette &palette, int i) {
	const palette::Material &ownMaterial = palette.material(i);

	yocto::material_data material;
	material.type = mapMaterialType(ownMaterial.type);
	const glm::vec4 color = core::Color::fromRGBA(palette.color(i));
	material.color = priv::toVec3f(color);
	if (ownMaterial.has(palette::MaterialProperty::MaterialEmit)) {
		material.scattering = priv::toVec3f(core::Color::fromRGBA(palette.emitColor(i)));
		if (material.type == yocto::material_type::matte) {
			material.type = yocto::material_type::volumetric;
		}
	}
	if (ownMaterial.has(palette::MaterialProperty::MaterialMetal)) {
		material.metallic = ownMaterial.value(palette::MaterialProperty::MaterialMetal);
	}
	if (ownMaterial.has(palette::MaterialProperty::MaterialRoughness)) {
		material.roughness = ownMaterial.value(palette::MaterialProperty::MaterialRoughness);
	}
	if (ownMaterial.has(palette::MaterialProperty::MaterialIndexOfRefraction)) {
		material.ior = ownMaterial.value(palette::MaterialProperty::MaterialIndexOfRefraction);
	}
	material.opacity = color.a;
	// TODO: map these
	// material.emission
	// material.scanisotropy
	// material.trdepth
	scene.materials.push_back(material);
}

#if 0
static void addEmissiveTexture(yocto::scene_data &scene, const palette::Palette &palette) {
	yocto::texture_data texture;
	texture.height = 1;
	texture.width = palette::PaletteMaxColors;
	for (int i = 0; i < palette.colorCount(); ++i) {
		const core::RGBA color = palette.emitColor(i);
		texture.pixelsb.push_back({color.r, color.g, color.b, color.a});
	}
	for (int i = palette.colorCount(); i < texture.width; ++i) {
		texture.pixelsb.push_back({});
	}
	scene.textures.push_back(texture);
}

static void addPaletteTexture(yocto::scene_data &scene, const palette::Palette &palette) {
	yocto::texture_data texture;
	texture.height = 1;
	texture.width = palette::PaletteMaxColors;
	for (int i = 0; i < palette.colorCount(); ++i) {
		const core::RGBA color = palette.color(i);
		texture.pixelsb.push_back({color.r, color.g, color.b, color.a});
	}
	for (int i = palette.colorCount(); i < texture.width; ++i) {
		texture.pixelsb.push_back({});
	}
	scene.textures.push_back(texture);
}
#endif

bool PathTracer::createScene(const scenegraph::SceneGraph &sceneGraph, const video::Camera *camera) {
	_state.scene = {};
	_state.lights = {};

	voxel::SurfaceExtractionType type = (voxel::SurfaceExtractionType)core::Var::getSafe(cfg::VoxelMeshMode)->intVal();
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

		const palette::Palette &palette = node.palette();
		voxel::SurfaceExtractionContext ctx =
			voxel::createContext(type, v, region, palette, mesh, region.getLowerCorner());

		voxel::extractSurface(ctx);

		// scenegraph::KeyFrameIndex keyFrameIdx = 0;
		// const scenegraph::SceneGraphTransform &transform = node.transform(keyFrameIdx);
		if (!createScene(sceneGraph, node, mesh.mesh[0], true)) {
			return false;
		}
		if (!createScene(sceneGraph, node, mesh.mesh[1], false)) {
			return false;
		}

		// addPaletteTexture(_state.scene, palette);
		// addEmissiveTexture(_state.scene, palette);

		for (int i = 0; i < palette.colorCount(); ++i) {
			setupMaterial(_state.scene, palette, i);
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
		if (currentSample) {
			*currentSample = 0;
		}
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
	image = yocto::get_image(_state.state);

	priv::YoctoImageReadStream stream(image);
	image::ImagePtr i = image::createEmptyImage("pathtracer");
	if (!i->loadRGBA(stream, image.width, image.height)) {
		return {};
	}
	return i;
}

} // namespace voxelpathtracer
