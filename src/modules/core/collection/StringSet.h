/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/Set.h"

namespace core {

/**
 * @brief String key based hash map
 * @sa core::Map
 * @sa core::String
 * @ingroup Collections
 */
using StringSet = core::Set<core::String, 11, core::StringHash>;

}
