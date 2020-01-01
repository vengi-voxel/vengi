/**
 * @file
 */

#pragma once

#include "voxel/RawVolume.h"
#include "core/Common.h"
#include "core/Trace.h"

namespace voxel {

/**
 * @brief Will skip air voxels on volume
 */
struct SkipEmpty {
	inline bool operator() (const voxel::Voxel& voxel) const {
		return !isAir(voxel.getMaterial());
	}
};

template<class Volume, class Visitor, typename Condition = SkipEmpty>
int visitVolume(const Volume& volume, Visitor&& visitor, Condition condition = Condition()) {
	core_trace_scoped(VisitVolume);
	const voxel::Region& region = volume.region();
	int cnt = 0;
	for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
		for (int32_t y = region.getLowerY(); y <= region.getUpperY(); ++y) {
			for (int32_t x = region.getLowerX(); x <= region.getUpperX(); ++x) {
				const Voxel& voxel = volume.voxel(x, y, z);
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

}
