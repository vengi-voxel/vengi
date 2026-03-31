/**
 * @file
 */

#pragma once

#include "core/String.h"

namespace core {

/**
 * @brief Use this for hash maps where the string is the key and is not modified after insertion.
 */
class StringCacheHash : public core::String {
public:
	uint64_t _hash = 0;
	StringCacheHash() = default;
	StringCacheHash(const char *str);
	StringCacheHash(const core::String &str);

	size_t operator()(const core::StringCacheHash &p) const {
		return _hash;
	}

	inline uint64_t hash() const {
		return _hash;
	}
};

} // namespace core