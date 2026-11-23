/**
 * @file
 */

#include "CrocotileFormat.h"
#include "MeshMaterial.h"
#include "Polygon.h"
#include "color/ColorUtil.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/collection/Map.h"
#include "image/Image.h"
#include "io/Base64ReadStream.h"
#include "io/BufferedReadWriteStream.h"
#include "io/MemoryReadStream.h"
#include "json/JSON.h"
#include "scenegraph/SceneGraph.h"
#include "math/Mat4f.h"
#include "math/Vec4f.h"
#include "math/Vec3f.h"
#include "math/Vec2f.h"
#include "math/Mat4Ops.h"
#include "math/Scalar.h"


namespace voxelformat {

namespace croco_priv {

static math::Vec3f toVec3(const json::Json &json) {
	if (json.isArray() && json.size() >= 3) {
		return math::Vec3f(json.get(0).floatVal(), json.get(1).floatVal(), json.get(2).floatVal());
	}
	return math::Vec3f(json.floatVal("x", 0.0f), json.floatVal("y", 0.0f), json.floatVal("z", 0.0f));
}

static math::Vec2f toVec2(const json::Json &json) {
	if (json.isArray() && json.size() >= 2) {
		return math::Vec2f(json.get(0).floatVal(), json.get(1).floatVal());
	}
	return math::Vec2f(json.floatVal("x", 0.0f), json.floatVal("y", 0.0f));
}

static color::RGBA toColor(const json::Json &json, color::RGBA fallback = color::RGBA(255, 255, 255, 255)) {
	if (!json.isValid() || json.isNull()) {
		return fallback;
	}
	const float r = json.floatVal("r", 1.0f);
	const float g = json.floatVal("g", 1.0f);
	const float b = json.floatVal("b", 1.0f);
	const float a = json.floatVal("a", 1.0f);
	return color::getRGBA(math::clamp(math::Vec4f(r, g, b, a), 0.0f, 1.0f));
}

static image::ImagePtr loadDataUrlImage(const core::String &source, const core::String &name) {
	if (!core::string::startsWith(source, "data:")) {
		return image::ImagePtr{};
	}
	const size_t mimetypeEndPos = source.find(";");
	if (mimetypeEndPos == core::String::npos) {
		Log::warn("No mimetype found in texture source for %s", name.c_str());
		return image::ImagePtr{};
	}
	const size_t encodingEnd = source.find(",");
	if (encodingEnd == core::String::npos) {
		Log::warn("No encoding found in texture source for %s", name.c_str());
		return image::ImagePtr{};
	}
	const core::String encoding = source.substr(mimetypeEndPos + 1, encodingEnd - mimetypeEndPos - 1);
	if (encoding != "base64") {
		Log::warn("Unsupported texture encoding '%s' for %s", encoding.c_str(), name.c_str());
		return image::ImagePtr{};
	}
	const core::String data = source.substr(encodingEnd + 1);
	if (data.size() < 16) {
		return image::ImagePtr{};
	}
	io::MemoryReadStream dataStream(data.c_str(), data.size());
	io::Base64ReadStream base64Stream(dataStream);
	io::BufferedReadWriteStream bufferedStream(base64Stream, data.size());
	image::ImagePtr image = image::loadImage(name, bufferedStream);
	if (!image || !image->isLoaded()) {
		Log::warn("Failed to decode embedded texture: %s", name.c_str());
		return image::ImagePtr{};
	}
	return image;
}

static math::Mat4f rotationMatrix(const json::Json &rotationJson) {
	if (!rotationJson.isValid() || !rotationJson.isObject()) {
		return math::Mat4f(1.0f);
	}
	const float x = rotationJson.floatVal("_x", rotationJson.floatVal("x", 0.0f));
	const float y = rotationJson.floatVal("_y", rotationJson.floatVal("y", 0.0f));
	const float z = rotationJson.floatVal("_z", rotationJson.floatVal("z", 0.0f));
	const core::String order = json::toStr(rotationJson, "_order", "XYZ");
	if (order == "YXZ") {
		return math::eulerAngleYXZ(y, x, z);
	}
	if (order == "ZXY") {
		return math::eulerAngleZXY(z, x, y);
	}
	if (order == "ZYX") {
		return math::eulerAngleZYX(z, y, x);
	}
	if (order == "YZX") {
		return math::eulerAngleYZX(y, z, x);
	}
	if (order == "XZY") {
		return math::eulerAngleXZY(x, z, y);
	}
	// Default Three.js / Crocotile order
	return math::eulerAngleXYZ(x, y, z);
}

static math::Mat4f localTransform(const json::Json &instanceJson) {
	const math::Vec3f pos = instanceJson.contains("position") ? toVec3(instanceJson.get("position")) : math::Vec3f(0.0f);
	math::Vec3f scale(1.0f);
	if (instanceJson.contains("scale")) {
		scale = toVec3(instanceJson.get("scale"));
		if (math::all(math::epsilonEqual(scale, math::Vec3f(0.0f), 0.00001f))) {
			scale = math::Vec3f(1.0f);
		}
	}
	math::Mat4f m(1.0f);
	m = math::translate(m, pos);
	if (instanceJson.contains("rotation")) {
		m *= rotationMatrix(instanceJson.get("rotation"));
	}
	m = math::scale(m, scale);
	return m;
}

struct InstanceInfo {
	int id = -1;
	int parentId = -1;
	math::Mat4f local{1.0f};
	math::Mat4f world{1.0f};
	bool worldComputed = false;
};

static math::Mat4f computeWorld(core::Map<int, InstanceInfo> &instances, int id) {
	auto iter = instances.find(id);
	if (iter == instances.end()) {
		return math::Mat4f(1.0f);
	}
	InstanceInfo &info = iter->value;
	if (info.worldComputed) {
		return info.world;
	}
	info.worldComputed = true;
	math::Mat4f parent(1.0f);
	if (info.parentId >= 0 && instances.hasKey(info.parentId)) {
		parent = computeWorld(instances, info.parentId);
	}
	info.world = parent * info.local;
	return info.world;
}

static void emitTri(Mesh &mesh, int materialIdx, const math::Vec3f &v0, const math::Vec3f &v1, const math::Vec3f &v2,
					const math::Vec2f &uv0, const math::Vec2f &uv1, const math::Vec2f &uv2, color::RGBA c0, color::RGBA c1,
					color::RGBA c2) {
	Polygon polygon;
	if (materialIdx >= 0) {
		polygon.setMaterialIndex(materialIdx);
	}
	polygon.addVertex(v0, uv0, c0);
	polygon.addVertex(v1, uv1, c1);
	polygon.addVertex(v2, uv2, c2);
	polygon.toTris(mesh);
}

/**
 * Match the reference crocotile_to_obj.py export: world = position + vertices, raw UVs
 * (no V flip - Image::colorAt already maps OpenGL-style V), faces as stored.
 */
static void appendTile(Mesh &mesh, const json::Json &tileJson, const math::Mat4f &transform, int defaultMaterialIdx) {
	if (!tileJson.contains("vertices") || !tileJson.contains("faces") || !tileJson.contains("uvs")) {
		return;
	}
	const json::Json verticesJson = tileJson.get("vertices");
	const json::Json facesJson = tileJson.get("faces");
	const json::Json uvsJson = tileJson.get("uvs");
	if (!verticesJson.isArray() || !facesJson.isArray() || !uvsJson.isArray()) {
		return;
	}

	const math::Vec3f tilePos = tileJson.contains("position") ? toVec3(tileJson.get("position")) : math::Vec3f(0.0f);
	int materialIdx = defaultMaterialIdx;
	if (tileJson.contains("texture")) {
		materialIdx = tileJson.intVal("texture", defaultMaterialIdx);
	}
	if (materialIdx < 0 || materialIdx >= (int)mesh.materials.size()) {
		materialIdx = defaultMaterialIdx;
		if (materialIdx < 0 || materialIdx >= (int)mesh.materials.size()) {
			materialIdx = -1;
		}
	}

	core::DynamicArray<math::Vec3f> positions;
	core::DynamicArray<color::RGBA> colors;
	positions.reserve(verticesJson.size());
	colors.reserve(verticesJson.size());

	const bool hasColors = tileJson.contains("colors") && tileJson.get("colors").isArray();
	const json::Json colorsJson = hasColors ? tileJson.get("colors") : json::Json();

	for (int i = 0; i < verticesJson.size(); ++i) {
		const math::Vec3f local = tilePos + toVec3(verticesJson.get(i));
		const math::Vec4f world = transform * math::Vec4f(local, 1.0f);
		positions.push_back(math::Vec3f(world));
		if (hasColors && i < colorsJson.size()) {
			colors.push_back(toColor(colorsJson.get(i)));
		} else {
			colors.push_back(color::RGBA(255, 255, 255, 255));
		}
	}

	const int faceCount = core_min(facesJson.size(), uvsJson.size());
	for (int fi = 0; fi < faceCount; ++fi) {
		const json::Json face = facesJson.get(fi);
		const json::Json faceUVs = uvsJson.get(fi);
		if (!face.isArray() || face.size() < 3 || !faceUVs.isArray() || faceUVs.size() < 3) {
			continue;
		}

		int idx[3];
		math::Vec2f uv[3];
		bool valid = true;
		for (int vi = 0; vi < 3; ++vi) {
			idx[vi] = face.get(vi).intVal();
			if (idx[vi] < 0 || idx[vi] >= (int)positions.size()) {
				valid = false;
				break;
			}
			uv[vi] = toVec2(faceUVs.get(vi));
		}
		if (!valid) {
			continue;
		}

		emitTri(mesh, materialIdx, positions[idx[0]], positions[idx[1]], positions[idx[2]], uv[0], uv[1], uv[2],
				colors[idx[0]], colors[idx[1]], colors[idx[2]]);
	}
}

} // namespace croco_priv

bool CrocotileFormat::voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
									 scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Failed to open crocotile file: %s", filename.c_str());
		return false;
	}

	core::String jsonString;
	stream->readString(stream->remaining(), jsonString);
	json::Json root = json::Json::parse(jsonString);
	if (!root.isValid() || !root.isObject()) {
		Log::error("Failed to parse crocotile JSON: %s", filename.c_str());
		return false;
	}
	if (!root.contains("model") || !root.get("model").isArray()) {
		Log::error("Crocotile file missing model array: %s", filename.c_str());
		return false;
	}

	const json::Json models = root.get("model");
	MeshMaterialArray materials;
	materials.reserve(models.size());

	for (int mi = 0; mi < models.size(); ++mi) {
		const json::Json model = models.get(mi);
		core::String texName = core::String::format("tileset_%i", mi);
		if (model.contains("imgFile") && model.get("imgFile").isObject()) {
			const core::String name = json::toStr(model.get("imgFile"), "name");
			if (!name.empty()) {
				texName = name;
			}
		}
		image::ImagePtr image;
		if (model.contains("texture")) {
			image = croco_priv::loadDataUrlImage(json::toStr(model, "texture"), texName);
		}
		if (image && image->isLoaded()) {
			MeshMaterialPtr mat = createMaterial(image);
			const core::String wrap = json::toStr(model, "imgWrap", "repeat");
			if (wrap == "edge") {
				mat->wrapS = image::TextureWrap::ClampToEdge;
				mat->wrapT = image::TextureWrap::ClampToEdge;
			} else if (wrap == "mirror") {
				mat->wrapS = image::TextureWrap::MirroredRepeat;
				mat->wrapT = image::TextureWrap::MirroredRepeat;
			}
			materials.push_back(mat);
		} else {
			materials.push_back(MeshMaterialPtr{});
			Log::debug("No texture for tileset %i in %s", mi, filename.c_str());
		}
	}

	Mesh mesh;
	mesh.materials = materials;

	// Scene tiles stored directly on tilesets
	for (int mi = 0; mi < models.size(); ++mi) {
		const json::Json model = models.get(mi);
		if (!model.contains("object") || !model.get("object").isArray()) {
			continue;
		}
		const json::Json objects = model.get("object");
		for (int ti = 0; ti < objects.size(); ++ti) {
			croco_priv::appendTile(mesh, objects.get(ti), math::Mat4f(1.0f), mi);
		}
	}

	// Prefab instances (with optional parent hierarchy)
	if (root.contains("prefabs") && root.get("prefabs").isArray()) {
		const json::Json prefabs = root.get("prefabs");
		core::Map<int, croco_priv::InstanceInfo> instances(128);

		for (int pi = 0; pi < prefabs.size(); ++pi) {
			const json::Json prefab = prefabs.get(pi);
			const core::String type = json::toStr(prefab, "type", "object");
			if (type != "object") {
				continue;
			}
			if (prefab.contains("visible") && !prefab.boolVal("visible", true)) {
				continue;
			}
			if (!prefab.contains("instances") || !prefab.get("instances").isArray()) {
				continue;
			}
			const json::Json instArr = prefab.get("instances");
			for (int ii = 0; ii < instArr.size(); ++ii) {
				const json::Json inst = instArr.get(ii);
				croco_priv::InstanceInfo info;
				info.id = inst.intVal("id", -1);
				info.parentId = inst.intVal("parentID", -1);
				info.local = croco_priv::localTransform(inst);
				if (info.id >= 0) {
					const int idCopy = info.id;
					instances.put(idCopy, info);
				}
			}
		}

		for (auto iter = instances.begin(); iter != instances.end(); ++iter) {
			croco_priv::computeWorld(instances, iter->key);
		}

		for (int pi = 0; pi < prefabs.size(); ++pi) {
			const json::Json prefab = prefabs.get(pi);
			const core::String type = json::toStr(prefab, "type", "object");
			if (type != "object") {
				continue;
			}
			if (prefab.contains("visible") && !prefab.boolVal("visible", true)) {
				continue;
			}
			if (!prefab.contains("object") || !prefab.get("object").isArray()) {
				continue;
			}
			if (!prefab.contains("instances") || !prefab.get("instances").isArray()) {
				continue;
			}
			const json::Json tiles = prefab.get("object");
			const json::Json instArr = prefab.get("instances");
			for (int ii = 0; ii < instArr.size(); ++ii) {
				const json::Json inst = instArr.get(ii);
				math::Mat4f world(1.0f);
				const int id = inst.intVal("id", -1);
				if (id >= 0) {
					auto iter = instances.find(id);
					if (iter != instances.end()) {
						world = iter->value.world;
					} else {
						world = croco_priv::localTransform(inst);
					}
				} else {
					world = croco_priv::localTransform(inst);
				}
				const int defaultMat = materials.size() > 1 ? 1 : 0;
				for (int ti = 0; ti < tiles.size(); ++ti) {
					croco_priv::appendTile(mesh, tiles.get(ti), world, defaultMat);
				}
			}
		}
	}

	if (mesh.vertices.empty() && mesh.indices.empty() && mesh.polygons.empty()) {
		Log::error("No geometry found in crocotile file: %s", filename.c_str());
		return false;
	}

	const core::String name = core::string::extractFilename(filename);
	const int nodeId = importMesh(name, sceneGraph, core::move(mesh), 0, true, ctx.progress);
	if (nodeId == InvalidNodeId) {
		Log::error("Failed to voxelize crocotile mesh: %s", filename.c_str());
		return false;
	}

	if (root.contains("config") && root.get("config").isObject()) {
		const json::Json config = root.get("config");
		scenegraph::SceneGraphNode &rootNode = sceneGraph.node(sceneGraph.root().id());
		rootNode.setProperty("tilesizeX", core::string::toString(config.intVal("tilesizeX", 16)));
		rootNode.setProperty("tilesizeY", core::string::toString(config.intVal("tilesizeY", 16)));
		if (config.contains("baseUnit")) {
			rootNode.setProperty("baseUnit", core::string::toString(config.intVal("baseUnit", 16)));
		}
	}

	return true;
}

} // namespace voxelformat
