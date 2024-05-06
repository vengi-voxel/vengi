/**
 * @file
 */

#include "PathBrush.h"
#include "core/collection/List.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/RawVolume.h"
#include "voxelutil/AStarPathfinder.h"
#include "voxelutil/VoxelUtil.h"
#include "scenegraph/SceneGraph.h"

namespace voxedit {

bool PathBrush::execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &context) {
	core::List<glm::ivec3> listResult(4096);
	const glm::ivec3 &start = context.referencePos;
	const glm::ivec3 &end = context.cursorPosition;
	const int activeNode = sceneGraph.activeNode();
	const scenegraph::SceneGraphNode &node = sceneGraph.node(activeNode);
	voxelutil::AStarPathfinderParams<voxel::RawVolume> params(
		sceneGraph.resolveVolume(node), start, end, &listResult,
		[=](const voxel::RawVolume *vol, const glm::ivec3 &pos) {
			if (!vol->region().containsPoint(pos)) {
				return false;
			}
			if (voxel::isBlocked(vol->voxel(pos).getMaterial())) {
				return false;
			}
			return voxelutil::isTouching(*vol, pos, _connectivity);
		},
		4.0f, 10000, _connectivity);
	voxelutil::AStarPathfinder pathfinder(params);
	if (!pathfinder.execute()) {
		Log::debug("Failed to execute pathfinder - is the reference position correctly placed on another voxel?");
		return false;
	}
	for (const glm::ivec3 &p : listResult) {
		wrapper.setVoxel(p.x, p.y, p.z, context.cursorVoxel);
	}
	return true;
}

void PathBrush::update(const BrushContext &ctx, double nowSeconds) {
	if (_state != ctx) {
		_state = ctx;
		markDirty();
	}
}

} // namespace voxedit
