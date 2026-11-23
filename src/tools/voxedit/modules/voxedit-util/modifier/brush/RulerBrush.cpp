/**
 * @file
 */

#include "RulerBrush.h"
#include "app/I18N.h"
#include "command/Command.h"
#include "voxel/Region.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"

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
	_startPos = math::Vec3i(0);
	_endPos = math::Vec3i(0);
}

bool RulerBrush::active() const {
	return _state != State::Idle;
}

voxel::Region RulerBrush::calcRegion(const BrushContext &ctx) const {
	return voxel::Region::InvalidRegion;
}

void RulerBrush::handles(const BrushContext &ctx, core::DynamicArray<BrushGizmoState> &output) const {
	if (hasHandle(ctx)) {
		BrushGizmoState handle;
		describeHandle(ctx, handle);
		output.push_back(handle);
	}
}

bool RulerBrush::hasHandle(const BrushContext &ctx) const {
	return _state != State::Idle && _startPos != _endPos;
}

void RulerBrush::describeHandle(const BrushContext &ctx, BrushGizmoState &state) const {
	if (!hasHandle(ctx)) {
		state.operations = BrushGizmo_None;
		return;
	}
	state.operations = BrushGizmo_Line;
	state.positions[0] = math::Vec3f(_startPos) + 0.5f;
	state.positions[1] = math::Vec3f(_endPos) + 0.5f;
	state.numPositions = 2;
}

float RulerBrush::euclideanDistance() const {
	const math::Vec3f d = math::Vec3f(_endPos - _startPos);
	return math::length(d);
}

int RulerBrush::manhattanDistance() const {
	const math::Vec3i d = math::abs(_endPos - _startPos);
	return d.x + d.y + d.z;
}

} // namespace voxedit
