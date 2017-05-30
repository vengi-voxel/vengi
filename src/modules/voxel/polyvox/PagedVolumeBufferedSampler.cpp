/**
 * @file
 */

#include "PagedVolume.h"
#include "core/Log.h"
#include "Utility.h"
#include "core/TimeProvider.h"

namespace voxel {

PagedVolume::BufferedSampler::BufferedSampler(const PagedVolume* volume, const Region& region) :
		_region(region), _chunkSideLengthPower(volume->_chunkSideLengthPower), _chunkMask(volume->_chunkMask) {
	const int border = 1;
	_region.grow(border);
	_minsX = _region.getLowerX();
	_minsY = _region.getLowerY();
	_minsZ = _region.getLowerZ();
	_regionWidth = _region.getWidthInVoxels();
	_regionHeight = _region.getHeightInVoxels();
	_regionDepth = _region.getDepthInVoxels();

	_buffer.reserve(_regionWidth * _regionHeight * _regionDepth);
	const glm::ivec3& offset = _region.getLowerCorner();
	const glm::ivec3& upper = _region.getUpperCorner();
	ChunkPtr chunk;
	int32_t xChunk = 0;
	int32_t yChunk = 0;
	int32_t zChunk = 0;

	core::RecursiveScopedReadLock readLock(volume->_rwLock);
	for (int32_t z = offset.z; z <= upper.z; ++z) {
		const uint32_t regZ = z - offset.z;

		for (int32_t y = offset.y; y <= upper.y; ++y) {
			const uint32_t regY = y - offset.y;

			for (int32_t x = offset.x; x <= upper.x; ++x) {
				const uint32_t regX = x - offset.x;
				const int vecIndex = index(regX, regY, regZ);

				const int nxChunk = x >> _chunkSideLengthPower;
				const int nyChunk = y >> _chunkSideLengthPower;
				const int nzChunk = z >> _chunkSideLengthPower;
				if (nullptr == chunk.get() || xChunk != nxChunk || yChunk != nyChunk || zChunk != nzChunk) {
					chunk = volume->chunk(nxChunk, nyChunk, nzChunk);
					xChunk = nxChunk;
					yChunk = nyChunk;
					zChunk = nzChunk;
				}

				const uint16_t xOffset = static_cast<uint16_t>(x & _chunkMask);
				const uint16_t yOffset = static_cast<uint16_t>(y & _chunkMask);
				const uint16_t zOffset = static_cast<uint16_t>(z & _chunkMask);
				_buffer[vecIndex] = chunk->voxel(xOffset, yOffset, zOffset);
			}
		}
	}
	_region.shrink(border);
}

PagedVolume::BufferedSampler::BufferedSampler(const PagedVolume& volume, const Region& region) :
		BufferedSampler(&volume, region) {
}

PagedVolume::BufferedSampler::~BufferedSampler() {
}

bool PagedVolume::BufferedSampler::setPosition(int32_t xPos, int32_t yPos, int32_t zPos) {
	_xPosInVolume = xPos;
	_yPosInVolume = yPos;
	_zPosInVolume = zPos;
	_xPosInBuffer = xPos - _minsX;
	_yPosInBuffer = yPos - _minsY;
	_zPosInBuffer = zPos - _minsZ;

	_currentVoxel = &_buffer[index(_xPosInBuffer, _yPosInBuffer, _zPosInBuffer)];
	return true;
}

bool PagedVolume::BufferedSampler::setVoxel(const Voxel& tValue) {
	if (_currentVoxel == nullptr) {
		return false;
	}
	*_currentVoxel = tValue;
	return true;
}

}
