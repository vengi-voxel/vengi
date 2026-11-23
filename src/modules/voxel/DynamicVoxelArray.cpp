/**
 * @brief
 */

#include "DynamicVoxelArray.h"
#include "math/Vec3i.h"

namespace voxel {

void DynamicVoxelArray::setVoxel(const math::Vec3i &pos, const Voxel &voxel) {
	push_back({pos, voxel});
}

bool DynamicVoxelArray::hasVoxel(const math::Vec3i &pos) const {
	for (const VoxelPosition &vp : *this) {
		if (vp.pos == pos) {
			return true;
		}
	}
	return false;
}

const Voxel &DynamicVoxelArray::voxel(const math::Vec3i &pos) const {
	for (const VoxelPosition &vp : *this) {
		if (vp.pos == pos) {
			return vp.voxel;
		}
	}
	static const Voxel air;
	return air;
}

} // namespace voxel