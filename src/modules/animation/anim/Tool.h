/**
 * @file
 */

#pragma once

#include "animation/Skeleton.h"
#include "animation/Animation.h"

namespace animation {
namespace tool {
extern void update(float animTime, ToolAnimationType animation, CharacterSkeleton& skeleton, const SkeletonAttribute& skeletonAttr);
}
}
