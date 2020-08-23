/**
 * @file
 */

#pragma once

#include "core/GLM.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>
#include "Bone.h"

namespace animation {

inline constexpr Bone zero() {
	return Bone{glm::zero<glm::vec3>(), glm::zero<glm::vec3>(), glm::quat_identity<float, glm::defaultp>()};
}

inline constexpr Bone translate(float x, float y, float z) {
	return Bone{glm::one<glm::vec3>(), glm::vec3(x, y, z), glm::quat_identity<float, glm::defaultp>()};
}

inline glm::quat rotateX(float angle) {
	return glm::angleAxis(angle, glm::right);
}

inline glm::quat rotateZ(float angle) {
	return glm::angleAxis(angle, glm::backward);
}

inline glm::quat rotateY(float angle) {
	return glm::angleAxis(angle, glm::up);
}

inline constexpr glm::quat rotateXZ(float angleX, float angleZ) {
	return glm::quat(glm::vec3(angleX, 0.0f, angleZ));
}

inline constexpr glm::quat rotateYZ(float angleY, float angleZ) {
	return glm::quat(glm::vec3(0.0f, angleY, angleZ));
}

inline constexpr glm::quat rotateXY(float angleX, float angleY) {
	return glm::quat(glm::vec3(angleX, angleY, 0.0f));
}

inline constexpr glm::quat rotateXYZ(float angleX, float angleY, float angleZ) {
	return glm::quat(glm::vec3(angleX, angleY, angleZ));
}

inline constexpr Bone mirrorX(const Bone& bone) {
	Bone mirrored = bone;
	mirrored.translation.x = -mirrored.translation.x;
	// the winding order is fixed by reverse index buffer filling
	mirrored.scale.x = -mirrored.scale.x;
	mirrored.orientation.x = -mirrored.orientation.x;
	mirrored.orientation.y = -mirrored.orientation.y;
	mirrored.orientation.z = -mirrored.orientation.z;
	return mirrored;
}

inline constexpr Bone mirrorXYZ(const Bone& bone) {
	Bone mirrored = bone;
	mirrored.translation = -mirrored.translation;
	// the winding order is fixed by reverse index buffer filling
	mirrored.scale = -mirrored.scale;
	return mirrored;
}

inline constexpr Bone mirrorXZ(const Bone& bone) {
	Bone mirrored = bone;
	mirrored.translation.x = -mirrored.translation.x;
	mirrored.translation.z = -mirrored.translation.z;
	// the winding order is fixed by reverse index buffer filling
	mirrored.scale.x = -mirrored.scale.x;
	mirrored.scale.z = -mirrored.scale.z;
	return mirrored;
}

inline constexpr glm::vec3 mirrorXZ(glm::vec3 translation) {
	translation.x = -translation.x;
	translation.z = -translation.z;
	return translation;
}

}
