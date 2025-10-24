/**
 * @file
 *
 * What this basically says is that
 * @li the positive x-axis is to your right
 * @li the positive y-axis is up
 * @li the positive z-axis is backwards
 * Think of your screen being the center of the 3 axes and the positive z-axis going through your
 * screen towards you.
 *
 * vengi is using a right-handed coordinate system like OpenGL or Maya
 *
 *  y as the up axis
 * +x to the right
 * +z pointing toward the viewer (so the camera looks down the -z axis)
 */

#pragma once

#include <glm/vec3.hpp>

namespace glm {

inline const vec3 &forward() {
	static const vec3 v(0.0f, 0.0f, -1.0f);
	return v;
}
inline const vec3 &backward() {
	static const vec3 v(0.0f, 0.0f, 1.0f);
	return v;
}
inline const vec3 &right() {
	static const vec3 v(1.0f, 0.0f, 0.0f);
	return v;
}
inline const vec3 &left() {
	static const vec3 v(-1.0f, 0.0f, 0.0f);
	return v;
}
inline const vec3 &up() {
	static const vec3 v(0.0f, 1.0f, 0.0f);
	return v;
}
inline const vec3 &down() {
	static const vec3 v(0.0f, -1.0f, 0.0f);
	return v;
}

} // namespace glm
