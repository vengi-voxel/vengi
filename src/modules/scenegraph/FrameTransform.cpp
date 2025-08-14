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

glm::vec3 FrameTransform::calcModelSpace(const glm::vec3 &worldPos) const {
	// TODO: PERF: cache the inverse
	const glm::mat4 &invModel = glm::inverse(worldMatrix());
	const glm::vec3 modelSpacePos(invModel * glm::vec4(worldPos, 1.0f));
	return modelSpacePos;
}

glm::vec3 FrameTransform::calcPivot(const glm::vec3 &dimensions, const glm::vec3 &normalizedPivot) const {
	return normalizedPivot * dimensions;
}

glm::vec3 FrameTransform::calcPosition(const glm::vec3 &pos, const glm::vec3 &pivot) const {
	return matrix * glm::vec4(pos - pivot, 1.0f);
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
	return glm::column(matrix, 3);
}

void FrameTransform::decompose(glm::vec3 &scale, glm::quat &orientation, glm::vec3 &translation) const {
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(matrix, scale, orientation, translation, skew, perspective);
}

glm::vec3 calculateExtents(const glm::vec3 &dimensions) {
	return dimensions / 2.0f;
}

glm::vec3 calculateCenter(const FrameTransform &transform, const glm::vec3 &worldPivot, const glm::vec3 &regionCenter) {
	return transform.worldMatrix() * glm::vec4(regionCenter - worldPivot, 1.0f);
}

} // namespace scenegraph
