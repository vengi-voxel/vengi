/**
 * @file
 */

#pragma once

#include "app/ForParallel.h"
#include "core/Common.h"
#include "core/StandardLib.h"
#include "core/collection/DynamicArray.h"
#include "core/concurrent/Lock.h"
#include "voxel/RawVolume.h"
#include "voxel/SparseChunk.h"
#include <glm/common.hpp>
#include <string.h>

namespace voxel {

namespace priv {

static CORE_FORCE_INLINE uint32_t tileVoxelBits(const Voxel &v) {
	uint32_t bits = 0u;
	memcpy(&bits, &v, sizeof(bits));
	return bits;
}

} // namespace priv

/**
 * @brief Unified write target for @c forEachTileWriteParallel.
 *
 * - On contiguous volumes: writes directly into the flat @c Voxel* array with zero staging overhead.
 * - On sparse volumes: writes lock-free into a 2 KiB L1-resident 8x8x8 stack tile and commits once
 *   via @c SparseChunk::prepareBrick + pointer-transfer adoption.
 */
class ParallelTileWriter {
private:
	Region _tileRegion;
	// Contiguous mode fields
	Voxel *_denseData = nullptr;
	glm::ivec3 _volLower{0};
	int64_t _denseW = 0;
	int64_t _denseStride = 0;
	// Sparse mode fields
	const RawVolume *_sparseVol = nullptr;
	glm::ivec3 _localBrickOrigin{0};
	Voxel _defaultVoxel{};
	uint32_t _defaultBits = 0u;
	bool _seeded = false;
	bool _modified = false;
	int _count = 0;
	uint64_t _mask[SparseChunk::BrickMaskWords];
	Voxel _tile[SparseChunk::BrickVoxels];

	void ensureSeeded() {
		if (_seeded) {
			return;
		}
		_seeded = true;
		_sparseVol->readSparseBrickTile(_localBrickOrigin, _tile, _mask, _count);
	}

public:
	ParallelTileWriter(const Region &tileRegion, Voxel *denseData, const glm::ivec3 &volLower, int64_t denseW,
					   int64_t denseStride)
		: _tileRegion(tileRegion), _denseData(denseData), _volLower(volLower), _denseW(denseW),
		  _denseStride(denseStride) {
	}

	ParallelTileWriter(const Region &tileRegion, const RawVolume *sparseVol, const glm::ivec3 &volLower,
					   const glm::ivec3 &localBrickOrigin, const Voxel &defaultVoxel)
		: _tileRegion(tileRegion), _volLower(volLower), _sparseVol(sparseVol), _localBrickOrigin(localBrickOrigin),
		  _defaultVoxel(defaultVoxel), _defaultBits(priv::tileVoxelBits(defaultVoxel)) {
	}

	CORE_FORCE_INLINE const Region &region() const {
		return _tileRegion;
	}

	CORE_FORCE_INLINE bool isContiguous() const {
		return _denseData != nullptr;
	}

	CORE_FORCE_INLINE const glm::ivec3 &localBrickOrigin() const {
		return _localBrickOrigin;
	}

	/**
	 * @brief Fast path for merging an aligned full 8x8x8 air-default source brick into a sparse air-default target.
	 * @return number of voxels changed (same semantics as Voxel::operator=).
	 */
	int mergeAlignedSparseBrick(const SparseChunk::Brick *srcBrick) {
		core_assert(_denseData == nullptr);
		if (srcBrick == nullptr || srcBrick->count == 0) {
			return 0;
		}
		const bool srcDense = srcBrick->dense();
		// Ultra-fast path when destination brick is currently empty: overwrite into fresh tile directly
		if (!_seeded &&
			_sparseVol->sparseBrickAt(_localBrickOrigin.x, _localBrickOrigin.y, _localBrickOrigin.z) == nullptr) {
			_seeded = true;
			_modified = true;
			_count = srcBrick->count;
			for (int w = 0; w < SparseChunk::BrickMaskWords; ++w) {
				_mask[w] = srcBrick->mask[w];
			}
			if (srcDense) {
				memcpy((void *)_tile, (const void *)srcBrick->voxels, sizeof(_tile));
			} else {
				core_memset((void *)_tile, 0, sizeof(_tile));
				int packed = 0;
				for (int w = 0; w < SparseChunk::BrickMaskWords; ++w) {
					uint64_t bits = srcBrick->mask[w];
					while (bits != 0u) {
						const int bit = SparseChunk::ctz64(bits);
						bits &= bits - 1u;
						_tile[(w << 6) | bit].overwrite(srcBrick->voxels[packed++]);
					}
				}
			}
			return _count;
		}

		ensureSeeded();
		int changed = 0;
		int packed = 0;
		for (int w = 0; w < SparseChunk::BrickMaskWords; ++w) {
			uint64_t bits = srcBrick->mask[w];
			while (bits != 0u) {
				const int bit = SparseChunk::ctz64(bits);
				bits &= bits - 1u;
				const int idx = (w << 6) | bit;
				const Voxel &sv = srcDense ? srcBrick->voxels[idx] : srcBrick->voxels[packed++];
				Voxel merged;
				merged.overwrite(_tile[idx]);
				merged = sv;
				const uint32_t newBits = priv::tileVoxelBits(merged);
				if (newBits == priv::tileVoxelBits(_tile[idx])) {
					continue;
				}
				_tile[idx].overwrite(merged);
				const uint64_t bitMask = (uint64_t)1u << bit;
				const bool wasSet = (_mask[w] & bitMask) != 0u;
				const bool nowSet = (newBits != _defaultBits);
				if (nowSet && !wasSet) {
					_mask[w] |= bitMask;
					++_count;
				} else if (!nowSet && wasSet) {
					_mask[w] &= ~bitMask;
					--_count;
				}
				_modified = true;
				++changed;
			}
		}
		return changed;
	}

