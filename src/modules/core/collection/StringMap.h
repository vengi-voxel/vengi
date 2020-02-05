/**
 * @file
 */

#pragma once

#include "core/collection/Map.h"
#include "core/String.h"

namespace core {

template<class V, size_t SIZE = 8>
using StringMap = core::Map<core::String, V, SIZE, core::StringHash>;

}
