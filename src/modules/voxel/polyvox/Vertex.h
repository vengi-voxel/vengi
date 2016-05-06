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
	glm::vec3 position;
	Voxel data;
};

/**
 * A specialised vertex format which encodes the data from the cubic extraction algorithm in a very
 * compact way. You will probably want to use the decodeVertex() function to turn it into a regular
 * Vertex for rendering, but advanced users should also be able to decode it on the GPU (not tested).
 */
struct CubicVertex {
	/**
	 * Each component of the position is stored as a single unsigned byte.
	 * The true position is found by offseting each component by 0.5f.
	 */
	glm::i8vec3 encodedPosition;

	/** A copy of the data which was stored in the voxel which generated this vertex. */
	Voxel data;
};

}
