/**
 * @file
 */

#include "SameColor.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {
namespace select {

void SameColor::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						 const voxel::Region &region, const AABBBrushState &state) {
	const voxel::Voxel &referenceVoxel = ctx.hitCursorVoxel;
	if (voxel::isAir(referenceVoxel.getMaterial())) {
		return;
	}
	auto func = [&wrapper](int x, int y, int z, const voxel::Voxel &voxel) {
		if (wrapper.modifierType() == ModifierType::Erase) {
			wrapper.removeFlagAt(x, y, z, voxel::FlagOutline);
		} else {
			wrapper.setFlagAt(x, y, z, voxel::FlagOutline);
		}
	};
	voxelutil::VisitVoxelColor condition(referenceVoxel);
	voxelutil::visitVolumeParallel(wrapper, region, func, condition);
}

} // namespace select
} // namespace voxedit
