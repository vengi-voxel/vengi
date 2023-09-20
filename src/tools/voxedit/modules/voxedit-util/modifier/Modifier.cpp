/**
 * @file
 */

#include "Modifier.h"
#include "../AxisUtil.h"
#include "../SceneManager.h"
#include "command/Command.h"
#include "core/Color.h"
#include "core/StringUtil.h"
#include "math/Axis.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "ui/dearimgui/imgui_internal.h"
#include "voxedit-util/modifier/Selection.h"
#include "voxedit-util/modifier/brush/ShapeBrush.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelgenerator/ShapeGenerator.h"
#include "voxelutil/AStarPathfinder.h"
#include "voxelutil/VoxelUtil.h"

namespace voxedit {

Modifier::Modifier() : _deleteExecuteButton(ModifierType::Erase) {
}

void Modifier::construct() {
	command::Command::registerActionButton("actionexecute", _actionExecuteButton, "Execute the modifier action");
	command::Command::registerActionButton("actionexecutedelete", _deleteExecuteButton,
										   "Execute the modifier action in delete mode");

	command::Command::registerCommand("actionselect", [&](const command::CmdArgs &args) {
		setModifierType(ModifierType::Select);
	}).setHelp("Change the modifier type to 'select'");

	command::Command::registerCommand("actioncolorpicker", [&](const command::CmdArgs &args) {
		setModifierType(ModifierType::ColorPicker);
	}).setHelp("Change the modifier type to 'color picker'");

	command::Command::registerCommand("actionpath", [&](const command::CmdArgs &args) {
		setModifierType(ModifierType::Path);
	}).setHelp("Change the modifier type to 'path'");

	command::Command::registerCommand("actionline", [&](const command::CmdArgs &args) {
		setModifierType(ModifierType::Line);
	}).setHelp("Change the modifier type to 'line'");

	command::Command::registerCommand("actionerase", [&](const command::CmdArgs &args) {
		setModifierType(ModifierType::Erase);
	}).setHelp("Change the modifier type to 'erase'");

	command::Command::registerCommand("actionplace", [&](const command::CmdArgs &args) {
		setModifierType(ModifierType::Place);
	}).setHelp("Change the modifier type to 'place'");

	command::Command::registerCommand("actionpaint", [&](const command::CmdArgs &args) {
		setModifierType(ModifierType::Paint);
	}).setHelp("Change the modifier type to 'paint'");

	command::Command::registerCommand("actionoverride", [&](const command::CmdArgs &args) {
		setModifierType(ModifierType::Place | ModifierType::Erase);
	}).setHelp("Change the modifier type to 'override'");

	_planeBrush.construct();
	_scriptBrush.construct();
	_shapeBrush.construct();
	_stampBrush.construct();
}

bool Modifier::init() {
	if (!_planeBrush.init()) {
		return false;
	}
	if (!_scriptBrush.init()) {
		return false;
	}
	if (!_shapeBrush.init()) {
		return false;
	}
	if (!_stampBrush.init()) {
		return false;
	}
	return true;
}

void Modifier::shutdown() {
	reset();
	_planeBrush.shutdown();
	_scriptBrush.shutdown();
	_shapeBrush.shutdown();
	_stampBrush.shutdown();
}

void Modifier::update(double nowSeconds) {
	switch (_brushType) {
	case BrushType::Shape:
		if (_shapeBrush.singleMode()) {
			if (_actionExecuteButton.pressed() && nowSeconds >= _nextSingleExecution) {
				_actionExecuteButton.execute(true);
				_nextSingleExecution = nowSeconds + 0.1;
			}
		}
		break;
	case BrushType::Stamp:
		if (_stampBrush.continuousMode()) {
			if (_actionExecuteButton.pressed() && nowSeconds >= _nextSingleExecution) {
				_actionExecuteButton.execute(true);
				_nextSingleExecution = nowSeconds + 0.1;
			}
		}
		break;
	default:
		break;
	}
	activeBrush()->update(_brushContext, nowSeconds);
}

void Modifier::reset() {
	unselect();
	_brushContext.gridResolution = 1;
	_brushContext.cursorPosition = glm::ivec3(0);
	_brushContext.cursorFace = voxel::FaceNames::Max;

	_modifierType = ModifierType::Place;
	_planeBrush.reset();
	_scriptBrush.reset();
	_shapeBrush.reset();
	_stampBrush.reset();
	setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 0));
}

