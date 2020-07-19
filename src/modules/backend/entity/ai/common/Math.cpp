/**
 * @file
 */

#include "Math.h"
#include <SDL_stdinc.h>

namespace backend {

bool parse(const core::String& in, glm::vec3& out) {
	if (SDL_sscanf(in.c_str(), "%f:%f:%f", &out.x, &out.y, &out.z) != 3) {
		return false;
	}

	return true;
}

}
