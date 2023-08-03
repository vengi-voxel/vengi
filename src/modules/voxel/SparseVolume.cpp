/**
 * @file
 */

#include "SparseVolume.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxel {

SparseVolume::SparseVolume(const voxel::Region &region) : _region(region), _isRegionValid(_region.isValid()) {
}

bool SparseVolume::setVoxel(const glm::ivec3 &pos, const voxel::Voxel &voxel) {
	if (_isRegionValid && !_region.containsPoint(pos)) {
		return false;
	}
	if (isAir(voxel.getMaterial())) {
		_map.remove(pos);
		return true;
	}
	_map.put(pos, voxel);
	return true;
}

const Voxel &SparseVolume::voxel(const glm::ivec3 &pos) const {
	auto iter = _map.find(pos);
	if (iter != _map.end()) {
		return iter->second;
	}
	return _emptyVoxel;
}

void SparseVolume::copyTo(voxel::RawVolumeWrapper &target) const {
	for (auto iter = _map.begin(); iter != _map.end(); ++iter) {
		const glm::ivec3 &pos = iter->first;
		const voxel::Voxel &voxel = iter->second;
		target.setVoxel(pos.x, pos.y, pos.z, voxel);
	}
}

void SparseVolume::copyFrom(const voxel::RawVolume &source) {
	auto visitor = [this](int x, int y, int z, const voxel::Voxel &voxel) { setVoxel(x, y, z, voxel); };
	voxelutil::visitVolume(source, visitor);
}

} // namespace voxel
