/**
 * @file
 */

#include "KenShapeFormat.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "io/BufferedReadWriteStream.h"
#include "io/Stream.h"
#include "io/ZipReadStream.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeProperties.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "json/JSON.h"

namespace voxelformat {

struct KenTile {
	int shape;
	int angle;
	int color;
	int depth;
	bool enabled;
	bool visited;
	int colorBack;
	int depthBack;

	KenTile(int _shape, int _angle, int _color, int _depth, bool _enabled, bool _visited, int _colorBack,
			 int _depthBack)
		: shape(_shape), angle(_angle), color(_color), depth(_depth), enabled(_enabled), visited(_visited),
		  colorBack(_colorBack), depthBack(_depthBack) {
	}
};

bool KenShapeFormat::loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
									   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
									   const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return 0;
	}

	io::ZipReadStream zipStream(*stream, stream->size());
	io::BufferedReadWriteStream wrapper(zipStream);
	const int64_t jsonSize = wrapper.size();
	core::String jsonStr(jsonSize, ' ');
	if (!wrapper.readString((int)jsonStr.size(), jsonStr.c_str())) {
		Log::error("Failed to read string from stream");
		return false;
	}

	nlohmann::json json = nlohmann::json::parse(jsonStr, nullptr, false, true);

	int maxDepth = 0;
	// parse tiles
	const nlohmann::json &tiles = json["tiles"];
	core::Buffer<KenTile> kenTiles;
	for (const auto &tile : tiles) {
		const int shape = tile.value("shape", 0);
		const int angle = tile.value("angle", 0);
		const int color = tile.value("color", -1);
		const int depth = tile.value("depth", 0);
		const bool enabled = tile.value("enabled", true);
		const bool visited = tile.value("visited", false);
		const int colorBack = tile.value("colorBack", -1);
		const int depthBack = tile.value("depthBack", -1);
		kenTiles.emplace_back(shape, angle, color, depth, enabled, visited, colorBack, depthBack);
		maxDepth = core_max(maxDepth, depth);
		maxDepth = core_max(maxDepth, depthBack);
	}

	// parse palette
	if (!json.contains("colors")) {
		Log::error("Missing colors in %s", filename.c_str());
		return false;
	}
	const nlohmann::json &colors = json["colors"];
	int n = 0;
	for (const auto &color : colors) {
		const std::string &hex = color.get<std::string>();
		color::RGBA rgba(0, 0, 0, 255);
		core::string::parseHex(hex.c_str(), rgba.r, rgba.g, rgba.b, rgba.a);
		palette.setColor(n, rgba);
		++n;
	}
	palette.setSize(n);
	Log::debug("Found %i colors", n);

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setPalette(palette);

	node.setName(json::toStr(json, "title"));
	node.setProperty(scenegraph::PropVersion, json::toStr(json, "version"));
	node.setProperty(scenegraph::PropAuthor, json::toStr(json, "author"));
	auto size = json["size"];
	const int w = size.value("x", 1) - 1;
	const int h = size.value("y", 1) - 1;
	const int alignment = json.value("alignment", 0);
	const int depthMultiplier = json.value("depthMultiplier", 0);
	Log::debug("size: w(%d) h(%d)", w, h);
	Log::debug("alignment: %d", alignment);
	Log::debug("depthMultiplier: %d", depthMultiplier);
	Log::debug("version: %s", node.property(scenegraph::PropVersion).c_str());
	Log::debug("author: %s", node.property(scenegraph::PropAuthor).c_str());
	Log::debug("title: %s", node.name().c_str());

	voxel::Region region(0, 0, -maxDepth, w, h, maxDepth);
	if (!region.isValid()) {
		Log::error("Invalid region");
		return false;
	}
	voxel::RawVolume *volume = new voxel::RawVolume(region);
	node.setVolume(volume, true);

	// fill volume
	glm::ivec3 pos(0, h, 0);
	for (const KenTile &tile : kenTiles) {
		if (!tile.enabled) {
			continue;
		}
		if (tile.color != -1) {
			int idx = tile.color;
			int backIdx = idx;
			if (tile.colorBack != -1) {
				backIdx = tile.colorBack;
			}

			int stepsBack;
			if (tile.depthBack > 0) {
				stepsBack = tile.depthBack;
			} else {
				stepsBack = tile.depth;
			}

			const voxel::Voxel backVoxel = voxel::createVoxel(voxel::VoxelType::Generic, backIdx);
			// TODO: PERF: use volume sampler
			for (int steps = 0; steps < stepsBack; ++steps) {
				volume->setVoxel(pos.x, pos.y, pos.z - steps, backVoxel);
			}

			const int stepsFront = tile.depth;
			const voxel::Voxel frontVoxel = voxel::createVoxel(voxel::VoxelType::Generic, idx);
			// TODO: PERF: use volume sampler
			for (int steps = 1; steps <= stepsFront; ++steps) {
				volume->setVoxel(pos.x, pos.y, pos.z + steps, frontVoxel);
			}
		}

		--pos.y;
		if (pos.y < 0) {
			pos.y = h;
			++pos.x;
		}
	}

	return sceneGraph.emplace(core::move(node)) != InvalidNodeId;
}

bool KenShapeFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
								const io::ArchivePtr &archive, const SaveContext &ctx) {
	return false;
}

size_t KenShapeFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive,
								   palette::Palette &palette, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return 0;
	}

	io::ZipReadStream zipStream(*stream, stream->size());
	io::BufferedReadWriteStream wrapper(zipStream);
	const int64_t size = wrapper.size();
	core::String jsonStr(size, ' ');
	if (!wrapper.readString((int)jsonStr.size(), jsonStr.c_str())) {
		Log::error("Failed to read string from stream");
		return false;
	}

	nlohmann::json json = nlohmann::json::parse(jsonStr, nullptr, false, true);
	if (!json.contains("colors")) {
		Log::error("Missing colors in %s", filename.c_str());
		return false;
	}
	const nlohmann::json &colors = json["colors"];
	int n = 0;
	for (const auto &color : colors) {
		const std::string &hex = color.get<std::string>();
		color::RGBA rgba(0, 0, 0, 255);
		core::string::parseHex(hex.c_str(), rgba.r, rgba.g, rgba.b, rgba.a);
		palette.setColor(n, rgba);
		++n;
	}
	palette.setSize(n);
	return palette.size();
}

} // namespace voxelformat
