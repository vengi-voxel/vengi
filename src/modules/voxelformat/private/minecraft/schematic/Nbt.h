/**
 * @file
 */

#pragma once

#include "../NamedBinaryTag.h"
#include "scenegraph/SceneGraph.h"

namespace voxelformat {
namespace nbt {

bool loadGroupsPalette(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph,
					   palette::Palette &palette, int dataVersion);

} // namespace nbt
} // namespace voxelformat
