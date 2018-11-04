/**
 * @file
 */

#pragma once

#include <glm/fwd.hpp>
#include <glm/vec4.hpp>
#include <vector>

namespace core {

union RGBA {
	struct {
		uint8_t r, g, b, a;
	};
	uint32_t rgba;
};

class Color {
public:
	static const unsigned int magnitude = 255;
	static const unsigned int magnitudef = 255.0f;
	static constexpr float scaleFactor = 0.7f;
	static const glm::vec4
		Clear,
		White,
		Black,
		LightGray,
		Gray,
		DarkGray,
		LightRed,
		Red,
		DarkRed,
		LightGreen,
		Green,
		DarkGreen,
		Lime,
		LightBlue,
		Blue,
		DarkBlue,
		SteelBlue,
		Olive,
		Pink,
		Purple,
		Yellow,
		Sandy,
		Cyan,
		Orange,
		Brown,
		LightBrown,
		DarkBrown;

	static int getClosestMatch(const glm::vec4& color, const std::vector<glm::vec4>& colors);
	static glm::vec4 fromRGB(const unsigned int rgbInt, const float a = 1.0f);
	static glm::vec4 fromRGBA(const unsigned int rgbaInt);
	static glm::vec4 fromRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
	static glm::vec4 fromHSB(const float hue, const float saturation, const float brightness, const float alpha = 1.0f);
	static glm::vec4 fromHex(const char* hex);

	static unsigned int getRGB(const glm::vec4&);
	static unsigned int getRGBA(const glm::vec4&);
	static unsigned int getBGRA(const glm::vec4& color);
	static void getHSB(const glm::vec4&, float& hue, float& saturation, float& brightness);

	static glm::vec4 alpha(const glm::vec4&, float alpha);
	static float brightness(const glm::vec4&);
	static float intensity(const glm::vec4&);

	static glm::vec4 darker(const glm::vec4& color, float f = 1.0f);
	static glm::vec4 brighter(const glm::vec4& color, float f = 1.0f);
};

}
