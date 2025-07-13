/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/Trace.h"
#include "core/collection/DynamicArray.h"
#include "core/concurrent/Lock.h"
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
	mutable core_trace_mutex(core::Lock, _lock, "RawVolumeWrapper");

public:
	class Sampler : public VolumeSampler<RawVolumeWrapper> {
	private:
		using Super = VolumeSampler<RawVolumeWrapper>;
		core::DynamicArray<glm::ivec3, 1024> _positions;
	public:
		VOLUMESAMPLERUSING;

		CORE_FORCE_INLINE CORE_NO_SANITIZE_ADDRESS bool setVoxel(const Voxel& voxel) {
			if (_currentPositionInvalid) {
				return false;
			}
			*_currentVoxel = voxel;
			if (_positions.size() >= _positions.capacity()) {
				_volume->addToDirtyRegion(_positions);
				_positions.clear();
			}
			_positions.push_back(_posInVolume);
			return true;
		}

		void flush() {
			_volume->addToDirtyRegion(_positions);
		}

		~Sampler() {
			flush();
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

	inline void addToDirtyRegion(const glm::ivec3 &pos) {
		core::ScopedLock lock(_lock);
		if (_dirtyRegion.isValid()) {
			_dirtyRegion.accumulate(pos);
		} else {
			_dirtyRegion = Region(pos, pos);
		}
	}

	template<class COLLECTION>
	void addToDirtyRegion(const COLLECTION &positions) {
		if (positions.empty()) {
			return;
		}
		core::ScopedLock lock(_lock);
		if (!_dirtyRegion.isValid()) {
			_dirtyRegion = Region(positions[0], positions[0]);
		}
		for (const glm::ivec3 &pos : positions) {
			_dirtyRegion.accumulate(pos);
		}
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

	CORE_FORCE_INLINE CORE_NO_SANITIZE_ADDRESS const Voxel& voxel(const glm::ivec3& pos) const {
		return _volume->voxel(pos.x, pos.y, pos.z);
	}

	CORE_FORCE_INLINE CORE_NO_SANITIZE_ADDRESS const Voxel& voxel(int x, int y, int z) const {
		return _volume->voxel(x, y, z);
	}

	CORE_FORCE_INLINE CORE_NO_SANITIZE_ADDRESS bool setVoxel(const glm::ivec3& pos, const Voxel& voxel) {
		return setVoxel(pos.x, pos.y, pos.z, voxel);
	}

	inline const Region& dirtyRegion() const {
		return _dirtyRegion;
	}

	/**
	 * @return @c false if the voxel was not placed because the given position is outside of the valid region, @c
	 * true if the voxel was placed in the region.
	 * @note The return values have a different meaning as for the wrapped RawVolume.
	 * @note You should never use this directly - but just with a sampler
	 */
	virtual bool setVoxel(int x, int y, int z, const Voxel& voxel) {
		const glm::ivec3 p(x, y, z);
		if (!_region.containsPoint(p)) {
			return false;
		}
		if (_volume->setVoxel(p, voxel)) {
			addToDirtyRegion(p);
		}
		return true;
	}
};

}
