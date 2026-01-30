/**
 * @file
 */

#include "SelectBrush.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Voxel.h"

namespace voxedit {

void SelectBrush::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						   const voxel::Region &region) {
	voxel::Region selectionRegion = region;
	if (_brushClamping) {
		selectionRegion.cropTo(ctx.targetVolumeRegion);
	}
	if (_remove) {
		wrapper.removeFlags(selectionRegion, voxel::FlagOutline);
	} else {
		wrapper.setFlags(selectionRegion, voxel::FlagOutline);
	}
}

} // namespace voxedit
