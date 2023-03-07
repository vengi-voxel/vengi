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
	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
						   voxel::Palette &palette, const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					io::SeekableWriteStream &stream, const SaveContext &ctx) override {
		return false;
	}
};

} // namespace voxelformat
