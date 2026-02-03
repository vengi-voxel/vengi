/**
 * @file
 */

#include "Brush.h"
#include "app/I18N.h"
#include "command/Command.h"
#include "core/Log.h"
#include "voxedit-util/AxisUtil.h"
#include "voxel/Region.h"

namespace voxedit {

SceneModifiedFlags Brush::sceneModifiedFlags() const {
	return _sceneModifiedFlags;
}

void Brush::reset() {
	_brushClamping = false;
	_referencePosition = glm::ivec3(0);
	_mirrorAxis = math::Axis::None;
	_mirrorPos = glm::ivec3(0);
	markDirty();
}

void Brush::update(const BrushContext &ctx, double nowSeconds) {
	_referencePosition = ctx.referencePos;
}

void Brush::setBrushClamping(bool brushClamping) {
	_brushClamping = brushClamping;
}

bool Brush::brushClamping() const {
	return _brushClamping;
}

void Brush::toggleMirrorAxis(math::Axis axis, const glm::ivec3 &mirrorPos) {
	if (_mirrorAxis == axis) {
		setMirrorAxis(math::Axis::None, mirrorPos);
	} else {
		setMirrorAxis(axis, mirrorPos);
	}
}

bool Brush::setMirrorAxis(math::Axis axis, const glm::ivec3 &mirrorPos) {
	if (_mirrorAxis == axis) {
		if (_mirrorPos != mirrorPos) {
			_mirrorPos = mirrorPos;
			return true;
		}
		return false;
	}
	_mirrorPos = mirrorPos;
	_mirrorAxis = axis;
	markDirty();
	return true;
}

bool Brush::beginBrush(const BrushContext &ctx) {
	return false;
}

void Brush::preExecute(const BrushContext &ctx, const voxel::RawVolume *volume) {
}

bool Brush::execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx) {
	voxel::Region region = calcRegion(ctx);
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

void Brush::endBrush(BrushContext &ctx) {
}

bool Brush::getMirrorAABB(glm::ivec3 &mins, glm::ivec3 &maxs) const {
	math::Axis mirrorAxis = _mirrorAxis;
	if (mirrorAxis == math::Axis::None) {
		return false;
	}
	const int index = getIndexForMirrorAxis(mirrorAxis);
	int deltaMaxs = _mirrorPos[index] - maxs[index] - 1;
	deltaMaxs *= 2;
	deltaMaxs += (maxs[index] - mins[index] + 1);
	mins[index] += deltaMaxs;
	maxs[index] += deltaMaxs;
	return true;
}

ModifierType Brush::modifierType(ModifierType type) const {
	ModifierType newType = type & _supportedModifiers;
	if (newType == ModifierType::None) {
		newType = _defaultModifier;
	}
	return newType;
}

/**
 * @brief Determine whether the brush should get rendered
 */
bool Brush::active() const {
	return true;
}

void Brush::construct() {
	// mirroraxisshapebrush, setshapebrushcenter, setshapebrushsingle, setshapebrushaabb
	// mirroraxispaintbrush, setpaintbrushcenter, setpaintbrushsingle, setpaintbrushaabb

	const core::String &cmdName = name().toLower() + "brush";
	command::Command::registerCommand("mirroraxis" + cmdName + "x")
		.setHandler([&](const command::CommandArgs &args) {
			toggleMirrorAxis(math::Axis::X, _referencePosition);
		}).setHelp(_("Mirror along the x axis at the reference position"));

	command::Command::registerCommand("mirroraxis" + cmdName + "y")
		.setHandler([&](const command::CommandArgs &args) {
			toggleMirrorAxis(math::Axis::Y, _referencePosition);
		}).setHelp(_("Mirror along the y axis at the reference position"));

	command::Command::registerCommand("mirroraxis" + cmdName + "z")
		.setHandler([&](const command::CommandArgs &args) {
			toggleMirrorAxis(math::Axis::Z, _referencePosition);
		}).setHelp(_("Mirror along the z axis at the reference position"));

	command::Command::registerCommand("mirroraxis" + cmdName + "none")
		.setHandler([&](const command::CommandArgs &args) {
			setMirrorAxis(math::Axis::None, _referencePosition);
		}).setHelp(_("Disable mirror axis"));
}

bool Brush::init() {
	return true;
}

void Brush::shutdown() {
	const core::String &cmdName = name().toLower() + "brush";
	command::Command::unregisterCommand("mirroraxis" + cmdName + "x");
	command::Command::unregisterCommand("mirroraxis" + cmdName + "y");
	command::Command::unregisterCommand("mirroraxis" + cmdName + "z");
	command::Command::unregisterCommand("mirroraxis" + cmdName + "none");
}

} // namespace voxedit
