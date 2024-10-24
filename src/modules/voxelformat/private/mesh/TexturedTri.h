/**
 * @file
 */

#pragma once

#include "image/Image.h"
#include "math/Tri.h"

namespace voxelformat {

struct TexturedTri : public math::Tri {
	TexturedTri() = default;
	inline TexturedTri(const glm::vec3 (&_vertices)[3], const glm::vec2 (&_uv)[3], const image::ImagePtr &_texture,
								 const core::RGBA (&_color)[3])
		: math::Tri::Tri(_vertices, _color) {
		for (int i = 0; i < 3; ++i) {
			this->uv[i] = _uv[i];
		}
		this->texture = _texture;
	}

	virtual ~TexturedTri() = default;

	glm::vec2 uv[3]{};
	image::ImagePtr texture;
	image::TextureWrap wrapS = image::TextureWrap::Repeat;
	image::TextureWrap wrapT = image::TextureWrap::Repeat;

	glm::vec2 centerUV() const;

	/**
	 * @return @c false if the given position is not within the triangle area. The value of uv should not be used in
	 * this case.
	 */
	[[nodiscard]] bool calcUVs(const glm::vec3 &pos, glm::vec2 &uv) const;
	core::RGBA colorAt(const glm::vec2 &uv, bool originUpperLeft = false) const;
	core::RGBA centerColor() const;

	// Sierpinski gasket with keeping the middle
	void subdivide(TexturedTri out[4]) const;
};

} // namespace voxelformat
