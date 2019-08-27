/**
 * @file
 */

#pragma once

#include "io/File.h"
#include "VoxFileFormat.h"

namespace voxelformat {

extern bool loadVolumeFormat(const io::FilePtr& filePtr, voxel::VoxelVolumes& newVolumes);

}
