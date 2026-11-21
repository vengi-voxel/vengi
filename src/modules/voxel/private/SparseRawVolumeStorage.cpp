/**
 * @file
 */

#include "SparseRawVolumeStorage.h"
#include "core/Assert.h"
#include "core/Common.h"
#include "core/StandardLib.h"
#include <glm/common.hpp>

namespace voxel {

static inline bool voxelEqualsRaw(const Voxel &a, const Voxel &b) {
	return SparseRawVolumeStorage::voxelBits(a) == SparseRawVolumeStorage::voxelBits(b);
}

static inline Voxel applyFlagOp(const Voxel &v, uint8_t flags, int operation) {
	uint32_t bits = SparseRawVolumeStorage::voxelBits(v);
	const uint32_t mask = (uint32_t)(flags & 0x3u) << 2u;
	if (operation == 0) {
		bits |= mask;
	} else if (operation == 1) {
		bits &= ~mask;
	} else {
		bits ^= mask;
	}
	Voxel out;
	memcpy((void *)&out, &bits, sizeof(out));
	return out;
}

static inline bool hasAnyFlag(const Voxel &v, uint8_t flags) {
	return (v.getFlags() & (flags & 0x3u)) != 0u;
}

SparseRawVolumeStorage::SparseRawVolumeStorage(const glm::ivec3 &dimensions) : _dimensions(dimensions) {
	if (dimensions.x <= 0 || dimensions.y <= 0 || dimensions.z <= 0) {
		return;
	}
	_chunkCounts.x = (dimensions.x + ChunkSide - 1) >> ChunkShift;
	_chunkCounts.y = (dimensions.y + ChunkSide - 1) >> ChunkShift;
	_chunkCounts.z = (dimensions.z + ChunkSide - 1) >> ChunkShift;
	const size_t xy = (size_t)_chunkCounts.x * (size_t)_chunkCounts.y;
	if (xy == 0u || (size_t)_chunkCounts.z > SIZE_MAX / xy) {
		return;
	}
	_directorySize = xy * (size_t)_chunkCounts.z;
	_slots = new ChunkSlot[_directorySize]();
}

SparseRawVolumeStorage::SparseRawVolumeStorage(const SparseRawVolumeStorage &other)
	: _dimensions(other._dimensions), _chunkCounts(other._chunkCounts), _directorySize(other._directorySize),
	  _default(other._default), _defaultIsZero(other._defaultIsZero) {
	if (_directorySize == 0u || other._slots == nullptr) {
		_directorySize = 0u;
		return;
	}
	_slots = new ChunkSlot[_directorySize]();
	for (size_t i = 0u; i < _directorySize; ++i) {
		const SparseChunk *src = other._slots[i].chunk.load();
		if (src != nullptr && !src->empty()) {
			_slots[i].chunk = src->clone();
		}
	}
}

SparseRawVolumeStorage::~SparseRawVolumeStorage() {
	if (_slots != nullptr) {
		for (size_t i = 0u; i < _directorySize; ++i) {
			delete _slots[i].chunk.load();
		}
		delete[] _slots;
		_slots = nullptr;
	}
	_directorySize = 0u;
}

SparseChunk *SparseRawVolumeStorage::findOrCreateChunkLocked(ChunkSlot &slot) {
	SparseChunk *chunk = slot.chunk.load();
	if (chunk == nullptr) {
		chunk = new SparseChunk(false);
		slot.chunk = chunk;
	}
	return chunk;
}

void SparseRawVolumeStorage::releaseChunkIfEmptyLocked(ChunkSlot &slot, SparseChunk *chunk) {
	if (chunk != nullptr && chunk->empty()) {
		// Free the 64 KiB cell table and brick pool blocks immediately while keeping the chunk shell valid
		// or clearing it completely; callers holding no pointers across writes observe empty().
		chunk->clear();
	}
}

const Voxel &SparseRawVolumeStorage::voxel(int lx, int ly, int lz) const {
	core_assert(lx >= 0 && lx < _dimensions.x);
	core_assert(ly >= 0 && ly < _dimensions.y);
	core_assert(lz >= 0 && lz < _dimensions.z);
	const SparseChunk *chunk = _slots[chunkIndexForLocal(lx, ly, lz)].chunk.load();
	if (chunk == nullptr) {
		return _default;
	}
	const int clx = lx & ChunkMask;
	const int cly = ly & ChunkMask;
	const int clz = lz & ChunkMask;
	if (core_likely(_defaultIsZero)) {
		const Voxel *v = chunk->voxel(clx, cly, clz);
		return v != nullptr ? *v : _default;
	}
	const Voxel *v = chunk->findVoxel(clx, cly, clz);
	return v != nullptr ? *v : _default;
}

bool SparseRawVolumeStorage::setVoxelLocked(ChunkSlot &slot, int clx, int cly, int clz, const Voxel &value,
											bool overwrite) {
	SparseChunk *chunk = slot.chunk.load();
	Voxel existing = _default;
	const Voxel *present = chunk != nullptr ? chunk->findVoxel(clx, cly, clz) : nullptr;
	if (present != nullptr) {
		existing.overwrite(*present);
	}
	Voxel merged;
	if (overwrite) {
		merged.overwrite(value);
	} else {
		merged.overwrite(existing);
		merged = value;
	}
	if (voxelEqualsRaw(merged, existing)) {
		return false;
	}
	if (voxelEqualsRaw(merged, _default)) {
		if (chunk != nullptr) {
			chunk->removeVoxel(clx, cly, clz, true);
			releaseChunkIfEmptyLocked(slot, chunk);
			bumpGeneration();
		}
		return true;
	}
	chunk = findOrCreateChunkLocked(slot);
	SparseChunk::Brick *brick = chunk->findOrCreateBrick(clx, cly, clz);
	const int idx = SparseChunk::voxelIndex(clx, cly, clz);
	if (chunk->setVoxelOverwrite(brick, idx, merged)) {
		bumpGeneration();
	}
	return true;
}

bool SparseRawVolumeStorage::setVoxel(int lx, int ly, int lz, const Voxel &value) {
	ChunkSlot &slot = _slots[chunkIndexForLocal(lx, ly, lz)];
	core::ScopedLock scoped(slot.lock);
	return setVoxelLocked(slot, lx & ChunkMask, ly & ChunkMask, lz & ChunkMask, value, false);
}

bool SparseRawVolumeStorage::setVoxelOverwrite(int lx, int ly, int lz, const Voxel &value) {
	ChunkSlot &slot = _slots[chunkIndexForLocal(lx, ly, lz)];
	core::ScopedLock scoped(slot.lock);
	return setVoxelLocked(slot, lx & ChunkMask, ly & ChunkMask, lz & ChunkMask, value, true);
}

void SparseRawVolumeStorage::setVoxelsRow(int lx, int ly, int lz, int count, const Voxel &value) {
	if (count <= 0) {
		return;
	}
	const int cy = ly >> ChunkShift;
	const int cz = lz >> ChunkShift;
	const int cly = ly & ChunkMask;
	const int clz = lz & ChunkMask;
	const int endX = lx + count;
	int x = lx;
	while (x < endX) {
		const int cx = x >> ChunkShift;
		const int chunkEndX = (cx + 1) << ChunkShift;
		const int segEnd = endX < chunkEndX ? endX : chunkEndX;
		const int clx = x & ChunkMask;
		const int segCount = segEnd - x;
		ChunkSlot &slot = _slots[chunkIndex(cx, cy, cz)];
		core::ScopedLock scoped(slot.lock);
		if (voxelEqualsRaw(value, _default)) {
			SparseChunk *chunk = slot.chunk.load();
			if (chunk != nullptr && !chunk->empty()) {
				bool changed = false;
				for (int i = 0; i < segCount; ++i) {
					changed |= setVoxelLocked(slot, clx + i, cly, clz, value, false);
				}
				(void)changed;
			}
		} else {
			SparseChunk *chunk = findOrCreateChunkLocked(slot);
			if (_defaultIsZero) {
				if (chunk->setVoxelsRow(clx, cly, clz, segCount, value) != 0) {
					bumpGeneration();
				}
			} else {
				for (int i = 0; i < segCount; ++i) {
					setVoxelLocked(slot, clx + i, cly, clz, value, false);
				}
			}
		}
		x = segEnd;
	}
}

void SparseRawVolumeStorage::copyRowFrom(int lx, int ly, int lz, int count, const Voxel *src, bool overwrite) {
	if (count <= 0 || src == nullptr) {
		return;
	}
	const int cy = ly >> ChunkShift;
	const int cz = lz >> ChunkShift;
	const int cly = ly & ChunkMask;
	const int clz = lz & ChunkMask;
	const int endX = lx + count;
	int x = lx;
	const Voxel *srcPtr = src;
	while (x < endX) {
		const int cx = x >> ChunkShift;
		const int chunkEndX = (cx + 1) << ChunkShift;
		const int segEnd = endX < chunkEndX ? endX : chunkEndX;
		const int clx = x & ChunkMask;
		const int segCount = segEnd - x;
		ChunkSlot &slot = _slots[chunkIndex(cx, cy, cz)];
		core::ScopedLock scoped(slot.lock);
		SparseChunk *chunk = slot.chunk.load();
		// Fast skip when target chunk is absent and source row is all zero (default air)
		if (chunk == nullptr && _defaultIsZero &&
			core::memchr_not(srcPtr, 0, (size_t)segCount * sizeof(Voxel)) == nullptr) {
			srcPtr += segCount;
			x = segEnd;
			continue;
		}
		for (int i = 0; i < segCount; ++i) {
			setVoxelLocked(slot, clx + i, cly, clz, srcPtr[i], overwrite);
		}
		srcPtr += segCount;
		x = segEnd;
	}
}

void SparseRawVolumeStorage::readRegion(const glm::ivec3 &localMins, const glm::ivec3 &size, Voxel *target) const {
	if (target == nullptr || size.x <= 0 || size.y <= 0 || size.z <= 0) {
		return;
	}
	const glm::ivec3 end = localMins + size;
	const int64_t rowStride = size.x;
	const int64_t planeStride = (int64_t)size.x * size.y;

	// Prefill target with default voxel
	if (_defaultIsZero) {
		core_memset((void *)target, 0, (size_t)planeStride * (size_t)size.z * sizeof(Voxel));
	} else {
		core_memset4((void *)target, voxelBits(_default), (size_t)planeStride * (size_t)size.z);
	}

	const int cxMin = localMins.x >> ChunkShift;
	const int cyMin = localMins.y >> ChunkShift;
	const int czMin = localMins.z >> ChunkShift;
	const int cxMax = (end.x - 1) >> ChunkShift;
	const int cyMax = (end.y - 1) >> ChunkShift;
	const int czMax = (end.z - 1) >> ChunkShift;

	for (int cz = czMin; cz <= czMax; ++cz) {
		const int zBase = cz << ChunkShift;
		const int z0 = core_max(localMins.z, zBase);
		const int z1 = core_min(end.z, zBase + ChunkSide);
		for (int cy = cyMin; cy <= cyMax; ++cy) {
			const int yBase = cy << ChunkShift;
			const int y0 = core_max(localMins.y, yBase);
			const int y1 = core_min(end.y, yBase + ChunkSide);
			for (int cx = cxMin; cx <= cxMax; ++cx) {
				const SparseChunk *chunk = _slots[chunkIndex(cx, cy, cz)].chunk.load();
				if (chunk == nullptr || chunk->empty()) {
					continue;
				}
				const int xBase = cx << ChunkShift;
				const int x0 = core_max(localMins.x, xBase);
				const int x1 = core_min(end.x, xBase + ChunkSide);
				const int lx0 = x0 - xBase;
				const int ly0 = y0 - yBase;
				const int lz0 = z0 - zBase;
				const int lx1 = x1 - xBase;
				const int ly1 = y1 - yBase;
				const int lz1 = z1 - zBase;

				if (_defaultIsZero) {
					// Copy overlapping brick rows directly into target
					for (int bz = lz0; bz < lz1;) {
						const int bzEnd = core_min(lz1, (bz | BrickMask) + 1);
						for (int by = ly0; by < ly1;) {
							const int byEnd = core_min(ly1, (by | BrickMask) + 1);
							for (int bx = lx0; bx < lx1;) {
								const int bxEnd = core_min(lx1, (bx | BrickMask) + 1);
								const SparseChunk::Brick *b = chunk->brick(bx, by, bz);
								if (b != nullptr && b->count > 0) {
									const int width = bxEnd - bx;
									const int bxAligned = bx & ~BrickMask;
									const int bxOff = bx & BrickMask;
									for (int z = bz; z < bzEnd; ++z) {
										const int64_t tz = (int64_t)(zBase + z - localMins.z) * planeStride;
										for (int y = by; y < byEnd; ++y) {
											Voxel scratch[BrickSide];
											const Voxel *row = chunk->readBrickRow(bxAligned, y, z, scratch);
											Voxel *dst = target + tz + (int64_t)(yBase + y - localMins.y) * rowStride +
														 (xBase + bx - localMins.x);
											memcpy((void *)dst, (const void *)(row + bxOff),
												   (size_t)width * sizeof(Voxel));
										}
									}
								}
								bx = bxEnd;
							}
							by = byEnd;
						}
						bz = bzEnd;
					}
				} else {
					// Non-zero default: target is already filled with _default; overlay stored voxels only.
					chunk->visit([&](int vx, int vy, int vz, const Voxel &v) {
						if (vx >= lx0 && vx < lx1 && vy >= ly0 && vy < ly1 && vz >= lz0 && vz < lz1) {
							const int64_t idx = (int64_t)(zBase + vz - localMins.z) * planeStride +
												(int64_t)(yBase + vy - localMins.y) * rowStride +
												(xBase + vx - localMins.x);
							target[idx].overwrite(v);
						}
					});
				}
			}
		}
	}
}

void SparseRawVolumeStorage::copyTo(Voxel *destination) const {
	readRegion(glm::ivec3(0), _dimensions, destination);
}

void SparseRawVolumeStorage::fill(const Voxel &value) {
	for (size_t i = 0u; i < _directorySize; ++i) {
		SparseChunk *chunk = _slots[i].chunk.load();
		if (chunk != nullptr) {
			delete chunk;
			_slots[i].chunk = nullptr;
		}
	}
	_default.overwrite(value);
	_defaultIsZero = (voxelBits(_default) == 0u);
	bumpGeneration();
}

bool SparseRawVolumeStorage::isEmpty(const glm::ivec3 &localMins, const glm::ivec3 &localMaxs) const {
	if (localMins.x > localMaxs.x || localMins.y > localMaxs.y || localMins.z > localMaxs.z) {
		return true;
	}
	const int cxMin = localMins.x >> ChunkShift;
	const int cyMin = localMins.y >> ChunkShift;
	const int czMin = localMins.z >> ChunkShift;
	const int cxMax = localMaxs.x >> ChunkShift;
	const int cyMax = localMaxs.y >> ChunkShift;
	const int czMax = localMaxs.z >> ChunkShift;

	if (_defaultIsZero) {
		for (int cz = czMin; cz <= czMax; ++cz) {
			const int zBase = cz << ChunkShift;
			const int lz0 = core_max(localMins.z - zBase, 0);
			const int lz1 = core_min(localMaxs.z - zBase, ChunkMask);
			for (int cy = cyMin; cy <= cyMax; ++cy) {
				const int yBase = cy << ChunkShift;
				const int ly0 = core_max(localMins.y - yBase, 0);
				const int ly1 = core_min(localMaxs.y - yBase, ChunkMask);
				for (int cx = cxMin; cx <= cxMax; ++cx) {
					const SparseChunk *chunk = _slots[chunkIndex(cx, cy, cz)].chunk.load();
					if (chunk == nullptr || chunk->empty()) {
						continue;
					}
					const int xBase = cx << ChunkShift;
					const int lx0 = core_max(localMins.x - xBase, 0);
					const int lx1 = core_min(localMaxs.x - xBase, ChunkMask);
					bool foundNonZero = false;
					chunk->visitBricks([&](int bx, int by, int bz, const SparseChunk::Brick &b) {
						if (foundNonZero) {
							return;
						}
						if (bx + BrickMask < lx0 || bx > lx1 || by + BrickMask < ly0 || by > ly1 ||
							bz + BrickMask < lz0 || bz > lz1) {
							return;
						}
						// Fast path: brick wholly inside query region; every stored voxel is non-default (non-zero)
						if (bx >= lx0 && bx + BrickMask <= lx1 && by >= ly0 && by + BrickMask <= ly1 && bz >= lz0 &&
							bz + BrickMask <= lz1) {
							foundNonZero = true;
							return;
						}
						const int zStart = core_max(lz0 - bz, 0);
						const int zEnd = core_min(lz1 - bz, BrickMask);
						const int yStart = core_max(ly0 - by, 0);
						const int yEnd = core_min(ly1 - by, BrickMask);
						const int xStart = core_max(lx0 - bx, 0);
						const int xEnd = core_min(lx1 - bx, BrickMask);
						const uint64_t xMask = (((uint64_t)1u << (xEnd - xStart + 1)) - 1u) << xStart;
						uint64_t sliceMask = 0u;
						for (int y = yStart; y <= yEnd; ++y) {
							sliceMask |= xMask << (y * BrickSide);
						}
						for (int z = zStart; z <= zEnd; ++z) {
							if ((b.mask[z] & sliceMask) != 0u) {
								foundNonZero = true;
								return;
							}
						}
					});
					if (foundNonZero) {
						return false;
					}
				}
			}
		}
		return true;
	}

	// Non-zero default: region is empty only if every voxel inside [localMins, localMaxs] is stored as zero.
	for (int z = localMins.z; z <= localMaxs.z; ++z) {
		for (int y = localMins.y; y <= localMaxs.y; ++y) {
			for (int x = localMins.x; x <= localMaxs.x; ++x) {
				if (voxelBits(voxel(x, y, z)) != 0u) {
					return false;
				}
			}
		}
	}
	return true;
}

bool SparseRawVolumeStorage::hasFlags(const glm::ivec3 &localMins, const glm::ivec3 &localMaxs, uint8_t flags) const {
	if ((flags & 0x3u) == 0u || localMins.x > localMaxs.x || localMins.y > localMaxs.y || localMins.z > localMaxs.z) {
		return false;
	}
	const bool defaultHasFlags = hasAnyFlag(_default, flags);
	if (!defaultHasFlags) {
		const int cxMin = localMins.x >> ChunkShift;
		const int cyMin = localMins.y >> ChunkShift;
		const int czMin = localMins.z >> ChunkShift;
		const int cxMax = localMaxs.x >> ChunkShift;
		const int cyMax = localMaxs.y >> ChunkShift;
		const int czMax = localMaxs.z >> ChunkShift;
		for (int cz = czMin; cz <= czMax; ++cz) {
			const int zBase = cz << ChunkShift;
			const int lz0 = core_max(localMins.z - zBase, 0);
			const int lz1 = core_min(localMaxs.z - zBase, ChunkMask);
			for (int cy = cyMin; cy <= cyMax; ++cy) {
				const int yBase = cy << ChunkShift;
				const int ly0 = core_max(localMins.y - yBase, 0);
				const int ly1 = core_min(localMaxs.y - yBase, ChunkMask);
				for (int cx = cxMin; cx <= cxMax; ++cx) {
					const SparseChunk *chunk = _slots[chunkIndex(cx, cy, cz)].chunk.load();
					if (chunk == nullptr || chunk->empty()) {
						continue;
					}
					const int xBase = cx << ChunkShift;
					const int lx0 = core_max(localMins.x - xBase, 0);
					const int lx1 = core_min(localMaxs.x - xBase, ChunkMask);
					bool found = false;
					chunk->visitBricks([&](int bx, int by, int bz, const SparseChunk::Brick &b) {
						if (found) {
							return;
						}
						if (bx + BrickMask < lx0 || bx > lx1 || by + BrickMask < ly0 || by > ly1 ||
							bz + BrickMask < lz0 || bz > lz1) {
							return;
						}
						const bool whole = bx >= lx0 && bx + BrickMask <= lx1 && by >= ly0 && by + BrickMask <= ly1 &&
										   bz >= lz0 && bz + BrickMask <= lz1;
						if (whole && !b.dense()) {
							for (int i = 0; i < b.count; ++i) {
								if (hasAnyFlag(b.voxels[i], flags)) {
									found = true;
									return;
								}
							}
							return;
						}
						const int zStart = core_max(lz0 - bz, 0);
						const int zEnd = core_min(lz1 - bz, BrickMask);
						const int yStart = core_max(ly0 - by, 0);
						const int yEnd = core_min(ly1 - by, BrickMask);
						const int xStart = core_max(lx0 - bx, 0);
						const int xEnd = core_min(lx1 - bx, BrickMask);
						for (int z = zStart; z <= zEnd; ++z) {
							if (b.mask[z] == 0u) {
								continue;
							}
							for (int y = yStart; y <= yEnd; ++y) {
								for (int x = xStart; x <= xEnd; ++x) {
									const int idx = (z * BrickSide + y) * BrickSide + x;
									if (hasAnyFlag(SparseChunk::voxel(&b, idx), flags)) {
										found = true;
										return;
									}
								}
							}
						}
					});
					if (found) {
						return true;
					}
				}
			}
		}
		return false;
	}

	for (int z = localMins.z; z <= localMaxs.z; ++z) {
		for (int y = localMins.y; y <= localMaxs.y; ++y) {
			for (int x = localMins.x; x <= localMaxs.x; ++x) {
				if (hasAnyFlag(voxel(x, y, z), flags)) {
					return true;
				}
			}
		}
	}
	return false;
}

bool SparseRawVolumeStorage::regionForFlag(uint8_t flag, glm::ivec3 &outMins, glm::ivec3 &outMaxs) const {
	if ((flag & 0x3u) == 0u) {
		return false;
	}
	bool found = false;
	glm::ivec3 mins(_dimensions);
	glm::ivec3 maxs(-1);

	if (!hasAnyFlag(_default, flag)) {
		for (int cz = 0; cz < _chunkCounts.z; ++cz) {
			const int zBase = cz << ChunkShift;
			for (int cy = 0; cy < _chunkCounts.y; ++cy) {
				const int yBase = cy << ChunkShift;
				for (int cx = 0; cx < _chunkCounts.x; ++cx) {
					const SparseChunk *chunk = _slots[chunkIndex(cx, cy, cz)].chunk.load();
					if (chunk == nullptr || chunk->empty()) {
						continue;
					}
					const int xBase = cx << ChunkShift;
					chunk->visit([&](int vx, int vy, int vz, const Voxel &v) {
						if (!hasAnyFlag(v, flag)) {
							return;
						}
						const glm::ivec3 p(xBase + vx, yBase + vy, zBase + vz);
						if (!found) {
							mins = p;
							maxs = p;
							found = true;
						} else {
							mins = (glm::min)(mins, p);
							maxs = (glm::max)(maxs, p);
						}
					});
				}
			}
		}
		if (found) {
			outMins = mins;
			outMaxs = maxs;
		}
		return found;
	}

	// Default voxel has the flag set: scan for the bounds of voxels keeping the flag
	for (int z = 0; z < _dimensions.z; ++z) {
		for (int y = 0; y < _dimensions.y; ++y) {
			for (int x = 0; x < _dimensions.x; ++x) {
				if (!hasAnyFlag(voxel(x, y, z), flag)) {
					continue;
				}
				const glm::ivec3 p(x, y, z);
				if (!found) {
					mins = p;
					maxs = p;
					found = true;
				} else {
					mins = (glm::min)(mins, p);
					maxs = (glm::max)(maxs, p);
				}
			}
		}
	}
	if (found) {
		outMins = mins;
		outMaxs = maxs;
	}
	return found;
}

void SparseRawVolumeStorage::modifyFlags(const glm::ivec3 &localMins, const glm::ivec3 &localMaxs, uint8_t flags,
										 int operation) {
	if ((flags & 0x3u) == 0u || localMins.x > localMaxs.x || localMins.y > localMaxs.y || localMins.z > localMaxs.z) {
		return;
	}
	const Voxel newDefault = applyFlagOp(_default, flags, operation);
	const bool defaultChanges = !voxelEqualsRaw(newDefault, _default);
	const bool fullVolume = localMins.x == 0 && localMins.y == 0 && localMins.z == 0 &&
							localMaxs.x == _dimensions.x - 1 && localMaxs.y == _dimensions.y - 1 &&
							localMaxs.z == _dimensions.z - 1;

	if (!defaultChanges) {
		// Unstored cells don't change; only modify stored cells inside [localMins, localMaxs]
		const int cxMin = localMins.x >> ChunkShift;
		const int cyMin = localMins.y >> ChunkShift;
		const int czMin = localMins.z >> ChunkShift;
		const int cxMax = localMaxs.x >> ChunkShift;
		const int cyMax = localMaxs.y >> ChunkShift;
		const int czMax = localMaxs.z >> ChunkShift;
		for (int cz = czMin; cz <= czMax; ++cz) {
			const int zBase = cz << ChunkShift;
			const int lz0 = core_max(localMins.z - zBase, 0);
			const int lz1 = core_min(localMaxs.z - zBase, ChunkMask);
			for (int cy = cyMin; cy <= cyMax; ++cy) {
				const int yBase = cy << ChunkShift;
				const int ly0 = core_max(localMins.y - yBase, 0);
				const int ly1 = core_min(localMaxs.y - yBase, ChunkMask);
				for (int cx = cxMin; cx <= cxMax; ++cx) {
					ChunkSlot &slot = _slots[chunkIndex(cx, cy, cz)];
					core::ScopedLock scoped(slot.lock);
					SparseChunk *chunk = slot.chunk.load();
					if (chunk == nullptr || chunk->empty()) {
						continue;
					}
					const int xBase = cx << ChunkShift;
					const int lx0 = core_max(localMins.x - xBase, 0);
					const int lx1 = core_min(localMaxs.x - xBase, ChunkMask);
					core::DynamicArray<glm::ivec3> toRemove;
					chunk->visitBricksMutable([&](int bx, int by, int bz, SparseChunk::Brick &b) {
						if (bx + BrickMask < lx0 || bx > lx1 || by + BrickMask < ly0 || by > ly1 ||
							bz + BrickMask < lz0 || bz > lz1) {
							return;
						}
						const int zStart = core_max(lz0 - bz, 0);
						const int zEnd = core_min(lz1 - bz, BrickMask);
						const int yStart = core_max(ly0 - by, 0);
						const int yEnd = core_min(ly1 - by, BrickMask);
						const int xStart = core_max(lx0 - bx, 0);
						const int xEnd = core_min(lx1 - bx, BrickMask);
						for (int z = zStart; z <= zEnd; ++z) {
							if (b.mask[z] == 0u) {
								continue;
							}
							for (int y = yStart; y <= yEnd; ++y) {
								for (int x = xStart; x <= xEnd; ++x) {
									const int idx = (z * BrickSide + y) * BrickSide + x;
									Voxel *vp = SparseChunk::mutableVoxelIfPresent(&b, idx);
									if (vp == nullptr) {
										continue;
									}
									const Voxel updated = applyFlagOp(*vp, flags, operation);
									if (voxelEqualsRaw(updated, _default)) {
										toRemove.push_back(glm::ivec3(bx + x, by + y, bz + z));
									} else {
										vp->overwrite(updated);
									}
								}
							}
						}
					});
					for (const glm::ivec3 &rp : toRemove) {
						chunk->removeVoxel(rp.x, rp.y, rp.z, true);
					}
					if (!toRemove.empty()) {
						releaseChunkIfEmptyLocked(slot, chunk);
						bumpGeneration();
					}
				}
			}
		}
		return;
	}

	if (fullVolume) {
		// Full-volume flag operation: update _default and apply op to every stored voxel!
		_default.overwrite(newDefault);
		_defaultIsZero = (voxelBits(_default) == 0u);
		for (size_t i = 0u; i < _directorySize; ++i) {
			ChunkSlot &slot = _slots[i];
			core::ScopedLock scoped(slot.lock);
			SparseChunk *chunk = slot.chunk.load();
			if (chunk == nullptr || chunk->empty()) {
				continue;
			}
			core::DynamicArray<glm::ivec3> toRemove;
			chunk->visitBricksMutable([&](int bx, int by, int bz, SparseChunk::Brick &b) {
				for (int z = 0; z < BrickSide; ++z) {
					uint64_t bits = b.mask[z];
					while (bits != 0u) {
						const int bit = __builtin_ctzll(bits);
						bits &= bits - 1u;
						const int idx = (z << 6) | bit;
						Voxel *vp = SparseChunk::mutableVoxelIfPresent(&b, idx);
						if (vp == nullptr) {
							continue;
						}
						const Voxel updated = applyFlagOp(*vp, flags, operation);
						if (voxelEqualsRaw(updated, _default)) {
							toRemove.push_back(glm::ivec3(bx + (bit & 7), by + (bit >> 3), bz + z));
						} else {
							vp->overwrite(updated);
						}
					}
				}
			});
			for (const glm::ivec3 &rp : toRemove) {
				chunk->removeVoxel(rp.x, rp.y, rp.z, true);
			}
			if (!toRemove.empty()) {
				releaseChunkIfEmptyLocked(slot, chunk);
			}
		}
		bumpGeneration();
		return;
	}

	// Sub-region where _default itself changes: write updated voxels cell-by-cell
	for (int z = localMins.z; z <= localMaxs.z; ++z) {
		for (int y = localMins.y; y <= localMaxs.y; ++y) {
			for (int x = localMins.x; x <= localMaxs.x; ++x) {
				const Voxel updated = applyFlagOp(voxel(x, y, z), flags, operation);
				setVoxelOverwrite(x, y, z, updated);
			}
		}
	}
}

bool SparseRawVolumeStorage::computeSolidBounds(glm::ivec3 &outMins, glm::ivec3 &outMaxs) const {
	if (isAir(_default.getMaterial())) {
		bool found = false;
		glm::ivec3 mins(_dimensions);
		glm::ivec3 maxs(-1);
		for (int cz = 0; cz < _chunkCounts.z; ++cz) {
			const int zBase = cz << ChunkShift;
			for (int cy = 0; cy < _chunkCounts.y; ++cy) {
				const int yBase = cy << ChunkShift;
				for (int cx = 0; cx < _chunkCounts.x; ++cx) {
					const SparseChunk *chunk = _slots[chunkIndex(cx, cy, cz)].chunk.load();
					if (chunk == nullptr || chunk->empty()) {
						continue;
					}
					const int xBase = cx << ChunkShift;
					chunk->visitBricks([&](int bx, int by, int bz, const SparseChunk::Brick &b) {
						const int wx0 = xBase + bx;
						const int wy0 = yBase + by;
						const int wz0 = zBase + bz;
						// Skip bricks completely enclosed by current solid bounds
						if (found && wx0 >= mins.x && wx0 + BrickMask <= maxs.x && wy0 >= mins.y &&
							wy0 + BrickMask <= maxs.y && wz0 >= mins.z && wz0 + BrickMask <= maxs.z) {
							return;
						}
						const bool dense = b.dense();
						int packed = 0;
						for (int w = 0; w < SparseChunk::BrickMaskWords; ++w) {
							uint64_t bits = b.mask[w];
							while (bits != 0u) {
								const int bit = __builtin_ctzll(bits);
								bits &= bits - 1u;
								const Voxel &v = dense ? b.voxels[(w << 6) | bit] : b.voxels[packed++];
								if (isAir(v.getMaterial())) {
									continue;
								}
								const glm::ivec3 p(wx0 + (bit & 7), wy0 + (bit >> 3), wz0 + w);
								if (!found) {
									mins = p;
									maxs = p;
									found = true;
								} else {
									mins = (glm::min)(mins, p);
									maxs = (glm::max)(maxs, p);
								}
							}
						}
					});
				}
			}
		}
		if (found) {
			outMins = mins;
			outMaxs = maxs;
		}
		return found;
	}

	// Non-air default: check if any chunks exist at all (if none, full volume is solid)
	bool anyChunk = false;
	for (size_t i = 0u; i < _directorySize; ++i) {
		const SparseChunk *chunk = _slots[i].chunk.load();
		if (chunk != nullptr && !chunk->empty()) {
			anyChunk = true;
			break;
		}
	}
	if (!anyChunk) {
		outMins = glm::ivec3(0);
		outMaxs = _dimensions - 1;
		return true;
	}

	bool found = false;
	glm::ivec3 mins(_dimensions);
	glm::ivec3 maxs(-1);
	for (int z = 0; z < _dimensions.z; ++z) {
		for (int y = 0; y < _dimensions.y; ++y) {
			for (int x = 0; x < _dimensions.x; ++x) {
				if (isAir(voxel(x, y, z).getMaterial())) {
					continue;
				}
				const glm::ivec3 p(x, y, z);
				if (!found) {
					mins = p;
					maxs = p;
					found = true;
				} else {
					mins = (glm::min)(mins, p);
					maxs = (glm::max)(maxs, p);
				}
			}
		}
	}
	if (found) {
		outMins = mins;
		outMaxs = maxs;
	}
	return found;
}

bool SparseRawVolumeStorage::equals(const SparseRawVolumeStorage &other) const {
	if (_dimensions != other._dimensions) {
		return false;
	}
	const bool sameDefault = voxelEqualsRaw(_default, other._default);
	if (sameDefault) {
		for (size_t i = 0u; i < _directorySize; ++i) {
			const SparseChunk *a = _slots[i].chunk.load();
			const SparseChunk *b = other._slots[i].chunk.load();
			const bool aEmpty = (a == nullptr || a->empty());
			const bool bEmpty = (b == nullptr || b->empty());
			if (aEmpty && bEmpty) {
				continue;
			}
			if (aEmpty != bEmpty || a->size() != b->size()) {
				return false;
			}
			bool equal = true;
			a->visit([&](int vx, int vy, int vz, const Voxel &va) {
				if (!equal) {
					return;
				}
				const Voxel *vb = b->findVoxel(vx, vy, vz);
				if (vb == nullptr || !voxelEqualsRaw(va, *vb)) {
					equal = false;
				}
			});
			if (!equal) {
				return false;
			}
		}
		return true;
	}

	for (int z = 0; z < _dimensions.z; ++z) {
		for (int y = 0; y < _dimensions.y; ++y) {
			for (int x = 0; x < _dimensions.x; ++x) {
				if (!voxelEqualsRaw(voxel(x, y, z), other.voxel(x, y, z))) {
					return false;
				}
			}
		}
	}
	return true;
}

bool SparseRawVolumeStorage::equalsDense(const Voxel *denseData) const {
	if (denseData == nullptr) {
		return false;
	}
	const int64_t w = _dimensions.x;
	const int64_t stride = (int64_t)_dimensions.x * _dimensions.y;
	if (_defaultIsZero) {
		// Compare chunk-by-chunk, skipping empty chunks with fast memchr_not!
		for (int cz = 0; cz < _chunkCounts.z; ++cz) {
			const int z0 = cz << ChunkShift;
			const int z1 = core_min(z0 + ChunkSide, _dimensions.z);
			for (int cy = 0; cy < _chunkCounts.y; ++cy) {
				const int y0 = cy << ChunkShift;
				const int y1 = core_min(y0 + ChunkSide, _dimensions.y);
				for (int cx = 0; cx < _chunkCounts.x; ++cx) {
					const int x0 = cx << ChunkShift;
					const int x1 = core_min(x0 + ChunkSide, _dimensions.x);
					const int spanW = x1 - x0;
					const SparseChunk *chunk = _slots[chunkIndex(cx, cy, cz)].chunk.load();
					if (chunk == nullptr || chunk->empty()) {
						for (int z = z0; z < z1; ++z) {
							const int64_t zBase = (int64_t)z * stride + x0;
							for (int y = y0; y < y1; ++y) {
								if (core::memchr_not(denseData + zBase + (int64_t)y * w, 0,
													 (size_t)spanW * sizeof(Voxel)) != nullptr) {
									return false;
								}
							}
						}
						continue;
					}
					for (int z = z0; z < z1; ++z) {
						const int64_t zBase = (int64_t)z * stride + x0;
						const int clz = z & ChunkMask;
						for (int y = y0; y < y1; ++y) {
							const int cly = y & ChunkMask;
							const Voxel *denseRow = denseData + zBase + (int64_t)y * w;
							for (int x = 0; x < spanW;) {
								const int clx = (x0 + x) & ChunkMask;
								const int bSpan = core_min(spanW - x, BrickSide - (clx & BrickMask));
								Voxel scratch[BrickSide];
								const Voxel *row = chunk->readBrickRow(clx & ~BrickMask, cly, clz, scratch);
								if (memcmp(denseRow + x, row + (clx & BrickMask), (size_t)bSpan * sizeof(Voxel)) != 0) {
									return false;
								}
								x += bSpan;
							}
						}
					}
				}
			}
		}
		return true;
	}

	for (int z = 0; z < _dimensions.z; ++z) {
		const int64_t zBase = (int64_t)z * stride;
		for (int y = 0; y < _dimensions.y; ++y) {
			const int64_t rowBase = zBase + (int64_t)y * w;
			for (int x = 0; x < _dimensions.x; ++x) {
				if (!voxelEqualsRaw(voxel(x, y, z), denseData[rowBase + x])) {
					return false;
				}
			}
		}
	}
	return true;
}

bool SparseRawVolumeStorage::move(const glm::ivec3 &shift) {
	const int w = _dimensions.x;
	const int h = _dimensions.y;
	const int d = _dimensions.z;
	glm::ivec3 t;
	t.x = (shift.x % w + w) % w;
	t.y = (shift.y % h + h) % h;
	t.z = (shift.z % d + d) % d;
	if (t.x == 0 && t.y == 0 && t.z == 0) {
		return true;
	}
	SparseRawVolumeStorage shifted(_dimensions);
	if (!shifted.valid()) {
		return false;
	}
	shifted._default.overwrite(_default);
	shifted._defaultIsZero = _defaultIsZero;

	for (int cz = 0; cz < _chunkCounts.z; ++cz) {
		const int zBase = cz << ChunkShift;
		for (int cy = 0; cy < _chunkCounts.y; ++cy) {
			const int yBase = cy << ChunkShift;
			for (int cx = 0; cx < _chunkCounts.x; ++cx) {
				const SparseChunk *chunk = _slots[chunkIndex(cx, cy, cz)].chunk.load();
				if (chunk == nullptr || chunk->empty()) {
					continue;
				}
				const int xBase = cx << ChunkShift;
				chunk->visit([&](int vx, int vy, int vz, const Voxel &v) {
					int nx = xBase + vx - t.x;
					if (nx < 0) {
						nx += w;
					}
					int ny = yBase + vy - t.y;
					if (ny < 0) {
						ny += h;
					}
					int nz = zBase + vz - t.z;
					if (nz < 0) {
						nz += d;
					}
					shifted.setVoxelOverwrite(nx, ny, nz, v);
				});
			}
		}
	}

	// Swap chunk pointers into our slots
	for (size_t i = 0u; i < _directorySize; ++i) {
		SparseChunk *oldChunk = _slots[i].chunk.load();
		SparseChunk *newChunk = shifted._slots[i].chunk.load();
		_slots[i].chunk = newChunk;
		shifted._slots[i].chunk = oldChunk;
	}
	bumpGeneration();
	return true;
}

void SparseRawVolumeStorage::sampleAt(int lx, int ly, int lz, const SparseChunk *&outChunk,
									  const SparseChunk::Brick *&outBrick, const Voxel *&outRow, bool &outWholeBrick,
									  glm::ivec3 &outLower, glm::ivec3 &outUpper, Voxel &outFallbackVoxel,
									  uint32_t &outGen) const {
	outGen = generation();
	outChunk = _slots[chunkIndexForLocal(lx, ly, lz)].chunk.load();
	const int clx = lx & ChunkMask;
	const int cly = ly & ChunkMask;
	const int clz = lz & ChunkMask;
	outBrick = outChunk != nullptr ? outChunk->brick(clx, cly, clz) : nullptr;
	if (outChunk == nullptr) {
		// Entire 256^3 chunk is uniform default!
		outLower = glm::ivec3(lx & ~ChunkMask, ly & ~ChunkMask, lz & ~ChunkMask);
		outUpper = glm::ivec3(core_min(lx | ChunkMask, _dimensions.x - 1),
							  core_min(ly | ChunkMask, _dimensions.y - 1),
							  core_min(lz | ChunkMask, _dimensions.z - 1));
		outWholeBrick = true;
		outRow = nullptr;
		outFallbackVoxel.overwrite(_default);
		return;
	}
	outLower = glm::ivec3(lx & ~BrickMask, ly & ~BrickMask, lz & ~BrickMask);
	outUpper = glm::ivec3(core_min(lx | BrickMask, _dimensions.x - 1), core_min(ly | BrickMask, _dimensions.y - 1),
						  core_min(lz | BrickMask, _dimensions.z - 1));

	if (outBrick == nullptr) {
		// Entire 8^3 brick is uniform default!
		outWholeBrick = true;
		outRow = nullptr;
		outFallbackVoxel.overwrite(_default);
		return;
	}

	if (core_likely(_defaultIsZero)) {
		if (outBrick->dense()) {
			outWholeBrick = true;
			outRow = outBrick->voxels + SparseChunk::voxelIndex(0, cly, clz);
			return;
		}
		const int idx = SparseChunk::voxelIndex(clx, cly, clz);
		outWholeBrick = false;
		outRow = SparseChunk::borrowBrickRow(outBrick, idx);
		if (outRow == nullptr) {
			outFallbackVoxel.overwrite(SparseChunk::voxel(outBrick, idx));
		}
		return;
	}

	// Non-zero default with present brick
	outWholeBrick = false;
	outRow = nullptr;
	const Voxel *v = outChunk->findVoxel(clx, cly, clz);
	outFallbackVoxel.overwrite(v != nullptr ? *v : _default);
}

size_t SparseRawVolumeStorage::allocatedBytes() const {
	size_t total = _directorySize * sizeof(ChunkSlot);
	for (size_t i = 0u; i < _directorySize; ++i) {
		const SparseChunk *chunk = _slots[i].chunk.load();
		if (chunk != nullptr) {
			total += sizeof(SparseChunk) + chunk->allocatedBytes();
		}
	}
	return total;
}

} // namespace voxel
