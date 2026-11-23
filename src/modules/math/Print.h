/**
 * @file
 */

#pragma once

#include "math/Mat4f.h"
#include "math/Quatf.h"
#include "math/Vec2f.h"
#include <ostream>
#include <stdio.h>

namespace math {

struct Text {
	char buf[256];
};

inline Text to_string(const Vec2f &v) {
	Text t;
	snprintf(t.buf, sizeof(t.buf), "vec2(%g, %g)", v.x, v.y);
	return t;
}
inline Text to_string(const Vec3f &v) {
	Text t;
	snprintf(t.buf, sizeof(t.buf), "vec3(%g, %g, %g)", v.x, v.y, v.z);
	return t;
}
inline Text to_string(const Vec4f &v) {
	Text t;
	snprintf(t.buf, sizeof(t.buf), "vec4(%g, %g, %g, %g)", v.x, v.y, v.z, v.w);
	return t;
}
inline Text to_string(const Vec3i &v) {
	Text t;
	snprintf(t.buf, sizeof(t.buf), "ivec3(%d, %d, %d)", v.x, v.y, v.z);
	return t;
}
inline Text to_string(const Vec4i &v) {
	Text t;
	snprintf(t.buf, sizeof(t.buf), "ivec4(%d, %d, %d, %d)", v.x, v.y, v.z, v.w);
	return t;
}
inline Text to_string(const Mat3f &m) {
	Text t;
	snprintf(t.buf, sizeof(t.buf), "mat3((%g, %g, %g), (%g, %g, %g), (%g, %g, %g))", m[0].x, m[0].y, m[0].z, m[1].x, m[1].y,
			 m[1].z, m[2].x, m[2].y, m[2].z);
	return t;
}
inline Text to_string(const Mat4f &m) {
	Text t;
	snprintf(t.buf, sizeof(t.buf), "mat4((%g, %g, %g, %g), (%g, %g, %g, %g), (%g, %g, %g, %g), (%g, %g, %g, %g))", m[0].x,
			 m[0].y, m[0].z, m[0].w, m[1].x, m[1].y, m[1].z, m[1].w, m[2].x, m[2].y, m[2].z, m[2].w, m[3].x, m[3].y, m[3].z,
			 m[3].w);
	return t;
}
inline Text to_string(const Quatf &q) {
	Text t;
	snprintf(t.buf, sizeof(t.buf), "quat(%g, {%g, %g, %g})", q.w, q.x, q.y, q.z);
	return t;
}

inline std::ostream &operator<<(std::ostream &os, const Text &t) {
	return os << t.buf;
}
inline std::ostream &operator<<(std::ostream &os, const Vec2f &v) {
	return os << to_string(v).buf;
}
inline std::ostream &operator<<(std::ostream &os, const Vec3f &v) {
	return os << to_string(v).buf;
}
inline std::ostream &operator<<(std::ostream &os, const Vec4f &v) {
	return os << to_string(v).buf;
}
inline std::ostream &operator<<(std::ostream &os, const Vec2i &v) {
	os << v.x << ", " << v.y;
	return os;
}
inline std::ostream &operator<<(std::ostream &os, const Vec3i &v) {
	os << v.x << ", " << v.y << ", " << v.z;
	return os;
}
inline std::ostream &operator<<(std::ostream &os, const Vec4i &v) {
	os << v.x << ", " << v.y << ", " << v.z << ", " << v.w;
	return os;
}
inline std::ostream &operator<<(std::ostream &os, const Mat4f &m) {
	return os << to_string(m).buf;
}
inline std::ostream &operator<<(std::ostream &os, const Mat3f &m) {
	return os << to_string(m).buf;
}
inline std::ostream &operator<<(std::ostream &os, const Quatf &q) {
	return os << to_string(q).buf;
}

}
