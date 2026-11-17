/**
 * @file
 */

#include "SparseVolume.h"
#include "DynamicVoxelArray.h"
#include <glm/common.hpp>

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

SparseVolume::Chunk *SparseVolume::findOrCreateChunk(const glm::ivec3 &chunkPos) {
	ChunkCacheEntry &entry = _writeChunkCache[writeChunkCacheSlot(chunkPos)];
	if (entry.chunk != nullptr && entry.pos == chunkPos) {
		return entry.chunk;
	}
	const auto iter = _chunks.find(chunkPos);
	if (iter != _chunks.end()) {
		entry.pos = chunkPos;
		entry.chunk = iter->value.get();
		return entry.chunk;
	}
	ChunkPtr chunk = core::make_shared<Chunk>();
	_chunks.put(chunkPos, chunk);
	entry.pos = chunkPos;
	entry.chunk = chunk.get();
	return entry.chunk;
}

void SparseVolume::removeChunkIfEmpty(const glm::ivec3 &chunkPos, Chunk *chunk) {
	if (chunk->empty()) {
		if (_cachedChunkPosition == chunkPos) {
			_cachedChunkPtr = ChunkPtr();
			_cachedChunkPosition = glm::ivec3(INT32_MAX, INT32_MAX, INT32_MAX);
		}
		ChunkCacheEntry &entry = _writeChunkCache[writeChunkCacheSlot(chunkPos)];
		if (entry.chunk == chunk) {
			entry.chunk = nullptr;
		}
		invalidateBrickCache();
		_chunks.remove(chunkPos);
	}
}

bool SparseVolume::removeVoxel(Chunk *chunk, const glm::ivec3 &localPos) {
	if (!chunk->removeVoxel(localPos.x, localPos.y, localPos.z)) {
		return false;
	}
	--_size;
	++_rowGeneration;
	if (chunk->brick(localPos.x, localPos.y, localPos.z) == nullptr) {
		invalidateBrickCache();
	}
	return true;
}

bool SparseVolume::setVoxel(const glm::ivec3 &pos, const voxel::Voxel &voxel) {
	if (_isRegionValid && !_region.containsPoint(pos)) {
		return false;
	}

	const bool isEmptyVoxel = !_storeEmptyVoxels && isAir(voxel.getMaterial());
	if (isEmptyVoxel) {
		const glm::ivec3 &chunkPos = sparse::chunkPosition(pos);
		Chunk *chunk = findChunk(chunkPos);
		if (!chunk) {
			return true;
		}
		removeVoxel(chunk, sparse::localPosition(pos));
		removeChunkIfEmpty(chunkPos, chunk);
		return true;
	}

	const glm::ivec3 brickPos(pos.x >> SparseChunk::BrickShift, pos.y >> SparseChunk::BrickShift,
							  pos.z >> SparseChunk::BrickShift);
	if (_cachedBrick == nullptr || brickPos != _cachedBrickPosition) {
		const glm::ivec3 &localPos = sparse::localPosition(pos);
		_cachedBrickChunk = findOrCreateChunk(sparse::chunkPosition(pos));
		_cachedBrick = _cachedBrickChunk->findOrCreateBrick(localPos.x, localPos.y, localPos.z);
		_cachedBrickPosition = brickPos;
	}
	const Voxel value(voxel);
	if (_cachedBrickChunk->setVoxel(_cachedBrick, SparseChunk::voxelIndex(pos.x, pos.y, pos.z), value)) {
		++_size;
		++_rowGeneration;
	}
	return true;
}

void SparseVolume::insertBatch(core::DynamicArray<VoxelPosition> &voxels) {
	if (voxels.empty()) {
		return;
	}
	// Sort by chunk position for optimal cache utilization
	voxels.sort([](const VoxelPosition &a, const VoxelPosition &b) {
		const glm::ivec3 ca = sparse::chunkPosition(a.pos);
		const glm::ivec3 cb = sparse::chunkPosition(b.pos);
		if (ca.z != cb.z) return ca.z < cb.z;
		if (ca.y != cb.y) return ca.y < cb.y;
		return ca.x < cb.x;
	});

	glm::ivec3 currentChunkPos(INT32_MAX);
	Chunk *currentChunk = nullptr;

	for (const VoxelPosition &vp : voxels) {
		if (_isRegionValid && !_region.containsPoint(vp.pos)) {
			continue;
		}
		const glm::ivec3 cp = sparse::chunkPosition(vp.pos);
		if (cp != currentChunkPos) {
			currentChunkPos = cp;
			currentChunk = findOrCreateChunk(cp);
		}
		const glm::ivec3 lp = sparse::localPosition(vp.pos);
		if (currentChunk->setVoxel(lp.x, lp.y, lp.z, vp.voxel)) {
			++_size;
			++_rowGeneration;
		}
	}
}

