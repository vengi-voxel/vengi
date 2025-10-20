/**
 * @file
 * @brief  Stream utility functions that we don't want in the main stream header because the additional headers are not
 * always needed/wanted
 */

#pragma once

#include "core/RGBA.h"
#include "io/Stream.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace io {

inline bool readVec2(io::ReadStream &s, glm::vec2 &v) {
	return s.readFloat(v.x) == 0 && s.readFloat(v.y) == 0;
}

inline bool readVec3(io::ReadStream &s, glm::vec3 &v) {
	return s.readFloat(v.x) == 0 && s.readFloat(v.y) == 0 && s.readFloat(v.z) == 0;
}

inline bool readVec4(io::ReadStream &s, glm::vec4 &v) {
	return s.readFloat(v.x) == 0 && s.readFloat(v.y) == 0 && s.readFloat(v.z) == 0 && s.readFloat(v.w) == 0;
}

inline bool readVec2(io::ReadStream &s, glm::dvec2 &v) {
	return s.readDouble(v.x) == 0 && s.readDouble(v.y) == 0;
}

inline bool readVec3(io::ReadStream &s, glm::dvec3 &v) {
	return s.readDouble(v.x) == 0 && s.readDouble(v.y) == 0 && s.readDouble(v.z) == 0;
}

inline bool readVec4(io::ReadStream &s, glm::dvec4 &v) {
	return s.readDouble(v.x) == 0 && s.readDouble(v.y) == 0 && s.readDouble(v.z) == 0 && s.readDouble(v.w) == 0;
}

inline bool readVec2(io::ReadStream &s, glm::ivec2 &v) {
	return s.readInt32(v.x) == 0 && s.readInt32(v.y) == 0;
}

inline bool readVec3(io::ReadStream &s, glm::ivec3 &v) {
	return s.readInt32(v.x) == 0 && s.readInt32(v.y) == 0 && s.readInt32(v.z) == 0;
}

inline bool readVec4(io::ReadStream &s, glm::ivec4 &v) {
	return s.readInt32(v.x) == 0 && s.readInt32(v.y) == 0 && s.readInt32(v.z) == 0 && s.readInt32(v.w) == 0;
}

inline bool readVec2(io::ReadStream &s, glm::i16vec2 &v) {
	return s.readInt16(v.x) == 0 && s.readInt16(v.y) == 0;
}

inline bool readVec3(io::ReadStream &s, glm::i16vec3 &v) {
	return s.readInt16(v.x) == 0 && s.readInt16(v.y) == 0 && s.readInt16(v.z) == 0;
}

inline bool readVec4(io::ReadStream &s, glm::i16vec4 &v) {
	return s.readInt16(v.x) == 0 && s.readInt16(v.y) == 0 && s.readInt16(v.z) == 0 && s.readInt16(v.w) == 0;
}

inline bool readVec2(io::ReadStream &s, glm::u16vec2 &v) {
	return s.readUInt16(v.x) == 0 && s.readUInt16(v.y) == 0;
}

inline bool readVec3(io::ReadStream &s, glm::u16vec3 &v) {
	return s.readUInt16(v.x) == 0 && s.readUInt16(v.y) == 0 && s.readUInt16(v.z) == 0;
}

inline bool readVec4(io::ReadStream &s, glm::u16vec4 &v) {
	return s.readUInt16(v.x) == 0 && s.readUInt16(v.y) == 0 && s.readUInt16(v.z) == 0 && s.readUInt16(v.w) == 0;
}

inline bool readQuat(io::ReadStream &s, glm::quat &q) {
	return s.readFloat(q.x) == 0 && s.readFloat(q.y) == 0 && s.readFloat(q.z) == 0 && s.readFloat(q.w) == 0;
}

inline bool readColor(io::ReadStream &s, glm::vec4 &color) {
	return s.readFloat(color.r) == 0 && s.readFloat(color.g) == 0 && s.readFloat(color.b) == 0 &&
		   s.readFloat(color.a) == 0;
}

inline bool readColor(io::ReadStream &s, core::RGBA &color) {
	return s.readUInt8(color.r) == 0 && s.readUInt8(color.g) == 0 && s.readUInt8(color.b) == 0 &&
		   s.readUInt8(color.a) == 0;
}

} // namespace io
