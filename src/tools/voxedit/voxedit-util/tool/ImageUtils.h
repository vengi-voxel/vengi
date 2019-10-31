/**
 * @file
 */

#pragma once

#include "image/Image.h"
#include "voxel/RawVolumeWrapper.h"

namespace voxedit {

extern void importHeightmap(voxel::RawVolumeWrapper& volume, const image::ImagePtr& image);
extern voxel::RawVolume* importAsPlane(const image::ImagePtr& image, uint8_t thickness = 1);

}
