/**
 * @file
 */

#pragma once

#include "animation/chr/CharacterSkeleton.h"

namespace animation {
namespace chr {
namespace run {
extern void update(float animTime, float velocity, CharacterSkeleton& skeleton, const CharacterSkeletonAttribute& skeletonAttr);
}
}
}
