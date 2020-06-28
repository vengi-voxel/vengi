/**
 * @file
 */

#pragma once

#include "VoxFileFormat.h"
#include "core/io/FileStream.h"
#include "core/io/File.h"
#include "core/String.h"

namespace voxel {

/**
 * @brief AceOfSpades VXL format
 *
 * https://silverspaceship.com/aosmap/
 */
class AoSVXLFormat : public VoxFileFormat {
public:
	bool loadGroups(const io::FilePtr& file, VoxelVolumes& volumes) override;
	bool saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) override;
};

}
