/**
 * @file
 */

#include "SceneUtil.h"
#include "math/Mat4f.h"
#include "math/Mat3f.h"
#include "math/Vec4f.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"
#include "math/Mat4Ops.h"

namespace scenegraph {

voxel::Region toRegion(const math::OBBF &obb) {
	return toRegion(toAABB(obb));
}

voxel::Region toRegion(const math::AABB<float> &aabb) {
	return voxel::Region(math::floor(aabb.getLowerCorner()), math::ceil(aabb.getUpperCorner() - 1.0f));
}

math::AABB<float> toAABB(const math::OBBF &obb) {
	const math::Vec3f &origin = obb.origin();
	const math::Mat3f &rotation = obb.rotation();
	const math::Vec3f &extends = obb.extents();
	math::Vec3f mins = origin;
	math::Vec3f maxs = origin;
	for (int i = 0; i < 3; ++i) {
		math::Vec3f worldAxis(0.0f);
		worldAxis[i] = 1.0f;

		// Project each OBB axis onto the world axis and compute the contribution
		float extent = math::abs(math::dot(rotation[0], worldAxis)) * extends.x +
					   math::abs(math::dot(rotation[1], worldAxis)) * extends.y +
					   math::abs(math::dot(rotation[2], worldAxis)) * extends.z;

		mins[i] = origin[i] - extent;
		maxs[i] = origin[i] + extent;
	}
	return {mins, maxs};
}

math::AABB<float> toAABB(const voxel::Region &region) {
	if (region.isValid()) {
		return math::AABB<float>(math::floor(region.getLowerCornerf()),
								 math::floor(math::Vec3f(region.getUpperCornerf() + 1.0f)));
	}
	return math::AABB<float>(1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f);
}

/**
 * @sa scenegraph::SceneGraph::worldMatrix()
 */
math::OBBF toOBB(bool sceneMode, const voxel::Region &region, const math::Vec3f &normalizedPivot,
					   const scenegraph::FrameTransform &transform) {
	core_assert(region.isValid());
	if (sceneMode) {
		const math::Vec3f extents = calculateExtents(region.getDimensionsInVoxels());
		const math::Mat4f &worldMatrix = transform.calculateWorldMatrix(normalizedPivot, region.getDimensionsInVoxels());
		const math::Vec3f center = worldMatrix * math::Vec4f(region.calcCenterf(), 1.0f);
		return math::OBBF(center, extents, worldMatrix);
	}
	return math::OBBF(math::floor(region.getLowerCornerf()),
							math::floor(math::Vec3f(region.getUpperCornerf() + 1.0f)));
}

static math::Vec3f obbLocalToWorld(const math::OBBF &obb, const math::Vec3f &local) {
	const math::Mat3f rot(obb.rotation());
	return obb.origin() + rot * local;
}

voxel::Region calcAdjacentRegion(const voxel::Region &sourceWorld, voxel::FaceNames face,
								 const math::Vec3i &newDims) {
	core_assert(sourceWorld.isValid());
	const math::Vec3i maxOffset = newDims - math::Vec3i(1);
	const math::Vec3i &sourceLower = sourceWorld.getLowerCorner();
	const math::Vec3i &sourceUpper = sourceWorld.getUpperCorner();
	math::Vec3i lower = sourceLower;
	switch (face) {
	case voxel::FaceNames::PositiveX:
		lower = math::Vec3i(sourceUpper.x + 1, sourceLower.y, sourceLower.z);
		break;
	case voxel::FaceNames::NegativeX:
		lower = math::Vec3i(sourceLower.x - newDims.x, sourceLower.y, sourceLower.z);
		break;
	case voxel::FaceNames::PositiveY:
		lower = math::Vec3i(sourceLower.x, sourceUpper.y + 1, sourceLower.z);
		break;
	case voxel::FaceNames::NegativeY:
		lower = math::Vec3i(sourceLower.x, sourceLower.y - newDims.y, sourceLower.z);
		break;
	case voxel::FaceNames::PositiveZ:
		lower = math::Vec3i(sourceLower.x, sourceLower.y, sourceUpper.z + 1);
		break;
	case voxel::FaceNames::NegativeZ:
		lower = math::Vec3i(sourceLower.x, sourceLower.y, sourceLower.z - newDims.z);
		break;
	case voxel::FaceNames::Max:
		return voxel::Region::InvalidRegion;
	}
	return voxel::Region(lower, lower + maxOffset);
}

math::OBBF calcAdjacentObb(const math::OBBF &source, voxel::FaceNames face, const math::Vec3f &newExtents) {
	const math::Vec3f &sourceExtents = source.extents();
	math::Vec3f localOffset(0.0f);
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
		return math::OBBF(math::Vec3f(0.0f), math::Vec3f(0.0f), math::Mat3f(1.0f));
	}
	const math::Mat3f rot(source.rotation());
	const math::Vec3f newOrigin = source.origin() + rot * localOffset;
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

