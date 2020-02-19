/**
 * @file
 */

#pragma once

#include <stdint.h>

namespace voxel {

/**
 * @note: this function only works for inputs which are a power of two and not zero
 * If this is not the case then the output is undefined.
 */
extern uint8_t logBase2(uint32_t uInput);

}
