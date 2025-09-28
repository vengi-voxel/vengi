/**
 * @file
 */

#include "Raycast.h"
#include "core/Common.h"

namespace voxelutil {

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

RaycastResult RaycastResult::interrupted(float length, float fract, const glm::ivec3 &normal) {
	RaycastResult r;
	r.type = Interrupted;
	r.length = length;
	r.fract = fract;
	r.normal = normal;
	return r;
}

glm::vec3 RaycastResult::adjustPoint(const glm::vec3 &point, float offset) const {
	return point - glm::vec3(normal) * (offset + 0.001f);
}

glm::vec3 RaycastResult::projectOnPlane(const glm::vec3 &v) const {
	glm::vec3 n = glm::normalize(glm::vec3(normal));
	return v - glm::dot(v, n) * n;
}

RaycastHit raycastFaceDetection(const glm::vec3 &rayOrigin, const glm::vec3 &hitPos, float offsetMins,
								float offsetMaxs) {
	const glm::vec3 &rayDirection = glm::normalize(hitPos - rayOrigin);
	return raycastFaceDetection(rayOrigin, rayDirection, hitPos, offsetMins, offsetMaxs);
}

RaycastHit raycastFaceDetection(const glm::vec3 &rayOrigin, const glm::vec3 &rayDirection, const glm::vec3 &hitPos,
								float offsetMins, float offsetMaxs) {
	const glm::vec3 mins = hitPos + offsetMins;
	const glm::vec3 maxs = hitPos + offsetMaxs;

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
	const float divx = glm::abs(rayDirection.x) <= glm::epsilon<float>()
						   ? (rayDirection.x < 0.0f ? -INFINITY : INFINITY)
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
	const float divy = glm::abs(rayDirection.y) <= glm::epsilon<float>()
						   ? (rayDirection.y < 0.0f ? -INFINITY : INFINITY)
						   : 1.0f / rayDirection.y;
	if (divy >= 0.0f) {
		tymin = (mins.y - rayOrigin.y) * divy;
		tymax = (maxs.y - rayOrigin.y) * divy;
	} else {
		tymin = (maxs.y - rayOrigin.y) * divy;
		tymax = (mins.y - rayOrigin.y) * divy;
	}

	if ((tmin > tymax) || (tymin > tmax)) {
		return {voxel::FaceNames::Max, -1.0f, glm::vec3(0)};
	}

	if (tymin > tmin) {
		tmin = tymin;
	}

	if (tymax < tmax) {
		tmax = tymax;
	}

	float tzmin;
	float tzmax;
	const float divz = glm::abs(rayDirection.z) <= glm::epsilon<float>()
						   ? (rayDirection.z < 0.0f ? -INFINITY : INFINITY)
						   : 1.0f / rayDirection.z;
	if (divz >= 0.0f) {
		tzmin = (mins.z - rayOrigin.z) * divz;
		tzmax = (maxs.z - rayOrigin.z) * divz;
	} else {
		tzmin = (maxs.z - rayOrigin.z) * divz;
		tzmax = (mins.z - rayOrigin.z) * divz;
	}

	if ((tmin > tzmax) || (tzmin > tmax)) {
		return {voxel::FaceNames::Max, -1.0f, glm::vec3(0)};
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
	glm::vec3 hitPoint = rayOrigin + rayDirection * rayLength;

	float frac = rayLength / glm::length(rayDirection); // normalize into [0,1]
	float current = glm::abs(x - mins.x);
	if (delta > current) {
		delta = current;
		face = voxel::FaceNames::NegativeX;
	}

	current = glm::abs(x - maxs.x);
	if (delta > current) {
		delta = current;
		face = voxel::FaceNames::PositiveX;
	}

	current = glm::abs(y - mins.y);
	if (delta > current) {
		delta = current;
		face = voxel::FaceNames::NegativeY;
	}

	current = glm::abs(y - maxs.y);
	if (delta > current) {
		delta = current;
		face = voxel::FaceNames::PositiveY;
	}

	current = glm::abs(z - mins.z);
	if (delta > current) {
		delta = current;
		face = voxel::FaceNames::NegativeZ;
	}

	current = glm::abs(z - maxs.z);
	if (delta > current) {
		face = voxel::FaceNames::PositiveZ;
	}

	return {face, frac, hitPoint};
}

} // namespace voxelutil
