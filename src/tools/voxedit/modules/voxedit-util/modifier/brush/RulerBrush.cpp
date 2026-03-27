/**
 * @file
 */

#include "RulerBrush.h"
#include "app/I18N.h"
#include "command/Command.h"
#include "voxel/Region.h"

namespace voxedit {

void RulerBrush::construct() {
	Super::construct();
	const core::String &cmdName = name().toLower() + "brush";
	command::Command::registerCommand("toggle" + cmdName + "referencepos")
		.setHandler([this](const command::CommandArgs &args) { setUseReferencePos(!useReferencePos()); })
		.setHelp(_("Toggle measuring from the reference position"));
}

void RulerBrush::shutdown() {
	Super::shutdown();
	const core::String &cmdName = name().toLower() + "brush";
	command::Command::unregisterCommand("toggle" + cmdName + "referencepos");
}

bool RulerBrush::beginBrush(const BrushContext &ctx) {
	if (_useReferencePos) {
		_startPos = ctx.referencePos;
		_endPos = ctx.cursorPosition;
		_state = (_startPos != _endPos) ? State::Measured : State::Idle;
		markDirty();
		return true;
	}
	if (_state == State::Tracking) {
		_endPos = ctx.cursorPosition;
		_state = State::Measured;
	} else {
		_startPos = ctx.cursorPosition;
		_endPos = ctx.cursorPosition;
		_state = State::Tracking;
	}
	markDirty();
	return true;
}

void RulerBrush::update(const BrushContext &ctx, double nowSeconds) {
	Super::update(ctx, nowSeconds);
	if (_useReferencePos) {
		if (_state == State::Measured) {
			_startPos = ctx.referencePos;
		}
		return;
	}
	if (_state == State::Tracking) {
		_endPos = ctx.cursorPosition;
	}
}

void RulerBrush::reset() {
	_state = State::Idle;
	_startPos = glm::ivec3(0);
	_endPos = glm::ivec3(0);
}

bool RulerBrush::active() const {
	return _state != State::Idle;
}

voxel::Region RulerBrush::calcRegion(const BrushContext &ctx) const {
	return voxel::Region::InvalidRegion;
}

bool RulerBrush::wantBrushGizmo(const BrushContext &ctx) const {
	return _state != State::Idle && _startPos != _endPos;
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
