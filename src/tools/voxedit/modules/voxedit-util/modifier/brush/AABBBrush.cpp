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

void AABBBrush::syncInput(const Brush &input) {
	if (const AABBBrush *source = input.aabb()) {
		_boxGesture = source->_boxGesture;
		_editedRegion = source->_editedRegion;
	}
}

bool AABBBrush::setRegion(const voxel::Region &region) {
	if (!region.isValid() || anyStrokeMode() || !_boxGesture.active()) {
		return false;
	}
	_editedRegion = region;
	markDirty();
	return true;
}

void AABBBrush::handles(const BrushContext &ctx, core::DynamicArray<BrushGizmoState> &output) const {
	Super::handles(ctx, output);
	if (!output.empty() || !_boxGesture.active() || anyStrokeMode()) {
		return;
	}
	const voxel::Region region = calcRegion(ctx);
	if (!region.isValid()) {
		return;
	}
	BrushGizmoState state;
	state.operations = BrushGizmo_Translate | BrushGizmo_Bounds;
	state.hasBounds = true;
	state.matrix[3] = glm::vec4(glm::vec3(region.getLowerCorner()), 1.0f);
	const glm::ivec3 size = region.getDimensionsInVoxels();
	for (int i = 0; i < 3; ++i) {
		state.bounds[i + 3] = (float)size[i];
		state.snap[i] = (float)ctx.gridResolution[i];
	}
	output.push_back(state);
}

bool AABBBrush::editHandle(uint32_t handleId, BrushContext &ctx, const glm::mat4 &matrix, const glm::mat4 &, uint32_t) {
	const voxel::Region previous = calcRegion(ctx);
	const glm::vec3 first = glm::vec3(matrix * glm::vec4(0, 0, 0, 1));
	const glm::vec3 second = glm::vec3(matrix * glm::vec4(glm::vec3(previous.getDimensionsInVoxels()), 1));
	const glm::ivec3 lower(glm::round(glm::min(first, second)));
	const glm::ivec3 upper = glm::max(lower, glm::ivec3(glm::round(glm::max(first, second))) - 1);
	return setRegion(voxel::Region(lower, upper));
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

	auto setStroke = [this](const command::CommandArgs &) { setStrokeMode(); };
	command::Command::registerCommand("set" + cmdName + "stroke")
		.setHandler(setStroke)
		.setHelp(_("Place voxels along the cursor while the action button is held"));

	auto setNoOverlap = [this](const command::CommandArgs &) { setStrokeNoOverlap(); };
	command::Command::registerCommand("set" + cmdName + "strokenooverlap")
		.setHandler(setNoOverlap)
		.setHelp(_("Like stroke mode, but do not replace the same voxel twice"));
}

void AABBBrush::onSceneChange() {
	Super::onSceneChange();
	_boxGesture.end();
}

void AABBBrush::reset() {
	Super::reset();
}

void AABBBrush::resetRuntime() {
	Super::resetRuntime();
	// Gesture state is transient; preserve the mode and radius preferences.
	_boxGesture.reset();
	_editedRegion = voxel::Region::InvalidRegion;
}

glm::ivec3 AABBBrush::applyGridResolution(const glm::ivec3 &inPos, const glm::ivec3 &resolution) const {
	return BoxGesture::snapToGrid(inPos, resolution);
}

bool AABBBrush::needsAdditionalAction(const BrushContext &ctx) const {
	return radius() == 0 && ctx.lockedAxis == math::Axis::None && BoxGesture::needsDepth(ctx, calcRegion(ctx));
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

bool AABBBrush::executeSample(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx) {
	setErrorReason("");
	voxel::Region region = calcRegion(ctx);
	region = extendRegionInOrthoMode(region,
									 ctx.targetVolumeRegion.isValid() ? ctx.targetVolumeRegion : wrapper.region(), ctx);
	generateMirrored(sceneGraph, wrapper, ctx, region);
	return errorReason().empty();
}

bool AABBBrush::execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx) {
	return executeSample(sceneGraph, wrapper, ctx);
}

glm::ivec3 AABBBrush::currentCursorPosition(const BrushContext &ctx) const {
	return _boxGesture.cursorPosition(ctx, radius());
}

bool AABBBrush::wantBox() const {
	return !anyStrokeMode();
}

bool AABBBrush::beginBrush(const BrushContext &ctx) {
	// A stroke can start even if a previous box span was left active.
	if (!anyStrokeMode() && _boxGesture.active()) {
		return false;
	}
	_editedRegion = voxel::Region::InvalidRegion;
	_boxGesture.begin(ctx, wantBox());
	_lastCursorPos = ctx.cursorPosition;

	return true;
}

void AABBBrush::update(const BrushContext &ctx, double nowSeconds) {
	Super::update(ctx, nowSeconds);

	if (ctx.cursorPosition != _lastCursorPos) {
		_lastCursorPos = ctx.cursorPosition;
		// we have to update the preview each time we move the cursor if the brush
		// is either spanning an aabb or follows the cursor in stroke mode.
		// Single-voxel shape stroke uses the red face cursor instead.
		if (_boxGesture.active() || (anyStrokeMode() && !preferFaceCursorPreview())) {
			markDirty();
		}
	}
}

bool AABBBrush::active() const {
	return _boxGesture.active() || anyStrokeMode();
}

bool AABBBrush::aborted(const BrushContext &ctx) const {
	// Stroke path painting does not require a valid hit face.
	if (anyStrokeMode()) {
		return false;
	}
	return _boxGesture.face() == voxel::FaceNames::Max && ctx.lockedAxis == math::Axis::None;
}

void AABBBrush::step(const BrushContext &ctx) {
	if (_boxGesture.step(ctx, radius())) {
		markDirty();
	}
}

void AABBBrush::endBrush(BrushContext &ctx) {
	_boxGesture.end();
	_editedRegion = voxel::Region::InvalidRegion;
}

bool AABBBrush::isMode(uint32_t mode) const {
	return _mode == mode;
}

void AABBBrush::setMode(uint32_t mode) {
	if (_mode != mode) {
		markSettingsChanged();
	}
	_mode = mode;
	if (anyStrokeMode()) {
		_boxGesture.stopSpanning();
	}
}

void AABBBrush::setRadius(int radius) {
	if (_radius != glm::abs(radius)) {
		markSettingsChanged();
	}
	_radius = glm::abs(radius);
	markDirty();
}

voxel::Region AABBBrush::calcRegion(const BrushContext &ctx) const {
	if (ctx.evaluatedRegion.isValid()) {
		return ctx.evaluatedRegion;
	}
	if (_editedRegion.isValid()) {
		return _editedRegion;
	}
	if (anyStrokeMode()) {
		return StrokeGesture::region(currentCursorPosition(ctx), radius(), ctx.gridResolution);
	}
	return _boxGesture.region(ctx, centerMode());
}

} // namespace voxedit
