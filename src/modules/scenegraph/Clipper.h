/**
 * @file
 */

#pragma once

#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "voxelutil/Raycast.h"

namespace scenegraph {

/**
 * @brief Clip against region boundaries and voxels
 */
class Clipper {
private:
	// Define bounding box size for one voxel
	glm::vec3 _boxSize{1.0f};

public:
	/**
	 * Clips the movement delta to avoid passing through solid voxels in the scene graph.
	 *
	 * @param frameIdx The frame index for animation transforms, or InvalidFrame for static.
	 * @param worldPosition The starting position in world coordinates.
	 * @param delta The intended movement vector in camera local space.
	 * @return The clipped movement delta that does not intersect solid voxels.
	 */
	voxelutil::RaycastResult clipDelta(const scenegraph::SceneGraph &sceneGraph, scenegraph::FrameIndex frameIdx,
									   const glm::vec3 &worldPosition, const glm::vec3 &delta,
									   const glm::mat3 &cameraOrientation) const;

	const glm::vec3 &boxSize() const {
		return _boxSize;
	}
	void setBoxSize(const glm::vec3 &boxSize) {
		_boxSize = boxSize;
	}
};

} // namespace scenegraph
