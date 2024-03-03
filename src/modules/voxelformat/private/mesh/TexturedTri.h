/**
 * @file
 */

#pragma once

#include "image/Image.h"
#include "math/Tri.h"

namespace voxelformat {

struct TexturedTri : public math::Tri {
	TexturedTri() = default;
	inline TexturedTri(const glm::vec3 (&vertices)[3], const glm::vec2 (&uv)[3], const image::ImagePtr &texture,
								 const core::RGBA (&color)[3])
		: math::Tri::Tri(vertices, color) {
		for (int i = 0; i < 3; ++i) {
			this->uv[i] = uv[i];
		}
		this->texture = texture;
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
	core::RGBA colorAt(const glm::vec2 &uv) const;
	core::RGBA centerColor() const;

	// Sierpinski gasket with keeping the middle
	void subdivide(TexturedTri out[4]) const;
};

} // namespace voxelformat
