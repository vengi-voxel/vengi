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
	voxel::RawVolumeWrapper planeWrapper(wrapper.volume(), wrapper.region());
	if (wrapper.modifierType() == ModifierType::Place) {
		voxelutil::extrudePlane(planeWrapper, context.cursorPosition, context.cursorFace, hitVoxel, context.cursorVoxel);
	} else if (wrapper.modifierType() == ModifierType::Erase) {
		voxelutil::erasePlane(planeWrapper, context.cursorPosition, context.cursorFace, hitVoxel);
	} else if (wrapper.modifierType() == ModifierType::Paint) {
		voxelutil::paintPlane(planeWrapper, context.cursorPosition, context.cursorFace, hitVoxel, context.cursorVoxel);
	} else if (wrapper.modifierType() == (ModifierType::Place | ModifierType::Erase)) {
		voxelutil::overridePlane(planeWrapper, context.cursorPosition, context.cursorFace, context.cursorVoxel);
	}
	wrapper.addDirtyRegion(planeWrapper.dirtyRegion());
	return true;
}

} // namespace voxedit
