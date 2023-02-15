/**
 * @file
 */

#pragma once

#include <stdint.h>

// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

namespace core {

uint32_t hash(const void *key, int len, uint32_t seed = 0u);
}
