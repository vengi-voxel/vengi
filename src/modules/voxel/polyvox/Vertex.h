/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "Voxel.h"

namespace voxel {

/**
 * Represents a vertex in a mesh and includes position and normal information.
 * There is also a 'data' member, which usually stores the (possibly interpolated)
 * value of the voxel(s) which caused the vertex to be generated.
 */
struct Vertex {
	glm::u8vec3 position;
	uint8_t ambientOcclusion;
	Voxel data;
};

}
