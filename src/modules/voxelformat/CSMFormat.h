/**
 * @file
 */

#pragma once

#include "VoxFileFormat.h"
#include "io/FileStream.h"
#include "io/File.h"
#include "core/String.h"

namespace voxel {

/**
 * @brief Chronovox Studio Model and Nick's Voxel Model
 */
class CSMFormat : public VoxFileFormat {
public:
	bool loadGroups(const io::FilePtr& file, VoxelVolumes& volumes) override;
	bool saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) override;
};

}
