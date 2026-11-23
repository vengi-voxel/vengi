/**
 * @file
 */

#include "Raycast.h"
#include "core/Common.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"
#include "math/Scalar.h"

namespace voxelutil {

bool clipRaySegmentToAABB(const math::Vec3f &start, const math::Vec3f &end, const math::Vec3f &mins, const math::Vec3f &maxs,
						  float &tEnter, float &tExit, math::Vec3i *entryNormal) {
	const math::Vec3f dir = end - start;
	tEnter = 0.0f;
	tExit = 1.0f;
	if (entryNormal != nullptr) {
		*entryNormal = math::Vec3i(0);
	}

	for (int axis = 0; axis < 3; ++axis) {
		const float d = dir[axis];
		const float o = start[axis];
		if (math::abs(d) <= math::epsilonf) {
			// Ray parallel to this slab - must start inside the slab.
			if (o < mins[axis] || o > maxs[axis]) {
				return false;
			}
			continue;
		}

		const float invD = 1.0f / d;
		float t1 = (mins[axis] - o) * invD;
		float t2 = (maxs[axis] - o) * invD;
		int nearSign = -1;
		if (t1 > t2) {
			core::exchange(t1, t2);
			nearSign = 1;
		}

		if (t1 > tEnter) {
			tEnter = t1;
			if (entryNormal != nullptr) {
				*entryNormal = math::Vec3i(0);
				// Entering through the near face: -axis when traveling positive.
				(*entryNormal)[axis] = nearSign;
			}
		}
		tExit = core_min(tExit, t2);
		if (tEnter > tExit) {
			return false;
		}
	}

	return tExit >= 0.0f && tEnter <= 1.0f;
}

bool RaycastResult::isSolidStart() const {
	return fract <= 0.0f;
}

bool RaycastResult::isCompleted() const {
	return type == Completed;
}

bool RaycastResult::isInterrupted() const {
	return type == Interrupted;
}

RaycastResult RaycastResult::completed(float length) {
	RaycastResult r;
	r.type = Completed;
	r.length = length;
	r.fract = 1.0f;
	return r;
}

RaycastResult RaycastResult::interrupted(float length, float fract, const math::Vec3i &normal) {
	RaycastResult r;
	r.type = Interrupted;
	r.length = length;
	r.fract = fract;
	r.normal = normal;
	return r;
}

math::Vec3f RaycastResult::adjustPoint(const math::Vec3f &point, float offset) const {
	return point - math::Vec3f(normal) * (offset + 0.001f);
}

math::Vec3f RaycastResult::projectOnPlane(const math::Vec3f &v) const {
	math::Vec3f n = math::normalize(math::Vec3f(normal));
	return v - math::dot(v, n) * n;
}

RaycastHit raycastFaceDetection(const math::Vec3f &rayOrigin, const math::Vec3f &hitPos, float offsetMins,
								float offsetMaxs) {
	const math::Vec3f &rayDirection = math::normalize(hitPos - rayOrigin);
	return raycastFaceDetection(rayOrigin, rayDirection, hitPos, offsetMins, offsetMaxs);
}

RaycastHit raycastFaceDetection(const math::Vec3f &rayOrigin, const math::Vec3f &rayDirection, const math::Vec3f &hitPos,
								float offsetMins, float offsetMaxs) {
	const math::Vec3f mins = hitPos + offsetMins;
	const math::Vec3f maxs = hitPos + offsetMaxs;

	/*
	 * Ray-box intersection using IEEE numerical properties to ensure that the
	 * test is both robust and efficient, as described in:
	 *
	 *      Amy Williams, Steve Barrus, R. Keith Morley, and Peter Shirley
	 *      "An Efficient and Robust Ray-Box Intersection Algorithm"
	 *      Journal of graphics tools, 10(1):49-54, 2005
	 *
	 */

	float tmin; // entry point
	float tmax; // exit point
	const float divx = math::abs(rayDirection.x) <= math::epsilonf
						   ? (rayDirection.x < 0.0f ? -math::Infinity : math::Infinity)
						   : 1.0f / rayDirection.x;
	if (divx >= 0.0f) {
		tmin = (mins.x - rayOrigin.x) * divx;
		tmax = (maxs.x - rayOrigin.x) * divx;
	} else {
		tmin = (maxs.x - rayOrigin.x) * divx;
		tmax = (mins.x - rayOrigin.x) * divx;
	}

	float tymin;
	float tymax;
	const float divy = math::abs(rayDirection.y) <= math::epsilonf
						   ? (rayDirection.y < 0.0f ? -math::Infinity : math::Infinity)
						   : 1.0f / rayDirection.y;
	if (divy >= 0.0f) {
		tymin = (mins.y - rayOrigin.y) * divy;
		tymax = (maxs.y - rayOrigin.y) * divy;
	} else {
		tymin = (maxs.y - rayOrigin.y) * divy;
		tymax = (mins.y - rayOrigin.y) * divy;
	}

	if ((tmin > tymax) || (tymin > tmax)) {
		return {voxel::FaceNames::Max, -1.0f, math::Vec3f(0)};
	}

	if (tymin > tmin) {
		tmin = tymin;
	}

	if (tymax < tmax) {
		tmax = tymax;
	}

	float tzmin;
	float tzmax;
	const float divz = math::abs(rayDirection.z) <= math::epsilonf
						   ? (rayDirection.z < 0.0f ? -math::Infinity : math::Infinity)
						   : 1.0f / rayDirection.z;
	if (divz >= 0.0f) {
		tzmin = (mins.z - rayOrigin.z) * divz;
		tzmax = (maxs.z - rayOrigin.z) * divz;
	} else {
		tzmin = (maxs.z - rayOrigin.z) * divz;
		tzmax = (mins.z - rayOrigin.z) * divz;
	}

	if ((tmin > tzmax) || (tzmin > tmax)) {
		return {voxel::FaceNames::Max, -1.0f, math::Vec3f(0)};
	}

	if (tzmin > tmin) {
		tmin = tzmin;
	}

	if (tzmax < tmax) {
		tmax = tzmax;
	}

	float rayLength = (tmin >= 0.0f) ? tmin : tmax;
	const float x = rayOrigin.x + rayDirection.x * rayLength;
	const float y = rayOrigin.y + rayDirection.y * rayLength;
	const float z = rayOrigin.z + rayDirection.z * rayLength;

	float delta = (std::numeric_limits<float>::max)();
	voxel::FaceNames face = voxel::FaceNames::Max;
	math::Vec3f hitPoint = rayOrigin + rayDirection * rayLength;

	float frac = rayLength / math::length(rayDirection); // normalize into [0,1]
	float current = math::abs(x - mins.x);
	if (delta > current) {
		delta = current;
		face = voxel::FaceNames::NegativeX;
	}

	current = math::abs(x - maxs.x);
	if (delta > current) {
		delta = current;
		face = voxel::FaceNames::PositiveX;
	}

	current = math::abs(y - mins.y);
	if (delta > current) {
		delta = current;
		face = voxel::FaceNames::NegativeY;
	}

	current = math::abs(y - maxs.y);
	if (delta > current) {
		delta = current;
		face = voxel::FaceNames::PositiveY;
	}

	current = math::abs(z - mins.z);
	if (delta > current) {
		delta = current;
		face = voxel::FaceNames::NegativeZ;
	}

	current = math::abs(z - maxs.z);
	if (delta > current) {
		face = voxel::FaceNames::PositiveZ;
	}

	return {face, frac, hitPoint};
}

} // namespace voxelutil
