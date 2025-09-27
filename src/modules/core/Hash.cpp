/**
 * @file
 */

#include "Hash.h"
#include <random>

namespace core {

// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.
uint32_t hash(const void *key, int len, uint32_t seed) {
	const uint8_t *data = (const uint8_t *)key;
	const int nblocks = len / 4;
	uint32_t h1 = seed;
	const uint32_t c1 = 0xcc9e2d51;
	const uint32_t c2 = 0x1b873593;

	const uint32_t *blocks = (const uint32_t *)(data + nblocks * 4);
	for (int i = -nblocks; i != 0; i++) {
		uint32_t block = blocks[i];

		block *= c1;
		block = (block << 15) | (block >> (32 - 15));
		block *= c2;

		h1 ^= block;
		h1 = (h1 << 13) | (h1 >> (32 - 13));
		h1 = h1 * 5 + 0xe6546b64;
	}

	const uint8_t *tail = (const uint8_t *)(data + nblocks * 4);
	uint32_t k1 = 0;

	switch (len & 3) {
	case 3:
		k1 ^= tail[2] << 16;
		k1 ^= tail[1] << 8;
		k1 ^= tail[0];
		k1 *= c1;
		k1 = (k1 << 15) | (k1 >> (32 - 15));
		k1 *= c2;
		h1 ^= k1;
		break;
	case 2:
		k1 ^= tail[1] << 8;
		k1 ^= tail[0];
		k1 *= c1;
		k1 = (k1 << 15) | (k1 >> (32 - 15));
		k1 *= c2;
		h1 ^= k1;
		break;
	case 1:
		k1 ^= tail[0];
		k1 *= c1;
		k1 = (k1 << 15) | (k1 >> (32 - 15));
		k1 *= c2;
		h1 ^= k1;
		break;
	}

	h1 ^= len;
	h1 ^= h1 >> 16;
	h1 *= 0x85ebca6b;
	h1 ^= h1 >> 13;
	h1 *= 0xc2b2ae35;
	h1 ^= h1 >> 16;

	return h1;
}

} // namespace core
