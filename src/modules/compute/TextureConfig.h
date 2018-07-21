/**
 * @file
 */

#pragma once

#include "Types.h"

namespace compute {

/**
 * @brief Configuration options or a texture buffer
 * @ingroup Compute
 */
class TextureConfig {
private:
	TextureType _type = TextureType::Texture2D;
	TextureFormat _format = TextureFormat::RGBA;
	TextureWrap _wrap = TextureWrap::None;
	TextureFilter _filter = TextureFilter::Linear;
	TextureDataFormat _dataformat = TextureDataFormat::UNSIGNED_INT8;
	bool _normalizedCoordinates = true;
public:
	TextureConfig& type(TextureType type);
	TextureConfig& format(TextureFormat format);
	TextureConfig& dataformat(TextureDataFormat format);
	TextureConfig& wrap(TextureWrap wrap);
	TextureConfig& filter(TextureFilter filter);
	TextureConfig& normalizedCoordinates(bool normalized);

	TextureType type() const;
	TextureFormat format() const;
	TextureDataFormat dataformat() const;
	TextureWrap wrap() const;
	TextureFilter filter() const;
	bool normalizedCoordinates() const;
};

inline TextureType TextureConfig::type() const {
	return _type;
}

inline TextureFormat TextureConfig::format() const {
	return _format;
}

inline TextureDataFormat TextureConfig::dataformat() const {
	return _dataformat;
}

inline TextureWrap TextureConfig::wrap() const {
	return _wrap;
}

inline TextureFilter TextureConfig::filter() const {
	return _filter;
}

inline bool TextureConfig::normalizedCoordinates() const {
	return _normalizedCoordinates;
}

}
