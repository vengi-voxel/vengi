/**
 * @file
 */

#include "Face.h"
#include "core/Common.h"

#include <glm/vec3.hpp>
#include <glm/common.hpp>

namespace voxel {

FaceNames raycastFaceDetection(const glm::vec3& rayOrigin,
		const glm::vec3& rayDirection, const glm::vec3& hitPos, float halfSize) {
	const glm::vec3 mins = hitPos - halfSize;
	const glm::vec3 maxs = hitPos + halfSize;

	/*
	 * Ray-box intersection using IEEE numerical properties to ensure that the
	 * test is both robust and efficient, as described in:
	 *
	 *      Amy Williams, Steve Barrus, R. Keith Morley, and Peter Shirley
	 *      "An Efficient and Robust Ray-Box Intersection Algorithm"
	 *      Journal of graphics tools, 10(1):49-54, 2005
	 *
	 */

	float tmin;
	float tmax;
	const float divx = 1.0f / rayDirection.x;
	if (divx >= 0.0f) {
		tmin = (mins.x - rayOrigin.x) * divx;
		tmax = (maxs.x - rayOrigin.x) * divx;
	} else {
		tmin = (maxs.x - rayOrigin.x) * divx;
		tmax = (mins.x - rayOrigin.x) * divx;
	}

	float tymin;
	float tymax;
	const float divy = 1.0f / rayDirection.y;
	if (divy >= 0.0f) {
		tymin = (mins.y - rayOrigin.y) * divy;
		tymax = (maxs.y - rayOrigin.y) * divy;
	} else {
		tymin = (maxs.y - rayOrigin.y) * divy;
		tymax = (mins.y - rayOrigin.y) * divy;
	}

	if ((tmin > tymax) || (tymin > tmax)) {
		return FaceNames::Max;
	}

	if (tymin > tmin) {
		tmin = tymin;
	}

	if (tymax < tmax) {
		tmax = tymax;
	}

	float tzmin;
	float tzmax;
	const float divz = 1.0f / rayDirection.z;
	if (divz >= 0.0f) {
		tzmin = (mins.z - rayOrigin.z) * divz;
		tzmax = (maxs.z - rayOrigin.z) * divz;
	} else {
		tzmin = (maxs.z - rayOrigin.z) * divz;
		tzmax = (mins.z - rayOrigin.z) * divz;
	}

	if ((tmin > tzmax) || (tzmin > tmax)) {
		return FaceNames::Max;
	}

	if (tzmin > tmin) {
		tmin = tzmin;
	}

	if (tzmax < tmax) {
		tmax = tzmax;
	}

	const float rayLength = core_min(tmin, tmax);
	const float x = rayOrigin.x + rayDirection.x * rayLength;
	const float y = rayOrigin.y + rayDirection.y * rayLength;
	const float z = rayOrigin.z + rayDirection.z * rayLength;

	float delta = (std::numeric_limits<float>::max)();
	FaceNames face = FaceNames::Max;

	float current = glm::abs(x - mins.x);
	if (delta > current) {
		delta = current;
		face = FaceNames::NegativeX;
	}

	current = glm::abs(x - maxs.x);
	if (delta > current) {
		delta = current;
		face = FaceNames::PositiveX;
	}

	current = glm::abs(y - mins.y);
	if (delta > current) {
		delta = current;
		face = FaceNames::NegativeY;
	}

	current = glm::abs(y - maxs.y);
	if (delta > current) {
		delta = current;
		face = FaceNames::PositiveY;
	}

	current = glm::abs(z - mins.z);
	if (delta > current) {
		delta = current;
		face = FaceNames::NegativeZ;
	}

	current = glm::abs(z - maxs.z);
	if (delta > current) {
		face = FaceNames::PositiveZ;
	}

	return face;
}

}
