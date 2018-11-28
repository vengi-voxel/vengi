#pragma once

#define GLM_FORCE_RADIANS

#include <functional>
#include <limits>
#include <math.h>

#include <glm/gtc/integer.hpp>
#include <glm/glm.hpp>

#include <glm/gtc/round.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/packing.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/noise.hpp>
#include <glm/gtc/epsilon.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/rotate_vector.hpp>

template<int N, typename T>
struct VecLessThan {
	inline bool operator()(const glm::vec<N, T>& lhs, const glm::vec<N, T>& rhs) const {
		return glm::all(glm::lessThan(lhs, rhs));
	}
};

namespace glm {
extern const glm::vec3 forward;
extern const glm::vec3 backward;
extern const glm::vec3 right;
extern const glm::vec3 left;
extern const glm::vec3 up;
extern const glm::vec3 down;

GLM_FUNC_QUALIFIER vec3 transform(const mat4& mat, const vec3& v) {
	const mat4::col_type& c1 = column(mat, 0);
	const mat4::col_type& c2 = column(mat, 1);
	const mat4::col_type& c3 = column(mat, 2);
	vec3 r(c1.x * v.x + c1.y * v.y + c1.z * v.z + c1.w,
		c2.x * v.x + c2.y * v.y + c2.z * v.z + c2.w,
		c3.x * v.x + c3.y * v.y + c3.z * v.z + c3.w);
	return r;
}

GLM_FUNC_QUALIFIER glm::vec3 transform(const mat3& mat, const glm::vec3& v) {
	const mat3::col_type& c1 = column(mat, 0);
	const mat3::col_type& c2 = column(mat, 1);
	const mat3::col_type& c3 = column(mat, 2);
	const vec3 r(dot(c1, v), dot(c2, v), dot(c3, v));
	return r;
}

GLM_FUNC_QUALIFIER glm::vec3 transform(const mat3x4& mat, const glm::vec3& v) {
	const mat3x4::col_type& c1 = column(mat, 0);
	const mat3x4::col_type& c2 = column(mat, 1);
	const mat3x4::col_type& c3 = column(mat, 2);
	const vec3 r(c1.x * v.x + c1.y * v.y + c1.z * v.z + c1.w,
		c2.x * v.x + c2.y * v.y + c2.z * v.z + c2.w,
		c3.x * v.x + c3.y * v.y + c3.z * v.z + c3.w);
	return r;
}

GLM_FUNC_QUALIFIER vec3 rotate(const mat4& mat, const vec3& v) {
	const mat4::col_type& c1 = column(mat, 0);
	const mat4::col_type& c2 = column(mat, 1);
	const mat4::col_type& c3 = column(mat, 2);
	const vec3 r(c1.x * v.x + c1.y * v.y + c1.z * v.z,
		c2.x * v.x + c2.y * v.y + c2.z * v.z,
		c3.x * v.x + c3.y * v.y + c3.z * v.z);
	return r;
}

GLM_FUNC_QUALIFIER vec3 project(const mat4& m, const vec3& p) {
	const vec4& r = m * vec4(p, 1);
	return vec3(r) / r.w;
}

GLM_FUNC_QUALIFIER glm::mat3x4 invert(const glm::mat3x4& o) {
	mat3x3 invrot(vec3(o[0].x, o[1].x, o[2].x), vec3(o[0].y, o[1].y, o[2].y), vec3(o[0].z, o[1].z, o[2].z));
	invrot[0] /= glm::length2(invrot[0]);
	invrot[1] /= glm::length2(invrot[1]);
	invrot[2] /= glm::length2(invrot[2]);
	const glm::vec3 trans(o[0].w, o[1].w, o[2].w);
	const glm::vec4 a(invrot[0], -dot(invrot[0], trans));
	const glm::vec4 b(invrot[1], -dot(invrot[1], trans));
	const glm::vec4 c(invrot[2], -dot(invrot[2], trans));
	return glm::mat3x4(a, b, c);
}

GLM_FUNC_QUALIFIER mat3x4 operator*(const mat3x4& lhs, const mat3x4 &o) {
	glm::vec4 c1 = (o[0] * lhs[0].x + o[1] * lhs[0].y + o[2] * lhs[0].z);
	c1.w += lhs[0].w;
	glm::vec4 c2 = (o[0] * lhs[1].x + o[1] * lhs[1].y + o[2] * lhs[1].z);
	c2.w += lhs[1].w;
	glm::vec4 c3 = (o[0] * lhs[2].x + o[1] * lhs[2].y + o[2] * lhs[2].z);
	c3.w += lhs[2].w;
	return mat3x4(c1, c2, c3);
}


}
