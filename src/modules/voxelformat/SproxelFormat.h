/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxelformat {

/**
 * @brief Sproxel importer (csv)
 *
 * @li https://github.com/emilk/sproxel/blob/master/ImportExport.cpp
 *
 * @ingroup Formats
 */
class SproxelFormat : public RGBAFormat {
public:
	bool loadGroupsRGBA(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, const voxel::Palette &palette) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
