/**
 * @file
 */

#pragma once

#include <stdint.h>
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

enum class ToolAnimationType : uint8_t {
	None, Swing, Stroke, Tense, Twiddle, Max
};

extern const char* toString(Animation anim);

extern const char* toString(ToolAnimationType anim);

extern ToolAnimationType toToolAnimationEnum(const char* anim);

}

/**
 * @}
 */
