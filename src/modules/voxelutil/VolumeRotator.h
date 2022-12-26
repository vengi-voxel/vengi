/**
 * @file
 */

#pragma once

#include "math/Axis.h"
#include <glm/fwd.hpp>

namespace voxel {
class RawVolume;
class Voxel;
} // namespace voxel

namespace voxelutil {
/**
 * @brief Rotate the given volume by the given angles in degree
 */
extern voxel::RawVolume *rotateVolume(const voxel::RawVolume *source, const glm::vec3 &angles, const glm::vec3 &normalizedPivot);
/**
 * @brief Rotate the given volume on the given axis by 90 degree. This method does not lose any voxels
 * @note The volume size might differ
 */
extern voxel::RawVolume *rotateAxis(const voxel::RawVolume *source, math::Axis axis);
/**
 * @brief Mirrors the given volume on the given axis
 */
extern voxel::RawVolume *mirrorAxis(const voxel::RawVolume *source, math::Axis axis);

} // namespace voxelutil
