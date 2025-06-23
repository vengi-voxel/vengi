/**
 * @file
 */

#include "Face.h"
#include "core/Common.h"
#include <math.h>

#include <glm/ext/scalar_constants.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <glm/common.hpp>

namespace voxel {

FaceNames toFaceNames(const core::String &in) {
	core::String lower = in.toLower();
	if (lower.contains("positivex") || lower.contains("east") || lower.contains("right")) {
		return voxel::FaceNames::PositiveX;
	}
	if (lower.contains("negativex") || lower.contains("west") || lower.contains("left")) {
		return voxel::FaceNames::NegativeX;
	}
	if (lower.contains("positivey") || lower.contains("up")) {
		return voxel::FaceNames::PositiveY;
	}
	if (lower.contains("negativey") || lower.contains("down")) {
		return voxel::FaceNames::NegativeY;
	}
	if (lower.contains("positivez") || lower.contains("south") || lower.contains("back")) {
		return voxel::FaceNames::PositiveZ;
	}
	if (lower.contains("negativez") || lower.contains("north") || lower.contains("front")) {
		return voxel::FaceNames::NegativeZ;
	}
	return voxel::FaceNames::Max;
}

const char *faceNameString(FaceNames face) {
	switch (face) {
	case voxel::FaceNames::PositiveX:
		return "PositiveX_East_Right";
	case voxel::FaceNames::NegativeX:
		return "NegativeX_West_Left";
	case voxel::FaceNames::PositiveY:
		return "PositiveY_Up";
	case voxel::FaceNames::NegativeY:
		return "NegativeY_Down";
	case voxel::FaceNames::PositiveZ:
		return "PositiveZ_South_Back";
	case voxel::FaceNames::NegativeZ:
		return "NegativeZ_North_Front";
	default:
		break;
	}
	return "Unknown";
}

math::Axis faceToAxis(FaceNames face) {
	switch (face) {
	case FaceNames::PositiveX:
	case FaceNames::NegativeX:
		return math::Axis::X;
	case FaceNames::PositiveY:
	case FaceNames::NegativeY:
		return math::Axis::Y;
	case FaceNames::PositiveZ:
	case FaceNames::NegativeZ:
		return math::Axis::Z;
	default:
		return math::Axis::X;
	}
}

FaceBits faceBits(FaceNames face) {
	switch (face) {
	case FaceNames::PositiveX:
		return FaceBits::PositiveX;
	case FaceNames::NegativeX:
		return FaceBits::NegativeX;
	case FaceNames::PositiveY:
		return FaceBits::PositiveY;
	case FaceNames::NegativeY:
		return FaceBits::NegativeY;
	case FaceNames::PositiveZ:
		return FaceBits::PositiveZ;
	case FaceNames::NegativeZ:
		return FaceBits::NegativeZ;
	default:
		break;
	}
	return FaceBits::None;
}

FaceNames raycastFaceDetection(const glm::vec3& rayOrigin, const glm::vec3& hitPos, float offsetMins, float offsetMaxs) {
	const glm::vec3& rayDirection = glm::normalize(hitPos - rayOrigin);
	return raycastFaceDetection(rayOrigin, rayDirection, hitPos, offsetMins, offsetMaxs);
}

FaceNames raycastFaceDetection(const glm::vec3& rayOrigin,
		const glm::vec3& rayDirection, const glm::vec3& hitPos, float offsetMins, float offsetMaxs) {
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

	float tmin;
	float tmax;
	const float divx = glm::abs(rayDirection.x) <= glm::epsilon<float>() ? (rayDirection.x < 0.0f ? -INFINITY : INFINITY) : 1.0f / rayDirection.x;
	if (divx >= 0.0f) {
		tmin = (mins.x - rayOrigin.x) * divx;
		tmax = (maxs.x - rayOrigin.x) * divx;
	} else {
		tmin = (maxs.x - rayOrigin.x) * divx;
		tmax = (mins.x - rayOrigin.x) * divx;
	}

	float tymin;
	float tymax;
	const float divy = glm::abs(rayDirection.y) <= glm::epsilon<float>() ? (rayDirection.y < 0.0f ? -INFINITY : INFINITY) : 1.0f / rayDirection.y;
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
	const float divz = glm::abs(rayDirection.z) <= glm::epsilon<float>() ? (rayDirection.z < 0.0f ? -INFINITY : INFINITY) : 1.0f / rayDirection.z;
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

} // namespace voxel
