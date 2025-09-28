/**
 * @file
 */

#include "Clipper.h"
#include "scenegraph/FrameTransform.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

#include "voxelutil/Raycast.h"

namespace scenegraph {

voxelutil::RaycastResult Clipper::clipDelta(const scenegraph::SceneGraph &sceneGraph, scenegraph::FrameIndex frameIdx,
											const glm::vec3 &worldPosition, const glm::vec3 &delta,
											const glm::mat3 &cameraOrientation) const {
	if (glm::all(glm::epsilonEqual(delta, glm::zero<glm::vec3>(), 0.0001f))) {
		return voxelutil::RaycastResult::interrupted(0.0f, 0.0f, glm::ivec3(0));
	}

	// Convert delta to world space
	const glm::vec3 worldDelta = cameraOrientation * delta;

	voxelutil::RaycastResult closest = voxelutil::RaycastResult::completed(glm::length(delta));
	for (const auto &e : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = e->second;
		if (!node.visible() || !node.isAnyModelNode()) {
			continue;
		}
		const voxel::RawVolume *volume = sceneGraph.resolveVolume(node);
		if (!volume) {
			continue;
		}

		// TODO: how to handle boxSize() - resp. boxSize() / 2.0f
		glm::vec3 start = worldPosition;
		glm::vec3 end = start + worldDelta;

		if (frameIdx != InvalidFrame) {
			const scenegraph::FrameTransform &transform = sceneGraph.transformForFrame(node, frameIdx);
			start = transform.calcModelSpace(start);
			end = transform.calcModelSpace(end);
		}

		auto callback = [](const voxel::RawVolume::Sampler &sampler) {
			const voxel::Voxel &voxel = sampler.voxel();
			return !voxel::isBlocked(voxel.getMaterial());
		};

		voxelutil::RaycastResult result = voxelutil::raycastWithEndpoints(volume, start, end, callback);
		if (result.isInterrupted() && result.fract < closest.fract) {
			closest = result;
		}
	}

	// No collisions, return requested move delta
	return closest;
}

} // namespace scenegraph