bool Modifier::start() {
	if (ShapeBrush *brush = activeShapeBrush()) {
		return brush->start(_brushContext);
	}
	return true;
}

void Modifier::executeAdditionalAction() {
	if (ShapeBrush *brush = activeShapeBrush()) {
		brush->step(_brushContext);
	}
}

void Modifier::invert(const voxel::Region &region) {
	if (!region.isValid()) {
		return;
	}
	if (!_selectionValid) {
		select(region.getLowerCorner(), region.getUpperCorner());
	} else {
		// TODO:
	}
}

void Modifier::unselect() {
	_selections.clear();
	_selectionValid = false;
}

bool Modifier::select(const glm::ivec3 &mins, const glm::ivec3 &maxs) {
	if (_locked) {
		return false;
	}
	const Selection sel{mins, maxs};
	if (!sel.isValid()) {
		return false;
	}
	_selectionValid = true;
	for (size_t i = 0; i < _selections.size();) {
		Selection &s = _selections[i];
		if (sel.containsRegion(s)) {
			_selections.erase(i);
		} else if (voxel::intersects(sel, s)) {
			// TODO: slice
			++i;
		} else {
			++i;
		}
	}
	_selections.push_back(sel);
	return true;
}

void Modifier::setReferencePosition(const glm::ivec3 &pos) {
	_brushContext.referencePos = pos;
}

bool Modifier::needsFurtherAction() {
	if (const ShapeBrush *brush = activeShapeBrush()) {
		return brush->needsFurtherAction(_brushContext);
	}
	return false;
}

glm::ivec3 Modifier::currentCursorPosition() {
	if (ShapeBrush *brush = activeShapeBrush()) {
		return brush->currentCursorPosition(_brushContext.cursorPosition);
	}
	return _brushContext.cursorPosition;
}

voxel::Region Modifier::calcBrushRegion() {
	if (ShapeBrush *brush = activeShapeBrush()) {
		return brush->calcRegion(_brushContext);
	}
	return voxel::Region::InvalidRegion;
}

glm::ivec3 Modifier::calcShapeBrushRegionSize() {
	return calcBrushRegion().getDimensionsInVoxels();
}

voxel::RawVolumeWrapper Modifier::createRawVolumeWrapper(voxel::RawVolume *volume) const {
	voxel::Region region = volume->region();
	if (_selectionValid) {
		voxel::Region srcRegion = accumulate(_selections);
		srcRegion.cropTo(region);
		return voxel::RawVolumeWrapper(volume, srcRegion);
	}
	return voxel::RawVolumeWrapper(volume, region);
}

void Modifier::setHitCursorVoxel(const voxel::Voxel &voxel) {
	_brushContext.hitCursorVoxel = voxel;
}

void Modifier::setVoxelAtCursor(const voxel::Voxel &voxel) {
	_brushContext.voxelAtCursor = voxel;
}

void Modifier::lock() {
	_locked = true;
}

void Modifier::unlock() {
	_locked = false;
}

bool Modifier::lineModifier(voxel::RawVolume *volume, const Callback &callback) {
	voxel::RawVolumeWrapper wrapper = createRawVolumeWrapper(volume);
	const glm::ivec3 &start = referencePosition();
	const glm::ivec3 &end = cursorPosition();
	voxel::Voxel voxel = cursorVoxel();
	if (isMode(ModifierType::Erase)) {
		voxel = voxel::createVoxel(voxel::VoxelType::Air, 0);
	}
	voxelutil::RaycastResult result = voxelutil::raycastWithEndpoints(&wrapper, start, end, [=](auto &sampler) {
		const bool air = voxel::isAir(sampler.voxel().getMaterial());
		if ((!isMode(ModifierType::Erase) && !isMode(ModifierType::Paint)) && !air) {
			return true;
		}
		sampler.setVoxel(voxel);
		return true;
	});
	Log::debug("result: %i", (int)result);
	const voxel::Region &modifiedRegion = wrapper.dirtyRegion();
	if (modifiedRegion.isValid()) {
		callback(modifiedRegion, _modifierType, true);
	}
	return true;
}

