/**
 * @file
 */

#pragma once

#include "math/Vec3f.h"
#include "core/SharedPtr.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/DynamicMap.h"
#include "voxel/SparseChunk.h"
#include "voxel/SparseVolumeSampler.h"
#include "voxel/SparseVolumeUtil.h"
#include "voxel/VolumeSamplerUtil.h"
#include "voxelutil/VolumeVisitor.h"
#include "math/Vec3i.h"

namespace voxel {

struct VoxelPosition;

/**
 * Non-thread-safe sparse volume implementation which stores data in a hashmap of 256^3 chunks that are made of
 * dense 8^3 bricks (see @c SparseChunk). This is useful for volumes where most of the voxels are empty. This is NOT
 * thread safe - use ConcurrentSparseVolume for parallel voxel algorithms.
 */
class SparseVolume {
private:
	using Chunk = SparseChunk;
	using ChunkPtr = core::SharedPtr<Chunk>;

	core::DynamicMap<math::Vec3i, ChunkPtr, 1031, math::Vec3iHash> _chunks;
	int _size = 0;
	static const constexpr voxel::Voxel _emptyVoxel{VoxelType::Air, 0, 0, 0};
	/** if this is a valid region, we limit the volume to this region */
	const voxel::Region _region;
	const bool _isRegionValid;
	bool _storeEmptyVoxels = false;

	// Last accessed chunk cache for performance
	mutable ChunkPtr _cachedChunkPtr;
	mutable math::Vec3i _cachedChunkPosition{INT32_MAX, INT32_MAX, INT32_MAX};

	// Last written brick - must be invalidated whenever a brick or chunk gets released
	Chunk *_cachedBrickChunk = nullptr;
	SparseChunk::Brick *_cachedBrick = nullptr;
	math::Vec3i _cachedBrickPosition{INT32_MAX, INT32_MAX, INT32_MAX};
	// incremented whenever a brick or chunk gets released - samplers compare it before using their cached pointers
	uint32_t _releaseGeneration = 0u;
	// Changes in occupancy can shift packed rows, move their storage or make cached air rows obsolete.
	uint32_t _rowGeneration = 0u;

	// direct mapped cache for the write path - spares the hashing for volumes with more than one chunk
	struct ChunkCacheEntry {
		math::Vec3i pos{0};
		Chunk *chunk = nullptr;
	};
	static constexpr int ChunkCacheShift = 2;
	static constexpr int ChunkCacheMask = (1 << ChunkCacheShift) - 1;
	ChunkCacheEntry _writeChunkCache[1 << (3 * ChunkCacheShift)];
	static inline int writeChunkCacheSlot(const math::Vec3i &chunkPos) {
		return (chunkPos.x & ChunkCacheMask) | ((chunkPos.y & ChunkCacheMask) << ChunkCacheShift) |
			   ((chunkPos.z & ChunkCacheMask) << (2 * ChunkCacheShift));
	}

	Chunk *findChunk(const math::Vec3i &chunkPos) const;
	Chunk *findOrCreateChunk(const math::Vec3i &chunkPos);
	void removeChunkIfEmpty(const math::Vec3i &chunkPos, Chunk *chunk);
	bool removeVoxel(Chunk *chunk, const math::Vec3i &localPos);

	inline void invalidateBrickCache() {
		_cachedBrick = nullptr;
		_cachedBrickChunk = nullptr;
		_cachedBrickPosition = math::Vec3i(INT32_MAX, INT32_MAX, INT32_MAX);
		++_releaseGeneration;
	}

public:
	/** References returned by voxel() can be invalidated by structural edits to the volume. */
	class Sampler : public SparseVolumeSampler<Sampler, SparseVolume> {
	private:
		using Super = SparseVolumeSampler<Sampler, SparseVolume>;
		friend Super;

		template<int Axis, int Direction>
		CORE_FORCE_INLINE bool moveCached(uint32_t offset) {
			if (_cachedPayload == nullptr) {
				return false;
			}
			if constexpr (Axis != 0) {
				if (!_cachedWholeBrick) {
					return false;
				}
			}
			int &coord = _posInVolume[Axis];
			const int available = Direction > 0 ? _cachedUpper[Axis] - coord : coord - _cachedLower[Axis];
			if (offset > (uint32_t)available) {
				return false;
			}
			if constexpr (Axis != 0) {
				// The borrowed row may have been released since the previous read.
				if (_cachedPayloadGeneration != _volume->_rowGeneration) {
					return false;
				}
				_cachedPayload += (int)offset * (Direction * (1 << (Axis * SparseChunk::BrickShift)));
			}
			coord += Direction * (int)offset;
			return true;
		}

	public:
		using Super::Super;

		bool setVoxel(const Voxel &voxel);

		CORE_FORCE_INLINE void movePositiveX(uint32_t offset = 1) {
			const int64_t newX = (int64_t)_posInVolume.x + offset;
			if (_cachedPayload && newX <= _cachedUpperX) {
				_posInVolume.x = (int)newX;
				return;
			}
			Super::movePositiveX(offset);
		}
		CORE_FORCE_INLINE void movePositiveY(uint32_t offset = 1) {
			if (!moveCached<1, 1>(offset)) {
				Super::movePositiveY(offset);
			}
		}
		CORE_FORCE_INLINE void movePositiveZ(uint32_t offset = 1) {
			if (!moveCached<2, 1>(offset)) {
				Super::movePositiveZ(offset);
			}
		}
		CORE_FORCE_INLINE void moveNegativeX(uint32_t offset = 1) {
			if (!moveCached<0, -1>(offset)) {
				Super::moveNegativeX(offset);
			}
		}
		CORE_FORCE_INLINE void moveNegativeY(uint32_t offset = 1) {
			if (!moveCached<1, -1>(offset)) {
				Super::moveNegativeY(offset);
			}
		}
		CORE_FORCE_INLINE void moveNegativeZ(uint32_t offset = 1) {
			if (!moveCached<2, -1>(offset)) {
				Super::moveNegativeZ(offset);
			}
		}

