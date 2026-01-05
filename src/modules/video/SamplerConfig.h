/**
 * @file
 */

#pragma once

#include "video/Types.h"
#include <glm/vec4.hpp>

namespace video {

struct SamplerConfig {
	TextureWrap wrapR = TextureWrap::Max;
	TextureWrap wrapS = TextureWrap::Max;
	TextureWrap wrapT = TextureWrap::Max;
	TextureFilter filterMag = TextureFilter::Linear;
	TextureFilter filterMin = TextureFilter::Linear;
	CompareFunc compareFunc = CompareFunc::Max;
	TextureCompareMode compareMode = TextureCompareMode::Max;
	// Reduces blur and shimmering at oblique viewing angles.
	// typical range is 1.0 (off) to 16.0 (max)
	float maxAnisotropy = 0.0f;
	// Level of Detail bias for mipmapped textures.
	// A positive bias -> selects lower-resolution mip levels sooner -> blurrier but faster.
	// A negative bias -> selects higher-resolution mip levels -> sharper but noisier/aliasing.
	float lodBias = 0.0f;
	bool useBorderColor = false;
	glm::vec4 borderColor{0.0f};
};

} // namespace video
