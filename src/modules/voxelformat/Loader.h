/**
 * @file
 */

#pragma once

#include "core/io/File.h"
#include "VoxFileFormat.h"

namespace voxelformat {

extern bool loadVolumeFormat(const io::FilePtr& filePtr, voxel::VoxelVolumes& newVolumes);
extern bool saveVolumeFormat(const io::FilePtr& filePtr, voxel::VoxelVolumes& volumes);

}
