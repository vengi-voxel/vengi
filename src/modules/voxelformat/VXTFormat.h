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
	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream& stream, scenegraph::SceneGraph &sceneGraph, voxel::Palette &palette, const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream, const SaveContext &ctx) override;
};

}
