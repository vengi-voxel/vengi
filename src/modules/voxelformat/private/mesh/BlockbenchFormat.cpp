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
static inline scenegraph::InterpolationType toInterpolationType(const nlohmann::json &json, const char *key, const scenegraph::InterpolationType defaultValue = scenegraph::InterpolationType::Linear) {
	const std::string val = json.value(key, "");
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
static T toNumber(const nlohmann::json &json, const char *key, T defaultValue) {
	auto iter = json.find(key);
	if (iter == json.end() || iter->is_null()) {
		return defaultValue;
	}
	if (!iter->is_number()) {
		Log::warn("Value is not a number: %s", key);
		return defaultValue;
	}
	return json[key];
}

static const glm::vec3 toVec3(const nlohmann::json &json, const glm::vec3 &defaultValue = glm::vec3(0.0f)) {
	if (json.is_array() && json.size() == 3) {
		return glm::vec3(json[0].get<float>(), json[1].get<float>(), json[2].get<float>());
	}
	auto iterX = json.find("x");
	auto iterY = json.find("y");
	auto iterZ = json.find("z");
	if (iterX == json.end() || iterY == json.end() || iterZ == json.end()) {
		return defaultValue;
	}
	if (!iterX->is_number() || !iterY->is_number() || !iterZ->is_number()) {
		return defaultValue;
	}
	// TODO: VOXELFORMAT: parse data_points - x, y and z - there can be strings inside for some - like "z": "0\n" and "z": 0
	// "data_points": [{"x": "0","y": "0","z": 90}],
	const float x = iterX.value().get<float>();
	const float y = iterY.value().get<float>();
	const float z = iterZ.value().get<float>();
	return glm::vec3(x, y, z);
}

static glm::vec3 toVec3(const nlohmann::json &json, const char *key, const glm::vec3 &defaultValue = glm::vec3(0.0f)) {
	auto iter = json.find(key);
	if (iter == json.end()) {
		return defaultValue;
	}
	return toVec3(*iter, defaultValue);
}

static BlockbenchFormat::BBElementType toType(const nlohmann::json &json, const char *key) {
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
					  const nlohmann::json &elementJson, const MeshMaterialArray &meshMaterialArray,
					  BlockbenchFormat::BBElement &bbElement) {
	if (elementJson.find("vertices") == elementJson.end()) {
		Log::error("Element is missing vertices in json file: %s", filename.c_str());
		return false;
	}

	const nlohmann::json &vertices = elementJson["vertices"];
	if (!vertices.is_object()) {
		Log::error("Vertices is not an object in json file: %s", filename.c_str());
		return false;
	}

	if (elementJson.find("faces") == elementJson.end()) {
		Log::error("Element is missing faces in json file: %s", filename.c_str());
		return false;
	}

	const nlohmann::json &faces = elementJson["faces"];
	if (!faces.is_object()) {
		Log::error("Faces is not an object in json file: %s", filename.c_str());
		return false;
	}

	for (const auto &face : faces.items()) {
		const nlohmann::json &faceData = face.value();
		if (faceData.find("uv") == faceData.end()) {
			Log::error("Face is missing uv in json file: %s", filename.c_str());
			return false;
		}

		const nlohmann::json &uv = faceData["uv"];
		if (!uv.is_object()) {
			Log::error("UV is not an object in json file: %s", filename.c_str());
			return false;
		}

		if (faceData.find("vertices") == faceData.end()) {
			Log::error("Face is missing vertices in json file: %s", filename.c_str());
			return false;
		}

		const nlohmann::json &faceVertices = faceData["vertices"];
		if (!faceVertices.is_array()) {
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
			const std::string &vertexName = vertex;
			auto vertexIter = vertices.find(vertexName);
			if (vertexIter == vertices.end() || !vertexIter->is_array() || vertexIter->size() != 3) {
				Log::error("Vertex is not an array of size 3 in json file: %s", filename.c_str());
				return false;
			}
			auto uvIter = uv.find(vertexName);
			if (uvIter == uv.end() || !uvIter->is_array() || uvIter->size() != 2) {
				Log::error("UV is not an array of size 2 in json file: %s", filename.c_str());
				return false;
			}
			const glm::vec3 pos(vertexIter.value()[0], vertexIter.value()[1], vertexIter.value()[2]);
			const int x = uvIter.value()[0];
			const int y = uvIter.value()[1];
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
					  const nlohmann::json &elementJson, const MeshMaterialArray &meshMaterialArray,
					  BlockbenchFormat::BBElement &bbElement) {
	if (elementJson.find("from") == elementJson.end() || elementJson.find("to") == elementJson.end()) {
		Log::error("Element is missing from or to in json file: %s", filename.c_str());
		return false;
	}

	const nlohmann::json &from = elementJson["from"];
	const nlohmann::json &to = elementJson["to"];
	if (!from.is_array() || from.size() != 3 || !to.is_array() || to.size() != 3) {
		Log::error("From or to is not an array of size 3 in json file: %s", filename.c_str());
		return false;
	}

	bbElement.cube.from = scale * priv::toVec3(elementJson, "from");
	bbElement.cube.to = scale * priv::toVec3(elementJson, "to");

	if (elementJson.find("faces") == elementJson.end()) {
		Log::error("Element is missing faces in json file: %s", filename.c_str());
		return false;
	}

	const nlohmann::json &faces = elementJson["faces"];
	if (!faces.is_object()) {
		Log::error("Faces is not an object in json file: %s", filename.c_str());
		return false;
	}

	for (const auto &face : faces.items()) {
		const core::String &faceName = face.key().c_str();
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

		const nlohmann::json &faceData = face.value();
		if (faceData.find("uv") == faceData.end()) {
			Log::error("Face is missing uv in json file: %s", filename.c_str());
			return false;
		}

		const nlohmann::json &uv = faceData["uv"];
		if (!uv.is_array() || uv.size() != 4) {
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
		int uvs[4]{uv[0], uv[1], uv[2], uv[3]};
		if (materialIdx >= 0 && meshMaterialArray[materialIdx]->texture) {
			const glm::vec2 uv0 = meshMaterialArray[materialIdx]->texture->uv(uvs[0], uvs[1]);
			const glm::vec2 uv1 = meshMaterialArray[materialIdx]->texture->uv(uvs[2] - 1, uvs[3] - 1);
			bbElement.cube.faces[(int)faceType].uvs[0] = uv0;
			bbElement.cube.faces[(int)faceType].uvs[1] = uv1;
		}
		bbElement.cube.faces[(int)faceType].textureIndex = materialIdx;
	}
	return true;
}

static bool parseElements(const glm::vec3 &scale, const core::String &filename, const BlockbenchFormat::BBMeta &bbMeta,
						  const nlohmann::json &elementsJson, const MeshMaterialArray &meshMaterialArray,
						  BlockbenchFormat::BBElementMap &bbElementMap, scenegraph::SceneGraph &sceneGraph) {
	for (const auto &elementJson : elementsJson) {
		BlockbenchFormat::BBElement bbElement;
		bbElement.uuid = core::UUID(json::toStr(elementJson, "uuid"));
		bbElement.name = json::toStr(elementJson, "name");
		bbElement.origin = scale * priv::toVec3(elementJson, "origin");
		bbElement.rotation = priv::toVec3(elementJson, "rotation");
		bbElement.rescale = elementJson.value("rescale", false);
		bbElement.locked = elementJson.value("locked", false);
		bbElement.box_uv = elementJson.value("box_uv", false);
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
						  const nlohmann::json &entryJson, BlockbenchFormat::BBNode &bbNode) {
	bbNode.name = json::toStr(entryJson, "name");
	bbNode.uuid = core::UUID(json::toStr(entryJson, "uuid"));
	bbNode.locked = entryJson.value("locked", false);
	bbNode.visible = entryJson.value("visibility", true);
	bbNode.mirror_uv = entryJson.value("mirror_uv", false);
	bbNode.origin = scale * priv::toVec3(entryJson, "origin");
	bbNode.rotation = priv::toVec3(entryJson, "rotation");
	bbNode.color = priv::toNumber(entryJson, "color", 0);

	Log::debug("Node name: %s (%i references)", bbNode.name.c_str(), (int)bbNode.referenced.size());

	auto childrenIter = entryJson.find("children");
	if (childrenIter == entryJson.end()) {
		return true;
	}
	const nlohmann::json &childrenJson = entryJson["children"];
	if (childrenJson.empty()) {
		return true;
	}
	if (!childrenJson.is_array()) {
		Log::error("Children is not an array in json file: %s", filename.c_str());
		return false;
	}

	for (auto iter = childrenJson.begin(); iter != childrenJson.end(); ++iter) {
		if (iter->is_string()) {
			core::UUID uuid = core::UUID(json::toStr(*iter));
			bbNode.referenced.emplace_back(uuid);
			continue;
		}
		if (!iter->is_object()) {
			Log::error("Child entry is not an object in json file: %s", filename.c_str());
			return false;
		}
		BlockbenchFormat::BBNode bbChildNode;
		if (!parseOutliner(scale, filename, bbMeta, *iter, bbChildNode)) {
			return false;
		}
		bbNode.children.emplace_back(core::move(bbChildNode));
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

	// TODO: VOXELFORMAT: is this (from > to) used to flip the uv coordinates or for culling?
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
	model.setVolume(new voxel::RawVolume(region), true);
	model.setName(bbElement.name);
	model.setLocked(bbNode.locked);
	model.setVisible(bbNode.visible);
	model.setRotation(glm::quat(glm::radians(bbElement.rotation)), true);
	// TODO: VOXELFORMAT: pivot or translation is still wrong and group rotations are not correctly applied yet
	const glm::vec3 pivot = (bbElement.origin - bbElement.cube.from) / size;
	model.setPivot(pivot);
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
		voxelutil::importFace(*model.volume(), model.region(), model.palette(), faceName, image, face.uvs[0], face.uvs[1],
							  bbElement.color);
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

static bool parseAnimations(const core::String &filename, const BlockbenchFormat::BBMeta &bbMeta, nlohmann::json &json,
							scenegraph::SceneGraph &sceneGraph) {
	// no animations found
	if (json.find("animations") == json.end()) {
		return true;
	}

	const nlohmann::json &animationsJson = json["animations"];
	if (!animationsJson.is_array()) {
		Log::error("Animations is not an array in json file: %s", filename.c_str());
		return false;
	}
	for (const auto &animationJson : animationsJson) {
		const core::String animationName = json::toStr(animationJson, "name");
		if (animationName.empty()) {
			continue;
		}
		sceneGraph.addAnimation(animationName);
#if BLOCKBENCH_ANIMATION
		priv::Animation animation;
		animation.uuid = core::UUID(json::toStr(animationJson, "uuid"));
		animation.name = animationName;
		animation.loop = json::toStr(animationJson, "loop");
		animation.overrideVal = animationJson["override"];
		animation.selected = animationJson["selected"];
		animation.length = animationJson["length"];
		animation.snapping = animationJson["snapping"];
		const core::String animTimeUpdate = json::toStr(animationJson, "anim_time_update");
		const core::String blendWeight = json::toStr(animationJson, "blend_weight");
		const core::String startDelay = json::toStr(animationJson, "start_delay");
		const core::String loopDelay = json::toStr(animationJson, "loop_delay");
		auto animatorsIter = animationJson.find("animators");
		if (animatorsIter == animationJson.end()) {
			Log::debug("No animators found in json file: %s", filename.c_str());
			continue;
		}
		const auto &object = animatorsIter->get<std::map<std::string, nlohmann::json>>();
		for (const auto &entry : object) {
			priv::Animator animator;
			animator.uuid = core::UUID(json::toStr(entry.first));
			const auto &animatorsJson = entry.second;
			animator.name = json::toStr(animatorsJson, "name");
			animator.type = json::toStr(animatorsJson, "type");
			auto keyFramesIter = animatorsJson.find("keyframes");
			if (keyFramesIter == animatorsJson.end()) {
				Log::debug("No keyframes found in json file: %s", filename.c_str());
				continue;
			}

			for (const auto &keyframeJson : animatorsJson["keyframes"]) {
				priv::KeyFrame kf;
				kf.channel = json::toStr(keyframeJson, "channel");
				kf.interpolation = priv::toInterpolationType(keyframeJson, "interpolation");
				kf.uuid = core::UUID(json::toStr(keyframeJson, "uuid"));
				kf.time = keyframeJson.value("time", 0.0f);
				kf.color = keyframeJson.value("color", 0);
				kf.bezierLinked = keyframeJson.value("bezier_linked", false);
				kf.bezierRightValue = priv::toVec3(keyframeJson, "bezier_right_value");
				kf.bezierRightTime = priv::toVec3(keyframeJson, "bezier_right_time");
				kf.bezierLeftValue = priv::toVec3(keyframeJson, "bezier_left_value");
				kf.bezierLeftTime = priv::toVec3(keyframeJson, "bezier_left_time");
				for (auto dataPointsIter = keyframeJson.find("data_points"); dataPointsIter != keyframeJson.end(); ++dataPointsIter) {
					kf.dataPoints.push_back(priv::toVec3(*dataPointsIter));
				}
				animator.keyframes.push_back(kf);
			}
			animation.animators.push_back(animator);
		}
		for (const priv::Animator &animator : animation.animators) {
			Log::debug("Animator: %s", animator.name.c_str());
			if (scenegraph::SceneGraphNode *node = sceneGraph.findNodeByUUID(animator.uuid)) {
				const core::String &uuidStr = node->uuid().str();
				Log::debug("Found node: %s (uuid: %s)", node->name().c_str(), uuidStr.c_str());
				const auto &keyframes = animator.keyframes;
				scenegraph::KeyFrameIndex keyFrameIdx = 0;
				node->keyFrames()->reserve(keyframes.size());
				// TODO: VOXELFORMAT: finish this
				for (const priv::KeyFrame &keyframe : keyframes) {
					Log::debug("Keyframe: %s", keyframe.channel.c_str());
					scenegraph::SceneGraphKeyFrame kf = node->keyFrame(keyFrameIdx);
					kf.interpolation = keyframe.interpolation;
					// TODO: VOXELFORMAT: the assumption here is that the keyframes are sorted by time
					kf.frameIdx = keyframe.time * 60.0f;
					scenegraph::SceneGraphTransform transform;
					if (keyframe.channel == "rotation") {
						transform.setLocalOrientation(glm::quat(glm::radians(keyframe.dataPoints[0])));
					} else if (keyframe.channel == "position") {
						transform.setLocalTranslation(keyframe.dataPoints[0]);
					} else if (keyframe.channel == "scale") {
						transform.setLocalScale(keyframe.dataPoints[0]);
					}
					kf.setTransform(transform);
				}
			} else {
				const core::String &uuidStr = animator.uuid.str();
				Log::warn("Node not found for uuid: %s", uuidStr.c_str());
			}
		}
#endif
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
	nlohmann::json json = nlohmann::json::parse(jsonString, nullptr, false, true);

	const nlohmann::json &metaJson = json["meta"];
	if (metaJson.find("format_version") == metaJson.end()) {
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
	bbMeta.box_uv = metaJson.value("box_uv", false);
	bbMeta.name = json::toStr(json, "name", core::string::extractFilename(filename));
	bbMeta.model_identifier = json::toStr(json, "model_identifier");

	auto resolutionJsonIter = json.find("resolution");
	if (resolutionJsonIter != json.end()) {
		const nlohmann::json &resolutionJson = *resolutionJsonIter;
		if (resolutionJson.is_object()) {
			bbMeta.resolution.x = priv::toNumber(resolutionJson, "width", 0);
			bbMeta.resolution.y = priv::toNumber(resolutionJson, "height", 0);
		}
	}

	const nlohmann::json &textures = json["textures"];
	if (!textures.is_array()) {
		Log::error("Textures is not an array in json file: %s", filename.c_str());
		return false;
	}

	MeshMaterialArray meshMaterialArray;
	meshMaterialArray.reserve(textures.size());

	for (const auto &texture : textures) {
		const core::String &name = json::toStr(texture, "name");
		// TODO: VOXELFORMAT: allow to load from "path" instead of "source"
		// relative_path
		const core::String &source = json::toStr(texture, "source");
		if (core::string::startsWith(source, "data:")) {
			const size_t mimetypeEndPos = source.find(";");
			if (mimetypeEndPos == core::String::npos) {
				Log::error("No mimetype found in source: %s", source.c_str());
				return false;
			}
			const core::String &mimetype = source.substr(5, mimetypeEndPos - 5);
			if (mimetype != "image/png") {
				Log::error("Unsupported mimetype: %s", mimetype.c_str());
				return false;
			}
			const size_t encodingEnd = source.find(",");
			if (encodingEnd == core::String::npos) {
				Log::error("No encoding found in source: %s", source.c_str());
				return false;
			}
			const core::String &encoding = source.substr(mimetypeEndPos + 1, encodingEnd - mimetypeEndPos - 1);
			if (encoding != "base64") {
				Log::error("Unsupported encoding: %s", encoding.c_str());
				return false;
			}
			const core::String &data = source.substr(encodingEnd + 1);
			Log::debug("Loading texture: %s with size: %d", name.c_str(), (int)data.size());
			io::MemoryReadStream dataStream(data.c_str(), data.size());
			io::Base64ReadStream base64Stream(dataStream);
			io::BufferedReadWriteStream bufferedStream(base64Stream, data.size());
			const image::ImagePtr &image = image::loadImage(name, bufferedStream);
			if (image->isLoaded()) {
				meshMaterialArray.emplace_back(createMaterial(image));
			}
			// TODO: VOXELFORMAT: this will destroy the material indices...
		}
	}

	const nlohmann::json &elementsJson = json["elements"];
	if (!elementsJson.is_array()) {
		Log::error("Elements is not an array in json file: %s", filename.c_str());
		return false;
	}

	const glm::vec3 scale = getInputScale();
	BBElementMap bbElementMap;
	if (!priv::parseElements(scale, filename, bbMeta, elementsJson, meshMaterialArray, bbElementMap, sceneGraph)) {
		Log::error("Failed to parse elements");
		return false;
	}

	const nlohmann::json &outlinerJson = json["outliner"];
	if (!outlinerJson.is_array()) {
		Log::error("Outliner is not an array in json file: %s", filename.c_str());
		return false;
	}

	BBNode bbRoot;
	for (const auto &entry : outlinerJson) {
		if (entry.is_object()) {
			if (!priv::parseOutliner(scale, filename, bbMeta, entry, bbRoot)) {
				Log::error("Failed to parse outliner");
				return false;
			}
		} else if (entry.is_string()) {
			core::String uuid = json::toStr(entry);
			bbRoot.referenced.emplace_back(core::UUID(uuid));
		}
	}

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
	rootNode.setProperty("model_format", bbMeta.modelFormat);

	return true;
}

} // namespace voxelformat
