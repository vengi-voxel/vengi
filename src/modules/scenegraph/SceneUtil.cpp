/**
 * @file
 */

#include "SceneUtil.h"

namespace scenegraph {

voxel::Region toRegion(const math::OBBF &obb) {
	return toRegion(toAABB(obb));
}

voxel::Region toRegion(const math::AABB<float> &aabb) {
	return voxel::Region(glm::floor(aabb.getLowerCorner()), glm::ceil(aabb.getUpperCorner() - 1.0f));
}

math::AABB<float> toAABB(const math::OBBF &obb) {
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

/**
 * @sa scenegraph::SceneGraph::worldMatrix()
 */
math::OBBF toOBB(bool sceneMode, const voxel::Region &region, const glm::vec3 &normalizedPivot,
					   const scenegraph::FrameTransform &transform) {
	core_assert(region.isValid());
	if (sceneMode) {
		const glm::vec3 extents = calculateExtents(region.getDimensionsInVoxels());
		const glm::mat4 &worldMatrix = transform.calculateWorldMatrix(normalizedPivot, region.getDimensionsInVoxels());
		const glm::vec3 center = worldMatrix * glm::vec4(region.calcCenterf(), 1.0f);
		return math::OBBF(center, extents, worldMatrix);
	}
	return math::OBBF(glm::floor(region.getLowerCornerf()),
							glm::floor(glm::vec3(region.getUpperCornerf() + 1.0f)));
}

static glm::vec3 obbLocalToWorld(const math::OBBF &obb, const glm::vec3 &local) {
	const glm::mat3 rot(obb.rotation());
	return obb.origin() + rot * local;
}

voxel::Region calcAdjacentRegion(const voxel::Region &sourceWorld, voxel::FaceNames face,
								 const glm::ivec3 &newDims) {
	core_assert(sourceWorld.isValid());
	const glm::ivec3 maxOffset = newDims - glm::ivec3(1);
	const glm::ivec3 &sourceLower = sourceWorld.getLowerCorner();
	const glm::ivec3 &sourceUpper = sourceWorld.getUpperCorner();
	glm::ivec3 lower = sourceLower;
	switch (face) {
	case voxel::FaceNames::PositiveX:
		lower = glm::ivec3(sourceUpper.x + 1, sourceLower.y, sourceLower.z);
		break;
	case voxel::FaceNames::NegativeX:
		lower = glm::ivec3(sourceLower.x - newDims.x, sourceLower.y, sourceLower.z);
		break;
	case voxel::FaceNames::PositiveY:
		lower = glm::ivec3(sourceLower.x, sourceUpper.y + 1, sourceLower.z);
		break;
	case voxel::FaceNames::NegativeY:
		lower = glm::ivec3(sourceLower.x, sourceLower.y - newDims.y, sourceLower.z);
		break;
	case voxel::FaceNames::PositiveZ:
		lower = glm::ivec3(sourceLower.x, sourceLower.y, sourceUpper.z + 1);
		break;
	case voxel::FaceNames::NegativeZ:
		lower = glm::ivec3(sourceLower.x, sourceLower.y, sourceLower.z - newDims.z);
		break;
	case voxel::FaceNames::Max:
		return voxel::Region::InvalidRegion;
	}
	return voxel::Region(lower, lower + maxOffset);
}

math::OBBF calcAdjacentObb(const math::OBBF &source, voxel::FaceNames face, const glm::vec3 &newExtents) {
	const glm::vec3 &sourceExtents = source.extents();
	glm::vec3 localOffset(0.0f);
	switch (face) {
	case voxel::FaceNames::PositiveX:
		localOffset.x = sourceExtents.x + newExtents.x;
		break;
	case voxel::FaceNames::NegativeX:
		localOffset.x = -(sourceExtents.x + newExtents.x);
		break;
	case voxel::FaceNames::PositiveY:
		localOffset.y = sourceExtents.y + newExtents.y;
		break;
	case voxel::FaceNames::NegativeY:
		localOffset.y = -(sourceExtents.y + newExtents.y);
		break;
	case voxel::FaceNames::PositiveZ:
		localOffset.z = sourceExtents.z + newExtents.z;
		break;
	case voxel::FaceNames::NegativeZ:
		localOffset.z = -(sourceExtents.z + newExtents.z);
		break;
	case voxel::FaceNames::Max:
		return math::OBBF(glm::vec3(0.0f), glm::vec3(0.0f), glm::mat3(1.0f));
	}
	const glm::mat3 rot(source.rotation());
	const glm::vec3 newOrigin = source.origin() + rot * localOffset;
	return math::OBBF(newOrigin, newExtents, rot);
}

ObbFaceHit traceObbFace(const math::OBBF &obb, const math::Ray &ray) {
	ObbFaceHit hit;
	float distance = 0.0f;
	if (!obb.intersect(ray.origin, ray.direction, distance)) {
		return hit;
	}
	hit.distance = distance;
	hit.hitPoint = ray.origin + ray.direction * distance;

	const glm::mat3 invRot = glm::inverse(glm::mat3(obb.rotation()));
	const glm::vec3 localHit = invRot * (hit.hitPoint - obb.origin());
	const glm::vec3 &extents = obb.extents();
	const float dx = extents.x - glm::abs(localHit.x);
	const float dy = extents.y - glm::abs(localHit.y);
	const float dz = extents.z - glm::abs(localHit.z);
	if (dx <= dy && dx <= dz) {
		hit.face = localHit.x >= 0.0f ? voxel::FaceNames::PositiveX : voxel::FaceNames::NegativeX;
	} else if (dy <= dx && dy <= dz) {
		hit.face = localHit.y >= 0.0f ? voxel::FaceNames::PositiveY : voxel::FaceNames::NegativeY;
	} else {
		hit.face = localHit.z >= 0.0f ? voxel::FaceNames::PositiveZ : voxel::FaceNames::NegativeZ;
	}
	return hit;
}

void obbFaceQuad(const math::OBBF &obb, voxel::FaceNames face, glm::vec3 quad[4]) {
	const glm::vec3 &extents = obb.extents();
	const float ex = extents.x;
	const float ey = extents.y;
	const float ez = extents.z;
	switch (face) {
	case voxel::FaceNames::PositiveX:
		quad[0] = obbLocalToWorld(obb, glm::vec3(ex, -ey, -ez));
		quad[1] = obbLocalToWorld(obb, glm::vec3(ex, ey, -ez));
		quad[2] = obbLocalToWorld(obb, glm::vec3(ex, ey, ez));
		quad[3] = obbLocalToWorld(obb, glm::vec3(ex, -ey, ez));
		break;
	case voxel::FaceNames::NegativeX:
		quad[0] = obbLocalToWorld(obb, glm::vec3(-ex, -ey, ez));
		quad[1] = obbLocalToWorld(obb, glm::vec3(-ex, ey, ez));
		quad[2] = obbLocalToWorld(obb, glm::vec3(-ex, ey, -ez));
		quad[3] = obbLocalToWorld(obb, glm::vec3(-ex, -ey, -ez));
		break;
	case voxel::FaceNames::PositiveY:
		quad[0] = obbLocalToWorld(obb, glm::vec3(-ex, ey, -ez));
		quad[1] = obbLocalToWorld(obb, glm::vec3(-ex, ey, ez));
		quad[2] = obbLocalToWorld(obb, glm::vec3(ex, ey, ez));
		quad[3] = obbLocalToWorld(obb, glm::vec3(ex, ey, -ez));
		break;
	case voxel::FaceNames::NegativeY:
		quad[0] = obbLocalToWorld(obb, glm::vec3(-ex, -ey, ez));
		quad[1] = obbLocalToWorld(obb, glm::vec3(-ex, -ey, -ez));
		quad[2] = obbLocalToWorld(obb, glm::vec3(ex, -ey, -ez));
		quad[3] = obbLocalToWorld(obb, glm::vec3(ex, -ey, ez));
		break;
	case voxel::FaceNames::PositiveZ:
		quad[0] = obbLocalToWorld(obb, glm::vec3(-ex, -ey, ez));
		quad[1] = obbLocalToWorld(obb, glm::vec3(ex, -ey, ez));
		quad[2] = obbLocalToWorld(obb, glm::vec3(ex, ey, ez));
		quad[3] = obbLocalToWorld(obb, glm::vec3(-ex, ey, ez));
		break;
	case voxel::FaceNames::NegativeZ:
		quad[0] = obbLocalToWorld(obb, glm::vec3(ex, -ey, -ez));
		quad[1] = obbLocalToWorld(obb, glm::vec3(-ex, -ey, -ez));
		quad[2] = obbLocalToWorld(obb, glm::vec3(-ex, ey, -ez));
		quad[3] = obbLocalToWorld(obb, glm::vec3(ex, ey, -ez));
		break;
	case voxel::FaceNames::Max:
		break;
	}
}

static void aabbCorners(const math::AABB<float> &aabb, glm::vec3 corners[8]) {
	const glm::vec3 &mins = aabb.getLowerCorner();
	const glm::vec3 &maxs = aabb.getUpperCorner();
	corners[0] = glm::vec3(mins.x, mins.y, mins.z);
	corners[1] = glm::vec3(maxs.x, mins.y, mins.z);
	corners[2] = glm::vec3(mins.x, maxs.y, mins.z);
	corners[3] = glm::vec3(maxs.x, maxs.y, mins.z);
	corners[4] = glm::vec3(mins.x, mins.y, maxs.z);
	corners[5] = glm::vec3(maxs.x, mins.y, maxs.z);
	corners[6] = glm::vec3(mins.x, maxs.y, maxs.z);
	corners[7] = glm::vec3(maxs.x, maxs.y, maxs.z);
}

int countSharedAabbCorners(const math::AABB<float> &a, const math::AABB<float> &b) {
	glm::vec3 cornersA[8];
	glm::vec3 cornersB[8];
	aabbCorners(a, cornersA);
	aabbCorners(b, cornersB);
	int shared = 0;
	for (int i = 0; i < 8; ++i) {
		for (int j = 0; j < 8; ++j) {
			if (glm::length(cornersA[i] - cornersB[j]) <= 0.0001f) {
				++shared;
				break;
			}
		}
	}
	return shared;
}

} // namespace scenegraph
