/**
 * @file
 */

#pragma once

#include <stdint.h>
#include <SDL_endian.h>

inline constexpr uint32_t FourCC(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	return (uint32_t) ((uint32_t(d) << 24) | (uint32_t(c) << 16) | (uint32_t(b) << 8) | uint32_t(a));
#else
	return (uint32_t) ((uint32_t(a) << 24) | (uint32_t(b) << 16) | (uint32_t(c) << 8) | uint32_t(d));
#endif
}

inline void FourCCRev(uint8_t out[4], uint32_t in) {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	out[3] = (uint8_t)((in >> 24) & 0xff);
	out[2] = (uint8_t)((in >> 16) & 0xff);
	out[1] = (uint8_t)((in >> 8) & 0xff);
	out[0] = (uint8_t)((in >> 0) & 0xff);
#else
	out[0] = (uint8_t)((in >> 24) & 0xff);
	out[1] = (uint8_t)((in >> 16) & 0xff);
	out[2] = (uint8_t)((in >> 8) & 0xff);
	out[3] = (uint8_t)((in >> 0) & 0xff);
#endif
}
