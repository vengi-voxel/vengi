/**
 * @file
 */

#pragma once

#include "color/RGBA.h"
#include "math/Math.h"

namespace math {

class Tri {
private:
	struct Vert {
		glm::vec3 pos{0};
		core::RGBA color{0, 0, 0};
	} _vertices[3];
	static_assert(sizeof(Vert) == 16, "Vert must have the expected size");

public:
	Tri() = default;
	inline constexpr Tri(const glm::vec3 (&v)[3], const core::RGBA (&c)[3]) {
		for (int i = 0; i < 3; ++i) {
			_vertices[i].pos = v[i];
			_vertices[i].color = c[i];
		}
	}

	void scaleVertices(float scale);
	void scaleVertices(const glm::vec3 &scale);

	glm::vec3 center() const;

	core::RGBA color0() const;
	core::RGBA color1() const;
	core::RGBA color2() const;

	const glm::vec3 &vertex0() const;
	const glm::vec3 &vertex1() const;
	const glm::vec3 &vertex2() const;

	glm::vec3 calculateBarycentric(const glm::vec3 &pos) const;
	/**
	 * @return @c true if the triangle is lying flat on one axis
	 */
	bool flat() const;
	glm::vec3 normal() const;
	float area() const;
	glm::vec3 mins() const;
	glm::vec3 maxs() const;
	glm::ivec3 roundedMins() const;
	glm::ivec3 roundedMaxs() const;

	// set the color for all three vertices
	void setColor(core::RGBA color);
	void setColor(const glm::vec4 &color);

	void setVertices(const glm::vec3 &v1, const glm::vec3 &v2, const glm::vec3 &v3);
	void setColor(const core::RGBA &c1, const core::RGBA &c2, const core::RGBA &c3);
};

inline glm::vec3 Tri::center() const {
	return (vertex0() + vertex1() + vertex2()) / 3.0f;
}

inline core::RGBA Tri::color0() const {
	return _vertices[0].color;
}

inline core::RGBA Tri::color1() const {
	return _vertices[1].color;
}

inline core::RGBA Tri::color2() const {
	return _vertices[2].color;
}

inline const glm::vec3 &Tri::vertex0() const {
	return _vertices[0].pos;
}

inline const glm::vec3 &Tri::vertex1() const {
	return _vertices[1].pos;
}

inline const glm::vec3 &Tri::vertex2() const {
	return _vertices[2].pos;
}

inline void Tri::setVertices(const glm::vec3 &v1, const glm::vec3 &v2, const glm::vec3 &v3) {
	_vertices[0].pos = v1;
	_vertices[1].pos = v2;
	_vertices[2].pos = v3;
}

inline void Tri::setColor(core::RGBA rgba) {
	setColor(rgba, rgba, rgba);
}

inline void Tri::setColor(const core::RGBA &c1, const core::RGBA &c2, const core::RGBA &c3) {
	_vertices[0].color = c1;
	_vertices[1].color = c2;
	_vertices[2].color = c3;
}

} // namespace math
