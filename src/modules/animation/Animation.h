/**
 * @file
 */

#pragma once

#include <stdint.h>

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

enum class Animation : uint8_t {
	Idle, Jump, Run, Glide, Tool, Max
};

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
