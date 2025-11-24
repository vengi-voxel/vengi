/**
 * @file
 */

#include "PathBrush.h"
#include "core/collection/List.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/RawVolume.h"
#include "voxelutil/AStarPathfinder.h"
#include "voxelutil/VoxelUtil.h"
#include "app/I18N.h"

namespace voxedit {

voxel::Region PathBrush::calcRegion(const BrushContext &ctx) const {
	return voxel::Region::InvalidRegion;
}

void PathBrush::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
						 const BrushContext &ctx, const voxel::Region &region) {
	core::List<glm::ivec3> listResult(4096);
	const glm::ivec3 &start = ctx.referencePos;
	const glm::ivec3 &end = ctx.cursorPosition;
	const int activeNode = sceneGraph.activeNode();
	const scenegraph::SceneGraphNode &node = sceneGraph.node(activeNode);
	auto func = [=](const voxel::RawVolume *vol, const glm::ivec3 &pos) {
		if (!vol->region().containsPoint(pos)) {
			return false;
		}
		if (voxel::isBlocked(vol->voxel(pos).getMaterial())) {
			return false;
		}
		return voxelutil::isTouching(*vol, pos, _connectivity);
	};
	voxelutil::AStarPathfinderParams<voxel::RawVolume> params(sceneGraph.resolveVolume(node), start, end, &listResult,
															  func, 4.0f, 10000, _connectivity);
	voxelutil::AStarPathfinder pathfinder(params);
	if (!pathfinder.execute()) {
		setErrorReason(
			_("Failed to execute pathfinder - is the reference position correctly placed on another voxel?"));
		return;
	}
	for (const glm::ivec3 &p : listResult) {
		wrapper.setVoxel(p.x, p.y, p.z, ctx.cursorVoxel);
	}
}

} // namespace voxedit
