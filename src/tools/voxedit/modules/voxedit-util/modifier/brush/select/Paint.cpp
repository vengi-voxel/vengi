/**
 * @file
 */

#include "Paint.h"
#include "scenegraph/SceneGraph.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxel/Connectivity.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {
namespace select {

voxel::Region Paint::calcRegion(const BrushContext &ctx, const AABBBrushState &state) const {
	const int rad = state.radius;
	if (rad > 0) {
		return voxel::Region(ctx.cursorPosition - rad, ctx.cursorPosition + rad);
	}
	return voxel::Region(ctx.cursorPosition, ctx.cursorPosition);
}

void Paint::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
					 const voxel::Region &region, const AABBBrushState &state) {
	const glm::ivec3 center = ctx.cursorPosition;
	const int rad = state.radius;
	const int radSq = rad * rad;
	const bool growOnly =
		_growRegion && wrapper.modifierType() != ModifierType::Erase && wrapper.node().hasSelection();
	auto func = [&wrapper](int x, int y, int z) {
		if (wrapper.modifierType() == ModifierType::Erase) {
			wrapper.removeFlagAt(x, y, z, voxel::FlagOutline);
		} else {
			wrapper.setFlagAt(x, y, z, voxel::FlagOutline);
		}
	};
	voxelutil::VisitSolid condition;
	auto paintFunc = [&](int x, int y, int z, const voxel::Voxel &voxel) {
		const int dx = x - center.x;
		const int dy = y - center.y;
		const int dz = z - center.z;
		if (dx * dx + dy * dy + dz * dz > radSq) {
			return;
		}
		if (growOnly) {
			bool hasSelectedNeighbor = false;
			for (const glm::ivec3 &off : ::voxel::arrayPathfinderFaces) {
				const glm::ivec3 npos(x + off.x, y + off.y, z + off.z);
				if (ctx.targetVolumeRegion.containsPoint(npos)) {
					const ::voxel::Voxel &neighborVoxel = wrapper.voxel(npos);
					if (!::voxel::isAir(neighborVoxel.getMaterial()) &&
						(neighborVoxel.getFlags() & ::voxel::FlagOutline)) {
						hasSelectedNeighbor = true;
						break;
					}
				}
			}
			if (!hasSelectedNeighbor) {
				return;
			}
		}
		func(x, y, z);
	};
	voxelutil::visitVolume(wrapper, region, paintFunc, condition);
}

} // namespace select
} // namespace voxedit
