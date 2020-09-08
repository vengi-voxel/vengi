/**
 * @file
 */

#pragma once

#include "animation/chr/CharacterSkeleton.h"

extern "C" void animation_chr_swim_update(double animTime, double velocity, animation::CharacterSkeleton* skeleton, const animation::CharacterSkeletonAttribute* skeletonAttr);
