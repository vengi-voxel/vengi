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
 * @note see voxel.vert for the shader layout
 */
struct VoxelVertex {
	glm::highp_vec3 position;
	/** 0 is the darkest, 3 is no occlusion at all */
	union {
		struct {
			uint8_t ambientOcclusion:2;
			uint8_t flags:1; // this should match the @c Voxel::_flags member
			uint8_t padding:5;
		};
		uint8_t info;
	};
	uint8_t colorIndex;
	uint8_t normalIndex; // NO_NORMAL means not set
	uint8_t padding2;
};
static_assert(sizeof(VoxelVertex) == 16, "Unexpected size of the vertex struct");

// TODO: maybe reduce to uint16_t and use glDrawElementsBaseVertex
typedef uint32_t IndexType;

}
