/**
 * @file
 */

#include "BlockbenchFormat.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "image/Image.h"
#include "io/Base64ReadStream.h"
#include "io/MemoryReadStream.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "util/Version.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxelformat/private/mesh/Polygon.h"
#include "voxelutil/ImageUtils.h"
#include "voxelutil/VoxelUtil.h"

#include <glm/trigonometric.hpp>
#include <json.hpp>

namespace voxelformat {

namespace priv {
static inline core::String toStr(const std::string &str) {
	core::String p(str.c_str());
	return p;
}

static inline core::String toStr(const nlohmann::json &json, const char *key, const core::String &defaultValue = "") {
	return toStr(json.value(key, defaultValue.c_str()));
}

template<class T>
static T toInt(const nlohmann::json &json, const char *key, T defaultValue) {
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

static glm::vec3 toVec3(const nlohmann::json &json, const char *key, const glm::vec3 &defaultValue = glm::vec3(0.0f)) {
	auto iter = json.find(key);
	if (iter == json.end() || !iter->is_array() || iter->size() != 3) {
		return defaultValue;
	}
	return glm::vec3(iter.value()[0], iter.value()[1], iter.value()[2]);
}

static BlockbenchFormat::ElementType toType(const nlohmann::json &json, const char *key) {
	const core::String &type = priv::toStr(json, key);
	if (type == "cube") {
		return BlockbenchFormat::ElementType::Cube;
	} else if (type == "mesh") {
		return BlockbenchFormat::ElementType::Mesh;
	}
	Log::debug("Unsupported element type: %s", type.c_str());
	return BlockbenchFormat::ElementType::Max;
}

static bool isSupportModelFormat(const core::String &modelFormat) {
	return modelFormat != "skin";
}

static bool parseMesh(const glm::vec3 &scale, const core::String &filename, const BlockbenchFormat::Meta &meta,
					  const nlohmann::json &elementJson, const BlockbenchFormat::Textures &textureArray,
					  BlockbenchFormat::Element &element) {
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

		const int textureIdx = priv::toInt(faceData, "texture", -1);
		const bool textureIdxValid = textureIdx >= 0 && textureIdx < (int)textureArray.size();
		Polygon polygon;
		if (textureIdxValid) {
			polygon.setTexture(textureArray[textureIdx]);
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
			const glm::vec2 uvCoords =
				textureIdxValid ? textureArray[textureIdx]->uv(uvIter.value()[0], uvIter.value()[1]) : glm::vec2(0.0f);

			polygon.addVertex(pos, uvCoords);
		}
		polygon.toTris(element.mesh.tris);
	}

	return true;
}

static bool parseCube(const glm::vec3 &scale, const core::String &filename, const BlockbenchFormat::Meta &meta,
					  const nlohmann::json &elementJson, const BlockbenchFormat::Textures &textureArray,
					  BlockbenchFormat::Element &element) {
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

	element.cube.from = scale * priv::toVec3(elementJson, "from");
	element.cube.to = scale * priv::toVec3(elementJson, "to");

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

		int textureIdx = -1;
		if (!textureArray.empty()) {
			textureIdx = priv::toInt(faceData, "texture", -1);
			if (textureIdx >= (int)textureArray.size()) {
				Log::error("Invalid texture index: %d", textureIdx);
				return false;
			}
		}

		Log::debug("faceName: %s, textureIdx: %d", faceName.c_str(), textureIdx);
		int uvs[4]{uv[0], uv[1], uv[2], uv[3]};
		if (textureIdx >= 0) {
			const glm::vec2 uv0 = textureArray[textureIdx]->uv(uvs[0], uvs[1]);
			const glm::vec2 uv1 = textureArray[textureIdx]->uv(uvs[2] - 1, uvs[3] - 1);
			element.cube.faces[(int)faceType].uvs[0] = uv0;
			element.cube.faces[(int)faceType].uvs[1] = uv1;
		}
		element.cube.faces[(int)faceType].textureIndex = textureIdx;
	}
	return true;
}

static bool parseElements(const glm::vec3 &scale, const core::String &filename, const BlockbenchFormat::Meta &meta,
						  const nlohmann::json &elementsJson, const BlockbenchFormat::Textures &textureArray,
						  BlockbenchFormat::ElementMap &elementMap, scenegraph::SceneGraph &sceneGraph) {
	for (const auto &elementJson : elementsJson) {
		BlockbenchFormat::Element element;
		element.uuid = priv::toStr(elementJson, "uuid");
		element.name = priv::toStr(elementJson, "name");
		element.origin = scale * priv::toVec3(elementJson, "origin");
		element.rotation = priv::toVec3(elementJson, "rotation");
		element.rescale = elementJson.value("rescale", false);
		element.locked = elementJson.value("locked", false);
		element.box_uv = elementJson.value("box_uv", false);
		element.color = priv::toInt(elementJson, "color", 0);
		element.type = priv::toType(elementJson, "type");
		if (element.type == BlockbenchFormat::ElementType::Max) {
			element.type = BlockbenchFormat::ElementType::Cube;
		}

		if (element.type == BlockbenchFormat::ElementType::Cube) {
			if (!parseCube(scale, filename, meta, elementJson, textureArray, element)) {
				return false;
			}
		} else if (element.type == BlockbenchFormat::ElementType::Mesh) {
			if (!parseMesh(scale, filename, meta, elementJson, textureArray, element)) {
				return false;
			}
		}

		elementMap.emplace(element.uuid, core::move(element));
	}
	return true;
}

static bool parseOutliner(const glm::vec3 &scale, const core::String &filename, const BlockbenchFormat::Meta &meta,
						  const nlohmann::json &entryJson, BlockbenchFormat::Node &node) {
	node.name = priv::toStr(entryJson, "name");
	node.uuid = priv::toStr(entryJson, "uuid");
	node.locked = entryJson.value("locked", false);
	node.visible = entryJson.value("visibility", true);
	node.mirror_uv = entryJson.value("mirror_uv", false);
	node.origin = scale * priv::toVec3(entryJson, "origin");
	node.rotation = priv::toVec3(entryJson, "rotation");
	node.color = priv::toInt(entryJson, "color", 0);

	Log::debug("Node name: %s (%i references)", node.name.c_str(), (int)node.referenced.size());

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
			core::String uuid = priv::toStr(*iter);
			node.referenced.emplace_back(uuid);
			continue;
		}
		if (!iter->is_object()) {
			Log::error("Child entry is not an object in json file: %s", filename.c_str());
			return false;
		}
		BlockbenchFormat::Node childNode;
		if (!parseOutliner(scale, filename, meta, *iter, childNode)) {
			return false;
		}
		node.children.emplace_back(core::move(childNode));
	}
	return true;
}

} // namespace priv

