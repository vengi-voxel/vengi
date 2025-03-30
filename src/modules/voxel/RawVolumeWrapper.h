/**
 * @file
 */

#pragma once

#include "voxel/RawVolume.h"

namespace voxel {

/**
 * @brief A wrapper for a RawVolume that performs a sanity check for the setVoxel call.
 */
class RawVolumeWrapper {
protected:
	RawVolume* _volume;
	Region _region;
	Region _dirtyRegion = Region::InvalidRegion;

public:
	class Sampler : public VolumeSampler<RawVolumeWrapper> {
	private:
		using Super = VolumeSampler<RawVolumeWrapper>;
	public:
		VOLUMESAMPLERUSING;

		inline bool setVoxel(const Voxel& voxel) {
			if (_currentPositionInvalid) {
				return false;
			}
			*_currentVoxel = voxel;
			voxel::Region &dirtyRegion = _volume->_dirtyRegion;
			if (dirtyRegion.isValid()) {
				dirtyRegion.accumulate(_posInVolume);
			} else {
				dirtyRegion = Region(_posInVolume, _posInVolume);
			}
			return true;
		}
	};

	RawVolumeWrapper(voxel::RawVolume* volume) :
			_volume(volume), _region(volume->region()) {
	}

	RawVolumeWrapper(voxel::RawVolume* volume, const voxel::Region &region) :
			_volume(volume), _region(region) {
		_region.cropTo(volume->region());
	}

	virtual ~RawVolumeWrapper() {}

	inline Voxel *voxels() const {
		return _volume->voxels();
	}

	inline int width() const {
		return _volume->width();
	}
	inline int height() const {
		return _volume->height();
	}
	inline int depth() const {
		return _volume->depth();
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

	inline RawVolume* volume() const {
		return _volume;
	}

	void fill(const voxel::Voxel &voxel) {
		_volume->fill(voxel);
		_dirtyRegion = _volume->region();
	}

	void clear() {
		_dirtyRegion = _volume->region();
		_volume->clear();
	}

	inline void setVolume(RawVolume* v) {
		if (_volume == v) {
			return;
		}
		_volume = v;
		_dirtyRegion = Region::InvalidRegion;
		if (_volume == nullptr) {
			_region = Region::InvalidRegion;
		} else {
			if (_region.isValid()) {
				_region.cropTo(_volume->region());
			} else {
				_region = _volume->region();
			}
		}
	}

	inline const Region& region() const {
		return _region;
	}

	void setRegion(const Region& region) {
		_region.cropTo(region);
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

	inline const Region& dirtyRegion() const {
		return _dirtyRegion;
	}

	/**
	 * @return @c false if the voxel was not placed because the given position is outside of the valid region, @c
	 * true if the voxel was placed in the region.
	 * @note The return values have a different meaning as for the wrapped RawVolume.
	 */
	virtual bool setVoxel(int x, int y, int z, const Voxel& voxel) {
		const glm::ivec3 p(x, y, z);
		if (!_region.containsPoint(p)) {
			return false;
		}
		if (_volume->setVoxel(p, voxel)) {
			if (_dirtyRegion.isValid()) {
				_dirtyRegion.accumulate(p);
			} else {
				_dirtyRegion = Region(p, p);
			}
		}
		return true;
	}
};

}
