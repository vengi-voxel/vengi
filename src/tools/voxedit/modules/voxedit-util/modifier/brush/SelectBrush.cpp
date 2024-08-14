/**
 * @file
 */

#include "SelectBrush.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/SelectionManager.h"

namespace voxedit {

void SelectBrush::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						   const voxel::Region &region) {
	// TODO: this doesn't work for preview
	_selectionManager->select(wrapper, region.getLowerCorner(), region.getUpperCorner());
}

} // namespace voxedit
