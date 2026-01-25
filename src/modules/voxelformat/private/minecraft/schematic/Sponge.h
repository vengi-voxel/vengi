/**
 * @file
 */

#pragma once

#include "../NamedBinaryTag.h"
#include "scenegraph/SceneGraph.h"

namespace voxelformat {
namespace sponge {

bool loadGroupsPaletteSponge1And2(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph,
								  palette::Palette &palette);
bool loadGroupsPaletteSponge3(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph,
							  palette::Palette &palette, int version);

} // namespace sponge
} // namespace voxelformat
