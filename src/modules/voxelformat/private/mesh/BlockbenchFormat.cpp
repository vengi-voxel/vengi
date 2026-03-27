/**
 * @file
 */

#include "BlockbenchFormat.h"
#include "MeshMaterial.h"
#include "Polygon.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/UUID.h"
#include "image/Image.h"
#include "io/Base64ReadStream.h"
#include "io/BufferedReadWriteStream.h"
#include "io/MemoryReadStream.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "scenegraph/SceneGraphKeyFrame.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeProperties.h"
#include "scenegraph/SceneGraphTransform.h"
#include "util/Version.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxelutil/ImportFace.h"

#include "json/JSON.h"
#include <glm/trigonometric.hpp>
#include <limits>

namespace voxelformat {

namespace priv {

struct KeyFrame {
	core::String channel; // "rotation", "position", "scale"
	core::Buffer<glm::vec3> dataPoints;
	core::UUID uuid;
	float time = 0.0f;
	int color; // none = -1, white = 0, black, red, green, blue, yellow, pink, purple, orange, brown, cyan, gray, lightgray
	scenegraph::InterpolationType interpolation = scenegraph::InterpolationType::Linear;
	bool bezierLinked = false;
	glm::vec3 bezierLeftTime{0.0f};
	glm::vec3 bezierLeftValue{0.0f};
	glm::vec3 bezierRightTime{0.0f};
	glm::vec3 bezierRightValue{0.0f};
};

struct Animator {
	core::UUID uuid;
	core::String name;
	core::String type; // "bone", "cube"
	core::DynamicArray<KeyFrame> keyframes;
};

struct Animation {
	core::UUID uuid;
	core::String name;
	core::String loop; // "loop", "once"
	bool overrideVal;
	bool selected;
	float length;
	int snapping;
	core::DynamicArray<Animator> animators;
};

#define BLOCKBENCH_ANIMATION 1
#if BLOCKBENCH_ANIMATION
static inline scenegraph::InterpolationType toInterpolationType(const json::Json &json, const char *key, const scenegraph::InterpolationType defaultValue = scenegraph::InterpolationType::Linear) {
	const core::String val = json.strVal(key, "");
	if (val.empty()) {
		return defaultValue;
	}
	if (val == "linear") {
		return scenegraph::InterpolationType::Linear;
	} else if (val == "ease_in") {
		return scenegraph::InterpolationType::QuadEaseIn;
	} else if (val == "ease_out") {
		return scenegraph::InterpolationType::QuadEaseOut;
	} else if (val == "ease_in_out") {
		return scenegraph::InterpolationType::QuadEaseInOut;
	} else if (val == "bezier") {
		return scenegraph::InterpolationType::CubicBezier;
	} else if (val == "catmullrom") {
		return scenegraph::InterpolationType::CatmullRom;
	}
	Log::warn("Unsupported interpolation type: %s", val.c_str());
	return defaultValue;
}
#endif

template<class T>
static T toNumber(const json::Json &json, const char *key, T defaultValue) {
	if (!json.contains(key)) {
		return defaultValue;
	}
	json::Json child = json.get(key);
	if (child.isNull()) {
		return defaultValue;
	}
	if (!child.isNumber()) {
		Log::warn("Value is not a number: %s", key);
		return defaultValue;
	}
	return (T)child.doubleVal();
}

static const glm::vec3 toVec3(const json::Json &json, const glm::vec3 &defaultValue = glm::vec3(0.0f)) {
	if (json.isArray() && json.size() == 3) {
		return glm::vec3(json.get(0).floatVal(), json.get(1).floatVal(), json.get(2).floatVal());
	}
	if (!json.contains("x") || !json.contains("y") || !json.contains("z")) {
		return defaultValue;
	}

	// Handle both string and numeric types in data_points
	// Blockbench can serialize values as strings (e.g., "0", "0\n") or numbers
	auto getFloatValue = [](const json::Json &val, float defaultVal) -> float {
		if (val.isNumber()) {
			return val.floatVal();
		} else if (val.isString()) {
			const core::String str = val.str();
			char *end = nullptr;
			const float result = strtof(str.c_str(), &end);
			if (end != str.c_str() && result != HUGE_VALF && result != -HUGE_VALF) {
				return result;
			}
			Log::debug("Failed to parse float from string: '%s'", str.c_str());
			return defaultVal;
		}
		return defaultVal;
	};

	const float x = getFloatValue(json.get("x"), defaultValue.x);
	const float y = getFloatValue(json.get("y"), defaultValue.y);
	const float z = getFloatValue(json.get("z"), defaultValue.z);
	return glm::vec3(x, y, z);
}

static glm::vec3 toVec3(const json::Json &json, const char *key, const glm::vec3 &defaultValue = glm::vec3(0.0f)) {
	if (!json.contains(key)) {
		return defaultValue;
	}
	return toVec3(json.get(key), defaultValue);
}

static BlockbenchFormat::BBElementType toType(const json::Json &json, const char *key) {
	const core::String &type = json::toStr(json, key);
	if (type == "cube") {
		return BlockbenchFormat::BBElementType::Cube;
	} else if (type == "mesh") {
		return BlockbenchFormat::BBElementType::Mesh;
	}
	Log::debug("Unsupported element type: %s", type.c_str());
	return BlockbenchFormat::BBElementType::Max;
}

static bool isSupportModelFormat(const core::String &modelFormat) {
	return modelFormat != "skin";
}

static bool parseMesh(const core::String &filename, const BlockbenchFormat::BBMeta &bbMeta,
					  const json::Json &elementJson, const MeshMaterialArray &meshMaterialArray,
					  BlockbenchFormat::BBElement &bbElement) {
	if (!elementJson.contains("vertices")) {
		Log::error("Element is missing vertices in json file: %s", filename.c_str());
		return false;
	}

	const json::Json &vertices = elementJson.get("vertices");
	if (!vertices.isObject()) {
		Log::error("Vertices is not an object in json file: %s", filename.c_str());
		return false;
	}

	if (!elementJson.contains("faces")) {
		Log::error("Element is missing faces in json file: %s", filename.c_str());
		return false;
	}

	const json::Json &faces = elementJson.get("faces");
	if (!faces.isObject()) {
		Log::error("Faces is not an object in json file: %s", filename.c_str());
		return false;
	}

	for (auto faceIt = faces.begin(); faceIt != faces.end(); ++faceIt) {
		const json::Json faceData = *faceIt;
		if (!faceData.contains("uv")) {
			Log::error("Face is missing uv in json file: %s", filename.c_str());
			return false;
		}

		const json::Json &uv = faceData.get("uv");
		if (!uv.isObject()) {
			Log::error("UV is not an object in json file: %s", filename.c_str());
			return false;
		}

		if (!faceData.contains("vertices")) {
			Log::error("Face is missing vertices in json file: %s", filename.c_str());
			return false;
		}

		const json::Json &faceVertices = faceData.get("vertices");
		if (!faceVertices.isArray()) {
			Log::error("Vertices is not an array in json file: %s", filename.c_str());
			return false;
		}

		const int materialIdx = priv::toNumber(faceData, "texture", -1);
		const bool materialIdxValid = materialIdx >= 0 && materialIdx < (int)meshMaterialArray.size();
		Polygon polygon;
		if (materialIdxValid) {
			polygon.setMaterialIndex(meshMaterialArray[materialIdx]);
		}
		for (const auto &vertex : faceVertices) {
			const core::String vertexName = vertex.str();
			if (!vertices.contains(vertexName.c_str())) {
				Log::error("Vertex not found in json file: %s", filename.c_str());
				return false;
			}
			json::Json vertexData = vertices.get(vertexName.c_str());
			if (!vertexData.isArray() || vertexData.size() != 3) {
				Log::error("Vertex is not an array of size 3 in json file: %s", filename.c_str());
				return false;
			}
			if (!uv.contains(vertexName.c_str())) {
				Log::error("UV not found for vertex in json file: %s", filename.c_str());
				return false;
			}
			json::Json uvData = uv.get(vertexName.c_str());
			if (!uvData.isArray() || uvData.size() != 2) {
				Log::error("UV is not an array of size 2 in json file: %s", filename.c_str());
				return false;
			}
			const glm::vec3 pos(vertexData.get(0).floatVal(), vertexData.get(1).floatVal(), vertexData.get(2).floatVal());
			const int x = uvData.get(0).intVal();
			const int y = uvData.get(1).intVal();
			glm::vec2 uvCoords;
			if (materialIdxValid) {
				uvCoords = meshMaterialArray[materialIdx]->texture ? meshMaterialArray[materialIdx]->texture->uv(x, y) : glm::vec2(0.0f);
			} else {
				uvCoords = glm::vec2(0.0f);
			}
			polygon.addVertex(pos, uvCoords);
		}
		polygon.toTris(bbElement.mesh);
	}
	return true;
}

static bool parseCube(const glm::vec3 &scale, const core::String &filename, const BlockbenchFormat::BBMeta &bbMeta,
					  const json::Json &elementJson, const MeshMaterialArray &meshMaterialArray,
					  BlockbenchFormat::BBElement &bbElement) {
	if (!elementJson.contains("from") || !elementJson.contains("to")) {
		Log::error("Element is missing from or to in json file: %s", filename.c_str());
		return false;
	}

	const json::Json &from = elementJson.get("from");
	const json::Json &to = elementJson.get("to");
	if (!from.isArray() || from.size() != 3 || !to.isArray() || to.size() != 3) {
		Log::error("From or to is not an array of size 3 in json file: %s", filename.c_str());
		return false;
	}

	bbElement.cube.from = scale * priv::toVec3(elementJson, "from");
	bbElement.cube.to = scale * priv::toVec3(elementJson, "to");

	if (!elementJson.contains("faces")) {
		Log::error("Element is missing faces in json file: %s", filename.c_str());
		return false;
	}

	const json::Json &faces = elementJson.get("faces");
	if (!faces.isObject()) {
		Log::error("Faces is not an object in json file: %s", filename.c_str());
		return false;
	}

	for (auto faceIt = faces.begin(); faceIt != faces.end(); ++faceIt) {
		const core::String faceName = faceIt.key();
		voxel::FaceNames faceType = voxel::FaceNames::Max;
		if (faceName == "north") {
			faceType = voxel::FaceNames::NegativeZ;
		} else if (faceName == "east") {
			faceType = voxel::FaceNames::PositiveX;
		} else if (faceName == "south") {
			faceType = voxel::FaceNames::PositiveZ;
		} else if (faceName == "west") {
			faceType = voxel::FaceNames::NegativeX;
		} else if (faceName == "up") {
			faceType = voxel::FaceNames::PositiveY;
		} else if (faceName == "down") {
			faceType = voxel::FaceNames::NegativeY;
		} else {
			Log::error("Unsupported face name: %s", faceName.c_str());
			continue;
		}

		const json::Json faceData = *faceIt;
		if (!faceData.contains("uv")) {
			Log::error("Face is missing uv in json file: %s", filename.c_str());
			return false;
		}

		const json::Json &uv = faceData.get("uv");
		if (!uv.isArray() || uv.size() != 4) {
			Log::error("UV is not an array of size 4 in json file: %s", filename.c_str());
			return false;
		}

		int materialIdx = -1;
		if (!meshMaterialArray.empty()) {
			materialIdx = priv::toNumber(faceData, "texture", -1);
			if (materialIdx >= (int)meshMaterialArray.size()) {
				Log::error("Invalid material index: %d", materialIdx);
				return false;
			}
		}

		Log::debug("faceName: %s, materialIdx: %d", faceName.c_str(), materialIdx);
		int uvs[4]{uv.get(0).intVal(), uv.get(1).intVal(), uv.get(2).intVal(), uv.get(3).intVal()};
		if (materialIdx >= 0 && meshMaterialArray[materialIdx]->texture) {
			const glm::vec2 uv0 = meshMaterialArray[materialIdx]->texture->uv(uvs[0], uvs[1]);
			const glm::vec2 uv1 = meshMaterialArray[materialIdx]->texture->uv(uvs[2] - 1, uvs[3] - 1);
			bbElement.cube.faces[(int)faceType].uvs[0] = uv0;
			bbElement.cube.faces[(int)faceType].uvs[1] = uv1;
		}
		bbElement.cube.faces[(int)faceType].textureIndex = materialIdx;
		bbElement.cube.faces[(int)faceType].color = faceData.intVal("color", -1);
	}
	return true;
}

static void computeElementsAABB(const json::Json &elementsJson, glm::vec3 &mins, glm::vec3 &maxs) {
	mins = glm::vec3(std::numeric_limits<float>::max());
	maxs = glm::vec3(std::numeric_limits<float>::lowest());
	for (const auto &elementJson : elementsJson) {
		if (elementJson.contains("from") && elementJson.contains("to")) {
			const glm::vec3 from = priv::toVec3(elementJson, "from");
			const glm::vec3 to = priv::toVec3(elementJson, "to");
			mins = glm::min(mins, glm::min(from, to));
			maxs = glm::max(maxs, glm::max(from, to));
		} else if (elementJson.contains("vertices")) {
			const json::Json &verticesJson = elementJson.get("vertices");
			if (verticesJson.isObject()) {
				for (auto vIt = verticesJson.begin(); vIt != verticesJson.end(); ++vIt) {
					json::Json entry = *vIt;
					if (entry.isArray() && entry.size() == 3) {
						const glm::vec3 pos(entry.get(0).floatVal(), entry.get(1).floatVal(), entry.get(2).floatVal());
						mins = glm::min(mins, pos);
						maxs = glm::max(maxs, pos);
					}
				}
			}
		}
	}
}

static bool parseElements(const glm::vec3 &scale, const core::String &filename, const BlockbenchFormat::BBMeta &bbMeta,
						  const json::Json &elementsJson, const MeshMaterialArray &meshMaterialArray,
						  BlockbenchFormat::BBElementMap &bbElementMap, scenegraph::SceneGraph &sceneGraph) {
	for (const auto &elementJson : elementsJson) {
		BlockbenchFormat::BBElement bbElement;
		bbElement.uuid = core::UUID(json::toStr(elementJson, "uuid"));
		bbElement.name = json::toStr(elementJson, "name");
		bbElement.origin = scale * priv::toVec3(elementJson, "origin");
		bbElement.rotation = priv::toVec3(elementJson, "rotation");
		bbElement.rescale = elementJson.boolVal("rescale", false);
		bbElement.locked = elementJson.boolVal("locked", false);
		bbElement.box_uv = elementJson.boolVal("box_uv", false);
		bbElement.color = priv::toNumber(elementJson, "color", 0);
		bbElement.type = priv::toType(elementJson, "type");
		if (bbElement.type == BlockbenchFormat::BBElementType::Max) {
			bbElement.type = BlockbenchFormat::BBElementType::Cube;
		}

		if (bbElement.type == BlockbenchFormat::BBElementType::Cube) {
			if (!parseCube(scale, filename, bbMeta, elementJson, meshMaterialArray, bbElement)) {
				return false;
			}
		} else if (bbElement.type == BlockbenchFormat::BBElementType::Mesh) {
			if (!parseMesh(filename, bbMeta, elementJson, meshMaterialArray, bbElement)) {
				return false;
			}
		}

		// make a copy here, the element is moved into the map
		const core::UUID uuidCopy = bbElement.uuid;
		bbElementMap.emplace(uuidCopy, core::move(bbElement));
	}
	return true;
}

static bool parseOutliner(const glm::vec3 &scale, const core::String &filename, const BlockbenchFormat::BBMeta &bbMeta,
						  const json::Json &entryJson, BlockbenchFormat::BBNode &bbNode) {
	bbNode.name = json::toStr(entryJson, "name");
	bbNode.uuid = core::UUID(json::toStr(entryJson, "uuid"));
	bbNode.locked = entryJson.boolVal("locked", false);
	bbNode.visible = entryJson.boolVal("visibility", true);
	bbNode.mirror_uv = entryJson.boolVal("mirror_uv", false);
	bbNode.origin = scale * priv::toVec3(entryJson, "origin");
	bbNode.rotation = priv::toVec3(entryJson, "rotation");
	bbNode.color = priv::toNumber(entryJson, "color", 0);
	bbNode.size = priv::toVec3(entryJson, "size", glm::vec3(1.0f));

	Log::debug("Node name: %s (%i references)", bbNode.name.c_str(), (int)bbNode.referenced.size());

	if (!entryJson.contains("children")) {
		return true;
	}
	const json::Json &childrenJson = entryJson.get("children");
	if (childrenJson.empty()) {
		return true;
	}
	if (!childrenJson.isArray()) {
		Log::error("Children is not an array in json file: %s", filename.c_str());
		return false;
	}

	for (auto iter = childrenJson.begin(); iter != childrenJson.end(); ++iter) {
		json::Json child = *iter;
		if (child.isString()) {
			core::UUID uuid = core::UUID(json::toStr(child));
			bbNode.referenced.push_back(uuid);
			continue;
		}
		if (!child.isObject()) {
			Log::error("Child entry is not an object in json file: %s", filename.c_str());
			return false;
		}
		BlockbenchFormat::BBNode bbChildNode;
		if (!parseOutliner(scale, filename, bbMeta, child, bbChildNode)) {
			return false;
		}
		bbNode.children.push_back(bbChildNode);
	}
	return true;
}

} // namespace priv

bool BlockbenchFormat::generateMesh(const BBNode &bbNode, BBElement &bbElement, const MeshMaterialArray &meshMaterialArray,
									scenegraph::SceneGraph &sceneGraph, int parent) const {
	Mesh &mesh = bbElement.mesh;
	mesh.materials = meshMaterialArray;
	const int nodeIdx = voxelizeMesh(bbElement.uuid, bbElement.name, sceneGraph, core::move(mesh), parent);
	if (nodeIdx == InvalidNodeId) {
		return false;
	}
	scenegraph::SceneGraphNode &model = sceneGraph.node(nodeIdx);
	model.setLocked(bbNode.locked);
	model.setVisible(bbNode.visible);
	sceneGraph.updateTransforms();
	model.setRotation(glm::quat(glm::radians(bbElement.rotation)), true);
	model.setTranslation(bbElement.origin, true);
	return true;
}

bool BlockbenchFormat::generateCube(const BBNode &bbNode, const BBElement &bbElement, const MeshMaterialArray &meshMaterialArray,
									scenegraph::SceneGraph &sceneGraph, int parent) const {
	const BBCube &cube = bbElement.cube;

	// In Blockbench, 'from' and 'to' define opposite corners of a cube, but they might not be in min/max order.
	// We normalize them to ensure we have proper bounds. This doesn't affect UV coordinates - those are handled separately per face.
	glm::vec3 mins = glm::min(bbElement.cube.from, bbElement.cube.to);
	glm::vec3 maxs = glm::max(bbElement.cube.from, bbElement.cube.to);

	glm::vec3 size = maxs - mins;
	// even a plane is one voxel for us
	size.x = glm::clamp(size.x, 1.0f, 1.0f + size.x);
	size.y = glm::clamp(size.y, 1.0f, 1.0f + size.y);
	size.z = glm::clamp(size.z, 1.0f, 1.0f + size.z);

	mins = glm::round(mins);
	maxs = mins + size - 1.0f;
	voxel::Region region(mins, maxs);
	if (!region.isValid()) {
		Log::error("Invalid region for element: %s (node: %s): %f:%f:%f/%f:%f:%f", bbElement.name.c_str(), bbNode.name.c_str(), mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
		return false;
	}
	scenegraph::SceneGraphNode model(scenegraph::SceneGraphNodeType::Model, bbElement.uuid);
	region.shift(-region.getLowerCorner());
	model.setVolume(new voxel::RawVolume(region));
	model.setName(bbElement.name);
	model.setLocked(bbNode.locked);
	model.setVisible(bbNode.visible);
	model.setRotation(glm::quat(glm::radians(bbElement.rotation)), true);

	// Calculate pivot: In Blockbench, origin is the pivot point in world coordinates.
	// We need to convert it to normalized coordinates relative to the cube's local space.
	// The pivot is the offset from the cube's corner (from) divided by the cube size.
	const glm::vec3 pivot = (bbElement.origin - bbElement.cube.from) / size;
	model.setPivot(pivot);

	// Set translation: Position the cube at its 'from' corner, then offset by the pivot
	// scaled to the voxel region dimensions.
	const glm::vec3 regionsize = region.getDimensionsInVoxels();
	model.setTranslation(bbElement.cube.from + pivot * regionsize, true);
	const voxel::FaceNames order[] = {voxel::FaceNames::NegativeX, voxel::FaceNames::PositiveX,
									  voxel::FaceNames::NegativeY, voxel::FaceNames::PositiveY,
									  voxel::FaceNames::NegativeZ, voxel::FaceNames::PositiveZ};
	for (int i = 0; i < lengthof(order); ++i) {
		const BBCubeFace &face = cube.faces[(int)order[i]];
		const voxel::FaceNames faceName = order[i];
		image::ImagePtr image;
		if (face.textureIndex >= 0 && meshMaterialArray[face.textureIndex]->texture) {
			image = meshMaterialArray[face.textureIndex]->texture;
		}
		const int faceColor = face.color >= 0 ? face.color : bbElement.color;
		voxelutil::importFace(*model.volume(), model.region(), model.palette(), faceName, image, face.uvs[0], face.uvs[1],
				      faceColor);
	}
	model.volume()->translate(-region.getLowerCorner());
	return sceneGraph.emplace(core::move(model), parent) != InvalidNodeId;
}

bool BlockbenchFormat::addNode(const BBNode &bbNode, const BBElementMap &bbElementMap, scenegraph::SceneGraph &sceneGraph,
							   const MeshMaterialArray &meshMaterialArray, int parent) const {
	Log::debug("node: %s with %i children", bbNode.name.c_str(), (int)bbNode.children.size());
	for (const core::UUID &uuid : bbNode.referenced) {
		auto elementIter = bbElementMap.find(uuid);
		if (elementIter == bbElementMap.end()) {
			const core::String uuidStr = uuid.str();
			Log::warn("Could not find node with uuid: %s", uuidStr.c_str());
			continue;
		}
		BBElement &bbElement = elementIter->value;
		if (bbElement.type == BBElementType::Cube) {
			if (!generateCube(bbNode, bbElement, meshMaterialArray, sceneGraph, parent)) {
				return false;
			}
		} else if (bbElement.type == BBElementType::Mesh) {
			if (!generateMesh(bbNode, bbElement, meshMaterialArray, sceneGraph, parent)) {
				return false;
			}
		} else {
			Log::warn("Unsupported element type: %i", (int)bbElement.type);
		}
	}
	for (const BBNode &bbChild : bbNode.children) {
		scenegraph::SceneGraphNode group(scenegraph::SceneGraphNodeType::Group, bbChild.uuid);
		group.setName(bbChild.name);
		group.setVisible(bbChild.visible);
		group.setLocked(bbChild.locked);
		group.setRotation(glm::quat(glm::radians(bbChild.rotation)), true);
		group.setScale(bbChild.size, true);
		group.setTranslation(bbChild.origin, true);
		int groupParent = sceneGraph.emplace(core::move(group), parent);
		if (groupParent == InvalidNodeId) {
			Log::error("Failed to add node: %s", bbChild.name.c_str());
			return false;
		}
		if (!addNode(bbChild, bbElementMap, sceneGraph, meshMaterialArray, groupParent)) {
			return false;
		}
	}
	return true;
}

void BlockbenchFormat::fixNode(BBNode &n) const {
	n.rotation.z = -n.rotation.z;
	for (BBNode &c : n.children) {
		fixNode(c);
	}
}

void BlockbenchFormat::processCompatibility(const BBMeta &meta, BBElementMap &elementMap, BBNode &root) const {
	// Compatibility notes:
	// The handling here is based on observed differences in historical Blockbench bbmodel formats.
	// See Blockbench bbmodel docs and the Blockbench source for format handling:
	//  - https://www.blockbench.net/wiki/docs/bbmodel
	//  - https://github.com/JannisX11/blockbench/blob/master/js/io/formats/bbmodel.js
	// Historically (pre-3.2) the Z-axis rotation was inverted compared to later versions; apply an inversion
	// for older format versions so models authored in those versions appear correctly.

	const int maj = meta.version.majorVersion;
	const int min = meta.version.minorVersion;
	if (maj <= 0 && min <= 0) {
		// unknown version, nothing to do
		return;
	}
	if (maj < 3 || (maj == 3 && min < 2)) {
		// Flip Z rotation for elements
		for (auto iter = elementMap.begin(); iter != elementMap.end(); ++iter) {
			auto *kv = *iter; // KeyValue*
			kv->value.rotation.z = -kv->value.rotation.z;
		}

		// Flip Z rotation recursively for nodes.
		fixNode(root);
	}
}

static bool parseAnimations(const core::String &filename, const BlockbenchFormat::BBMeta &bbMeta, json::Json &json,
							scenegraph::SceneGraph &sceneGraph) {
	// no animations found
	if (!json.contains("animations")) {
		return true;
	}

	const json::Json &animationsJson = json.get("animations");
	if (!animationsJson.isArray()) {
		Log::error("Animations is not an array in json file: %s", filename.c_str());
		return false;
	}
	bool removeDefaultAnimation = true;
	for (const auto &animationJson : animationsJson) {
		const core::String animationName = json::toStr(animationJson, "name");
		if (animationName.empty()) {
			continue;
		}
		if (animationName == DEFAULT_ANIMATION) {
			removeDefaultAnimation = false;
		}
		sceneGraph.addAnimation(animationName);
		sceneGraph.setAnimation(animationName);
#if BLOCKBENCH_ANIMATION
		priv::Animation animation;
		animation.uuid = core::UUID(json::toStr(animationJson, "uuid"));
		animation.name = animationName;
		animation.loop = json::toStr(animationJson, "loop");
		animation.overrideVal = animationJson.boolVal("override", false);
		animation.selected = animationJson.boolVal("selected", false);
		animation.length = animationJson.floatVal("length", 0.0f);
		animation.snapping = animationJson.intVal("snapping", 0);
		const core::String animTimeUpdate = json::toStr(animationJson, "anim_time_update");
		const core::String blendWeight = json::toStr(animationJson, "blend_weight");
		const core::String startDelay = json::toStr(animationJson, "start_delay");
		const core::String loopDelay = json::toStr(animationJson, "loop_delay");
		if (!animationJson.contains("animators")) {
			Log::debug("No animators found in json file: %s", filename.c_str());
			continue;
		}
		json::Json animatorsObject = animationJson.get("animators");
		for (auto animIt = animatorsObject.begin(); animIt != animatorsObject.end(); ++animIt) {
			priv::Animator animator;
			animator.uuid = core::UUID(animIt.key());
			const json::Json animatorsJson = *animIt;
			animator.name = json::toStr(animatorsJson, "name");
			animator.type = json::toStr(animatorsJson, "type");
			if (!animatorsJson.contains("keyframes")) {
				Log::debug("No keyframes found in json file: %s", filename.c_str());
				continue;
			}

			for (const auto &keyframeJson : animatorsJson.get("keyframes")) {
				priv::KeyFrame kf;
				kf.channel = json::toStr(keyframeJson, "channel");
				kf.interpolation = priv::toInterpolationType(keyframeJson, "interpolation");
				kf.uuid = core::UUID(json::toStr(keyframeJson, "uuid"));
				kf.time = keyframeJson.floatVal("time", 0.0f);
				kf.color = keyframeJson.intVal("color", 0);
				kf.bezierLinked = keyframeJson.boolVal("bezier_linked", false);
				kf.bezierRightValue = priv::toVec3(keyframeJson, "bezier_right_value");
				kf.bezierRightTime = priv::toVec3(keyframeJson, "bezier_right_time");
				kf.bezierLeftValue = priv::toVec3(keyframeJson, "bezier_left_value");
				kf.bezierLeftTime = priv::toVec3(keyframeJson, "bezier_left_time");

				// Parse data_points array
				if (keyframeJson.contains("data_points") && keyframeJson.get("data_points").isArray()) {
					for (const auto &dataPoint : keyframeJson.get("data_points")) {
						kf.dataPoints.push_back(priv::toVec3(dataPoint));
					}
				}
				animator.keyframes.push_back(kf);
			}
			animation.animators.push_back(animator);
		}
		for (const priv::Animator &animator : animation.animators) {
			Log::debug("Animator: %s with %d keyframes", animator.name.c_str(), (int)animator.keyframes.size());
			scenegraph::SceneGraphNode *node = sceneGraph.findNodeByUUID(animator.uuid);
			if (!node) {
				const core::String &uuidStr = animator.uuid.str();
				Log::warn("Node not found for uuid: %s", uuidStr.c_str());
				continue;
			}

			const core::String &uuidStr = node->uuid().str();
			Log::debug("Found node: %s (uuid: %s)", node->name().c_str(), uuidStr.c_str());

			// Sort keyframes by time to ensure correct ordering
			core::DynamicArray<priv::KeyFrame> sortedKeyframes = animator.keyframes;
			core::sort(sortedKeyframes.begin(), sortedKeyframes.end(), [](const priv::KeyFrame &a, const priv::KeyFrame &b) {
				return a.time < b.time;
			});

			for (const priv::KeyFrame &keyframe : sortedKeyframes) {
				if (keyframe.dataPoints.empty()) {
					Log::debug("Keyframe has no data points: channel=%s, time=%f", keyframe.channel.c_str(), keyframe.time);
					continue;
				}

				Log::debug("Keyframe: channel=%s, time=%f, interpolation=%d, dataPoints=%d",
						   keyframe.channel.c_str(), keyframe.time, (int)keyframe.interpolation,
						   (int)keyframe.dataPoints.size());

				// Blockbench uses seconds, vengi uses frames at 60fps
				const scenegraph::FrameIndex frameIdx = keyframe.time * 60.0f;

				// Get or create keyframe at this frame
				scenegraph::KeyFrameIndex kfIdx;
				if (!node->hasKeyFrameForFrame(frameIdx, &kfIdx)) {
					kfIdx = node->addKeyFrame(frameIdx);
				}

				scenegraph::SceneGraphKeyFrame &kf = node->keyFrame(kfIdx);
				kf.interpolation = keyframe.interpolation;

				// Get existing transform to preserve other channel values
				scenegraph::SceneGraphTransform transform = kf.transform();
				const glm::vec3 &value = keyframe.dataPoints[0];

				if (keyframe.channel == "rotation") {
					// Blockbench uses degrees, convert to quaternion
					transform.setLocalOrientation(glm::quat(glm::radians(value)));
					Log::debug("  Rotation: %.2f, %.2f, %.2f degrees", value.x, value.y, value.z);
				} else if (keyframe.channel == "position") {
					transform.setLocalTranslation(value);
					Log::debug("  Position: %.2f, %.2f, %.2f", value.x, value.y, value.z);
				} else if (keyframe.channel == "scale") {
					transform.setLocalScale(value);
					Log::debug("  Scale: %.2f, %.2f, %.2f", value.x, value.y, value.z);
				} else {
					Log::warn("Unknown animation channel: %s", keyframe.channel.c_str());
					continue;
				}

				node->setTransform(kfIdx, transform);

				// Handle bezier curves for cubic interpolation
				if (keyframe.interpolation == scenegraph::InterpolationType::CubicBezier) {
					// TODO: VOXELFORMAT: Store bezier control points if the SceneGraphKeyFrame supports it
					// For now, the interpolation type is set but control points are not used
					Log::debug("  Bezier linked=%d, leftTime=(%.2f,%.2f,%.2f), rightTime=(%.2f,%.2f,%.2f)",
							   keyframe.bezierLinked,
							   keyframe.bezierLeftTime.x, keyframe.bezierLeftTime.y, keyframe.bezierLeftTime.z,
							   keyframe.bezierRightTime.x, keyframe.bezierRightTime.y, keyframe.bezierRightTime.z);
				}
			}
		}
#endif
	}
	if (removeDefaultAnimation && sceneGraph.animations().size() > 1) {
		sceneGraph.removeAnimation(DEFAULT_ANIMATION);
	}
	return true;
}

bool BlockbenchFormat::voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
									  scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return false;
	}

