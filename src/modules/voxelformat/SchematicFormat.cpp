/**
 * @file
 */

#include "SchematicFormat.h"
#include "core/Color.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/StringMap.h"
#include "io/BufferedReadWriteStream.h"
#include "io/File.h"
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
#include "voxelformat/SceneGraphNode.h"
#include "voxel/PaletteLookup.h"
#include "voxelformat/private/SchematicIntReader.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeMerger.h"

#include <glm/common.hpp>

namespace voxelformat {

bool SchematicFormat::loadGroupsPalette(const core::String &filename, io::SeekableReadStream &stream,
										SceneGraph &sceneGraph, voxel::Palette &palette) {
	palette.minecraft();
	io::ZipReadStream zipStream(stream);
	priv::NamedBinaryTagContext ctx;
	ctx.stream = &zipStream;
	const priv::NamedBinaryTag &schematic = priv::NamedBinaryTag::parse(ctx);
	if (!schematic.valid()) {
		Log::error("Could not find 'Schematic' tag");
		return false;
	}

	const core::String &extension = core::string::extractExtension(filename);
	if (extension == "nbt") {
		const int dataVersion = schematic.get("DataVersion").int32(-1);
		if (loadNbt(schematic, sceneGraph, palette, dataVersion)) {
			return true;
		}
	}

	const int version = schematic.get("Version").int32(-1);
	Log::debug("Load schematic version %i", version);
	switch (version) {
	case 1:
	case 2:
		// WorldEdit legacy
		if (loadSponge1And2(schematic, sceneGraph, palette)) {
			return true;
		}
		// fall through
	case 3:
	default:
		if (loadSponge3(schematic, sceneGraph, palette, version)) {
			return true;
		}
	}
	schematic.print();
	return false;
}

bool SchematicFormat::loadSponge1And2(const priv::NamedBinaryTag &schematic, SceneGraph &sceneGraph,
									  voxel::Palette &palette) {
	const priv::NamedBinaryTag &blockData = schematic.get("BlockData");
	if (blockData.valid() && blockData.type() == priv::TagType::BYTE_ARRAY) {
		return parseBlockData(schematic, sceneGraph, palette, blockData);
	}
	Log::error("Could not find valid 'BlockData' tags");
	return false;
}

bool SchematicFormat::loadSponge3(const priv::NamedBinaryTag &schematic, SceneGraph &sceneGraph,
								  voxel::Palette &palette, int version) {
	const priv::NamedBinaryTag &blocks = schematic.get("Blocks");
	if (blocks.valid() && blocks.type() == priv::TagType::BYTE_ARRAY) {
		return parseBlocks(schematic, sceneGraph, palette, blocks, version);
	}
	Log::error("Could not find valid 'Blocks' tags");
	return false;
}

bool SchematicFormat::loadNbt(const priv::NamedBinaryTag &schematic, SceneGraph &sceneGraph, voxel::Palette &palette, int dataVersion) {
	const priv::NamedBinaryTag &blocks = schematic.get("blocks");
	if (blocks.valid() && blocks.type() == priv::TagType::LIST) {
		const priv::NBTList &list = *blocks.list();
		glm::ivec3 mins((std::numeric_limits<int32_t>::max)() / 2);
		glm::ivec3 maxs((std::numeric_limits<int32_t>::min)() / 2);
		for (const priv::NamedBinaryTag &compound : list) {
			if (compound.type() != priv::TagType::COMPOUND) {
				Log::error("Unexpected nbt type: %i", (int)compound.type());
				return false;
			}
			const priv::NamedBinaryTag& pos = compound.get("pos");
			if (pos.type() != priv::TagType::LIST) {
				Log::error("Unexpected nbt type for pos: %i", (int)pos.type());
				return false;
			}
			const priv::NBTList &positions = *pos.list();
			if (positions.size() != 3) {
				Log::error("Unexpected nbt pos list entry count: %i", (int)positions.size());
				return false;
			}
			const int state = compound.get("state").int32(-1);
			if (state == -1) {
				Log::error("Unexpected state");
				return false;
			}
			const int x = positions[0].int32(-1);
			const int y = positions[1].int32(-1);
			const int z = positions[2].int32(-1);
			const glm::ivec3 v(x, y, z);
			mins = (glm::min)(mins, v);
			maxs = (glm::max)(maxs, v);
		}
		const voxel::Region region(mins, maxs);
		voxel::RawVolume *volume = new voxel::RawVolume(region);
		for (const priv::NamedBinaryTag &compound : list) {
			const int state = compound.get("state").int32();
			const priv::NBTList &positions = *compound.get("pos").list();
			const int x = positions[0].int32(-1);
			const int y = positions[1].int32(-1);
			const int z = positions[2].int32(-1);
			const glm::ivec3 v(x, y, z);
			volume->setVoxel(v, voxel::createVoxel(voxel::VoxelType::Generic, state));
		}
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(volume, true);
		voxel::Palette palette;
		palette.minecraft();
		node.setPalette(palette);
		int nodeId = sceneGraph.emplace(core::move(node));
		return nodeId != -1;
	}
	Log::error("Could not find valid 'blocks' tags");
	return false;
}

static glm::ivec3 voxelPosFromIndex(int width, int depth, int idx) {
	const int planeSize = width * depth;
	core_assert(planeSize != 0);
	const int y = idx / planeSize;
	const int offset = idx - (y * planeSize);
	const int z = offset / width;
	const int x = offset - z * width;
	return glm::ivec3(x, y, z);
}

bool SchematicFormat::parseBlockData(const priv::NamedBinaryTag &schematic, SceneGraph &sceneGraph,
									 voxel::Palette &palette, const priv::NamedBinaryTag &blockData) {
	const core::DynamicArray<int8_t> *blocks = blockData.byteArray();
	if (blocks == nullptr) {
		Log::error("Invalid BlockData - expected byte array");
		return false;
	}
	core::Buffer<int> mcpal;
	const int paletteEntry = parsePalette(schematic, mcpal);

	const int16_t width = schematic.get("Width").int16();
	const int16_t height = schematic.get("Height").int16();
	const int16_t depth = schematic.get("Length").int16();

	if (width == 0 || depth == 0) {
		Log::error("Invalid width or length found");
		return false;
	}

	voxel::PaletteLookup palLookup(palette);
	voxel::RawVolume *volume = new voxel::RawVolume(voxel::Region(0, 0, 0, width - 1, height - 1, depth - 1));
	SchematicIntReader reader(blocks);
	int index = 0;
	int32_t palIdx = 0;
	while (reader.readInt32(palIdx) != -1) {
		if (palIdx != 0) {
			uint8_t currentPalIdx;
			if (paletteEntry == 0) {
				currentPalIdx = palIdx;
			} else {
				currentPalIdx = mcpal[palIdx];
			}
			if (currentPalIdx != 0) {
				const glm::ivec3 &pos = voxelPosFromIndex(width, depth, index);
				volume->setVoxel(pos, voxel::createVoxel(voxel::VoxelType::Generic, currentPalIdx));
			}
		}
		++index;
	}

	const int32_t x = schematic.get("x").int32();
	const int32_t y = schematic.get("y").int32();
	const int32_t z = schematic.get("z").int32();
	volume->translate(glm::ivec3(x, y, z));

	SceneGraphNode node(SceneGraphNodeType::Model);
	node.setVolume(volume, true);
	node.setPalette(palLookup.palette());
	int nodeId = sceneGraph.emplace(core::move(node));
	if (nodeId == -1) {
		return false;
	}
	parseMetadata(schematic, sceneGraph, sceneGraph.node(nodeId));
	return true;
}

bool SchematicFormat::parseBlocks(const priv::NamedBinaryTag &schematic, SceneGraph &sceneGraph,
								  voxel::Palette &palette, const priv::NamedBinaryTag &blocks, int version) {
	core::Buffer<int> mcpal;
	const int paletteEntry = parsePalette(schematic, mcpal);

	const int16_t width = schematic.get("Width").int16();
	const int16_t height = schematic.get("Height").int16();
	const int16_t depth = schematic.get("Length").int16();

	// TODO: Support for WorldEdit's AddBlocks is missing
	// * https://github.com/EngineHub/WorldEdit/blob/master/worldedit-core/src/main/java/com/sk89q/worldedit/extent/clipboard/io/MCEditSchematicReader.java#L171
	// * https://github.com/mcedit/mcedit2/blob/master/src/mceditlib/schematic.py#L143
	// * https://github.com/Lunatrius/Schematica/blob/master/src/main/java/com/github/lunatrius/schematica/world/schematic/SchematicAlpha.java

	voxel::PaletteLookup palLookup(palette);
	voxel::RawVolume *volume = new voxel::RawVolume(voxel::Region(0, 0, 0, width - 1, height - 1, depth - 1));
	for (int x = 0; x < width; ++x) {
		for (int y = 0; y < height; ++y) {
			for (int z = 0; z < depth; ++z) {
				const int idx = (y * depth + z) * width + x;
				const uint8_t palIdx = (*blocks.byteArray())[idx];
				if (palIdx != 0u) {
					uint8_t currentPalIdx;
					if (paletteEntry == 0 || palIdx > paletteEntry) {
						currentPalIdx = palIdx;
					} else {
						currentPalIdx = mcpal[palIdx];
					}
					volume->setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, currentPalIdx));
				}
			}
		}
	}

	const int32_t x = schematic.get("x").int32();
	const int32_t y = schematic.get("y").int32();
	const int32_t z = schematic.get("z").int32();
	volume->translate(glm::ivec3(x, y, z));

	SceneGraphNode node(SceneGraphNodeType::Model);
	node.setVolume(volume, true);
	node.setPalette(palLookup.palette());
	int nodeId = sceneGraph.emplace(core::move(node));
	if (nodeId == -1) {
		return false;
	}
	parseMetadata(schematic, sceneGraph, sceneGraph.node(nodeId));
	return true;
}

