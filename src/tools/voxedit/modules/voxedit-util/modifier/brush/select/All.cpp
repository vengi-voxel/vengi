/**
 * @file
 */

#include "All.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {
namespace select {

void All::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
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
}

bool All::needsAdditionalAction(const BrushContext &ctx) const {
	return true;
}

} // namespace select
} // namespace voxedit
