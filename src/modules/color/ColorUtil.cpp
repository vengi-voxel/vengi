/**
 * @file
 */

#include "ColorUtil.h"
#include "Color.h"
#include "core/Common.h"
#include "core/GLM.h"
#include <glm/ext/scalar_integer.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/epsilon.hpp>

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/type_aligned.hpp>

#include "core/sdl/SDLSystem.h"
#include <stdio.h>

namespace color {

glm::vec4 fromRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
	return glm::aligned_vec4(r, g, b, a) / color::magnitudef;
}

RGBA fromHSB(const float hue, const float saturation, const float brightness, const float alpha) {
	if (0.00001f > brightness) {
		return RGBA(0, 0, 0, alpha * 255.0f);
	}
	if (0.00001f > saturation) {
		return RGBA(brightness * 255.0f, brightness * 255.0f, brightness * 255.0f, alpha * 255.0f);
	}
	const float h = (hue - SDL_floorf(hue)) * 6.0f;
	const float f = h - SDL_floorf(h);
	const uint8_t p = (uint8_t)(brightness * (1.f - saturation) * 255.0f);
	const uint8_t q = (uint8_t)(brightness * (1.f - saturation * f) * 255.0f);
	const uint8_t t = (uint8_t)(brightness * (1.f - (saturation * (1.f - f))) * 255.0f);
	RGBA color;
	color.a = alpha * 255.0f;
	switch (static_cast<int>(h)) {
	case 0:
		color.r = brightness * 255.0f;
		color.g = t;
		color.b = p;
		break;
	case 1:
		color.r = q;
		color.g = brightness * 255.0f;
		color.b = p;
		break;
	case 2:
		color.r = p;
		color.g = brightness * 255.0f;
		color.b = t;
		break;
	case 3:
		color.r = p;
		color.g = q;
		color.b = brightness * 255.0f;
		break;
	case 4:
		color.r = t;
		color.g = p;
		color.b = brightness * 255.0f;
		break;
	case 5:
		color.r = brightness * 255.0f;
		color.g = p;
		color.b = q;
		break;
	}
	return color;
}

core::String toHex(const RGBA rgba, bool hashPrefix) {
	core::String hex;
	if (hashPrefix) {
		hex.append("#");
	}
	hex.append(core::String::format("%02x%02x%02x%02x", rgba.r, rgba.g, rgba.b, rgba.a));
	return hex;
}

RGBA fromHex(const char *hex) {
	uint32_t r = 0x00;
	uint32_t g = 0x00;
	uint32_t b = 0x00;
	uint32_t a = 0xff;
	if (0 == SDL_strncmp(hex, "0x", 2) || 0 == SDL_strncmp(hex, "0X", 2)) {
		hex += 2;
	} else if (hex[0] == '#') {
		hex += 1;
	}
	if (sscanf(hex, "%02x%02x%02x%02x", &r, &g, &b, &a) == 3) {
		a = 0xff;
	}
	return RGBA(r, g, b, a);
}

core::String print(RGBA rgba, bool colorAsHex) {
	core::String buf("\033[0m", 4);
	if (colorAsHex) {
		buf = toHex(rgba);
		buf.append(" ");
	}
	if (rgba.a != 0) {
		buf.append("\033[38;2;");
		buf.append(rgba.r).append(";");
		buf.append(rgba.g).append(";");
		buf.append(rgba.b).append("m");
	}
	buf.append("\033[48;2;");
	buf.append(rgba.r).append(";");
	buf.append(rgba.g).append(";");
	buf.append(rgba.b).append("m");
	buf.append(u8"\u2587");
	buf.append("\033[0m");
	return buf;
}

// https://www.compuphase.com/cmetric.htm
static float getDistanceApprox(RGBA rgba, RGBA rgba2) {
	const int rmean = (rgba2.r + rgba.r) / 2;
	const int r = rgba2.r - rgba.r;
	const int g = rgba2.g - rgba.g;
	const int b = rgba2.b - rgba.b;
	return (float)(((512 + rmean) * r * r) >> 8) + 4.0f * g * g + (float)(((767 - rmean) * b * b) >> 8);
}

