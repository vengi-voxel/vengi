/**
 * @file
 */

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace util {

inline glm::vec3 getDirection(float yaw) {
	const float angleRadians = yaw - glm::half_pi<float>();
	const glm::vec3 direction(glm::sin(angleRadians), 0.0, glm::cos(angleRadians));
	return direction;
}

inline glm::vec3 getDirection(float pitch, float yaw) {
	const float cosV = glm::cos(pitch);
	const float cosH = glm::cos(yaw);
	const float sinH = glm::sin(yaw);
	const float sinV = glm::sin(pitch);
	const glm::vec3 direction(cosV * sinH, sinV, cosV * cosH);
	return direction;
}


}
