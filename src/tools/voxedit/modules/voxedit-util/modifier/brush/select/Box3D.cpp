/**
 * @file
 */

#include "Box3D.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {
namespace select {

void Box3D::reset() {
	_selectionRegion = voxel::Region::InvalidRegion;
}

void Box3D::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
					 const voxel::Region &region, const AABBBrushState &state) {
	auto func = [&wrapper](int x, int y, int z, const voxel::Voxel &voxel) {
		if (wrapper.modifierType() == ModifierType::Erase) {
			wrapper.removeFlagAt(x, y, z, voxel::FlagOutline);
		} else {
			wrapper.setFlagAt(x, y, z, voxel::FlagOutline);
		}
	};
	voxelutil::VisitSolid condition;
	voxelutil::visitVolumeParallel(wrapper, region, func, condition);
	if (wrapper.modifierType() == ModifierType::Erase) {
		_selectionRegion = voxel::Region::InvalidRegion;
	} else {
		_selectionRegion = region;
	}
}

bool Box3D::needsAdditionalAction(const BrushContext &ctx) const {
	return true;
}

} // namespace select
} // namespace voxedit
