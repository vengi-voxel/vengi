/**
 * @file
 */

#pragma once

#include "core/io/File.h"
#include "animation/AnimationSettings.h"
#include "animation/chr/CharacterSkeletonAttribute.h"

namespace voxedit {

extern bool saveAnimationEntityLua(const animation::AnimationSettings& settings, const animation::SkeletonAttribute& sa, const char *name, const io::FilePtr& file);

}
