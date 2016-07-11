#pragma once

#define GLM_FORCE_RADIANS
#define GLM_SWIZZLE

#include <glm/glm.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/noise.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/detail/func_common.hpp>
#include <limits>
#include <cmath>

namespace glm {
static const glm::vec3& forward  = glm::vec3( 0.0f,  0.0f, -1.0f);
static const glm::vec3& backward = glm::vec3( 0.0f,  0.0f,  1.0f);
static const glm::vec3& right    = glm::vec3( 1.0f,  0.0f,  0.0f);
static const glm::vec3& left     = glm::vec3(-1.0f,  0.0f,  0.0f);
static const glm::vec3& up       = glm::vec3( 0.0f,  1.0f,  0.0f);
static const glm::vec3& down     = glm::vec3( 0.0f, -1.0f,  0.0f);

GLM_FUNC_QUALIFIER bvec4 isnan(quat const & x) {
	const vec4 v(x.x, x.y, x.z, x.w);
	return isnan(v);
}

}
