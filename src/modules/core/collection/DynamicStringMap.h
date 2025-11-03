/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicMap.h"

namespace core {

/**
 * @brief String key based dynamic hash map
 * @sa core::DynamicMap
 * @sa core::String
 * @ingroup Collections
 */
template<class VALUETYPE, size_t BUCKETSIZE = 11, size_t BLOCK_SIZE = 256>
using DynamicStringMap = core::DynamicMap<core::String, VALUETYPE, BUCKETSIZE, core::StringHash, core::privdynamicmap::EqualCompare, BLOCK_SIZE>;

} // namespace core
