/**
 * @file
 */

#pragma once

#include "scenegraph/SceneGraphAnimation.h"
#include "voxelutil/ImageUtils.h"

namespace scenegraph {

class SceneGraph;

/**
 * @brief Render every visible model in @p sceneGraph, or one subtree.
 *
 * @param nodeId @c InvalidNodeId walks the whole scene. Any other id renders that
 * node when it is a model, plus every model child under it. Volumes are placed
 * with the frame transform so neighbors from different nodes occlude each other.
 * @param frameIdx Keyframe used to bake each node's world transform.
 */
[[nodiscard]] image::ImagePtr renderToImage(const SceneGraph &sceneGraph, const voxelutil::ImageRenderConfig &config,
											int nodeId = InvalidNodeId, FrameIndex frameIdx = 0);

/**
 * @brief Isometric render of every visible model in @p sceneGraph, or one subtree.
 *
 * Nodes are baked into one volume before stamping, so face visibility and
 * outlines see across child boundaries. @p nodeId and @p frameIdx match
 * @c renderToImage().
 */
[[nodiscard]] image::ImagePtr renderIsometricImage(const SceneGraph &sceneGraph,
												   const voxelutil::IsoRenderConfig &config, int nodeId = InvalidNodeId,
												   FrameIndex frameIdx = 0);

} // namespace scenegraph
