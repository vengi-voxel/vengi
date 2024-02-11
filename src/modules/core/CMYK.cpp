/**
 * @file
 */

#include "CMYK.h"
#include "core/Common.h"

namespace core {

core::RGBA CMYK::toRGB() const {
	const uint8_t r = (uint8_t)(255.0f * (1.0f - cmyk[0]) * (1.0f - cmyk[3]));
	const uint8_t g = (uint8_t)(255.0f * (1.0f - cmyk[1]) * (1.0f - cmyk[3]));
	const uint8_t b = (uint8_t)(255.0f * (1.0f - cmyk[2]) * (1.0f - cmyk[3]));
	return RGBA(r, g, b);
}

CMYK CMYK::fromRGB(const core::RGBA &rgb) {
	const float fr = (float)rgb.r / 255.0f;
	const float fg = (float)rgb.g / 255.0f;
	const float fb = (float)rgb.b / 255.0f;
	const float k = 1.0f - core_max(core_max(fr, fg), fb);
	const float c = (1.0f - fr - k) / (1.0f - k);
	const float m = (1.0f - fg - k) / (1.0f - k);
	const float y = (1.0f - fb - k) / (1.0f - k);
	return CMYK(c, m, y, k);
}

CMYK &CMYK::operator=(const CMYK &other) {
	if (this == &other) {
		return *this;
	}
	cmyk[0] = other.cmyk[0];
	cmyk[1] = other.cmyk[1];
	cmyk[2] = other.cmyk[2];
	cmyk[3] = other.cmyk[3];
	return *this;
}

} // namespace core
