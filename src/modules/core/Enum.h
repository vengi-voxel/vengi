/**
 * @file
 */

#pragma once

#include <stddef.h>
#include <type_traits>

#define CORE_ENUM_BIT_OPERATIONS(EnumClassName) \
	inline constexpr EnumClassName operator&(EnumClassName __x, EnumClassName __y) { \
		return static_cast<EnumClassName>(static_cast<int>(__x) & static_cast<int>(__y)); \
	} \
	inline constexpr EnumClassName operator|(EnumClassName __x, EnumClassName __y) { \
		return static_cast<EnumClassName>(static_cast<int>(__x) | static_cast<int>(__y)); \
	} \
	inline constexpr EnumClassName operator^(EnumClassName __x, EnumClassName __y) { \
		return static_cast<EnumClassName>(static_cast<int>(__x) ^ static_cast<int>(__y)); \
	} \
	inline constexpr EnumClassName operator~(EnumClassName __x) { \
		return static_cast<EnumClassName>(~static_cast<int>(__x)); \
	} \
	inline EnumClassName& operator&=(EnumClassName & __x, EnumClassName __y) { \
		__x = __x & __y; \
		return __x; \
	} \
	inline EnumClassName& operator|=(EnumClassName & __x, EnumClassName __y) { \
		__x = __x | __y; \
		return __x; \
	} \
	inline EnumClassName& operator^=(EnumClassName & __x, EnumClassName __y) { \
		__x = __x ^ __y; \
		return __x; \
	}

namespace core {
template<typename T>
constexpr typename std::underlying_type<T>::type enumVal(const T& val) {
	return static_cast<typename std::underlying_type<T>::type>(val);
}
}

struct EnumClassHash {
template<typename T>
size_t operator()(T t) const {
	return static_cast<size_t>(core::enumVal(t));
}
};
