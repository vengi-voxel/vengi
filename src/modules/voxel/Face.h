/**
 * @file
 */

#pragma once

#include <glm/fwd.hpp>

namespace voxel {

enum class FaceNames {
	PositiveX, PositiveY, PositiveZ, NegativeX, NegativeY, NegativeZ, Max
};

inline bool isHorizontalFace(FaceNames face) {
	return face == FaceNames::PositiveX || face == FaceNames::PositiveZ || face == FaceNames::NegativeX || face == FaceNames::NegativeZ;
}

inline bool isVerticalFace(FaceNames face) {
	 return face == FaceNames::PositiveY || face == FaceNames::NegativeY;
}

extern FaceNames raycastFaceDetection(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, const glm::ivec3& hitPos);

}
