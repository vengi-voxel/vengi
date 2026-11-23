/**
 * @file
 * @brief  Stream utility functions that we don't want in the main stream header because the additional headers are not
 * always needed/wanted
 */

#pragma once

#include "color/RGBA.h"
#include "io/Stream.h"
#include "math/U16Vec3.h"
#include "math/Vec4d.h"
#include "math/Vec4i.h"
#include "math/Vec4f.h"
#include "math/Vec3d.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"
#include "math/Vec2d.h"
#include "math/Vec2i.h"
#include "math/Vec2f.h"

namespace io {

inline bool readVec2(io::ReadStream &s, math::Vec2f &v) {
	return s.readFloat(v.x) == 0 && s.readFloat(v.y) == 0;
}

inline bool readVec3(io::ReadStream &s, math::Vec3f &v) {
	return s.readFloat(v.x) == 0 && s.readFloat(v.y) == 0 && s.readFloat(v.z) == 0;
}

inline bool readVec4(io::ReadStream &s, math::Vec4f &v) {
	return s.readFloat(v.x) == 0 && s.readFloat(v.y) == 0 && s.readFloat(v.z) == 0 && s.readFloat(v.w) == 0;
}

inline bool readVec2(io::ReadStream &s, math::Vec2d &v) {
	return s.readDouble(v.x) == 0 && s.readDouble(v.y) == 0;
}

inline bool readVec3(io::ReadStream &s, math::Vec3d &v) {
	return s.readDouble(v.x) == 0 && s.readDouble(v.y) == 0 && s.readDouble(v.z) == 0;
}

inline bool readVec4(io::ReadStream &s, math::Vec4d &v) {
	return s.readDouble(v.x) == 0 && s.readDouble(v.y) == 0 && s.readDouble(v.z) == 0 && s.readDouble(v.w) == 0;
}

inline bool readVec2(io::ReadStream &s, math::Vec2i &v) {
	return s.readInt32(v.x) == 0 && s.readInt32(v.y) == 0;
}

inline bool readVec3(io::ReadStream &s, math::Vec3i &v) {
	return s.readInt32(v.x) == 0 && s.readInt32(v.y) == 0 && s.readInt32(v.z) == 0;
}

inline bool readVec4(io::ReadStream &s, math::Vec4i &v) {
	return s.readInt32(v.x) == 0 && s.readInt32(v.y) == 0 && s.readInt32(v.z) == 0 && s.readInt32(v.w) == 0;
}

inline bool readVec3(io::ReadStream &s, math::U16Vec3 &v) {
	return s.readUInt16(v.x) == 0 && s.readUInt16(v.y) == 0 && s.readUInt16(v.z) == 0;
}

inline bool readColor(io::ReadStream &s, math::Vec4f &color) {
	return s.readFloat(color.r) == 0 && s.readFloat(color.g) == 0 && s.readFloat(color.b) == 0 &&
		   s.readFloat(color.a) == 0;
}

inline bool readColor(io::ReadStream &s, color::RGBA &color) {
	return s.readUInt8(color.r) == 0 && s.readUInt8(color.g) == 0 && s.readUInt8(color.b) == 0 &&
		   s.readUInt8(color.a) == 0;
}

} // namespace io
