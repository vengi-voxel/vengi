/**
 * @file
 */

#include "LineBrush.h"
#include "app/I18N.h"
#include "command/Command.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelgenerator/ShapeGenerator.h"

namespace voxedit {

bool LineBrush::allowNextBezierPreview(const BrushContext &ctx, int selectedSegment, int segmentCount) {
	if (ctx.brushGizmoActive) {
		return false;
	}
	return selectedSegment == -1 || selectedSegment == segmentCount - 1;
}

void LineBrush::onActivated() {
	_commitPending = false;
	_active = false;
	_lastVolume = nullptr;
}

bool LineBrush::hasPendingChanges() const {
	return _bezier && !_segments.empty();
}

bool LineBrush::onDeactivated() {
	_commitPending = hasPendingChanges();
	return _commitPending;
}

void LineBrush::construct() {
	Super::construct();
	const core::String &cmdName = name().toLower() + "brush";
	command::Command::registerCommand("toggle" + cmdName + "continuous")
		.setHandler([this](const command::CommandArgs &args) { setContinuous(!continuous()); })
		.setHelp(_("Toggle continuous line drawing"));
	command::Command::registerCommand("toggle" + cmdName + "bezier")
		.setHandler([this](const command::CommandArgs &args) { setBezier(!bezier()); })
		.setHelp(_("Toggle quadratic bezier mode for the line brush"));
}

bool LineBrush::beginBrush(const BrushContext &ctx) {
	if (_active) {
		return false;
	}
	_active = true;
	return true;
}

void LineBrush::preExecute(const BrushContext &ctx, const voxel::RawVolume *volume) {
	Super::preExecute(ctx, volume);
	_lastVolume = volume;
}

void LineBrush::shutdown() {
	Super::shutdown();
	const core::String &cmdName = name().toLower() + "brush";
	command::Command::unregisterCommand("toggle" + cmdName + "continuous");
	command::Command::unregisterCommand("toggle" + cmdName + "bezier");
}

void LineBrush::reset() {
	_state = LineState();
	clearPendingSegments();
	_commitPending = false;
	_active = false;
	_lastVolume = nullptr;
	_hasCustomControlPoint = false;
	_controlPoint = glm::zero<glm::ivec3>();
	_thickness = 1;
}

bool LineBrush::execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx) {
	if (!_bezier) {
		return Super::execute(sceneGraph, wrapper, ctx);
	}

	if (_commitPending) {
		for (const BezierSegment &segment : _segments) {
			voxelgenerator::shape::drawBezierSegment(wrapper, segment, ctx.cursorVoxel, _stipplePattern, _thickness);
		}
		return true;
	}

	if (wrapper.volume() == _lastVolume) {
		lockSegment(ctx);
		markDirty();
		return true;
	}

	for (const BezierSegment &segment : _segments) {
		voxelgenerator::shape::drawBezierSegment(wrapper, segment, ctx.cursorVoxel, _stipplePattern, _thickness);
	}
	if (allowNextBezierPreview(ctx, _selectedSegment, (int)_segments.size()) &&
		ctx.referencePos != ctx.cursorPosition) {
		const BezierSegment previewSegment{ctx.referencePos, ctx.cursorPosition, previewControlPoint(ctx)};
		voxelgenerator::shape::drawBezierSegment(wrapper, previewSegment, ctx.cursorVoxel, _stipplePattern, _thickness);
	}
	return true;
}

void LineBrush::generate(scenegraph::SceneGraph &, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						 const voxel::Region &region) {
	const glm::ivec3 &start = ctx.referencePos;
	const glm::ivec3 &end = ctx.cursorPosition;
	voxel::Voxel voxel = ctx.cursorVoxel;
	int stippleState = 0;
	if (!_bezier) {
		voxelgenerator::shape::drawStippledLine(wrapper, start, end, voxel, _stipplePattern, stippleState, false, _thickness);
		return;
	}

	voxelgenerator::shape::drawBezierSegment(wrapper, BezierSegment{start, end, controlPoint(ctx)}, voxel,
											 _stipplePattern, _thickness);
}

void LineBrush::endBrush(BrushContext &ctx) {
	_active = false;
	if (_bezier && !_segments.empty()) {
		ctx.referencePos = _segments.back().end;
	}
	if (_commitPending) {
		clearPendingSegments();
		_commitPending = false;
	}
	_hasCustomControlPoint = false;
	if (_continuous) {
		ctx.referencePos = ctx.cursorPosition;
	}
}

void LineBrush::update(const BrushContext &ctx, double nowSeconds) {
	Super::update(ctx, nowSeconds);
	syncControlPoint(ctx);
	const glm::ivec3 currentControlPoint = controlPoint(ctx);
	if (_state.hasChanges(ctx, _bezier, currentControlPoint)) {
		_state.update(ctx, _bezier, currentControlPoint);
		markDirty();
	}
}

