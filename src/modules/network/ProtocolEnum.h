/**
 * @file
 */

#pragma once

#include "core/Enum.h"
#include "Shared_generated.h"
#include <SDL_stdinc.h>

namespace network {

template<class T>
inline T getEnum(const char* name, const char * const *names) {
	int i = 0;
	while (*names) {
		if (!SDL_strcmp(*names, name)) {
			return static_cast<T>(i);
		}
		++i;
		++names;
	}
	return T::NONE;
}

template<typename ENUM>
inline const char *toString(ENUM val, const char * const *strArray) {
	return strArray[core::enumVal(val)];
}

template<class E>
struct EnumHash {
	inline size_t operator()(const E& protocolEnum) const {
		return static_cast<size_t>(protocolEnum);
	}
};

}
