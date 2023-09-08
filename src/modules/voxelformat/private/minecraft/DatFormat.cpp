/**
 * @file
 */

#include "DatFormat.h"
#include "app/App.h"
#include "core/Color.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "io/ZipReadStream.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphUtil.h"
#include "voxel/Palette.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeMerger.h"
#include "MCRFormat.h"
#include "MinecraftPaletteMap.h"
#include "NamedBinaryTag.h"

#include <glm/common.hpp>

namespace voxelformat {

bool DatFormat::loadGroupsPalette(const core::String &filename, io::SeekableReadStream &stream,
								  scenegraph::SceneGraph &sceneGraph, voxel::Palette &palette,
								  const LoadContext &loadctx) {
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
		scenegraph::SceneGraphNode groupNode(scenegraph::SceneGraphNodeType::Group);
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
	if (!io::filesystem()->list(core::string::path(baseName, "region"), entities, "*.mca,*.mcr")) {
		Log::error("Failed to search minecraft region files");
		return false;
	}

	if (entities.empty()) {
		Log::error("Could not find any region file");
		return false;
	}

	int nodesAdded = 0;
	core::ThreadPool &threadPool = app::App::getInstance()->threadPool();

	core::DynamicArray<std::future<scenegraph::SceneGraph>> futures;
	Log::info("Found %i region files", (int)entities.size());
	for (const io::FilesystemEntry &e : entities) {
		if (e.type != io::FilesystemEntry::Type::file) {
			continue;
		}
		const core::String &regionFilename = core::string::path(baseName, "region", e.name);
		const io::FilePtr &file = io::filesystem()->open(regionFilename);
		if (!file->validHandle()) {
			Log::warn("Could not open %s", regionFilename.c_str());
			continue;
		}
		futures.emplace_back(threadPool.enqueue([file, &loadctx]() {
			io::FileStream fileStream(file);
			scenegraph::SceneGraph newSceneGraph;
			if (fileStream.size() <= 2l * MCRFormat::SECTOR_BYTES) {
				Log::debug("Skip empty region file %s", file->name().c_str());
				return core::move(newSceneGraph);
			}
			MCRFormat mcrFormat;
			if (!mcrFormat.load(file->name(), fileStream, newSceneGraph, loadctx)) {
				Log::warn("Could not load %s", file->name().c_str());
			}
			const scenegraph::SceneGraph::MergedVolumePalette &merged = newSceneGraph.merge();
			newSceneGraph.clear();
			if (merged.first == nullptr) {
				return core::move(newSceneGraph);
			}
			scenegraph::SceneGraphNode node;
			node.setVolume(merged.first, true);
			node.setPalette(merged.second);
			newSceneGraph.emplace(core::move(node));
			return core::move(newSceneGraph);
		}));
	}
	Log::debug("Scheduled %i regions", (int)futures.size());
	int count = 0;
	for (auto &f : futures) {
		scenegraph::SceneGraph newSceneGraph = core::move(f.get());
		nodesAdded += scenegraph::addSceneGraphNodes(sceneGraph, newSceneGraph, rootNode);
		Log::debug("... loaded %i", count++);
	}

	return nodesAdded > 0;
}

} // namespace voxelformat
