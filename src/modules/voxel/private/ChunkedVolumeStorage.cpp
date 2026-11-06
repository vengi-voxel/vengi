/**
 * @file
 */

#include "ChunkedVolumeStorage.h"
#include "core/Assert.h"
#include "core/StandardLib.h"
#include <new>
#include <glm/common.hpp>

namespace voxel {

ChunkedVolumeStorage::ChunkedVolumeStorage(const glm::ivec3 &dimensions) : _dimensions(dimensions) {
	core_assert(glm::all(glm::greaterThan(dimensions, glm::ivec3(0))));
	_chunkDimensions = glm::min(dimensions, glm::ivec3(ChunkSide));
	// Subtract before rounding up to avoid overflowing dimensions near INT_MAX.
	_chunkCounts = (dimensions - 1) / _chunkDimensions + 1;
	_directorySize = 1;
	_chunkVoxels = 1;
	for (int axis = 0; axis < 3; ++axis) {
		if (_directorySize > SIZE_MAX / (size_t)_chunkCounts[axis]) {
			_directorySize = 0;
			return;
		}
		_directorySize *= (size_t)_chunkCounts[axis];
		_chunkVoxels *= (size_t)_chunkDimensions[axis];
	}
	if (_directorySize > SIZE_MAX / sizeof(void *)) {
		_directorySize = 0;
		return;
	}
	// Use native atomic loads in the sampler hot path, avoiding an out-of-line
	// SDL call per voxel. Publication still synchronizes chunk initialization.
	static_assert(std::atomic<Voxel *>::is_always_lock_free, "Chunk pointers must be lock-free");
	static_assert(sizeof(std::atomic<Voxel *>) == sizeof(void *), "Unexpected atomic pointer size");
	_chunks = (std::atomic<Voxel *> *)core_malloc(_directorySize * sizeof(*_chunks));
	if (_chunks) {
		for (size_t i = 0; i < _directorySize; ++i) {
			new (_chunks + i) std::atomic<Voxel *>(nullptr);
		}
	}
}

ChunkedVolumeStorage::ChunkedVolumeStorage(const ChunkedVolumeStorage &other)
	: ChunkedVolumeStorage(other._dimensions) {
	core_memcpy(&_default, &other._default, sizeof(Voxel));
	if (!valid() || !other.valid()) {
		core_free(_chunks);
		_chunks = nullptr;
		return;
	}
	for (size_t i = 0; i < _directorySize; ++i) {
		if (const Voxel *source = other.chunk(i)) {
			Voxel *destination = createChunk(i);
			if (destination == nullptr) {
				// A failed copy must not masquerade as a valid, partially empty volume.
				for (size_t j = 0; j < _directorySize; ++j) {
					core_free(chunk(j));
				}
				core_free(_chunks);
				_chunks = nullptr;
				return;
			}
			core_memcpy(destination, source, _chunkVoxels * sizeof(Voxel));
		}
	}
}

ChunkedVolumeStorage::~ChunkedVolumeStorage() {
	if (_chunks) {
		for (size_t i = 0; i < _directorySize; ++i) {
			core_free(chunk(i));
		}
		core_free(_chunks);
	}
}

bool ChunkedVolumeStorage::valid() const {
	return _chunks != nullptr;
}

size_t ChunkedVolumeStorage::chunkIndex(const glm::ivec3 &pos) const {
	const glm::ivec3 p = pos >> 5;
	return (size_t)p.x + (size_t)_chunkCounts.x * ((size_t)p.y + (size_t)_chunkCounts.y * (size_t)p.z);
}

size_t ChunkedVolumeStorage::voxelIndex(const glm::ivec3 &pos) const {
	const glm::ivec3 p = pos & (ChunkSide - 1);
	return (size_t)p.x + (size_t)_chunkDimensions.x * ((size_t)p.y + (size_t)_chunkDimensions.y * (size_t)p.z);
}

Voxel *ChunkedVolumeStorage::chunk(size_t index) const {
	return _chunks[index].load(std::memory_order_acquire);
}

Voxel *ChunkedVolumeStorage::createChunk(size_t index) {
	if (Voxel *existing = chunk(index)) {
		return existing;
	}
	Voxel *created = (Voxel *)core_malloc(_chunkVoxels * sizeof(Voxel));
	if (created == nullptr) {
		return nullptr;
	}
	uint32_t pattern;
	static_assert(sizeof(Voxel) == sizeof(pattern), "Expected a four-byte voxel");
	core_memcpy(&pattern, &_default, sizeof(pattern));
	core_memset4(created, pattern, _chunkVoxels);
	Voxel *expected = nullptr;
	if (!_chunks[index].compare_exchange_strong(expected, created, std::memory_order_acq_rel, std::memory_order_acquire)) {
		core_free(created);
		return chunk(index);
	}
	return created;
}

const Voxel &ChunkedVolumeStorage::voxel(const glm::ivec3 &localPos) const {
	core_assert(valid());
	core_assert(glm::all(glm::greaterThanEqual(localPos, glm::ivec3(0))) &&
				glm::all(glm::lessThan(localPos, _dimensions)));
	const Voxel *data = chunk(chunkIndex(localPos));
	return data ? data[voxelIndex(localPos)] : _default;
}

bool ChunkedVolumeStorage::setVoxel(const glm::ivec3 &localPos, const Voxel &value) {
	if (voxel(localPos) == value) {
		return false;
	}
	Voxel *data = createChunk(chunkIndex(localPos));
	if (data == nullptr) {
		return false;
	}
	data[voxelIndex(localPos)] = value;
	return true;
}

Voxel *ChunkedVolumeStorage::writableVoxel(const glm::ivec3 &localPos) {
	Voxel *data = createChunk(chunkIndex(localPos));
	return data ? data + voxelIndex(localPos) : nullptr;
}

const Voxel *ChunkedVolumeStorage::row(const glm::ivec3 &localPos, int &count) const {
	count = core_min(ChunkSide - (localPos.x & (ChunkSide - 1)), _dimensions.x - localPos.x);
	const Voxel *data = chunk(chunkIndex(localPos));
	return data ? data + voxelIndex(localPos) : nullptr;
}

Voxel *ChunkedVolumeStorage::sampleData(const glm::ivec3 &localPos, glm::ivec3 &dataLower, glm::ivec3 &dataUpper, int64_t &yStride, int64_t &zStride, std::atomic<Voxel *> *&chunkEntry, const Voxel *&defaultVoxel) const {
	dataLower = localPos & ~(ChunkSide - 1);
	dataUpper = glm::min(dataLower + (ChunkSide - 1), _dimensions - 1);
	yStride = _chunkDimensions.x;
	zStride = (int64_t)_chunkDimensions.x * _chunkDimensions.y;
	chunkEntry = &_chunks[chunkIndex(localPos)];
	defaultVoxel = &_default;
	Voxel *data = chunkEntry->load(std::memory_order_acquire);
	return data ? data + voxelIndex(localPos) : nullptr;
}

void ChunkedVolumeStorage::fill(const Voxel &value) {
	// Copy first: value may itself refer to an allocated cell.
	uint32_t pattern;
	core_memcpy(&pattern, &value, sizeof(pattern));
	core_memcpy(&_default, &pattern, sizeof(pattern));
	for (size_t i = 0; i < _directorySize; ++i) {
		if (Voxel *data = chunk(i)) {
			core_memset4(data, pattern, _chunkVoxels);
		}
	}
}

void ChunkedVolumeStorage::copyTo(Voxel *destination) const {
	for (int z = 0; z < _dimensions.z; ++z) {
		for (int y = 0; y < _dimensions.y; ++y) {
			for (int x = 0; x < _dimensions.x;) {
				const glm::ivec3 pos(x, y, z);
				const int count = core_min(_chunkDimensions.x - x % _chunkDimensions.x, _dimensions.x - x);
				const size_t offset = (size_t)x + (size_t)_dimensions.x * ((size_t)y + (size_t)_dimensions.y * (size_t)z);
				if (const Voxel *data = chunk(chunkIndex(pos))) {
					core_memcpy(destination + offset, data + voxelIndex(pos), count * sizeof(Voxel));
				} else {
					uint32_t pattern;
					core_memcpy(&pattern, &_default, sizeof(pattern));
					core_memset4(destination + offset, pattern, count);
				}
				x += count;
			}
		}
	}
}

size_t ChunkedVolumeStorage::allocatedBytes() const {
	if (!valid()) {
		return 0;
	}
	size_t bytes = _directorySize * sizeof(void *);
	for (size_t i = 0; i < _directorySize; ++i) {
		if (chunk(i)) {
			bytes += _chunkVoxels * sizeof(Voxel);
		}
	}
	return bytes;
}

} // namespace voxel
