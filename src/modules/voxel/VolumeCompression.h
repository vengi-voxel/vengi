/**
 * @file
 */

#pragma once

#include "voxel/RawVolume.h"

namespace voxel {

voxel::RawVolume *toVolume(const uint8_t *data, uint32_t dataSize, const voxel::Region &region);

} // namespace voxel
