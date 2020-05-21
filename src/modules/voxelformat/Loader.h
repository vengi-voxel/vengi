/**
 * @file
 */

#pragma once

#include "core/io/File.h"
#include "VoxFileFormat.h"

namespace voxelformat {

extern const char *SUPPORTED_VOXEL_FORMATS_LOAD;
extern const char *SUPPORTED_VOXEL_FORMATS_SAVE;

extern bool loadVolumeFormat(const io::FilePtr& filePtr, voxel::VoxelVolumes& newVolumes);
extern bool saveVolumeFormat(const io::FilePtr& filePtr, voxel::VoxelVolumes& volumes);
extern void clearVolumes(voxel::VoxelVolumes& volumes);

}
