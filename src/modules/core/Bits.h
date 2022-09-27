/**
 * @file
 */

#pragma once

#include "core/String.h"
#include <stdint.h>

namespace core {

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

} // namespace core
