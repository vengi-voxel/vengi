/**
 * @file
 */

#pragma once

#include "animation/chr/CharacterSkeleton.h"
#include "animation/AnimationSystem.h"

extern "C" void ANIM_APIENTRY animation_chr_run_update(double animTime, double velocity, animation::CharacterSkeleton* skeleton, const animation::CharacterSkeletonAttribute* skeletonAttr);
