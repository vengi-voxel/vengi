/**
 * @file
 */

#pragma once

#include "Voxel.h"
#include <glm/vec3.hpp>

namespace voxel {

/**
 * @brief Represents a vertex in a mesh and includes position and ambient occlusion
 * as well as color and material information.
 */
struct VoxelVertex {
	glm::i16vec3 position;
	/** 0 is the darkest, 3 is no occlusion at all */
	uint8_t ambientOcclusion;
	uint8_t colorIndex;
};
static_assert(sizeof(VoxelVertex) == 8, "Unexpected size of the vertex struct");

// TODO: maybe reduce to uint16_t and use glDrawElementsBaseVertex
typedef uint32_t IndexType;

}
