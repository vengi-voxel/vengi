/**
 * @file
 */

#pragma once

#include "core/collection/Buffer.h"
#include "voxel/Region.h"

namespace voxedit {

using Selection = voxel::Region;
using Selections = core::Buffer<Selection>;

} // namespace voxedit
