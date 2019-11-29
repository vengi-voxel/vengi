/**
 * @file
 */

#pragma once

#include "core/io/File.h"
#include "animation/chr/CharacterSettings.h"

namespace voxedit {

extern bool saveCharacterLua(const animation::AnimationSettings& settings, const animation::CharacterSkeletonAttribute& sa, const char *name, const io::FilePtr& file);

}
