/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"

namespace voxedit {
namespace network {

core::DynamicArray<core::String> getNetworkAdapters();

} // namespace network
} // namespace voxedit
