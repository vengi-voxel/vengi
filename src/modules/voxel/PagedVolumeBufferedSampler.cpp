/**
 * @file
 */

#include "PagedVolume.h"
#include "core/Log.h"
#include "Morton.h"
#include "core/TimeProvider.h"

namespace voxel {

PagedVolume::BufferedSampler::BufferedSampler(const PagedVolume* volume, const Region& region) {
	const int32_t chunkMask = volume->_chunkMask;
	const uint8_t chunkSideLengthPower = volume->_chunkSideLengthPower;
	Region r = region;
	r.grow(1);
	_minsX = r.getLowerX();
	_minsY = r.getLowerY();
	_minsZ = r.getLowerZ();
	_regionWidth = r.getWidthInVoxels();
	_regionHeight = r.getHeightInVoxels();
	_regionDepth = r.getDepthInVoxels();
	_zOffset = _regionWidth * _regionHeight;

	_buffer.reserve(_zOffset * _regionDepth);
	const glm::ivec3& offset = r.getLowerCorner();
	const glm::ivec3& upper = r.getUpperCorner();
	ChunkPtr chunk;

	std::unordered_map<glm::ivec3, ChunkPtr> chunks;

	core::RecursiveScopedReadLock readLock(volume->_volumeLock);
	glm::ivec3 chunkPos((std::numeric_limits<int>::min)()), newChunkPos;
	for (int32_t z = offset.z; z <= upper.z; ++z) {
		const uint32_t regZ = z - offset.z;
		const uint16_t zOffset = static_cast<uint16_t>(z & chunkMask);
		newChunkPos.z = z >> chunkSideLengthPower;

		for (int32_t y = offset.y; y <= upper.y; ++y) {
			const uint32_t regY = y - offset.y;
			const uint16_t yOffset = static_cast<uint16_t>(y & chunkMask);
			newChunkPos.y = y >> chunkSideLengthPower;

			int vecIndex = index(0, regY, regZ);
			for (int32_t x = offset.x; x <= upper.x; ++x, ++vecIndex) {
				newChunkPos.x = x >> chunkSideLengthPower;
				if (chunkPos != newChunkPos) {
					chunkPos = newChunkPos;
					auto i = chunks.find(chunkPos);
					if (i != chunks.end()) {
						chunk = i->second;
					} else {
						chunk = volume->chunk(chunkPos.x, chunkPos.y, chunkPos.z);
						chunks[chunkPos] = chunk;
					}
				}

				const uint16_t xOffset = static_cast<uint16_t>(x & chunkMask);
				const uint32_t index = morton256_x[xOffset] | morton256_y[yOffset] | morton256_z[zOffset];
				_buffer[vecIndex] = chunk->_data[index];
			}
		}
	}
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
