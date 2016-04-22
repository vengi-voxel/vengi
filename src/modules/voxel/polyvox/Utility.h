#pragma once

#include "core/Common.h"
#include <cstdint>

namespace voxel {

inline bool isPowerOf2(uint32_t uInput) {
	if (uInput == 0)
		return false;
	return (uInput & (uInput - 1)) == 0;
}

//Note: this function only works for inputs which are a power of two and not zero
//If this is not the case then the output is undefined.
inline uint8_t logBase2(uint32_t uInput) {
	//Release mode validation
	if (uInput == 0) {
		core_assert_msg(false, "Cannot compute the log of zero.");
	}
	if (!isPowerOf2(uInput)) {
		core_assert_msg(false, "Input must be a power of two in order to compute the log.");
	}

	uint32_t uResult = 0;
	while ((uInput >> uResult) != 0) {
		++uResult;
	}
	return static_cast<uint8_t>(uResult - 1);
}

}
