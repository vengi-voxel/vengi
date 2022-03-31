/**
 * @file
 */

#pragma once

#include "image/Image.h"
#include "voxel/RawVolumeWrapper.h"

namespace voxelutil {

extern void importHeightmap(voxel::RawVolumeWrapper& volume, const image::ImagePtr& image, const voxel::Voxel &underground, const voxel::Voxel &surface);
extern voxel::RawVolume* importAsPlane(const image::ImagePtr& image, uint8_t thickness = 1);
extern voxel::RawVolume* importAsVolume(const image::ImagePtr& image, const image::ImagePtr& heightmap, uint8_t maxHeight);

}
