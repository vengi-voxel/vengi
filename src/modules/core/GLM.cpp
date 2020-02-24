/**
 * @file
 */

#include "GLM.h"
#include <glm/gtc/matrix_access.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

namespace glm {

float intersectLines(const glm::vec3& p1,
		const glm::vec3& p2, const glm::vec3& p3, const glm::vec3& p4,
		glm::vec3& pa, glm::vec3& pb, float *pmua,
		float *pmub) {
	const glm::vec3 p13 = p1 - p3;
	const glm::vec3 p43 = p4 - p3;
	const vec3 eps(glm::epsilon<float>());
	if (glm::all(glm::lessThan(glm::abs(p43), eps))) {
		return false;
	}
	const glm::vec3 p21 = p2 - p1;
	if (glm::all(glm::lessThan(glm::abs(p21), eps))) {
		return false;
	}

	const float d1343 = glm::dot(p13, p43);
	const float d4321 = glm::dot(p43, p21);
	const float d1321 = glm::dot(p13, p21);
	const float d4343 = glm::dot(p43, p43);
	const float d2121 = glm::dot(p21, p21);

	const float denom = d2121 * d4343 - d4321 * d4321;
	if (glm::abs(denom) < glm::epsilon<float>()) {
		return false;
	}
	const float numer = d1343 * d4321 - d1321 * d4343;
	const float mua = numer / denom;
	const float mub = (d1343 + d4321 * mua) / d4343;

	pa = p1 + mua * p21;
	pb = p3 + mub * p43;

	if (pmua != nullptr) {
		*pmua = mua;
	}
	if (pmub != nullptr) {
		*pmub = mub;
	}

	return true;
}

vec3 transform(const mat4& mat, const vec3& v) {
	const mat4::col_type& c1 = column(mat, 0);
	const mat4::col_type& c2 = column(mat, 1);
	const mat4::col_type& c3 = column(mat, 2);
	vec3 r(c1.x * v.x + c1.y * v.y + c1.z * v.z + c1.w,
		c2.x * v.x + c2.y * v.y + c2.z * v.z + c2.w,
		c3.x * v.x + c3.y * v.y + c3.z * v.z + c3.w);
	return r;
}

vec3 rotate(const mat4& mat, const vec3& v) {
	const mat4::col_type& c1 = column(mat, 0);
	const mat4::col_type& c2 = column(mat, 1);
	const mat4::col_type& c3 = column(mat, 2);
	const vec3 r(c1.x * v.x + c1.y * v.y + c1.z * v.z,
		c2.x * v.x + c2.y * v.y + c2.z * v.z,
		c3.x * v.x + c3.y * v.y + c3.z * v.z);
	return r;
}

vec3 project(const mat4& m, const vec3& p) {
	const vec4& r = m * vec4(p, 1);
	return vec3(r) / r.w;
}

glm::mat3x4 invert(const glm::mat3x4& o) {
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

mat3x4 operator*(const mat3x4& lhs, const mat3x4 &o) {
	glm::vec4 c1 = (o[0] * lhs[0].x + o[1] * lhs[0].y + o[2] * lhs[0].z);
	c1.w += lhs[0].w;
	glm::vec4 c2 = (o[0] * lhs[1].x + o[1] * lhs[1].y + o[2] * lhs[1].z);
	c2.w += lhs[1].w;
	glm::vec4 c3 = (o[0] * lhs[2].x + o[1] * lhs[2].y + o[2] * lhs[2].z);
	c3.w += lhs[2].w;
	return mat3x4(c1, c2, c3);
}

}

