/**
 * @file
 *
 * Right-handed, like OpenGL or Maya: +x right, +y up, +z toward the viewer.
 * The camera looks down the -z axis.
 */

#pragma once

#include "math/Vec3f.h"

namespace math {

inline const Vec3f &forward() {
	static const Vec3f v(0.0f, 0.0f, -1.0f);
	return v;
}
inline const Vec3f &backward() {
	static const Vec3f v(0.0f, 0.0f, 1.0f);
	return v;
}
inline const Vec3f &right() {
	static const Vec3f v(1.0f, 0.0f, 0.0f);
	return v;
}
inline const Vec3f &left() {
	static const Vec3f v(-1.0f, 0.0f, 0.0f);
	return v;
}
inline const Vec3f &up() {
	static const Vec3f v(0.0f, 1.0f, 0.0f);
	return v;
}
inline const Vec3f &down() {
	static const Vec3f v(0.0f, -1.0f, 0.0f);
	return v;
}

}
