/**
 * @file
 */

#include "Math.h"
#include <stdio.h>

namespace backend {

glm::vec3 parse(const core::String& in) {
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;

#ifdef _MSC_VER
	if (::sscanf_s(in.c_str(), "%f:%f:%f", &x, &y, &z) != 3) {
#else
	if (::sscanf(in.c_str(), "%f:%f:%f", &x, &y, &z) != 3) {
#endif
		return VEC3_INFINITE;
	}

	return glm::vec3(x, y, z);
}

}
