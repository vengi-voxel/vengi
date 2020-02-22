/**
 * @file
 */

#include "Utility.h"
#include "core/Assert.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/round.hpp>

namespace voxel {

uint8_t logBase2(uint32_t uInput) {
	core_assert_msg(uInput != 0, "Cannot compute the log of zero.");
	core_assert_msg(glm::isPowerOfTwo(uInput), "Input must be a power of two in order to compute the log.");

	uint32_t uResult = 0;
	while ((uInput >> uResult) != 0) {
		++uResult;
	}
	return static_cast<uint8_t>(uResult - 1);
}

}
