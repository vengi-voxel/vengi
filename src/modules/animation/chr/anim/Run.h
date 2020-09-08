/**
 * @file
 */

#pragma once

#include "animation/chr/CharacterSkeleton.h"

extern "C" void animation_chr_run_update(double animTime, double velocity, animation::CharacterSkeleton* skeleton, const animation::CharacterSkeletonAttribute* skeletonAttr);
