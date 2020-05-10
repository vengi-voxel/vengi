/**
 * @file
 */

#pragma once

#include "animation/chr/CharacterSkeleton.h"

namespace animation {
namespace chr {
namespace run {
extern void update(double animTime, double velocity, CharacterSkeleton& skeleton, const CharacterSkeletonAttribute& skeletonAttr);
}
}
}
