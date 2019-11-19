/**
 * @file
 */

#pragma once

#include <stdint.h>

namespace animation {

enum class BoneId : uint8_t {
	Head,
	Chest,
	Belt,
	Pants,
	LeftHand,
	RightHand,
	LeftFoot,
	RightFoot,
	Tool,
	LeftShoulder,
	RightShoulder,
	/** The glider bone vertices are scaled to zero in most animations */
	Glider,
	/** The translation, orientation and scaling influences all other bones */
	Torso,
	LeftWing,
	RightWing,
	Tail,
	Max
};

struct BoneIds {
	uint8_t bones[2];
	bool mirrored[2] { false, false };
	uint8_t num;
};

}
