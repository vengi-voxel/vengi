/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"

namespace network {

core::DynamicArray<core::String> getNetworkAdapters();

} // namespace network
