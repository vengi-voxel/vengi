/**
 * @file
 */

#pragma once

#include "voxel/Connectivity.h"
#include "voxel/Region.h"

namespace voxelutil {

/**
 * @brief Visit cells in an inclusive region where @a inside is true.
 * @a shell keeps a cell only when one of its six face neighbors fails @a inside.
 * Those neighbors are tested even when they lie outside the region, so a solid region becomes a one-voxel shell.
 * @return number of visited cells
 */
template<class INSIDE, class VISITOR>
inline int visitInside(const voxel::Region &region, bool shell, INSIDE &&inside, VISITOR &&visitor) {
	if (!region.isValid()) {
		return 0;
	}
	int visited = 0;
	const glm::ivec3 &lo = region.getLowerCorner();
	const glm::ivec3 &hi = region.getUpperCorner();
	for (int z = lo.z; z <= hi.z; ++z) {
		for (int y = lo.y; y <= hi.y; ++y) {
			for (int x = lo.x; x <= hi.x; ++x) {
				if (!inside(x, y, z)) {
					continue;
				}
				if (shell) {
					bool onShell = false;
					for (const glm::ivec3 &off : voxel::arrayPathfinderFaces) {
						if (!inside(x + off.x, y + off.y, z + off.z)) {
							onShell = true;
							break;
						}
					}
					if (!onShell) {
						continue;
					}
				}
				visitor(x, y, z);
				++visited;
			}
		}
	}
	return visited;
}

} // namespace voxelutil
