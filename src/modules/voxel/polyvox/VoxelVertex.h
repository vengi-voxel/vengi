/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "Voxel.h"
#include <glm/fwd.hpp>

namespace voxel {

/**
 * @brief Represents a vertex in a mesh and includes position and ambient occlusion
 * as well as color and material information.
 */
struct VoxelVertex {
	glm::ivec3 position;
	uint8_t ambientOcclusion;
	uint8_t colorIndex;
	// currently we only need to know whether it's water, or not.
	VoxelType material;
	uint8_t padding[1];
};
static_assert(sizeof(VoxelVertex) == 16, "Unexpected size of the vertex struct");

}
