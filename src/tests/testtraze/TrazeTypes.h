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

struct Bike {
	uint32_t playerId;
	glm::ivec2 currentLocation;
	BikeDirection direction;
};

struct Player {
	std::string name;
	uint32_t id;
	uint32_t frags;
	uint8_t colorIndex;
	glm::vec4 color;
};

enum class TickerType {
	Frag, Suicide, Unknown
};

struct Ticker {
	TickerType type = TickerType::Unknown;
	int casualty;
	uint32_t fragger;
};

}
