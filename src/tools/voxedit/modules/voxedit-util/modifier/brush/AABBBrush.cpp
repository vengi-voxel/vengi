/**
 * @file
 */

#include "AABBBrush.h"
#include "app/I18N.h"
#include "command/Command.h"
#include "core/Log.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/SceneModifiedFlags.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxel/Face.h"
#include "voxel/Region.h"
#include "voxelutil/VolumeSelect.h"

namespace voxedit {

namespace {

SceneModifiedFlags strokeAccumFlags() {
	return SceneModifiedFlags::NoUndo & ~SceneModifiedFlags::ResetTrace;
}

} // namespace

AABBBrush::AABBBrush(BrushType type, ModifierType defaultModifier, ModifierType supportedModifiers)
	: Super(type, defaultModifier, supportedModifiers) {
}

void AABBBrush::construct() {
	Super::construct();
	// TODO: BRUSH: some aabb brushes don't support center or stroke mode (e.g. the plane brush)
	const core::String &cmdName = name().toLower() + "brush";
	command::Command::registerCommand("set" + cmdName + "center")
		.setHandler([this](const command::CommandArgs &args) {
			setCenterMode();
		}).setHelp(_("Grow the box from the first click as the center"));

	auto setBox = [this](const command::CommandArgs &) { setBoxMode(); };
	command::Command::registerCommand("set" + cmdName + "box")
		.setHandler(setBox)
		.setHelp(_("Click and drag to define a box of voxels"));
	// Backward-compatible alias for older keybindings / scripts
	command::Command::registerCommand("set" + cmdName + "aabb").setHandler(setBox).setHelp(_("Alias for box mode"));

	auto setStroke = [this](const command::CommandArgs &) { setStrokeMode(); };
	command::Command::registerCommand("set" + cmdName + "stroke")
		.setHandler(setStroke)
		.setHelp(_("Place voxels along the cursor while the action button is held"));
	command::Command::registerCommand("set" + cmdName + "single").setHandler(setStroke).setHelp(_("Alias for stroke mode"));

	auto setNoOverlap = [this](const command::CommandArgs &) { setStrokeNoOverlap(); };
	command::Command::registerCommand("set" + cmdName + "strokenooverlap")
		.setHandler(setNoOverlap)
		.setHelp(_("Like stroke mode, but do not replace the same voxel twice"));
	command::Command::registerCommand("set" + cmdName + "singlemove")
		.setHandler(setNoOverlap)
		.setHelp(_("Alias for stroke no-overlap mode"));
}

void AABBBrush::onSceneChange() {
	Super::onSceneChange();
	_secondPosValid = false;
	_boxMode = false;
	_aabbFace = voxel::FaceNames::Max;
	_strokeHasLastPos = false;
	_strokeActive = false;
	_strokeDirtyRegion = voxel::Region::InvalidRegion;
	_pendingUndoRegion = voxel::Region::InvalidRegion;
}

void AABBBrush::reset() {
	Super::reset();
	_secondPosValid = false;
	_boxMode = false;
	// Preserve _mode (AABB/Single/Center) - it is a user preference set via UI commands
	// (e.g., SelectBrush::setSelectMode(Paint) calls setSingleMode()). Resetting it here
	// causes a mismatch: the derived brush still shows its mode in the UI, but the
	// underlying AABBBrush reverts to AABB behavior after a brush type round-trip.
	_aabbFace = voxel::FaceNames::Max;
	_aabbFirstPos = glm::ivec3(0);
	_aabbSecondPos = glm::ivec3(0);
	_strokeHasLastPos = false;
	_strokeActive = false;
	_strokeDirtyRegion = voxel::Region::InvalidRegion;
	_pendingUndoRegion = voxel::Region::InvalidRegion;
}

glm::ivec3 AABBBrush::applyGridResolution(const glm::ivec3 &inPos, const glm::ivec3 &resolution) const {
	glm::ivec3 pos = inPos;
	if (resolution.x > 0 && pos.x % resolution.x != 0) {
		pos.x = (pos.x / resolution.x) * resolution.x;
	}
	if (resolution.y > 0 && pos.y % resolution.y != 0) {
		pos.y = (pos.y / resolution.y) * resolution.y;
	}
	if (resolution.z > 0 && pos.z % resolution.z != 0) {
		pos.z = (pos.z / resolution.z) * resolution.z;
	}
	return pos;
}

bool AABBBrush::needsAdditionalAction(const BrushContext &ctx) const {
	if (radius() > 0 || ctx.lockedAxis != math::Axis::None) {
		return false;
	}
	const voxel::Region &region = calcRegion(ctx);
	const glm::ivec3 &delta = region.getDimensionsInVoxels();
	int greater = 0;
	int equal = 0;
	for (int i = 0; i < 3; ++i) {
		if (delta[i] > ctx.gridResolution[i]) {
			++greater;
		} else if (delta[i] == ctx.gridResolution[i]) {
			++equal;
		}
	}
	// if two dimensions are spanning the plane already but one is not,
	// we need to span the third dimension by allowing the cursor to
	// still move
	return greater == 2 && equal == 1;
}

voxel::Region AABBBrush::extendRegionInOrthoMode(const voxel::Region &brushRegion, const voxel::Region &volumeRegion,
												 const BrushContext &ctx) const {
	if (ctx.fixedOrthoSideView) {
		if (radius() > 0) {
			// TODO: BRUSH
			return brushRegion;
		}
		glm::ivec3 mins = brushRegion.getLowerCorner();
		glm::ivec3 maxs = brushRegion.getUpperCorner();
		switch (ctx.cursorFace) {
		case voxel::FaceNames::PositiveX:
		case voxel::FaceNames::NegativeX:
			mins.x = volumeRegion.getLowerX();
			maxs.x = volumeRegion.getUpperX();
			break;
		case voxel::FaceNames::PositiveY:
		case voxel::FaceNames::NegativeY:
			mins.y = volumeRegion.getLowerY();
			maxs.y = volumeRegion.getUpperY();
			break;
		case voxel::FaceNames::PositiveZ:
		case voxel::FaceNames::NegativeZ:
			mins.z = volumeRegion.getLowerZ();
			maxs.z = volumeRegion.getUpperZ();
			break;
		case voxel::FaceNames::Max:
			return brushRegion;
		default:
			break;
		}
		Log::debug("extend region in fixed ortho side view: %s to mins: %i:%i:%i, maxs: %i:%i:%i, face: %i",
				   brushRegion.toString().c_str(), mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z, (int)ctx.cursorFace);
		return voxel::Region{mins, maxs};
	}
	return brushRegion;
}

void AABBBrush::generateMirrored(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
								 const BrushContext &ctx, const voxel::Region &region) {
	glm::ivec3 minsMirror = region.getLowerCorner();
	glm::ivec3 maxsMirror = region.getUpperCorner();
	if (!getMirrorBox(minsMirror, maxsMirror)) {
		generate(sceneGraph, wrapper, ctx, region);
	} else {
		Log::debug("Execute mirror action");
		const voxel::Region second(minsMirror, maxsMirror);
		if (voxel::intersects(region, second)) {
			generate(sceneGraph, wrapper, ctx, voxel::Region(region.getLowerCorner(), maxsMirror));
		} else {
			generate(sceneGraph, wrapper, ctx, region);
			generate(sceneGraph, wrapper, ctx, second);
		}
	}
}

bool AABBBrush::execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx) {
	setErrorReason("");

	// Path accumulation is only for a live mouse-drag stroke on the real volume.
	// Preview execute() must not advance _strokeLastPos / dirty accumulation.
	if (anyStrokeMode() && _strokeActive && !ctx.preview) {
		const glm::ivec3 end = currentCursorPosition(ctx);
		auto runDab = [&](const glm::ivec3 &center) {
			BrushContext dabCtx = ctx;
			dabCtx.cursorPosition = center;
			voxel::Region region = calcRegion(dabCtx);
			region = extendRegionInOrthoMode(region, wrapper.region(), dabCtx);
			generateMirrored(sceneGraph, wrapper, dabCtx, region);
		};

		if (_strokeHasLastPos && _strokeLastPos != end) {
			bool skipFirst = true;
			voxelutil::bresenham3d(_strokeLastPos, end, [&](const glm::ivec3 &p) {
				if (skipFirst) {
					skipFirst = false;
					return;
				}
				runDab(p);
			});
		} else {
			runDab(end);
		}
		_strokeLastPos = end;
		_strokeHasLastPos = true;

		const voxel::Region &dirty = wrapper.dirtyRegion();
		if (dirty.isValid()) {
			if (_strokeDirtyRegion.isValid()) {
				_strokeDirtyRegion.accumulate(dirty);
			} else {
				_strokeDirtyRegion = dirty;
			}
		}
		return true;
	}

	voxel::Region region = calcRegion(ctx);
	region = extendRegionInOrthoMode(region, wrapper.region(), ctx);
	generateMirrored(sceneGraph, wrapper, ctx, region);
	return true;
}

