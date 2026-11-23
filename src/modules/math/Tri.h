/**
 * @file
 */

#pragma once

#include "color/RGBA.h"
#include "math/Vec4f.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"

namespace math {

class Tri {
private:
	struct Vert {
		math::Vec3f pos{0};
		color::RGBA color{128, 128, 128};
	} _vertices[3];
	static_assert(sizeof(Vert) == 16, "Vert must have the expected size");

public:
	Tri() = default;
	inline constexpr Tri(const math::Vec3f (&v)[3], const color::RGBA (&c)[3]) {
		for (int i = 0; i < 3; ++i) {
			_vertices[i].pos = v[i];
			_vertices[i].color = c[i];
		}
	}

	void scaleVertices(float scale);
	void scaleVertices(const math::Vec3f &scale);

	math::Vec3f center() const;

	color::RGBA color0() const;
	color::RGBA color1() const;
	color::RGBA color2() const;

	const math::Vec3f &vertex0() const;
	const math::Vec3f &vertex1() const;
	const math::Vec3f &vertex2() const;

	math::Vec3f calculateBarycentric(const math::Vec3f &pos) const;
	/**
	 * @return @c true if the triangle is lying flat on one axis
	 */
	bool flat() const;
	math::Vec3f normal() const;
	float area() const;
	math::Vec3f mins() const;
	math::Vec3f maxs() const;
	math::Vec3i roundedMins() const;
	math::Vec3i roundedMaxs() const;

	// set the color for all three vertices
	void setColor(color::RGBA color);
	void setColor(const math::Vec4f &color);

	void setVertices(const math::Vec3f &v1, const math::Vec3f &v2, const math::Vec3f &v3);
	void setColor(const color::RGBA &c1, const color::RGBA &c2, const color::RGBA &c3);
};

inline math::Vec3f Tri::center() const {
	return (vertex0() + vertex1() + vertex2()) / 3.0f;
}

inline color::RGBA Tri::color0() const {
	return _vertices[0].color;
}

inline color::RGBA Tri::color1() const {
	return _vertices[1].color;
}

inline color::RGBA Tri::color2() const {
	return _vertices[2].color;
}

inline const math::Vec3f &Tri::vertex0() const {
	return _vertices[0].pos;
}

inline const math::Vec3f &Tri::vertex1() const {
	return _vertices[1].pos;
}

inline const math::Vec3f &Tri::vertex2() const {
	return _vertices[2].pos;
}

inline void Tri::setVertices(const math::Vec3f &v1, const math::Vec3f &v2, const math::Vec3f &v3) {
	_vertices[0].pos = v1;
	_vertices[1].pos = v2;
	_vertices[2].pos = v3;
}

inline void Tri::setColor(color::RGBA rgba) {
	setColor(rgba, rgba, rgba);
}

inline void Tri::setColor(const color::RGBA &c1, const color::RGBA &c2, const color::RGBA &c3) {
	_vertices[0].color = c1;
	_vertices[1].color = c2;
	_vertices[2].color = c3;
}

} // namespace math
