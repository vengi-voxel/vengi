/**
 * @file
 */

#pragma once

#include "core/CMYK.h"
#include "core/String.h"
#include "core/RGBA.h"
#include <glm/fwd.hpp>
#include <glm/vec4.hpp>
#include <float.h>

namespace core {

class Color {
public:
	static const uint32_t magnitude = 255;
	static const float magnitudef;
	static const float scaleFactor;
	static const glm::vec4& Clear();
	static const glm::vec4& White();
	static const glm::vec4& Black();
	static const glm::vec4& LightGray();
	static const glm::vec4& Gray();
	static const glm::vec4& DarkGray();
	static const glm::vec4& LightRed();
	static const glm::vec4& Red();
	static const glm::vec4& DarkRed();
	static const glm::vec4& LightGreen();
	static const glm::vec4& Green();
	static const glm::vec4& DarkGreen();
	static const glm::vec4& Lime();
	static const glm::vec4& LightBlue();
	static const glm::vec4& Blue();
	static const glm::vec4& DarkBlue();
	static const glm::vec4& SteelBlue();
	static const glm::vec4& Olive();
	static const glm::vec4& Pink();
	static const glm::vec4& Purple();
	static const glm::vec4& Yellow();
	static const glm::vec4& Sandy();
	static const glm::vec4& Cyan();
	static const glm::vec4& Orange();
	static const glm::vec4& Brown();
	static const glm::vec4& LightBrown();
	static const glm::vec4& DarkBrown();

	enum class Distance { Approximation, HSB, Max };
	static float getDistance(RGBA rgba, RGBA rgba2, Distance d);
	static float getDistance(RGBA color, float hue, float saturation, float brightness);

	static core::String print(RGBA rgba, bool colorAsHex = true);

	static core::RGBA flattenRGB(uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint8_t f);

	enum class ColorReductionType {
		Octree,
		Wu,
		MedianCut,
		KMeans,
		NeuQuant,

		Max
	};
	static ColorReductionType toColorReductionType(const char *str);
	static const char* toColorReductionTypeString(Color::ColorReductionType type);

	/**
	 * @return @c -1 on error or the amount of @code colors <= maxTargetBufColors @endcode
	 */
	static int quantize(RGBA* targetBuf, size_t maxTargetBufColors, const RGBA* inputBuf, size_t inputBufColors, ColorReductionType type = ColorReductionType::MedianCut);

	static inline glm::vec4 fromRGBA(const RGBA rgba) {
		return fromRGBA(rgba.r, rgba.g, rgba.b, rgba.a);
	}
	static glm::vec4 fromRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
	static glm::vec4 fromHSB(const float hue, const float saturation, const float brightness, const float alpha = 1.0f);
	static glm::vec4 fromHex(const char* hex);
	static core::String toHex(const RGBA rgba, bool hashPrefix = true);

	static RGBA getRGBA(const glm::vec4&);
	/**
	 * @brief Calculate the Hue, Saturation, and Brightness (HSB) of the given color.
	 */
	static void getHSB(const glm::vec4&, float& hue, float& saturation, float& brightness);
	/**
	 * https://en.wikipedia.org/wiki/CIELAB_color_space
	 *
	 * @param[out] L lightness of the color (0 yields black and 100 indicates diffuse white; specular white may be higher)
	 * @param[out] a position between red and green (negative values indicate green and positive values indicate red)
	 * @param[out] b position between yellow and blue (negative values indicate blue and positive values indicate yellow)
	 */
	static void getCIELab(const glm::vec4&, float& L, float& a, float &b);

	static glm::vec3 gray(const glm::vec3&);
	static glm::vec4 gray(const glm::vec4&);
	static glm::vec4 alpha(const glm::vec4&, float alpha);
	static RGBA alpha(const RGBA rgba, uint8_t alpha);
	static float brightness(const glm::vec4&);
	static uint8_t brightness(const core::RGBA &color);
	static float intensity(const glm::vec4&);

	static glm::vec4 darker(const glm::vec4& color, float f = 1.0f);
	static glm::vec4 brighter(const glm::vec4& color, float f = 1.0f);
	static core::RGBA brighter(const core::RGBA color, float f = 1.0f);
	static core::RGBA darker(const core::RGBA& color, float f = 1.0f);
};

}
