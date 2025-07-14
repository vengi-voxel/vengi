/**
 * @file
 */

#pragma once

#include "voxel/RawVolume.h"

namespace voxelutil {

/**
 * @brief Resizes a volume to cut off empty parts
 */
[[nodiscard]] voxel::RawVolume* cropVolume(const voxel::RawVolume* volume, const glm::ivec3& mins, const glm::ivec3& maxs);

/**
 * @brief Resizes a volume to cut off empty parts
 */
[[nodiscard]] voxel::RawVolume *cropVolume(const voxel::RawVolume *volume);

}
