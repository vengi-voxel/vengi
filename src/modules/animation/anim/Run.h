/**
 * @file
 */

#pragma once

#include "animation/Skeleton.h"

namespace animation {
namespace run {
extern void update(float animTime, float velocity, CharacterSkeleton& skeleton, const SkeletonAttribute& skeletonAttr);
}
}
