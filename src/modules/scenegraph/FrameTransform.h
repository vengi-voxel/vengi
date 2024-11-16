/**
 * @file
 */

#pragma once

#include <glm/fwd.hpp>
#include <glm/mat4x4.hpp>

namespace scenegraph {

struct FrameTransform {
	glm::mat4 matrix;

	inline const glm::mat4 &worldMatrix() const {
		return matrix;
	}

	glm::vec3 translation() const;
	glm::vec3 scale() const;
	void decompose(glm::vec3 &scale, glm::quat &orientation, glm::vec3 &translation) const;
};

} // namespace scenegraph
