/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/collection/DynamicArray.h"
#include "core/concurrent/Lock.h"
#include "voxel/RawVolume.h"
#include "math/Vec3i.h"

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
		core::DynamicArray<math::Vec3i, 1024> _positions;
	public:
		VOLUMESAMPLERUSING;

		CORE_FORCE_INLINE CORE_NO_SANITIZE_ADDRESS bool setVoxel(const Voxel& voxel) {
			if (_currentPositionInvalid) {
				return false;
			}
			if (core_likely(_currentVoxel != nullptr)) {
				*_currentVoxel = voxel;
			} else {
				writeCurrentSparse(voxel, false);
			}
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

	inline Voxel *linearVoxels() const {
		return _volume->linearVoxels();
	}

	inline const Region &volumeRegion() const {
		return _volume->volumeRegion();
	}

	inline const Voxel &borderValue() const {
		return _volume->borderValue();
	}

	inline const core::AtomicInt *sparseGenerationPtr() const {
		return _volume->sparseGenerationPtr();
	}

	inline void sampleSparseAt(int lx, int ly, int lz, const SparseChunk *&outChunk,
							   const SparseChunk::Brick *&outBrick, const Voxel *&outRow, bool &outWholeBrick,
							   math::Vec3i &outLower, math::Vec3i &outUpper, Voxel &outFallbackVoxel,
							   uint32_t &outGen) const {
		_volume->sampleSparseAt(lx, ly, lz, outChunk, outBrick, outRow, outWholeBrick, outLower, outUpper,
								outFallbackVoxel, outGen);
	}

	inline bool setVoxelOverwrite(int32_t x, int32_t y, int32_t z, const Voxel &voxel) {
		return _volume->setVoxelOverwrite(x, y, z, voxel);
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

	inline void addToDirtyRegion(const math::Vec3i &pos) {
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
		for (const math::Vec3i &pos : positions) {
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

	CORE_FORCE_INLINE CORE_NO_SANITIZE_ADDRESS const Voxel& voxel(const math::Vec3i& pos) const {
		return _volume->voxel(pos.x, pos.y, pos.z);
	}

	CORE_FORCE_INLINE CORE_NO_SANITIZE_ADDRESS const Voxel& voxel(int x, int y, int z) const {
		return _volume->voxel(x, y, z);
	}

	CORE_FORCE_INLINE CORE_NO_SANITIZE_ADDRESS bool setVoxel(const math::Vec3i& pos, const Voxel& voxel) {
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
		const math::Vec3i p(x, y, z);
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