	const math::Mat3f invRot = math::inverse(math::Mat3f(obb.rotation()));
	const math::Vec3f localHit = invRot * (hit.hitPoint - obb.origin());
	const math::Vec3f &extents = obb.extents();
	const float dx = extents.x - math::abs(localHit.x);
	const float dy = extents.y - math::abs(localHit.y);
	const float dz = extents.z - math::abs(localHit.z);
	if (dx <= dy && dx <= dz) {
		hit.face = localHit.x >= 0.0f ? voxel::FaceNames::PositiveX : voxel::FaceNames::NegativeX;
	} else if (dy <= dx && dy <= dz) {
		hit.face = localHit.y >= 0.0f ? voxel::FaceNames::PositiveY : voxel::FaceNames::NegativeY;
	} else {
		hit.face = localHit.z >= 0.0f ? voxel::FaceNames::PositiveZ : voxel::FaceNames::NegativeZ;
	}
	return hit;
}

void obbFaceQuad(const math::OBBF &obb, voxel::FaceNames face, math::Vec3f quad[4]) {
	const math::Vec3f &extents = obb.extents();
	const float ex = extents.x;
	const float ey = extents.y;
	const float ez = extents.z;
	switch (face) {
	case voxel::FaceNames::PositiveX:
		quad[0] = obbLocalToWorld(obb, math::Vec3f(ex, -ey, -ez));
		quad[1] = obbLocalToWorld(obb, math::Vec3f(ex, ey, -ez));
		quad[2] = obbLocalToWorld(obb, math::Vec3f(ex, ey, ez));
		quad[3] = obbLocalToWorld(obb, math::Vec3f(ex, -ey, ez));
		break;
	case voxel::FaceNames::NegativeX:
		quad[0] = obbLocalToWorld(obb, math::Vec3f(-ex, -ey, ez));
		quad[1] = obbLocalToWorld(obb, math::Vec3f(-ex, ey, ez));
		quad[2] = obbLocalToWorld(obb, math::Vec3f(-ex, ey, -ez));
		quad[3] = obbLocalToWorld(obb, math::Vec3f(-ex, -ey, -ez));
		break;
	case voxel::FaceNames::PositiveY:
		quad[0] = obbLocalToWorld(obb, math::Vec3f(-ex, ey, -ez));
		quad[1] = obbLocalToWorld(obb, math::Vec3f(-ex, ey, ez));
		quad[2] = obbLocalToWorld(obb, math::Vec3f(ex, ey, ez));
		quad[3] = obbLocalToWorld(obb, math::Vec3f(ex, ey, -ez));
		break;
	case voxel::FaceNames::NegativeY:
		quad[0] = obbLocalToWorld(obb, math::Vec3f(-ex, -ey, ez));
		quad[1] = obbLocalToWorld(obb, math::Vec3f(-ex, -ey, -ez));
		quad[2] = obbLocalToWorld(obb, math::Vec3f(ex, -ey, -ez));
		quad[3] = obbLocalToWorld(obb, math::Vec3f(ex, -ey, ez));
		break;
	case voxel::FaceNames::PositiveZ:
		quad[0] = obbLocalToWorld(obb, math::Vec3f(-ex, -ey, ez));
		quad[1] = obbLocalToWorld(obb, math::Vec3f(ex, -ey, ez));
		quad[2] = obbLocalToWorld(obb, math::Vec3f(ex, ey, ez));
		quad[3] = obbLocalToWorld(obb, math::Vec3f(-ex, ey, ez));
		break;
	case voxel::FaceNames::NegativeZ:
		quad[0] = obbLocalToWorld(obb, math::Vec3f(ex, -ey, -ez));
		quad[1] = obbLocalToWorld(obb, math::Vec3f(-ex, -ey, -ez));
		quad[2] = obbLocalToWorld(obb, math::Vec3f(-ex, ey, -ez));
		quad[3] = obbLocalToWorld(obb, math::Vec3f(ex, ey, -ez));
		break;
	case voxel::FaceNames::Max:
		break;
	}
}

static void aabbCorners(const math::AABB<float> &aabb, math::Vec3f corners[8]) {
	const math::Vec3f &mins = aabb.getLowerCorner();
	const math::Vec3f &maxs = aabb.getUpperCorner();
	corners[0] = math::Vec3f(mins.x, mins.y, mins.z);
	corners[1] = math::Vec3f(maxs.x, mins.y, mins.z);
	corners[2] = math::Vec3f(mins.x, maxs.y, mins.z);
	corners[3] = math::Vec3f(maxs.x, maxs.y, mins.z);
	corners[4] = math::Vec3f(mins.x, mins.y, maxs.z);
	corners[5] = math::Vec3f(maxs.x, mins.y, maxs.z);
	corners[6] = math::Vec3f(mins.x, maxs.y, maxs.z);
	corners[7] = math::Vec3f(maxs.x, maxs.y, maxs.z);
}

int countSharedAabbCorners(const math::AABB<float> &a, const math::AABB<float> &b) {
	math::Vec3f cornersA[8];
	math::Vec3f cornersB[8];
	aabbCorners(a, cornersA);
	aabbCorners(b, cornersB);
	int shared = 0;
	for (int i = 0; i < 8; ++i) {
		for (int j = 0; j < 8; ++j) {
			if (math::length(cornersA[i] - cornersB[j]) <= 0.0001f) {
				++shared;
				break;
			}
		}
	}
	return shared;
}

} // namespace scenegraph
