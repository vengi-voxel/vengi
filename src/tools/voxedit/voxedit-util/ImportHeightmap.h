/**
 * @file
 */

#pragma once

#include "image/Image.h"
#include "voxel/polyvox/RawVolume.h"

namespace voxedit {

void importHeightmap(voxel::RawVolume& volume, const image::ImagePtr& image);

}