bool BlockbenchFormat::generateMesh(const Node &node, const Element &element, const Textures &textureArray,
									scenegraph::SceneGraph &sceneGraph, int parent) const {
	const Mesh &mesh = element.mesh;
	const int nodeIdx = voxelizeNode(element.uuid, element.name, sceneGraph, mesh.tris, parent);
	if (nodeIdx == InvalidNodeId) {
		return false;
	}
	scenegraph::SceneGraphNode &model = sceneGraph.node(nodeIdx);
	model.setLocked(node.locked);
	model.setVisible(node.visible);
	sceneGraph.updateTransforms();
	model.setRotation(glm::quat(glm::radians(element.rotation)), true);
	model.setTranslation(element.origin, true);
	return true;
}

bool BlockbenchFormat::generateCube(const Node &node, const Element &element, const Textures &textureArray,
									scenegraph::SceneGraph &sceneGraph, int parent) const {
	const Cube &cube = element.cube;

	// TODO: VOXELFORMAT: is this (from > to) used to flip the uv coordinates or for culling?
	glm::vec3 mins = glm::min(element.cube.from, element.cube.to);
	glm::vec3 maxs = glm::max(element.cube.from, element.cube.to);

	glm::vec3 size = maxs - mins;
	// even a plane is one voxel for us
	size.x = glm::clamp(size.x, 1.0f, 1.0f + size.x);
	size.y = glm::clamp(size.y, 1.0f, 1.0f + size.y);
	size.z = glm::clamp(size.z, 1.0f, 1.0f + size.z);

	mins = glm::round(mins);
	maxs = mins + size - 1.0f;
	voxel::Region region(mins, maxs);
	if (!region.isValid()) {
		Log::error("Invalid region for element: %s (node: %s): %f:%f:%f/%f:%f:%f", element.name.c_str(), node.name.c_str(), mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
		return false;
	}
	scenegraph::SceneGraphNode model(scenegraph::SceneGraphNodeType::Model, element.uuid);
	region.shift(-region.getLowerCorner());
	model.setVolume(new voxel::RawVolume(region), true);
	model.setName(element.name);
	model.setLocked(node.locked);
	model.setVisible(node.visible);
	model.setRotation(glm::quat(glm::radians(element.rotation)), true);
	// TODO: VOXELFORMAT: pivot or translation is still wrong and group rotations are not correctly applied yet
	const glm::vec3 pivot = (element.origin - element.cube.from) / size;
	model.setPivot(pivot);
	const glm::vec3 regionsize = region.getDimensionsInVoxels();
	model.setTranslation(element.cube.from + pivot * regionsize, true);
	const voxel::FaceNames order[] = {voxel::FaceNames::NegativeX, voxel::FaceNames::PositiveX,
									  voxel::FaceNames::NegativeY, voxel::FaceNames::PositiveY,
									  voxel::FaceNames::NegativeZ, voxel::FaceNames::PositiveZ};
	for (int i = 0; i < lengthof(order); ++i) {
		const CubeFace &face = cube.faces[(int)order[i]];
		const voxel::FaceNames faceName = order[i];
		image::ImagePtr image;
		if (face.textureIndex >= 0) {
			image = textureArray[face.textureIndex];
		}
		voxel::RawVolumeWrapper wrapper(model.volume());
		voxelutil::importFace(wrapper, wrapper.region(), model.palette(), faceName, image, face.uvs[0], face.uvs[1],
							  element.color);
	}
	model.volume()->region().shift(-region.getLowerCorner());
	return sceneGraph.emplace(core::move(model), parent) != InvalidNodeId;
}

bool BlockbenchFormat::addNode(const Node &node, const ElementMap &elementMap, scenegraph::SceneGraph &sceneGraph,
							   const Textures &textureArray, int parent) const {
	Log::debug("node: %s with %i children", node.name.c_str(), (int)node.children.size());
	for (const core::String &uuid : node.referenced) {
		auto elementIter = elementMap.find(uuid);
		if (elementIter == elementMap.end()) {
			Log::warn("Could not find node with uuid: %s", uuid.c_str());
			continue;
		}
		const Element &element = elementIter->value;
		if (element.type == ElementType::Cube) {
			if (!generateCube(node, element, textureArray, sceneGraph, parent)) {
				return false;
			}
		} else if (element.type == ElementType::Mesh) {
			if (!generateMesh(node, element, textureArray, sceneGraph, parent)) {
				return false;
			}
		} else {
			Log::warn("Unsupported element type: %i", (int)element.type);
		}
	}
	for (const Node &child : node.children) {
		scenegraph::SceneGraphNode group(scenegraph::SceneGraphNodeType::Group, child.uuid);
		group.setName(child.name);
		group.setVisible(child.visible);
		group.setLocked(child.locked);
		group.setRotation(glm::quat(glm::radians(child.rotation)), true);
		group.setTranslation(child.origin, true);
		int groupParent = sceneGraph.emplace(core::move(group), parent);
		if (groupParent == InvalidNodeId) {
			Log::error("Failed to add node: %s", child.name.c_str());
			return false;
		}
		if (!addNode(child, elementMap, sceneGraph, textureArray, groupParent)) {
			return false;
		}
	}
	return true;
}

static bool parseAnimations(const core::String &filename, const BlockbenchFormat::Meta &meta, nlohmann::json &json,
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
		const core::String animationName = priv::toStr(animationJson, "name");
		if (animationName.empty()) {
			continue;
		}
		sceneGraph.addAnimation(animationName);
#if 0
		// const core::String uuid = priv::toStr(animationJson, "uuid");
		const core::String loop = priv::toStr(animationJson, "loop"); // "once"
		const bool overrideVal = animationJson["override"];
		const bool selected = animationJson["selected"];
		const int length = animationJson["length"];
		const int snapping = animationJson["snapping"];
		// TODO: VOXELFORMAT: load animations
		// "anim_time_update": "",
		// "blend_weight": "",
		// "start_delay": "",
		// "loop_delay": "",
		for (auto animatorsIter = animationJson.find("animators"); animatorsIter != animationJson.end();
			 ++animatorsIter) {
			const core::String uuid = priv::toStr(animatorsIter.key());
			const auto &animatorsJson = animatorsIter.value();
			const core::String animatorName = priv::toStr(animatorsJson, "name");
			const core::String type = priv::toStr(animatorsJson, "type"); // "bone"
			for (auto keyframesIter = animatorsJson.find("keyframes"); keyframesIter != animatorsJson.end();
				 ++keyframesIter) {
				const auto &keyframeJson = keyframesIter.value();
				const core::String keyframeChannel = priv::toStr(keyframeJson, "channel"); // "rotation", "position"
				const core::String keyframeInterpolation = priv::toStr(keyframeJson, "interpolation"); // "linear", "catmullrom"
				const core::String keyframeUuid = priv::toStr(keyframeJson, "uuid");
				const float keyframeTime = keyframeJson.value("time", 0.0f);
				const int keyframeColor = keyframeJson.value("color", 0);
				const bool keyframeBezierLinked = keyframeJson.value("bezier_linked", false);
				const glm::vec3 keyframeBezierRightValue = priv::toVec3(keyframeJson, "bezier_right_value");
				const glm::vec3 keyframeBezierRightTime = priv::toVec3(keyframeJson, "bezier_right_time");
				const glm::vec3 keyframeBezierLeftValue = priv::toVec3(keyframeJson, "bezier_left_value");
				const glm::vec3 keyframeBezierLeftTime = priv::toVec3(keyframeJson, "bezier_left_time");
				// TODO: VOXELFORMAT: parse data_points - x, y and z - there can be strings inside for some - like "z": "0\n" and "z": 0
				// "data_points": [{"x": "0","y": "0","z": 90}],
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

	Meta meta;
	meta.formatVersion = priv::toStr(metaJson, "format_version");
	meta.version = util::parseVersion(meta.formatVersion);
	meta.modelFormat = priv::toStr(metaJson, "model_format");
	if (!priv::isSupportModelFormat(meta.modelFormat)) {
		Log::error("Unsupported model format: %s", meta.modelFormat.c_str());
		return false;
	}
	meta.creationTimestamp = priv::toInt(metaJson, "creation_time", (uint64_t)0);
	meta.box_uv = metaJson.value("box_uv", false);
	meta.name = priv::toStr(json, "name", core::string::extractFilename(filename));
	meta.model_identifier = priv::toStr(json, "model_identifier");

	auto resolutionJsonIter = json.find("resolution");
	if (resolutionJsonIter != json.end()) {
		const nlohmann::json &resolutionJson = *resolutionJsonIter;
		if (resolutionJson.is_object()) {
			meta.resolution.x = priv::toInt(resolutionJson, "width", 0);
			meta.resolution.y = priv::toInt(resolutionJson, "height", 0);
		}
	}

	const nlohmann::json &textures = json["textures"];
	if (!textures.is_array()) {
		Log::error("Textures is not an array in json file: %s", filename.c_str());
		return false;
	}

	Textures textureArray;
	textureArray.reserve(textures.size());

	for (const auto &texture : textures) {
		const core::String &name = priv::toStr(texture, "name");
		// TODO: VOXELFORMAT: allow to load from "path" instead of "source"
		// relative_path
		const core::String &source = priv::toStr(texture, "source");
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
			const image::ImagePtr &image = image::loadImage(name, base64Stream, data.size());
			if (image->isLoaded()) {
				textureArray.emplace_back(image);
			}
		}
	}

	const nlohmann::json &elementsJson = json["elements"];
	if (!elementsJson.is_array()) {
		Log::error("Elements is not an array in json file: %s", filename.c_str());
		return false;
	}

	const glm::vec3 scale = getInputScale();
	ElementMap elementMap;
	if (!priv::parseElements(scale, filename, meta, elementsJson, textureArray, elementMap, sceneGraph)) {
		Log::error("Failed to parse elements");
		return false;
	}

	const nlohmann::json &outlinerJson = json["outliner"];
	if (!outlinerJson.is_array()) {
		Log::error("Outliner is not an array in json file: %s", filename.c_str());
		return false;
	}

	Node root;
	for (const auto &entry : outlinerJson) {
		if (entry.is_object()) {
			if (!priv::parseOutliner(scale, filename, meta, entry, root)) {
				Log::error("Failed to parse outliner");
				return false;
			}
		} else if (entry.is_string()) {
			core::String uuid = priv::toStr(entry);
			root.referenced.emplace_back(uuid);
		}
	}

	if (!parseAnimations(filename, meta, json, sceneGraph)) {
		Log::error("Failed to parse animations");
		// don't abort because we can still load the model without animations
	}

	if (!addNode(root, elementMap, sceneGraph, textureArray, 0)) {
		Log::error("Failed to add node");
		return false;
	}

	scenegraph::SceneGraphNode &rootNode = sceneGraph.node(sceneGraph.root().id());
	rootNode.setProperty("version", meta.formatVersion);
	rootNode.setProperty("model_format", meta.modelFormat);

	return true;
}

} // namespace voxelformat