	protected:
		CORE_FORCE_INLINE const Voxel &sampledVoxel() const {
			if (_cachedPayload == nullptr) {
				return Super::sampledVoxel();
			}
			if (_cachedPayloadGeneration == _volume->_rowGeneration) {
				return _cachedPayload[_posInVolume.x & (SparseChunk::BrickSide - 1)];
			}
			return _volume->voxel(_posInVolume);
		}

		// Cached chunk for performance
		mutable Chunk *_cachedChunk = nullptr;
		mutable math::Vec3i _cachedChunkPos{0, 0, 0};
		SparseChunk::Brick *_cachedBrick = nullptr;
		const Voxel *_cachedPayload = nullptr;
		int _cachedUpperX = 0;
		uint32_t _cachedPayloadGeneration = 0u;
		// SparseVolume::_releaseGeneration at the time _cachedChunk and _cachedBrick were resolved
		uint32_t _cachedGeneration = 0u;
		bool _cachedWholeBrick = false;
		math::Vec3i _cachedLower{0};
		math::Vec3i _cachedUpper{0};

		void invalidateSample() {
			_cachedPayload = nullptr;
		}

		void updateCurrentVoxel(bool cacheRow = true);
		void sampleVoxel(const math::Vec3i &pos, bool cacheRow);
		template<int Axis>
		void moved(int oldCoord, int newCoord, int delta);
	};

	// invalid region means unlimited size
	SparseVolume(const voxel::Region &limitRegion = voxel::Region::InvalidRegion);

	void setStoreEmptyVoxels(bool storeEmptyVoxels) {
		_storeEmptyVoxels = storeEmptyVoxels;
	}

	[[nodiscard]] inline const voxel::Region &region() const {
		return _region;
	}

	[[nodiscard]] Region calculateRegion() const;

	inline bool setVoxel(int x, int y, int z, const voxel::Voxel &voxel) {
		return setVoxel({x, y, z}, voxel);
	}

	bool setVoxel(const math::Vec3i &pos, const voxel::Voxel &voxel);

	// Batch set voxels in a row (same Y, same Z, varying X)
	void setVoxelsRow(int x, int y, int z, int count, const Voxel &voxel);

	/**
	 * @brief Batch insert voxels from a DynamicVoxelArray.
	 * Sorts the array by chunk position for optimal cache utilization.
	 */
	void insertBatch(core::DynamicArray<VoxelPosition> &voxels);

	void clear();

	/**
	 * Gets a voxel at the position given by @c x,y,z coordinates
	 */
	[[nodiscard]] const Voxel &voxel(int32_t x, int32_t y, int32_t z) const;

	/**
	 * @param pos The 3D position of the voxel
	 * @return The voxel value
	 */
	[[nodiscard]] const Voxel &voxel(const math::Vec3i &pos) const;

	[[nodiscard]] bool hasVoxel(int x, int y, int z) const;

	[[nodiscard]] bool hasVoxel(const math::Vec3i &pos) const;

	[[nodiscard]] inline bool empty() const {
		return size() == 0;
	}

	[[nodiscard]] inline size_t size() const {
		return (size_t)_size;
	}

	/**
	 * @return The width of the volume in voxels. Note that this value is inclusive, so that if the valid range is e.g.
	 * 0 to 63 then the width is 64.
	 * @sa height(), getDepth()
	 */
	int32_t width() const;

	/**
	 * @return The height of the volume in voxels. Note that this value is inclusive, so that if the valid range is e.g.
	 * 0 to 63 then the height is 64.
	 * @sa width(), getDepth()
	 */
	int32_t height() const;

	/**
	 * @return The depth of the volume in voxels. Note that this value is inclusive, so that if the valid range is e.g.
	 * 0 to 63 then the depth is 64.
	 * @sa width(), height()
	 */
	int32_t depth() const;

	template<class Volume>
	void copyTo(Volume &target) const {
		sparse::copyTo(_chunks, target, nullptr, [](const Chunk &chunk, auto &&func) { chunk.visit(func); });
	}

	template<class Volume>
	void copyTo(Volume &target, const voxel::Region &region) const {
		sparse::copyTo(_chunks, target, &region, [](const Chunk &chunk, auto &&func) { chunk.visit(func); });
	}

	template<class Volume>
	void copyFrom(const Volume &source) {
		auto func = [this](int x, int y, int z, const voxel::Voxel &voxel) { setVoxel(x, y, z, voxel); };
		voxelutil::visitVolume(source, func);
	}
};

extern template class SparseVolumeSampler<SparseVolume::Sampler, SparseVolume>;

template<>
inline bool setVoxels<SparseVolume>(SparseVolume &volume, int x, int y, int z, int nx, int nz, const Voxel *voxels,
									int amount) {
	sparse::clipToLowerCorner(volume.region(), x, y, z, nx, nz, voxels, amount);
	for (int lz = 0; lz < nz; ++lz) {
		for (int ly = 0; ly < amount; ++ly) {
			volume.setVoxelsRow(x, y + ly, z + lz, nx, voxels[ly]);
		}
	}
	return true;
}

inline int32_t SparseVolume::width() const {
	return _region.getWidthInVoxels();
}

inline int32_t SparseVolume::height() const {
	return _region.getHeightInVoxels();
}

inline int32_t SparseVolume::depth() const {
	return _region.getDepthInVoxels();
}

} // namespace voxel
