/**
 * @file
 */

#include "BlockbenchFormat.h"
#include "core/Color.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "glm/geometric.hpp"
#include "glm/trigonometric.hpp"
#include "image/Image.h"
#include "io/Base64ReadStream.h"
#include "io/MemoryReadStream.h"
#include "math/Axis.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Voxel.h"
#include "voxelformat/private/mesh/MeshFormat.h"
#include "voxelformat/private/mesh/TexturedTri.h"
#include "voxelutil/VoxelUtil.h"

#include <json.hpp>

namespace voxelformat {

namespace priv {
static core::String toStr(const std::string &in) {
	core::String p(in.c_str());
	return p;
}

static glm::vec3 toVec3(const nlohmann::json &json, const char *key, const glm::vec3 &defaultValue = glm::vec3(0.0f)) {
	auto iter = json.find(key);
	if (iter == json.end() || !iter->is_array() || iter->size() != 3) {
		return defaultValue;
	}
	return glm::vec3(iter.value()[0], iter.value()[1], iter.value()[2]);
}

} // namespace priv

static bool parseElements(const core::String &filename, const nlohmann::json &elementsJson,
						  const BlockbenchFormat::Textures &textureArray, BlockbenchFormat::ElementMap &elementMap,
						  scenegraph::SceneGraph &sceneGraph) {
	for (const auto &elementJson : elementsJson) {
		const core::String &type = priv::toStr(elementJson.value("type", ""));
		if (type != "cube") {
			Log::warn("Unsupported element type: %s", type.c_str());
			continue;
		}

		BlockbenchFormat::Element element;
		element.uuid = priv::toStr(elementJson.value("uuid", ""));

		if (elementJson.find("from") == elementJson.end() || elementJson.find("to") == elementJson.end()) {
			Log::error("Element is missing from or to in json file: %s", filename.c_str());
			return false;
		}

		element.name = priv::toStr(elementJson.value("name", ""));

		const nlohmann::json &from = elementJson["from"];
		const nlohmann::json &to = elementJson["to"];
		if (!from.is_array() || from.size() != 3 || !to.is_array() || to.size() != 3) {
			Log::error("From or to is not an array of size 3 in json file: %s", filename.c_str());
			return false;
		}

		element.from = priv::toVec3(elementJson, "from");
		element.to = priv::toVec3(elementJson, "to");
		element.origin = priv::toVec3(elementJson, "origin");
		element.rotation = priv::toVec3(elementJson, "rotation");

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

			const int textureIdx = faceData.value("texture", -1);
			if (textureIdx < 0 || textureIdx >= (int)textureArray.size()) {
				Log::error("Invalid texture index: %d", textureIdx);
				return false;
			}

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
			Log::debug("faceName: %s, textureIdx: %d", faceName.c_str(), textureIdx);
			const float w = textureArray[textureIdx]->width();
			const float h = textureArray[textureIdx]->height();
			const glm::vec2 uv0 = glm::vec2(uv[0], uv[1]);
			const glm::vec2 uv1 = glm::vec2(uv[2], uv[3]);
			const glm::vec2 size(w, h);
			const glm::vec2 fuv0 = uv0 / size;
			const glm::vec2 fuv1 = uv1 / size;
			Log::debug("final uv0: %f:%f, uv1: %f:%f", fuv0.x, fuv0.y, fuv1.x, fuv1.y);
			element.cube.faces[(int)faceType].uvs[0] = fuv0;
			element.cube.faces[(int)faceType].uvs[1] = fuv1;
			element.cube.faces[(int)faceType].textureIndex = textureIdx;
		}

		elementMap.emplace(element.uuid, core::move(element));
	}
	return true;
}

static bool parseOutliner(const core::String &filename, const nlohmann::json &entry, BlockbenchFormat::Node &node) {
	node.name = priv::toStr(entry.value("name", ""));
	node.uuid = priv::toStr(entry.value("uuid", ""));
	node.locked = entry.value("locked", false);
	node.visible = entry.value("visible", true);
	node.origin = priv::toVec3(entry, "origin");
	node.rotation = priv::toVec3(entry, "rotation");

	Log::debug("Node name: %s (%i references)", node.name.c_str(), (int)node.referenced.size());

	auto childrenIter = entry.find("children");
	if (childrenIter == entry.end()) {
		return true;
	}
	const nlohmann::json &children = entry["children"];
	if (children.empty()) {
		return true;
	}
	if (!children.is_array()) {
		Log::error("Children is not an array in json file: %s", filename.c_str());
		return false;
	}

	for (auto iter = children.begin(); iter != children.end(); ++iter) {
		if (iter->is_string()) {
			core::String uuid = priv::toStr(*iter);
			node.referenced.emplace_back(uuid);
			continue;
		}
		if (!iter->is_object()) {
			Log::error("Entry is not an object in json file: %s", filename.c_str());
			return false;
		}
		BlockbenchFormat::Node childNode;
		if (!parseOutliner(filename, *iter, childNode)) {
			return false;
		}
		node.children.emplace_back(core::move(childNode));
	}
	return true;
}

