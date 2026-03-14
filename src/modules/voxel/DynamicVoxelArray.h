/**
 * @brief
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "voxel/Voxel.h"
#include <glm/vec3.hpp>

namespace voxel {

struct VoxelPosition {
	glm::ivec3 pos{0};
	Voxel voxel;
};

/**
 * @brief Runtime for everything in here is O(N) at least
 * @note Also implements some of the Volume methods to be used in some templated algorithms
 */
class DynamicVoxelArray : public core::DynamicArray<VoxelPosition> {
public:
	/**
	 * @brief This doesn't check if the voxel already exists at the position, it just adds to the end of the array. Use
	 * hasVoxel() to check for existence before adding if needed.
	 * Runtime is O(1)
	 */
	void setVoxel(const glm::ivec3 &pos, const Voxel &voxel);

	/**
	 * @brief Runtime is O(1)
	 */
	void setVoxel(int x, int y, int z, const Voxel &voxel);

	/**
	 * @brief Runtime is O(N)
	 */
	bool hasVoxel(const glm::ivec3 &pos) const;

	/**
	 * @brief Runtime is O(N)
	 */
	const Voxel &voxel(const glm::ivec3 &pos) const;
	const Voxel &voxel(int x, int y, int z) const;
};

inline void DynamicVoxelArray::setVoxel(int x, int y, int z, const Voxel &voxel) {
	setVoxel(glm::ivec3(x, y, z), voxel);
}

inline const Voxel &DynamicVoxelArray::voxel(int x, int y, int z) const {
	return voxel(glm::ivec3(x, y, z));
}

} // namespace voxel