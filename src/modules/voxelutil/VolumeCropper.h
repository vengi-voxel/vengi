/**
 * @file
 */

#pragma once

#include "voxel/RawVolume.h"

namespace voxelutil {

/**
 * @brief Creates a cropped volume by cutting off parts without voxels
 */
[[nodiscard]] voxel::RawVolume* cropVolume(const voxel::RawVolume* volume, const glm::ivec3& mins, const glm::ivec3& maxs);

/**
 * @brief Creates a cropped volume by cutting off parts without voxels
 */
[[nodiscard]] voxel::RawVolume *cropVolume(const voxel::RawVolume *volume);

}
