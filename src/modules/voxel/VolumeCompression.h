/**
 * @file
 */

#pragma once

#include <stdint.h>

namespace voxel {

class RawVolume;
class Region;

voxel::RawVolume *toVolume(const uint8_t *data, uint32_t dataSize, const voxel::Region &region);

} // namespace voxel
