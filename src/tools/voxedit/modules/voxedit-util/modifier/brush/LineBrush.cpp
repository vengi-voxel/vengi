/**
 * @file
 */

#include "LineBrush.h"
#include "app/I18N.h"
#include "command/Command.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxelutil/Raycast.h"

namespace voxedit {

void LineBrush::construct() {
	Super::construct();
	const core::String &cmdName = name().toLower() + "brush";
	command::Command::registerCommand("toggle" + cmdName + "continuous", [&](const command::CmdArgs &args) {
		setContinuous(!continuous());
	}).setHelp(_("Change the modifier shape type"));
}

void LineBrush::reset() {
	_state = LineState();
}

void LineBrush::generate(scenegraph::SceneGraph &, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						 const voxel::Region &region) {
	const glm::ivec3 &start = ctx.referencePos;
	const glm::ivec3 &end = ctx.cursorPosition;
	voxel::Voxel voxel = ctx.cursorVoxel;
	int stippleState = 0;
	voxelutil::raycastWithEndpoints(&wrapper, start, end, [&, this](auto &sampler) {
		const glm::ivec3 &pos = sampler.position();
		if (_stipplePattern[stippleState % _stipplePattern.bits()]) {
			wrapper.setVoxel(pos.x, pos.y, pos.z, voxel);
		}
		++stippleState;
		return true;
	});
	wrapper.setVoxel(end.x, end.y, end.z, voxel);
}

void LineBrush::endBrush(BrushContext &ctx) {
	Super::endBrush(ctx);
	if (_continuous) {
		ctx.referencePos = ctx.cursorPosition;
	}
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
