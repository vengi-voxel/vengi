/**
 * @file
 */

#include "Connected.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {
namespace select {

void Connected::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						 const voxel::Region &region, const AABBBrushState &state) {
	const voxel::Voxel &referenceVoxel = ctx.hitCursorVoxel;
	if (voxel::isAir(referenceVoxel.getMaterial())) {
		return;
	}
	const glm::ivec3 &startPos = ctx.cursorPosition;
	if (wrapper.modifierType() == ModifierType::Erase) {
		wrapper.removeFlagAt(startPos.x, startPos.y, startPos.z, voxel::FlagOutline);
	} else {
		wrapper.setFlagAt(startPos.x, startPos.y, startPos.z, voxel::FlagOutline);
	}
	auto func = [&wrapper](int x, int y, int z, const voxel::Voxel &voxel) {
		if (wrapper.modifierType() == ModifierType::Erase) {
			wrapper.removeFlagAt(x, y, z, voxel::FlagOutline);
		} else {
			wrapper.setFlagAt(x, y, z, voxel::FlagOutline);
		}
	};
	voxelutil::visitConnectedByCondition(wrapper, startPos, func);
}

} // namespace select
} // namespace voxedit
