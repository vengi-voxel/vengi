/**
 * @file
 */

#include "SelectBrush.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/SelectionManager.h"

namespace voxedit {

void SelectBrush::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						   const voxel::Region &region) {
	// TODO: BRUSH: this doesn't work for preview
	voxel::Region selectionRegion = region;
	if (_brushClamping) {
		selectionRegion.cropTo(ctx.targetVolumeRegion);
	}
	_selectionManager->select(wrapper, selectionRegion.getLowerCorner(), selectionRegion.getUpperCorner());
}

} // namespace voxedit
