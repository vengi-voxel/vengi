/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxelformat {

/**
 * @brief Minecraft level dat format
 *
 * https://minecraft.fandom.com/wiki/Level.dat
 *
 * @ingroup Formats
 */
class DatFormat : public PaletteFormat {
protected:
	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph,
						   voxel::Palette &palette) override;
	bool saveGroups(const SceneGraph &sceneGraph, const core::String &filename,
					io::SeekableWriteStream &stream) override {
		return false;
	}
};

} // namespace voxelformat
