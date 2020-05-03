/**
 * @file
 */

#pragma once

#include <stdint.h>

inline constexpr uint32_t FourCC(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
	return ((uint32_t) ((uint32_t(d) << 24) | (uint32_t(c) << 16) | (uint32_t(b) << 8) | uint32_t(a)));
}

inline void FourCCRev(uint8_t out[4], uint32_t in) {
	out[3] = (uint8_t)((in >> 24) & 0xff);
	out[2] = (uint8_t)((in >> 16) & 0xff);
	out[1] = (uint8_t)((in >> 8) & 0xff);
	out[0] = (uint8_t)((in >> 0) & 0xff);
}
