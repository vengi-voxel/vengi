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
	auto func = [&](int x, int y, int z, voxel::Voxel voxel) {
		voxel.setNormal(ctx.normalColorIndex);
		wrapper.setVoxel(x, y, z, voxel);
	};
	voxelutil::visitVolumeParallel(wrapper, region, func);
}

} // namespace voxedit