voxel::Region AABBBrush::consumePendingUndoRegion() {
	const voxel::Region region = _pendingUndoRegion;
	_pendingUndoRegion = voxel::Region::InvalidRegion;
	return region;
}

glm::ivec3 AABBBrush::currentCursorPosition(const BrushContext &ctx) const {
	glm::ivec3 pos = ctx.cursorPosition;
	if (_secondPosValid) {
		if (radius() > 0) {
			return _aabbSecondPos;
		}
		const math::Axis axis = voxel::faceToAxis(_aabbFace);
		if (axis != math::Axis::None) {
			const int idx = math::getIndexForAxis(axis);
			pos[(idx + 1) % 3] = _aabbSecondPos[(idx + 1) % 3];
			pos[(idx + 2) % 3] = _aabbSecondPos[(idx + 2) % 3];
		}
	}
	return pos;
}

bool AABBBrush::wantBox() const {
	return !anyStrokeMode();
}

bool AABBBrush::beginBrush(const BrushContext &ctx) {
	// Stroke mode must always be able to start, even if a previous box span was left active.
	if (anyStrokeMode()) {
		_boxMode = false;
	} else if (_boxMode) {
		return false;
	}

	// the order here matters - don't change _boxMode earlier here
	_aabbFirstPos = applyGridResolution(ctx.cursorPosition, ctx.gridResolution);
	_lastCursorPos = ctx.cursorPosition;
	_secondPosValid = false;
	_boxMode = wantBox();
	_aabbFace = ctx.cursorFace;
	_strokeHasLastPos = false;
	_strokeDirtyRegion = voxel::Region::InvalidRegion;
	_pendingUndoRegion = voxel::Region::InvalidRegion;
	if (anyStrokeMode()) {
		_strokeActive = true;
		_sceneModifiedFlags = strokeAccumFlags();
	}
	return true;
}

