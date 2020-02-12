/**
 * @file
 */

#include "Octree.h"
#include "math/Frustum.h"
#include "math/AABB.h"
#include "core/GLM.h"
#include "core/Log.h"
#include <glm/gtc/constants.hpp>

namespace math {

math::AABB<int> computeAABB(const Frustum& area, const glm::vec3& gridSize) {
	const AABB<float>& aabb = area.aabb();
	glm::vec3 mins = aabb.mins();
	const glm::vec3& resultMins = glm::mod(mins, gridSize);
	mins -= resultMins;

	glm::vec3 maxs = aabb.maxs();
	const glm::vec3& resultMaxs = glm::mod(maxs, gridSize);
	for (int i = 0; i < resultMaxs.length(); ++i) {
		if (glm::abs(resultMaxs[i]) > glm::epsilon<float>()) {
			maxs[i] += (gridSize[i] - resultMaxs[i]);
		}
	}

	const glm::vec3 width = maxs - mins;
	if (!glm::all(glm::epsilonEqual(width, gridSize, glm::epsilon<float>()))) {
		const glm::vec3 resultMod = glm::mod(width / gridSize, 2.0f);
		for (int i = 0; i < resultMod.length(); ++i) {
			if (glm::abs(resultMod[i]) > glm::epsilon<float>()) {
				maxs[i] += gridSize[i];
			}
		}
	}

	const AABB<int> final(mins, maxs);
	return final;
}

}
