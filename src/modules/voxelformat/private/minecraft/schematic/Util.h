/**
 * @file
 */

#pragma once

#include "../NamedBinaryTag.h"
#include "core/collection/Buffer.h"
#include <glm/fwd.hpp>

namespace voxelformat {
namespace schematic {

using SchematicPalette = core::Buffer<int>;

glm::ivec3 parsePosList(const priv::NamedBinaryTag &compound, const core::String &key);

} // namespace schematic
} // namespace voxelformat