static float getDistanceHSB(const RGBA &rgba, float hue, float saturation, float brightness) {
	float chue;
	float csaturation;
	float cbrightness;
	getHSB(rgba, chue, csaturation, cbrightness);

	const float weightHue = 0.8f;
	const float weightSaturation = 0.1f;
	const float weightValue = 0.1f;
	const float dH = chue - hue;
	const float dS = csaturation - saturation;
	const float dV = cbrightness - brightness;
	return weightHue * dH * dH + weightValue * dV * dV + weightSaturation * dS * dS;
}

static float getDistanceHSB(const RGBA &rgba, RGBA rgba2) {
	float hue;
	float saturation;
	float brightness;
	getHSB(fromRGBA(rgba), hue, saturation, brightness);
	return getDistanceHSB(rgba2, hue, saturation, brightness);
}

double srgbToLinear(uint8_t c) {
	double v = c / 255.0;
	return (v <= 0.04045) ? (v / 12.92) : pow((v + 0.055) / 1.055, 2.4);
}

void rgbToXyz(uint8_t r, uint8_t g, uint8_t b, double &X, double &Y, double &Z) {
	double R = srgbToLinear(r);
	double G = srgbToLinear(g);
	double B = srgbToLinear(b);

	// sRGB to XYZ conversion matrix (D65 white point)
	X = R * 0.4124564 + G * 0.3575761 + B * 0.1804375;
	Y = R * 0.2126729 + G * 0.7151522 + B * 0.0721750;
	Z = R * 0.0193339 + G * 0.1191920 + B * 0.9503041;
}

void xyzToLab(double X, double Y, double Z, double &L, double &a, double &b) {
	// D65 reference white point
	double Xr = 0.95047, Yr = 1.00000, Zr = 1.08883;

	auto f = [](double t) { return (t > 0.008856) ? pow(t, 1.0 / 3.0) : (7.787 * t + 16.0 / 116.0); };

	double fx = f(X / Xr);
	double fy = f(Y / Yr);
	double fz = f(Z / Zr);

	L = (116.0 * fy) - 16.0;
	a = 500.0 * (fx - fy);
	b = 200.0 * (fy - fz);
}

double deltaE76(double L1, double a1, double b1, double L2, double a2, double b2) {
	return sqrt(pow(L2 - L1, 2) + pow(a2 - a1, 2) + pow(b2 - b1, 2));
}

double deltaE76(RGBA c1, RGBA c2) {
	double X1, Y1, Z1, X2, Y2, Z2;
	rgbToXyz(c1.r, c1.g, c1.b, X1, Y1, Z1);
	rgbToXyz(c2.r, c2.g, c2.b, X2, Y2, Z2);

	double L1, a1, b1, L2, a2, b2;
	xyzToLab(X1, Y1, Z1, L1, a1, b1);
	xyzToLab(X2, Y2, Z2, L2, a2, b2);

	return deltaE76(L1, a1, b1, L2, a2, b2);
}

float getDistance(RGBA rgba, RGBA rgba2, Distance d) {
	if (rgba == rgba2) {
		return 0.0f;
	}
	if (d == Distance::Approximation) {
		return getDistanceApprox(rgba, rgba2);
	}
	return getDistanceHSB(rgba, rgba2);
}

float getDistance(RGBA rgba, float hue, float saturation, float brightness) {
	return getDistanceHSB(rgba, hue, saturation, brightness);
}

RGBA flattenRGB(uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint8_t f) {
	if (f <= 1u) {
		return RGBA(r, g, b, a);
	}
	return RGBA(r / f * f, g / f * f, b / f * f, a);
}

void getCIELab(RGBA color, float &L, float &a, float &b) {
	getCIELab(fromRGBA(color), L, a, b);
}

