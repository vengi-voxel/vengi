/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "voxel/Region.h"

namespace voxedit {

using Selection = voxel::Region;
using Selections = core::DynamicArray<Selection>;

inline bool contains(const Selections &selections, int x, int y, int z) {
	for (const Selection &sel : selections) {
		if (sel.containsPoint(x, y, z)) {
			return true;
		}
	}
	return false;
};

inline voxel::Region accumulate(const Selections &selections) {
	if (selections.empty()) {
		return voxel::Region::InvalidRegion;
	}
	voxel::Region r = voxel::Region::InvalidRegion;
	for (const Selection &selection : selections) {
		if (r.isValid()) {
			r.accumulate(selection);
		} else {
			r = selection;
		}
	}
	return r;
}

}
