/**
 * @file
 */

#pragma once

#include "io/File.h"
#include "io/FormatDescription.h"
#include "VoxelVolumes.h"
#include "video/Texture.h"
#include "core/collection/Array.h"

namespace voxelformat {

extern const io::FormatDescription SUPPORTED_VOXEL_FORMATS_LOAD[];
extern const char *SUPPORTED_VOXEL_FORMATS_LOAD_LIST[];
extern const io::FormatDescription SUPPORTED_VOXEL_FORMATS_SAVE[];

extern size_t loadVolumePalette(const io::FilePtr& filePtr, core::Array<uint32_t, 256> &palette);
extern image::ImagePtr loadVolumeScreenshot(const io::FilePtr& filePtr);
extern bool loadVolumeFormat(const io::FilePtr& filePtr, voxel::VoxelVolumes& newVolumes);
extern bool saveVolumeFormat(const io::FilePtr& filePtr, voxel::VoxelVolumes& volumes);
extern bool saveMeshFormat(const io::FilePtr& filePtr, voxel::VoxelVolumes& volumes);
extern bool isMeshFormat(const core::String& filename);
/**
 * @brief Save both to volume or to mesh - depends on the given file extension
 */
extern bool saveFormat(const io::FilePtr& filePtr, voxel::VoxelVolumes& volumes);
extern void clearVolumes(voxel::VoxelVolumes& volumes);

}
