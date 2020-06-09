/**
 * @file
 */

#pragma once

#include "core/collection/Map.h"
#include "core/String.h"

namespace core {

/**
 * @brief String key based hash map
 * @sa core::Map
 * @sa core::String
 * @ingroup Collections
 */
template<class V, size_t SIZE = 11>
using StringMap = core::Map<core::String, V, SIZE, core::StringHash>;

}
