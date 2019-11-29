/**
 * @file
 */

#pragma once

#include "CharacterSkeletonAttribute.h"
#include "animation/AnimationSettings.h"
#include <string>

namespace animation {

extern bool loadCharacterSettings(const std::string& luaString, AnimationSettings& settings, CharacterSkeletonAttribute& skeletonAttr);

}
