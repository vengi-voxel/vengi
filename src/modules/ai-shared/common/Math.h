/**
 * @file
 */
#pragma once

#include <glm/vec3.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/vector_relational.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/geometric.hpp>
#include <limits.h>
#include <math.h>
#include "core/String.h"

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

extern glm::vec3 parse(const core::String& in);

}
