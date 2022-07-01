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
#include "voxelformat/private/PaletteLookup.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeMerger.h"

#include <glm/common.hpp>

namespace voxelformat {

bool SchematicFormat::loadGroupsPalette(const core::String &filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph, voxel::Palette &palette) {
	palette.minecraft();
	io::ZipReadStream zipStream(stream);
	priv::NamedBinaryTagContext ctx;
	ctx.stream = &zipStream;
	const priv::NamedBinaryTag &schematic = priv::NamedBinaryTag::parse(ctx);
	if (!schematic.valid()) {
		Log::error("Could not find 'Schematic' tag");
		return false;
	}

	const priv::NamedBinaryTag &blocks = schematic.get("Blocks");
	if (blocks.valid() && blocks.type() == priv::TagType::BYTE_ARRAY) {
		return parseBlocks(schematic, sceneGraph, palette, blocks);
	}
	const priv::NamedBinaryTag &blockData = schematic.get("BlockData");
	if (blockData.valid() && blockData.type() == priv::TagType::BYTE_ARRAY) {
		// TODO: not yet supported
		return parseBlocks(schematic, sceneGraph, palette, blockData);
	}
	Log::error("Could not find valid 'Blocks' or 'BlockData' tags");
	return false;
}

bool SchematicFormat::parseBlocks(const priv::NamedBinaryTag &schematic, SceneGraph &sceneGraph, voxel::Palette &palette, const priv::NamedBinaryTag &blocks) {
	const int16_t width = schematic.get("Width").int16();
	const int16_t height = schematic.get("Height").int16();
	const int16_t depth = schematic.get("Length").int16();

	core::Buffer<int> mcpal;
	mcpal.resize(voxel::PaletteMaxColors);
	int paletteEntry = 0;
	const priv::NamedBinaryTag &blockIds = schematic.get("BlockIDs"); // MCEdit2
	if (blockIds.valid()) {
		const int blockCnt = (int)blockIds.compound()->size();
		const PaletteMap &map = getPaletteMap();
		for (int i = 0; i < blockCnt; ++i) {
			const priv::NamedBinaryTag &nbt = blockIds.get(core::String::format("%i", i));
			const core::String *value = nbt.string();
			if (value == nullptr) {
				Log::warn("Empty string in BlockIDs for %i", i);
				continue;
			}
			// skip minecraft:
			const core::String key = value->contains("minecraft:") ? value->substr(10) : *value;
			auto iter = map.find(key);
			if (iter == map.end()) {
				Log::warn("Could not find a color mapping for '%s'", key.c_str());
				mcpal[i] = 1; // map to stone
			} else {
				mcpal[i] = iter->value.palIdx;
			}
			++paletteEntry;
		}
	}

	PaletteLookup palLookup(palette);
	voxel::RawVolume *volume = new voxel::RawVolume(voxel::Region(0, 0, 0, width - 1, height - 1, depth - 1));
	for (int x = 0; x < width; ++x) {
		for (int y = 0; y < height; ++y) {
			for (int z = 0; z < depth; ++z) {
				const int idx = (y * depth + z) * width + x;
				const uint8_t palIdx = (*blocks.byteArray())[idx];
				if (palIdx != 0u) {
					uint8_t currentPalIdx;
					if (paletteEntry == 0) {
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
	sceneGraph.emplace(core::move(node));
	return true;
}

bool SchematicFormat::saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) {
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
	{
		core::DynamicArray<int8_t> blocks;
		blocks.resize(size.x * size.y * size.z);

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
