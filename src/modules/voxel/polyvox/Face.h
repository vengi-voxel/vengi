/**
 * @file
 */

#pragma once

namespace voxel {

enum FaceNames {
	PositiveX, PositiveY, PositiveZ, NegativeX, NegativeY, NegativeZ, NoOfFaces
};

inline bool isHorizontalFace(FaceNames face) {
	return face == PositiveX || face == PositiveZ || face == NegativeX || face == NegativeZ;
}

inline bool isVerticalFace(FaceNames face) {
	 return face == PositiveY || face == NegativeY;
}

extern FaceNames raycastFaceDetection(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, const glm::ivec3& hitPos);

}
