/**
 * @file
 */

#pragma once

#include "core/RGBA.h"
#include "math/Math.h"

namespace math {

class Tri {
public:
	Tri() = default;
	inline constexpr Tri(const glm::vec3 (&_vertices)[3], const core::RGBA (&_color)[3]) {
		for (int i = 0; i < 3; ++i) {
			this->vertices[i] = _vertices[i];
		}
		for (int i = 0; i < 3; ++i) {
			this->color[i] = _color[i];
		}
	}
	virtual ~Tri() = default;
	glm::vec3 vertices[3]{};
	core::RGBA color[3]{core::RGBA(0, 0, 0), core::RGBA(0, 0, 0), core::RGBA(0, 0, 0)};

	glm::vec3 center() const {
		return (vertices[0] + vertices[1] + vertices[2]) / 3.0f;
	}

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

inline void Tri::setVertices(const glm::vec3 &v1, const glm::vec3 &v2, const glm::vec3 &v3) {
	vertices[0] = v1;
	vertices[1] = v2;
	vertices[2] = v3;
}

inline void Tri::setColor(const core::RGBA &c1, const core::RGBA &c2, const core::RGBA &c3) {
	color[0] = c1;
	color[1] = c2;
	color[2] = c3;
}

} // namespace math
