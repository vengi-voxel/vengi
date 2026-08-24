/**
 * @file
 */

#pragma once

#include <glm/vec3.hpp>

namespace voxel {
class RawVolume;
}

namespace voxelutil {

/**
 * @brief Creates a cropped volume by cutting off parts without voxels
 */
[[nodiscard]] voxel::RawVolume *cropVolume(const voxel::RawVolume *volume, const glm::ivec3 &mins,
										   const glm::ivec3 &maxs);

/**
 * @brief Creates a cropped volume by cutting off parts without voxels
 */
[[nodiscard]] voxel::RawVolume *cropVolume(const voxel::RawVolume *volume);

/**
 * @brief Computes the tight axis-aligned bounds of non-air voxels in @p volume.
 * @return @c false if the volume contains no solid voxels.
 */
[[nodiscard]] bool computeSolidBounds(const voxel::RawVolume *volume, glm::ivec3 &mins, glm::ivec3 &maxs);

} // namespace voxelutil
