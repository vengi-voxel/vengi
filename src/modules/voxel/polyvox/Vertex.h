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
	glm::vec3 position;
	glm::vec3 normal;
	Voxel data;
};

}
