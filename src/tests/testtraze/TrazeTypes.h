/**
 * @file
 */

#pragma once

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

struct Bike {
	PlayerId playerId;
	glm::ivec2 currentLocation;
	BikeDirection direction;
};

struct Player {
	std::string name;
	PlayerId id = 0u;
	uint32_t frags = 0u;
	uint8_t colorIndex = 0u;
	glm::vec4 color {0.0f};
};

enum class TickerType {
	Frag, Suicide, Unknown
};

struct Ticker {
	TickerType type = TickerType::Unknown;
	int casualty;
	PlayerId fragger;
};

}
