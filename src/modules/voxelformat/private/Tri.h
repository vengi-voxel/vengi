/**
 * @file
 */

#pragma once

#include "core/RGBA.h"
#include "image/Image.h"
#include "math/Math.h"
#include "video/Types.h"

namespace voxelformat {

// https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexParameter.xhtml
enum TextureWrap {
	Repeat, // causes the integer part of the s coordinate to be ignored; the GL uses only the fractional part, thereby
			// creating a repeating pattern.
	ClampToEdge, // causes s coordinates to be clamped to the range [1/2N,1−1/2N], where N is the size of the texture in
				 // the direction of clamping.
	MirroredRepeat, // causes the s coordinate to be set to the fractional part of the texture coordinate if the integer
					// part of s is even; if the integer part of s is odd, then the s texture coordinate is set to
					// 1−frac(s), where frac(s) represents the fractional part of s

	Max
};

struct Tri {
	glm::vec3 vertices[3];
	glm::vec2 uv[3];
	image::ImagePtr texture;
	core::RGBA color{0xFFFFFFFF};
	TextureWrap wrapS = TextureWrap::Repeat;
	TextureWrap wrapT = TextureWrap::Repeat;

	constexpr glm::vec2 centerUV() const {
		return (uv[0] + uv[1] + uv[2]) / 3.0f;
	}

	constexpr glm::vec3 center() const {
		return (vertices[0] + vertices[1] + vertices[2]) / 3.0f;
	}

	float area() const;
	glm::vec3 mins() const;
	glm::vec3 maxs() const;
	core::RGBA colorAt(const glm::vec2 &uv) const;

	// Sierpinski gasket with keeping the middle
	void subdivide(Tri out[4]) const;
};

} // namespace voxelformat
