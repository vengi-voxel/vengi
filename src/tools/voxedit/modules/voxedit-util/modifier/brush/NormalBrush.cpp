/**
 * @file
 */

#include "NormalBrush.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {

void NormalBrush::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						  const voxel::Region &region) {
	int normalIndex = ctx.normalColorIndex;
	if (_paintMode == PaintMode::Auto) {
		// TODO: BRUSH: Implement me - see VoxelNormalUtil.h
	}
	auto func = [&](int x, int y, int z, voxel::Voxel voxel) {
		voxel.setNormal(normalIndex);
		wrapper.setVoxel(x, y, z, voxel);
	};
	voxelutil::visitVolumeParallel(wrapper, region, func);
}

} // namespace voxedit
