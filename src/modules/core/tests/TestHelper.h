/**
 * @file
 */

#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

namespace core {

template<typename T, glm::qualifier P = glm::defaultp>
inline ::core::String& operator+= (::core::String& in, const glm::tvec1<T, P>& vec) {
	const std::string& tmp = glm::to_string(vec);
	in.append(tmp.c_str());
	return in;
}

template<typename T, glm::qualifier P = glm::defaultp>
inline ::core::String& operator+= (::core::String& in, const glm::tvec2<T, P>& vec) {
	const std::string& tmp = glm::to_string(vec);
	in.append(tmp.c_str());
	return in;
}

template<typename T, glm::qualifier P = glm::defaultp>
inline ::core::String& operator+= (::core::String& in, const glm::tvec3<T, P>& vec) {
	const std::string& tmp = glm::to_string(vec);
	in.append(tmp.c_str());
	return in;
}

template<typename T, glm::qualifier P = glm::defaultp>
inline ::core::String& operator+= (::core::String& in, const glm::tvec4<T, P>& vec) {
	const std::string& tmp = glm::to_string(vec);
	in.append(tmp.c_str());
	return in;
}

inline ::core::String& operator+= (::core::String& in, const glm::mat4& mat) {
	const std::string& tmp = glm::to_string(mat);
	in.append(tmp.c_str());
	return in;
}

inline ::core::String& operator+= (::core::String& in, const glm::mat3& mat) {
	const std::string& tmp = glm::to_string(mat);
	in.append(tmp.c_str());
	return in;
}

inline ::std::ostream& operator<<(::std::ostream& os, const glm::mat4& mat) {
	return os << "mat4x4[" << glm::to_string(mat) << "]";
}

inline ::std::ostream& operator<<(::std::ostream& os, const glm::mat3& mat) {
	return os << "mat3x3[" << glm::to_string(mat) << "]";
}

inline std::ostream &operator<<(::std::ostream &os, const core::String &dt) {
	return os << dt.c_str();
}

template<typename T, glm::qualifier P = glm::defaultp>
inline ::std::ostream& operator<<(::std::ostream& os, const glm::tvec4<T, P>& vec) {
	return os << "vec4[" << glm::to_string(vec) << "]";
}

template<typename T, glm::qualifier P = glm::defaultp>
inline ::std::ostream& operator<<(::std::ostream& os, const glm::tvec3<T, P>& vec) {
	return os << "vec3[" << glm::to_string(vec) << "]";
}

template<typename T, glm::qualifier P = glm::defaultp>
inline ::std::ostream& operator<<(::std::ostream& os, const glm::tvec2<T, P>& vec) {
	return os << "vec2[" << glm::to_string(vec) << "]";
}

template<typename T, glm::qualifier P = glm::defaultp>
inline ::std::ostream& operator<<(::std::ostream& os, const glm::tvec1<T, P>& vec) {
	return os << "vec1[" << glm::to_string(vec) << "]";
}

}

inline std::ostream& operator<<(std::ostream& stream, const glm::ivec2& v) {
	stream << "(x: " << v.x << ", y: " << v.y << ")";
	return stream;
}

inline std::ostream& operator<<(std::ostream& stream, const glm::vec2& v) {
	stream << "(x: " << v.x << ", y: " << v.y << ")";
	return stream;
}

inline std::ostream& operator<<(std::ostream& stream, const glm::ivec3& v) {
	stream << "(x: " << v.x << ", y: " << v.y << ", z: " << v.z << ")";
	return stream;
}

inline std::ostream& operator<<(std::ostream& stream, const glm::vec3& v) {
	stream << "(x: " << v.x << ", y: " << v.y << ", z: " << v.z << ")";
	return stream;
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
