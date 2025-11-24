/**
 * @file
 */

#include "LineBrush.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxelutil/Raycast.h"

namespace voxedit {

void LineBrush::generate(scenegraph::SceneGraph &, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						 const voxel::Region &region) {
	const glm::ivec3 &start = ctx.referencePos;
	const glm::ivec3 &end = ctx.cursorPosition;
	voxel::Voxel voxel = ctx.cursorVoxel;
	voxelutil::raycastWithEndpoints(&wrapper, start, end, [&](auto &sampler) {
		const glm::ivec3 &pos = sampler.position();
		wrapper.setVoxel(pos.x, pos.y, pos.z, voxel);
		return true;
	});
	wrapper.setVoxel(end.x, end.y, end.z, voxel);
}

void LineBrush::update(const BrushContext &ctx, double nowSeconds) {
	Super::update(ctx, nowSeconds);
	if (_state != ctx) {
		_state = ctx;
		markDirty();
	}
}

voxel::Region LineBrush::calcRegion(const BrushContext &ctx) const {
	const glm::ivec3 mins = glm::min(ctx.referencePos, ctx.cursorPosition);
	const glm::ivec3 maxs = glm::max(ctx.referencePos, ctx.cursorPosition);
	return voxel::Region(mins, maxs);
}

} // namespace voxedit
