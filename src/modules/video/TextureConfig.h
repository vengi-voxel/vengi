/**
 * @file
 */

#pragma once

#include "Types.h"

namespace video {

class TextureConfig {
private:
	TextureWrap _wrapS = TextureWrap::Max;
	TextureWrap _wrapT = TextureWrap::Max;
	TextureFilter _filterMag = TextureFilter::Linear;
	TextureFilter _filterMin = TextureFilter::Linear;
	TextureType _type = TextureType::Texture2D;
	TextureFormat _format = TextureFormat::RGBA;
	CompareFunc _compareFunc = CompareFunc::Max;
	TextureCompareMode _compareMode = TextureCompareMode::Max;
public:
	TextureConfig& wrap(TextureWrap wrap);
	TextureConfig& wrapS(TextureWrap wrap);
	TextureConfig& wrapT(TextureWrap wrap);
	TextureConfig& filter(TextureFilter filter);
	TextureConfig& filterMag(TextureFilter filter);
	TextureConfig& filterMin(TextureFilter filter);
	TextureConfig& type(TextureType type);
	TextureConfig& format(TextureFormat format);
	TextureConfig& compareFunc(CompareFunc func);
	TextureConfig& compareMode(TextureCompareMode mode);

	TextureWrap wrapS() const;
	TextureWrap wrapT() const;
	TextureFilter filterMag() const;
	TextureFilter filterMin() const;
	TextureType type() const;
	TextureFormat format() const;
	CompareFunc compareFunc() const;
	TextureCompareMode compareMode() const;
};

inline TextureFilter TextureConfig::filterMag() const{
	return _filterMag;
}

inline TextureFilter TextureConfig::filterMin() const{
	return _filterMin;
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

}
