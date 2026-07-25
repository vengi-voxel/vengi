/**
 * @file
 */

#include "Brush.h"
#include "app/I18N.h"
#include "command/Command.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "voxedit-util/AxisUtil.h"
#include "voxel/Region.h"
#include <glm/common.hpp>

namespace voxedit {

glm::ivec3 parseGridResolution(const core::String &str) {
	core::DynamicArray<core::String> tokens;
	tokens.reserve(3);
	core::string::splitString(str, tokens, " :,\t");
	if (tokens.empty()) {
		return glm::ivec3(1);
	}
	if (tokens.size() == 1) {
		const int n = glm::clamp(tokens[0].toInt(), 1, 64);
		return glm::ivec3(n);
	}
	glm::ivec3 out(1);
	const size_t n = core_min(tokens.size(), (size_t)3);
	for (size_t i = 0; i < n; ++i) {
		out[(int)i] = glm::clamp(tokens[i].toInt(), 1, 64);
	}
	return out;
}

SceneModifiedFlags Brush::sceneModifiedFlags() const {
	return _sceneModifiedFlags;
}

void Brush::onSceneChange() {
	markDirty();
}

void Brush::reset() {
	_clampToVolume = false;
	_referencePosition = glm::ivec3(0);
	_mirrorAxis = math::Axis::None;
	_mirrorPos = glm::ivec3(0);
	markDirty();
}

void Brush::update(const BrushContext &ctx, double nowSeconds) {
	_referencePosition = ctx.referencePos;
}

void Brush::setClampToVolume(bool clampToVolume) {
	_clampToVolume = clampToVolume;
}

bool Brush::clampToVolume() const {
	return _clampToVolume;
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
	return true;
}

void Brush::endBrush(BrushContext &ctx) {
}

bool Brush::getMirrorBox(glm::ivec3 &mins, glm::ivec3 &maxs) const {
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
	// mirroraxisshapebrush, setshapebrushcenter, setshapebrushstroke, setshapebrushbox
	// mirroraxispaintbrush, setpaintbrushcenter, setpaintbrushstroke, setpaintbrushbox

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
		}).setHelp(_("Turn off mirroring"));
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

bool Brush::wantBrushGizmo(const BrushContext &ctx) const {
	return false;
}

void Brush::brushGizmoState(const BrushContext &ctx, BrushGizmoState &state) const {
}

bool Brush::applyBrushGizmo(BrushContext &ctx, const glm::mat4 &matrix,
							const glm::mat4 &deltaMatrix, uint32_t operation) {
	return false;
}

} // namespace voxedit
