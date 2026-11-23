/**
 * @file
 */

#include "Octree.h"
#include "math/Frustum.h"
#include "math/AABB.h"
#include "core/Log.h"
#include "math/Vec3f.h"
#include "math/Scalar.h"

namespace math {

math::AABB<int> computeAABB(const Frustum& area, const math::Vec3f& gridSize) {
	const AABB<float>& aabb = area.aabb();
	math::Vec3f mins = aabb.mins();
	const math::Vec3f& resultMins = math::mod(mins, gridSize);
	mins -= resultMins;

	math::Vec3f maxs = aabb.maxs();
	const math::Vec3f& resultMaxs = math::mod(maxs, gridSize);
	for (int i = 0; i < resultMaxs.length(); ++i) {
		if (math::abs(resultMaxs[i]) > math::epsilonf) {
			maxs[i] += (gridSize[i] - resultMaxs[i]);
		}
	}

	const math::Vec3f width = maxs - mins;
	if (!math::all(math::epsilonEqual(width, gridSize, math::epsilonf))) {
		const math::Vec3f resultMod = math::mod(width / gridSize, 2.0f);
		for (int i = 0; i < resultMod.length(); ++i) {
			if (math::abs(resultMod[i]) > math::epsilonf) {
				maxs[i] += gridSize[i];
			}
		}
	}

	const AABB<int> final(mins, maxs);
	return final;
}

}
