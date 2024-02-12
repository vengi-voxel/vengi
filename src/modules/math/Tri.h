/**
 * @file
 */

#pragma once

#include "core/RGBA.h"
#include "math/Math.h"

namespace math {

struct Tri {
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
};

} // namespace math
