/**
 * @file
 */

#include "Modifier.h"
#include "../SceneManager.h"
#include "command/Command.h"
#include "core/Log.h"
#include "math/Axis.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/Selection.h"
#include "voxedit-util/modifier/brush/ShapeBrush.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelutil/VoxelUtil.h"

namespace voxedit {

Modifier::Modifier() : _deleteExecuteButton(ModifierType::Erase) {
	_brushes.push_back(&_planeBrush);
	_brushes.push_back(&_shapeBrush);
	_brushes.push_back(&_stampBrush);
	_brushes.push_back(&_lineBrush);
	_brushes.push_back(&_pathBrush);
	core_assert(_brushes.size() == (int)BrushType::Max - 1);
}

void Modifier::construct() {
	command::Command::registerActionButton("actionexecute", _actionExecuteButton, "Execute the modifier action");
	command::Command::registerActionButton("actionexecutedelete", _deleteExecuteButton,
										   "Execute the modifier action in delete mode");

	command::Command::registerCommand("resizetoselection", [&](const command::CmdArgs &args) {
		const voxel::Region &region = accumulate(_selections);
		sceneMgr().resize(sceneMgr().sceneGraph().activeNode(), region);
	}).setHelp("Resize the volume to the current selection");

	command::Command::registerCommand("actionselect", [&](const command::CmdArgs &args) {
		setModifierType(ModifierType::Select);
	}).setHelp("Change the modifier type to 'select'");

	command::Command::registerCommand("actioncolorpicker", [&](const command::CmdArgs &args) {
		setModifierType(ModifierType::ColorPicker);
	}).setHelp("Change the modifier type to 'color picker'");

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

	for (const Brush *b : _brushes) {
		command::Command::registerCommand("action" + b->name().toLower(), [&](const command::CmdArgs &args) {
			setBrushType(b->type());
		}).setHelp("Change the brush type to '" + b->name() + "'");
	}

	command::Command::registerCommand("lock", [&] (const command::CmdArgs& args) {
		if (args.size() != 1) {
			Log::info("Usage: lock <x|y|z>");
			return;
		}
		const math::Axis axis = math::toAxis(args[0]);
		const bool unlock = (_brushContext.lockedAxis & axis) == axis;
		setLockedAxis(axis, unlock);
	}).setHelp("Toggle locked mode for the given axis at the current cursor position").setArgumentCompleter(command::valueCompleter({"x", "y", "z"}));

	command::Command::registerCommand("lockx", [&] (const command::CmdArgs& args) {
		const math::Axis axis = math::Axis::X;
		const bool unlock = (_brushContext.lockedAxis & axis) == axis;
		setLockedAxis(axis, unlock);
	}).setHelp("Toggle locked mode for the x axis at the current cursor position");

	command::Command::registerCommand("locky", [&](const command::CmdArgs &args) {
		const math::Axis axis = math::Axis::Y;
		const bool unlock = (_brushContext.lockedAxis & axis) == axis;
		setLockedAxis(axis, unlock);
	}).setHelp("Toggle locked mode for the y axis at the current cursor position");

	command::Command::registerCommand("lockz", [&](const command::CmdArgs &args) {
		const math::Axis axis = math::Axis::Z;
		const bool unlock = (_brushContext.lockedAxis & axis) == axis;
		setLockedAxis(axis, unlock);
	}).setHelp("Toggle locked mode for the z axis at the current cursor position");

	for (Brush *b : _brushes) {
		b->construct();
	}
}

void Modifier::setLockedAxis(math::Axis axis, bool unlock) {
	if (unlock) {
		_brushContext.lockedAxis &= ~axis;
	} else {
		_brushContext.lockedAxis |= axis;
	}
}

bool Modifier::init() {
	for (Brush *b : _brushes) {
		if (!b->init()) {
			Log::error("Failed to initialize the %s brush", b->name().c_str());
		}
	}
	return true;
}

void Modifier::shutdown() {
	reset();
	for (Brush *b : _brushes) {
		b->shutdown();
	}
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
	if (Brush *brush = activeBrush()) {
		brush->update(_brushContext, nowSeconds);
	}
}

void Modifier::reset() {
	unselect();
	_brushContext.gridResolution = 1;
	_brushContext.cursorPosition = glm::ivec3(0);
	_brushContext.cursorFace = voxel::FaceNames::Max;

	_modifierType = ModifierType::Place;
	for (Brush *b : _brushes) {
		b->reset();
	}
	setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 0));
}

bool Modifier::start() {
	if (isMode(ModifierType::Select)) {
		_selectStartPosition = _brushContext.cursorPosition;
		_selectStartPositionValid = true;
		return true;
	}

	if (AABBBrush *brush = activeAABBBrush()) {
		return brush->start(_brushContext);
	}
	return false;
}

