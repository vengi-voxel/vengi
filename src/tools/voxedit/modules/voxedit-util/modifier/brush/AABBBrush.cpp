/**
 * @file
 */

#include "AABBBrush.h"
#include "app/I18N.h"
#include "command/Command.h"
#include "core/Log.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxel/Face.h"
#include "voxel/Region.h"

namespace voxedit {

AABBBrush::AABBBrush(BrushType type, ModifierType defaultModifier, ModifierType supportedModifiers)
	: Super(type, defaultModifier, supportedModifiers) {
}

void AABBBrush::construct() {
	Super::construct();
	// TODO: BRUSH: some aabb brushes don't support center or single mode (e.g. the plane brush)
	const core::String &cmdName = name().toLower() + "brush";
	command::Command::registerCommand("set" + cmdName + "center")
		.setHandler([this](const command::CommandArgs &args) {
			setCenterMode();
		}).setHelp(_("Set center plane building"));

	command::Command::registerCommand("set" + cmdName + "aabb")
		.setHandler([this](const command::CommandArgs &args) {
			setAABBMode();
		}).setHelp(_("Set default aabb voxel building mode"));

	command::Command::registerCommand("set" + cmdName + "single")
		.setHandler([this](const command::CommandArgs &args) {
			setSingleMode();
		}).setHelp(_("Set single voxel building mode - continue setting voxels until you release the action button"));

	command::Command::registerCommand("set" + cmdName + "singlemove")
		.setHandler([this](const command::CommandArgs &args) {
			setSingleModeMove();
		}).setHelp(_("Set single voxel building mode - continue setting voxels until you release the action button - but don't overwrite the last voxel"));
}

void AABBBrush::reset() {
	Super::reset();
	_secondPosValid = false;
	_aabbMode = false;
	_mode = 0u;
	_aabbFace = voxel::FaceNames::Max;
	_aabbFirstPos = glm::ivec3(0);
	_aabbSecondPos = glm::ivec3(0);
}

glm::ivec3 AABBBrush::applyGridResolution(const glm::ivec3 &inPos, int resolution) const {
	glm::ivec3 pos = inPos;
	if (pos.x % resolution != 0) {
		pos.x = (pos.x / resolution) * resolution;
	}
	if (pos.y % resolution != 0) {
		pos.y = (pos.y / resolution) * resolution;
	}
	if (pos.z % resolution != 0) {
		pos.z = (pos.z / resolution) * resolution;
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
		if (delta[i] > ctx.gridResolution) {
			++greater;
		} else if (delta[i] == ctx.gridResolution) {
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

bool AABBBrush::execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx) {
	setErrorReason("");
	voxel::Region region = calcRegion(ctx);
	region = extendRegionInOrthoMode(region, wrapper.region(), ctx);
	glm::ivec3 minsMirror = region.getLowerCorner();
	glm::ivec3 maxsMirror = region.getUpperCorner();
	if (!getMirrorAABB(minsMirror, maxsMirror)) {
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
	return true;
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

bool AABBBrush::wantAABB() const {
	return !anySingleMode();
}

bool AABBBrush::beginBrush(const BrushContext &ctx) {
	if (_aabbMode) {
		return false;
	}

	// the order here matters - don't change _aabbMode earlier here
	_aabbFirstPos = applyGridResolution(ctx.cursorPosition, ctx.gridResolution);
	_lastCursorPos = ctx.cursorPosition;
	_secondPosValid = false;
	_aabbMode = wantAABB();
	_aabbFace = ctx.cursorFace;
	return true;
}

void AABBBrush::update(const BrushContext &ctx, double nowSeconds) {
	Super::update(ctx, nowSeconds);

	if (ctx.cursorPosition != _lastCursorPos) {
		_lastCursorPos = ctx.cursorPosition;
		// we have to update the preview each time we move the cursor if the brush
		// is either spanning an aabb or has a radius set in single mode
		if (_aabbMode || radius() > 0) {
			markDirty();
		}
	}
}

bool AABBBrush::active() const {
	return _aabbMode || anySingleMode();
}

bool AABBBrush::aborted(const BrushContext &ctx) const {
	return _aabbFace == voxel::FaceNames::Max && ctx.lockedAxis == math::Axis::None;
}

void AABBBrush::step(const BrushContext &ctx) {
	if (!_aabbMode || radius() > 0 || ctx.lockedAxis != math::Axis::None) {
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
	_secondPosValid = false;
	_aabbMode = false;
	_aabbFace = voxel::FaceNames::Max;
}

bool AABBBrush::isMode(uint32_t mode) const {
	return _mode == mode;
}

void AABBBrush::setMode(uint32_t mode) {
	_mode = mode;
	if (singleModeMove()) {
		_sceneModifiedFlags = SceneModifiedFlags::NoResetTrace;
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
	if (!anySingleMode() && centerMode()) {
		const glm::ivec3 &first = applyGridResolution(_aabbFirstPos, ctx.gridResolution);
		const glm::ivec3 &delta = glm::abs(pos - first);
		return voxel::Region(first - delta, first + delta);
	}
	const glm::ivec3 &first = anySingleMode() ? pos : applyGridResolution(_aabbFirstPos, ctx.gridResolution);
	const int rad = radius();
	if (rad > 0) {
		// TODO: BRUSH: _radius should only go into one direction (see BrushContext::_cursorFace) (only paint the surface)
		return voxel::Region(first - rad, first + rad);
	}

	const int size = ctx.gridResolution;
	const glm::ivec3 &mins = (glm::min)(first, pos);
	const glm::ivec3 &maxs = (glm::max)(first, pos) + (size - 1);
	return voxel::Region(mins, maxs);
}

} // namespace voxedit
