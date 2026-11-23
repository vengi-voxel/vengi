/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/String.h"
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

constexpr uint32_t bits(uint32_t x, uint8_t offset, uint8_t len) {
	const uint32_t tmp = x >> offset;
	return tmp & ((1u << len) - 1u);
}

template<typename TYPE>
core::String toBitString(TYPE val) {
	size_t l = sizeof(val) * 8;
	core::String str(l, '0');
	for (size_t i = 0; i < l; ++i) {
		if ((val & (1 << i)) != 0) {
			str[l - 1 - i] = '1';
		}
	}
	return str;
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
