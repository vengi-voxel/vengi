/**
 * @file
 */

#include "FrameTransform.h"
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

namespace scenegraph {

glm::vec3 FrameTransform::scale() const {
	glm::vec3 sc;
	glm::quat orientation;
	glm::vec3 translation;
	decompose(sc, orientation, translation);
	return sc;
}

glm::vec3 FrameTransform::translation() const {
	return glm::column(matrix, 3);
}

void FrameTransform::decompose(glm::vec3 &scale, glm::quat &orientation, glm::vec3 &translation) const {
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(matrix, scale, orientation, translation, skew, perspective);
}

} // namespace scenegraph
