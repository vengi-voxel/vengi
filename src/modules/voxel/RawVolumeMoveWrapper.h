/**
 * @file
 */

#pragma once

#include "voxel/RawVolume.h"

namespace voxel {

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
	class Sampler : public RawVolume::Sampler {
	private:
		using Super = RawVolume::Sampler;
	public:
		Sampler(const RawVolumeWrapper* volume) : Super(volume->volume()) {}

		Sampler(const RawVolumeWrapper& volume) : Super(volume.volume()) {};
	};

	RawVolumeMoveWrapper(voxel::RawVolume* volume) :
			_volume(volume), _region(volume->region()) {
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

	inline const Region& region() const {
		return _region;
	}

	inline const Voxel& voxel(const glm::ivec3& pos) const {
		return _volume->voxel(pos.x, pos.y, pos.z);
	}

	inline const Voxel& voxel(int x, int y, int z) const {
		return _volume->voxel(x, y, z);
	}

	inline bool setVoxel(const glm::ivec3& pos, const Voxel& voxel) {
		return setVoxel(pos.x, pos.y, pos.z, voxel);
	}

	inline bool setVoxel(int x, int y, int z, const Voxel& voxel) {
		if (!_region.containsPoint(x, y, z)) {
			const glm::ivec3& into = _region.moveInto(x, y, z);
			_volume->setVoxel(into, voxel);
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
