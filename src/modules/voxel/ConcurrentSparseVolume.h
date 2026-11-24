/**
 * @file
 */

#pragma once

#include "core/GLM.h"
#include "core/SharedPtr.h"
#include "core/collection/DynamicMap.h"
#include "core/concurrent/Atomic.h"
#include "core/concurrent/Lock.h"
#include "voxel/SparseChunk.h"
#include "voxel/SparseVolumeSampler.h"
#include "voxel/SparseVolumeUtil.h"
#include "voxel/VolumeSamplerUtil.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxel {

/**
 * Sparse volume implementation which stores data in a hashmap of 256^3 chunks that are made of dense 8^3 bricks (see
 * @c SparseChunk). This is useful for volumes where most of the voxels are empty. This is using mutexes to be thread
 * safe for the parallel voxel algorithms. But when doing a lot of single voxel operations, you should use a RawVolume
 * - also for cache locallity.
 *
 * Bricks are not released when voxels get removed - only removing a whole chunk or clearing the volume frees memory.
 * This keeps references returned by @c voxel() pointing to valid memory while other threads remove voxels.
 */
class ConcurrentSparseVolume {
private:
	struct Chunk {
		// dense bricks - voxel() hands out references that must stay valid while other threads insert
		SparseChunk data{true};
		mutable core_trace_mutex(core::Lock, lock, "ConcurrentSparseVolumeChunk");
	};

	using ChunkPtr = core::SharedPtr<Chunk>;

	core::DynamicMap<glm::ivec3, ChunkPtr, 1031, glm::hash<glm::ivec3>> _chunks;
	mutable core_trace_mutex(core::Lock, _chunkLock, "ConcurrentSparseVolumeChunks");
	core::AtomicInt _size{0};
	static const constexpr voxel::Voxel _emptyVoxel{VoxelType::Air, 0, 0, 0};
	/** if this is a valid region, we limit the volume to this region */
	const voxel::Region _region;
	const bool _isRegionValid;
	bool _storeEmptyVoxels = false;

	// Last accessed chunk cache for performance
	mutable ChunkPtr _cachedChunkPtr;
	mutable glm::ivec3 _cachedChunkPosition{INT32_MAX, INT32_MAX, INT32_MAX};

	Chunk *findChunkUnsafe(const glm::ivec3 &chunkPos) const;
	ChunkPtr findOrCreateChunkCached(const glm::ivec3 &chunkPos);

public:
	class Sampler : public SparseVolumeSampler<Sampler, ConcurrentSparseVolume> {
	private:
		using Super = SparseVolumeSampler<Sampler, ConcurrentSparseVolume>;
		friend Super;

	public:
		using Super::Super;

		bool setVoxel(const Voxel &voxel);

	protected:
		// Cached chunk for performance
		mutable Chunk *_cachedChunk = nullptr;
		mutable glm::ivec3 _cachedChunkPos{0, 0, 0};
		mutable glm::ivec3 _cachedLocal{0, 0, 0};

		void updateCurrentVoxel();
		template<int Axis>
		inline void moved(int, int, int) {
			updateCurrentVoxel();
		}

		void invalidateChunkCache();
	};

	// invalid region means unlimited size
	ConcurrentSparseVolume(const voxel::Region &limitRegion = voxel::Region::InvalidRegion);

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

	bool setVoxel(const glm::ivec3 &pos, const voxel::Voxel &voxel);

	// Batch set voxels in a row (same Y, same Z, varying X)
	void setVoxelsRow(int x, int y, int z, int count, const Voxel &voxel);

	void clear();

	/**
	 * Gets a voxel at the position given by @c x,y,z coordinates
	 */
	[[nodiscard]] const Voxel &voxel(int32_t x, int32_t y, int32_t z) const;

	/**
	 * @param pos The 3D position of the voxel
	 * @return The voxel value
	 */
	[[nodiscard]] const Voxel &voxel(const glm::ivec3 &pos) const;

	[[nodiscard]] bool hasVoxel(int x, int y, int z) const;

	[[nodiscard]] bool hasVoxel(const glm::ivec3 &pos) const;

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
		core::ScopedLock mapLock(_chunkLock);
		sparse::copyTo(_chunks, target, nullptr, [](const Chunk &chunk, auto &&func) {
			core::ScopedLock chunkGuard(chunk.lock);
			chunk.data.visit(func);
		});
	}

	template<class Volume>
	void copyTo(Volume &target, const voxel::Region &region) const {
		core::ScopedLock mapLock(_chunkLock);
		sparse::copyTo(_chunks, target, &region, [](const Chunk &chunk, auto &&func) {
			core::ScopedLock chunkGuard(chunk.lock);
			chunk.data.visit(func);
		});
	}

	template<class Volume>
	void copyFrom(const Volume &source) {
		auto func = [this](int x, int y, int z, const voxel::Voxel &voxel) { setVoxel(x, y, z, voxel); };
		voxelutil::visitVolume(source, func);
	}
};

extern template class SparseVolumeSampler<ConcurrentSparseVolume::Sampler, ConcurrentSparseVolume>;

template<>
inline bool setVoxels<ConcurrentSparseVolume>(ConcurrentSparseVolume &volume, int x, int y, int z, int nx, int nz,
											  const Voxel *voxels, int amount) {
	sparse::clipToLowerCorner(volume.region(), x, y, z, nx, nz, voxels, amount);
	// Use parallel processing for Z slices, with optimized row setting
	app::for_parallel(0, nz, [nx, amount, &volume, &voxels, x, y, z](int start, int end) {
		for (int lz = start; lz < end; ++lz) {
			for (int ly = 0; ly < amount; ++ly) {
				volume.setVoxelsRow(x, y + ly, z + lz, nx, voxels[ly]);
			}
		}
	});
	return true;
}

inline int32_t ConcurrentSparseVolume::width() const {
	return _region.getWidthInVoxels();
}

inline int32_t ConcurrentSparseVolume::height() const {
	return _region.getHeightInVoxels();
}

inline int32_t ConcurrentSparseVolume::depth() const {
	return _region.getDepthInVoxels();
}

} // namespace voxel
