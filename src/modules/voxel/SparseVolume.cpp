/**
 * @file
 */

#include "SparseVolume.h"

namespace voxel {

SparseVolume::SparseVolume(const voxel::Region &region) : _region(region), _isRegionValid(_region.isValid()) {
}

glm::ivec3 SparseVolume::chunkPosition(const glm::ivec3 &pos) {
	auto chunkCoord = [](int value) {
		if (value >= 0) {
			return value / ChunkSide;
		}
		const int absValue = -value;
		return -((absValue + ChunkMask) / ChunkSide);
	};
	return glm::ivec3(chunkCoord(pos.x), chunkCoord(pos.y), chunkCoord(pos.z));
}

glm::ivec3 SparseVolume::chunkBase(const glm::ivec3 &chunkPos) {
	return chunkPos * ChunkSide;
}

glm::u8vec3 SparseVolume::localPosition(const glm::ivec3 &pos, const glm::ivec3 &chunkPos) {
	const glm::ivec3 base = chunkBase(chunkPos);
	return glm::u8vec3(pos.x - base.x, pos.y - base.y, pos.z - base.z);
}

uint32_t SparseVolume::packLocal(const glm::u8vec3 &localPos) {
	return (uint32_t(localPos.x) << 16) | (uint32_t(localPos.y) << 8) | uint32_t(localPos.z);
}

glm::u8vec3 SparseVolume::unpackLocal(uint32_t packedLocalPos) {
	return glm::u8vec3((packedLocalPos >> 16) & 0xFFu, (packedLocalPos >> 8) & 0xFFu, packedLocalPos & 0xFFu);
}

glm::ivec3 SparseVolume::worldPosition(const glm::ivec3 &chunkPos, uint32_t packedLocalPos) {
	const glm::ivec3 base = chunkBase(chunkPos);
	const glm::u8vec3 local = unpackLocal(packedLocalPos);
	return glm::ivec3(base.x + (int)local.x, base.y + (int)local.y, base.z + (int)local.z);
}

SparseVolume::ChunkPtr SparseVolume::findChunk(const glm::ivec3 &chunkPos) const {
	core::ScopedLock lock(_chunkLock);
	const auto iter = _chunks.find(chunkPos);
	if (iter != _chunks.end()) {
		return iter->value;
	}
	return ChunkPtr();
}

SparseVolume::ChunkPtr SparseVolume::findOrCreateChunk(const glm::ivec3 &chunkPos) {
	core::ScopedLock lock(_chunkLock);
	const auto iter = _chunks.find(chunkPos);
	if (iter != _chunks.end()) {
		return iter->value;
	}
	ChunkPtr chunk = core::make_shared<Chunk>();
	_chunks.put(chunkPos, chunk);
	return chunk;
}

void SparseVolume::removeChunkIfEmpty(const glm::ivec3 &chunkPos, const ChunkPtr &chunk) {
	core::ScopedLock mapLock(_chunkLock);
	const auto iter = _chunks.find(chunkPos);
	if (iter == _chunks.end() || iter->value != chunk) {
		return;
	}
	core::ScopedLock chunkLock(chunk->lock);
	if (chunk->voxels.empty()) {
		_chunks.remove(chunkPos);
	}
}

bool SparseVolume::setVoxel(const glm::ivec3 &pos, const voxel::Voxel &voxel) {
	if (_isRegionValid && !_region.containsPoint(pos)) {
		return false;
	}

	const glm::ivec3 chunkPos = chunkPosition(pos);
	const glm::u8vec3 localPos = localPosition(pos, chunkPos);
	const uint32_t packedLocalPos = packLocal(localPos);

	if (!_storeEmptyVoxels && isAir(voxel.getMaterial())) {
		ChunkPtr chunk = findChunk(chunkPos);
		if (!chunk) {
			return true;
		}
		bool removed = false;
		bool emptyAfter = false;
		{
			core::ScopedLock chunkGuard(chunk->lock);
			removed = chunk->voxels.remove(packedLocalPos);
			if (removed) {
				_size.decrement(1);
			}
			emptyAfter = chunk->voxels.empty();
		}
		if (emptyAfter) {
			removeChunkIfEmpty(chunkPos, chunk);
		}
		return true;
	}

	ChunkPtr chunk = findOrCreateChunk(chunkPos);
	core::ScopedLock chunkGuard(chunk->lock);
	const auto iter = chunk->voxels.find(packedLocalPos);
	const bool hadVoxel = iter != chunk->voxels.end();
	chunk->voxels.put(packedLocalPos, voxel);
	if (!hadVoxel) {
		_size.increment(1);
	}
	return true;
}

const Voxel &SparseVolume::voxel(const glm::ivec3 &pos) const {
	if (_isRegionValid && !_region.containsPoint(pos)) {
		return _emptyVoxel;
	}
	const glm::ivec3 chunkPos = chunkPosition(pos);
	const glm::u8vec3 localPos = localPosition(pos, chunkPos);
	const uint32_t packedLocalPos = packLocal(localPos);
	ChunkPtr chunk = findChunk(chunkPos);
	if (!chunk) {
		return _emptyVoxel;
	}
	core::ScopedLock chunkGuard(chunk->lock);
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
	const glm::ivec3 chunkPos = chunkPosition(pos);
	const glm::u8vec3 localPos = localPosition(pos, chunkPos);
	const uint32_t packedLocalPos = packLocal(localPos);
	ChunkPtr chunk = findChunk(chunkPos);
	if (!chunk) {
		return false;
	}
	core::ScopedLock chunkGuard(chunk->lock);
	return chunk->voxels.hasKey(packedLocalPos);
}

const Voxel &SparseVolume::voxel(int32_t x, int32_t y, int32_t z) const {
	return voxel(glm::ivec3(x, y, z));
}

bool SparseVolume::hasVoxel(int x, int y, int z) const {
	return hasVoxel(glm::ivec3(x, y, z));
}

void SparseVolume::clear() {
	core::ScopedLock mapLock(_chunkLock);
	_chunks.clear();
	_size = 0;
}

SparseVolume::Sampler::Sampler(const SparseVolume *volume) : _volume(const_cast<SparseVolume *>(volume)) {
}

SparseVolume::Sampler::Sampler(const SparseVolume &volume) : _volume(const_cast<SparseVolume *>(&volume)) {
}

SparseVolume::Sampler::~Sampler() {
}

bool SparseVolume::Sampler::setVoxel(const Voxel &voxel) {
	if (_currentPositionInvalid) {
		return false;
	}
	_volume->setVoxel(_posInVolume, voxel);
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

	// Then we update the voxel pointer
	if (currentPositionValid()) {
		_currentVoxel = _volume->voxel(_posInVolume);
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

	// Then we update the voxel pointer
	if (!bIsOldPositionValid) {
		setPosition(_posInVolume);
	} else if (currentPositionValid()) {
		_currentVoxel = _volume->voxel(_posInVolume);
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

	// Then we update the voxel pointer
	if (!bIsOldPositionValid) {
		setPosition(_posInVolume);
	} else if (currentPositionValid()) {
		_currentVoxel = _volume->voxel(_posInVolume);
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

	// Then we update the voxel pointer
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

	// Then we update the voxel pointer
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

	// Then we update the voxel pointer
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

	// Then we update the voxel pointer
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
	core::ScopedLock mapLock(_chunkLock);
	for (auto chunkIter = _chunks.begin(); chunkIter != _chunks.end(); ++chunkIter) {
		const glm::ivec3 &chunkPos = chunkIter->key;
		const ChunkPtr &chunk = chunkIter->value;
		if (!chunk) {
			continue;
		}
		core::ScopedLock chunkLock(chunk->lock);
		for (auto voxelIter = chunk->voxels.begin(); voxelIter != chunk->voxels.end(); ++voxelIter) {
			const glm::ivec3 pos = worldPosition(chunkPos, voxelIter->key);
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
