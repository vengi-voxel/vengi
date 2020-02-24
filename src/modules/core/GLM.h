/**
 * @file
 */

#pragma once

#include <glm/fwd.hpp>
#include <glm/vec3.hpp>

namespace glm {

constexpr vec3 forward  = vec3( 0.0f,  0.0f, -1.0f);
constexpr vec3 backward = vec3( 0.0f,  0.0f,  1.0f);
constexpr vec3 right    = vec3( 1.0f,  0.0f,  0.0f);
constexpr vec3 left     = vec3(-1.0f,  0.0f,  0.0f);
constexpr vec3 up       = vec3( 0.0f,  1.0f,  0.0f);
constexpr vec3 down     = vec3( 0.0f, -1.0f,  0.0f);

/**
 * Calculate the line segment PaPb that is the shortest route between
 * two lines P1P2 and P3P4. Calculate also the values of @c mua and @c mub where
 *  Pa = P1 + mua (P2 - P1)
 *  Pb = P3 + mub (P4 - P3)
 * @return @c false if no solution exists.
 */
float intersectLines(const glm::vec3& p1,
		const glm::vec3& p2, const glm::vec3& p3, const glm::vec3& p4,
		glm::vec3& pa, glm::vec3& pb, float *pmua = nullptr,
		float *pmub = nullptr);

vec3 transform(const mat4& mat, const vec3& v);

vec3 rotate(const mat4& mat, const vec3& v);

vec3 project(const mat4& m, const vec3& p);

glm::mat3x4 invert(const glm::mat3x4& o);

mat3x4 operator*(const mat3x4& lhs, const mat3x4 &o);

}