	core::String jsonString;
	stream->readString(stream->remaining(), jsonString);
	json::Json json = json::Json::parse(jsonString);

	const json::Json &metaJson = json.get("meta");
	if (!metaJson.contains("format_version")) {
		Log::error("No format_version found in json file: %s", filename.c_str());
		return false;
	}

	BBMeta bbMeta;
	bbMeta.formatVersion = json::toStr(metaJson, "format_version");
	bbMeta.version = util::parseVersion(bbMeta.formatVersion);
	bbMeta.modelFormat = json::toStr(metaJson, "model_format");
	if (!priv::isSupportModelFormat(bbMeta.modelFormat)) {
		Log::error("Unsupported model format: %s", bbMeta.modelFormat.c_str());
		return false;
	}
	bbMeta.creationTimestamp = priv::toNumber(metaJson, "creation_time", (uint64_t)0);
	bbMeta.box_uv = metaJson.boolVal("box_uv", false);
	bbMeta.name = json::toStr(json, "name", core::string::extractFilename(filename));
	bbMeta.model_identifier = json::toStr(json, "model_identifier");

	if (json.contains("resolution")) {
		const json::Json resolutionJson = json.get("resolution");
		if (resolutionJson.isObject()) {
			bbMeta.resolution.x = priv::toNumber(resolutionJson, "width", 0);
			bbMeta.resolution.y = priv::toNumber(resolutionJson, "height", 0);
		}
	}

