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
	bool loadGroups(const core::String &filename, io::SeekableReadStream& stream, VoxelVolumes& volumes) override;
	bool saveGroups(const VoxelVolumes& volumes, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
