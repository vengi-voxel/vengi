#pragma once

#include "Shared_generated.h"
#include <type_traits>
#include <functional>

namespace network {

template<class T>
inline T getEnum(const char* name, const char **names) {
	int i = 0;
	while (*names) {
		if (!strcmp(*names, name)) {
			return static_cast<T>(i);
		}
		++i;
		++names;
	}
	return T::NONE;
}

template<class E>
struct EnumHash {
	inline std::size_t operator()(const E& k) const {
		return std::hash<typename std::underlying_type<E>::type>()(k);
	}
};

}
