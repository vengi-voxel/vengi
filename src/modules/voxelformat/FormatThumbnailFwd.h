/**
 * @file
 */

#pragma once

#include "image/ImageFwd.h"

namespace scenegraph {
class SceneGraph;
}

namespace voxelformat {

struct ThumbnailContext;

/**
 * @brief Callback to create a thumbnail for saving the current scene graph.
 * Some formats supports storing embedded screenshots of the voxel model. This callback
 * must return a RGBA image in the given size
 */
typedef image::ImagePtr (*ThumbnailCreator)(const scenegraph::SceneGraph &, const ThumbnailContext &);

} // namespace voxelformat
