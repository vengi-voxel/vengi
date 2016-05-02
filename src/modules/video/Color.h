#pragma once

#include <glm/glm.hpp>

namespace video {

class Color {
public:
	static const unsigned int magnitude = 255;
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

	static glm::vec4 FromRGB(const unsigned int rgbInt, const float a = 1.0f);
	static glm::vec4 FromRGBA(const unsigned int rgbaInt);
	static glm::vec4 FromHSB(const float h, const float s, const float b, const float a = 1.0f);

	static unsigned int GetRGB(const glm::vec4&);
	static unsigned int GetRGBA(const glm::vec4&);
	static void GetHSB(const glm::vec4&, float& h, float& s, float& b);

	static float Brightness(const glm::vec4&);
	static float Intensity(const glm::vec4&);

	static glm::vec4 Darker(const glm::vec4& color, float f = 1.0f);
	static glm::vec4 Brighter(const glm::vec4& color, float f = 1.0f);
};

}
