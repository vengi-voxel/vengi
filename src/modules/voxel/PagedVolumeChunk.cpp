/**
 * @file
 */

#include "PagedVolume.h"
#include "Morton.h"
#include "math/Functions.h"
#include "core/Common.h"
#include "core/StandardLib.h"

namespace voxel {

PagedVolume::Chunk::Chunk(const glm::ivec3& pos, uint16_t sideLength, Pager* pager) :
		_pager(pager), _chunkSpacePosition(pos) {
	core_assert_msg(_pager, "No valid pager supplied to chunk constructor.");
	core_assert_msg(sideLength <= 256, "Chunk side length cannot be greater than 256.");

	// Compute the side length
	_sideLength = sideLength;
	_sideLengthPower = math::logBase2(sideLength);

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

bool PagedVolume::Chunk::setData(const Voxel* voxels, size_t sizeInBytes) {
	if (sizeInBytes != dataSizeInBytes()) {
		return false;
	}
	_dataModified = true;
	core_memcpy((uint8_t*)_data, (const uint8_t*)voxels, sizeInBytes);
	return true;
}

Voxel* PagedVolume::Chunk::data() const {
	return _data;
}

uint32_t PagedVolume::Chunk::dataSizeInBytes() const {
	return voxels() * sizeof(Voxel);
}

uint32_t PagedVolume::Chunk::voxels() const {
	return _sideLength * _sideLength * _sideLength;
}

const Voxel& PagedVolume::Chunk::voxel(uint32_t x, uint32_t y, uint32_t z) const {
	// This code is not usually expected to be called by the user, with the exception of when implementing paging
	// of uncompressed data. It's a performance critical code path
	core_assert_msg(x < _sideLength, "Supplied position is outside of the chunk. asserted %u > %u", x, _sideLength);
	core_assert_msg(y < _sideLength, "Supplied position is outside of the chunk. asserted %u > %u", y, _sideLength);
	core_assert_msg(z < _sideLength, "Supplied position is outside of the chunk. asserted %u > %u", z, _sideLength);
	core_assert_msg(_data, "No uncompressed data - chunk must be decompressed before accessing voxels.");

	const uint32_t index = morton256_x[x] | morton256_y[y] | morton256_z[z];
	return _data[index];
}

const Voxel& PagedVolume::Chunk::voxel(const glm::i16vec3& pos) const {
	return voxel(pos.x, pos.y, pos.z);
}

void PagedVolume::Chunk::setVoxel(uint32_t x, uint32_t y, uint32_t z, const Voxel& value) {
	// This code is not usually expected to be called by the user, with the exception of when implementing paging
	// of uncompressed data. It's a performance critical code path
	core_assert_msg(x < _sideLength, "Supplied position is outside of the chunk");
	core_assert_msg(y < _sideLength, "Supplied position is outside of the chunk");
	core_assert_msg(z < _sideLength, "Supplied position is outside of the chunk");
	core_assert_msg(_data, "No uncompressed data - chunk must be decompressed before accessing voxels.");

	const uint32_t index = morton256_x[x] | morton256_y[y] | morton256_z[z];
	_data[index] = value;
	_dataModified = true;
}

void PagedVolume::Chunk::setVoxels(uint32_t x, uint32_t z, const Voxel* values, int amount) {
	setVoxels(x, 0, z, values, amount);
}

void PagedVolume::Chunk::setVoxels(uint32_t x, uint32_t y, uint32_t z, const Voxel* values, int amount) {
	// This code is not usually expected to be called by the user, with the exception of when implementing paging
	// of uncompressed data. It's a performance critical code path
	core_assert_msg(amount <= _sideLength, "Supplied amount exceeds chunk boundaries");
	core_assert_msg(x < _sideLength, "Supplied x position is outside of the chunk");
	core_assert_msg(y < _sideLength, "Supplied y position is outside of the chunk");
	core_assert_msg(z < _sideLength, "Supplied z position is outside of the chunk");
	core_assert_msg(_data, "No uncompressed data - chunk must be decompressed before accessing voxels.");

	for (int i = y; i < amount; ++i) {
		const uint32_t index = morton256_x[x] | morton256_y[i] | morton256_z[z];
		_data[index] = values[i];
	}
	_dataModified = true;
}

int16_t PagedVolume::Chunk::sideLength() const {
	return _sideLength;
}

const glm::ivec3& PagedVolume::Chunk::chunkPos() const {
	return _chunkSpacePosition;
}

void PagedVolume::Chunk::setVoxel(const glm::i16vec3& pos, const Voxel& value) {
	setVoxel(pos.x, pos.y, pos.z, value);
}

uint32_t PagedVolume::Chunk::calculateSizeInBytes(uint32_t sideLength) {
	// Note: We disregard the size of the other class members as they are likely to be very small compared to the size of the
	// allocated voxel data. This also keeps the reported size as a power of two, which makes other memory calculations easier.
	const uint32_t sizeInBytes = sideLength * sideLength * sideLength * sizeof(Voxel);
	return sizeInBytes;
}

}
