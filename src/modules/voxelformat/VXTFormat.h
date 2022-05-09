/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxelformat {

/**
 * @brief VXT files are tilesets of (static) vxm models
 */
class VXTFormat : public PaletteFormat {
public:
	bool loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