void AABBBrush::update(const BrushContext &ctx, double nowSeconds) {
	Super::update(ctx, nowSeconds);

	if (ctx.cursorPosition != _lastCursorPos) {
		_lastCursorPos = ctx.cursorPosition;
		// we have to update the preview each time we move the cursor if the brush
		// is either spanning an aabb or has a radius set in single mode
		if (_boxMode || radius() > 0) {
			markDirty();
		}
	}
}

bool AABBBrush::active() const {
	return _boxMode || anyStrokeMode();
}

bool AABBBrush::aborted(const BrushContext &ctx) const {
	// Stroke path painting does not require a valid hit face.
	if (anyStrokeMode()) {
		return false;
	}
	return _aabbFace == voxel::FaceNames::Max && ctx.lockedAxis == math::Axis::None;
}

void AABBBrush::step(const BrushContext &ctx) {
	if (!_boxMode || radius() > 0 || ctx.lockedAxis != math::Axis::None) {
		return;
	}
	glm::ivec3 pos = currentCursorPosition(ctx);
	_aabbSecondPos = pos;
	if (!_secondPosValid || pos != _aabbSecondPos) {
		markDirty();
	}
	_secondPosValid = true;
}

void AABBBrush::endBrush(BrushContext &ctx) {
	if (anyStrokeMode() && _strokeDirtyRegion.isValid()) {
		_pendingUndoRegion = _strokeDirtyRegion;
	}
	_strokeDirtyRegion = voxel::Region::InvalidRegion;
	_strokeHasLastPos = false;
	_strokeActive = false;
	_secondPosValid = false;
	_boxMode = false;
	_aabbFace = voxel::FaceNames::Max;
}

bool AABBBrush::isMode(uint32_t mode) const {
	return _mode == mode;
}

void AABBBrush::setMode(uint32_t mode) {
	_mode = mode;
	if (anyStrokeMode()) {
		_boxMode = false;
		_secondPosValid = false;
		_sceneModifiedFlags = strokeAccumFlags();
	} else {
		_sceneModifiedFlags = SceneModifiedFlags::All;
	}
}

void AABBBrush::setRadius(int radius) {
	_radius = glm::abs(radius);
	markDirty();
}

voxel::Region AABBBrush::calcRegion(const BrushContext &ctx) const {
	const glm::ivec3 &pos = currentCursorPosition(ctx);
	if (!anyStrokeMode() && centerMode()) {
		const glm::ivec3 &first = applyGridResolution(_aabbFirstPos, ctx.gridResolution);
		const glm::ivec3 &delta = glm::abs(pos - first);
		return voxel::Region(first - delta, first + delta);
	}
	const glm::ivec3 &first = anyStrokeMode() ? pos : applyGridResolution(_aabbFirstPos, ctx.gridResolution);
	const int rad = radius();
	if (rad > 0) {
		// TODO: BRUSH: _radius should only go into one direction (see BrushContext::_cursorFace) (only paint the surface)
		return voxel::Region(first - rad, first + rad);
	}

	const glm::ivec3 &size = ctx.gridResolution;
	const glm::ivec3 &mins = (glm::min)(first, pos);
	const glm::ivec3 &maxs = (glm::max)(first, pos) + (size - 1);
	return voxel::Region(mins, maxs);
}

} // namespace voxedit
