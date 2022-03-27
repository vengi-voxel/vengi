/**
 * @file
 */

#include "SchematicFormat.h"
#include "core/Color.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/StringMap.h"
#include "io/File.h"
#include "io/MemoryReadStream.h"
#include "io/ZipReadStream.h"
#include "private/MinecraftPaletteMap.h"
#include "private/NamedBinaryTag.h"
#include "voxel/MaterialColor.h"
#include "voxel/Palette.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeMerger.h"

#include <glm/common.hpp>

namespace voxelformat {

bool SchematicFormat::loadGroups(const core::String &filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph) {
	_palette.minecraft();
	const voxel::Palette &palette = voxel::getPalette();
	for (size_t i = 0; i < _palette.size(); ++i) {
		_paletteMapping[i] = palette.getClosestMatch(core::Color::fromRGBA(_palette.colors[i]));
	}
	io::ZipReadStream zipStream(stream);
	priv::NamedBinaryTagContext ctx;
	ctx.stream = &zipStream;
	const priv::NamedBinaryTag &schematic = priv::NamedBinaryTag::parse(ctx);
	if (!schematic.valid()) {
		Log::error("Could not find 'Schematic' tag");
		return false;
	}

	const int16_t width = schematic.get("Width").int16();
	const int16_t height = schematic.get("Height").int16();
	const int16_t depth = schematic.get("Length").int16();
	voxel::RawVolume *volume = new voxel::RawVolume(voxel::Region(0, 0, 0, width, height, depth));
	const priv::NamedBinaryTag &blocks = schematic.get("Blocks");
	if (!blocks.valid()) {
		Log::error("Could not find 'Blocks' tag");
		return false;
	}

	// const int8_t itemStackVersion = schematic.get("itemStackVersion").int8(); // MCEdit2

	core::Buffer<int> pal;
	pal.resize(voxel::PaletteMaxColors);
	int paletteEntry = 0;
	const priv::NamedBinaryTag &blockIds = schematic.get("BlockIDs"); // MCEdit2
	if (blockIds.valid()) {
		const int blockCnt = (int)blockIds.compound()->size();
		const PaletteMap &map = getPaletteMap();
		for (int i = 0; i < blockCnt; ++i) {
			const priv::NamedBinaryTag &nbt = blockIds.get(core::String::format("%i", i));
			const core::String *value = nbt.string();
			if (value == nullptr) {
				continue;
			}
			// skip minecraft:
			const core::String key = value->contains("minecraft:") ? value->substr(10) : *value;
			auto iter = map.find(key);
			if (iter == map.end()) {
				Log::debug("Could not find a color mapping for '%s'", key.c_str());
				pal[paletteEntry] = -1;
			} else {
				pal[paletteEntry] = iter->value;
			}
			++paletteEntry;
		}
	}

	// const priv::NamedBinaryTag &materials = schematic.get("Materials");
	// Classic, Pocket, Alpha
	// const priv::NamedBinaryTag &schematicaMapping = schematic.get("SchematicaMapping");
	// const priv::NamedBinaryTag &data = schematic.get("Data");

	for (int x = 0; x < width; ++x) {
		for (int y = 0; y < height; ++y) {
			for (int z = 0; z < depth; ++z) {
				const int idx = (y * depth + z) * width + x;
				const uint8_t palIdx = (*blocks.byteArray())[idx];
				if (palIdx != 0u) {
					if (paletteEntry == 0) {
						volume->setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, palIdx));
					} else {
						volume->setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, pal[palIdx]));
					}
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
	sceneGraph.emplace(core::move(node));
	return true;
}

} // namespace voxelformat
