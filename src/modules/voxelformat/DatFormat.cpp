/**
 * @file
 */

#include "DatFormat.h"
#include "app/App.h"
#include "core/Color.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/StringMap.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "io/MemoryReadStream.h"
#include "io/ZipReadStream.h"
#include "io/ZipWriteStream.h"
#include "private/MinecraftPaletteMap.h"
#include "private/NamedBinaryTag.h"
#include "voxel/MaterialColor.h"
#include "voxel/Palette.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelformat/MCRFormat.h"
#include "voxelformat/SceneGraphNode.h"
#include "voxelformat/SceneGraphUtil.h"
#include "voxelformat/private/PaletteLookup.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeMerger.h"

#include <glm/common.hpp>

namespace voxelformat {

bool DatFormat::loadGroupsPalette(const core::String &filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph,
								  voxel::Palette &palette) {
	palette.minecraft();
	io::ZipReadStream zipStream(stream);
	priv::NamedBinaryTagContext ctx;
	ctx.stream = &zipStream;
	const priv::NamedBinaryTag &root = priv::NamedBinaryTag::parse(ctx);
	if (!root.valid()) {
		Log::error("Could not find 'root' tag");
		return false;
	}

	const priv::NamedBinaryTag &data = root.get("Data");
	if (!data.valid()) {
		Log::error("Could not find 'Data' tag");
		return false;
	}
	if (data.type() != priv::TagType::COMPOUND) {
		Log::error("Tag 'Data' is no compound (%i)", (int)data.type());
		return false;
	}

	const priv::NamedBinaryTag &levelName = data.get("LevelName");
	int rootNode = sceneGraph.root().id();
	if (levelName.valid() && levelName.type() == priv::TagType::STRING) {
		const core::String &name = *levelName.string();
		voxelformat::SceneGraphNode groupNode(voxelformat::SceneGraphNodeType::Group);
		groupNode.setName(name);
		rootNode = sceneGraph.emplace(core::move(groupNode));
		Log::debug("Level name: %s", name.c_str());
	}
	const priv::NamedBinaryTag &levelVersion = data.get("version");
	if (levelVersion.valid() && levelVersion.type() == priv::TagType::INT) {
		const int version = levelVersion.int32();
		Log::debug("Level nbt version: %i", version);
	}
	const priv::NamedBinaryTag &dataVersion = data.get("Version");
	if (dataVersion.valid() && dataVersion.type() == priv::TagType::COMPOUND) {
		const int version = dataVersion.get("Id").int32();
		const core::String *versionName = dataVersion.get("Name").string();
		const core::String *versionSeries = dataVersion.get("Series").string();
		Log::debug("Minecraft version: (data: %i, name: %s, series: %s)", version,
				   versionName ? versionName->c_str() : "-", versionSeries ? versionSeries->c_str() : "-");
	}
	core::DynamicArray<io::FilesystemEntry> entities;
	const core::String baseName = core::string::extractPath(filename);
	if (!io::filesystem()->list(core::string::path(baseName, "region"), entities, "*.mca")) {
		Log::error("Failed to search minecraft region files");
		return false;
	}

	if (entities.empty()) {
		Log::error("Could not find any region file");
		return false;
	}

	Log::info("Found %i region files", (int)entities.size());
	for (const io::FilesystemEntry &e : entities) {
		if (e.type != io::FilesystemEntry::Type::file) {
			continue;
		}
		const core::String &filename = core::string::path(baseName, "region", e.name);
		const io::FilePtr &file = io::filesystem()->open(filename);
		if (!file->validHandle()) {
			Log::warn("Could not open %s", filename.c_str());
			continue;
		}
		io::FileStream stream(file);
		MCRFormat mcrFormat;
		SceneGraph newSceneGraph;
		if (!mcrFormat.loadGroups(filename, stream, newSceneGraph)) {
			Log::warn("Could not load %s", filename.c_str());
			continue;
		}
		voxelformat::addSceneGraphNodes(sceneGraph, newSceneGraph, rootNode);
	}

	return true;
}

} // namespace voxelformat
