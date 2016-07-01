#pragma once

#include "Types.h"

#define GLM_FORCE_RADIANS
//#define GLM_SWIZZLE

DISABLE_WARNING(shadow,shadow,0)
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/norm.hpp>
ENABLE_WARNING(shadow,shadow,0)
#include <limits>
#include <cmath>

namespace ai {

inline float toRadians (float degree) {
	return glm::radians(degree);
}

inline bool isInfinite (const glm::vec3& vec) {
	const auto& inf = glm::isinf(vec);
	return inf.x && inf.y && inf.z;
}

inline float toDegrees (float radians) {
	return glm::degrees(radians);
}

inline glm::vec3 fromRadians(float radians) {
	return glm::vec3(cosf(radians), 0.0f, sinf(radians));
}

inline double angle(const glm::vec3& v) {
	const double _angle = ::atan2(v.z, v.x);
	return _angle;
}

inline glm::vec3 advance (const glm::vec3& src, const glm::vec3& direction, const float scale) {
	const float _x = src.x + scale * direction.x;
	const float _y = src.y + scale * direction.y;
	const float _z = src.z + scale * direction.z;
	return glm::vec3(_x, _y, _z);
}

template<typename T>
inline T clamp(T a, T low, T high) {
	return glm::clamp(a, low, high);
}

static const glm::vec3 ZERO(0.0f);
static const glm::vec3 INFINITE(std::numeric_limits<float>::infinity());

inline glm::vec3 parse(const std::string& in) {
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;

	if (::sscanf(in.c_str(), "%f:%f:%f", &x, &y, &z) != 3) {
		return INFINITE;
	}

	return glm::vec3(x, y, z);
}

}
