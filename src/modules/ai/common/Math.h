/**
 * @file
 */
#pragma once

#define GLM_FORCE_RADIANS
//#define GLM_SWIZZLE

#include <glm/vec3.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/epsilon.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/norm.hpp>
#include <limits.h>
#include <math.h>
#include <string>

namespace ai {

inline float toRadians (float degree) {
	return glm::radians(degree);
}

inline bool isInfinite (const glm::vec3& vec) {
	return glm::any(glm::isinf(vec));
}

inline float toDegrees (float radians) {
	return glm::degrees(radians);
}

inline glm::vec3 fromRadians(float radians) {
	return glm::vec3(glm::cos(radians), 0.0f, glm::sin(radians));
}

inline float angle(const glm::vec3& v) {
	const float _angle = glm::atan(v.z, v.x);
	return _angle;
}

inline glm::vec3 advance (const glm::vec3& src, const glm::vec3& direction, const float scale) {
	return src + (scale * direction);
}

template<typename T>
inline T clamp(T a, T low, T high) {
	return glm::clamp(a, low, high);
}

static const glm::vec3 ZERO(0.0f);
static const glm::vec3 VEC3_INFINITE(std::numeric_limits<float>::infinity());

inline glm::vec3 parse(const std::string& in) {
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
