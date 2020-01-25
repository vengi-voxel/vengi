/**
 * @file
 */

#include "TextureConfig.h"

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
}
