/**
 * @file
 */

#pragma once

#include "animation/animal/bird/BirdSkeleton.h"
#include "animation/AnimationSystem.h"

extern "C" void ANIM_APIENTRY animation_animal_bird_run_update(double animTime, double velocity, animation::BirdSkeleton* skeleton, const animation::BirdSkeletonAttribute* skeletonAttr);
