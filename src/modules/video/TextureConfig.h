/**
 * @file
 */

#pragma once

#include "Types.h"
#include <glm/vec4.hpp>

namespace video {

/**
 * @brief Configuration options or a @c Texture
 * @ingroup Video
 */
class TextureConfig {
private:
	TextureWrap _wrapR = TextureWrap::Max;
	TextureWrap _wrapS = TextureWrap::Max;
	TextureWrap _wrapT = TextureWrap::Max;
	TextureFilter _filterMag = TextureFilter::Linear;
	TextureFilter _filterMin = TextureFilter::Linear;
	TextureType _type = TextureType::Texture2D;
	TextureFormat _format = TextureFormat::RGBA;
	CompareFunc _compareFunc = CompareFunc::Max;
	TextureCompareMode _compareMode = TextureCompareMode::Max;
	uint8_t _layers = 1u;
	uint8_t _alignment = 1u;
	// Reduces blur and shimmering at oblique viewing angles.
	// typical range is 1.0 (off) to 16.0 (max)
	float _maxAnisotropy = 0.0f;
	// Level of Detail bias for mipmapped textures.
	// A positive bias -> selects lower-resolution mip levels sooner -> blurrier but faster.
	// A negative bias -> selects higher-resolution mip levels -> sharper but noisier/aliasing.
	float _lodBias = 0.0f;
	bool _useBorderColor = false;
	glm::vec4 _borderColor{0.0f};
	int _samples = 0;

public:
	TextureConfig &wrap(TextureWrap wrap);
	TextureConfig &wrapR(TextureWrap wrap);
	TextureConfig &wrapS(TextureWrap wrap);
	TextureConfig &wrapT(TextureWrap wrap);
	TextureConfig &filter(TextureFilter filter);
	TextureConfig &filterMag(TextureFilter filter);
	TextureConfig &filterMin(TextureFilter filter);
	TextureConfig &type(TextureType type);
	TextureConfig &format(TextureFormat format);
	TextureConfig &compareFunc(CompareFunc func);
	TextureConfig &compareMode(TextureCompareMode mode);
	TextureConfig &borderColor(const glm::vec4 &borderColor);
	TextureConfig &samples(int samples);
	TextureConfig &maxAnisotropy(float aniso);
	TextureConfig &lodBias(float bias);
	/**
	 * @param[in] layers The amount of layers for the given texture.
	 * @see TextureType
	 */
	TextureConfig &layers(uint8_t layers);
	/**
	 * @param[in] alignment A value of 0 doesn't change the default.
	 * Valid values are @c 0, @c 1, @c 2, @c 4 and @c 8.
	 */
	TextureConfig &alignment(uint8_t alignment);

	TextureWrap wrapR() const;
	TextureWrap wrapS() const;
	TextureWrap wrapT() const;
	TextureFilter filterMag() const;
	TextureFilter filterMin() const;
	TextureType type() const;
	TextureFormat format() const;
	int samples() const;
	float maxAnisotropy() const;
	float lodBias() const;
	uint8_t layers() const;
	uint8_t alignment() const;
	CompareFunc compareFunc() const;
	TextureCompareMode compareMode() const;
	bool useBorderColor() const;
	const glm::vec4 &borderColor() const;
};

inline int TextureConfig::samples() const {
	return _samples;
}

inline float TextureConfig::maxAnisotropy() const {
	return _maxAnisotropy;
}

inline float TextureConfig::lodBias() const {
	return _lodBias;
}

inline TextureFilter TextureConfig::filterMag() const {
	return _filterMag;
}

inline TextureFilter TextureConfig::filterMin() const {
	return _filterMin;
}

inline TextureWrap TextureConfig::wrapR() const {
	return _wrapR;
}

inline TextureWrap TextureConfig::wrapS() const {
	return _wrapS;
}

inline TextureWrap TextureConfig::wrapT() const {
	return _wrapT;
}

inline TextureType TextureConfig::type() const {
	return _type;
}

inline TextureFormat TextureConfig::format() const {
	return _format;
}

inline CompareFunc TextureConfig::compareFunc() const {
	return _compareFunc;
}

inline TextureCompareMode TextureConfig::compareMode() const {
	return _compareMode;
}

inline uint8_t TextureConfig::layers() const {
	return _layers;
}

inline uint8_t TextureConfig::alignment() const {
	return _alignment;
}

inline bool TextureConfig::useBorderColor() const {
	return _useBorderColor;
}

inline const glm::vec4 &TextureConfig::borderColor() const {
	return _borderColor;
}

video::TextureConfig createDefaultTextureConfig();
video::TextureConfig createDefaultMultiSampleTextureConfig();

} // namespace video
