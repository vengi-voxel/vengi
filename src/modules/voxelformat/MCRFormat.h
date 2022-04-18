/**
 * @file
 */

#pragma once

#include "Format.h"
#include "core/collection/DynamicArray.h"

namespace io {
class ZipReadStream;
}


namespace voxelformat {

namespace priv {
class NamedBinaryTag;
using NBTCompound = core::StringMap<NamedBinaryTag>;
using NBTList = core::DynamicArray<NamedBinaryTag>;
}

/**
 * A minecraft chunk contains the terrain and entity information about a grid of the size 16x256x16
 *
 * A section is 16x16x16 and a chunk contains max 16 sections. Section 0 is the bottom, section 15 is the top
 *
 * @note This is stored in NBT format
 *
 * older version:
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
 * newer version
 * the block_states are under a sections compound
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
 * @note https://minecraft.fandom.com/wiki/Region_file_format
 * @note https://minecraft.fandom.com/wiki/Chunk_format
 * @note https://github.com/UnknownShadow200/ClassiCube/blob/master/src/Formats.c
 */
class MCRFormat : public Format {
private:
	static constexpr int VERSION_GZIP = 1;
	static constexpr int VERSION_DEFLATE = 2;
	static constexpr int SECTOR_BYTES = 4096;
	static constexpr int SECTOR_INTS = SECTOR_BYTES / 4;
	static constexpr int MAX_SIZE = 16;

	struct Offsets {
		uint32_t offset;
		uint8_t sectorCount;
	} _offsets[SECTOR_INTS];

	struct MinecraftSectionPalette {
		core::Buffer<uint8_t> pal;
		uint32_t numBits = 0u;
	};

	using SectionVolumes = core::DynamicArray<voxel::RawVolume *>;

	voxel::RawVolume* error(SectionVolumes &volumes);
	voxel::RawVolume* finalize(SectionVolumes& volumes, int xPos, int zPos);

	static int getVoxel(int dataVersion, const priv::NamedBinaryTag &data, const glm::ivec3 &pos);
	static int getVoxel(int dataVersion, const MinecraftSectionPalette& secPal, const priv::NamedBinaryTag& data, const glm::ivec3 &pos);

	// shared across versions
	bool parsePaletteList(int dataVersion, const priv::NamedBinaryTag& palette, MinecraftSectionPalette &sectionPal);
	bool parseBlockStates(int dataVersion, const priv::NamedBinaryTag &data, SectionVolumes &volumes, int sectionY, const MinecraftSectionPalette &secPal);

	// new version (>= 2844)
	voxel::RawVolume* parseSections(int dataVersion, const priv::NamedBinaryTag &root, int sector);

	// old version (< 2844)
	voxel::RawVolume* parseLevelCompound(int dataVersion, const priv::NamedBinaryTag &root, int sector);

	bool readCompressedNBT(SceneGraph& sceneGraph, io::SeekableReadStream &stream, int sector);
	bool loadMinecraftRegion(SceneGraph& sceneGraph, io::SeekableReadStream &stream);

	bool saveSections(const voxelformat::SceneGraph &sceneGraph, priv::NBTList &sections, int sector);
	bool saveCompressedNBT(const voxelformat::SceneGraph &sceneGraph, io::SeekableWriteStream& stream, int sector);
	bool saveMinecraftRegion(const voxelformat::SceneGraph &sceneGraph, io::SeekableWriteStream& stream);
public:
	bool loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