voxel::Region LineBrush::calcRegion(const BrushContext &ctx) const {
	if (_bezier) {
		voxel::Region region = voxel::Region::InvalidRegion;
		for (const BezierSegment &segment : _segments) {
			const voxel::Region segmentBounds = voxelgenerator::shape::bezierRegion(segment, _thickness);
			if (!region.isValid()) {
				region = segmentBounds;
			} else {
				region.accumulate(segmentBounds);
			}
		}
		if (allowNextBezierPreview(ctx, _selectedSegment, (int)_segments.size()) &&
			ctx.referencePos != ctx.cursorPosition) {
			const voxel::Region previewBounds =
				voxelgenerator::shape::bezierRegion({ctx.referencePos, ctx.cursorPosition, previewControlPoint(ctx)}, _thickness);
			if (!region.isValid()) {
				region = previewBounds;
			} else {
				region.accumulate(previewBounds);
			}
		}
		return region;
	}
	glm::ivec3 mins = glm::min(ctx.referencePos, ctx.cursorPosition);
	glm::ivec3 maxs = glm::max(ctx.referencePos, ctx.cursorPosition);
	if (_thickness > 1) {
		mins -= _thickness;
		maxs += _thickness;
	}
	return voxel::Region(mins, maxs);
}

glm::ivec3 LineBrush::defaultControlPoint(const BrushContext &ctx) const {
	return ctx.referencePos + (ctx.cursorPosition - ctx.referencePos) / 2;
}

glm::ivec3 LineBrush::previewControlPoint(const BrushContext &ctx) const {
	if (_hasCustomControlPoint) {
		return _controlPoint;
	}
	return defaultControlPoint(ctx);
}

glm::ivec3 LineBrush::controlPoint(const BrushContext &ctx) const {
	if (_selectedSegment >= 0 && _selectedSegment < (int)_segments.size()) {
		return _segments[_selectedSegment].control;
	}
	return previewControlPoint(ctx);
}

void LineBrush::syncControlPoint(const BrushContext &ctx) {
	if (!_bezier) {
		return;
	}
	if (_selectedSegment >= 0 && _selectedSegment < (int)_segments.size()) {
		return;
	}
	const glm::ivec3 newDefault = defaultControlPoint(ctx);
	if (!_hasCustomControlPoint) {
		_controlPoint = newDefault;
		return;
	}
	if (_state.referencePos == ctx.referencePos && _state.cursorPosition == ctx.cursorPosition) {
		return;
	}
	const glm::ivec3 oldDefault = _state.referencePos + (_state.cursorPosition - _state.referencePos) / 2;
	_controlPoint += newDefault - oldDefault;
}

bool LineBrush::wantBrushGizmo(const BrushContext &ctx) const {
	return _bezier && ((!_segments.empty() && _selectedSegment >= 0) || ctx.referencePos != ctx.cursorPosition);
}

void LineBrush::brushGizmoState(const BrushContext &ctx, BrushGizmoState &state) const {
	if (!wantBrushGizmo(ctx)) {
		state.operations = BrushGizmo_None;
		return;
	}
	const glm::ivec3 cp = controlPoint(ctx);
	state.matrix = glm::translate(glm::mat4(1.0f), glm::vec3(cp));
	state.operations = BrushGizmo_BezierControl | BrushGizmo_Line;
	state.snap = (float)ctx.gridResolution;
	state.localMode = false;

	glm::ivec3 start, end;
	if (_selectedSegment >= 0 && _selectedSegment < (int)_segments.size()) {
		start = _segments[_selectedSegment].start;
		end = _segments[_selectedSegment].end;
	} else {
		start = ctx.referencePos;
		end = ctx.cursorPosition;
	}
	state.positions[0] = glm::vec3(start) + 0.5f;
	state.positions[1] = glm::vec3(cp) + 0.5f;
	state.positions[2] = glm::vec3(end) + 0.5f;
	state.numPositions = 3;
}

bool LineBrush::applyBrushGizmo(BrushContext &ctx, const glm::mat4 &matrix, const glm::mat4 &deltaMatrix,
								uint32_t operation) {
	const glm::ivec3 newControlPoint = glm::ivec3(glm::round(glm::vec3(matrix[3])));
	if (newControlPoint == controlPoint(ctx)) {
		return false;
	}
	if (_selectedSegment >= 0 && _selectedSegment < (int)_segments.size()) {
		_segments[_selectedSegment].control = newControlPoint;
		markDirty();
		return true;
	}
	setControlPoint(newControlPoint);
	return true;
}

bool LineBrush::active() const {
	return _bezier ? hasPendingChanges() || Super::active() : Super::active();
}

void LineBrush::lockSegment(const BrushContext &ctx) {
	const glm::ivec3 segmentStart = _segments.empty() ? ctx.referencePos : _segments.back().end;
	const glm::ivec3 segmentEnd = ctx.cursorPosition;
	if (segmentStart == segmentEnd) {
		return;
	}
	const glm::ivec3 segmentControl =
		_hasCustomControlPoint ? _controlPoint : segmentStart + (segmentEnd - segmentStart) / 2;
	_segments.push_back({segmentStart, segmentEnd, segmentControl});
	_selectedSegment = (int)_segments.size() - 1;
	_hasCustomControlPoint = false;
	_controlPoint = glm::zero<glm::ivec3>();
}

void LineBrush::clearPendingSegments() {
	_segments.clear();
	_selectedSegment = -1;
}

bool LineBrush::selectSegment(int index) {
	if (index < 0 || index >= (int)_segments.size()) {
		return false;
	}
	if (_selectedSegment == index) {
		return true;
	}
	_selectedSegment = index;
	_hasCustomControlPoint = false;
	markDirty();
	return true;
}

} // namespace voxedit
