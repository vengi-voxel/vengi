/**
 * @file
 */

#include "DatFormat.h"
#include "MCRFormat.h"
#include "NamedBinaryTag.h"
#include "app/Async.h"
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
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeMerger.h"

#include <SDL3/SDL_stdinc.h>

namespace voxelformat {
namespace priv {

static constexpr int RegionBlockSize = 512;

static int blockToRegionCoord(int block) {
	return block < 0 ? (block - (RegionBlockSize - 1)) / RegionBlockSize : block / RegionBlockSize;
}

static bool parseRegionFileCoords(const core::String &name, int &rx, int &rz) {
	char type = '\0';
	if (SDL_sscanf(name.c_str(), "r.%i.%i.mc%c", &rx, &rz, &type) != 3) {
		return false;
	}
	return type == 'a' || type == 'r';
}

static bool parseLoadRegionAABB(const core::String &value, voxel::Region &out) {
	if (value.empty()) {
		return false;
	}
	int minX = 0;
	int minY = 0;
	int minZ = 0;
	int maxX = 0;
	int maxY = 0;
	int maxZ = 0;
	if (SDL_sscanf(value.c_str(), "%i %i %i %i %i %i", &minX, &minY, &minZ, &maxX, &maxY, &maxZ) != 6) {
		Log::warn("Ignoring invalid voxformat_mcloadregion value: '%s'", value.c_str());
		return false;
	}
	out = voxel::Region(minX, minY, minZ, maxX, maxY, maxZ);
	if (!out.isValid()) {
		Log::warn("Ignoring invalid voxformat_mcloadregion AABB: '%s'", value.c_str());
		return false;
	}
	return true;
}

static bool regionFileIntersectsAABB(int rx, int rz, const voxel::Region &aabb) {
	const int minX = rx * RegionBlockSize;
	const int minZ = rz * RegionBlockSize;
	const int maxX = minX + RegionBlockSize - 1;
	const int maxZ = minZ + RegionBlockSize - 1;
	const voxel::Region fileRegion(minX, aabb.getLowerY(), minZ, maxX, aabb.getUpperY(), maxZ);
	return voxel::intersects(fileRegion, aabb);
}

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
	scenegraph::SceneGraph sceneGraph;
	bool ok = false;
};

static RegionLoadResult loadRegionNode(const core::String &regionFilename, const io::ArchivePtr &archive,
									   const LoadContext &regionCtx) {
	RegionLoadResult result;
	MCRFormat mcrFormat;
	if (!mcrFormat.load(regionFilename, archive, result.sceneGraph, regionCtx)) {
		Log::debug("Could not load %s", regionFilename.c_str());
		return result;
	}

	const bool mergeRegions = core::getVar(cfg::VoxformatMCMergeRegions)->boolVal();
	if (!mergeRegions) {
		result.ok = result.sceneGraph.size(scenegraph::SceneGraphNodeType::AllModels) > 0;
		return result;
	}

	const bool separateWater = core::getVar(cfg::VoxformatMCSeparateWater)->boolVal();
	scenegraph::SceneGraph mergedGraph;

	core::Buffer<const voxel::RawVolume *> terrainVols;
	core::Buffer<const voxel::RawVolume *> waterVols;
	core::Buffer<const voxel::RawVolume *> invisibleVols;
	palette::Palette terrainPalette;
	palette::Palette waterPalette;
	palette::Palette invisiblePalette;
	palette::NormalPalette normalPalette;
	bool hasTerrainPalette = false;
	result.sceneGraph.visitChildren(result.sceneGraph.root().id(), true, [&](const scenegraph::SceneGraphNode &node) {
		if (!node.isAnyModelNode() || node.volume() == nullptr) {
			return;
		}
		if (node.name() == MCRFormat::WaterNodeName) {
			if (separateWater) {
				waterVols.push_back(node.volume());
				if (waterPalette.colorCount() == 0) {
					waterPalette = node.palette();
				}
			} else {
				terrainVols.push_back(node.volume());
				if (!hasTerrainPalette) {
					terrainPalette = node.palette();
					normalPalette = node.normalPalette();
					hasTerrainPalette = true;
				}
			}
			return;
		}
		if (node.name() == MCRFormat::InvisibleNodeName) {
			invisibleVols.push_back(node.volume());
			if (invisiblePalette.colorCount() == 0) {
				invisiblePalette = node.palette();
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
	voxel::RawVolume *water = separateWater ? mergeAndCrop(waterVols) : nullptr;
	voxel::RawVolume *invisible = mergeAndCrop(invisibleVols);
	if (terrain == nullptr && water == nullptr && invisible == nullptr) {
		return RegionLoadResult{};
	}

	const core::String regionName = core::string::extractFilenameWithExtension(regionFilename);
	int terrainParent = mergedGraph.root().id();
	if (terrain != nullptr) {
		scenegraph::SceneGraphNode terrainNode(scenegraph::SceneGraphNodeType::Model);
		terrainNode.setVolume(terrain);
		terrainNode.setPalette(hasTerrainPalette ? terrainPalette
												: (waterPalette.colorCount() > 0 ? waterPalette : invisiblePalette));
		terrainNode.setNormalPalette(normalPalette);
		terrainNode.setName(regionName);
		terrainParent = mergedGraph.emplace(core::move(terrainNode));
		if (terrainParent == InvalidNodeId) {
			terrainParent = mergedGraph.root().id();
		}
	}
	if (water != nullptr) {
		const palette::Palette &srcPal = hasTerrainPalette ? terrainPalette : waterPalette;
		scenegraph::SceneGraphNode waterNode = MCRFormat::createWaterNode(water, srcPal);
		if (terrain == nullptr) {
			waterNode.setName(regionName);
		}
		mergedGraph.emplace(core::move(waterNode), terrainParent);
	}
	if (invisible != nullptr) {
		const palette::Palette &srcPal =
			hasTerrainPalette ? terrainPalette
							  : (invisiblePalette.colorCount() > 0 ? invisiblePalette : waterPalette);
		scenegraph::SceneGraphNode invisibleNode = MCRFormat::createInvisibleNode(invisible, srcPal);
		if (terrain == nullptr && water == nullptr) {
			invisibleNode.setName(regionName);
		}
		mergedGraph.emplace(core::move(invisibleNode), terrainParent);
	}
	result.sceneGraph = core::move(mergedGraph);
	result.ok = true;
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

	const int spawnX = data.get("SpawnX").int32(0);
	const int spawnY = data.get("SpawnY").int32(0);
	const int spawnZ = data.get("SpawnZ").int32(0);
	const int spawnRegionX = blockToRegionCoord(spawnX);
	const int spawnRegionZ = blockToRegionCoord(spawnZ);
	const int loadRadius = core::getVar(cfg::VoxformatMCLoadRadius)->intVal();
	voxel::Region loadAABB = voxel::Region::InvalidRegion;
	const bool hasLoadAABB = parseLoadRegionAABB(core::getVar(cfg::VoxformatMCLoadRegion)->strVal(), loadAABB);
	Log::debug("Spawn %i:%i:%i (region %i:%i), load radius %i, aabb filter %s", spawnX, spawnY, spawnZ, spawnRegionX,
			   spawnRegionZ, loadRadius, hasLoadAABB ? "on" : "off");

	io::ArchiveFiles entities;
	const core::String baseName = core::string::extractDir(filename);
	archive->list(core::string::path(baseName, "region"), entities, "*.mca,*.mcr");
	if (entities.empty()) {
		Log::error("Could not find any region file");
		return false;
	}

	io::ArchiveFiles filtered;
	filtered.reserve(entities.size());
	for (const io::FilesystemEntry &e : entities) {
		if (e.type != io::FilesystemEntry::Type::file) {
			continue;
		}
		int rx = 0;
		int rz = 0;
		if (!parseRegionFileCoords(e.name, rx, rz)) {
			Log::debug("Skip region file with unexpected name: %s", e.name.c_str());
			continue;
		}
		if (loadRadius >= 0) {
			const int dx = rx > spawnRegionX ? rx - spawnRegionX : spawnRegionX - rx;
			const int dz = rz > spawnRegionZ ? rz - spawnRegionZ : spawnRegionZ - rz;
			if (core_max(dx, dz) > loadRadius) {
				continue;
			}
		}
		if (hasLoadAABB && !regionFileIntersectsAABB(rx, rz, loadAABB)) {
			continue;
		}
		filtered.push_back(e);
	}
	if (filtered.empty()) {
		Log::error("No Minecraft region files left after applying load filters (radius=%i)", loadRadius);
		return false;
	}

	const int regionCount = (int)filtered.size();
	core::DynamicArray<RegionLoadResult> nodes;
	nodes.resize(regionCount);
	Log::info("Found %i region files (%i after filter)", (int)entities.size(), regionCount);

	core::AtomicInt regionsDone{0};
	app::for_parallel(0, regionCount, [&nodes, &filtered, &baseName, &archive, &loadctx, &regionsDone,
									   regionCount](int start, int end) {
		for (int i = start; i < end; ++i) {
			const io::FilesystemEntry &e = filtered[i];
			const core::String &regionFilename = core::string::path(baseName, "region", e.name);
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
		if (!loaded.ok) {
			continue;
		}
		nodesAdded += scenegraph::addSceneGraphNodes(sceneGraph, loaded.sceneGraph, rootNode);
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
