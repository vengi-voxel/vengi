/**
 * @file
 */

#pragma once

#include "animation/chr/CharacterSkeleton.h"
#include "animation/AnimationSystem.h"

extern "C" void ANIM_APIENTRY animation_chr_sit_update(double animTime, animation::CharacterSkeleton* skeleton, const animation::CharacterSkeletonAttribute* skeletonAttr);
