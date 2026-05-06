/**
 * @file
 */

#include "FlatSurface.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {
namespace select {

void FlatSurface::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						   const voxel::Region &region, const AABBBrushState &state) {
	if (ctx.cursorFace == voxel::FaceNames::Max) {
		return;
	}
	const glm::ivec3 &startPos = ctx.cursorPosition;
	if (voxel::isAir(wrapper.voxel(startPos).getMaterial())) {
		return;
	}
	auto func = [&wrapper](int x, int y, int z, const voxel::Voxel &voxel) {
		if (wrapper.modifierType() == ModifierType::Erase) {
			wrapper.removeFlagAt(x, y, z, voxel::FlagOutline);
		} else {
			wrapper.setFlagAt(x, y, z, voxel::FlagOutline);
		}
	};
	voxelutil::visitFlatSurface(wrapper, startPos, ctx.cursorFace, _flatDeviation, func);
}

} // namespace select
} // namespace voxedit
