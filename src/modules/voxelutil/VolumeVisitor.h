/**
 * @file
 */

#pragma once

#include "voxel/RawVolume.h"
#include "core/Common.h"
#include "core/Trace.h"

namespace voxelutil {

/**
 * @brief Will skip air voxels on volume
 */
struct SkipEmpty {
	inline bool operator() (const voxel::Voxel& voxel) const {
		return !isAir(voxel.getMaterial());
	}
};

template<class Volume, class Visitor, typename Condition = SkipEmpty>
int visitVolume(const Volume& volume, const voxel::Region& region, int xOff, int yOff, int zOff, Visitor&& visitor, Condition condition = Condition()) {
	core_trace_scoped(VisitVolume);
	int cnt = 0;
	for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
		for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
			for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
				const voxel::Voxel& voxel = volume.voxel(x, y, z);
				if (!condition(voxel)) {
					continue;
				}
				visitor(x, y, z, voxel);
				++cnt;
			}
		}
	}
	return cnt;
}

template<class Volume, class Visitor, typename Condition = SkipEmpty>
int visitVolume(const Volume& volume, int xOff, int yOff, int zOff, Visitor&& visitor, Condition condition = Condition()) {
	const voxel::Region& region = volume.region();
	return visitVolume(volume, region, xOff, yOff, zOff, visitor, condition);
}

template<class Volume, class Visitor, typename Condition = SkipEmpty>
int visitVolume(const Volume& volume, Visitor&& visitor, Condition condition = Condition()) {
	return visitVolume(volume, 1, 1, 1, visitor, condition);
}

template<class Volume, class Visitor, typename Condition = SkipEmpty>
int visitVolume(const Volume& volume, const voxel::Region& region, Visitor&& visitor, Condition condition = Condition()) {
	return visitVolume(volume, region, 1, 1, 1, visitor, condition);
}

}
