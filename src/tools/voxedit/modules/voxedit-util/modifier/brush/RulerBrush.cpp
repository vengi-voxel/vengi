/**
 * @file
 */

#include "RulerBrush.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Region.h"

namespace voxedit {

bool RulerBrush::beginBrush(const BrushContext &ctx) {
	_active = true;
	_startPos = ctx.cursorPosition;
	_endPos = ctx.cursorPosition;
	markDirty();
	return true;
}

bool RulerBrush::execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx) {
	_endPos = ctx.cursorPosition;
	return true;
}

void RulerBrush::endBrush(BrushContext &ctx) {
	_endPos = ctx.cursorPosition;
}

void RulerBrush::reset() {
	_active = false;
	_startPos = glm::ivec3(0);
	_endPos = glm::ivec3(0);
}

bool RulerBrush::active() const {
	return _active;
}

voxel::Region RulerBrush::calcRegion(const BrushContext &ctx) const {
	return voxel::Region::InvalidRegion;
}

bool RulerBrush::wantBrushGizmo(const BrushContext &ctx) const {
	return _active && _startPos != _endPos;
}

void RulerBrush::brushGizmoState(const BrushContext &ctx, BrushGizmoState &state) const {
	if (!wantBrushGizmo(ctx)) {
		state.operations = BrushGizmo_None;
		return;
	}
	state.operations = BrushGizmo_Line;
	state.positions[0] = glm::vec3(_startPos) + 0.5f;
	state.positions[1] = glm::vec3(_endPos) + 0.5f;
	state.numPositions = 2;
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