void SparseVolume::setVoxelsRow(int x, int y, int z, int count, const Voxel &voxel) {
	const Voxel value(voxel);
	const bool isEmptyVoxel = !_storeEmptyVoxels && isAir(value.getMaterial());

	if (isEmptyVoxel) {
		for (int i = 0; i < count; ++i) {
			setVoxel(x + i, y, z, value);
		}
		return;
	}

	const int endX = x + count;
	int currentX = x;
	while (currentX < endX) {
		const glm::ivec3 pos(currentX, y, z);
		const glm::ivec3 chunkPos = sparse::chunkPosition(pos);
		const glm::ivec3 localPos = sparse::localPosition(pos);
		const int chunkEndX = sparse::chunkBase(chunkPos).x + sparse::ChunkSide;
		const int processUntilX = (endX < chunkEndX) ? endX : chunkEndX;
		Chunk *chunk = findOrCreateChunk(chunkPos);
		const int added = chunk->setVoxelsRow(localPos.x, localPos.y, localPos.z, processUntilX - currentX, value);
		_size += added;
		if (added != 0) {
			++_rowGeneration;
		}
		currentX = processUntilX;
	}
}

const Voxel &SparseVolume::voxel(const glm::ivec3 &pos) const {
	if (_isRegionValid && !_region.containsPoint(pos)) {
		return _emptyVoxel;
	}
	const Chunk *chunk = findChunk(sparse::chunkPosition(pos));
	if (!chunk) {
		return _emptyVoxel;
	}
	const glm::ivec3 &localPos = sparse::localPosition(pos);
	const Voxel *v = chunk->voxel(localPos.x, localPos.y, localPos.z);
	if (v == nullptr) {
		return _emptyVoxel;
	}
	return *v;
}

bool SparseVolume::hasVoxel(const glm::ivec3 &pos) const {
	if (_isRegionValid && !_region.containsPoint(pos)) {
		return false;
	}
	const Chunk *chunk = findChunk(sparse::chunkPosition(pos));
	if (!chunk) {
		return false;
	}
	const glm::ivec3 &localPos = sparse::localPosition(pos);
	return chunk->hasVoxel(localPos.x, localPos.y, localPos.z);
}

const Voxel &SparseVolume::voxel(int32_t x, int32_t y, int32_t z) const {
	return voxel(glm::ivec3(x, y, z));
}

bool SparseVolume::hasVoxel(int x, int y, int z) const {
	return hasVoxel(glm::ivec3(x, y, z));
}

void SparseVolume::clear() {
	++_rowGeneration;
	invalidateBrickCache();
	for (ChunkCacheEntry &entry : _writeChunkCache) {
		entry.chunk = nullptr;
	}
	_chunks.clear();
	_size = 0;
	_cachedChunkPtr = ChunkPtr();
	_cachedChunkPosition = glm::ivec3(INT32_MAX, INT32_MAX, INT32_MAX);
}

