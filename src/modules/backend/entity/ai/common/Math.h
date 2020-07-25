/**
 * @file
 */
#pragma once

#include <glm/vec3.hpp>
#include <glm/trigonometric.hpp>
#include "core/String.h"

namespace backend {

inline float angle(const glm::vec3& v) {
	return glm::atan(v.z, v.x);
}

inline glm::vec3 advance (const glm::vec3& src, const glm::vec3& direction, const float scale) {
	return src + (scale * direction);
}

static const glm::vec3 ZERO(0.0f);

extern bool parse(const core::String& in, glm::vec3& out);

}
