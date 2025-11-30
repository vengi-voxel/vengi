/**
 * @file
 */

#pragma once

#include "color/RGBA.h"
#include "core/String.h"
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace color {

enum class Distance : uint8_t {
	// computational less expensive distance function
	Approximation,
	// hue, saturation, brightness distance function
	HSB,
	Max
};
float getDistance(RGBA rgba, RGBA rgba2, Distance d);
float getDistance(RGBA color, float hue, float saturation, float brightness);

// Helper function to convert sRGB component to linear space
double srgbToLinear(uint8_t c);

// Convert RGB to XYZ color space
void rgbToXyz(uint8_t r, uint8_t g, uint8_t b, double &X, double &Y, double &Z);

// Convert XYZ to LAB color space

void xyzToLab(double X, double Y, double Z, double &L, double &a, double &b);

// Compute Delta E (CIE76)
// <= 1.0  Imperceptible
// 1-2     Noticeable on close inspection
// 2-10    Perceptible at a glance
// 11-49   Distinct but related colors
// 50-100  Completely different colors
double deltaE76(double L1, double a1, double b1, double L2, double a2, double b2);
double deltaE76(RGBA c1, RGBA c2);

core::String print(RGBA rgba, bool colorAsHex = true);

color::RGBA flattenRGB(uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint8_t f);

glm::vec4 fromRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

inline glm::vec4 fromRGBA(const RGBA rgba) {
	return fromRGBA(rgba.r, rgba.g, rgba.b, rgba.a);
}
color::RGBA fromHSB(const float hue, const float saturation, const float brightness, const float alpha = 1.0f);
color::RGBA fromHex(const char *hex);
core::String toHex(const RGBA rgba, bool hashPrefix = true);

RGBA getRGBA(const glm::vec4 &);
RGBA getRGBA(const glm::vec3 &);
/**
 * @brief Calculate the Hue, Saturation, and Brightness (HSB) of the given color.
 */
void getHSB(const glm::vec4 &, float &hue, float &saturation, float &brightness);
void getHSB(color::RGBA color, float &hue, float &saturation, float &brightness);
/**
 * https://en.wikipedia.org/wiki/CIELAB_color_space
 *
 * @param[out] L lightness of the color (0 yields black and 100 indicates diffuse white; specular white may be
 * higher)
 * @param[out] a position between red and green (negative values indicate green and positive values indicate red)
 * @param[out] b position between yellow and blue (negative values indicate blue and positive values indicate
 * yellow)
 */
void getCIELab(const glm::vec4 &, float &L, float &a, float &b);
void getCIELab(color::RGBA color, float &L, float &a, float &b);
color::RGBA fromCIELab(const glm::vec4 &in);

glm::vec3 gray(const glm::vec3 &);
glm::vec4 gray(const glm::vec4 &);
glm::vec4 alpha(const glm::vec4 &, float alpha);
RGBA alpha(const RGBA rgba, uint8_t alpha);
float brightness(const glm::vec4 &);
uint8_t brightness(const color::RGBA &color);
float intensity(const glm::vec4 &);

const glm::vec4 &contrastTextColor(const glm::vec4 &background);
glm::vec4 darker(const glm::vec4 &color, float f = 1.0f);
glm::vec4 brighter(const glm::vec4 &color, float f = 1.0f);
color::RGBA brighter(const color::RGBA color, float f = 1.0f);
color::RGBA darker(const color::RGBA &color, float f = 1.0f);

} // namespace color
