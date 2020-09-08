/**
 * @file
 */

#pragma once

#include <glm/vec3.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>

namespace animation {

namespace _private {
static constexpr float torsoScale = 1.0f / 15.0f;
}

/**
 * Defines the scale, translation and orientation of a bone that is used to calculate
 * the animation matrices for the shader.
 * @ingroup Animation
 */
struct Bone {
	glm::vec3 scale { 1.0f };
	glm::vec3 translation { 0.0f };
	glm::quat orientation = glm::quat_identity<float, glm::defaultp>();

	/**
	 * @brief Computes the bone matrix in the order scale, rotate, translate
	 */
	glm::mat4 matrix() const;

	/**
	 * @brief Perform linear interpolation between the given previous @c Bone instance and the current instance
	 * in relation to the time passed factor
	 * @param[in] deltaTimeSeconds The time that passed since the last update was called.
	 */
	void lerp(const Bone& previous, double deltaTimeSeconds);
};

}
