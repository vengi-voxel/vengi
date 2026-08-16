/**
 * @file
 */

#include "DatFormat.h"
#include "MCRFormat.h"
#include "NamedBinaryTag.h"
#include "app/Async.h"
#include "color/Color.h"
#include "core/Common.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicArray.h"
#include "core/concurrent/Atomic.h"
#include "io/Archive.h"
#include "io/ZipReadStream.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphUtil.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeMerger.h"

#include <glm/common.hpp>

namespace voxelformat {
namespace priv {

static voxel::RawVolume *mergeAndCrop(const core::Buffer<const voxel::RawVolume *> &volumes) {
	if (volumes.empty()) {
		return nullptr;
	}
	voxel::RawVolume *merged = voxelutil::merge(volumes);
	if (merged == nullptr) {
		return nullptr;
	}
	if (voxel::RawVolume *cropped = voxelutil::cropVolume(merged)) {
		delete merged;
		return cropped;
	}
	if (merged->isEmpty(merged->region())) {
		delete merged;
		return nullptr;
	}
	return merged;
}

struct RegionLoadResult {
	scenegraph::SceneGraphNode *terrain = nullptr;
	scenegraph::SceneGraphNode *water = nullptr;
};

static RegionLoadResult loadRegionNode(const core::String &regionFilename, const io::ArchivePtr &archive,
									   const LoadContext &regionCtx) {
	RegionLoadResult result;
	MCRFormat mcrFormat;
	scenegraph::SceneGraph newSceneGraph;
	if (!mcrFormat.load(regionFilename, archive, newSceneGraph, regionCtx)) {
		Log::debug("Could not load %s", regionFilename.c_str());
		return result;
	}
	const bool separateWater = core::getVar(cfg::VoxformatMCSeparateWater)->boolVal();
	if (!separateWater) {
		const scenegraph::SceneGraph::MergeResult &merged = newSceneGraph.merge();
		if (!merged.hasVolume()) {
			return result;
		}
		result.terrain = new scenegraph::SceneGraphNode(scenegraph::SceneGraphNodeType::Model);
		result.terrain->setVolume(merged.volume());
		result.terrain->setPalette(merged.palette);
		result.terrain->setNormalPalette(merged.normalPalette);
		result.terrain->setName(core::string::extractFilenameWithExtension(regionFilename));
		return result;
	}

	core::Buffer<const voxel::RawVolume *> terrainVols;
	core::Buffer<const voxel::RawVolume *> waterVols;
	palette::Palette terrainPalette;
	palette::Palette waterPalette;
	palette::NormalPalette normalPalette;
	bool hasTerrainPalette = false;
	newSceneGraph.visitChildren(newSceneGraph.root().id(), true, [&](const scenegraph::SceneGraphNode &node) {
		if (!node.isAnyModelNode() || node.volume() == nullptr) {
			return;
		}
		if (node.name() == MCRFormat::WaterNodeName) {
			waterVols.push_back(node.volume());
			if (waterPalette.colorCount() == 0) {
				waterPalette = node.palette();
			}
			return;
		}
		terrainVols.push_back(node.volume());
		if (!hasTerrainPalette) {
			terrainPalette = node.palette();
			normalPalette = node.normalPalette();
			hasTerrainPalette = true;
		}
	});

	voxel::RawVolume *terrain = mergeAndCrop(terrainVols);
	voxel::RawVolume *water = mergeAndCrop(waterVols);
	if (terrain == nullptr && water == nullptr) {
		return result;
	}

	const core::String regionName = core::string::extractFilenameWithExtension(regionFilename);
	if (terrain != nullptr) {
		result.terrain = new scenegraph::SceneGraphNode(scenegraph::SceneGraphNodeType::Model);
		result.terrain->setVolume(terrain);
		result.terrain->setPalette(hasTerrainPalette ? terrainPalette : waterPalette);
		result.terrain->setNormalPalette(normalPalette);
		result.terrain->setName(regionName);
	}
	if (water != nullptr) {
		const palette::Palette &srcPal = hasTerrainPalette ? terrainPalette : waterPalette;
		result.water = new scenegraph::SceneGraphNode(MCRFormat::createWaterNode(water, srcPal));
		if (result.terrain == nullptr) {
			result.water->setName(regionName);
		}
	}
	return result;
}

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

	const int regionCount = (int)entities.size();
	core::DynamicArray<RegionLoadResult> nodes;
	nodes.resize(regionCount);
	Log::info("Found %i region files", regionCount);

	core::AtomicInt regionsDone{0};
	app::for_parallel(0, regionCount, [&nodes, &entities, &baseName, &archive, &loadctx, &regionsDone,
									   regionCount](int start, int end) {
		for (int i = start; i < end; ++i) {
			const io::FilesystemEntry &e = entities[i];
			if (e.type != io::FilesystemEntry::Type::file) {
				regionsDone.increment();
				continue;
			}
			const core::String &regionFilename = core::string::path(baseName, "region", e.name);
			// Nested MCR progress would race across parallel regions; report by completed
			// region count on the shared parent sink instead.
			LoadContext regionCtx;
			nodes[i] = loadRegionNode(regionFilename, archive, regionCtx);
			const int completed = regionsDone.increment() + 1;
			loadctx.report(e.name.c_str(), completed, regionCount);
		}
	});
	loadctx.report("regions", regionCount, regionCount);
	Log::debug("Scheduled %i regions", (int)nodes.size());
	int nodesAdded = 0;
	for (RegionLoadResult &loaded : nodes) {
		if (loaded.terrain == nullptr && loaded.water == nullptr) {
			continue;
		}
		int parentId = rootNode;
		if (loaded.terrain != nullptr) {
			parentId = sceneGraph.emplace(core::move(*loaded.terrain), rootNode);
			delete loaded.terrain;
			loaded.terrain = nullptr;
			if (parentId == InvalidNodeId) {
				parentId = rootNode;
			} else {
				++nodesAdded;
			}
		}
		if (loaded.water != nullptr) {
			const int waterId = sceneGraph.emplace(core::move(*loaded.water), parentId);
			delete loaded.water;
			loaded.water = nullptr;
			if (waterId != InvalidNodeId) {
				++nodesAdded;
			}
		}
		Log::debug("... loaded %i", nodesAdded);
	}

	return nodesAdded > 0;
}

} // namespace priv

bool DatFormat::loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
								  scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
								  const LoadContext &loadctx) {
	loadctx.setProgressText("Minecraft DAT");
	loadctx.setProgress(0.0f);
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
	const bool loaded = priv::load(filename, ctx, sceneGraph, archive, loadctx);
	if (loaded) {
		loadctx.setProgress(1.0f);
	}
	return loaded;
}

} // namespace voxelformat
