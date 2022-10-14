/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxelformat {

/**
 * @brief SLAB6 vox format
 *
 * @ingroup Formats
 */
class SLAB6VoxFormat : public PaletteFormat {
protected:
	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream& stream, SceneGraph &sceneGraph, voxel::Palette &palette) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
