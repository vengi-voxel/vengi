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

static bool loadMetadataJson(const json::Json &json, Metadata &metadata) {
	// metadata is optional
	if (!json.contains("metadata")) {
		return true;
	}
	json::Json metadataJson = json.get("metadata");
	if (metadataJson.contains("palettes")) {
		json::Json palettesJson = metadataJson.get("palettes");
		for (auto it = palettesJson.begin(); it != palettesJson.end(); ++it) {
			const core::String name = it.key();
			palette::Palette palette;
			palette.setName(name);
			json::Json paletteJson = *it;
			if (paletteJson.isArray()) {
				int idx = 0;
				for (auto cIt = paletteJson.begin(); cIt != paletteJson.end(); ++cIt) {
					json::Json paletteColorJson = *cIt;
					if (!paletteColorJson.contains("rgba")) {
						Log::error("RGBA not found in json file");
						return false;
					}
					const core::String rgba = paletteColorJson.get("rgba").str();
					color::RGBA colorRGBA = color::fromHex(rgba.c_str());
					palette.setColor(idx, colorRGBA);
					++idx;
				}
			}
			metadata.palettes.emplace(name, core::move(palette));
		}
	}

	if (metadataJson.contains("properties")) {
		json::Json propertiesJson = metadataJson.get("properties");
		for (auto it = propertiesJson.begin(); it != propertiesJson.end(); ++it) {
			const core::String name = it.key();
			json::Json valueJson = *it;
			if (valueJson.isString()) {
				metadata.properties.put(name, valueJson.str());
			}
		}
	}

	if (metadataJson.contains("points")) {
		json::Json pointsJson = metadataJson.get("points");
		for (auto it = pointsJson.begin(); it != pointsJson.end(); ++it) {
			const core::String name = it.key();
			json::Json valueJson = *it;
			if (valueJson.isArray() && valueJson.size() == 3) {
				const glm::vec3 pointPos(valueJson.get(0).floatVal(), valueJson.get(2).floatVal(), valueJson.get(1).floatVal());
				metadata.points.emplace_back(name, pointPos);
			} else {
				Log::error("Invalid format for vec3 property: %s", name.c_str());
			}
		}
	}
	return true;
}

bool loadJson(scenegraph::SceneGraph &sceneGraph, palette::Palette &palette, const core::String &jsonStr) {
	json::Json json = json::Json::parse(jsonStr.c_str());
	if (json.isNull()) {
		Log::error("Failed to parse json file");
		return false;
	}

	core::String version = "0.0";
	if (json.contains("version")) {
		version = json.get("version").str();
	}
	scenegraph::SceneGraphNode &root = sceneGraph.node(0);
	root.setProperty("version", version.c_str());

	Metadata globalMetadata;
	if (!loadMetadataJson(json, globalMetadata)) {
		Log::error("Failed to load metadata");
		return false;
	}

	if (!json.contains("models")) {
		Log::error("Models not found in json file");
		return false;
	}

	json::Json modelsJson = json.get("models");
	for (auto it = modelsJson.begin(); it != modelsJson.end(); ++it) {
		const core::String name = it.key();
		json::Json modelJson = *it;
		if (!modelJson.contains("geometry")) {
			Log::error("Geometry not found in json file");
			return false;
		}

		Metadata metadata;
		if (modelJson.contains("metadata")) {
			if (!loadMetadataJson(modelJson, metadata)) {
				Log::error("Failed to load metadata for model");
				return false;
			}
		}

		json::Json geometryJson = modelJson.get("geometry");
		if (!geometryJson.contains("size")) {
			Log::error("Size not found in json file");
			return false;
		}

		json::Json sizeJson = geometryJson.get("size");
		if (sizeJson.size() != 3) {
			Log::error("Size must have 3 elements");
			return false;
		}

		const int width = sizeJson.get(0).intVal();
		const int depth = sizeJson.get(1).intVal();
		const int height = sizeJson.get(2).intVal();
		Log::debug("Model: '%s', size: %dx%dx%d", name.c_str(), width, height, depth);

		if (!geometryJson.contains("z85")) {
			Log::error("Z85 not found in json file");
			return false;
		}

		const core::String z85 = geometryJson.get("z85").str();
		if (z85.empty()) {
			Log::error("Empty z85 encoded data");
			return false;
		}
		io::MemoryReadStream z85InStream(z85.c_str(), z85.size());

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

static bool writeMetadataJson(json::Json &json, const scenegraph::SceneGraph &sceneGraph,
							  const scenegraph::SceneGraphNode &node) {
	Metadata metadata = createMetadata(sceneGraph, node);

	json::Json metadataJson = json::Json::object();

	json::Json palettesJson = json::Json::object();
	for (const auto &entry : metadata.palettes) {
		const core::String &name = entry->first;
		const palette::Palette &palette = entry->second;
		json::Json colorsJson = json::Json::array();
		for (int i = 0; i < (int)palette.size(); ++i) {
			const color::RGBA &color = palette.color(i);
			json::Json colorJson = json::Json::object();
			colorJson.set("rgba", color::toHex(color));
			colorsJson.push_back(colorJson);
		}
		json::Json paletteJson = json::Json::object();
		paletteJson.set("colors", colorsJson);
		palettesJson.set(name.c_str(), paletteJson);
	}
	metadataJson.set("palettes", palettesJson);

	json::Json propertiesJson = json::Json::object();
	for (const auto &entry : metadata.properties) {
		const core::String &name = entry->first;
		const core::String &value = entry->second;
		propertiesJson.set(name.c_str(), value.c_str());
	}
	metadataJson.set("properties", propertiesJson);

	json::Json pointsJson = json::Json::object();
	for (const PointNode &pointNode : metadata.points) {
		json::Json pointArr = json::Json::array();
		pointArr.push_back((double)pointNode.pointPos.x);
		pointArr.push_back((double)pointNode.pointPos.z);
		pointArr.push_back((double)pointNode.pointPos.y);
		pointsJson.set(pointNode.name.c_str(), pointArr);
	}
	metadataJson.set("points", pointsJson);

	json.set("metadata", metadataJson);
	return true;
}

bool saveJson(const scenegraph::SceneGraph &sceneGraph, io::SeekableWriteStream &stream) {
	json::Json json = json::Json::object();
	json.set("version", "0.0");

	writeMetadataJson(json, sceneGraph, sceneGraph.root());

	json::Json modelsJson = json::Json::object();
	for (const auto &entry : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = entry->value;
		if (!node.isAnyModelNode()) {
			continue;
		}
		const voxel::RawVolume *volume = sceneGraph.resolveVolume(node);

		json::Json modelJson = json::Json::object();

		// Write per-model metadata
		writeMetadataJson(modelJson, sceneGraph, node);

		json::Json geometryJson = json::Json::object();

		json::Json sizeArr = json::Json::array();
		const voxel::Region &region = volume->region();
		const glm::ivec3 &dim = region.getDimensionsInVoxels();
		sizeArr.push_back(dim.x);
		sizeArr.push_back(dim.z);
		sizeArr.push_back(dim.y);
		geometryJson.set("size", sizeArr);

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
		geometryJson.set("z85", z85.c_str());

		modelJson.set("geometry", geometryJson);
		modelsJson.set(node.name().c_str(), modelJson);
	}
	json.set("models", modelsJson);

	const core::String &jsonString = json.dump();
	if (!stream.writeString(jsonString.c_str(), false)) {
		Log::error("Failed to write json file");
		return false;
	}
	return true;
}

} // namespace benv

} // namespace voxelformat
