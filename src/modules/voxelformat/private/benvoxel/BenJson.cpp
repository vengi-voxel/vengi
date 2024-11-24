/**
 * @file
 */

#include "BenJson.h"
#include "BenShared.h"
#include "core/Color.h"
#include "core/Log.h"
#include "io/BufferedReadWriteStream.h"
#include "io/MemoryReadStream.h"
#include "io/Z85.h"
#include "io/ZipReadStream.h"
#include "scenegraph/SceneGraphNode.h"
#include "json/JSON.h"

namespace voxelformat {

namespace benv {

static bool loadMetadataJson(nlohmann::json &json, Metadata &metadata) {
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
				for (const auto &paletteColorJson : paletteJson) {
					if (paletteColorJson.find("rgba") == paletteColorJson.end()) {
						Log::error("RGBA not found in json file");
						return false;
					}
					const std::string &rgba = paletteColorJson["rgba"];
					core::RGBA colorRGBA = core::Color::fromHex(rgba.c_str());
					palette.tryAdd(colorRGBA, false);
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
				const glm::vec3 pointPos(valueJson[0], valueJson[1], valueJson[2]);
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
		io::MemoryReadStream z85InStream(z85.data(), z85.size());

		io::BufferedReadWriteStream z85Stream;
		if (!io::Z85::decode(z85Stream, z85InStream)) {
			Log::error("Failed to decode z85");
			return false;
		}
		if (z85Stream.seek(0) != 0) {
			Log::error("Failed to seek to start of stream");
			return false;
		}
		io::ZipReadStream zipStream(z85Stream, z85Stream.size());
		io::BufferedReadWriteStream wrapper(zipStream);
		Metadata metadata; // TODO: VOXELFORMAT: metadata per model?
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

} // namespace benv

} // namespace voxelformat
