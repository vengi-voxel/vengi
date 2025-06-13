/**
 * @file
 */

#pragma once

#include "core/RGBA.h"
#include "math/Math.h"

namespace math {

class Tri {
private:
	glm::vec3 _vertices[3]{};
	core::RGBA _color[3]{core::RGBA(0, 0, 0), core::RGBA(0, 0, 0), core::RGBA(0, 0, 0)};

public:
	Tri() = default;
	inline constexpr Tri(const glm::vec3 (&v)[3], const core::RGBA (&c)[3]) {
		for (int i = 0; i < 3; ++i) {
			this->_vertices[i] = v[i];
		}
		for (int i = 0; i < 3; ++i) {
			this->_color[i] = c[i];
		}
	}
	virtual ~Tri() = default;

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
static_assert(sizeof(Tri) == 56, "Tri must have the expected size");

inline glm::vec3 Tri::center() const {
	return (vertex0() + vertex1() + vertex2()) / 3.0f;
}

inline core::RGBA Tri::color0() const {
	return _color[0];
}

inline core::RGBA Tri::color1() const {
	return _color[1];
}

inline core::RGBA Tri::color2() const {
	return _color[2];
}

inline const glm::vec3 &Tri::vertex0() const {
	return _vertices[0];
}

inline const glm::vec3 &Tri::vertex1() const {
	return _vertices[1];
}

inline const glm::vec3 &Tri::vertex2() const {
	return _vertices[2];
}

inline void Tri::setVertices(const glm::vec3 &v1, const glm::vec3 &v2, const glm::vec3 &v3) {
	_vertices[0] = v1;
	_vertices[1] = v2;
	_vertices[2] = v3;
}

inline void Tri::setColor(core::RGBA rgba) {
	_color[0] = rgba;
	_color[1] = rgba;
	_color[2] = rgba;
}

inline void Tri::setColor(const core::RGBA &c1, const core::RGBA &c2, const core::RGBA &c3) {
	_color[0] = c1;
	_color[1] = c2;
	_color[2] = c3;
}

} // namespace math
