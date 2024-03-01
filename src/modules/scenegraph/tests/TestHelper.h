/**
 * @file
 */

#pragma once

#include "palette/Palette.h"
#include "math/Math.h"
#include <gtest/gtest.h>

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/string_cast.hpp>

namespace glm {
inline ::std::ostream &operator<<(::std::ostream &os, const mat4x4 &matrix) {
	os << to_string(matrix);
	return os;
}
inline ::std::ostream &operator<<(::std::ostream &os, const mat3x3 &matrix) {
	os << to_string(matrix);
	return os;
}
inline ::std::ostream &operator<<(::std::ostream &os, const mat4x3 &matrix) {
	os << to_string(matrix);
	return os;
}
inline ::std::ostream &operator<<(::std::ostream &os, const vec2 &v) {
	os << to_string(v);
	return os;
}
inline ::std::ostream &operator<<(::std::ostream &os, const vec3 &v) {
	os << to_string(v);
	return os;
}
inline ::std::ostream &operator<<(::std::ostream &os, const vec4 &v) {
	os << to_string(v);
	return os;
}
inline ::std::ostream &operator<<(::std::ostream &os, const ivec2 &v) {
	os << to_string(v);
	return os;
}
inline ::std::ostream &operator<<(::std::ostream &os, const ivec3 &v) {
	os << to_string(v);
	return os;
}
inline ::std::ostream &operator<<(::std::ostream &os, const ivec4 &v) {
	os << to_string(v);
	return os;
}
}

namespace palette {

inline ::std::ostream &operator<<(::std::ostream &os, const palette::Palette &palette) {
	return os << palette::Palette::print(palette).c_str();
}

inline ::std::ostream &operator<<(::std::ostream &os, const palette::Material &material) {
	os << "Material: " << (int)material.type << " ";
	for (uint32_t i = 0; i < palette::MaterialProperty::MaterialMax - 1; ++i) {
		if (!material.has((palette::MaterialProperty)i)) {
			continue;
		}
		os << palette::MaterialPropertyNames[i] << ": " << material.value((palette::MaterialProperty)i) << ", ";
	}
	return os;
}


}
