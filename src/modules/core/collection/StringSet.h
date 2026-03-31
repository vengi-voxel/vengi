/**
 * @file
 */

#pragma once

#include "core/StringCacheHash.h"
#include "core/collection/Set.h"

namespace core {

/**
 * @brief String key based hash set
 * @sa core::Set
 * @sa core::StringCacheHash
 * @ingroup Collections
 */
using StringSet = core::Set<core::StringCacheHash, 11, core::StringCacheHash>;

}