RGBA fromCIELab(const glm::vec4 &in) {
	float fy = (in.r + 16.0f) / 116.0f;
	float fx = in.g / 500.0f + fy;
	float fz = fy - in.b / 200.0f;

	float x, y, z;
	const float delta = 6.0f / 29.0f;

	if (fx > delta) {
		x = fx * fx * fx;
	} else {
		x = (fx - 16.0f / 116.0f) / 7.787f;
	}

	if (fy > delta) {
		y = fy * fy * fy;
	} else {
		y = (fy - 16.0f / 116.0f) / 7.787f;
	}

	if (fz > delta) {
		z = fz * fz * fz;
	} else {
		z = (fz - 16.0f / 116.0f) / 7.787f;
	}

	x *= 95.047f / 100.0f;
	y *= 100.000f / 100.0f;
	z *= 108.883f / 100.0f;

	float r = x * 3.2406f + y * -1.5372f + z * -0.4986f;
	float g = x * -0.9689f + y * 1.8758f + z * 0.0415f;
	float b = x * 0.0557f + y * -0.2040f + z * 1.0570f;

	if (r > 0.0031308f) {
		r = 1.055f * glm::pow(r, 1.0f / 2.4f) - 0.055f;
	} else {
		r = 12.92f * r;
	}

	if (g > 0.0031308f) {
		g = 1.055f * glm::pow(g, 1.0f / 2.4f) - 0.055f;
	} else {
		g = 12.92f * g;
	}

	if (b > 0.0031308f) {
		b = 1.055f * glm::pow(b, 1.0f / 2.4f) - 0.055f;
	} else {
		b = 12.92f * b;
	}

	return RGBA((uint8_t)glm::clamp(r * color::magnitude, 0.0f, 255.0f),
				(uint8_t)glm::clamp(g * color::magnitude, 0.0f, 255.0f),
				(uint8_t)glm::clamp(b * color::magnitude, 0.0f, 255.0f),
				255u);
}

void getCIELab(const glm::vec4 &color, float &L, float &a, float &b) {
	float red, green, blue;
	if (color.r > 0.04045f) {
		red = glm::pow(((color.r + 0.055f) / 1.055f), 2.4f);
	} else {
		red = color.r / 12.92f;
	}

	if (color.g > 0.04045f) {
		green = glm::pow(((color.g + 0.055f) / 1.055f), 2.4f);
	} else {
		green = color.g / 12.92f;
	}

	if (color.b > 0.04045f) {
		blue = glm::pow(((color.b + 0.055f) / 1.055f), 2.4f);
	} else {
		blue = color.b / 12.92f;
	}

	red = red * 100.0f;
	green = green * 100.0f;
	blue = blue * 100.0f;

	// XYZ color space
	const float x = red * 0.4124f + green * 0.3576f + blue * 0.1805f;
	const float y = red * 0.2126f + green * 0.7152f + blue * 0.0722f;
	const float z = red * 0.0193f + green * 0.1192f + blue * 0.9505f;

	// standard illuminant D65
	float fx = x / 95.047f;
	float fy = y / 100.0f;
	float fz = z / 108.883f;

	if (fx > 0.008856f) {
		fx = glm::pow(fx, 1.0f / 3.0f);
	} else {
		fx = (7.787f * fx) + (4.0f / 29.0f);
	}

	if (fy > 0.008856f) {
		fy = glm::pow(fy, 1.0f / 3.0f);
	} else {
		fy = (7.787f * fy) + (4.0f / 29.0f);
	}

	if (fz > 0.008856f) {
		fz = glm::pow(fz, 1.0f / 3.0f);
	} else {
		fz = (7.787f * fz) + (4.0f / 29.0f);
	}

	L = 116.0f * fy - 16.0f;
	a = 500.0f * (fx - fy);
	b = 200.0f * (fy - fz);
}

RGBA toRGBA(const glm::vec4 &color) {
	return RGBA{(uint8_t)(color.r * color::magnitude), (uint8_t)(color.g * color::magnitude),
				(uint8_t)(color.b * color::magnitude), (uint8_t)(color.a * color::magnitude)};
}

RGBA getRGBA(const glm::vec3 &color) {
	return RGBA{(uint8_t)(color.r * color::magnitude), (uint8_t)(color.g * color::magnitude),
				(uint8_t)(color.b * color::magnitude), 255u};
}

