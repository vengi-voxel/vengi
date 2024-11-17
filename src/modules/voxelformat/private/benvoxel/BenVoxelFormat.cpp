/**
 * @file
 */

#include "BenVoxelFormat.h"
#include "core/Color.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "io/BufferedReadWriteStream.h"
#include "io/MemoryReadStream.h"
#include "io/Z85.h"
#include "io/ZipReadStream.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "json/JSON.h"

namespace voxelformat {

bool BenVoxelFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
								const io::ArchivePtr &archive, const SaveContext &ctx) {
	return false;
}

bool BenVoxelFormat::loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
									   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
									   const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return false;
	}

	core::String jsonStr;
	if (!stream->readString(stream->size(), jsonStr)) {
		Log::error("Failed to read json file");
		return false;
	}
	nlohmann::json json = nlohmann::json::parse(jsonStr.c_str());
	if (json.is_null()) {
		Log::error("Failed to parse json file");
		return false;
	}

	std::string version = "0.0";
	if (json.find("version") != json.end()) {
		version = json["version"];
	}
	sceneGraph.node(0).setProperty("version", version.c_str());

	if (json.find("metadata") == json.end()) {
		Log::error("Metadata not found in json file");
		return false;
	}

	const nlohmann::json &metadata = json["metadata"];
	if (metadata.find("palettes") == metadata.end()) {
		Log::error("Palettes not found in json file");
		return false;
	}

	const nlohmann::json &palettes = metadata["palettes"];
	for (const auto &entry : palettes.items()) {
		const core::String &name = entry.key().c_str();
		const nlohmann::json &jpalette = entry.value();
		if (jpalette.is_array()) {
			for (const auto &color : jpalette) {
				if (color.find("rgba") == color.end()) {
					Log::error("RGBA not found in json file");
					return false;
				}
				const std::string &rgba = color["rgba"];
				core::RGBA colorRGBA = core::Color::fromHex(rgba.c_str());
				palette.tryAdd(colorRGBA, false);
			}
		}
	}

	if (metadata.find("properties") != metadata.end()) {
		const nlohmann::json &properties = metadata["properties"];
		for (const auto &entry : properties.items()) {
			const core::String &name = entry.key().c_str();
			const nlohmann::json &value = entry.value();
			if (value.is_string()) {
				std::string valueStr = value;
				sceneGraph.node(0).setProperty(name.c_str(), valueStr.c_str());
			}
		}
	}

	if (metadata.find("points") != metadata.end()) {
		const nlohmann::json &points = metadata["points"];
		for (const auto &entry : points.items()) {
			const core::String &name = entry.key().c_str();
			const nlohmann::json &value = entry.value();
			if (value.is_array() && value.size() == 3) {
				const glm::vec3 pointPos(value[0], value[1], value[2]);
				scenegraph::SceneGraphNode pointNode(scenegraph::SceneGraphNodeType::Point);
				pointNode.setName(name);
				scenegraph::SceneGraphTransform transform;
				transform.setLocalTranslation(pointPos);
				pointNode.setTransform(0, transform);
				sceneGraph.emplace(core::move(pointNode));
			} else {
				Log::error("Invalid format for vec3 property: %s", name.c_str());
			}
		}
	}

	if (json.find("models") == json.end()) {
		Log::error("Models not found in json file");
		return false;
	}

	const nlohmann::json &models = json["models"];
	for (const auto &entry : models.items()) {
		const core::String &name = entry.key().c_str();
		const nlohmann::json &model = entry.value();
		if (model.find("geometry") == model.end()) {
			Log::error("Geometry not found in json file");
			return false;
		}

		const nlohmann::json &geometry = model["geometry"];
		if (geometry.find("size") == geometry.end()) {
			Log::error("Size not found in json file");
			return false;
		}

		const nlohmann::json &size = geometry["size"];
		if (size.size() != 3) {
			Log::error("Size must have 3 elements");
			return false;
		}

		const int width = size[0];
		const int height = size[1];
		const int depth = size[2];
		Log::debug("Model: '%s', size: %dx%dx%d", name.c_str(), width, height, depth);

		if (geometry.find("z85") == geometry.end()) {
			Log::error("Z85 not found in json file");
			return false;
		}

		const std::string &z85 = geometry["z85"];
		io::BufferedReadWriteStream z85Stream;
		io::MemoryReadStream z85InStream(z85.data(), z85.size());
		if (!io::Z85::decode(z85Stream, z85InStream)) {
			Log::error("Failed to decode z85");
			return false;
		}
		if (z85Stream.seek(0) == -1) {
			Log::error("Failed to seek to start of stream");
			return false;
		}

		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setName(name);
		voxel::Region region(0, 0, 0, width - 1, height - 1, depth - 1);
		voxel::RawVolume *v = new voxel::RawVolume(region);
		node.setVolume(v, true);

		io::ZipReadStream zipStream(z85Stream, z85Stream.size());
		while (!zipStream.eos()) {
			uint16_t x, y, z;
			if (zipStream.readUInt16(x) == -1) {
				Log::error("Failed to read x");
				return false;
			}
			if (zipStream.readUInt16(y) == -1) {
				Log::error("Failed to read y");
				return false;
			}
			if (zipStream.readUInt16(z) == -1) {
				Log::error("Failed to read z");
				return false;
			}
			uint8_t color;
			if (zipStream.readUInt8(color) == -1) {
				Log::error("Failed to read color");
				return false;
			}
			if (color == 0) {
				continue;
			}
			v->setVoxel(x, y, z, voxel::createVoxel(palette, color));
		}

		sceneGraph.emplace(core::move(node));
	}

	return true;
}

size_t BenVoxelFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive,
								   palette::Palette &palette, const LoadContext &ctx) {
	return 0;
}

} // namespace voxelformat
