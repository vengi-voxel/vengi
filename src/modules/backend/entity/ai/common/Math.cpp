/**
 * @file
 */

#include "Math.h"
#include <SDL_stdinc.h>

namespace backend {

glm::vec3 parse(const core::String& in) {
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;

	if (SDL_sscanf(in.c_str(), "%f:%f:%f", &x, &y, &z) != 3) {
		return VEC3_INFINITE;
	}

	return glm::vec3(x, y, z);
}

}
