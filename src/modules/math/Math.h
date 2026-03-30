/**
 * @file
 */

#pragma once

#include "core/GLM.h"
#include <glm/mat4x4.hpp>

namespace math {

/**
 * @brief Compute the determinant of the upper-left 3x3 submatrix of a 4x4 matrix.
 * Useful for checking if a transform has an odd number of reflections (negative determinant
 * means 1 or 3 negative scale axes).
 */
inline float det3x3(const glm::mat4 &m) {
	return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1])
		 - m[1][0] * (m[0][1] * m[2][2] - m[0][2] * m[2][1])
		 + m[2][0] * (m[0][1] * m[1][2] - m[0][2] * m[1][1]);
}

glm::vec3 polarToVector(float longitude, float latitude);
void vectorToPolar(const glm::vec3& vector, float& longitude, float& latitude);

glm::ivec3 transform(const glm::mat4x4 &mat, const glm::ivec3 &pos, const glm::vec3 &pivot);
glm::vec3 transform(const glm::mat4x4 &mat, const glm::vec3 &pos, const glm::vec3 &pivot);

}
