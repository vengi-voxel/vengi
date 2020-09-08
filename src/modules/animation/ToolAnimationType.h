/**
 * @file
 */

#pragma once

#include <stdint.h>

namespace animation {

enum class ToolAnimationType : uint8_t {
	None, Swing, Stroke, Tense, Twiddle, Max
};

extern const char* toString(ToolAnimationType anim);

extern ToolAnimationType toToolAnimationEnum(const char* anim);

}

/**
 * @}
 */
