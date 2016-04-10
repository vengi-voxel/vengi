#pragma once

#include "Vector.h"

#include <bitset>
#include <vector>

namespace PolyVox {

/**
 * Represents a vertex in a mesh and includes position and normal information.
 * There is also a 'data' member, which usually stores the (possibly interpolated)
 * value of the voxel(s) which caused the vertex to be generated.
 */
template<typename _DataType>
struct Vertex {
	typedef _DataType DataType;

	Vector3DFloat position;
	Vector3DFloat normal;
	DataType data;
};

}
