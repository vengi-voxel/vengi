/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxel {

/**
 * A minecraft chunk contains the terrain and entity information about a grid of the size 16x256x16
 *
 * A section is 16x16x16 and a chunk contains max 16 sections. Section 0 is the bottom, section 15 is the top
 *
 * @note This is stored in NBT format
 *
 * @code
 * root tag (compound)
 *   \-- DataVersion - version of the nbt chunk
 *   \-- Level - chunk data (compound)
 *     \-- xPos - x pos in chunk relative to the origin (not the region)
 *     \-- yPos - y pos in chunk relative to the origin (not the region)
 *     \-- Sections (list)
 *       \-- section (compound)
 *         \-- Y: Range 0 to 15 (bottom to top) - if empty, section is empty
 *         \-- Palette
 *         \-- BlockLight - 2048 bytes
 *         \-- BlockStates
 *         \-- SkyLight
 * @endcode
 *
 * @code
 * byte Nibble4(byte[] arr, int index) {
 *   return index%2 == 0 ? arr[index/2]&0x0F : (arr[index/2]>>4)&0x0F;
 * }
 * int BlockPos = y*16*16 + z*16 + x;
 * compound Block = Palette[change_array_element_size(BlockStates,Log2(length(Palette)))[BlockPos]]
 * string BlockName = Block.Name;
 * compound BlockState = Block.Properties;
 * byte Blocklight = Nibble4(BlockLight, BlockPos);
 * byte Skylight = Nibble4(SkyLight, BlockPos);
 * @endcode
 *
 * @note https://github.com/Voxtric/Minecraft-Level-Ripper/blob/master/WorldConverterV2/Processor.cs
 * @note https://github.com/dougbinks/enkiMI/
 */
class MCRFormat : public Format {
public:
	bool loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override {
		return false;
	}
	size_t loadPalette(const core::String &filename, io::SeekableReadStream& file, core::Array<uint32_t, 256> &palette) override;
};

}
