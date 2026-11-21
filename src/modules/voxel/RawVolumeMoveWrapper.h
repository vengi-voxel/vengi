/**
 * @file
 */

#pragma once

#include "voxel/RawVolume.h"
#include "voxel/VolumeSampler.h"

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
	class Sampler : public VolumeSampler<RawVolumeMoveWrapper> {
	private:
		using Super = VolumeSampler<RawVolumeMoveWrapper>;
	public:
		VOLUMESAMPLERUSING;

		inline bool setVoxel(const Voxel &voxel) {
			if (_currentPositionInvalid) {
				const glm::ivec3& into = _region.moveInto(position().x, position().y, position().z);
				_volume->setVoxel(into, voxel);
				return true;
			}
			if (core_likely(_currentVoxel != nullptr)) {
				*_currentVoxel = voxel;
			} else {
				writeCurrentSparse(voxel, false);
			}
			return true;
		}
	};

	RawVolumeMoveWrapper(voxel::RawVolume* volume) :
			_volume(volume), _region(volume->region()) {
	}

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

	inline const uint32_t *sparseGenerationPtr() const {
		return _volume->sparseGenerationPtr();
	}

	inline void sampleSparseAt(int lx, int ly, int lz, const SparseChunk *&outChunk,
							   const SparseChunk::Brick *&outBrick, const Voxel *&outRow, bool &outWholeBrick,
							   glm::ivec3 &outLower, glm::ivec3 &outUpper, Voxel &outFallbackVoxel,
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
};

}
