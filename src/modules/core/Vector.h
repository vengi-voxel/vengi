/**
 * @file
 */

#pragma once

#include <vector>
#include <type_traits>

namespace core {

template<class T>
inline size_t vectorSize(const std::vector<T>& v) {
	return v.size() * sizeof(T);
}

template<class T>
inline size_t vectorCapacity(const std::vector<T>& v) {
	return v.capacity() * sizeof(T);
}

template<typename T, typename _ = void>
struct isVector : std::false_type {
};

template<typename T>
struct isVector<T, typename std::enable_if<
				std::is_same<
					typename std::decay<T>::type,
					std::vector<typename std::decay<T>::type::value_type, typename std::decay<T>::type::allocator_type>
				>::value
			>::type
		> : std::true_type {
};

}
