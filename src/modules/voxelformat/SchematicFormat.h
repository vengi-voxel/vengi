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
 */
class SchematicFormat : public PaletteFormat {
protected:
	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream& stream, SceneGraph &sceneGraph, voxel::Palette &palette) override;
public:
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
