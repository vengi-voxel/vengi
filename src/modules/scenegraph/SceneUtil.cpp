/**
 * @file
 */

#include "SceneUtil.h"

namespace scenegraph {

math::AABB<float> toAABB(const voxel::Region &region) {
	if (region.isValid()) {
		return math::AABB<float>(glm::floor(region.getLowerCornerf()),
								 glm::floor(glm::vec3(region.getUpperCornerf() + 1.0f)));
	}
	return math::AABB<float>(1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f);
}

math::OBB<float> toOBB(bool sceneMode, const voxel::Region &region, const glm::vec3 &normalizedPivot,
					   const scenegraph::FrameTransform &transform) {
	core_assert(region.isValid());
	if (sceneMode) {
		const glm::vec3 pivot = calculateWorldPivot(transform, normalizedPivot, region.getDimensionsInVoxels());
		const glm::vec3 extents = calculateExtents(region.getDimensionsInVoxels());
		const glm::vec3 center = calculateCenter(transform, pivot, region.calcCenterf());
		const glm::mat3x3 &matrix = transform.matrix;
		return math::OBB<float>(center, extents, matrix);
	}
	return math::OBB<float>(glm::floor(region.getLowerCornerf()),
							glm::floor(glm::vec3(region.getUpperCornerf() + 1.0f)));
}

} // namespace scenegraph
