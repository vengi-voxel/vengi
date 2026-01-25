/**
 * @file
 */

#pragma once

#include "image/Image.h"
#include "scenegraph/SceneGraph.h"

namespace voxelformat {
namespace axiom {

bool loadGroupsPalette(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph, palette::Palette &palette);
image::ImagePtr loadScreenshot(io::SeekableReadStream *stream);

} // namespace axiom
} // namespace voxelformat
