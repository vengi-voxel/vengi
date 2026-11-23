/**
 * @brief
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "voxel/Voxel.h"
#include "math/Vec3i.h"

namespace voxel {

struct VoxelPosition {
	math::Vec3i pos{0};
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
	void setVoxel(const math::Vec3i &pos, const Voxel &voxel);

	/**
	 * @brief Runtime is O(1)
	 */
	void setVoxel(int x, int y, int z, const Voxel &voxel);

	/**
	 * @brief Runtime is O(N)
	 */
	bool hasVoxel(const math::Vec3i &pos) const;

	/**
	 * @brief Runtime is O(N)
	 */
	const Voxel &voxel(const math::Vec3i &pos) const;
	const Voxel &voxel(int x, int y, int z) const;
};

inline void DynamicVoxelArray::setVoxel(int x, int y, int z, const Voxel &voxel) {
	setVoxel(math::Vec3i(x, y, z), voxel);
}

inline const Voxel &DynamicVoxelArray::voxel(int x, int y, int z) const {
	return voxel(math::Vec3i(x, y, z));
}

} // namespace voxel