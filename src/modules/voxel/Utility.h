/**
 * @file
 */

#pragma once

#include "core/Assert.h"
#include <glm/gtc/round.hpp>
#include <stdint.h>

namespace voxel {

/**
 * @note: this function only works for inputs which are a power of two and not zero
 * If this is not the case then the output is undefined.
 */
inline uint8_t logBase2(uint32_t uInput) {
	core_assert_msg(uInput != 0, "Cannot compute the log of zero.");
	core_assert_msg(glm::isPowerOfTwo(uInput), "Input must be a power of two in order to compute the log.");

	uint32_t uResult = 0;
	while ((uInput >> uResult) != 0) {
		++uResult;
	}
	return static_cast<uint8_t>(uResult - 1);
}

}
