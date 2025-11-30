/**
 * @file
 */

#pragma once

#include "RGBA.h"
#include "core/String.h"
#include <float.h>
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace color {

class Color {
public:
	static const uint32_t magnitude = 255;
	static const float magnitudef;
	static const float scaleFactor;
	static const glm::vec4 &Clear();
	static const glm::vec4 &White();
	static const glm::vec4 &Black();
	static const glm::vec4 &LightGray();
	static const glm::vec4 &Gray();
	static const glm::vec4 &DarkGray();
	static const glm::vec4 &LightRed();
	static const glm::vec4 &Red();
	static const glm::vec4 &DarkRed();
	static const glm::vec4 &LightGreen();
	static const glm::vec4 &Green();
	static const glm::vec4 &DarkGreen();
	static const glm::vec4 &Lime();
	static const glm::vec4 &LightBlue();
	static const glm::vec4 &Blue();
	static const glm::vec4 &DarkBlue();
	static const glm::vec4 &SteelBlue();
	static const glm::vec4 &Olive();
	static const glm::vec4 &Pink();
	static const glm::vec4 &Purple();
	static const glm::vec4 &Yellow();
	static const glm::vec4 &LightYellow();
	static const glm::vec4 &Sandy();
	static const glm::vec4 &Cyan();
	static const glm::vec4 &Orange();
	static const glm::vec4 &Brown();
	static const glm::vec4 &LightBrown();
	static const glm::vec4 &DarkBrown();

	enum class Distance {
		// computational less expensive distance function
		Approximation,
		// hue, saturation, brightness distance function
		HSB,
		Max
	};
	static float getDistance(RGBA rgba, RGBA rgba2, Distance d);
	static float getDistance(RGBA color, float hue, float saturation, float brightness);

	// Helper function to convert sRGB component to linear space
	static double srgbToLinear(uint8_t c);

	// Convert RGB to XYZ color space
	static void rgbToXyz(uint8_t r, uint8_t g, uint8_t b, double &X, double &Y, double &Z);

	// Convert XYZ to LAB color space

	static void xyzToLab(double X, double Y, double Z, double &L, double &a, double &b);

	// Compute Delta E (CIE76)
	// <= 1.0  Imperceptible
	// 1-2     Noticeable on close inspection
	// 2-10    Perceptible at a glance
	// 11-49   Distinct but related colors
	// 50-100  Completely different colors
	static double deltaE76(double L1, double a1, double b1, double L2, double a2, double b2);
	static double deltaE76(RGBA c1, RGBA c2);

	static core::String print(RGBA rgba, bool colorAsHex = true);

	static color::RGBA flattenRGB(uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint8_t f);

	enum class ColorReductionType {
		Octree,
		Wu,
		MedianCut,
		KMeans,
		NeuQuant,

		Max
	};
	static ColorReductionType toColorReductionType(const char *str);
	static const char *toColorReductionTypeString(Color::ColorReductionType type);

	/**
	 * @return @c -1 on error or the amount of @code colors <= maxTargetBufColors @endcode
	 */
	static int quantize(RGBA *targetBuf, size_t maxTargetBufColors, const RGBA *inputBuf, size_t inputBufColors,
						ColorReductionType type = ColorReductionType::MedianCut);

	static inline glm::vec4 fromRGBA(const RGBA rgba) {
		return fromRGBA(rgba.r, rgba.g, rgba.b, rgba.a);
	}
	static glm::vec4 fromRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
	static color::RGBA fromHSB(const float hue, const float saturation, const float brightness,
							  const float alpha = 1.0f);
	static color::RGBA fromHex(const char *hex);
	static core::String toHex(const RGBA rgba, bool hashPrefix = true);

	static RGBA getRGBA(const glm::vec4 &);
	static RGBA getRGBA(const glm::vec3 &);
	/**
	 * @brief Calculate the Hue, Saturation, and Brightness (HSB) of the given color.
	 */
	static void getHSB(const glm::vec4 &, float &hue, float &saturation, float &brightness);
	static void getHSB(color::RGBA color, float &hue, float &saturation, float &brightness);
	/**
	 * https://en.wikipedia.org/wiki/CIELAB_color_space
	 *
	 * @param[out] L lightness of the color (0 yields black and 100 indicates diffuse white; specular white may be
	 * higher)
	 * @param[out] a position between red and green (negative values indicate green and positive values indicate red)
	 * @param[out] b position between yellow and blue (negative values indicate blue and positive values indicate
	 * yellow)
	 */
	static void getCIELab(const glm::vec4 &, float &L, float &a, float &b);
	static void getCIELab(color::RGBA color, float &L, float &a, float &b);
	static color::RGBA fromCIELab(const glm::vec4 &in);

	static glm::vec3 gray(const glm::vec3 &);
	static glm::vec4 gray(const glm::vec4 &);
	static glm::vec4 alpha(const glm::vec4 &, float alpha);
	static RGBA alpha(const RGBA rgba, uint8_t alpha);
	static float brightness(const glm::vec4 &);
	static uint8_t brightness(const color::RGBA &color);
	static float intensity(const glm::vec4 &);

	static const glm::vec4 &contrastTextColor(const glm::vec4 &background);
	static glm::vec4 darker(const glm::vec4 &color, float f = 1.0f);
	static glm::vec4 brighter(const glm::vec4 &color, float f = 1.0f);
	static color::RGBA brighter(const color::RGBA color, float f = 1.0f);
	static color::RGBA darker(const color::RGBA &color, float f = 1.0f);
};

} // namespace color
