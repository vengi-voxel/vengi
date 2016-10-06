#include "SphereNoise.h"

namespace noise {

float sphereNoise(const glm::vec2& v, const glm::vec2& dim) {
	const float lat = v.y / dim.y * 180 - 90;
	const float lon = v.x / dim.x * 360 - 180;
	const float r = glm::cos(glm::radians(lat));

	glm::vec3 pos(glm::uninitialize);
	pos.x = (r * glm::cos(glm::radians(lon)) + 1.0f) * 0.5f;
	pos.y = (glm::sin(glm::radians(lat)) + 1.0f) * 0.5f;
	pos.z = (r * glm::sin(glm::radians(lon)) + 1.0f) * 0.5f;
	return glm::simplex(pos);
}

}
