/**
 * @file
 */

#include "RawVolumeRotateWrapper.h"

namespace voxelutil {

RawVolumeRotateWrapper::RawVolumeRotateWrapper(const voxel::RawVolume *volume, math::Axis axis) :
		_volume(volume), _axis(axis) {
	const voxel::Region srcRegion = volume->region();
	_region = srcRegion;
	if (axis == math::Axis::Y) {
		_region.setLowerX(srcRegion.getLowerZ());
		_region.setLowerZ(srcRegion.getLowerX());
		_region.setUpperX(srcRegion.getUpperZ());
		_region.setUpperZ(srcRegion.getUpperX());
	} else if (axis == math::Axis::X) {
		_region.setLowerY(srcRegion.getLowerZ());
		_region.setLowerZ(srcRegion.getLowerX());
		_region.setUpperY(srcRegion.getUpperZ());
		_region.setUpperZ(srcRegion.getUpperY());
	} else if (axis == math::Axis::Z) {
		_region.setLowerY(srcRegion.getLowerX());
		_region.setLowerX(srcRegion.getLowerY());
		_region.setUpperY(srcRegion.getUpperX());
		_region.setUpperX(srcRegion.getUpperY());
	}
}

voxel::Region RawVolumeRotateWrapper::region() const {
	return _region;
}

const voxel::Voxel &RawVolumeRotateWrapper::voxel(int x, int y, int z) const {
	glm::ivec3 pos(x, y, z);
	if (_axis == math::Axis::X) {
		const int tmp = pos.y;
		pos.y = pos.z;
		pos.z = tmp;
	} else if (_axis == math::Axis::Y) {
		const int tmp = pos.x;
		pos.x = pos.z;
		pos.z = tmp;
	} else if (_axis == math::Axis::Z) {
		const int tmp = pos.x;
		pos.x = pos.y;
		pos.y = tmp;
	}
	return _volume->voxel(pos.x, pos.y, pos.z);
}

}