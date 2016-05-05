#pragma once

#include <array>

namespace frontend {

// this size must match the color uniform size in the shader
typedef std::array<glm::vec4, 32> MaterialColorArray;

inline const MaterialColorArray& getMaterialColors() {
	static const MaterialColorArray materialColors = {
		video::Color::LightBlue,	// air
		video::Color::Lime,			// grass1
		video::Color::DarkBrown,	// wood1
		// leaves
		video::Color::LightGreen,
		video::Color::DarkGreen,
		video::Color::Darker(video::Color::DarkGreen),
		video::Color::Green,
		video::Color::Olive,
		video::Color::Darker(video::Color::Olive),
		video::Color::Orange,
		video::Color::Darker(video::Color::Orange),
		video::Color::Darker(video::Color::Yellow),
		video::Color::Darker(video::Color::Yellow, 2.0f),
		// leaves end
		video::Color::DarkGray,		// rock1
		video::Color::DarkGray,		// rock2
		video::Color::DarkGray,		// rock3
		video::Color::DarkGray,		// rock4
		video::Color::Sandy,		// sand1
		video::Color::Sandy,		// sand2
		video::Color::Sandy,		// sand3
		video::Color::Sandy,		// sand4
		video::Color::White,		// clouds
		glm::vec4(glm::vec3(video::Color::Blue), 0.4f),	// water
		video::Color::Brown,		// dirt1
		video::Color::Brown,		// dirt2
		video::Color::Brown,		// dirt3
		video::Color::Brown,		// dirt4
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink,
		video::Color::Pink
	};
	return materialColors;
}

}
