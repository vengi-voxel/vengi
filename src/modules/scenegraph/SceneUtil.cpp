/**
 * @file
 */

#include "SceneUtil.h"

namespace scenegraph {

voxel::Region toRegion(const math::OBB<float> &obb) {
	return toRegion(toAABB(obb));
}

voxel::Region toRegion(const math::AABB<float> &aabb) {
	return voxel::Region(glm::floor(aabb.getLowerCorner()), glm::ceil(aabb.getUpperCorner() - 1.0f));
}

math::AABB<float> toAABB(const math::OBB<float> &obb) {
	const glm::vec3 &origin = obb.origin();
	const glm::mat3 &rotation = obb.rotation();
	const glm::vec3 &extends = obb.extents();
	glm::vec3 mins = origin;
	glm::vec3 maxs = origin;
	for (int i = 0; i < 3; ++i) {
		glm::vec3 worldAxis(0.0f);
		worldAxis[i] = 1.0f;

		// Project each OBB axis onto the world axis and compute the contribution
		float extent = glm::abs(glm::dot(rotation[0], worldAxis)) * extends.x +
					   glm::abs(glm::dot(rotation[1], worldAxis)) * extends.y +
					   glm::abs(glm::dot(rotation[2], worldAxis)) * extends.z;

		mins[i] = origin[i] - extent;
		maxs[i] = origin[i] + extent;
	}
	return {mins, maxs};
}

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
