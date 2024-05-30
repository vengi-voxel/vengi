/**
 * @file
 */

#include "LineBrush.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxelutil/Raycast.h"

namespace voxedit {

void LineBrush::generate(scenegraph::SceneGraph &, ModifierVolumeWrapper &wrapper, const BrushContext &context, const voxel::Region &region) {
	const glm::ivec3 &start = context.referencePos;
	const glm::ivec3 &end = context.cursorPosition;
	voxel::Voxel voxel = context.cursorVoxel;
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

voxel::Region LineBrush::calcRegion(const BrushContext &context) const {
	const glm::ivec3 mins = glm::min(context.referencePos, context.cursorPosition);
	const glm::ivec3 maxs = glm::max(context.referencePos, context.cursorPosition);
	return voxel::Region(mins, maxs);
}

} // namespace voxedit
