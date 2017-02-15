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
		if (_validRegion.containsPoint(x, 0, z)) {
			// first part goes into the chunk
			const int h = _validRegion.getHeightInVoxels();
			_chunk->setVoxels(x - _validRegion.getLowerX(), z - _validRegion.getLowerZ(), voxels, std::min(h, amount));
			amount -= h;
			if (amount > 0) {
				// everything else goes into the volume
				core_assert(_pagedVolume != nullptr);
				_pagedVolume->setVoxels(x, z, voxels + h, amount);
			}
			return true;
		}
		// TODO: add region/chunk support here, too
		core_assert(_pagedVolume != nullptr);
		_pagedVolume->setVoxels(x, z, voxels, amount);
		return true;
	}

	inline bool setVoxels(int x, int y, int z, const Voxel* voxels, int amount) {
		for (int i = 0; i < amount; ++i) {
			if (_validRegion.containsPoint(x, y + i, z)) {
				_chunk->setVoxel(x - _validRegion.getLowerX(), y - _validRegion.getLowerY(), z - _validRegion.getLowerZ(), voxels[y]);
			} else {
				_pagedVolume->setVoxel(x, y, z, voxels[y]);
			}
		}
		return true;
	}
};

}
