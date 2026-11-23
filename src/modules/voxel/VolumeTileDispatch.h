/**
 * @file
 */

#pragma once

#include "app/ForParallel.h"
#include "core/Common.h"
#include "core/StandardLib.h"
#include "core/Trace.h"
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
		  _defaultBits(priv::tileVoxelBits(defaultVoxel)) {
	}

	CORE_FORCE_INLINE const Region &region() const {
		return _tileRegion;
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

	core_trace_mutex(core::Lock, mergeLock, "VolumeTileDispatchMerge");
	core::DynamicArray<RawVolume::PreparedTileBrick> allPrepared;

	app::for_parallel(bzMin, bzMax + 1, [&](int bzStart, int bzEnd) {
		core::DynamicArray<RawVolume::PreparedTileBrick, 256u> localPrepared;
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

/**
 * @brief Lock-free parallel in-place mutation of all non-air voxels in @p region.
 *
 * Callback signature: @c void fn(int x, int y, int z, Voxel &voxel)
 * Requirements: @p fn must NOT change occupancy (must keep non-air voxels non-air, e.g. recoloring/shading/flags).
 */
template<typename VoxelFunc>
inline void forEachOccupiedVoxelParallel(RawVolume &vol, const Region &region, VoxelFunc &&fn) {
	if (!vol.valid() || !region.isValid()) {
		return;
	}
	Region r = region;
	if (!r.cropTo(vol.region())) {
		return;
	}
	const glm::ivec3 &volLower = vol.region().getLowerCorner();
	if (vol.isContiguous()) {
		Voxel *data = vol.linearVoxels();
		const int64_t w = vol.width();
		const int64_t stride = vol.region().stride();
		const int zCount = r.getDepthInVoxels();
		const int yCount = r.getHeightInVoxels();
		const int xCount = r.getWidthInVoxels();
		const int64_t xOff = (int64_t)r.getLowerX() - (int64_t)volLower.x;
		const int64_t yOff = (int64_t)r.getLowerY() - (int64_t)volLower.y;
		const int64_t zOff = (int64_t)r.getLowerZ() - (int64_t)volLower.z;
		app::for_parallel(0, zCount, [&](int start, int end) {
			for (int z = start; z < end; ++z) {
				const int wz = r.getLowerZ() + z;
				const int64_t zBase = (zOff + z) * stride + xOff;
				for (int y = 0; y < yCount; ++y) {
					const int wy = r.getLowerY() + y;
					Voxel *row = data + zBase + (yOff + y) * w;
					for (int x = 0; x < xCount; ++x) {
						if (!isAir(row[x].getMaterial())) {
							fn(r.getLowerX() + x, wy, wz, row[x]);
						}
					}
				}
			}
		});
		return;
	}

	if (vol.isSparseAirDefault()) {
		core::DynamicArray<RawVolume::OccupiedBrickSpan> bricks;
		vol.collectOccupiedSparseBricks(r, bricks);
		if (bricks.empty()) {
			return;
		}
		const int rMinX = r.getLowerX();
		const int rMaxX = r.getUpperX();
		const int rMinY = r.getLowerY();
		const int rMaxY = r.getUpperY();
		const int rMinZ = r.getLowerZ();
		const int rMaxZ = r.getUpperZ();
		app::for_parallel(0, (int)bricks.size(), [&](int start, int end) {
			for (int i = start; i < end; ++i) {
				const RawVolume::OccupiedBrickSpan &span = bricks[i];
				SparseChunk::Brick *b = span.brick;
				const int ox = span.worldOrigin.x;
				const int oy = span.worldOrigin.y;
				const int oz = span.worldOrigin.z;
				const bool fullInside = (ox >= rMinX && ox + 7 <= rMaxX && oy >= rMinY && oy + 7 <= rMaxY &&
										 oz >= rMinZ && oz + 7 <= rMaxZ);
				if (b->count == SparseChunk::BrickVoxels && fullInside) {
					Voxel *voxels = b->voxels;
					for (int lz = 0; lz < SparseChunk::BrickSide; ++lz) {
						const int wz = oz + lz;
						for (int ly = 0; ly < SparseChunk::BrickSide; ++ly) {
							const int wy = oy + ly;
							Voxel *row = voxels + ((lz << 6) | (ly << 3));
							for (int lx = 0; lx < SparseChunk::BrickSide; ++lx) {
								if (!isAir(row[lx].getMaterial())) {
									fn(ox + lx, wy, wz, row[lx]);
								}
							}
						}
					}
					continue;
				}
				const bool dense = b->dense();
				int packed = 0;
				for (int w = 0; w < SparseChunk::BrickMaskWords; ++w) {
					const int wz = oz + w;
					uint64_t bits = b->mask[w];
					if (bits == 0u) {
						continue;
					}
					if (wz < rMinZ || wz > rMaxZ) {
						if (!dense) {
							packed += SparseChunk::popcount64(bits);
						}
						continue;
					}
					while (bits != 0u) {
						const int bit = core::countTrailingZeros(bits);
						bits &= bits - 1u;
						Voxel &v = dense ? b->voxels[(w << 6) | bit] : b->voxels[packed++];
						const int wx = ox + (bit & 7);
						const int wy = oy + (bit >> 3);
						if ((fullInside || (wx >= rMinX && wx <= rMaxX && wy >= rMinY && wy <= rMaxY)) &&
							!isAir(v.getMaterial())) {
							fn(wx, wy, wz, v);
						}
					}
				}
			}
		});
		vol.notifySparseModified();
		return;
	}

	// Non-air default sparse fallback: safe tile-based parallel write
	forEachTileWriteParallel(vol, r, [&](ParallelTileWriter &tile) {
		const Region &tr = tile.region();
		for (int z = tr.getLowerZ(); z <= tr.getUpperZ(); ++z) {
			for (int y = tr.getLowerY(); y <= tr.getUpperY(); ++y) {
				for (int x = tr.getLowerX(); x <= tr.getUpperX(); ++x) {
					Voxel v = vol.voxel(x, y, z);
					if (!isAir(v.getMaterial())) {
						Voxel copy = v;
						fn(x, y, z, copy);
						if (!(copy == v)) {
							tile.setVoxelOverwrite(x, y, z, copy);
						}
					}
				}
			}
		}
	});
}

} // namespace voxel
