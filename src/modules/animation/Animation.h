/**
 * @file
 */

#pragma once

#include <stdint.h>
#include "ToolAnimationType.h"
#include "Shared_generated.h"

/**
 * @defgroup Animation
 * @{
 * @brief Skeletal animation with lua configuration support.
 *
 * @sa Skeleton
 * @sa Bone
 * @sa SkeletonAttribute
 */

namespace animation {

using Animation = ::network::Animation;

extern const char* toString(Animation anim);

}

/**
 * @}
 */
