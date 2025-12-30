/**
 * @file
 */

#include "NormalBrush.h"
#include "palette/NormalPalette.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxel/VoxelNormalUtil.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {

void NormalBrush::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						   const voxel::Region &region) {
	if (_paintMode == PaintMode::Auto) {
		auto func = [&](int x, int y, int z, voxel::Voxel voxel) {
			voxel::RawVolume::Sampler sampler(wrapper.volume());
			sampler.setPosition(x, y, z);
			const glm::vec3 &normal = voxel::calculateNormal(sampler, voxel::Connectivity::TwentySixConnected);
			const int normalPaletteIndex = wrapper.node().normalPalette().getClosestMatch(normal);
			if (normalPaletteIndex == palette::PaletteNormalNotFound) {
				return;
			}
			voxel.setNormal(normalPaletteIndex + NORMAL_PALETTE_OFFSET);
			wrapper.setVoxel(x, y, z, voxel);
		};
		voxelutil::visitVolumeParallel(wrapper, region, func);
	} else {
		int normalIndex = ctx.normalIndex;
		auto func = [&](int x, int y, int z, voxel::Voxel voxel) {
			voxel.setNormal(normalIndex);
			wrapper.setVoxel(x, y, z, voxel);
		};
		voxelutil::visitVolumeParallel(wrapper, region, func);
	}
}

} // namespace voxedit
