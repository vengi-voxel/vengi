#include "Color.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace video {

const glm::vec4 Color::Clear        = glm::vec4(  0.f,   0,   0,   0) / glm::vec4(Color::magnitude);
const glm::vec4 Color::White        = glm::vec4(255.f, 255, 255, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::Black        = glm::vec4(  0.f,   0,   0, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::Lime         = glm::vec4(109.f, 198,   2, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::Pink         = glm::vec4(248.f,   4,  62, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::LightBlue    = glm::vec4(  0.f, 153, 203, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::DarkBlue     = glm::vec4( 55.f, 116, 145, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::Orange       = glm::vec4(252.f, 167,   0, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::Yellow       = glm::vec4(255.f, 255,   0, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::Sandy        = glm::vec4(237.f, 232, 160, 255) / glm::vec4(Color::magnitude);
// TODO: not yet nicelified
const glm::vec4 Color::LightGray    = glm::vec4(192.f, 192, 192, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::Gray         = glm::vec4(128.f, 128, 128, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::DarkGray     = glm::vec4( 84.f,  84,  84, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::LightRed     = glm::vec4(255.f,  96,  96, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::Red          = glm::vec4(255.f,   0,   0, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::DarkRed      = glm::vec4(128.f,   0,   0, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::LightGreen   = glm::vec4( 96.f, 255,  96, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::Green        = glm::vec4(  0.f, 255,   0, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::DarkGreen    = glm::vec4(  0.f, 128,   0, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::Blue         = glm::vec4(  0.f,   0, 255, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::SteelBlue    = glm::vec4( 35.f, 107, 142, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::Olive        = glm::vec4(128.f, 128,   0, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::Purple       = glm::vec4(128.f,   0, 128, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::Cyan         = glm::vec4(  0.f, 255, 255, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::Brown        = glm::vec4(107.f,  66,  38, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::LightBrown   = glm::vec4(150.f, 107,  72, 255) / glm::vec4(Color::magnitude);
const glm::vec4 Color::DarkBrown    = glm::vec4( 82.f,  43,  26, 255) / glm::vec4(Color::magnitude);

glm::vec4 Color::FromRGB(const unsigned int rgbInt, const float a) {
	return glm::vec4(static_cast<float>(rgbInt >> 16 & 0xFF) / Color::magnitude, static_cast<float>(rgbInt >> 8 & 0xFF) / Color::magnitude,
			static_cast<float>(rgbInt & 0xFF) / Color::magnitude, a);
}

glm::vec4 Color::FromRGBA(const unsigned int rgbaInt) {
	return glm::vec4(static_cast<float>(rgbaInt >> 24 & 0xFF) / Color::magnitude, static_cast<float>(rgbaInt >> 16 & 0xFF) / Color::magnitude,
			static_cast<float>(rgbaInt >> 8 & 0xFF) / Color::magnitude, static_cast<float>(rgbaInt & 0xFF) / Color::magnitude);
}

glm::vec4 Color::FromHSB(const float hue, const float saturation, const float brightness, const float alpha) {
	if (std::numeric_limits<float>::epsilon() > brightness) {
		return glm::vec4(0.f, 0.f, 0.f, alpha);
	}
	if (std::numeric_limits<float>::epsilon() > saturation) {
		return glm::vec4(brightness, brightness, brightness, alpha);
	}
	glm::vec4 color;
	color.a = alpha;
	float h = (hue - std::floor(hue)) * 6.f;
	float f = h - std::floor(h);
	float p = brightness * (1.f - saturation);
	float q = brightness * (1.f - saturation * f);
	float t = brightness * (1.f - (saturation * (1.f - f)));
	switch (static_cast<int>(h)) {
	case 0:
		color.r = brightness;
		color.g = t;
		color.b = p;
		break;
	case 1:
		color.r = q;
		color.g = brightness;
		color.b = p;
		break;
	case 2:
		color.r = p;
		color.g = brightness;
		color.b = t;
		break;
	case 3:
		color.r = p;
		color.g = q;
		color.b = brightness;
		break;
	case 4:
		color.r = t;
		color.g = p;
		color.b = brightness;
		break;
	case 5:
		color.r = brightness;
		color.g = p;
		color.b = q;
		break;
	}
	return color;
}

unsigned int Color::GetRGB(const glm::vec4& color) {
	return static_cast<int>(color.r * magnitude) << 16 | static_cast<int>(color.g * magnitude) << 8 | static_cast<int>(color.b * magnitude);
}

unsigned int Color::GetRGBA(const glm::vec4& color) {
	return static_cast<int>(color.r * magnitude) << 24 | static_cast<int>(color.g * magnitude) << 16 | static_cast<int>(color.b * magnitude) << 8
			| static_cast<int>(color.a * magnitude);
}

void Color::GetHSB(const glm::vec4& color, float& hue, float& saturation, float& brightness) {
	brightness = Brightness(color);
	float minBrightness = std::min(color.r, std::min(color.g, color.b));
	if (std::fabs(brightness - minBrightness) < std::numeric_limits<float>::epsilon()) {
		hue = 0.f;
		saturation = 0.f;
		return;
	}
	float r = (brightness - color.r) / (brightness - minBrightness);
	float g = (brightness - color.g) / (brightness - minBrightness);
	float b = (brightness - color.b) / (brightness - minBrightness);
	if (std::fabs(color.r - brightness) < std::numeric_limits<float>::epsilon()) {
		hue = b - g;
	} else if (std::fabs(color.g - brightness) < std::numeric_limits<float>::epsilon()) {
		hue = 2.f + r - b;
	} else {
		hue = 4.f + g - r;
	}
	hue /= 6.f;
	if (hue < 0.f)
		hue += 1.f;
	saturation = (brightness - minBrightness) / brightness;
}

float Color::Brightness(const glm::vec4& color) {
	return std::max(color.r, std::max(color.g, color.b));
}

float Color::Intensity(const glm::vec4& color) {
	return (color.r + color.g + color.b) / 3.f;
}

glm::vec4 Color::Darker(const glm::vec4& color, float f) {
	f = std::pow(scaleFactor, f);
	return glm::vec4(glm::clamp(glm::vec3(color) * f, 0.0f, 1.0f), color.a);
}

glm::vec4 Color::Brighter(const glm::vec4& color, float f) {
	static float min = 21.f / magnitude;
	glm::vec3 result = glm::vec3(color);
	f = std::pow(scaleFactor, f);
	if (glm::all(glm::equal(glm::vec3(), result)))
		return glm::vec4(min / f, min / f, min / f, color.a);
	if (result.r > 0.f && result.r < min)
		result.r = min;
	if (result.g > 0.f && result.g < min)
		result.g = min;
	if (result.b > 0.f && result.b < min)
		result.b = min;
	return glm::vec4(glm::clamp(result / f, 0.f, 1.f), color.a);
}

}