CORE_FORCE_INLINE void SparseVolume::Sampler::sampleVoxel(const glm::ivec3 &pos, bool cacheRow) {
	static constexpr Voxel emptyBrick[SparseChunk::BrickVoxels]{};
	constexpr int mask = SparseChunk::BrickSide - 1;
	const int idx = SparseChunk::voxelIndex(pos.x, pos.y, pos.z);
	_cachedWholeBrick = !cacheRow && (_cachedBrick == nullptr || _cachedBrick->dense());
	if (_cachedWholeBrick) {
		_cachedPayload = (_cachedBrick ? _cachedBrick->voxels : emptyBrick) + (idx & ~mask);
		_cachedLower = pos & ~mask;
		_cachedUpper = pos | mask;
		if (_volume->_isRegionValid) {
			_cachedLower = glm::max(_cachedLower, region().getLowerCorner());
			_cachedUpper = glm::min(_cachedUpper, region().getUpperCorner());
		}
		_cachedUpperX = _cachedUpper.x;
	} else {
		_cachedPayload = cacheRow ? SparseChunk::borrowBrickRow(_cachedBrick, idx) : nullptr;
		if (_cachedPayload == nullptr) {
			_currentVoxel.overwrite(SparseChunk::voxel(_cachedBrick, idx));
			return;
		}
		const int lower = pos.x & ~mask;
		const int upper = pos.x | mask;
		_cachedLower.x = _volume->_isRegionValid ? core_max(lower, region().getLowerX()) : lower;
		_cachedUpperX = _volume->_isRegionValid ? core_min(upper, region().getUpperX()) : upper;
	}
	_cachedPayloadGeneration = _volume->_rowGeneration;
}

void SparseVolume::Sampler::updateCurrentVoxel(bool cacheRow) {
	int x, y, z;
	readPosition(x, y, z);
	const glm::ivec3 chunkPos(x >> SparseChunk::SideShift, y >> SparseChunk::SideShift, z >> SparseChunk::SideShift);
	if (_cachedChunk == nullptr || _cachedChunkPos != chunkPos || _cachedGeneration != _volume->_releaseGeneration) {
		_cachedChunk = _volume->findChunk(chunkPos);
		_cachedChunkPos = chunkPos;
		_cachedGeneration = _volume->_releaseGeneration;
	}
	_cachedBrick = _cachedChunk
					   ? _cachedChunk->brick(x & sparse::ChunkMask, y & sparse::ChunkMask, z & sparse::ChunkMask)
					   : nullptr;
	sampleVoxel({x, y, z}, cacheRow);
}

bool SparseVolume::Sampler::setVoxel(const Voxel &voxel) {
	if (_currentPositionInvalid) {
		return false;
	}

	// Fast path: the caches are updated on every position change
	if (_cachedChunk && _cachedGeneration == _volume->_releaseGeneration) {
		const bool isEmptyVoxel = !_volume->_storeEmptyVoxels && isAir(voxel.getMaterial());
		const glm::ivec3 localPos = sparse::localPosition(_posInVolume);
		if (isEmptyVoxel) {
			// releasing the brick bumps the release generation - the next access resolves the brick again
			_volume->removeVoxel(_cachedChunk, localPos);
			_cachedPayload = nullptr;
			_currentVoxel.overwrite(SparseVolume::_emptyVoxel);
			return true;
		}
		if (_cachedBrick == nullptr) {
			_cachedBrick = _cachedChunk->findOrCreateBrick(localPos.x, localPos.y, localPos.z);
		}
		const int idx = SparseChunk::voxelIndex(localPos.x, localPos.y, localPos.z);
		const Voxel value(voxel);
		if (_cachedChunk->setVoxel(_cachedBrick, idx, value)) {
			++_volume->_size;
			++_volume->_rowGeneration;
		}
		sampleVoxel(_posInVolume, _cachedPayload != nullptr);
		return true;
	}

	// Slow path: no chunk yet - fall back to volume setVoxel
	_volume->setVoxel(_posInVolume, voxel);
	updateCurrentVoxel();
	return true;
}

template<int Axis>
void SparseVolume::Sampler::moved(int oldCoord, int newCoord, int) {
	if (_cachedBrick != nullptr && (newCoord >> SparseChunk::BrickShift) == (oldCoord >> SparseChunk::BrickShift) &&
		_cachedGeneration == _volume->_releaseGeneration) {
		sampleVoxel(_posInVolume, Axis == 0);
		return;
	}
	updateCurrentVoxel(Axis == 0);
}

template class SparseVolumeSampler<SparseVolume::Sampler, SparseVolume>;

Region SparseVolume::calculateRegion() const {
	if (_size == 0) {
		return Region::InvalidRegion;
	}
	return sparse::calculateRegion(_chunks, [](const Chunk &chunk, glm::ivec3 &mins, glm::ivec3 &maxs) {
		return chunk.calculateBounds(mins, maxs);
	});
}

} // namespace voxel
