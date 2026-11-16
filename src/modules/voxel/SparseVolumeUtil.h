/**
 * @file
 * @brief Helpers that are shared by @c SparseVolume and @c ConcurrentSparseVolume
 */

#pragma once

#include "core/GLM.h"
#include "voxel/Region.h"
#include "voxel/SparseChunk.h"
#include "voxel/Voxel.h"

namespace voxel {
namespace sparse {

static constexpr int ChunkSide = SparseChunk::Side;
static constexpr int ChunkMask = ChunkSide - 1;

inline glm::ivec3 chunkPosition(const glm::ivec3 &pos) {
	// arithmetic shift - rounds towards negative infinity
	return glm::ivec3(pos.x >> SparseChunk::SideShift, pos.y >> SparseChunk::SideShift, pos.z >> SparseChunk::SideShift);
}

inline glm::ivec3 chunkBase(const glm::ivec3 &chunkPos) {
	return chunkPos * ChunkSide;
}

inline glm::ivec3 localPosition(const glm::ivec3 &pos) {
	return glm::ivec3(pos.x & ChunkMask, pos.y & ChunkMask, pos.z & ChunkMask);
}

/**
 * @brief Moves the start of a @c setVoxels() block to the lower corner of the given region
 * @param voxels The column - skips the entries that are below the region
 */
inline void clipToLowerCorner(const Region &region, int &x, int &y, int &z, int &nx, int &nz, const Voxel *&voxels,
							  int &amount) {
	if (!region.isValid()) {
		return;
	}
	const int xDiff = region.getLowerX() - x;
	if (xDiff > 0) {
		x += xDiff;
		nx -= xDiff;
	}
	const int yDiff = region.getLowerY() - y;
	if (yDiff > 0) {
		y += yDiff;
		voxels += yDiff;
		amount -= yDiff;
	}
	const int zDiff = region.getLowerZ() - z;
	if (zDiff > 0) {
		z += zDiff;
		nz -= zDiff;
	}
}

/**
 * @param chunkBounds Called with the chunk and the local mins and maxs output - returns @c false if the chunk doesn't
 * contain any voxel
 */
template<class ChunkMap, class BoundsFunc>
Region calculateRegion(const ChunkMap &chunks, BoundsFunc &&chunkBounds) {
	glm::ivec3 mins(0);
	glm::ivec3 maxs(0);
	bool initialized = false;
	for (auto iter = chunks.begin(); iter != chunks.end(); ++iter) {
		if (!iter->value) {
			continue;
		}
		glm::ivec3 chunkMins;
		glm::ivec3 chunkMaxs;
		if (!chunkBounds(*iter->value.get(), chunkMins, chunkMaxs)) {
			continue;
		}
		const glm::ivec3 base = chunkBase(iter->key);
		chunkMins += base;
		chunkMaxs += base;
		if (!initialized) {
			mins = chunkMins;
			maxs = chunkMaxs;
			initialized = true;
			continue;
		}
		mins = (glm::min)(mins, chunkMins);
		maxs = (glm::max)(maxs, chunkMaxs);
	}
	if (!initialized) {
		return Region::InvalidRegion;
	}
	return Region{mins, maxs};
}

/**
 * @param region Only copy the voxels inside this region - @c nullptr copies all voxels
 * @param visitChunk Called with the chunk and a functor that must be passed to @c SparseChunk::visit()
 */
template<class ChunkMap, class Volume, class VisitFunc>
void copyTo(const ChunkMap &chunks, Volume &target, const Region *region, VisitFunc &&visitChunk) {
	for (auto iter = chunks.begin(); iter != chunks.end(); ++iter) {
		if (!iter->value) {
			continue;
		}
		const glm::ivec3 base = chunkBase(iter->key);
		if (region == nullptr) {
			visitChunk(*iter->value.get(), [&target, &base](int x, int y, int z, const Voxel &voxel) {
				target.setVoxel(base.x + x, base.y + y, base.z + z, voxel);
			});
			continue;
		}
		const Region chunkRegion(base, base + (ChunkSide - 1));
		// TODO: PERF: no need to construct a full region here - add an intersects() version that takes the base and chunk size directly
		if (!intersects(*region, chunkRegion)) {
			continue;
		}
		visitChunk(*iter->value.get(), [&target, &base, region](int x, int y, int z, const Voxel &voxel) {
			const glm::ivec3 pos(base.x + x, base.y + y, base.z + z);
			if (region->containsPoint(pos)) {
				target.setVoxel(pos.x, pos.y, pos.z, voxel);
			}
		});
	}
}

} // namespace sparse
} // namespace voxel
