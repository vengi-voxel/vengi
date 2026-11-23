/**
 * @file
 */

#include "ConcurrentSparseVolume.h"
#include "math/Vec3i.h"

namespace voxel {

ConcurrentSparseVolume::ConcurrentSparseVolume(const voxel::Region &region) : _region(region), _isRegionValid(_region.isValid()) {
}

ConcurrentSparseVolume::Chunk *ConcurrentSparseVolume::findChunkUnsafe(const math::Vec3i &chunkPos) const {
	const auto iter = _chunks.find(chunkPos);
	if (iter != _chunks.end()) {
		return iter->value.get();
	}
	return nullptr;
}

ConcurrentSparseVolume::ChunkPtr ConcurrentSparseVolume::findOrCreateChunkCached(const math::Vec3i &chunkPos) {
	core::ScopedLock mapLock(_chunkLock);
	if (_cachedChunkPosition == chunkPos && _cachedChunkPtr) {
		return _cachedChunkPtr;
	}
	ChunkPtr chunkPtr;
	const auto iter = _chunks.find(chunkPos);
	if (iter != _chunks.end()) {
		chunkPtr = iter->value;
	} else {
		chunkPtr = core::make_shared<Chunk>();
		_chunks.put(chunkPos, chunkPtr);
	}
	_cachedChunkPtr = chunkPtr;
	_cachedChunkPosition = chunkPos;
	return chunkPtr;
}

bool ConcurrentSparseVolume::setVoxel(const math::Vec3i &pos, const voxel::Voxel &voxel) {
	if (_isRegionValid && !_region.containsPoint(pos)) {
		return false;
	}

	const math::Vec3i &chunkPos = sparse::chunkPosition(pos);
	const math::Vec3i &localPos = sparse::localPosition(pos);
	const bool isEmptyVoxel = !_storeEmptyVoxels && isAir(voxel.getMaterial());

	if (isEmptyVoxel) {
		core::ScopedLock mapLock(_chunkLock);
		Chunk *chunk = findChunkUnsafe(chunkPos);
		if (!chunk) {
			return true;
		}
		bool emptyAfter = false;
		{
			core::ScopedLock chunkGuard(chunk->lock);
			if (chunk->data.removeVoxel(localPos.x, localPos.y, localPos.z, false)) {
				_size.decrement(1);
			}
			emptyAfter = chunk->data.empty();
		}
		if (emptyAfter) {
			if (_cachedChunkPosition == chunkPos) {
				_cachedChunkPtr = ChunkPtr();
				_cachedChunkPosition = math::Vec3i(INT32_MAX, INT32_MAX, INT32_MAX);
			}
			_chunks.remove(chunkPos);
		}
		return true;
	}

	ChunkPtr chunkPtr = findOrCreateChunkCached(chunkPos);
	Chunk *chunk = chunkPtr.get();
	core::ScopedLock chunkGuard(chunk->lock);
	if (chunk->data.setVoxel(localPos.x, localPos.y, localPos.z, voxel)) {
		_size.increment(1);
	}
	return true;
}

void ConcurrentSparseVolume::setVoxelsRow(int x, int y, int z, int count, const Voxel &voxel) {
	const bool isEmptyVoxel = !_storeEmptyVoxels && isAir(voxel.getMaterial());

	if (isEmptyVoxel) {
		for (int i = 0; i < count; ++i) {
			setVoxel(x + i, y, z, voxel);
		}
		return;
	}

	const int endX = x + count;
	int currentX = x;
	while (currentX < endX) {
		const math::Vec3i pos(currentX, y, z);
		const math::Vec3i chunkPos = sparse::chunkPosition(pos);
		const math::Vec3i localPos = sparse::localPosition(pos);
		const int chunkEndX = sparse::chunkBase(chunkPos).x + sparse::ChunkSide;
		const int processUntilX = (endX < chunkEndX) ? endX : chunkEndX;

		ChunkPtr chunkPtr = findOrCreateChunkCached(chunkPos);
		Chunk *chunk = chunkPtr.get();
		int added;
		{
			core::ScopedLock chunkGuard(chunk->lock);
			added = chunk->data.setVoxelsRow(localPos.x, localPos.y, localPos.z, processUntilX - currentX, voxel);
		}
		if (added > 0) {
			_size.increment(added);
		}
		currentX = processUntilX;
	}
}

const Voxel &ConcurrentSparseVolume::voxel(const math::Vec3i &pos) const {
	if (_isRegionValid && !_region.containsPoint(pos)) {
		return _emptyVoxel;
	}
	const math::Vec3i &chunkPos = sparse::chunkPosition(pos);
	const math::Vec3i &localPos = sparse::localPosition(pos);

	core::ScopedLock mapLock(_chunkLock);
	const Chunk *chunk = findChunkUnsafe(chunkPos);
	if (!chunk) {
		return _emptyVoxel;
	}

	core::ScopedLock chunkGuard(chunk->lock);
	const Voxel *v = chunk->data.voxel(localPos.x, localPos.y, localPos.z);
	if (v == nullptr) {
		return _emptyVoxel;
	}
	return *v;
}

bool ConcurrentSparseVolume::hasVoxel(const math::Vec3i &pos) const {
	if (_isRegionValid && !_region.containsPoint(pos)) {
		return false;
	}
	const math::Vec3i &chunkPos = sparse::chunkPosition(pos);
	const math::Vec3i &localPos = sparse::localPosition(pos);

	core::ScopedLock mapLock(_chunkLock);
	const Chunk *chunk = findChunkUnsafe(chunkPos);
	if (!chunk) {
		return false;
	}

	core::ScopedLock chunkGuard(chunk->lock);
	return chunk->data.hasVoxel(localPos.x, localPos.y, localPos.z);
}

const Voxel &ConcurrentSparseVolume::voxel(int32_t x, int32_t y, int32_t z) const {
	return voxel(math::Vec3i(x, y, z));
}

bool ConcurrentSparseVolume::hasVoxel(int x, int y, int z) const {
	return hasVoxel(math::Vec3i(x, y, z));
}

void ConcurrentSparseVolume::clear() {
	core::ScopedLock mapLock(_chunkLock);
	_chunks.clear();
	_size = 0;
	_cachedChunkPtr = ChunkPtr();
	_cachedChunkPosition = math::Vec3i(INT32_MAX, INT32_MAX, INT32_MAX);
}

void ConcurrentSparseVolume::Sampler::invalidateChunkCache() {
	_cachedChunk = nullptr;
}

void ConcurrentSparseVolume::Sampler::updateCurrentVoxel() {
	int x, y, z;
	readPosition(x, y, z);
	const math::Vec3i pos(x, y, z);
	const math::Vec3i chunkPos = sparse::chunkPosition(pos);
	_cachedLocal = sparse::localPosition(pos);
	if (!_cachedChunk || _cachedChunkPos != chunkPos) {
		core::ScopedLock mapLock(_volume->_chunkLock);
		_cachedChunk = _volume->findChunkUnsafe(chunkPos);
		_cachedChunkPos = chunkPos;
	}
	if (_cachedChunk) {
		core::ScopedLock chunkGuard(_cachedChunk->lock);
		const Voxel *v = _cachedChunk->data.voxel(_cachedLocal.x, _cachedLocal.y, _cachedLocal.z);
		_currentVoxel.overwrite(v ? *v : ConcurrentSparseVolume::_emptyVoxel);
	} else {
		_currentVoxel.overwrite(ConcurrentSparseVolume::_emptyVoxel);
	}
}

bool ConcurrentSparseVolume::Sampler::setVoxel(const Voxel &voxel) {
	if (_currentPositionInvalid) {
		return false;
	}

	// Fast path: use cached chunk if valid
	if (_cachedChunk) {
		const math::Vec3i currentChunkPos = sparse::chunkPosition(_posInVolume);
		if (currentChunkPos == _cachedChunkPos) {
			const math::Vec3i localPos = sparse::localPosition(_posInVolume);
			const bool isEmptyVoxel = !_volume->_storeEmptyVoxels && isAir(voxel.getMaterial());

			core::ScopedLock chunkGuard(_cachedChunk->lock);
			if (isEmptyVoxel) {
				if (_cachedChunk->data.removeVoxel(localPos.x, localPos.y, localPos.z, false)) {
					_volume->_size.decrement(1);
				}
				_currentVoxel.overwrite(ConcurrentSparseVolume::_emptyVoxel);
			} else {
				if (_cachedChunk->data.setVoxel(localPos.x, localPos.y, localPos.z, voxel)) {
					_volume->_size.increment(1);
				}
				_currentVoxel.overwrite(*_cachedChunk->data.voxel(localPos.x, localPos.y, localPos.z));
			}
			return true;
		}
	}

	// Slow path: fall back to volume setVoxel
	_volume->setVoxel(_posInVolume, voxel);
	_currentVoxel.overwrite(_volume->voxel(_posInVolume));
	invalidateChunkCache();
	return true;
}

template class SparseVolumeSampler<ConcurrentSparseVolume::Sampler, ConcurrentSparseVolume>;

Region ConcurrentSparseVolume::calculateRegion() const {
	if (_size == 0) {
		return Region::InvalidRegion;
	}

	core::ScopedLock mapLock(_chunkLock);
	return sparse::calculateRegion(_chunks, [](const Chunk &chunk, math::Vec3i &mins, math::Vec3i &maxs) {
		core::ScopedLock chunkLock(chunk.lock);
		return chunk.data.calculateBounds(mins, maxs);
	});
}

} // namespace voxel
