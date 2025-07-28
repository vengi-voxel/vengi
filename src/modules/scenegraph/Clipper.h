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
	// Define bounding box size for one voxel
	glm::vec3 _boxSize{1.0f};

public:
	glm::vec3 clipDelta(const scenegraph::SceneGraph &sceneGraph, scenegraph::FrameIndex frameIdx,
						const glm::vec3 &worldPosition, const glm::vec3 &delta, const glm::mat3 &cameraOrientation);

	const glm::vec3 &boxSize() const {
		return _boxSize;
	}
	void setBoxSize(const glm::vec3 &boxSize) {
		_boxSize = boxSize;
	}
};

} // namespace scenegraph
