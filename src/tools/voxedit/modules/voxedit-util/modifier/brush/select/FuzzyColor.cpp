/**
 * @file
 */

#include "FuzzyColor.h"
#include "palette/Palette.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {
namespace select {

void FuzzyColor::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
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
	const palette::Palette &palette = wrapper.node().palette();
	voxelutil::VisitVoxelFuzzyColor condition(palette, referenceVoxel.getColor(), _colorThreshold);
	voxelutil::visitVolumeParallel(wrapper, region, func, condition);
}

} // namespace select
} // namespace voxedit
