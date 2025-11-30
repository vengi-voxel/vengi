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

/**
 * @brief Calculates the distance between two colors based on the given Distance function.
 */
float getDistance(RGBA rgba, RGBA rgba2, Distance d);
/**
 * @brief Calculates the distance between a color and HSB values.
 */
float getDistance(RGBA color, float hue, float saturation, float brightness);

/**
 * @brief Converts an sRGB component to linear space.
 */
double srgbToLinear(uint8_t c);

/**
 * @brief Converts RGB to XYZ color space.
 */
void rgbToXyz(uint8_t r, uint8_t g, uint8_t b, double &X, double &Y, double &Z);

/**
 * @brief Converts XYZ to LAB color space.
 */
void xyzToLab(double X, double Y, double Z, double &L, double &a, double &b);

/**
 * @brief Compute Delta E (CIE76)
 *
 * <= 1.0  Imperceptible
 * 1-2     Noticeable on close inspection
 * 2-10    Perceptible at a glance
 * 11-49   Distinct but related colors
 * 50-100  Completely different colors
 */
double deltaE76(double L1, double a1, double b1, double L2, double a2, double b2);
/**
 * @brief Compute Delta E (CIE76) between two colors.
 */
double deltaE76(RGBA c1, RGBA c2);

/**
 * @brief Returns a string representation of the color with ansi colors to be printed to terminals.
 */
core::String print(RGBA rgba, bool colorAsHex = true);

/**
 * @brief Flattens RGB values with a factor.
 */
color::RGBA flattenRGB(uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint8_t f);

/**
 * @brief Converts RGBA components to a vec4.
 */
glm::vec4 fromRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

/**
 * @brief Converts RGBA struct to a vec4.
 */
inline glm::vec4 fromRGBA(const RGBA rgba) {
	return fromRGBA(rgba.r, rgba.g, rgba.b, rgba.a);
}

/**
 * @brief Creates a color from HSB values.
 */
color::RGBA fromHSB(const float hue, const float saturation, const float brightness, const float alpha = 1.0f);
/**
 * @brief Creates a color from a hex string.
 */
color::RGBA fromHex(const char *hex);
/**
 * @brief Converts a color to a hex string.
 */
core::String toHex(const RGBA rgba, bool hashPrefix = true);

/**
 * @brief Converts vec4 to RGBA.
 */
RGBA toRGBA(const glm::vec4 &);
/**
 * @brief Converts vec3 to RGBA.
 */
RGBA getRGBA(const glm::vec3 &);
/**
 * @brief Calculate the Hue, Saturation, and Brightness (HSB) of the given color.
 */
void getHSB(const glm::vec4 &, float &hue, float &saturation, float &brightness);
/**
 * @brief Calculate the Hue, Saturation, and Brightness (HSB) of the given color.
 */
void getHSB(color::RGBA color, float &hue, float &saturation, float &brightness);
/**
 * @brief Converts color to CIELab color space.
 * https://en.wikipedia.org/wiki/CIELAB_color_space
 *
 * @param[out] L lightness of the color (0 yields black and 100 indicates diffuse white; specular white may be
 * higher)
 * @param[out] a position between red and green (negative values indicate green and positive values indicate red)
 * @param[out] b position between yellow and blue (negative values indicate blue and positive values indicate
 * yellow)
 */
void getCIELab(const glm::vec4 &, float &L, float &a, float &b);
/**
 * @brief Converts color to CIELab color space.
 */
void getCIELab(color::RGBA color, float &L, float &a, float &b);
/**
 * @brief Converts CIELab to RGBA.
 */
color::RGBA fromCIELab(const glm::vec4 &in);

/**
 * @brief Converts color to grayscale.
 */
glm::vec3 gray(const glm::vec3 &);
/**
 * @brief Converts color to grayscale.
 */
glm::vec4 gray(const glm::vec4 &);
/**
 * @brief Sets the alpha component of a color.
 */
glm::vec4 alpha(const glm::vec4 &, float alpha);
/**
 * @brief Sets the alpha component of a color.
 */
RGBA alpha(const RGBA rgba, uint8_t alpha);
/**
 * @brief Calculates the brightness of a color.
 */
float brightness(const glm::vec4 &);
/**
 * @brief Calculates the brightness of a color.
 */
uint8_t brightness(const color::RGBA &color);
/**
 * @brief Calculates the intensity of a color.
 */
float intensity(const glm::vec4 &);

/**
 * @brief Returns a contrasting text color (black or white) for the given background.
 */
const glm::vec4 &contrastTextColor(const glm::vec4 &background);
/**
 * @brief Returns a darker version of the color.
 */
glm::vec4 darker(const glm::vec4 &color, float f = 1.0f);
/**
 * @brief Returns a brighter version of the color.
 */
glm::vec4 brighter(const glm::vec4 &color, float f = 1.0f);
/**
 * @brief Returns a brighter version of the color.
 */
color::RGBA brighter(const color::RGBA color, float f = 1.0f);
/**
 * @brief Returns a darker version of the color.
 */
color::RGBA darker(const color::RGBA &color, float f = 1.0f);

} // namespace color
