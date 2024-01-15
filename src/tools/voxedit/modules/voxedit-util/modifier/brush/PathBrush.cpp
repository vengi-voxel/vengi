/**
 * @file
 */

#include "PathBrush.h"
#include "core/collection/List.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxelutil/AStarPathfinder.h"
#include "voxelutil/VoxelUtil.h"

namespace voxedit {

bool PathBrush::execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
						const BrushContext &context) {
	core::List<glm::ivec3> listResult(4096);
	const glm::ivec3 &start = context.referencePos;
	const glm::ivec3 &end = context.cursorPosition;
	voxelutil::AStarPathfinderParams<ModifierVolumeWrapper> params(
		&wrapper, start, end, &listResult,
		[](const ModifierVolumeWrapper *vol, const glm::ivec3 &pos) {
			if (voxel::isBlocked(vol->voxel(pos).getMaterial())) {
				return false;
			}
			return voxelutil::isTouching(*vol, pos);
		},
		4.0f, 10000, voxelutil::Connectivity::EighteenConnected);
	voxelutil::AStarPathfinder pathfinder(params);
	if (!pathfinder.execute()) {
		Log::warn("Failed to execute pathfinder - is the reference position correctly placed on another voxel?");
		return false;
	}
	for (const glm::ivec3 &p : listResult) {
		wrapper.setVoxel(p.x, p.y, p.z, context.cursorVoxel);
	}
	return true;
}

} // namespace voxedit
