/**
 * @file
 */

#include "NormalBrush.h"
#include "palette/NormalPalette.h"
#include "scenegraph/SceneGraph.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxel/VoxelNormalUtil.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {

void NormalBrush::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						   const voxel::Region &region) {
	if (_paintMode == PaintMode::Auto) {
		const voxel::RawVolume *source = wrapper.volume();
		if (ctx.preview) {
			source = sceneGraph.resolveVolume(sceneGraph.node(sceneGraph.activeNode()));
		}
		auto func = [&](int x, int y, int z, voxel::Voxel voxel) {
			voxel::RawVolume::Sampler sampler(source);
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
		const int normalIndex = ctx.normalIndex + NORMAL_PALETTE_OFFSET;
		auto func = [&](int x, int y, int z, voxel::Voxel voxel) {
			voxel.setNormal(normalIndex);
			wrapper.setVoxel(x, y, z, voxel);
		};
		voxelutil::visitVolumeParallel(wrapper, region, func);
	}
}

} // namespace voxedit
