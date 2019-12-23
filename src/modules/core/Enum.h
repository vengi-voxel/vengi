/**
 * @file
 * @mainpage VoxelEngine documentation
 *
 * - [GitLab page](http://gitlab.com/mgerhardy/engine/)
 */

#pragma once

#include <stddef.h>
#include <utility>

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

namespace std {
template<typename T>
constexpr typename underlying_type<T>::type enum_value(const T& val) {
	return static_cast<typename underlying_type<T>::type>(val);
}
}

struct EnumClassHash {
template<typename T>
size_t operator()(T t) const {
	return static_cast<size_t>(std::enum_value(t));
}
};
