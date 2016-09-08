#pragma once

#include "Shared_generated.h"

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

}
