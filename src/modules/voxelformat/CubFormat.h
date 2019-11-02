/**
 * @file
 */

#pragma once

#include "VoxFileFormat.h"
#include "core/io/FileStream.h"
#include "core/io/File.h"
#include <string>

namespace voxel {

/**
 * @brief CubeWorld cub format
 *
 * The first 12 bytes of the file are the width, depth and height of the volume (uint32_t little endian).
 * The remaining parts are the RGB values (3 bytes)
 */
class CubFormat : public VoxFileFormat {
public:
	bool loadGroups(const io::FilePtr& file, VoxelVolumes& volumes) override;
	bool saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) override;
};

}
