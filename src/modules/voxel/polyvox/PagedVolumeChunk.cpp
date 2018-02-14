/**
 * @file
 */

#include "PagedVolume.h"
#include "Morton.h"
#include "Utility.h"
#include "core/Common.h"

namespace voxel {

PagedVolume::Chunk::Chunk(const glm::ivec3& v3dPosition, uint16_t uSideLength, Pager* pPager) :
		_pager(pPager), _chunkSpacePosition(v3dPosition) {
	core_assert_msg(_pager, "No valid pager supplied to chunk constructor.");
	core_assert_msg(uSideLength <= 256, "Chunk side length cannot be greater than 256.");

	// Compute the side length
	_sideLength = uSideLength;
	_sideLengthPower = logBase2(uSideLength);

	// Allocate the data
	const uint32_t uNoOfVoxels = _sideLength * _sideLength * _sideLength;
	_data = (Voxel*)core_malloc(uNoOfVoxels * sizeof(Voxel));
	core_memset(_data, 0, uNoOfVoxels * sizeof(Voxel));
}

PagedVolume::Chunk::~Chunk() {
	if (_dataModified && _pager) {
		_pager->pageOut(this);
	}

	core_free(_data);
	_data = nullptr;
}

Voxel* PagedVolume::Chunk::data() const {
	return _data;
}

uint32_t PagedVolume::Chunk::dataSizeInBytes() const {
	return _sideLength * _sideLength * _sideLength * sizeof(Voxel);
}

const Voxel& PagedVolume::Chunk::voxel(uint32_t uXPos, uint32_t uYPos, uint32_t uZPos) const {
	// This code is not usually expected to be called by the user, with the exception of when implementing paging
	// of uncompressed data. It's a performance critical code path
	core_assert_msg(uXPos < _sideLength, "Supplied position is outside of the chunk. asserted %u > %u", uXPos, _sideLength);
	core_assert_msg(uYPos < _sideLength, "Supplied position is outside of the chunk. asserted %u > %u", uYPos, _sideLength);
	core_assert_msg(uZPos < _sideLength, "Supplied position is outside of the chunk. asserted %u > %u", uZPos, _sideLength);
	core_assert_msg(_data, "No uncompressed data - chunk must be decompressed before accessing voxels.");

	const uint32_t index = morton256_x[uXPos] | morton256_y[uYPos] | morton256_z[uZPos];
	core::RecursiveScopedReadLock readLock(_rwLock);
	return _data[index];
}

const Voxel& PagedVolume::Chunk::voxel(const glm::i16vec3& v3dPos) const {
	return voxel(v3dPos.x, v3dPos.y, v3dPos.z);
}

void PagedVolume::Chunk::setVoxel(uint32_t uXPos, uint32_t uYPos, uint32_t uZPos, const Voxel& tValue) {
	// This code is not usually expected to be called by the user, with the exception of when implementing paging
	// of uncompressed data. It's a performance critical code path
	core_assert_msg(uXPos < _sideLength, "Supplied position is outside of the chunk");
	core_assert_msg(uYPos < _sideLength, "Supplied position is outside of the chunk");
	core_assert_msg(uZPos < _sideLength, "Supplied position is outside of the chunk");
	core_assert_msg(_data, "No uncompressed data - chunk must be decompressed before accessing voxels.");

	const uint32_t index = morton256_x[uXPos] | morton256_y[uYPos] | morton256_z[uZPos];
	core::RecursiveScopedWriteLock writeLock(_rwLock);
	_data[index] = tValue;
	_dataModified = true;
}

void PagedVolume::Chunk::setVoxels(uint32_t uXPos, uint32_t uZPos, const Voxel* tValues, int amount) {
	setVoxels(uXPos, 0, uZPos, tValues, amount);
}

void PagedVolume::Chunk::setVoxels(uint32_t uXPos, uint32_t uYPos, uint32_t uZPos, const Voxel* tValues, int amount) {
	// This code is not usually expected to be called by the user, with the exception of when implementing paging
	// of uncompressed data. It's a performance critical code path
	core_assert_msg(amount <= _sideLength, "Supplied amount exceeds chunk boundaries");
	core_assert_msg(uXPos < _sideLength, "Supplied x position is outside of the chunk");
	core_assert_msg(uYPos < _sideLength, "Supplied y position is outside of the chunk");
	core_assert_msg(uZPos < _sideLength, "Supplied z position is outside of the chunk");
	core_assert_msg(_data, "No uncompressed data - chunk must be decompressed before accessing voxels.");

	core::RecursiveScopedWriteLock writeLock(_rwLock);
	for (int y = uYPos; y < amount; ++y) {
		const uint32_t index = morton256_x[uXPos] | morton256_y[y] | morton256_z[uZPos];
		_data[index] = tValues[y];
	}
	_dataModified = true;
}

void PagedVolume::Chunk::setVoxel(const glm::i16vec3& v3dPos, const Voxel& tValue) {
	setVoxel(v3dPos.x, v3dPos.y, v3dPos.z, tValue);
}

uint32_t PagedVolume::Chunk::calculateSizeInBytes() const {
	// Call through to the static version
	return calculateSizeInBytes(_sideLength);
}

uint32_t PagedVolume::Chunk::calculateSizeInBytes(uint32_t uSideLength) {
	// Note: We disregard the size of the other class members as they are likely to be very small compared to the size of the
	// allocated voxel data. This also keeps the reported size as a power of two, which makes other memory calculations easier.
	const uint32_t uSizeInBytes = uSideLength * uSideLength * uSideLength * sizeof(Voxel);
	return uSizeInBytes;
}

}