void BlockbenchFormat::fillFace(scenegraph::SceneGraphNode &node, voxel::FaceNames faceName,
								const image::ImagePtr &image, const glm::vec2 &uv0, const glm::vec2 &uv1) const {
	voxel::RawVolumeWrapper wrapper(node.volume());
	const voxel::Region &region = wrapper.region();
	const glm::ivec3 &mins = region.getLowerCorner();
	const glm::ivec3 &maxs = region.getUpperCorner();
	const math::Axis axis = faceToAxis(faceName);
	const int axisIdx = math::getIndexForAxis(axis);
	const int fixedAxisMins = mins[axisIdx];
	const glm::vec3 size = region.getDimensionsInVoxels();
	const palette::Palette &palette = node.palette();

	if (faceName == voxel::FaceNames::NegativeZ) {
		// north, forward
		int z = fixedAxisMins;
		for (int y = mins.y; y <= maxs.y; ++y) {
			const float yFactor = (float)(y - mins.y) / size.y;
			for (int x = mins.x; x <= maxs.x; ++x) {
				const float xFactor = (float)(x - mins.x) / size.x;
				const float u = glm::mix(uv0.x, uv1.x, xFactor);
				const float v = glm::mix(uv0.y, uv1.y, yFactor);
				const core::RGBA color = image->colorAt({u, v});
				int palIdx = palette.getClosestMatch(color);
				if (palIdx == palette::PaletteColorNotFound) {
					palIdx = 0;
				}
				wrapper.setVoxel(x, y, z, voxel::createVoxel(palette, palIdx));
			}
		}
	} else if (faceName == voxel::FaceNames::PositiveZ) {
		int z = maxs.z;
		// south, backward
		for (int y = mins.y; y <= maxs.y; ++y) {
			const float yFactor = (float)(y - mins.y) / size.y;
			for (int x = mins.x; x <= maxs.x; ++x) {
				const float xFactor = (float)(x - mins.x) / size.x;
				const float u = glm::mix(uv0.x, uv1.x, xFactor);
				const float v = glm::mix(uv0.y, uv1.y, yFactor);
				const core::RGBA color = image->colorAt({u, v});
				int palIdx = palette.getClosestMatch(color);
				if (palIdx == palette::PaletteColorNotFound) {
					palIdx = 0;
				}
				wrapper.setVoxel(x, y, z, voxel::createVoxel(palette, palIdx));
			}
		}
	} else if (faceName == voxel::FaceNames::NegativeX) {
		int x = fixedAxisMins;
		for (int z = mins.z + 1; z < maxs.z; ++z) {
			const float zFactor = (float)(z - mins.z) / size.z;
			for (int y = mins.y + 1; y < maxs.y; ++y) {
				const float yFactor = (float)(y - mins.y) / size.y;
				const float u = glm::mix(uv0.x, uv1.x, zFactor);
				const float v = glm::mix(uv0.y, uv1.y, yFactor);
				const core::RGBA color = image->colorAt({u, v});
				int palIdx = palette.getClosestMatch(color);
				if (palIdx == palette::PaletteColorNotFound) {
					palIdx = 0;
				}
				wrapper.setVoxel(x, y, z, voxel::createVoxel(palette, palIdx));
			}
		}
	} else if (faceName == voxel::FaceNames::PositiveX) {
		int x = maxs.x;
		for (int z = mins.z + 1; z < maxs.z; ++z) {
			const float zFactor = (float)(z - mins.z) / size.z;
			for (int y = mins.y + 1; y < maxs.y; ++y) {
				const float yFactor = (float)(y - mins.y) / size.y;
				const float u = glm::mix(uv0.x, uv1.x, zFactor);
				const float v = glm::mix(uv0.y, uv1.y, yFactor);
				const core::RGBA color = image->colorAt({u, v});
				int palIdx = palette.getClosestMatch(color);
				if (palIdx == palette::PaletteColorNotFound) {
					palIdx = 0;
				}
				wrapper.setVoxel(x, y, z, voxel::createVoxel(palette, palIdx));
			}
		}
	} else if (faceName == voxel::FaceNames::NegativeY) {
		int y = fixedAxisMins;
		// down
		for (int z = mins.z + 1; z < maxs.z; ++z) {
			const float zFactor = (float)(z - mins.z) / size.z;
			for (int x = mins.x; x <= maxs.x; ++x) {
				const float xFactor = (float)(x - mins.x) / size.x;
				const float u = glm::mix(uv0.x, uv1.x, xFactor);
				const float v = glm::mix(uv0.y, uv1.y, zFactor);
				const core::RGBA color = image->colorAt({u, v});
				int palIdx = palette.getClosestMatch(color);
				if (palIdx == palette::PaletteColorNotFound) {
					palIdx = 0;
				}
				wrapper.setVoxel(x, y, z, voxel::createVoxel(palette, palIdx));
			}
		}
	} else if (faceName == voxel::FaceNames::PositiveY) {
		int y = maxs.y;
		// up
		for (int z = mins.z + 1; z < maxs.z; ++z) {
			const float zFactor = (float)(z - mins.z) / size.z;
			for (int x = mins.x; x <= maxs.x; ++x) {
				const float xFactor = (float)(x - mins.x) / size.x;
				const float u = glm::mix(uv0.x, uv1.x, xFactor);
				const float v = glm::mix(uv0.y, uv1.y, zFactor);
				const core::RGBA color = image->colorAt({u, v});
				int palIdx = palette.getClosestMatch(color);
				if (palIdx == palette::PaletteColorNotFound) {
					palIdx = 0;
				}
				wrapper.setVoxel(x, y, z, voxel::createVoxel(palette, palIdx));
			}
		}
	}
}

