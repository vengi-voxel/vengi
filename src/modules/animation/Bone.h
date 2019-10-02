/**
 * @file
 */

#pragma once

#include "core/GLM.h"

namespace animation {

namespace _private {
static constexpr float defaultScale = 1.0f / 11.0f;
}

struct Bone {
	glm::vec3 scale { _private::defaultScale };
	glm::vec3 translation { 0.0f };
	glm::quat orientation = glm::quat_identity<float, glm::defaultp>();

	/**
	 * @brief Computes the bone matrix in the order scale, rotate, translate
	 */
	glm::mat4 matrix() const;

	/**
	 * @brief Perform linear interpolation between the given previous @c Bone instance and the current instance
	 * in relation to the time passed factor
	 * @param[in] dt The time that passed since the last update was called. Given in the range @c [0.0f-1.0f]
	 */
	void lerp(const Bone& previous, float dt);
};

}
