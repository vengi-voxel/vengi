/**
 * @file
 */

#include "TextureConfig.h"

namespace compute {

TextureConfig& TextureConfig::type(TextureType type) {
	_type = type;
	return *this;
}

TextureConfig& TextureConfig::dataformat(TextureDataFormat dataformat) {
	_dataformat = dataformat;
	return *this;
}

TextureConfig& TextureConfig::format(TextureFormat format) {
	_format = format;
	return *this;
}

TextureConfig& TextureConfig::wrap(TextureWrap wrap) {
	_wrap = wrap;
	return *this;
}

TextureConfig& TextureConfig::filter(TextureFilter filter) {
	_filter = filter;
	return *this;
}

TextureConfig& TextureConfig::normalizedCoordinates(bool normalized) {
	_normalizedCoordinates = normalized;
	return *this;
}

}
