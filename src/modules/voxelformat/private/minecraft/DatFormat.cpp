/**
 * @file
 */

#include "DatFormat.h"
#include "app/Async.h"
#include "core/Color.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "io/Archive.h"
#include "io/ZipReadStream.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphUtil.h"
#include "palette/Palette.h"
#include "voxel/Voxel.h"
#include "MCRFormat.h"
#include "NamedBinaryTag.h"

#include <glm/common.hpp>

namespace voxelformat {

bool DatFormat::loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
								  scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
								  const LoadContext &loadctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	palette.minecraft();
	priv::NamedBinaryTagContext ctx;
	io::ZipReadStream zipStream(*stream);
	if (zipStream.err()) {
		Log::debug("Loading from uncompressed stream (bedrock)");
		// bedrock is uncompressed and little endian
		ctx.stream = stream;
		ctx.bedrock = true;
		uint32_t fileType;
		stream->readUInt32(fileType);
		Log::debug("File type: %u", fileType);
		uint32_t fileLengthWithoutHeader;
		stream->readUInt32(fileLengthWithoutHeader);
		Log::debug("File length without header: %u", fileLengthWithoutHeader);
	} else {
		Log::debug("Loading from zip stream");
		ctx.stream = &zipStream;
		ctx.bedrock = false;
	}
	priv::NamedBinaryTag root = priv::NamedBinaryTag::parse(ctx);
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
	io::ArchiveFiles entities;
	const core::String baseName = core::string::extractDir(filename);
	archive->list(core::string::path(baseName, "region"), entities, "*.mca,*.mcr");
	if (entities.empty()) {
		Log::error("Could not find any region file");
		return false;
	}

	int nodesAdded = 0;

	core::DynamicArray<std::future<scenegraph::SceneGraph>> futures;
	Log::info("Found %i region files", (int)entities.size());
	for (const io::FilesystemEntry &e : entities) {
		if (e.type != io::FilesystemEntry::Type::file) {
			continue;
		}
		const core::String &regionFilename = core::string::path(baseName, "region", e.name);
		futures.emplace_back(app::async([regionFilename, &archive, &loadctx]() {
			MCRFormat mcrFormat;
			scenegraph::SceneGraph newSceneGraph;
			if (!mcrFormat.load(regionFilename, archive, newSceneGraph, loadctx)) {
				Log::debug("Could not load %s", regionFilename.c_str());
				return core::move(newSceneGraph);
			}
			const scenegraph::SceneGraph::MergeResult &merged = newSceneGraph.merge();
			newSceneGraph.clear();
			if (!merged.hasVolume()) {
				return core::move(newSceneGraph);
			}
			scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
			node.setVolume(merged.volume(), true);
			node.setPalette(merged.palette);
			node.setNormalPalette(merged.normalPalette);
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
