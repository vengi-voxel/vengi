/**
 * @file
 */

#pragma once

#include <type_traits>

template<typename ARRAY>
constexpr int _lengthof() {
	using TYPE = typename std::remove_reference<ARRAY>::type;
	static_assert(std::is_array<TYPE>::value, "lengthof() requires an array argument");
	return (int)std::extent<TYPE>::value;
}

#define lengthof(a) _lengthof<decltype(a)>()
