#include "Math.h"

namespace math {

glm::ivec3 transform(const glm::mat4x4 &mat, const glm::ivec3 &pos, const glm::vec3 &pivot) {
	const glm::vec4 v((float)pos.x - 0.5f - pivot.x, (float)pos.y - 0.5f - pivot.y, (float)pos.z - 0.5f - pivot.z,
					  1.0f);
	const glm::vec3 &e = mat * v;
	return glm::floor(e + 0.5f + pivot);
}

} // namespace math
