/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxel {

/**
 * @brief Sproxel importer (csv)
 *
 * https://github.com/emilk/sproxel/blob/master/ImportExport.cpp
 */
class SproxelFormat : public Format {
public:
	bool loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
