/**
 * @file
 */

#pragma once

#include <glm/fwd.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <gtest/gtest.h>
#include "core/String.h"
#include "core/collection/DynamicArray.h"

namespace core {

inline std::ostream &operator<<(::std::ostream &os, const String &dt) {
	return os << dt.c_str();
}

}

namespace glm {

inline ::std::ostream& operator<<(::std::ostream& os, const mat4& mat) {
	return os << "mat4x4[" << glm::to_string(mat) << "]";
}

inline ::std::ostream& operator<<(::std::ostream& os, const mat3& mat) {
	return os << "mat3x3[" << glm::to_string(mat) << "]";
}

template<typename T, glm::qualifier P = glm::defaultp>
inline ::std::ostream& operator<<(::std::ostream& os, const tvec4<T, P>& vec) {
	return os << "vec4[" << glm::to_string(vec) << "]";
}

template<typename T, glm::qualifier P = glm::defaultp>
inline ::std::ostream& operator<<(::std::ostream& os, const tvec3<T, P>& vec) {
	return os << "vec3[" << glm::to_string(vec) << "]";
}

template<typename T, glm::qualifier P = glm::defaultp>
inline ::std::ostream& operator<<(::std::ostream& os, const tvec2<T, P>& vec) {
	return os << "vec2[" << glm::to_string(vec) << "]";
}

template<typename T, glm::qualifier P = glm::defaultp>
inline ::std::ostream& operator<<(::std::ostream& os, const tvec1<T, P>& vec) {
	return os << "vec1[" << glm::to_string(vec) << "]";
}

inline std::ostream& operator<<(std::ostream& stream, const ivec2& v) {
	stream << "(x: " << v.x << ", y: " << v.y << ")";
	return stream;
}

inline std::ostream& operator<<(std::ostream& stream, const vec2& v) {
	stream << "(x: " << v.x << ", y: " << v.y << ")";
	return stream;
}

inline std::ostream& operator<<(std::ostream& stream, const ivec3& v) {
	stream << "(x: " << v.x << ", y: " << v.y << ", z: " << v.z << ")";
	return stream;
}

inline std::ostream& operator<<(std::ostream& stream, const vec3& v) {
	stream << "(x: " << v.x << ", y: " << v.y << ", z: " << v.z << ")";
	return stream;
}

}

template<class T>
core::String toString(const core::DynamicArray<T>& v) {
	core::String str;
	str.reserve(4096);
	for (auto i = v.begin(); i != v.end();) {
		str += "'";
		str += *i;
		str += "'";
		if (++i != v.end()) {
			str += ", ";
		}
	}
	return str;
}
