/**
 * @file
 */

#pragma once

#include <vector>
#include "core/String.h"
#include <stdint.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace traze {

struct GameInfo {
	core::String name;
	int activePlayers;
};

enum class BikeDirection {
	N, S, W, E
};

using PlayerId = uint32_t;

using Score = std::vector<core::String>;

struct Spawn {
	glm::ivec2 position;
	bool own;
};

struct Bike {
	PlayerId playerId;
	glm::ivec2 currentLocation;
	BikeDirection direction;
};

struct Player {
	core::String name;
	PlayerId id = 0u;
	uint32_t frags = 0u;
	uint32_t owned = 0u;
	uint8_t colorIndex = 0u;
	glm::vec4 color {0.0f};
};

enum class TickerType {
	Frag, Suicide, Collision, Unknown
};

struct Ticker {
	TickerType type = TickerType::Unknown;
	int casualty;
	PlayerId fragger;
};

}
