/**
 * @file
 */

#pragma once

#include "core/Color.h"
#include <array>

namespace frontend {

// this size must match the color uniform size in the shader
typedef std::array<glm::vec4, 32> MaterialColorArray;

inline const MaterialColorArray& getMaterialColors() {
	static const MaterialColorArray materialColors = {{
		core::Color::LightBlue,	// air
		core::Color::Lime,			// grass1
		core::Color::DarkBrown,	// wood1
		// leaves
		core::Color::LightGreen,
		core::Color::DarkGreen,
		core::Color::Darker(core::Color::DarkGreen),
		core::Color::Green,
		core::Color::Olive,
		core::Color::Darker(core::Color::Olive),
		core::Color::Orange,
		core::Color::Darker(core::Color::Orange),
		core::Color::Darker(core::Color::Yellow),
		core::Color::Darker(core::Color::Yellow, 2.0f),
		// leaves end
		core::Color::DarkGray,		// rock1
		core::Color::DarkGray,		// rock2
		core::Color::DarkGray,		// rock3
		core::Color::DarkGray,		// rock4
		core::Color::Sandy,		// sand1
		core::Color::Sandy,		// sand2
		core::Color::Sandy,		// sand3
		core::Color::Sandy,		// sand4
		core::Color::White,		// clouds
		glm::vec4(glm::vec3(core::Color::Blue), 0.4f),	// water
		core::Color::Brown,		// dirt1
		core::Color::Brown,		// dirt2
		core::Color::Brown,		// dirt3
		core::Color::Brown,		// dirt4
		core::Color::Pink,
		core::Color::Pink,
		core::Color::Pink,
		core::Color::Pink,
		core::Color::Pink
	}};
	return materialColors;
}

}
