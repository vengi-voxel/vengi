#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_SWIZZLE
#define GLM_SWIZZLE // TODO: deprecated - remove me someday

#include <glm/glm.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <glm/exponential.hpp>
#include <glm/common.hpp>
#include <glm/trigonometric.hpp>

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/noise.hpp>
#include <glm/gtc/epsilon.hpp>

#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include <limits>
#include <cmath>

namespace glm {
extern const glm::vec3 forward;
extern const glm::vec3 backward;
extern const glm::vec3 right;
extern const glm::vec3 left;
extern const glm::vec3 up;
extern const glm::vec3 down;

// TODO: will be part of glm 0.9.8
GLM_FUNC_QUALIFIER bvec4 isnan(quat const & x) {
	const vec4 v(x.x, x.y, x.z, x.w);
	return isnan(v);
}

GLM_FUNC_QUALIFIER bvec4 isinf(quat const & x) {
	const vec4 v(x.x, x.y, x.z, x.w);
	return isinf(v);
}

}
