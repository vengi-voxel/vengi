/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief veloren terrain format
 *
 * https://gitlab.com/veloren/veloren/
 *
 * V3:
 *   u64 version      // must equal 0x3352ACEEA7890003
 *   u64 count        // number of blocks in the vector
 *   BlocksV3[count]  // array of blocks
 *
 * BlocksV3:
 *   u8   x
 *   u8   y
 *   i16  z
 *   u32  blockId (u8 type, u8 r, u8 g, u8 b)
 *
 *
 * V2:
 *   u64 version      // magic = 0x3352ACEEA7890002
 *   u64 count        // number of blocks in vector
 *   BlocksV2[count]  // array of tuples
 *
 * BlocksV2:
 *   u8    x
 *   u8    y
 *   i16   z
 *   Block b
 *
 *
 * V1:
 *   u64 count       // number of map entries
 *   BlocksV1[count] // array of entries
 *
 * BlocksV1:
 *   i32   x
 *   i32   y
 *   i32   z
 *   Block b
 *
 * Block:
 *   u8 type
 *   u8 r
 *   u8 g
 *   u8 b
 *
 * @ingroup Formats
 */
class VelorenTerrainFormat : public RGBAFormat {
protected:
	enum BlockType : uint8_t {
		Air = 0x00,
		Water = 0x01,
		Rock = 0x10,
		WeakRock = 0x11,
		Lava = 0x12,
		GlowingRock = 0x13,
		GlowingWeakRock = 0x14,
		Grass = 0x20,
		Snow = 0x21,
		ArtSnow = 0x22,
		Earth = 0x30,
		Sand = 0x31,
		Wood = 0x40,
		Leaves = 0x41,
		GlowingMushroom = 0x42,
		Ice = 0x43,
		ArtLeaves = 0x44,
		Misc = 0xFE
	};

	bool loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						const palette::Palette &palette, const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;

public:
	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;

	bool singleVolume() const override {
		return true;
	}

	static const io::FormatDescription &format() {
		static io::FormatDescription f{
			"Veloren terrain",
			{"dat"},
			{{'\x03', '\x00', '\x89', '\xa7'}, {'\x02', '\x00', '\x89', '\xa7'}, {'\x01', '\x00', '\x89', '\xa7'}},
			VOX_FORMAT_FLAG_PALETTE_EMBEDDED | FORMAT_FLAG_SAVE | VOX_FORMAT_FLAG_RGB};
		return f;
	}
};

} // namespace voxelformat
