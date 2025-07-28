/**
 * @file
 */

#pragma once

#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphAnimation.h"

namespace scenegraph {

/**
 * @brief Clip against region boundaries and voxels
 */
class Clipper {
private:
	scenegraph::SceneGraph &_sceneGraph;
	// Define bounding box size for one voxel
	glm::vec3 _boxSize{1.0f};
public:
	Clipper(scenegraph::SceneGraph &sceneGraph) : _sceneGraph(sceneGraph) {
	}
	glm::vec3 clipDelta(scenegraph::FrameIndex frameIdx, const glm::vec3 &worldPosition, const glm::vec3 &delta,
						const glm::mat3 &cameraOrientation);
};

} // namespace scenegraph
