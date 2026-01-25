/**
 * @file
 */

#pragma once

#include "../NamedBinaryTag.h"
#include "scenegraph/SceneGraph.h"

namespace voxelformat {
namespace litematic {

bool loadGroupsPalette(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph,
					   palette::Palette &palette);

} // namespace litematic
} // namespace voxelformat
