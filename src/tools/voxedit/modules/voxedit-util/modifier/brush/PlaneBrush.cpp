/**
 * @file
 */

#include "PlaneBrush.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxelutil/VoxelUtil.h"

namespace voxedit {

bool PlaneBrush::execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
						 const BrushContext &context) {
	voxel::Voxel hitVoxel = context.hitCursorVoxel;
	// TODO: context.gridResolution
	// TODO: context.lockedAxis support
	if (wrapper.modifierType() == ModifierType::Place) {
		voxelutil::extrudePlane(wrapper, context.cursorPosition, context.cursorFace, hitVoxel, context.cursorVoxel);
	} else if (wrapper.modifierType() == ModifierType::Erase) {
		voxelutil::erasePlane(wrapper, context.cursorPosition, context.cursorFace, hitVoxel);
	} else if (wrapper.modifierType() == ModifierType::Paint) {
		voxelutil::paintPlane(wrapper, context.cursorPosition, context.cursorFace, hitVoxel, context.cursorVoxel);
	} else if (wrapper.modifierType() == (ModifierType::Place | ModifierType::Erase)) {
		voxelutil::overridePlane(wrapper, context.cursorPosition, context.cursorFace, context.cursorVoxel);
	}
	return true;
}

} // namespace voxedit