bool BlockbenchFormat::generateVolumeFromElement(const Node &node, const Element &element, const Textures &textureArray,
												 scenegraph::SceneGraph &sceneGraph, int parent) const {
	const Cube &cube = element.cube;
	glm::vec3 size = element.to - element.from;
	// even a plane is one voxel for us
	size.x = glm::clamp(size.x, 1.0f, 1.0f + size.x);
	size.y = glm::clamp(size.y, 1.0f, 1.0f + size.y);
	size.z = glm::clamp(size.z, 1.0f, 1.0f + size.z);
	const glm::vec3 mins = glm::round(element.from);
	const glm::vec3 maxs = mins + size - 1.0f;
	voxel::Region region(mins, maxs);
	if (!region.isValid()) {
		Log::error("Invalid region for element: %s (node: %s)", element.name.c_str(), node.name.c_str());
		return false;
	}
	scenegraph::SceneGraphNode model(scenegraph::SceneGraphNodeType::Model, element.uuid);
	region.shift(-region.getLowerCorner());
	model.setVolume(new voxel::RawVolume(region), true);
	model.setName(element.name);
	model.setLocked(node.locked);
	model.setVisible(node.visible);
	model.rotate(glm::quat(glm::radians(element.rotation)), true);
	model.translate(element.from, true);
	const glm::vec3 pivot = (element.origin - element.from) / size;
	model.setPivot(pivot);
	for (int i = 0; i < (int)voxel::FaceNames::Max; ++i) {
		const Face &face = cube.faces[i];
		if (face.textureIndex == -1) {
			Log::error("No texture index for face: %i", i);
			continue;
		}
		voxel::FaceNames faceName = (voxel::FaceNames)i;
		fillFace(model, faceName, textureArray[face.textureIndex], face.uvs[0], face.uvs[1]);
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
		generateVolumeFromElement(node, element, textureArray, sceneGraph, parent);
	}
	for (const Node &child : node.children) {
		scenegraph::SceneGraphNode group(scenegraph::SceneGraphNodeType::Group, child.uuid);
		group.setName(child.name);
		group.setVisible(child.visible);
		group.setLocked(child.locked);
		group.rotate(glm::quat(glm::radians(child.rotation)), true);
		group.translate(child.origin, true);
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

bool BlockbenchFormat::voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
									  scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return false;
	}

	core::String jsonString;
	stream->readString(stream->remaining(), jsonString);
	nlohmann::json json = nlohmann::json::parse(jsonString);

	const nlohmann::json &meta = json["meta"];
	if (meta.find("format_version") == meta.end()) {
		Log::error("No format_version found in json file: %s", filename.c_str());
		return false;
	}

	// model_format: free bedrock_old java_block

	const core::String &formatVersion = priv::toStr(meta["format_version"]);
	if (formatVersion != "4.5") {
		Log::error("Unsupported format version: %s", formatVersion.c_str());
		return false;
	}

	const nlohmann::json &textures = json["textures"];
	if (!textures.is_array()) {
		Log::error("Textures is not an array in json file: %s", filename.c_str());
		return false;
	}

	Textures textureArray;
	textureArray.reserve(textures.size());

	for (const auto &texture : textures) {
		const core::String &name = priv::toStr(texture["name"]);
		// TODO: allow to load from "path" instead of "source"
		const core::String &source = priv::toStr(texture["source"]);
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
			textureArray.emplace_back(image::loadImage(name, base64Stream, data.size()));
		}
	}

	const nlohmann::json &elementsJson = json["elements"];
	if (!elementsJson.is_array()) {
		Log::error("Elements is not an array in json file: %s", filename.c_str());
		return false;
	}

	ElementMap elementMap;
	if (!parseElements(filename, elementsJson, textureArray, elementMap, sceneGraph)) {
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
		if (!entry.is_object()) {
			Log::error("Entry is not an object in json file: %s", filename.c_str());
			return false;
		}
		if (!parseOutliner(filename, entry, root)) {
			Log::error("Failed to parse outliner");
			return false;
		}
	}

	if (!addNode(root, elementMap, sceneGraph, textureArray, 0)) {
		Log::error("Failed to add node");
		return false;
	}

	return true;
}

} // namespace voxelformat
