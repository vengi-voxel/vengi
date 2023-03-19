/**
 * @file
 */

#pragma once

#include <stdint.h>

namespace core {

uint32_t hash(const void *key, int len, uint32_t seed = 0u);

// Fowler–Noll–Vo hash function CC0
// http://www.isthe.com/chongo/tech/comp/fnv/
// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
constexpr uint64_t hash(const char *inString, uint64_t seed = 14695981039346656037UL) {
	uint64_t hash = seed;
	for (const char *c = inString; *c != 0; ++c) {
		hash ^= uint64_t(*c);
		hash = hash * 1099511628211UL;
	}
	return hash;
}

}
