/**
 * @file
 */

#pragma once

#include "Format.h"

namespace io {
class ZipReadStream;
}


namespace voxelformat {

namespace priv {
class NamedBinaryTag;
}

/**
 * @note https://minecraft.fandom.com/wiki/Schematic_file_format
 * @note https://github.com/SpongePowered/Schematic-Specification/tree/master/versions
 */
class SchematicFormat : public PaletteFormat {
protected:
	bool loadSponge1And2(const priv::NamedBinaryTag &schematic, SceneGraph &sceneGraph, voxel::Palette &palette);
	bool parseBlockData(const priv::NamedBinaryTag &schematic, SceneGraph &sceneGraph, voxel::Palette &palette, const priv::NamedBinaryTag &blocks);

	bool loadSponge3(const priv::NamedBinaryTag &schematic, SceneGraph &sceneGraph, voxel::Palette &palette);
	bool parseBlocks(const priv::NamedBinaryTag &schematic, SceneGraph &sceneGraph, voxel::Palette &palette, const priv::NamedBinaryTag &blocks);

	int parsePalette(const priv::NamedBinaryTag &schematic, core::Buffer<int> &mcpal) const;
	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream& stream, SceneGraph &sceneGraph, voxel::Palette &palette) override;
public:
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
