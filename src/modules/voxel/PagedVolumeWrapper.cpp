/**
 * @file
 */

#include "PagedVolumeWrapper.h"

#include <utility>
#include "core/Common.h"
#include "Morton.h"

namespace voxel {

PagedVolumeWrapper::Sampler::Sampler(const PagedVolumeWrapper* volume) :
		Super(volume->volume()), _chunk(volume->_chunk) {
}

PagedVolumeWrapper::Sampler::Sampler(const PagedVolumeWrapper& volume) :
		Super(volume.volume()), _chunk(volume._chunk) {
}

void PagedVolumeWrapper::Sampler::setPosition(int32_t xPos, int32_t yPos, int32_t zPos) {
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

	const glm::ivec3& p = _chunk->_chunkSpacePosition;
	if (p.x == xChunk && p.y == yChunk && p.z == zChunk) {
		_currentChunk = _chunk;
	} else {
		_currentChunk = _volume->chunk(xChunk, yChunk, zChunk);
	}

	_currentVoxel = _currentChunk->_data + voxelIndexInChunk;
}

PagedVolumeWrapper::PagedVolumeWrapper(PagedVolume* voxelStorage, PagedVolume::ChunkPtr chunk, const Region& region) :
		_pagedVolume(voxelStorage), _chunk(std::move(chunk)), _region(region) {
	if (_chunk != nullptr) {
		_validRegion = _chunk->region();
	}
}

const Voxel& PagedVolumeWrapper::voxel(int x, int y, int z) const {
	if (_validRegion.containsPoint(x, y, z)) {
		core_assert(_chunk != nullptr);
		const int relX = x - _validRegion.getLowerX();
		const int relY = y - _validRegion.getLowerY();
		const int relZ = z - _validRegion.getLowerZ();
		return _chunk->voxel(relX, relY, relZ);
	}
	core_assert(_pagedVolume != nullptr);
	return _pagedVolume->voxel(x, y, z);
}

bool PagedVolumeWrapper::setVoxel(int x, int y, int z, const Voxel& voxel) {
	if (_validRegion.containsPoint(x, y, z)) {
		core_assert(_chunk != nullptr);
		const int relX = x - _validRegion.getLowerX();
		const int relY = y - _validRegion.getLowerY();
		const int relZ = z - _validRegion.getLowerZ();
		_chunk->setVoxel(relX, relY, relZ, voxel);
		return true;
	}
	core_assert(_pagedVolume != nullptr);
	_pagedVolume->setVoxel(x, y, z, voxel);
	return true;
}

bool PagedVolumeWrapper::setVoxels(int x, int y, int z, int nx, int nz, const Voxel* voxels, int amount) {
	for (int j = 0; j < nx; ++j) {
		for (int k = 0; k < nz; ++k) {
			const int fx = x + j;
			const int fz = z + k;
			int left = amount;
			if (_validRegion.containsPoint(fx, y, fz)) {
				// first part goes into the chunk
				const int h = _validRegion.getHeightInVoxels();
				_chunk->setVoxels(fx - _validRegion.getLowerX(), y - _validRegion.getLowerY(), fz - _validRegion.getLowerZ(), voxels, core_min(h, left));
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

}
