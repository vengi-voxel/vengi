/**
 * @file
 */

#pragma once

#include "image/Image.h"
#include <glm/vec2.hpp>

namespace voxelformat {

struct ThumbnailContext {
	glm::ivec2 outputSize;
};

class SceneGraph;
class SceneGraphNode;

/**
 * @brief Callback to create a thumbnail for saving the current scene graph.
 * Some formats supports storing embedded screenshots of the voxel model. This callback
 * must return a RGBA image in the given size
 * @note duplicated in VolumeFormat.h
 */
typedef image::ImagePtr (*ThumbnailCreator)(const SceneGraph &, const ThumbnailContext &);

} // namespace voxelformat
