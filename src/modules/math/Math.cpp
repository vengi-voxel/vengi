/**
 * @file
 */

#include "Math.h"
#include <glm/trigonometric.hpp>
#include <glm/mat4x4.hpp>

namespace math {

glm::vec3 polarToVector(float longitude, float latitude) {
	float num = glm::cos(latitude);
	float num2 = glm::sin(latitude);
	float num3 = glm::cos(longitude);
	float num4 = glm::sin(longitude);
	return glm::vec3(num3 * num, num2, num4 * num);
}

void vectorToPolar(const glm::vec3& vector, float& longitude, float& latitude) {
	float num = glm::sqrt(vector.x * vector.x + vector.z * vector.z);
	if (num > 0.0f) {
		longitude = glm::atan(vector.z, vector.x);
		latitude = glm::atan(vector.y, num);
	} else {
		longitude = 0.0f;
		latitude = glm::atan(vector.y, num);
	}
}

glm::ivec3 transform(const glm::mat4x4 &mat, const glm::ivec3 &pos, const glm::vec3 &pivot) {
	const glm::vec4 v((float)pos.x - 0.5f - pivot.x, (float)pos.y - 0.5f - pivot.y, (float)pos.z - 0.5f - pivot.z,
					  1.0f);
	const glm::vec3 &e = glm::vec3(mat * v) + 0.5f + pivot;
	const glm::ivec3 f = glm::floor(e);
	return f;
}

glm::vec3 transform(const glm::mat4x4 &mat, const glm::vec3 &pos, const glm::vec3 &pivot) {
	const glm::vec4 v(pos.x - 0.5f - pivot.x, pos.y - 0.5f - pivot.y, pos.z - 0.5f - pivot.z,
					  1.0f);
	return glm::vec3(mat * v) + 0.5f + pivot;
}

} // namespace math
