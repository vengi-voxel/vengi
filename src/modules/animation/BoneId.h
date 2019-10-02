/**
 * @file
 */

#pragma once

#include <stdint.h>

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
	Max
};
