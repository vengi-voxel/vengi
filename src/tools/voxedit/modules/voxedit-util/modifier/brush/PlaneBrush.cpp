/**
 * @file
 */

#include "PlaneBrush.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxelutil/VoxelUtil.h"

namespace voxedit {

voxel::Region PlaneBrush::calcRegion(const BrushContext &context) const {
	return voxel::Region::InvalidRegion;
}

void PlaneBrush::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
						  const BrushContext &context, const voxel::Region &region) {
	voxel::Voxel hitVoxel = context.hitCursorVoxel;
	// TODO: context.gridResolution
	// TODO: context.lockedAxis support
	const glm::ivec3 &mins = context.cursorPosition;
	if (wrapper.modifierType() == ModifierType::Place) {
		voxelutil::extrudePlane(wrapper, mins, context.cursorFace, hitVoxel, context.cursorVoxel, _thickness);
	} else if (wrapper.modifierType() == ModifierType::Erase) {
		voxelutil::erasePlane(wrapper, mins, context.cursorFace, hitVoxel);
	} else if (wrapper.modifierType() == ModifierType::Override) {
		voxelutil::overridePlane(wrapper, mins, context.cursorFace, context.cursorVoxel);
	}
}

} // namespace voxedit
