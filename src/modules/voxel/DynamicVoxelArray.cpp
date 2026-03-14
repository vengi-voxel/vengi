/**
 * @brief
 */

#include "DynamicVoxelArray.h"

namespace voxel {

void DynamicVoxelArray::setVoxel(const glm::ivec3 &pos, const Voxel &voxel) {
	push_back({pos, voxel});
}

bool DynamicVoxelArray::hasVoxel(const glm::ivec3 &pos) const {
	for (const VoxelPosition &vp : *this) {
		if (vp.pos == pos) {
			return true;
		}
	}
	return false;
}

const Voxel &DynamicVoxelArray::voxel(const glm::ivec3 &pos) const {
	for (const VoxelPosition &vp : *this) {
		if (vp.pos == pos) {
			return vp.voxel;
		}
	}
	static const Voxel air;
	return air;
}

} // namespace voxel