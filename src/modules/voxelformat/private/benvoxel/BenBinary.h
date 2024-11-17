/**
 * @file
 */

#pragma once

#include "scenegraph/SceneGraph.h"

namespace voxelformat {
namespace benv {

bool loadBinary(scenegraph::SceneGraph &sceneGraph, palette::Palette &palette, io::SeekableReadStream &stream);

} // namespace benv
} // namespace voxelformat