void Modifier::executeAdditionalAction() {
	if (isMode(ModifierType::Select) || isMode(ModifierType::ColorPicker)) {
		return;
	}
	if (AABBBrush *brush = activeAABBBrush()) {
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
	for (size_t i = 0; i < _selections.size(); ++i) {
		const Selection &s = _selections[i];
		if (s.containsRegion(sel)) {
			return true;
		}
	}

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
	if (isMode(ModifierType::Select)) {
		return false;
	}
	if (const AABBBrush *brush = activeAABBBrush()) {
		return brush->needsFurtherAction(_brushContext);
	}
	return false;
}

glm::ivec3 Modifier::currentCursorPosition() {
	if (AABBBrush *brush = activeAABBBrush()) {
		return brush->currentCursorPosition(_brushContext.cursorPosition);
	}
	return _brushContext.cursorPosition;
}

voxel::Region Modifier::calcBrushRegion() {
	if (_brushType == BrushType::Shape) {
		return _shapeBrush.calcRegion(_brushContext);
	} else if (_brushType == BrushType::Stamp) {
		return _stampBrush.calcRegion(_brushContext);
	}
	return voxel::Region::InvalidRegion;
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

voxel::Region Modifier::calcSelectionRegion() const {
	const glm::ivec3 &mins = glm::min(_selectStartPosition, _brushContext.cursorPosition);
	const glm::ivec3 &maxs = glm::max(_selectStartPosition, _brushContext.cursorPosition);
	return voxel::Region(mins, maxs);
}

bool Modifier::execute(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, const Callback &callback) {
	if (_locked) {
		return false;
	}
	if (aborted()) {
		return false;
	}
	if (isMode(ModifierType::Select)) {
		const voxel::Region &region = calcSelectionRegion();
		const glm::ivec3 &mins = region.getLowerCorner();
		const glm::ivec3 &maxs = region.getUpperCorner();
		Log::debug("select mode mins: %i:%i:%i, maxs: %i:%i:%i", mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
		select(mins, maxs);
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

	executeBrush(sceneGraph, node, _modifierType, _brushContext.cursorVoxel, callback);

	return true;
}

bool Modifier::executeBrush(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
							ModifierType modifierType, const voxel::Voxel &voxel, const Callback &callback) {
	ModifierVolumeWrapper wrapper(node, modifierType, _selections);
	voxel::Voxel prevVoxel = _brushContext.cursorVoxel;
	_brushContext.cursorVoxel = voxel;
	if (Brush *brush = activeBrush()) {
		brush->execute(sceneGraph, wrapper, _brushContext);
	}
	const voxel::Region &modifiedRegion = wrapper.dirtyRegion();
	if (modifiedRegion.isValid()) {
		voxel::logRegion("Dirty region", modifiedRegion);
		callback(modifiedRegion, _modifierType, true);
	}
	_brushContext.cursorVoxel = prevVoxel;
	return true;
}

Brush *Modifier::activeBrush() {
	for (Brush *b : _brushes) {
		if (b->type() == _brushType) {
			return b;
		}
	}
	return nullptr;
}

AABBBrush *Modifier::activeAABBBrush() {
	if (_brushType == BrushType::Shape) {
		return &_shapeBrush;
	}
	return nullptr;
}

const AABBBrush *Modifier::activeAABBBrush() const {
	if (_brushType == BrushType::Shape) {
		return &_shapeBrush;
	}
	return nullptr;
}

void Modifier::stop() {
	if (isMode(ModifierType::Select)) {
		_selectStartPositionValid = false;
		return;
	}
	if (AABBBrush *brush = activeAABBBrush()) {
		brush->stop(_brushContext);
	}
}

bool Modifier::modifierTypeRequiresExistingVoxel() const {
	return isMode(ModifierType::Erase) || isMode(ModifierType::Paint) || isMode(ModifierType::Select);
}

void Modifier::setBrushType(BrushType type) {
	_brushType = type;
	const bool isBrush = _brushType != BrushType::None;
	const bool modifierIsBrush = (_modifierType & ModifierType::Brush) != ModifierType::None;
	if (isBrush && !modifierIsBrush) {
		setModifierType(ModifierType::Place);
	}
}

void Modifier::setGridResolution(int gridSize) {
	_brushContext.gridResolution = core_max(1, gridSize);
}

void Modifier::setModifierType(ModifierType type) {
	const bool isBrush = _brushType != BrushType::None;
	const bool modifierIsBrush = (type & ModifierType::Brush) != ModifierType::None;
	_modifierType = type;
	if (!isBrush && modifierIsBrush) {
		setBrushType(BrushType::Shape);
	}
}

} // namespace voxedit
