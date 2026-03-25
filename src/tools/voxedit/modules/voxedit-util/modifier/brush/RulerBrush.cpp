/**
 * @file
 */

#include "RulerBrush.h"
#include "core/Log.h"
#include "core/collection/BitSet.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxelgenerator/ShapeGenerator.h"

namespace voxedit {

bool RulerBrush::beginBrush(const BrushContext &ctx) {
	_active = true;
	_startPos = ctx.cursorPosition;
	_endPos = ctx.cursorPosition;
	markDirty();
	return true;
}

void RulerBrush::preExecute(const BrushContext &ctx, const voxel::RawVolume *volume) {
	Super::preExecute(ctx, volume);
	_lastVolume = volume;
}

bool RulerBrush::execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx) {
	_endPos = ctx.cursorPosition;
	if (_startPos == _endPos) {
		return true;
	}
	// only draw the line during preview, not on the real volume commit
	if (wrapper.volume() == _lastVolume) {
		return true;
	}
	markDirty();
	return Super::execute(sceneGraph, wrapper, ctx);
}

void RulerBrush::generate(scenegraph::SceneGraph &, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						   const voxel::Region &region) {
	static constexpr int StipplePatternBits = 9;
	static const core::BitSet<StipplePatternBits> solidPattern = []() {
		core::BitSet<StipplePatternBits> pattern;
		for (int i = 0; i < StipplePatternBits; ++i) {
			pattern.set(i, true);
		}
		return pattern;
	}();
	int stippleState = 0;
	voxelgenerator::shape::drawStippledLine(wrapper, _startPos, _endPos, ctx.cursorVoxel, solidPattern, stippleState, false);
}

void RulerBrush::endBrush(BrushContext &ctx) {
	_endPos = ctx.cursorPosition;
	const glm::ivec3 d = delta();
	Log::info("Ruler: start=(%i, %i, %i) end=(%i, %i, %i) delta=(%i, %i, %i) length=%.2f manhattan=%i",
			  _startPos.x, _startPos.y, _startPos.z,
			  _endPos.x, _endPos.y, _endPos.z,
			  d.x, d.y, d.z,
			  euclideanDistance(), manhattanDistance());
}

void RulerBrush::reset() {
	_active = false;
	_lastVolume = nullptr;
	_startPos = glm::ivec3(0);
	_endPos = glm::ivec3(0);
}

bool RulerBrush::active() const {
	return _active;
}

voxel::Region RulerBrush::calcRegion(const BrushContext &ctx) const {
	const glm::ivec3 mins = glm::min(_startPos, ctx.cursorPosition);
	const glm::ivec3 maxs = glm::max(_startPos, ctx.cursorPosition);
	return voxel::Region(mins, maxs);
}

float RulerBrush::euclideanDistance() const {
	const glm::vec3 d = glm::vec3(_endPos - _startPos);
	return glm::length(d);
}

int RulerBrush::manhattanDistance() const {
	const glm::ivec3 d = glm::abs(_endPos - _startPos);
	return d.x + d.y + d.z;
}

} // namespace voxedit
