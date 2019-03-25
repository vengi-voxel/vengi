/**
 * @file
 */

#pragma once

#include <vector>
#include <string>
#include <stdint.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace traze {

struct GameInfo {
	std::string name;
	int activePlayers;
};

enum class BikeDirection {
	N, S, W, E
};

using PlayerId = uint32_t;

using Score = std::vector<std::string>;

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
	std::string name;
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
