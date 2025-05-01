/**
 * @file
 */

#include "Algorithm.h"
#include <stdint.h>
#include <string.h>

namespace core {

void *memchr_not(const void *s, int c, size_t n) {
	const unsigned char *p = (const unsigned char *)s;
	unsigned char uc = (unsigned char)c;

	// Create a size_t word with all bytes set to uc
	size_t mask = uc;
	mask |= mask << 8;
	mask |= mask << 16;
#if SIZE_MAX > 0xFFFFFFFF
	mask |= mask << 32;
#endif

	// Align pointer to size_t
	while (((uintptr_t)p % sizeof(size_t)) != 0 && n > 0) {
		if (*p != uc)
			return (void *)p;
		p++;
		n--;
	}

	// Process word-at-a-time
	const size_t *wp = (const size_t *)p;
	size_t wn = n / sizeof(size_t);

	for (size_t i = 0; i < wn; i++) {
		size_t word = wp[i];
		size_t cmp = word ^ mask; // XOR to find mismatches

		// If any byte differs, cmp will have a non-zero byte
		if (cmp != 0) {
			// Find the first differing byte
			p = (const unsigned char *)&wp[i];
			for (size_t j = 0; j < sizeof(size_t); j++) {
				if (p[j] != uc)
					return (void *)(p + j);
			}
		}
	}

	// Process remaining bytes
	p = (const unsigned char *)(wp + wn);
	n %= sizeof(size_t);
	for (size_t i = 0; i < n; i++) {
		if (p[i] != uc)
			return (void *)(p + i);
	}

	return nullptr;
}

} // namespace core
