/**
 * @file
 */

#pragma once

#include "animation/chr/CharacterSkeleton.h"
#include "animation/ToolAnimationType.h"

namespace animation {
namespace chr {
namespace tool {
extern void update(double animTime, ToolAnimationType animation, CharacterSkeleton& skeleton, const CharacterSkeletonAttribute& skeletonAttr);
}
}
}
