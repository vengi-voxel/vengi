/**
 * @file
 */

#include "Functions.h"
#include "core/Assert.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/round.hpp>

namespace math {

uint8_t logBase2(uint32_t input) {
	core_assert_msg(input != 0u, "Cannot compute the log of zero.");
	core_assert_msg(glm::isPowerOfTwo(input), "Input must be a power of two in order to compute the log.");

	uint32_t result = 0u;
	while ((input >> result) != 0u) {
		++result;
	}
	return static_cast<uint8_t>(result - 1);
}

uint8_t logBase(uint32_t base, uint32_t input) {
	uint32_t result = 0u;
	while (input /= base) {
		++result;
	}
	return result;
}

}