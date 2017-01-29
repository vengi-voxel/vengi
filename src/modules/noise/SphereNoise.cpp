#include "SphereNoise.h"

namespace noise {

/**
 * @brief Transforms the latitude and longitude unit sphere coordinates into the
 * cartesian coordinates and uses those as input for the noise function.
 * @param[in] latitude Given in degrees - must be [-90,90]
 * @param[in] longitude Given in degrees - must be [-180,180]
 */
float sphereNoise(float longitude, float latitude) {
	const float latRad = glm::radians(latitude);
	const float longRad = glm::radians(longitude);
	const float r = glm::cos(latRad);
	glm::vec3 pos(glm::uninitialize);
	pos.x = glm::sin(longRad) * r;
	pos.y = glm::sin(latRad);
	pos.z = glm::cos(longRad) * r;
	return glm::simplex(pos);
}

}
