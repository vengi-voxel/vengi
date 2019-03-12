/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "Voxel.h"
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>

namespace voxel {

/**
 * @brief Represents a vertex in a mesh and includes position and ambient occlusion
 * as well as color and material information.
 */
struct alignas(16) VoxelVertex {
	glm::ivec3 position;
	/** 0 is the darkest, 3 is no occlusion at all */
	uint8_t ambientOcclusion;
	uint8_t colorIndex;
	/* currently we only need to know whether it's water, or not. */
	VoxelType material;
	uint8_t padding[1];
};
static_assert(sizeof(VoxelVertex) == 16, "Unexpected size of the vertex struct");

}
