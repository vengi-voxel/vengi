/**
 * @file
 */

#pragma once

#include "CharacterSkeletonAttribute.h"
#include "animation/AnimationSettings.h"
#include "core/String.h"
#include "core/Common.h"
#include <string>
#include <array>
#include <stdint.h>

namespace animation {

/**
 * @brief Attributes for the character meshes
 * @sa SkeletonAttribute
 */
struct CharacterSettings : public AnimationSettings {
	CharacterSkeletonAttribute skeletonAttr;
	std::string race;
	std::string gender;

	CharacterSettings() :
			AnimationSettings( { "head", "chest", "belt", "pants", "hand",
					"foot", "shoulder", "glider" }) {
	}

	bool update();
};

extern bool loadCharacterSettings(const std::string& luaString, CharacterSettings& settings);

}
