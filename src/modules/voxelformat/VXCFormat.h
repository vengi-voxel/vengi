/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxel {

/**
 * @brief VXC files are just a list of compressed files
 */
class VXCFormat : public Format {
public:
	bool loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
