/**
 * @file
 */

#include "FrameTransform.h"
#include "voxel/Region.h"
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

glm::vec3 FrameTransform::calcModelSpace(const glm::vec3 &worldPos) const {
	if (!_inverseCalculated) {
		_inverseMatrix = glm::inverse(worldMatrix());
		_inverseCalculated = true;
	}
	const glm::vec3 modelSpacePos(_inverseMatrix * glm::vec4(worldPos, 1.0f));
	return modelSpacePos;
}

glm::mat3 FrameTransform::calcNormalMatrix() const {
	return glm::transpose(_inverseMatrix);
}

glm::vec3 FrameTransform::calcWorldNormal(const glm::vec3 &normal) const {
	return glm::normalize(calcNormalMatrix() * normal);
}

glm::vec3 FrameTransform::calcPosition(const glm::vec3 &pos, const glm::vec3 &pivot) const {
	return _matrix * glm::vec4(pos - pivot, 1.0f);
}

glm::mat4 FrameTransform::calculateWorldMatrix(const glm::vec3 &normalizedPivot, const glm::vec3 &dimensions) const {
	return glm::translate(worldMatrix(), -normalizedPivot * dimensions);
}

glm::vec3 FrameTransform::scale() const {
	if (_scaleCalculated) {
		return _scale;
	}
	glm::vec3 sc;
	glm::quat orientation;
	glm::vec3 translation;
	decompose(sc, orientation, translation);
	_scale = sc;
	_scaleCalculated = true;
	return sc;
}

glm::vec3 FrameTransform::translation() const {
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
