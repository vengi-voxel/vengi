/**
 * @file
 */

#include "voxel/Connectivity.h"
#include "voxel/Voxel.h"
#include <glm/geometric.hpp>

namespace voxel {

template<typename SAMPLER>
glm::vec3 calculateNormal(SAMPLER &sampler, voxel::Connectivity connectivity) {
	const glm::ivec3 pos = sampler.position();
	glm::ivec3 sum(0);
	switch (connectivity) {
	case voxel::Connectivity::TwentySixConnected:
		for (const glm::ivec3 &offset : voxel::arrayPathfinderCorners) {
			const glm::ivec3 &volPos = pos + offset;
			if (!sampler.setPosition(volPos)) {
				continue;
			}
			if (voxel::isBlocked(sampler.voxel().getMaterial())) {
				sum += offset;
			}
		}
		/* fallthrough */

	case voxel::Connectivity::EighteenConnected:
		for (const glm::ivec3 &offset : voxel::arrayPathfinderEdges) {
			const glm::ivec3 &volPos = pos + offset;
			if (!sampler.setPosition(volPos)) {
				continue;
			}
			if (voxel::isBlocked(sampler.voxel().getMaterial())) {
				sum += offset;
			}
		}
		/* fallthrough */

	case voxel::Connectivity::SixConnected:
		for (const glm::ivec3 &offset : voxel::arrayPathfinderFaces) {
			const glm::ivec3 &volPos = pos + offset;
			if (!sampler.setPosition(volPos)) {
				continue;
			}
			if (voxel::isBlocked(sampler.voxel().getMaterial())) {
				sum += offset;
			}
		}
		break;
	}
	if (sum.x == 0 && sum.y == 0 && sum.z == 0) {
		return glm::vec3(0.0f);
	}
	return glm::normalize(glm::vec3(sum));
}

} // namespace voxel