void getHSB(RGBA color, float &chue, float &csaturation, float &cbrightness) {
	getHSB(fromRGBA(color), chue, csaturation, cbrightness);
}

void getHSB(const glm::vec4 &color, float &chue, float &csaturation, float &cbrightness) {
	cbrightness = brightness(color);
	const float minBrightness = core_min(color.r, core_min(color.g, color.b));
	const float delta = cbrightness - minBrightness;
	if (SDL_fabs(delta) < 0.00001f) {
		chue = 0.f;
		csaturation = 0.f;
		return;
	}
	const float r = (cbrightness - color.r) / delta;
	const float g = (cbrightness - color.g) / delta;
	const float b = (cbrightness - color.b) / delta;
	if (SDL_fabs(color.r - cbrightness) < 0.00001f) {
		chue = b - g;
	} else if (SDL_fabs(color.g - cbrightness) < 0.00001f) {
		chue = 2.f + r - b;
	} else {
		chue = 4.f + g - r;
	}
	chue /= 6.f;
	if (chue < 0.f) {
		chue += 1.f;
	}
	csaturation = delta / cbrightness;
}

glm::vec4 alpha(const glm::vec4 &c, float alpha) {
	return glm::vec4(c.r, c.g, c.b, alpha);
}

RGBA alpha(const RGBA rgba, uint8_t alpha) {
	return RGBA(rgba.r, rgba.g, rgba.b, alpha);
}

float brightness(const glm::vec4 &color) {
	return core_max(color.r, core_max(color.g, color.b));
}

uint8_t brightness(const RGBA &color) {
	return core_max(color.r, core_max(color.g, color.b));
}

float intensity(const glm::vec4 &color) {
	return (color.r + color.g + color.b) / 3.f;
}

glm::vec4 gray(const glm::vec4 &color) {
	const float gray = (0.21f * color.r + 0.72f * color.g + 0.07f * color.b);
	return glm::vec4(gray, gray, gray, color.a);
}

glm::vec3 gray(const glm::vec3 &color) {
	const float gray = (0.21f * color.r + 0.72f * color.g + 0.07f * color.b);
	return glm::vec3(gray, gray, gray);
}

RGBA darker(const RGBA &color, float f) {
	f = (float)SDL_pow(color::scaleFactor, f);
	RGBA result = color;
	result.r = (uint8_t)((float)result.r * f);
	result.g = (uint8_t)((float)result.g * f);
	result.b = (uint8_t)((float)result.b * f);
	return result;
}

const glm::vec4 &contrastTextColor(const glm::vec4 &background) {
	// Compute luminance using the Rec. 709 formula
	float luminance = 0.2126f * background.r + 0.7152f * background.g + 0.0722f * background.b;

	// Use white text on dark backgrounds, black text on light backgrounds
	return (luminance < 0.5f) ? color::White() : color::Black();
}

glm::vec4 darker(const glm::vec4 &color, float f) {
	f = (float)SDL_pow(color::scaleFactor, f);
	return glm::vec4(glm::clamp(glm::vec3(color) * f, 0.0f, 1.0f), color.a);
}

RGBA brighter(const RGBA color, float f) {
	return toRGBA(brighter(fromRGBA(color), f));
}

glm::vec4 brighter(const glm::vec4 &color, float f) {
	static float min = 21.f / color::magnitude;
	glm::vec3 result = glm::vec3(color);
	f = (float)SDL_pow(color::scaleFactor, f);
	if (glm::all(glm::epsilonEqual(glm::zero<glm::vec3>(), result, 0.00001f))) {
		return glm::vec4(min / f, min / f, min / f, color.a);
	}
	if (result.r > 0.f && result.r < min) {
		result.r = min;
	}
	if (result.g > 0.f && result.g < min) {
		result.g = min;
	}
	if (result.b > 0.f && result.b < min) {
		result.b = min;
	}
	return glm::vec4(glm::clamp(result / f, 0.f, 1.f), color.a);
}

} // namespace color
