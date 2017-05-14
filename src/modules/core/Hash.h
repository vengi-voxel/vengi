#pragma once

#include "MurmurHash3.h"

namespace core {

inline uint32_t fastHash(const void *data, int len, uint32_t seed = 0u) {
	uint32_t ret;
	MurmurHash3_x86_32(data, len, seed, &ret);
	return ret;
}

}
