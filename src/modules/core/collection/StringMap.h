/**
 * @file
 */

#pragma once

#include "core/StringCacheHash.h"
#include "core/collection/Map.h"

namespace core {

/**
 * @brief String key based hash map
 * @sa core::Map
 * @sa core::String
 * @sa core::StringCacheHash
 * @ingroup Collections
 */
template<class VALUETYPE, size_t BUCKETSIZE = 11>
using StringMap = core::Map<core::StringCacheHash, VALUETYPE, BUCKETSIZE, core::StringCacheHash>;

} // namespace core
