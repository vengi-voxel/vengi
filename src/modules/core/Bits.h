/**
 * @file
 */

#pragma once

#include <stdint.h>

namespace core {

constexpr uint32_t bits(uint32_t x, uint8_t offset, uint8_t len) {
	const uint32_t tmp = x >> offset;
	return tmp & ((1u << len) - 1u);
}

} // namespace core
