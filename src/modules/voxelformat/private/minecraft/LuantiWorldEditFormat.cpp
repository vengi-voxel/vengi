/**
 * @file
 */

#include "LuantiWorldEditFormat.h"
#include "color/RGBA.h"
#include "commonlua/LUA.h"
#include "core/Hash.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/StringMap.h"
#include "io/Archive.h"
#include "io/Stream.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

namespace {

struct LuantiNode {
	glm::ivec3 pos;
	core::String name;
};

static bool getIntField(lua_State *state, int tableIndex, const char *fieldName, int &value) {
	lua_getfield(state, tableIndex, fieldName);
	if (!lua_isnumber(state, -1)) {
		lua_pop(state, 1);
		return false;
	}
	value = (int)lua_tointeger(state, -1);
	lua_pop(state, 1);
	return true;
}

static bool getStringField(lua_State *state, int tableIndex, const char *fieldName, core::String &value) {
	lua_getfield(state, tableIndex, fieldName);
	if (!lua_isstring(state, -1)) {
		lua_pop(state, 1);
		return false;
	}
	value = lua_tostring(state, -1);
	lua_pop(state, 1);
	return true;
}

static bool isAirNode(const core::String &name) {
	return name.empty() || name == "air" || name == "ignore" || name == "mg:ignore";
}

static color::RGBA nodeColor(const core::String &name) {
	const uint32_t hash = core::hash((const void *)name.c_str(), (int)name.size());
	const uint8_t r = 64u + (hash & 0x7fu);
	const uint8_t g = 64u + ((hash >> 8u) & 0x7fu);
	const uint8_t b = 64u + ((hash >> 16u) & 0x7fu);
	return color::RGBA(r, g, b, 255);
}

static uint8_t paletteIndex(const core::String &name, palette::Palette &palette, core::StringMap<uint8_t> &paletteIndices) {
	uint8_t palIdx;
	if (paletteIndices.get(name, palIdx)) {
		return palIdx;
	}

	const color::RGBA rgba = nodeColor(name);
	if (palette.colorCount() < palette::PaletteMaxColors) {
		palIdx = (uint8_t)palette.colorCount();
		palette.setColor(palIdx, rgba);
		palette.setColorName(palIdx, name);
	} else {
		palIdx = (uint8_t)palette.getClosestMatch(rgba);
		Log::warn("Palette limit reached while loading Luanti WorldEdit node '%s' - reusing palette index %u",
				  name.c_str(), palIdx);
	}
	paletteIndices.put(name, palIdx);
	return palIdx;
}

} // namespace

namespace voxelformat {

bool LuantiWorldEditFormat::loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
										scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
										const LoadContext &ctx) {
	(void)ctx;
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}

	core::String luaString;
	if (!stream->readString((int)stream->size(), luaString)) {
		Log::error("Could not read file %s", filename.c_str());
		return false;
	}

	core::String luaChunk = luaString;
	const size_t separator = luaString.find(":");
	if (separator != core::String::npos) {
		const core::String header = luaString.substr(0, separator);
		if (core::string::isInteger(header)) {
			const int version = core::string::toInt(header);
			if (version != 5) {
				Log::error("Unsupported Luanti WorldEdit version %i for file %s", version, filename.c_str());
				return false;
			}
			luaChunk = luaString.substr(separator + 1);
		}
	}

	lua::LUA lua;
	lua::StackChecker stackChecker(lua);
	if (!lua.load(luaChunk, 1)) {
		Log::error("Failed to execute Luanti WorldEdit lua data for %s: %s", filename.c_str(), lua.error().c_str());
		return false;
	}
	if (!lua_istable(lua, -1)) {
		Log::error("Expected Luanti WorldEdit data table in %s", filename.c_str());
		lua.pop();
		return false;
	}

	const int rootIndex = lua_gettop(lua);
	const size_t nodeCount = lua_objlen(lua, rootIndex);
	if (nodeCount == 0u) {
		Log::error("No Luanti WorldEdit nodes found in %s", filename.c_str());
		lua.pop();
		return false;
	}

	core::DynamicArray<LuantiNode> nodes;
	nodes.reserve(nodeCount);

	glm::ivec3 mins(0);
	glm::ivec3 maxs(0);
	bool hasBounds = false;
	for (size_t i = 0u; i < nodeCount; ++i) {
		lua_rawgeti(lua, rootIndex, (int)i + 1);
		if (!lua_istable(lua, -1)) {
			Log::error("Invalid Luanti WorldEdit node entry %i in %s", (int)i, filename.c_str());
			lua.pop(2);
			return false;
		}

		LuantiNode entry;
		if (!getIntField(lua, lua_gettop(lua), "x", entry.pos.x) || !getIntField(lua, lua_gettop(lua), "y", entry.pos.y) ||
			!getIntField(lua, lua_gettop(lua), "z", entry.pos.z) || !getStringField(lua, lua_gettop(lua), "name", entry.name)) {
			Log::error("Incomplete Luanti WorldEdit node entry %i in %s", (int)i, filename.c_str());
			lua.pop(2);
			return false;
		}

		if (!hasBounds) {
			mins = maxs = entry.pos;
			hasBounds = true;
		} else {
			mins = glm::min(mins, entry.pos);
			maxs = glm::max(maxs, entry.pos);
		}
		nodes.push_back(core::move(entry));
		lua.pop();
	}
	lua.pop();

	if (!hasBounds) {
		Log::error("No Luanti WorldEdit bounds found in %s", filename.c_str());
		return false;
	}

	const voxel::Region region(mins, maxs);
	voxel::RawVolume *volume = new voxel::RawVolume(region);
	core::StringMap<uint8_t> paletteIndices;
	for (const LuantiNode &entry : nodes) {
		if (isAirNode(entry.name)) {
			continue;
		}
		const uint8_t palIdx = paletteIndex(entry.name, palette, paletteIndices);
		volume->setVoxel(entry.pos.x, entry.pos.y, entry.pos.z, voxel::createVoxel(palette, palIdx));
	}

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume);
	node.setPalette(palette);
	node.setName(core::string::extractFilename(filename));
	return sceneGraph.emplace(core::move(node)) != InvalidNodeId;
}

bool LuantiWorldEditFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
								 const io::ArchivePtr &archive, const SaveContext &ctx) {
	return false;
}

} // namespace voxelformat
