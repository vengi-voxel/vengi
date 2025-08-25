/**
 * @file
 */

#include "Clipper.h"
#include "scenegraph/FrameTransform.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"

namespace scenegraph {

glm::vec3 Clipper::clipDelta(const scenegraph::SceneGraph &sceneGraph, scenegraph::FrameIndex frameIdx,
							 const glm::vec3 &worldPosition, const glm::vec3 &delta,
							 const glm::mat3 &cameraOrientation) const {
	if (glm::all(glm::epsilonEqual(delta, glm::zero<glm::vec3>(), 0.0001f))) {
		return delta;
	}

	// Convert delta to world space
	const glm::vec3 worldDelta = cameraOrientation * delta;
	const glm::vec3 boxHalfExtents = _boxSize * 0.5f;
	const glm::vec3 intermediatePos = worldPosition + worldDelta;

	// Calculate the bounding box corners to collide with
	const glm::vec3 minCorner = intermediatePos - boxHalfExtents;
	const glm::vec3 maxCorner = intermediatePos + boxHalfExtents;

	glm::ivec3 minVoxel(0);
	glm::ivec3 maxVoxel(0);
	if (frameIdx == InvalidFrame) {
		minVoxel = glm::floor(minCorner);
		maxVoxel = glm::ceil(maxCorner);
	}

	for (const auto &e : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = e->second;
		if (!node.visible() || !node.isAnyModelNode()) {
			continue;
		}
		const voxel::RawVolume *volume = sceneGraph.resolveVolume(node);
		if (!volume) {
			continue;
		}

		if (frameIdx != InvalidFrame) {
			// Convert corners to voxel-space
			const scenegraph::FrameTransform &transform = sceneGraph.transformForFrame(node, frameIdx);
			minVoxel = glm::floor(transform.calcModelSpace(minCorner));
			maxVoxel = glm::ceil(transform.calcModelSpace(maxCorner));
		}

		// Iterate through all voxels inside the region
		const voxel::Region region(minVoxel, maxVoxel - 1);
		core_assert(region.getDimensionsInVoxels().x <= 2 && region.getDimensionsInVoxels().y <= 2 &&
					region.getDimensionsInVoxels().z <= 2);
		auto visitor = [&](int, int, int, const voxel::Voxel &) {
			// break loop on first match - we found a solid voxel
			return true;
		};
		const int cnt = voxelutil::visitVolume(*volume, region, visitor, voxelutil::SkipEmpty());
		if (cnt == 0) {
			// No voxels in this region - no clipping
			continue;
		}
		// No movement allowed because we hit at least one solid voxel in the region
		return {0.0f, 0.0f, 0.0f};
	}

	// No collisions, return requested move delta
	return delta;
}

} // namespace scenegraph
