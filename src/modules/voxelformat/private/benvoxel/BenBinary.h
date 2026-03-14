/**
 * @file
 */

#pragma once

#include "io/Stream.h"
#include "scenegraph/SceneGraph.h"

namespace voxelformat {
namespace benv {

bool loadBinary(scenegraph::SceneGraph &sceneGraph, palette::Palette &palette, io::SeekableReadStream &stream);
bool saveBinary(const scenegraph::SceneGraph &sceneGraph, io::SeekableWriteStream &stream);

} // namespace benv
} // namespace voxelformat
