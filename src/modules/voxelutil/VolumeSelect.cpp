/**
 * @file
 */

#include "VolumeSelect.h"

namespace voxelutil {

bool lassoContains(const core::DynamicArray<glm::ivec3> &path, int pu, int pv, int uAxis, int vAxis) {
	const int n = (int)path.size();
	bool inside = false;
	for (int i = 0, j = n - 1; i < n; j = i++) {
		const double xi = (double)path[i][uAxis];
		const double yi = (double)path[i][vAxis];
		const double xj = (double)path[j][uAxis];
		const double yj = (double)path[j][vAxis];
		const double px = (double)pu;
		const double py = (double)pv;
		if (((yi > py) != (yj > py)) && (px < (xj - xi) * (py - yi) / (yj - yi) + xi)) {
			inside = !inside;
		}
	}
	return inside;
}

} // namespace voxelutil
