/**
 * @file
 */

#pragma once

#include "voxel/Connectivity.h"
#include "voxel/Voxel.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"

namespace voxel {

template<typename SAMPLER>
math::Vec3f calculateNormal(SAMPLER &sampler, voxel::Connectivity connectivity) {
	const math::Vec3i pos = sampler.position();
	math::Vec3i sum(0);
	switch (connectivity) {
	case voxel::Connectivity::TwentySixConnected:
		for (const math::Vec3i &offset : voxel::arrayPathfinderCorners) {
			const math::Vec3i &volPos = pos + offset;
			if (!sampler.setPosition(volPos)) {
				continue;
			}
			if (voxel::isBlocked(sampler.voxel().getMaterial())) {
				sum += offset;
			}
		}
		/* fallthrough */

	case voxel::Connectivity::EighteenConnected:
		for (const math::Vec3i &offset : voxel::arrayPathfinderEdges) {
			const math::Vec3i &volPos = pos + offset;
			if (!sampler.setPosition(volPos)) {
				continue;
			}
			if (voxel::isBlocked(sampler.voxel().getMaterial())) {
				sum += offset;
			}
		}
		/* fallthrough */

	case voxel::Connectivity::SixConnected:
		for (const math::Vec3i &offset : voxel::arrayPathfinderFaces) {
			const math::Vec3i &volPos = pos + offset;
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
		return math::Vec3f(0.0f);
	}
	return math::normalize(math::Vec3f(-sum));
}

} // namespace voxel
