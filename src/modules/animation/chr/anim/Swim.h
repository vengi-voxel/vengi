/**
 * @file
 */

#pragma once

#include "animation/chr/CharacterSkeleton.h"

namespace animation {
namespace chr {
namespace swim {
extern void update(float animTime, float velocity, CharacterSkeleton& skeleton, const CharacterSkeletonAttribute& skeletonAttr);
}
}
}
