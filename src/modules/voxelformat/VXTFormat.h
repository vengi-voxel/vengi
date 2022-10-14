/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxelformat {

/**
 * @brief VXT files are tilesets of (static) vxm models
 *
 * @ingroup Formats
 */
class VXTFormat : public PaletteFormat {
protected:
	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream& stream, SceneGraph &sceneGraph, voxel::Palette &palette) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
