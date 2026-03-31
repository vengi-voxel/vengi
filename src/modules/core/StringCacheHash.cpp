/**
 * @file
 */

#include "core/StringCacheHash.h"
#include "core/Hash.h"

namespace core {

StringCacheHash::StringCacheHash(const char *str) : core::String(str) {
	_hash = core::hash((const void *)c_str(), (int)size());
}

StringCacheHash::StringCacheHash(const core::String &str) : core::String(str) {
	_hash = core::hash((const void *)c_str(), (int)size());
}

} // namespace core