/**
 * @file
 */

#pragma once

#include <glm/fwd.hpp>

namespace voxel {

enum class FaceNames {
	PositiveX = 0, PositiveY = 1, PositiveZ = 2, NegativeX = 3, NegativeY = 4, NegativeZ = 5, Max
};

inline bool isHorizontalFace(FaceNames face) {
	return face == FaceNames::PositiveX || face == FaceNames::PositiveZ || face == FaceNames::NegativeX || face == FaceNames::NegativeZ;
}

inline bool isVerticalFace(FaceNames face) {
	 return face == FaceNames::PositiveY || face == FaceNames::NegativeY;
}

extern FaceNames raycastFaceDetection(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, const glm::vec3& hitPos, float offsetMins = -0.5f, float offsetMaxs = 0.5f);
extern FaceNames raycastFaceDetection(const glm::vec3& rayOrigin, const glm::vec3& hitPos, float offsetMins = -0.5f, float offsetMaxs = 0.5f);

}
