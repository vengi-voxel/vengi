/**
 * @file
 */

#pragma once

#include "image/Image.h"
#include "voxel/polyvox/RawVolumeWrapper.h"

namespace voxedit {

void importHeightmap(voxel::RawVolumeWrapper& volume, const image::ImagePtr& image);

}
