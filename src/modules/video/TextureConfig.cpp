/**
 * @file
 */

#include "TextureConfig.h"
#include "core/Log.h"
#include "video/Renderer.h"
#include <glm/common.hpp>

namespace video {

TextureConfig& TextureConfig::wrap(TextureWrap wrap) {
	_wrapR = _wrapS = _wrapT = wrap;
	return *this;
}

TextureConfig& TextureConfig::wrapR(TextureWrap wrap) {
	_wrapR = wrap;
	return *this;
}

TextureConfig& TextureConfig::wrapS(TextureWrap wrap) {
	_wrapS = wrap;
	return *this;
}

TextureConfig& TextureConfig::wrapT(TextureWrap wrap) {
	_wrapT = wrap;
	return *this;
}

TextureConfig& TextureConfig::filter(TextureFilter filter) {
	_filterMag = _filterMin = filter;
	return *this;
}

TextureConfig& TextureConfig::filterMag(TextureFilter filter) {
	_filterMag = filter;
	return *this;
}

TextureConfig& TextureConfig::filterMin(TextureFilter filter) {
	_filterMin = filter;
	return *this;
}

TextureConfig& TextureConfig::maxAnisotropy(float aniso) {
	const float maxAnisotropy = video::limit(video::Limit::MaxAnisotropy);
	if (aniso <= 1.0f) {
		aniso = maxAnisotropy;
	}
	// Anisotropy values < 1.0 are not meaningful. Enforce a sensible minimum here.
	_maxAnisotropy = glm::clamp(aniso, 1.0f, maxAnisotropy);
	return *this;
}

TextureConfig& TextureConfig::type(TextureType type) {
	_type = type;
	return *this;
}

TextureConfig& TextureConfig::format(TextureFormat format) {
	_format = format;
	return *this;
}

TextureConfig& TextureConfig::compareFunc(CompareFunc func) {
	_compareFunc = func;
	return *this;
}

TextureConfig& TextureConfig::compareMode(TextureCompareMode mode) {
	_compareMode = mode;
	return *this;
}

TextureConfig& TextureConfig::layers(uint8_t layers) {
	_layers = layers;
	return *this;
}

TextureConfig& TextureConfig::alignment(uint8_t alignment) {
	_alignment = alignment;
	return *this;
}

TextureConfig& TextureConfig::borderColor(const glm::vec4& borderColor) {
	_useBorderColor = true;
	_borderColor = borderColor;
	return *this;
}

TextureConfig& TextureConfig::samples(int samples) {
	_samples = glm::clamp(samples, 0, video::limiti(Limit::MaxSamples));
	if (samples != _samples) {
		Log::warn("Could get get the requested number of samples - using %i instead of %i", _samples, samples);
	}
	return *this;
}

TextureConfig& TextureConfig::lodBias(float bias) {
    _lodBias = bias;
    return *this;
}

video::TextureConfig createDefaultTextureConfig() {
	video::TextureConfig cfg;
	cfg.wrap(video::TextureWrap::ClampToEdge);
	cfg.filter(video::TextureFilter::Linear);
	cfg.type(video::TextureType::Texture2D);
	cfg.format(video::TextureFormat::RGBA);
	return cfg;
}

video::TextureConfig createDefaultMultiSampleTextureConfig() {
	video::TextureConfig cfg;
	cfg.wrap(video::TextureWrap::ClampToEdge);
	cfg.filter(video::TextureFilter::Linear);
	cfg.samples(4);
	cfg.type(video::TextureType::Texture2DMultisample);
	cfg.format(video::TextureFormat::RGBA);
	return cfg;
}

}
