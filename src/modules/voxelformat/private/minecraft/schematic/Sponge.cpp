/**
 * @file
 */

#include "Sponge.h"
#include "../MinecraftPaletteMap.h"
#include "IntReader.h"
#include "Util.h"
#include "core/StringUtil.h"
#include "voxel/RawVolume.h"

namespace voxelformat {
namespace sponge {

static int loadMCEdit2Palette(const priv::NamedBinaryTag &schematic, schematic::SchematicPalette &mcpal) {
	const priv::NamedBinaryTag &blockIds = schematic.get("BlockIDs");
	if (!blockIds.valid()) {
		return -1;
	}
	Log::debug("Found MCEdit2 BlockIDs");
	mcpal.resize(palette::PaletteMaxColors);
	int paletteEntry = 0;
	const int blockCnt = (int)blockIds.compound()->size();
	Log::debug("Loading BlockIDs with %i entries", blockCnt);
	// TODO: FOR_PARALLEL
	for (int i = 0; i < blockCnt; ++i) {
		const priv::NamedBinaryTag &nbt = blockIds.get(core::String::format("%i", i));
		const core::String *value = nbt.string();
		if (value == nullptr) {
			Log::warn("Empty string in BlockIDs for %i", i);
			continue;
		}
		// map to stone on default
		mcpal[i] = findPaletteIndex(*value, 1);
		++paletteEntry;
	}
	return paletteEntry;
}

static int loadWorldEditPalette(const priv::NamedBinaryTag &schematic, schematic::SchematicPalette &mcpal) {
	const int paletteMax = schematic.get("PaletteMax").int32(-1);
	if (paletteMax == -1) {
		return -1;
	}
	Log::debug("Found WorldEdit PaletteMax %i", paletteMax);
	const priv::NamedBinaryTag &palette = schematic.get("Palette");
	if (palette.valid() && palette.type() == priv::TagType::COMPOUND) {
		if ((int)palette.compound()->size() != paletteMax) {
			return -1;
		}
		mcpal.resize(paletteMax);
		int paletteEntry = 0;
		for (const auto &c : *palette.compound()) {
			const core::String &key = c->key;
			const int palIdx = c->second.int32(-1);
			if (palIdx < 0) {
				Log::warn("Failed to get int value for %s", key.c_str());
				continue;
			}
			if (palIdx >= paletteMax) {
				Log::warn("Palette index %i is out of bounds", palIdx);
				continue;
			}
			// map to stone on default
			mcpal[palIdx] = findPaletteIndex(key, 1);
			++paletteEntry;
		}
		return paletteEntry;
	}
	return -1;
}

// https://github.com/Lunatrius/Schematica/
static int loadSchematicaPalette(const priv::NamedBinaryTag &schematic, schematic::SchematicPalette &mcpal) {
	const priv::NamedBinaryTag &schematicaMapping = schematic.get("SchematicaMapping");
	if (!schematicaMapping.valid()) {
		return -1;
	}
	if (schematicaMapping.type() != priv::TagType::COMPOUND) {
		return -1;
	}
	Log::debug("Found SchematicaMapping");
	int paletteEntry = 0;
	for (const auto &c : *schematicaMapping.compound()) {
		core::String key = c->key;
		const int palIdx = c->second.int16(-1);
		if (palIdx < 0) {
			Log::warn("Failed to get int value for %s", key.c_str());
			continue;
		}
		// map to stone on default
		mcpal[palIdx] = findPaletteIndex(key, 1);
		++paletteEntry;
	}
	return paletteEntry;
}

static int parsePalette(const priv::NamedBinaryTag &schematic, schematic::SchematicPalette &mcpal) {
	int paletteEntry = loadMCEdit2Palette(schematic, mcpal);
	if (paletteEntry != -1) {
		return paletteEntry;
	}
	paletteEntry = loadWorldEditPalette(schematic, mcpal);
	if (paletteEntry != -1) {
		return paletteEntry;
	}
	paletteEntry = loadSchematicaPalette(schematic, mcpal);
	if (paletteEntry != -1) {
		return paletteEntry;
	}
	Log::warn("Could not find valid 'BlockIDs' or 'Palette' tag");
	return -1;
}

static void addMetadata_r(const core::String &key, const priv::NamedBinaryTag &nbt, scenegraph::SceneGraph &sceneGraph,
						  scenegraph::SceneGraphNode &node) {
	switch (nbt.type()) {
	case priv::TagType::COMPOUND: {
		scenegraph::SceneGraphNode compoundNode(scenegraph::SceneGraphNodeType::Group);
		compoundNode.setName(key);
		int nodeId = sceneGraph.emplace(core::move(compoundNode), node.id());
		for (const auto &e : *nbt.compound()) {
			addMetadata_r(e->key, e->value, sceneGraph, sceneGraph.node(nodeId));
		}
		break;
	}
	case priv::TagType::END:
	case priv::TagType::BYTE:
		node.setProperty(key, core::string::toString(nbt.int8()));
		break;
	case priv::TagType::SHORT:
		node.setProperty(key, core::string::toString(nbt.int16()));
		break;
	case priv::TagType::INT:
		node.setProperty(key, core::string::toString(nbt.int32()));
		break;
	case priv::TagType::LONG:
		node.setProperty(key, core::string::toString(nbt.int64()));
		break;
	case priv::TagType::FLOAT:
		node.setProperty(key, core::string::toString(nbt.float32()));
		break;
	case priv::TagType::DOUBLE:
		node.setProperty(key, core::string::toString(nbt.float64()));
		break;
	case priv::TagType::STRING:
		node.setProperty(key, nbt.string());
		break;
	case priv::TagType::LIST: {
		const priv::NBTList &list = *nbt.list();
		scenegraph::SceneGraphNode listNode(scenegraph::SceneGraphNodeType::Group);
		listNode.setName(core::String::format("%s: %i", key.c_str(), (int)list.size()));
		int nodeId = sceneGraph.emplace(core::move(listNode), node.id());
		for (const priv::NamedBinaryTag &e : list) {
			addMetadata_r(key, e, sceneGraph, sceneGraph.node(nodeId));
		}
		break;
	}
	case priv::TagType::BYTE_ARRAY:
		node.setProperty(key, "Byte Array");
		break;
	case priv::TagType::INT_ARRAY:
		node.setProperty(key, "Int Array");
		break;
	case priv::TagType::LONG_ARRAY:
		node.setProperty(key, "Long Array");
		break;
	case priv::TagType::MAX:
		break;
	}
}

static void parseMetadata(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph,
						  scenegraph::SceneGraphNode &node) {
	const priv::NamedBinaryTag &metadata = schematic.get("Metadata");
	if (metadata.valid()) {
		if (const core::String *str = metadata.get("Name").string()) {
			node.setName(*str);
		}
		if (const core::String *str = metadata.get("Author").string()) {
			node.setProperty(scenegraph::PropAuthor, *str);
		}
	}
	const int version = schematic.get("Version").int32(-1);
	if (version != -1) {
		node.setProperty(scenegraph::PropVersion, core::string::toString(version));
	}
	core_assert_msg(node.id() != -1, "The node should already be part of the scene graph");
	for (const auto &e : *schematic.compound()) {
		addMetadata_r(e->key, e->value, sceneGraph, node);
	}
}

static bool parseBlockData(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph,
						   palette::Palette &palette, const priv::NamedBinaryTag &blockData) {
	const core::Buffer<int8_t> *blocks = blockData.byteArray();
	if (blocks == nullptr) {
		Log::error("Invalid BlockData - expected byte array");
		return false;
	}
	schematic::SchematicPalette mcpal;
	const int paletteEntry = parsePalette(schematic, mcpal);

	const int16_t width = schematic.get("Width").int16();
	const int16_t height = schematic.get("Height").int16();
	const int16_t depth = schematic.get("Length").int16();

	if (width == 0 || depth == 0) {
		Log::error("Invalid width or length found");
		return false;
	}

	schematic::IntReader reader(blocks);

	const voxel::Region region(0, 0, 0, width - 1, height - 1, depth - 1);
	voxel::RawVolume *volume = new voxel::RawVolume(region);
	// TODO: PERF: FOR_PARALLEL: maybe load all pal indices first?
	voxel::RawVolume::Sampler sampler(volume);
	sampler.setPosition(0, 0, 0);
	for (int y = 0; y < height; y++) {
		voxel::RawVolume::Sampler sampler2 = sampler;
		for (int z = 0; z < depth; z++) {
			voxel::RawVolume::Sampler sampler3 = sampler2;
			for (int x = 0; x < width; x++) {
				int32_t palIdx = 0;
				if (reader.readInt32(palIdx) == -1) {
					break;
				}
				if (palIdx != 0) {
					uint8_t currentPalIdx = (paletteEntry == 0) ? palIdx : mcpal[palIdx];
					if (currentPalIdx != 0) {
						sampler3.setVoxel(voxel::createVoxel(palette, currentPalIdx));
					}
				}
				sampler3.movePositiveX();
			}
			sampler2.movePositiveZ();
			if (reader.eos()) {
				break;
			}
		}
		sampler.movePositiveY();
	}

	const int32_t x = schematic.get("x").int32();
	const int32_t y = schematic.get("y").int32();
	const int32_t z = schematic.get("z").int32();
	volume->translate(glm::ivec3(x, y, z));

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume, true);
	node.setPalette(palette);
	const int nodeId = sceneGraph.emplace(core::move(node));
	if (nodeId == InvalidNodeId) {
		return false;
	}
	parseMetadata(schematic, sceneGraph, sceneGraph.node(nodeId));
	return true;
}

static bool parseBlocks(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph,
						palette::Palette &palette, const priv::NamedBinaryTag &blocks, int version) {
	schematic::SchematicPalette mcpal;
	const int paletteEntry = parsePalette(schematic, mcpal);

	const int16_t width = schematic.get("Width").int16();
	const int16_t height = schematic.get("Height").int16();
	const int16_t depth = schematic.get("Length").int16();

	// TODO: VOXELFORMAT: Support for WorldEdit's AddBlocks is missing
	// * https://github.com/EngineHub/WorldEdit/blob/master/worldedit-core/src/main/java/com/sk89q/worldedit/extent/clipboard/io/MCEditSchematicReader.java#L171
	// * https://github.com/mcedit/mcedit2/blob/master/src/mceditlib/schematic.py#L143
	// * https://github.com/Lunatrius/Schematica/blob/master/src/main/java/com/github/lunatrius/schematica/world/schematic/SchematicAlpha.java

	const voxel::Region region(0, 0, 0, width - 1, height - 1, depth - 1);
	voxel::RawVolume *volume = new voxel::RawVolume(region);
	auto fn = [volume, depth, height, width, blocks, paletteEntry, &mcpal, &palette](int start, int end) {
		voxel::RawVolume::Sampler sampler(volume);
		sampler.setPosition(0, 0, start);
		for (int z = start; z < end; ++z) {
			voxel::RawVolume::Sampler sampler2 = sampler;
			for (int y = 0; y < height; ++y) {
				voxel::RawVolume::Sampler sampler3 = sampler2;
				const int stride = (y * depth + z) * width;
				for (int x = 0; x < width; ++x) {
					const int idx = stride + x;
					const uint8_t palIdx = (*blocks.byteArray())[idx];
					if (palIdx != 0u) {
						uint8_t currentPalIdx;
						if (paletteEntry == 0 || palIdx > paletteEntry) {
							currentPalIdx = palIdx;
						} else {
							currentPalIdx = mcpal[palIdx];
						}
						sampler3.setVoxel(voxel::createVoxel(palette, currentPalIdx));
					}
					sampler3.movePositiveX();
				}
				sampler2.movePositiveY();
			}
			sampler.movePositiveZ();
		}
	};
	app::for_parallel(0, depth, fn);

	const int32_t x = schematic.get("x").int32();
	const int32_t y = schematic.get("y").int32();
	const int32_t z = schematic.get("z").int32();
	volume->translate(glm::ivec3(x, y, z));

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume, true);
	node.setPalette(palette);
	const int nodeId = sceneGraph.emplace(core::move(node));
	if (nodeId == InvalidNodeId) {
		return false;
	}
	parseMetadata(schematic, sceneGraph, sceneGraph.node(nodeId));
	return true;
}

bool loadGroupsPaletteSponge1And2(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph,
								  palette::Palette &palette) {
	Log::debug("WorldEdit legacy");
	const priv::NamedBinaryTag &blockData = schematic.get("BlockData");
	if (blockData.valid() && blockData.type() == priv::TagType::BYTE_ARRAY) {
		return parseBlockData(schematic, sceneGraph, palette, blockData);
	}
	Log::error("Could not find valid 'BlockData' tags");
	return false;
}

bool loadGroupsPaletteSponge3(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph,
							  palette::Palette &palette, int version) {
	Log::debug("Sponge 3");
	const priv::NamedBinaryTag &blocks = schematic.get("Blocks");
	if (blocks.valid() && blocks.type() == priv::TagType::BYTE_ARRAY) {
		return parseBlocks(schematic, sceneGraph, palette, blocks, version);
	}
	Log::error("Could not find valid 'Blocks' tags");
	return false;
}

} // namespace sponge
} // namespace voxelformat
