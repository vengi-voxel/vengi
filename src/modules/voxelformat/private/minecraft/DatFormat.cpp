/**
 * @file
 */

#include "DatFormat.h"
#include "MCRFormat.h"
#include "NamedBinaryTag.h"
#include "app/Async.h"
#include "color/Color.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "io/Archive.h"
#include "io/ZipReadStream.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphUtil.h"
#include "voxel/Voxel.h"

#include <glm/common.hpp>

namespace voxelformat {
namespace priv {

static bool load(const core::String &filename, priv::NamedBinaryTagContext &ctx, scenegraph::SceneGraph &sceneGraph,
				 const io::ArchivePtr &archive, const LoadContext &loadctx) {
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

	core::DynamicArray<scenegraph::SceneGraphNode*> nodes;
	nodes.resize(entities.size());
	Log::info("Found %i region files", (int)entities.size());
	app::for_parallel(0, entities.size(), [&nodes, &entities, &baseName, &archive, &loadctx](int start, int end) {
		for (int i = start; i < end; ++i) {
			const io::FilesystemEntry &e = entities[i];
			if (e.type != io::FilesystemEntry::Type::file) {
				continue;
			}
			const core::String &regionFilename = core::string::path(baseName, "region", e.name);
			MCRFormat mcrFormat;
			scenegraph::SceneGraph newSceneGraph;
			if (!mcrFormat.load(regionFilename, archive, newSceneGraph, loadctx)) {
				Log::debug("Could not load %s", regionFilename.c_str());
				continue;
			}
			const scenegraph::SceneGraph::MergeResult &merged = newSceneGraph.merge();
			if (!merged.hasVolume()) {
				continue;
			}
			scenegraph::SceneGraphNode *node = new scenegraph::SceneGraphNode(scenegraph::SceneGraphNodeType::Model);
			node->setVolume(merged.volume(), true);
			node->setPalette(merged.palette);
			node->setNormalPalette(merged.normalPalette);
			nodes[i] = node;
		}
	});
	Log::debug("Scheduled %i regions", (int)nodes.size());
	int nodesAdded = 0;
	for (scenegraph::SceneGraphNode *node : nodes) {
		if (node == nullptr) {
			continue;
		}
		sceneGraph.emplace(core::move(*node), rootNode);
		delete node;
		Log::debug("... loaded %i", nodesAdded++);
	}

	return nodesAdded > 0;
}

} // namespace priv

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
#if 0
	const bool bedrock = !io::ZipReadStream::isZipStream(*stream);
	if (bedrock) {
		// bedrock is uncompressed and little endian
		Log::debug("Loading from uncompressed stream (bedrock)");
		ctx.stream = stream;
		ctx.bedrock = true;
		uint32_t fileType;
		if (stream->readUInt32(fileType) == -1) {
			Log::error("Failed to read file type");
			return false;
		}
		Log::debug("File type: %u", fileType);
		uint32_t fileLengthWithoutHeader;
		if (stream->readUInt32(fileLengthWithoutHeader) == -1) {
			Log::error("Failed to read file length without header");
			return false;
		}
		Log::debug("File length without header: %u", fileLengthWithoutHeader);
		return priv::load(filename, ctx, sceneGraph, archive, loadctx);
	}
#endif
	Log::debug("Loading from zip stream");
	io::ZipReadStream zipStream(*stream);
	ctx.stream = &zipStream;
	ctx.bedrock = false;
	return priv::load(filename, ctx, sceneGraph, archive, loadctx);
}

} // namespace voxelformat
