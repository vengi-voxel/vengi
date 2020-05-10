/**
 * @file
 */

#pragma once

#include "voxel/Voxel.h"
#include "voxel/Constants.h"

namespace voxelutil {

struct FloorTraceResult {
	constexpr FloorTraceResult() : heightLevel(voxel::NO_FLOOR_FOUND), voxel() {
	}
	FloorTraceResult(int y, const voxel::Voxel &v) : heightLevel(y), voxel(v) {
	}
	int heightLevel;
	voxel::Voxel voxel;

	bool isValid() const {
		return heightLevel != voxel::NO_FLOOR_FOUND;
	}
};

}