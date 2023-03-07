/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "voxelformat/FormatThumbnail.h"
#include "io/Stream.h"

namespace scenegraph {
class SceneGraph;
}

namespace voxelrender {

image::ImagePtr volumeThumbnail(const scenegraph::SceneGraph &sceneGraph, const voxelformat::ThumbnailContext &ctx);
bool volumeTurntable(const scenegraph::SceneGraph &sceneGraph, const core::String &imageFile, voxelformat::ThumbnailContext ctx, int loops);


} // namespace voxelrender
