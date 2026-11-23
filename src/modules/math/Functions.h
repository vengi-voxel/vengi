/**
 * @file
 */

#pragma once

#include "core/Bits.h"
#include <stdint.h>

namespace math {

// Index of the highest set bit. Zero returns -1.
inline int findMSB(int v) {
	if (v == 0) {
		return -1;
	}
	return 31 - core::countLeadingZeros((uint32_t)v);
}

extern uint8_t logBase2(uint32_t input);
extern uint8_t logBase(uint32_t base, uint32_t input);

}