	/** Write voxel with Voxel::operator= normal-merge semantics; returns true if voxel changed. */
	CORE_FORCE_INLINE bool setVoxel(int x, int y, int z, const Voxel &v) {
		if (core_likely(_denseData != nullptr)) {
			const int64_t idx = ((int64_t)x - (int64_t)_volLower.x) +
								((int64_t)y - (int64_t)_volLower.y) * _denseW +
								((int64_t)z - (int64_t)_volLower.z) * _denseStride;
			if (_denseData[idx] == v) {
				return false;
			}
			_denseData[idx] = v;
			return true;
		}
		ensureSeeded();
		const int lx = (int)((int64_t)x - (int64_t)_volLower.x) - _localBrickOrigin.x;
		const int ly = (int)((int64_t)y - (int64_t)_volLower.y) - _localBrickOrigin.y;
		const int lz = (int)((int64_t)z - (int64_t)_volLower.z) - _localBrickOrigin.z;
		const int idx = (lz << 6) | (ly << 3) | lx;
		Voxel merged;
		merged.overwrite(_tile[idx]);
		merged = v;
		const uint32_t newBits = priv::tileVoxelBits(merged);
		if (newBits == priv::tileVoxelBits(_tile[idx])) {
			return false;
		}
		_tile[idx].overwrite(merged);
		const uint64_t bit = (uint64_t)1u << ((ly << 3) | lx);
		const bool wasSet = (_mask[lz] & bit) != 0u;
		const bool nowSet = (newBits != _defaultBits);
		if (nowSet && !wasSet) {
			_mask[lz] |= bit;
			++_count;
		} else if (!nowSet && wasSet) {
			_mask[lz] &= ~bit;
			--_count;
		}
		_modified = true;
		return true;
	}

	/** Write voxel with full field overwrite semantics; returns true if voxel changed. */
	CORE_FORCE_INLINE bool setVoxelOverwrite(int x, int y, int z, const Voxel &v) {
		if (core_likely(_denseData != nullptr)) {
			const int64_t idx = ((int64_t)x - (int64_t)_volLower.x) +
								((int64_t)y - (int64_t)_volLower.y) * _denseW +
								((int64_t)z - (int64_t)_volLower.z) * _denseStride;
			if (priv::tileVoxelBits(_denseData[idx]) == priv::tileVoxelBits(v)) {
				return false;
			}
			_denseData[idx].overwrite(v);
			return true;
		}
		ensureSeeded();
		const int lx = (int)((int64_t)x - (int64_t)_volLower.x) - _localBrickOrigin.x;
		const int ly = (int)((int64_t)y - (int64_t)_volLower.y) - _localBrickOrigin.y;
		const int lz = (int)((int64_t)z - (int64_t)_volLower.z) - _localBrickOrigin.z;
		const int idx = (lz << 6) | (ly << 3) | lx;
		const uint32_t newBits = priv::tileVoxelBits(v);
		if (newBits == priv::tileVoxelBits(_tile[idx])) {
			return false;
		}
		_tile[idx].overwrite(v);
		const uint64_t bit = (uint64_t)1u << ((ly << 3) | lx);
		const bool wasSet = (_mask[lz] & bit) != 0u;
		const bool nowSet = (newBits != _defaultBits);
		if (nowSet && !wasSet) {
			_mask[lz] |= bit;
			++_count;
		} else if (!nowSet && wasSet) {
			_mask[lz] &= ~bit;
			--_count;
		}
		_modified = true;
		return true;
	}

	bool modified() const {
		return _modified;
	}

