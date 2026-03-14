/**
 * @file
 */

#pragma once

#include "io/Stream.h"
#include "scenegraph/SceneGraph.h"

namespace voxelformat {
namespace benv {

bool loadJson(scenegraph::SceneGraph &sceneGraph, palette::Palette &palette, const core::String &jsonStr);
bool saveJson(const scenegraph::SceneGraph &sceneGraph, io::SeekableWriteStream &stream);

} // namespace benv
} // namespace voxelformat
