/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "voxel/polyvox/Region.h"
#include "voxel/polyvox/PagedVolume.h"

namespace voxel {

/**
 * @brief Wrapper around a PagedVolume that reduces the amount of needed locks.
 */
class PagedVolumeWrapper {
private:
	PagedVolume* _pagedVolume;
	PagedVolume::Chunk* _chunk;
	Region _validRegion;
	Region _region;
public:
	class Sampler : public PagedVolume::Sampler {
	private:
		using Super = PagedVolume::Sampler;
		PagedVolume::Chunk* _chunk;
	public:
		Sampler(const PagedVolumeWrapper* volume) : Super(volume->getVolume()), _chunk(volume->_chunk) {}

		Sampler(const PagedVolumeWrapper& volume) : Super(volume.getVolume()), _chunk(volume._chunk) {};

		void setPosition(int32_t xPos, int32_t yPos, int32_t zPos) override {
			_xPosInVolume = xPos;
			_yPosInVolume = yPos;
			_zPosInVolume = zPos;

			// Then we update the voxel pointer
			const int32_t xChunk = _xPosInVolume >> _volume->_chunkSideLengthPower;
			const int32_t yChunk = _yPosInVolume >> _volume->_chunkSideLengthPower;
			const int32_t zChunk = _zPosInVolume >> _volume->_chunkSideLengthPower;

			_xPosInChunk = static_cast<uint16_t>(_xPosInVolume - (xChunk << _volume->_chunkSideLengthPower));
			_yPosInChunk = static_cast<uint16_t>(_yPosInVolume - (yChunk << _volume->_chunkSideLengthPower));
			_zPosInChunk = static_cast<uint16_t>(_zPosInVolume - (zChunk << _volume->_chunkSideLengthPower));
			const uint32_t voxelIndexInChunk = morton256_x[_xPosInChunk] | morton256_y[_yPosInChunk] | morton256_z[_zPosInChunk];

			PagedVolume::Chunk* currentChunk;
			const glm::ivec3& p = _chunk->_chunkSpacePosition;
			if (p.x == xChunk && p.y == yChunk && p.z == zChunk) {
				currentChunk = _chunk;
			} else {
				PagedVolume::VolumeLockGuard scopedLock(_volume->_lock);
				currentChunk = _volume->getChunk(xChunk, yChunk, zChunk);
			}

			_currentVoxel = currentChunk->_data + voxelIndexInChunk;
		}
	};

	PagedVolumeWrapper(PagedVolume* voxelStorage, PagedVolume::Chunk* chunk, const Region& region) :
			_pagedVolume(voxelStorage), _chunk(chunk), _region(region) {
		if (_chunk != nullptr) {
			_validRegion = _chunk->getRegion();
		}
	}

	inline const Region& getRegion() const {
		return _region;
	}

	inline operator PagedVolume& () const {
		return *_pagedVolume;
	}

	inline operator const PagedVolume& () const {
		return *_pagedVolume;
	}

	inline operator PagedVolume* () const {
		return _pagedVolume;
	}

	inline operator const PagedVolume* () const {
		return _pagedVolume;
	}

	inline PagedVolume::Chunk* getChunk() const {
		return _chunk;
	}

	inline PagedVolume* getVolume() const {
		return _pagedVolume;
	}

	inline bool setVoxel(const glm::ivec3& pos, const Voxel& voxel) {
		return setVoxel(pos.x, pos.y, pos.z, voxel);
	}

	inline const Voxel& getVoxel(const glm::ivec3& pos) const {
		return getVoxel(pos.x, pos.y, pos.z);
	}

	inline const Voxel& getVoxel(int x, int y, int z) const {
		if (_validRegion.containsPoint(x, y, z)) {
			core_assert(_chunk != nullptr);
			return _chunk->getVoxel(x - _validRegion.getLowerX(), y - _validRegion.getLowerY(), z - _validRegion.getLowerZ());
		}
		core_assert(_pagedVolume != nullptr);
		return _pagedVolume->getVoxel(x, y, z);
	}

	inline bool setVoxel(int x, int y, int z, const Voxel& voxel) {
		if (_validRegion.containsPoint(x, y, z)) {
			core_assert(_chunk != nullptr);
			_chunk->setVoxel(x - _validRegion.getLowerX(), y - _validRegion.getLowerY(), z - _validRegion.getLowerZ(), voxel);
			return true;
		}
		core_assert(_pagedVolume != nullptr);
		_pagedVolume->setVoxel(x, y, z, voxel);
		return true;
	}

	inline bool setVoxels(int x, int z, const Voxel* voxels, int amount) {
		return setVoxels(x, 0, z, 1, 1, voxels, amount);
	}

	inline bool setVoxels(int x, int y, int z, int nx, int nz, const Voxel* voxels, int amount) {
		for (int j = 0; j < nx; ++j) {
			for (int k = 0; k < nz; ++k) {
				const int fx = x + j;
				const int fz = z + k;
				int left = amount;
				if (_validRegion.containsPoint(fx, y, fz)) {
					// first part goes into the chunk
					const int h = _validRegion.getHeightInVoxels();
					_chunk->setVoxels(fx - _validRegion.getLowerX(), y - _validRegion.getLowerY(), fz - _validRegion.getLowerZ(), voxels, std::min(h, left));
					left -= h;
					if (left > 0) {
						// everything else goes into the volume
						core_assert(_pagedVolume != nullptr);
						_pagedVolume->setVoxels(fx, y + h, fz, 1, 1, voxels + h, left);
					}
				} else {
					// TODO: add region/chunk support here, too
					core_assert(_pagedVolume != nullptr);
					_pagedVolume->setVoxels(fx, y, fz, 1, 1, voxels, left);
				}
			}
		}
		return true;
	}

	inline bool setVoxels(int x, int y, int z, const Voxel* voxels, int amount) {
		return setVoxels(x, y, z, 1, 1, voxels, amount);
	}
};

}