int SchematicFormat::parsePalette(const priv::NamedBinaryTag &schematic, core::Buffer<int> &mcpal) const {
	const priv::NamedBinaryTag &blockIds = schematic.get("BlockIDs"); // MCEdit2
	if (blockIds.valid()) {
		mcpal.resize(voxel::PaletteMaxColors);
		int paletteEntry = 0;
		const int blockCnt = (int)blockIds.compound()->size();
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
	const int paletteMax = schematic.get("PaletteMax").int32(-1); // WorldEdit
	if (paletteMax != -1) {
		const priv::NamedBinaryTag &palette = schematic.get("Palette");
		if (palette.valid() && palette.type() == priv::TagType::COMPOUND) {
			if ((int)palette.compound()->size() != paletteMax) {
				return -1;
			}
			mcpal.resize(paletteMax);
			int paletteEntry = 0;
			for (const auto &c : *palette.compound()) {
				core::String key = c->key;
				const int palIdx = c->second.int32(-1);
				if (palIdx == -1) {
					Log::warn("Failed to get int value for %s", key.c_str());
					continue;
				}
				// map to stone on default
				mcpal[palIdx] = findPaletteIndex(key, 1);
				++paletteEntry;
			}
			return paletteEntry;
		}
	}
	return -1;
}

void SchematicFormat::parseMetadata(const priv::NamedBinaryTag &schematic, SceneGraph &sceneGraph,
									voxelformat::SceneGraphNode &node) {
	const priv::NamedBinaryTag &metadata = schematic.get("Metadata");
	if (metadata.valid()) {
		if (const core::String *str = metadata.get("Name").string()) {
			node.setName(*str);
		}
		if (const core::String *str = metadata.get("Author").string()) {
			node.setProperty("Author", *str);
		}
	}
	const int version = schematic.get("Version").int32(-1);
	if (version != -1) {
		node.setProperty("Version", core::string::toString(version));
	}
	core_assert_msg(node.id() != -1, "The node should already be part of the scene graph");
	for (const auto &e : *schematic.compound()) {
		addMetadata_r(e->key, e->value, sceneGraph, node);
	}
}

void SchematicFormat::addMetadata_r(const core::String &key, const priv::NamedBinaryTag &nbt, SceneGraph &sceneGraph,
									voxelformat::SceneGraphNode &node) {
	switch (nbt.type()) {
	case priv::TagType::COMPOUND: {
		SceneGraphNode compoundNode(SceneGraphNodeType::Group);
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
		SceneGraphNode listNode(SceneGraphNodeType::Group);
		listNode.setName(core::string::format("%s: %i", key.c_str(), (int)list.size()));
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

bool SchematicFormat::saveGroups(const SceneGraph &sceneGraph, const core::String &filename,
								 io::SeekableWriteStream &stream) {
	// save as sponge-3
	const SceneGraph::MergedVolumePalette &merged = sceneGraph.merge();
	if (merged.first == nullptr) {
		Log::error("Failed to merge volumes");
		return false;
	}
	core::ScopedPtr<voxel::RawVolume> scopedPtr(merged.first);
	const voxel::Region &region = merged.first->region();
	const glm::ivec3 &size = region.getDimensionsInVoxels();
	const glm::ivec3 &mins = region.getLowerCorner();

	io::ZipWriteStream zipStream(stream);

	priv::NBTCompound compound;
	compound.put("Width", (int16_t)size.x);
	compound.put("Height", (int16_t)size.y);
	compound.put("Length", (int16_t)size.z);
	compound.put("x", (int32_t)mins.x);
	compound.put("y", (int32_t)mins.y);
	compound.put("z", (int32_t)mins.z);
	compound.put("Materials", priv::NamedBinaryTag("Alpha"));
	compound.put("Version", 3);
	// TODO: palette
	{
		core::DynamicArray<int8_t> blocks;
		blocks.resize((size_t)size.x * (size_t)size.y * (size_t)size.z);

		for (int x = 0; x < size.x; ++x) {
			for (int y = 0; y < size.y; ++y) {
				for (int z = 0; z < size.z; ++z) {
					const int idx = (y * size.z + z) * size.x + x;
					const voxel::Voxel &voxel = merged.first->voxel(mins.x + x, mins.y + y, mins.z + z);
					if (voxel::isAir(voxel.getMaterial())) {
						blocks[idx] = 0;
					} else {
						const uint8_t currentPalIdx = voxel.getColor();
						blocks[idx] = (int8_t)currentPalIdx;
					}
				}
			}
		}
		compound.put("Blocks", priv::NamedBinaryTag(core::move(blocks)));
	}
	const priv::NamedBinaryTag tag(core::move(compound));
	return priv::NamedBinaryTag::write(tag, "Schematic", zipStream);
}

} // namespace voxelformat
