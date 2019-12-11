/**
 * @file
 */

#pragma once

#include <stdint.h>

namespace animation {

/**
 * @brief Available bone types that are mapped to the AnimationEntity vertices
 */
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
	Body,
	Max
};

extern BoneId toBoneId(const char *name);
extern const char* toBoneId(const BoneId id);

struct BoneIds {
	BoneId bones[2] { BoneId::Max, BoneId::Max };
	bool mirrored[2] { false, false };
	uint8_t num = 0;
};

}
