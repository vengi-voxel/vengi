/**
 * @file
 */

#pragma once

#include "io/File.h"
#include "VoxelVolumes.h"

namespace voxelformat {

extern const char *SUPPORTED_VOXEL_FORMATS_LOAD;
extern const char *SUPPORTED_VOXEL_FORMATS_LOAD_LIST[];
extern const char *SUPPORTED_VOXEL_FORMATS_SAVE;

extern bool loadVolumeFormat(const io::FilePtr& filePtr, voxel::VoxelVolumes& newVolumes);
extern bool saveVolumeFormat(const io::FilePtr& filePtr, voxel::VoxelVolumes& volumes);
extern void clearVolumes(voxel::VoxelVolumes& volumes);

}
