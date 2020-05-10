/**
 * @file
 */

#pragma once

#include "animation/chr/CharacterSkeleton.h"

namespace animation {
namespace chr {
namespace swim {
extern void update(double animTime, double velocity, CharacterSkeleton& skeleton, const CharacterSkeletonAttribute& skeletonAttr);
}
}
}
