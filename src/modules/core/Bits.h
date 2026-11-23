/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include <stdint.h>
#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#endif

namespace core {

/** Returns the index of the first set bit, or 64 for zero. */
CORE_FORCE_INLINE int countTrailingZeros(uint64_t value) {
#if defined(__GNUC__) || defined(__clang__)
	return value != 0 ? __builtin_ctzll(value) : 64;
#elif defined(_MSC_VER)
	unsigned long bit;
	if (_BitScanForward(&bit, (uint32_t)value)) {
		return (int)bit;
	}
	return _BitScanForward(&bit, (uint32_t)(value >> 32)) ? 32 + (int)bit : 64;
#else
	if (value == 0) {
		return 64;
	}
	int bit = 0;
	while ((value & 1u) == 0) {
		++bit;
		value >>= 1;
	}
	return bit;
#endif
}

/** Returns the number of leading zero bits in a 32-bit word, or 32 for zero. */
CORE_FORCE_INLINE int countLeadingZeros(uint32_t value) {
#if defined(__GNUC__) || defined(__clang__)
	return value != 0u ? __builtin_clz(value) : 32;
#elif defined(_MSC_VER)
	unsigned long bit;
	return _BitScanReverse(&bit, value) ? 31 - (int)bit : 32;
#else
	if (value == 0u) {
		return 32;
	}
	int bit = 0;
	while ((value & 0x80000000u) == 0u) {
		++bit;
		value <<= 1;
	}
	return bit;
#endif
}

constexpr uint32_t bits(uint32_t x, uint8_t offset, uint8_t len) {
	const uint32_t tmp = x >> offset;
	return tmp & ((1u << len) - 1u);
}

inline int countSetBits(uint32_t number) {
	int count = 0;
	while (number) {
		count += number & 1;
		number >>= 1;
	}
	return count;
}

} // namespace core
