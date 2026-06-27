/**
 * @file
 */

#include "VolumeSelect.h"

namespace voxelutil {

bool lassoContains(const core::DynamicArray<glm::ivec3> &path, int pu, int pv, int uAxis, int vAxis) {
	const int n = (int)path.size();
	for (int i = 0, j = n - 1; i < n; j = i++) {
		const int xi = path[i][uAxis];
		const int yi = path[i][vAxis];
		const int xj = path[j][uAxis];
		const int yj = path[j][vAxis];
		const long long cross = (long long)(xi - xj) * (long long)(pv - yj) -
								(long long)(yi - yj) * (long long)(pu - xj);
		if (cross == 0) {
			const int minX = glm::min(xi, xj);
			const int maxX = glm::max(xi, xj);
			const int minY = glm::min(yi, yj);
			const int maxY = glm::max(yi, yj);
			if (pu >= minX && pu <= maxX && pv >= minY && pv <= maxY) {
				return true;
			}
		}
	}
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
