/**
 * @file
 */

#pragma once

#include <stdint.h>

namespace voxel {

/**
 * @brief Sampling type for interpolation
 */
enum class VoxelSampling : uint8_t {
	Nearest,
	Linear,
	Cubic,

	Max
};

} // namespace voxel