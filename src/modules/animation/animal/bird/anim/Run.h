/**
 * @file
 */

#pragma once

#include "animation/animal/bird/BirdSkeleton.h"

extern "C" void animation_animal_bird_run_update(double animTime, double velocity, animation::BirdSkeleton* skeleton, const animation::BirdSkeletonAttribute* skeletonAttr);
