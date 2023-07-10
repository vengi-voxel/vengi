/**
 * @file
 */

#include "VBXFormat.h"
#include "core/Color.h"
#include "core/Log.h"
#include "core/RGBA.h"
#include "core/StringUtil.h"
#include "core/Tokenizer.h"
#include "core/collection/DynamicSet.h"
#include "core/collection/StringMap.h"
#include "io/BufferedReadWriteStream.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "util/Base64.h"
#include "util/IniParser.h"
#include "voxel/Palette.h"
#include "voxel/RawVolume.h"
#include "voxelformat/private/mesh/GLTFFormat.h"
#include "voxelformat/VolumeFormat.h"
#include <limits.h>

namespace voxelformat {

bool VBXFormat::loadGLB(const core::String &data, scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) const {
	if (!core::string::startsWith(data, "data:application/octet-stream;base64,")) {
		Log::error("Invalid data URI: %s", data.c_str());
		return false;
	}
	io::BufferedReadWriteStream stream;
	if (!util::Base64::decode(stream, data.substr(37))) {
		Log::error("Failed to decode base64 data: %s", data.c_str());
	}
	stream.seek(0);
	GLTFFormat format;
	if (!format.load("file.glb", stream, sceneGraph, ctx)) {
		Log::error("Failed to load embedded glb file: %s", data.c_str());
	}
	return true;
}

template<class FUNC>
static bool loadVoxels(const core::String &voxels, FUNC func) {
	size_t n = 0;
	for (;;) {
		size_t iter = voxels.find(";", n);
		if (iter == core::String::npos) {
			break;
		}
		const core::String &voxel = voxels.substr(n, iter - n);
		n = iter + 1;
		core::DynamicArray<core::String> tokens;
		core::string::splitString(voxel, tokens, ",");
		if (tokens.size() != 5) {
			Log::error("Invalid voxel: %s", voxel.c_str());
			return false;
		}
		const bool visible = core::string::toBool(tokens[4]);
		if (!visible) {
			continue;
		}
		const int x = core::string::toInt(tokens[0]);
		const int y = core::string::toInt(tokens[1]);
		const int z = core::string::toInt(tokens[2]);
		const core::RGBA color = core::Color::getRGBA(core::Color::fromHex(tokens[3].c_str()));
		const glm::ivec3 pos(x, y, z);
		func(pos, color);
	}
	return true;
}

bool VBXFormat::loadGroupsRGBA(const core::String &filename, io::SeekableReadStream &stream,
							   scenegraph::SceneGraph &sceneGraph, const voxel::Palette &palette,
							   const LoadContext &ctx) {
	core::StringMap<core::StringMap<core::String>> ini;
	if (!util::parseIni(stream, ini)) {
		Log::error("Failed to parse ini file: %s", filename.c_str());
		return false;
	}

	auto project = ini.find("project");
	if (project != ini.end()) {
		scenegraph::SceneGraphNode &rootNode = sceneGraph.node(0);
		for (auto iter : project->second) {
			if (iter->first == "name") {
				rootNode.setName(iter->second);
			} else {
				rootNode.setProperty(iter->first, iter->second);
			}
		}
	}

	auto data = ini.find("data");
	if (data == ini.end()) {
		Log::error("No data section found in: %s", filename.c_str());
		return false;
	}
	for (auto iter : data->second) {
		if (iter->first == "voxels") {
			glm::ivec3 mins(INT_MAX);
			glm::ivec3 maxs(INT_MIN);
			if (!loadVoxels(iter->second, [&mins, &maxs](const glm::ivec3 &pos, const core::RGBA &color) {
					mins = glm::min(mins, pos);
					maxs = glm::max(maxs, pos);
				})) {
				Log::error("Failed to load voxel volume dimensions from: %s", iter->second.c_str());
				return false;
			}
			voxel::Region region(mins, maxs);
			if (!region.isValid()) {
				Log::error("Invalid mins/maxs for region: %d/%d/%d - %d/%d/%d", mins.x, mins.y, mins.z, maxs.x, maxs.y,
						   maxs.z);
				continue;
			}
			voxel::RawVolume *volume = new voxel::RawVolume(region);
			if (!loadVoxels(iter->second, [volume, &palette](const glm::ivec3 &pos, const core::RGBA &color) {
					volume->setVoxel(pos, voxel::createVoxel(palette, palette.getClosestMatch(color)));
				})) {
				Log::error("Failed to load voxel volume from: %s", iter->second.c_str());
				delete volume;
				return false;
			}
			scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
			node.setVolume(volume, true);
			node.setName(filename);
			node.setPalette(palette);
			if (sceneGraph.emplace(core::move(node)) == InvalidNodeId) {
				Log::error("Failed to add node to scene graph: %s", filename.c_str());
			}
		} else if (iter->first == "bakes") {
			if (!loadGLB(iter->second, sceneGraph, ctx)) {
				Log::error("Failed to load bakes from: %s", iter->second.c_str());
				return false;
			}
		}
	}

	return true;
}

size_t VBXFormat::loadPalette(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette,
							  const LoadContext &ctx) {
	util::IniMap ini;
	if (!util::parseIni(stream, ini)) {
		Log::error("Failed to parse ini file: %s", filename.c_str());
		return 0;
	}

	core::DynamicSet<core::RGBA, 11, core::RGBAHasher> colors;
	Log::debug("create palette");

	auto data = ini.find("data");
	if (data == ini.end()) {
		for (auto iter : data->second) {
			if (iter->first == "voxels") {
				if (!loadVoxels(iter->second,
								[&colors](const glm::ivec3 &pos, const core::RGBA &color) { colors.insert(color); })) {
					Log::error("Failed to load voxel volume dimensions from: %s", iter->second.c_str());
					return 0;
				}
			}
		}
	}

	const size_t colorCount = colors.size();
	core::Buffer<core::RGBA> colorBuffer;
	colorBuffer.reserve(colorCount);
	for (const auto &e : colors) {
		colorBuffer.push_back(e->first);
	}
	palette.quantize(colorBuffer.data(), colorBuffer.size());
	return palette.colorCount();
}

} // namespace voxelformat