	RawVolume::PreparedTileBrick finalizeSparse() const {
		RawVolume::PreparedTileBrick out;
		out.chunkIndex = _sparseVol->sparseChunkIndex(_localBrickOrigin);
		out.brick = SparseChunk::prepareBrick(_localBrickOrigin.x & (SparseChunk::Side - 1),
											  _localBrickOrigin.y & (SparseChunk::Side - 1),
											  _localBrickOrigin.z & (SparseChunk::Side - 1), _tile, _mask, _count);
		return out;
	}
};

/**
 * @brief Storage-aware parallel write dispatcher for RawVolume.
 *
 * - Contiguous volumes: partitions Z-slices and writes directly into linear memory (zero staging).
 * - Sparse volumes: partitions brick-Z bands (aligned to 8), stages writes in 2 KiB L1 stack tiles,
 *   packs bricks lock-free per thread, and installs them via O(1) pointer transfer.
 */
template<typename TileFunc>
inline void forEachTileWriteParallel(RawVolume &dst, const Region &writeRegion, TileFunc &&fn) {
	if (!dst.valid() || !writeRegion.isValid()) {
		return;
	}
	Region r = writeRegion;
	if (!r.cropTo(dst.region())) {
		return;
	}
	const glm::ivec3 &volLower = dst.region().getLowerCorner();
	if (dst.isContiguous()) {
		Voxel *denseData = dst.linearVoxels();
		const int64_t denseW = dst.width();
		const int64_t denseStride = dst.region().stride();
		const int zCount = r.getDepthInVoxels();
		app::for_parallel(0, zCount, [&](int start, int end) {
			const Region band(r.getLowerX(), r.getLowerY(), r.getLowerZ() + start, r.getUpperX(), r.getUpperY(),
							  r.getLowerZ() + end - 1);
			ParallelTileWriter writer(band, denseData, volLower, denseW, denseStride);
			fn(writer);
		});
		return;
	}

	const int lx0 = (int)((int64_t)r.getLowerX() - (int64_t)volLower.x);
	const int ly0 = (int)((int64_t)r.getLowerY() - (int64_t)volLower.y);
	const int lz0 = (int)((int64_t)r.getLowerZ() - (int64_t)volLower.z);
	const int lx1 = (int)((int64_t)r.getUpperX() - (int64_t)volLower.x);
	const int ly1 = (int)((int64_t)r.getUpperY() - (int64_t)volLower.y);
	const int lz1 = (int)((int64_t)r.getUpperZ() - (int64_t)volLower.z);

	const int bxMin = lx0 >> SparseChunk::BrickShift;
	const int byMin = ly0 >> SparseChunk::BrickShift;
	const int bzMin = lz0 >> SparseChunk::BrickShift;
	const int bxMax = lx1 >> SparseChunk::BrickShift;
	const int byMax = ly1 >> SparseChunk::BrickShift;
	const int bzMax = lz1 >> SparseChunk::BrickShift;
	const Voxel defaultVoxel = dst.sparseDefaultVoxel();

	core::Lock mergeLock;
	core::DynamicArray<RawVolume::PreparedTileBrick> allPrepared;

	app::for_parallel(bzMin, bzMax + 1, [&](int bzStart, int bzEnd) {
		core::DynamicArray<RawVolume::PreparedTileBrick> localPrepared;
		for (int bz = bzStart; bz < bzEnd; ++bz) {
			const int localZ = bz << SparseChunk::BrickShift;
			const int wz0 = (int)((int64_t)volLower.z + core_max(localZ, lz0));
			const int wz1 = (int)((int64_t)volLower.z + core_min(localZ + SparseChunk::BrickSide - 1, lz1));
			for (int by = byMin; by <= byMax; ++by) {
				const int localY = by << SparseChunk::BrickShift;
				const int wy0 = (int)((int64_t)volLower.y + core_max(localY, ly0));
				const int wy1 = (int)((int64_t)volLower.y + core_min(localY + SparseChunk::BrickSide - 1, ly1));
				for (int bx = bxMin; bx <= bxMax; ++bx) {
					const int localX = bx << SparseChunk::BrickShift;
					const int wx0 = (int)((int64_t)volLower.x + core_max(localX, lx0));
					const int wx1 = (int)((int64_t)volLower.x + core_min(localX + SparseChunk::BrickSide - 1, lx1));
					const Region tileReg(wx0, wy0, wz0, wx1, wy1, wz1);
					ParallelTileWriter writer(tileReg, &dst, volLower, glm::ivec3(localX, localY, localZ),
											  defaultVoxel);
					fn(writer);
					if (writer.modified()) {
						localPrepared.push_back(writer.finalizeSparse());
					}
				}
			}
		}
		if (!localPrepared.empty()) {
			core::ScopedLock scoped(mergeLock);
			allPrepared.reserve(allPrepared.size() + localPrepared.size());
			for (const RawVolume::PreparedTileBrick &item : localPrepared) {
				allPrepared.push_back(item);
			}
		}
	});
	dst.adoptPreparedSparseBricks(allPrepared);
}

} // namespace voxel