bool Modifier::pathModifier(voxel::RawVolume *volume, const Callback &callback) {
	core::List<glm::ivec3> listResult(4096);
	const glm::ivec3 &start = referencePosition();
	const glm::ivec3 &end = cursorPosition();
	voxelutil::AStarPathfinderParams<voxel::RawVolume> params(
		volume, start, end, &listResult,
		[=](const voxel::RawVolume *vol, const glm::ivec3 &pos) {
			if (voxel::isBlocked(vol->voxel(pos).getMaterial())) {
				return false;
			}
			return voxelutil::isTouching(vol, pos);
		},
		4.0f, 10000, voxelutil::Connectivity::EighteenConnected);
	voxelutil::AStarPathfinder pathfinder(params);
	if (!pathfinder.execute()) {
		Log::warn("Failed to execute pathfinder - is the reference position correctly placed on another voxel?");
		return false;
	}
	voxel::RawVolumeWrapper wrapper = createRawVolumeWrapper(volume);
	for (const glm::ivec3 &p : listResult) {
		wrapper.setVoxel(p, cursorVoxel());
	}
	const voxel::Region &modifiedRegion = wrapper.dirtyRegion();
	if (modifiedRegion.isValid()) {
		callback(modifiedRegion, _modifierType, true);
	}
	return true;
}

bool Modifier::execute(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, const Callback &callback) {
	if (_locked) {
		return false;
	}
	if (aborted()) {
		return false;
	}
	if (isMode(ModifierType::Select)) {
		const voxel::Region a = calcBrushRegion(); // TODO: only works for shape brush
		Log::debug("select mode");
		select(a.getLowerCorner(), a.getUpperCorner());
		if (_selectionValid) {
			callback(accumulate(_selections), _modifierType, false);
		}
		return true;
	}

	voxel::RawVolume *volume = node.volume();
	if (volume == nullptr) {
		Log::debug("No volume given - can't perform action");
		return true;
	}

	if (isMode(ModifierType::ColorPicker)) {
		setCursorVoxel(hitCursorVoxel());
		return true;
	}
	if (isMode(ModifierType::Line)) {
		return lineModifier(volume, callback);
	}
	if (isMode(ModifierType::Path)) {
		return pathModifier(volume, callback);
	}

	runModifier(sceneGraph, node, _modifierType, _brushContext.cursorVoxel, callback);

	return true;
}

bool Modifier::runModifier(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
						   ModifierType modifierType, const voxel::Voxel &voxel, const Callback &callback) {
	ModifierVolumeWrapper wrapper(node, modifierType, _selections);
	voxel::Voxel prevVoxel = _brushContext.cursorVoxel;
	_brushContext.cursorVoxel = voxel;
	activeBrush()->execute(sceneGraph, wrapper, _brushContext);
	const voxel::Region &modifiedRegion = wrapper.dirtyRegion();
	if (modifiedRegion.isValid()) {
		voxel::logRegion("Dirty region", modifiedRegion);
		callback(modifiedRegion, _modifierType, true);
	}
	_brushContext.cursorVoxel = prevVoxel;
	return true;
}

Brush *Modifier::activeBrush() {
	switch (_brushType) {
	case BrushType::Plane:
		return &_planeBrush;
	case BrushType::Script:
		return &_scriptBrush;
	case BrushType::Shape:
		return &_shapeBrush;
	case BrushType::Stamp:
		return &_stampBrush;
	case BrushType::Max:
		break;
	}
	return nullptr;
}

ShapeBrush *Modifier::activeShapeBrush() {
	if (_brushType == BrushType::Shape) {
		return &_shapeBrush;
	}
	return nullptr;
}

const ShapeBrush *Modifier::activeShapeBrush() const {
	if (_brushType == BrushType::Shape) {
		return &_shapeBrush;
	}
	return nullptr;
}

void Modifier::stop() {
	if (ShapeBrush *brush = activeShapeBrush()) {
		brush->stop(_brushContext);
	}
}

bool Modifier::modifierTypeRequiresExistingVoxel() const {
	return isMode(ModifierType::Erase) || isMode(ModifierType::Paint) || isMode(ModifierType::Select);
}

void Modifier::setBrushType(BrushType type) {
	_brushType = type;
}

void Modifier::setGridResolution(int gridSize) {
	_brushContext.gridResolution = core_max(1, gridSize);
}

void Modifier::setModifierType(ModifierType type) {
	_modifierType = type;
}

} // namespace voxedit
