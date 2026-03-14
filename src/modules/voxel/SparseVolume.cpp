/**
 * @file
 */

#include "SparseVolume.h"

namespace voxel {

SparseVolume::SparseVolume(const voxel::Region &region) : _region(region), _isRegionValid(_region.isValid()) {
}

SparseVolume::Chunk *SparseVolume::findChunk(const glm::ivec3 &chunkPos) const {
	if (_cachedChunkPosition == chunkPos && _cachedChunkPtr) {
		return _cachedChunkPtr.get();
	}
	const auto iter = _chunks.find(chunkPos);
	if (iter != _chunks.end()) {
		_cachedChunkPtr = iter->value;
		_cachedChunkPosition = chunkPos;
		return iter->value.get();
	}
	return nullptr;
}

SparseVolume::ChunkPtr SparseVolume::findOrCreateChunk(const glm::ivec3 &chunkPos) {
	if (_cachedChunkPosition == chunkPos && _cachedChunkPtr) {
		return _cachedChunkPtr;
	}
	const auto iter = _chunks.find(chunkPos);
	if (iter != _chunks.end()) {
		_cachedChunkPtr = iter->value;
		_cachedChunkPosition = chunkPos;
		return iter->value;
	}
	ChunkPtr chunk = core::make_shared<Chunk>();
	_chunks.put(chunkPos, chunk);
	_cachedChunkPtr = chunk;
	_cachedChunkPosition = chunkPos;
	return chunk;
}

void SparseVolume::removeChunkIfEmpty(const glm::ivec3 &chunkPos, Chunk *chunk) {
	if (chunk->voxels.empty()) {
		if (_cachedChunkPosition == chunkPos) {
			_cachedChunkPtr = ChunkPtr();
			_cachedChunkPosition = glm::ivec3(INT32_MAX, INT32_MAX, INT32_MAX);
		}
		_chunks.remove(chunkPos);
	}
}

bool SparseVolume::setVoxel(const glm::ivec3 &pos, const voxel::Voxel &voxel) {
	if (_isRegionValid && !_region.containsPoint(pos)) {
		return false;
	}

	const glm::ivec3 &chunkPos = chunkPosition(pos);
	const glm::u8vec3 &localPos = localPosition(pos, chunkPos);
	const uint32_t packedLocalPos = packLocal(localPos);
	const bool isEmptyVoxel = !_storeEmptyVoxels && isAir(voxel.getMaterial());

	if (isEmptyVoxel) {
		Chunk *chunk = findChunk(chunkPos);
		if (!chunk) {
			return true;
		}
		if (chunk->voxels.remove(packedLocalPos)) {
			--_size;
		}
		removeChunkIfEmpty(chunkPos, chunk);
		return true;
	}

	ChunkPtr chunkPtr = findOrCreateChunk(chunkPos);
	Chunk *chunk = chunkPtr.get();
	const size_t sizeBefore = chunk->voxels.size();
	chunk->voxels.put(packedLocalPos, voxel);
	if (chunk->voxels.size() > sizeBefore) {
		++_size;
	}
	return true;
}

void SparseVolume::setVoxelsRow(int x, int y, int z, int count, const Voxel &voxel) {
	const bool isEmptyVoxel = !_storeEmptyVoxels && isAir(voxel.getMaterial());

	if (isEmptyVoxel) {
		for (int i = 0; i < count; ++i) {
			setVoxel(x + i, y, z, voxel);
		}
		return;
	}

	const int zChunk = z >> 8;
	const uint8_t zLocal = z & 0xFF;
	const int yChunk = y >> 8;
	const uint8_t yLocal = y & 0xFF;

	int processed = 0;
	while (processed < count) {
		const int currentX = x + processed;
		const int xChunk = currentX >> 8;
		const uint8_t xLocal = currentX & 0xFF;

		const glm::ivec3 chunkPos = {xChunk, yChunk, zChunk};

		ChunkPtr chunkPtr = findOrCreateChunk(chunkPos);
		Chunk *chunk = chunkPtr.get();

		const int chunkEndX = ((xChunk + 1) << 8);
		const int rowEndX = x + count;
		const int processUntilX = (rowEndX < chunkEndX) ? rowEndX : chunkEndX;
		const int voxelsInChunk = processUntilX - currentX;

		const size_t sizeBefore = chunk->voxels.size();

		for (int i = 0; i < voxelsInChunk; ++i) {
			const uint32_t packedLocal = (uint32_t(xLocal + i) << 16) | (uint32_t(yLocal) << 8) | zLocal;
			chunk->voxels.put(packedLocal, voxel);
		}

		const size_t sizeAfter = chunk->voxels.size();
		const int newCount = (int)(sizeAfter - sizeBefore);

		if (newCount > 0) {
			_size += newCount;
		}

		processed += voxelsInChunk;
	}
}

const Voxel &SparseVolume::voxel(const glm::ivec3 &pos) const {
	if (_isRegionValid && !_region.containsPoint(pos)) {
		return _emptyVoxel;
	}
	const glm::ivec3 &chunkPos = chunkPosition(pos);
	const glm::u8vec3 &localPos = localPosition(pos, chunkPos);
	const uint32_t packedLocalPos = packLocal(localPos);

	const Chunk *chunk = findChunk(chunkPos);
	if (!chunk) {
		return _emptyVoxel;
	}

	const auto iter = chunk->voxels.find(packedLocalPos);
	if (iter != chunk->voxels.end()) {
		return iter->second;
	}
	return _emptyVoxel;
}

bool SparseVolume::hasVoxel(const glm::ivec3 &pos) const {
	if (_isRegionValid && !_region.containsPoint(pos)) {
		return false;
	}
	const glm::ivec3 &chunkPos = chunkPosition(pos);
	const glm::u8vec3 &localPos = localPosition(pos, chunkPos);
	const uint32_t packedLocalPos = packLocal(localPos);

	const Chunk *chunk = findChunk(chunkPos);
	if (!chunk) {
		return false;
	}

	return chunk->voxels.hasKey(packedLocalPos);
}

const Voxel &SparseVolume::voxel(int32_t x, int32_t y, int32_t z) const {
	return voxel(glm::ivec3(x, y, z));
}

bool SparseVolume::hasVoxel(int x, int y, int z) const {
	return hasVoxel(glm::ivec3(x, y, z));
}

void SparseVolume::clear() {
	_chunks.clear();
	_size = 0;
	_cachedChunkPtr = ChunkPtr();
	_cachedChunkPosition = glm::ivec3(INT32_MAX, INT32_MAX, INT32_MAX);
}

SparseVolume::Sampler::Sampler(const SparseVolume *volume) : _volume(const_cast<SparseVolume *>(volume)) {
}

SparseVolume::Sampler::Sampler(const SparseVolume &volume) : _volume(const_cast<SparseVolume *>(&volume)) {
}

SparseVolume::Sampler::~Sampler() {
}

void SparseVolume::Sampler::updateChunkCache() const {
	const glm::ivec3 chunkPos = SparseVolume::chunkPosition(_posInVolume);
	if (_cachedChunk && _cachedChunkPos == chunkPos) {
		const glm::u8vec3 localPos = SparseVolume::localPosition(_posInVolume, chunkPos);
		_cachedPackedLocal = SparseVolume::packLocal(localPos);
		return;
	}
	_cachedChunk = _volume->findChunk(chunkPos);
	_cachedChunkPos = chunkPos;
	const glm::u8vec3 localPos = SparseVolume::localPosition(_posInVolume, chunkPos);
	_cachedPackedLocal = SparseVolume::packLocal(localPos);
}

void SparseVolume::Sampler::invalidateChunkCache() {
	_cachedChunk = nullptr;
}

bool SparseVolume::Sampler::setVoxel(const Voxel &voxel) {
	if (_currentPositionInvalid) {
		return false;
	}

	// Fast path: use cached chunk if valid
	if (_cachedChunk) {
		const glm::ivec3 currentChunkPos = SparseVolume::chunkPosition(_posInVolume);
		if (currentChunkPos == _cachedChunkPos) {
			const glm::u8vec3 localPos = SparseVolume::localPosition(_posInVolume, _cachedChunkPos);
			const uint32_t packedLocalPos = SparseVolume::packLocal(localPos);
			const bool isEmptyVoxel = !_volume->_storeEmptyVoxels && isAir(voxel.getMaterial());

			if (isEmptyVoxel) {
				if (_cachedChunk->voxels.remove(packedLocalPos)) {
					--_volume->_size;
				}
				_currentVoxel = SparseVolume::_emptyVoxel;
			} else {
				const auto iter = _cachedChunk->voxels.find(packedLocalPos);
				const bool hadVoxel = iter != _cachedChunk->voxels.end();
				_cachedChunk->voxels.put(packedLocalPos, voxel);
				if (!hadVoxel) {
					++_volume->_size;
				}
				_currentVoxel = voxel;
			}
			return true;
		}
	}

	// Slow path: fall back to volume setVoxel
	_volume->setVoxel(_posInVolume, voxel);
	_currentVoxel = _volume->voxel(_posInVolume);
	invalidateChunkCache();
	return true;
}

bool SparseVolume::Sampler::setPosition(int32_t xPos, int32_t yPos, int32_t zPos) {
	_posInVolume.x = xPos;
	_posInVolume.y = yPos;
	_posInVolume.z = zPos;

	const voxel::Region &region = this->region();
	_currentPositionInvalid = 0u;
	if (region.isValid()) {
		if (!region.containsPointInX(xPos)) {
			_currentPositionInvalid |= SAMPLER_INVALIDX;
		}
		if (!region.containsPointInY(yPos)) {
			_currentPositionInvalid |= SAMPLER_INVALIDY;
		}
		if (!region.containsPointInZ(zPos)) {
			_currentPositionInvalid |= SAMPLER_INVALIDZ;
		}
	}

	if (currentPositionValid()) {
		updateChunkCache();
		if (_cachedChunk) {
			const auto iter = _cachedChunk->voxels.find(_cachedPackedLocal);
			if (iter != _cachedChunk->voxels.end()) {
				_currentVoxel = iter->second;
			} else {
				_currentVoxel = SparseVolume::_emptyVoxel;
			}
		} else {
			_currentVoxel = SparseVolume::_emptyVoxel;
		}
		return true;
	}
	return false;
}

void SparseVolume::Sampler::movePositive(math::Axis axis, uint32_t offset) {
	switch (axis) {
	case math::Axis::X:
		movePositiveX(offset);
		break;
	case math::Axis::Y:
		movePositiveY(offset);
		break;
	case math::Axis::Z:
		movePositiveZ(offset);
		break;
	default:
		break;
	}
}

void SparseVolume::Sampler::movePositiveX(uint32_t offset) {
	const bool bIsOldPositionValid = currentPositionValid();

	_posInVolume.x += (int)offset;

	if (region().isValid()) {
		if (!region().containsPointInX(_posInVolume.x)) {
			_currentPositionInvalid |= SAMPLER_INVALIDX;
		} else {
			_currentPositionInvalid &= ~SAMPLER_INVALIDX;
		}
	}

	if (!bIsOldPositionValid) {
		setPosition(_posInVolume);
	} else if (currentPositionValid()) {
		updateChunkCache();
		if (_cachedChunk) {
			const auto iter = _cachedChunk->voxels.find(_cachedPackedLocal);
			if (iter != _cachedChunk->voxels.end()) {
				_currentVoxel = iter->second;
			} else {
				_currentVoxel = SparseVolume::_emptyVoxel;
			}
		} else {
			_currentVoxel = SparseVolume::_emptyVoxel;
		}
	}
}

void SparseVolume::Sampler::movePositiveY(uint32_t offset) {
	const bool bIsOldPositionValid = currentPositionValid();

	_posInVolume.y += (int)offset;

	if (region().isValid()) {
		if (!region().containsPointInY(_posInVolume.y)) {
			_currentPositionInvalid |= SAMPLER_INVALIDY;
		} else {
			_currentPositionInvalid &= ~SAMPLER_INVALIDY;
		}
	}

	if (!bIsOldPositionValid) {
		setPosition(_posInVolume);
	} else if (currentPositionValid()) {
		updateChunkCache();
		if (_cachedChunk) {
			const auto iter = _cachedChunk->voxels.find(_cachedPackedLocal);
			if (iter != _cachedChunk->voxels.end()) {
				_currentVoxel = iter->second;
			} else {
				_currentVoxel = SparseVolume::_emptyVoxel;
			}
		} else {
			_currentVoxel = SparseVolume::_emptyVoxel;
		}
	}
}

void SparseVolume::Sampler::movePositiveZ(uint32_t offset) {
	const bool bIsOldPositionValid = currentPositionValid();

	_posInVolume.z += (int)offset;

	if (region().isValid()) {
		if (!region().containsPointInZ(_posInVolume.z)) {
			_currentPositionInvalid |= SAMPLER_INVALIDZ;
		} else {
			_currentPositionInvalid &= ~SAMPLER_INVALIDZ;
		}
	}

	if (!bIsOldPositionValid) {
		setPosition(_posInVolume);
	} else if (currentPositionValid()) {
		_currentVoxel = _volume->voxel(_posInVolume);
	}
}

void SparseVolume::Sampler::moveNegative(math::Axis axis, uint32_t offset) {
	switch (axis) {
	case math::Axis::X:
		moveNegativeX(offset);
		break;
	case math::Axis::Y:
		moveNegativeY(offset);
		break;
	case math::Axis::Z:
		moveNegativeZ(offset);
		break;
	default:
		break;
	}
}

void SparseVolume::Sampler::moveNegativeX(uint32_t offset) {
	const bool bIsOldPositionValid = currentPositionValid();

	_posInVolume.x -= (int)offset;

	if (region().isValid()) {
		if (!region().containsPointInX(_posInVolume.x)) {
			_currentPositionInvalid |= SAMPLER_INVALIDX;
		} else {
			_currentPositionInvalid &= ~SAMPLER_INVALIDX;
		}
	}

	if (!bIsOldPositionValid) {
		setPosition(_posInVolume);
	} else if (currentPositionValid()) {
		_currentVoxel = _volume->voxel(_posInVolume);
	}
}

void SparseVolume::Sampler::moveNegativeY(uint32_t offset) {
	const bool bIsOldPositionValid = currentPositionValid();

	_posInVolume.y -= (int)offset;

	if (region().isValid()) {
		if (!region().containsPointInY(_posInVolume.y)) {
			_currentPositionInvalid |= SAMPLER_INVALIDY;
		} else {
			_currentPositionInvalid &= ~SAMPLER_INVALIDY;
		}
	}

	if (!bIsOldPositionValid) {
		setPosition(_posInVolume);
	} else if (currentPositionValid()) {
		_currentVoxel = _volume->voxel(_posInVolume);
	}
}

void SparseVolume::Sampler::moveNegativeZ(uint32_t offset) {
	const bool bIsOldPositionValid = currentPositionValid();

	_posInVolume.z -= (int)offset;

	if (region().isValid()) {
		if (!region().containsPointInZ(_posInVolume.z)) {
			_currentPositionInvalid |= SAMPLER_INVALIDZ;
		} else {
			_currentPositionInvalid &= ~SAMPLER_INVALIDZ;
		}
	}

	if (!bIsOldPositionValid) {
		setPosition(_posInVolume);
	} else if (currentPositionValid()) {
		_currentVoxel = _volume->voxel(_posInVolume);
	}
}

Region SparseVolume::calculateRegion() const {
	if (_size == 0) {
		return Region::InvalidRegion;
	}

	glm::aligned_ivec4 mins(0);
	glm::aligned_ivec4 maxs(0);
	bool initialized = false;
	for (auto chunkIter = _chunks.begin(); chunkIter != _chunks.end(); ++chunkIter) {
		const glm::ivec3 &chunkPos = chunkIter->key;
		const ChunkPtr &chunk = chunkIter->value;
		if (!chunk) {
			continue;
		}
		for (auto voxelIter = chunk->voxels.begin(); voxelIter != chunk->voxels.end(); ++voxelIter) {
			const glm::ivec3 &pos = worldPosition(chunkPos, voxelIter->key);
			const glm::aligned_ivec4 aligned(pos, 0);
			if (!initialized) {
				mins = aligned;
				maxs = aligned;
				initialized = true;
				continue;
			}
			maxs = (glm::max)(maxs, aligned);
			mins = (glm::min)(mins, aligned);
		}
	}
	if (!initialized) {
		return Region::InvalidRegion;
	}
	return voxel::Region{mins, maxs};
}

} // namespace voxel
