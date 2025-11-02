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

void FrameTransform::setWorldMatrix(const glm::mat4 &m) {
	_matrix = m;
	resetCache();
}

const glm::mat4 &FrameTransform::worldMatrix() const {
	return _matrix;
}

glm::mat4 FrameTransform::calculateWorldMatrix(const glm::vec3 &normalizedPivot, const glm::vec3 &dimensions) const {
	return glm::translate(worldMatrix(), -normalizedPivot * dimensions);
}

const glm::vec3 &FrameTransform::worldScale() const {
	if (_scaleCalculated) {
		return _scale;
	}
	glm::vec3 sc;
	glm::quat orientation;
	glm::vec3 translation;
	decompose(sc, orientation, translation);
	_scale = sc;
	_scaleCalculated = true;
	return _scale;
}

glm::vec3 FrameTransform::worldTranslation() const {
	return glm::column(_matrix, 3);
}

void FrameTransform::decompose(glm::vec3 &scale, glm::quat &orientation, glm::vec3 &translation) const {
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(_matrix, scale, orientation, translation, skew, perspective);
}

glm::vec3 calculateExtents(const glm::vec3 &dimensions) {
	return dimensions / 2.0f;
}

} // namespace scenegraph