	const json::Json &textures = json.get("textures");
	if (!textures.isArray()) {
		Log::error("Textures is not an array in json file: %s", filename.c_str());
		return false;
	}

	MeshMaterialArray meshMaterialArray;
	meshMaterialArray.reserve(textures.size());

	for (const auto &texture : textures) {
		const core::String &name = json::toStr(texture, "name");
		const core::String &source = json::toStr(texture, "source");
		const core::String &path = json::toStr(texture, "path");
		const core::String &relativePath = json::toStr(texture, "relative_path");

		image::ImagePtr image;

		// Try to load from base64 encoded source
		if (core::string::startsWith(source, "data:")) {
			const size_t mimetypeEndPos = source.find(";");
			if (mimetypeEndPos == core::String::npos) {
				Log::warn("No mimetype found in source for texture: %s", name.c_str());
			} else {
				const core::String &mimetype = source.substr(5, mimetypeEndPos - 5);
				if (mimetype != "image/png" && mimetype != "image/jpeg") {
					Log::warn("Unsupported mimetype: %s for texture: %s", mimetype.c_str(), name.c_str());
				} else {
					const size_t encodingEnd = source.find(",");
					if (encodingEnd == core::String::npos) {
						Log::warn("No encoding found in source for texture: %s", name.c_str());
					} else {
						const core::String &encoding = source.substr(mimetypeEndPos + 1, encodingEnd - mimetypeEndPos - 1);
						if (encoding != "base64") {
							Log::warn("Unsupported encoding: %s for texture: %s", encoding.c_str(), name.c_str());
						} else {
							const core::String &data = source.substr(encodingEnd + 1);
							Log::debug("Loading texture: %s with size: %d", name.c_str(), (int)data.size());
							io::MemoryReadStream dataStream(data.c_str(), data.size());
							io::Base64ReadStream base64Stream(dataStream);
							io::BufferedReadWriteStream bufferedStream(base64Stream, data.size());
							image = image::loadImage(name, bufferedStream);
							if (!image->isLoaded()) {
								Log::warn("Failed to load texture from base64: %s", name.c_str());
							}
						}
					}
				}
			}
		}
		// Try to load from external file path
		else if (!path.empty()) {
			Log::debug("Loading texture from path: %s", path.c_str());
			core::ScopedPtr<io::SeekableReadStream> pathStream(archive->readStream(path));
			if (pathStream) {
				image = image::loadImage(path, *pathStream);
				if (!image->isLoaded()) {
					Log::warn("Failed to load texture from path: %s", path.c_str());
				}
			} else {
				Log::warn("Could not open stream for texture path: %s", path.c_str());
			}
		}
		// Try to load from relative path
		else if (!relativePath.empty()) {
			Log::debug("Loading texture from relative path: %s", relativePath.c_str());
			const core::String fullPath = core::string::path(filename, relativePath);
			core::ScopedPtr<io::SeekableReadStream> relPathStream(archive->readStream(fullPath));
			if (relPathStream) {
				image = image::loadImage(fullPath, *relPathStream);
				if (!image->isLoaded()) {
					Log::warn("Failed to load texture from relative path: %s", relativePath.c_str());
				}
			} else {
				Log::warn("Could not open stream for relative texture path: %s", fullPath.c_str());
			}
		}

		// Always add material to array (even if null) to preserve indices
		if (image && image->isLoaded()) {
			meshMaterialArray.push_back(createMaterial(image));
		} else {
			// Add null material to maintain correct indexing
			meshMaterialArray.push_back(MeshMaterialPtr{});
			Log::debug("Added null material at index %d for texture: %s", (int)meshMaterialArray.size() - 1, name.c_str());
		}
	}
	const json::Json &elementsJson = json.get("elements");
	if (!elementsJson.isArray()) {
		Log::error("Elements is not an array in json file: %s", filename.c_str());
		return false;
	}

