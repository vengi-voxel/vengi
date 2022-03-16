/**
 * @file
 */

#pragma once

#include "image/Image.h"
#include "voxel/RawVolumeWrapper.h"

namespace voxel {
class Palette;
}

namespace voxelutil {

extern void importHeightmap(voxel::RawVolumeWrapper& volume, const image::ImagePtr& image, const voxel::Voxel &underground, const voxel::Voxel &surface);
extern voxel::RawVolume* importAsPlane(const image::ImagePtr& image, uint8_t thickness = 1);
extern bool importPalette(const core::String& file, voxel::Palette &palette);


}
