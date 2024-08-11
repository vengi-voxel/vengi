/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "voxel/Region.h"

namespace voxedit {

using Selection = voxel::Region;
using Selections = core::DynamicArray<Selection>;

} // namespace voxedit
