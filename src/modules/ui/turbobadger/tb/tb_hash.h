/**
 * @file
 */

#pragma once

#include "tb_types.h"

namespace tb {

// On C++ compilers that support it, use constexpr for hash so that
// TBID comparisons turn into simple uint32 comparisons compile time.

// FNV constants
static constexpr uint32_t basis = 2166136261U;
static constexpr uint32_t prime = 16777619U;

// compile-time hash helper function
constexpr uint32_t TBGetHash_one(char c, const char* remain, uint32_t value)
{
	return c == 0 ? value : TBGetHash_one(remain[0], remain + 1, (value ^ c) * prime);
}

// compile-time hash
constexpr uint32_t TBGetHash(const char* str)
{
	return (str && *str) ? TBGetHash_one(str[0], str + 1, basis) : 0;
}

#define TBIDC(str) tb::TBGetHash(str)

} // namespace tb

