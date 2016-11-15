/**
 * @file
 */

#pragma once

#include "voxel/polyvox/RawVolume.h"

namespace voxel {
namespace generate {

/**
 * @brief A wrapper for a RawVolume that performs a sanity check for the setVoxel call. But in relation to
 * the normal RawVolumeWrapper class, this class moves voxel in from the other side if they are outside
 * the valid region.
 */
class RawVolumeMoveWrapper {
private:
	RawVolume* _volume;
	const Region& _region;

public:
	RawVolumeMoveWrapper(voxel::RawVolume* volume) :
			_volume(volume), _region(volume->getRegion()) {
	}

	inline operator RawVolume& () const {
		return *_volume;
	}

	inline operator const RawVolume& () const {
		return *_volume;
	}

	inline operator RawVolume* () const {
		return _volume;
	}

	inline operator const RawVolume* () const {
		return _volume;
	}

	inline const Region& getRegion() const {
		return _region;
	}

	inline const Voxel& getVoxel(const glm::ivec3& pos) const {
		return _volume->getVoxel(pos.x, pos.y, pos.z);
	}

	inline const Voxel& getVoxel(int x, int y, int z) const {
		return _volume->getVoxel(x, y, z);
	}

	inline bool setVoxel(const glm::ivec3& pos, const Voxel& voxel) {
		return setVoxel(pos.x, pos.y, pos.z, voxel);
	}

	inline bool setVoxel(int x, int y, int z, const Voxel& voxel) {
		if (!_region.containsPoint(x, y, z)) {
			const glm::ivec3& size = _region.getDimensionsInVoxels();
			int nx = x - _region.getLowerX();
			int ny = y - _region.getLowerY();
			int nz = z - _region.getLowerZ();
			while (nx < 0) {
				nx += _region.getWidthInVoxels();
			}
			while (ny < 0) {
				ny += _region.getHeightInVoxels();
			}
			while (nz < 0) {
				nz += _region.getDepthInVoxels();
			}
			const int ox = nx % size.x;
			const int oy = ny % size.y;
			const int oz = nz % size.z;
			core_assert_msg(_region.containsPoint(ox, oy, oz),
					"shifted(%i:%i:%i) is outside the valid region for pos(%i:%i:%i), size(%i:%i:%i)",
					ox, oy, oz, x, y, z, size.x, size.y, size.z);
			_volume->setVoxel(ox, oy, oz, voxel);
			return true;
		}
		_volume->setVoxel(x, y, z, voxel);
		return true;
	}

	inline bool setVoxels(int x, int z, const Voxel* voxels, int amount) {
		for (int y = 0; y < amount; ++y) {
			setVoxel(x, y, z, voxels[y]);
		}
		return true;
	}
};

}
}
