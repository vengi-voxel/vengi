#include "SphereNoise.h"

namespace noise {

/**
 * @brief Transforms the latitude and longitude unit sphere coordinates into the
 * cartesian coordinates and uses those as input for the noise function.
 * @param[in] latitude Given in degrees - must be [-90,90]
 * @param[in] longitude Given in degrees - must be [-180,180]
 */
float sphereNoise(float longitude, float latitude) {
	const float r = glm::cos(glm::radians(latitude));
	glm::vec3 pos(glm::uninitialize);
	pos.x = (r * glm::cos(glm::radians(longitude)) + 1.0f) * 0.5f;
	pos.y = (glm::sin(glm::radians(latitude)) + 1.0f) * 0.5f;
	pos.z = (r * glm::sin(glm::radians(longitude)) + 1.0f) * 0.5f;
	return glm::simplex(pos);
}

}
