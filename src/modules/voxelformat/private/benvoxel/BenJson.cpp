/**
 * @file
 */

#include "BenJson.h"
#include "BenShared.h"
#include "color/Color.h"
#include "core/Log.h"
#include "io/BufferedReadWriteStream.h"
#include "io/MemoryReadStream.h"
#include "io/Z85.h"
#include "io/ZipReadStream.h"
#include "io/ZipWriteStream.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "json/JSON.h"

namespace voxelformat {

namespace benv {

static bool loadMetadataJson(const nlohmann::json &json, Metadata &metadata) {
	// metadata is optional
	if (json.find("metadata") == json.end()) {
		return true;
	}
	const nlohmann::json &metadataJson = json["metadata"];
	if (metadataJson.find("palettes") != metadataJson.end()) {
		const nlohmann::json &palettesJson = metadataJson["palettes"];
		for (const auto &paletteJsonEntry : palettesJson.items()) {
			const core::String &name = paletteJsonEntry.key().c_str();
			palette::Palette palette;
			palette.setName(name);
			const nlohmann::json &paletteJson = paletteJsonEntry.value();
			if (paletteJson.is_array()) {
				int idx = 0;
				for (const auto &paletteColorJson : paletteJson) {
					if (paletteColorJson.find("rgba") == paletteColorJson.end()) {
						Log::error("RGBA not found in json file");
						return false;
					}
					const std::string &rgba = paletteColorJson["rgba"];
					color::RGBA colorRGBA = color::fromHex(rgba.c_str());
					palette.setColor(idx, colorRGBA);
					++idx;
				}
			}
			metadata.palettes.emplace(name, core::move(palette));
		}
	}

	if (metadataJson.find("properties") != metadataJson.end()) {
		const nlohmann::json &propertiesJson = metadataJson["properties"];
		for (const auto &propertyJson : propertiesJson.items()) {
			const core::String &name = propertyJson.key().c_str();
			const nlohmann::json &valueJson = propertyJson.value();
			if (valueJson.is_string()) {
				const std::string &valueStr = valueJson;
				metadata.properties.emplace(name, valueStr.c_str());
			}
		}
	}

	if (metadataJson.find("points") != metadataJson.end()) {
		const nlohmann::json &pointsJson = metadataJson["points"];
		for (const auto &pointJson : pointsJson.items()) {
			const core::String &name = pointJson.key().c_str();
			const nlohmann::json &valueJson = pointJson.value();
			if (valueJson.is_array() && valueJson.size() == 3) {
				const glm::vec3 pointPos(valueJson[0], valueJson[2], valueJson[1]);
				metadata.points.emplace_back(name, pointPos);
			} else {
				Log::error("Invalid format for vec3 property: %s", name.c_str());
			}
		}
	}
	return true;
}

bool loadJson(scenegraph::SceneGraph &sceneGraph, palette::Palette &palette, const core::String &jsonStr) {
	nlohmann::json json = nlohmann::json::parse(jsonStr.c_str());
	if (json.is_null()) {
		Log::error("Failed to parse json file");
		return false;
	}

	std::string version = "0.0";
	if (json.find("version") != json.end()) {
		version = json["version"];
	}
	scenegraph::SceneGraphNode &root = sceneGraph.node(0);
	root.setProperty("version", version.c_str());

	Metadata globalMetadata;
	if (!loadMetadataJson(json, globalMetadata)) {
		Log::error("Failed to load metadata");
		return false;
	}

	if (json.find("models") == json.end()) {
		Log::error("Models not found in json file");
		return false;
	}

	const nlohmann::json &modelsJson = json["models"];
	for (const auto &modelJsonEntry : modelsJson.items()) {
		const core::String &name = modelJsonEntry.key().c_str();
		const nlohmann::json &modelJson = modelJsonEntry.value();
		if (modelJson.find("geometry") == modelJson.end()) {
			Log::error("Geometry not found in json file");
			return false;
		}

		Metadata metadata;
		if (modelJson.find("metadata") != modelJson.end()) {
			if (!loadMetadataJson(modelJson, metadata)) {
				Log::error("Failed to load metadata for model");
				return false;
			}
		}

		const nlohmann::json &geometryJson = modelJson["geometry"];
		if (geometryJson.find("size") == geometryJson.end()) {
			Log::error("Size not found in json file");
			return false;
		}

		const nlohmann::json &sizeJson = geometryJson["size"];
		if (sizeJson.size() != 3) {
			Log::error("Size must have 3 elements");
			return false;
		}

		const int width = sizeJson[0];
		const int depth = sizeJson[1];
		const int height = sizeJson[2];
		Log::debug("Model: '%s', size: %dx%dx%d", name.c_str(), width, height, depth);

		if (geometryJson.find("z85") == geometryJson.end()) {
			Log::error("Z85 not found in json file");
			return false;
		}

		const std::string &z85 = geometryJson["z85"];
		if (z85.empty()) {
			Log::error("Empty z85 encoded data");
			return false;
		}
		io::MemoryReadStream z85InStream(z85.data(), z85.size());

		io::BufferedReadWriteStream z85Stream(z85.size());
		if (!io::Z85::decode(z85Stream, z85InStream)) {
			Log::error("Failed to decode z85");
			return false;
		}
		if (z85Stream.seek(0) != 0) {
			Log::error("Failed to seek to start of z85 stream");
			return false;
		}
		io::ZipReadStream zipStream(z85Stream, z85Stream.size());
		io::BufferedReadWriteStream wrapper(zipStream);
		if (wrapper.empty()) {
			Log::error("Could not load deflated z85 data of size %i", (int)z85Stream.size());
			return false;
		}
		int nodeId =
			createModelNode(sceneGraph, palette, name, width, height, depth, wrapper, globalMetadata, metadata);
		if (nodeId == InvalidNodeId) {
			return false;
		}
	}
	for (const PointNode &pointNode : globalMetadata.points) {
		if (!addPointNode(sceneGraph, pointNode.name, pointNode.pointPos)) {
			Log::error("Failed to add point node");
			return false;
		}
	}

	return true;
}

static bool writeMetadataJson(nlohmann::json &json, const scenegraph::SceneGraph &sceneGraph,
							  const scenegraph::SceneGraphNode &node) {
	Metadata metadata = createMetadata(sceneGraph, node);
	nlohmann::json &metadataJson = json["metadata"];
	nlohmann::json &palettesJson = metadataJson["palettes"];
	for (const auto &entry : metadata.palettes) {
		const core::String &name = entry->first;
		const palette::Palette &palette = entry->second;
		nlohmann::json &paletteJson = palettesJson[name.c_str()];
		nlohmann::json &colorsJson = paletteJson["colors"];
		for (int i = 0; i < (int)palette.size(); ++i) {
			const color::RGBA &color = palette.color(i);
			nlohmann::json &colorJson = colorsJson[i];
			colorJson["rgba"] = color::toHex(color);
		}
	}

	nlohmann::json &propertiesJson = metadataJson["properties"];
	for (const auto &entry : metadata.properties) {
		const core::String &name = entry->first;
		const core::String &value = entry->second;
		propertiesJson[name.c_str()] = value.c_str();
	}

	nlohmann::json &pointsJson = metadataJson["points"];
	for (const PointNode &pointNode : metadata.points) {
		nlohmann::json &pointJson = pointsJson[pointNode.name.c_str()];
		pointJson.push_back(pointNode.pointPos.x);
		pointJson.push_back(pointNode.pointPos.z);
		pointJson.push_back(pointNode.pointPos.y);
	}

	return true;
}

bool saveJson(const scenegraph::SceneGraph &sceneGraph, io::SeekableWriteStream &stream) {
	nlohmann::json json;
	json["version"] = "0.0";

	writeMetadataJson(json, sceneGraph, sceneGraph.root());

	nlohmann::json &modelsJson = json["models"];
	for (const auto &entry : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = entry->value;
		if (!node.isAnyModelNode()) {
			continue;
		}
		const voxel::RawVolume *volume = sceneGraph.resolveVolume(node);
		nlohmann::json &modelJson = modelsJson[node.name().c_str()];

		nlohmann::json &metadataJson = modelJson["metadata"];
		if (!writeMetadataJson(metadataJson, sceneGraph, node)) {
			Log::error("Failed to write metadata");
			return false;
		}

		nlohmann::json &geometryJson = modelJson["geometry"];

		nlohmann::json &sizeJson = geometryJson["size"];
		const voxel::Region &region = volume->region();
		const glm::ivec3 &dim = region.getDimensionsInVoxels();
		sizeJson.push_back(dim.x);
		sizeJson.push_back(dim.z);
		sizeJson.push_back(dim.y);

		io::BufferedReadWriteStream wrapper;
		io::ZipWriteStream zipStream(wrapper, 6, true);
		if (!saveModel(sceneGraph, node, zipStream, false)) {
			Log::error("Failed to save model binary");
			return false;
		}
		if (!zipStream.flush()) {
			Log::error("Failed to flush zip stream");
			return false;
		}
		if (wrapper.seek(0) == -1) {
			Log::error("Failed to seek to start of stream");
			return false;
		}
		const core::String &z85 = io::Z85::encode(wrapper);
		geometryJson["z85"] = z85.c_str();
	}

	const auto &jsonString = json.dump();
	if (!stream.writeString(jsonString.c_str(), false)) {
		Log::error("Failed to write json file");
		return false;
	}
	return true;
}

} // namespace benv

} // namespace voxelformat
