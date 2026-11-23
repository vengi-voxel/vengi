#include "math/Vec3i.h"
/**
 * @file
 */

#pragma once


namespace voxel {
class RawVolume;
class Region;
} // namespace voxel

namespace voxelutil {

/**
 * @brief Creates a new volume with the source volume voxels but with the given region size
 */
[[nodiscard]] voxel::RawVolume *resize(const voxel::RawVolume *source, const voxel::Region &region);
[[nodiscard]] voxel::RawVolume *resize(const voxel::RawVolume *source, const math::Vec3i &size, bool extendMins = false);

} // namespace voxelutil