	glm::vec3 elementsMins, elementsMaxs;
	priv::computeElementsAABB(elementsJson, elementsMins, elementsMaxs);
	const glm::vec3 scale = getInputScale(elementsMins, elementsMaxs);
	BBElementMap bbElementMap;
	if (!priv::parseElements(scale, filename, bbMeta, elementsJson, meshMaterialArray, bbElementMap, sceneGraph)) {
		Log::error("Failed to parse elements");
		return false;
	}

	const json::Json &outlinerJson = json.get("outliner");
	if (!outlinerJson.isArray()) {
		Log::error("Outliner is not an array in json file: %s", filename.c_str());
		return false;
	}

	BBNode bbRoot;
	for (const auto &entry : outlinerJson) {
		if (entry.isObject()) {
			// Parse group as a child node
			BBNode bbChildNode;
			if (!priv::parseOutliner(scale, filename, bbMeta, entry, bbChildNode)) {
				Log::error("Failed to parse outliner");
				return false;
			}
			bbRoot.children.push_back(bbChildNode);
		} else if (entry.isString()) {
			// Direct element reference at root level
			core::String uuid = json::toStr(entry);
			bbRoot.referenced.push_back(core::UUID(uuid));
		}
	}

	// Apply compatibility fixes for older Blockbench versions before creating scene nodes
	processCompatibility(bbMeta, bbElementMap, bbRoot);

	if (!addNode(bbRoot, bbElementMap, sceneGraph, meshMaterialArray, 0)) {
		Log::error("Failed to add node");
		return false;
	}

	if (!parseAnimations(filename, bbMeta, json, sceneGraph)) {
		Log::error("Failed to parse animations");
		// don't abort because we can still load the model without animations
	}

	scenegraph::SceneGraphNode &rootNode = sceneGraph.node(sceneGraph.root().id());
	rootNode.setProperty(scenegraph::PropVersion, bbMeta.formatVersion);
	rootNode.setProperty(scenegraph::PropTitle, bbMeta.name);
	rootNode.setProperty("model_format", bbMeta.modelFormat);
	rootNode.setProperty("model_identifier", bbMeta.model_identifier);

	return true;
}

} // namespace voxelformat
