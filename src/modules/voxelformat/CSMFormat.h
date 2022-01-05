/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxel {

/**
 * @brief Chronovox Studio Model and Nick's Voxel Model
 */
class CSMFormat : public Format {
public:
	bool loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& volumes) override;
	bool saveGroups(const SceneGraph& volumes, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
