/**
 * @file
 */

#pragma once

#include "voxel/Palette.h"
#include "math/Math.h"
#include <gtest/gtest.h>

#define GLM_ENABLE_EXPERIMENTAL
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

namespace voxel {

inline ::std::ostream &operator<<(::std::ostream &os, const voxel::Palette &palette) {
	return os << voxel::Palette::print(palette).c_str();
}

}
