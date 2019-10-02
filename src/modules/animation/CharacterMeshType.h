/**
 * @file
 */

#pragma once

#include <stdint.h>

namespace animation {

enum class CharacterMeshType : uint8_t {
	Head = 0,
	Chest,
	Belt,
	Pants,
	Hand,
	Foot,
	Shoulder,
	Glider,
	Max
};

extern const char *toString(CharacterMeshType type);

}
