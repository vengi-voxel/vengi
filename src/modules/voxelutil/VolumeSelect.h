/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include <glm/common.hpp>
#include <glm/vec3.hpp>

namespace voxel {
class RawVolume;
}

namespace voxelutil {

/**
 * @brief Point-in-polygon test using ray casting in 2D face-plane coordinates
 * @param path Polygon vertices
 * @param pu U-axis coordinate of the test point
 * @param pv V-axis coordinate of the test point
 * @param uAxis Index of the U axis in ivec3 (0=x,1=y,2=z)
 * @param vAxis Index of the V axis in ivec3
 */
bool lassoContains(const core::DynamicArray<glm::ivec3> &path, int pu, int pv, int uAxis, int vAxis);

/**
 * @brief Draw the lasso edge between two vertices by scanning the face-normal column
 *        at each Bresenham step and marking the outermost surface voxel.
 * @param sampler Callable(glm::ivec3) -> voxel::Voxel used to sample the volume
 * @param markFunc Callable(int x, int y, int z, voxel::Voxel) called for each hit surface voxel
 */
template<typename Sampler, typename MarkFunc>
void drawLassoEdgeSurface(Sampler &&sampler, const glm::ivec3 &a, const glm::ivec3 &b,
						  int uAxis, int vAxis, int wAxis, bool positiveNormal,
						  const voxel::Region &region, MarkFunc &&markFunc) {
	const int wLo = region.getLowerCorner()[wAxis];
	const int wHi = region.getUpperCorner()[wAxis];

	int u = a[uAxis];
	int v = a[vAxis];
	const int du = glm::abs(b[uAxis] - u);
	const int dv = glm::abs(b[vAxis] - v);
	const int su = a[uAxis] < b[uAxis] ? 1 : -1;
	const int sv = a[vAxis] < b[vAxis] ? 1 : -1;
	int err = du - dv;

	while (true) {
		glm::ivec3 pos;
		pos[uAxis] = u;
		pos[vAxis] = v;
		// Scan from the face inward along W to find the outermost surface voxel
		if (positiveNormal) {
			for (int w = wHi; w >= wLo; --w) {
				pos[wAxis] = w;
				const voxel::Voxel vox = sampler(pos);
				if (!voxel::isAir(vox.getMaterial())) {
					markFunc(pos.x, pos.y, pos.z, vox);
					break;
				}
			}
		} else {
			for (int w = wLo; w <= wHi; ++w) {
				pos[wAxis] = w;
				const voxel::Voxel vox = sampler(pos);
				if (!voxel::isAir(vox.getMaterial())) {
					markFunc(pos.x, pos.y, pos.z, vox);
					break;
				}
			}
		}

		if (u == b[uAxis] && v == b[vAxis]) {
			break;
		}
		const int e2 = 2 * err;
		if (e2 > -dv) {
			err -= dv;
			u += su;
		}
		if (e2 < du) {
			err += du;
			v += sv;
		}
	}
}

/**
 * @brief Calculate the bounding region of all voxels that have the given flag set
 * @param volume The volume to scan
 * @param flag The voxel flag bitmask to search for (e.g. @c voxel::FlagOutline)
 * @return The bounding region of matching voxels, or @c voxel::Region::InvalidRegion if none found
 */
voxel::Region regionForFlag(const voxel::RawVolume &volume, uint8_t flag);

} // namespace voxelutil
