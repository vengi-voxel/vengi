/**
 * @file
 */

#include "Modifier.h"
#include "math/Axis.h"
#include "core/Color.h"
#include "core/StringUtil.h"
#include "command/Command.h"
#include "ui/dearimgui/imgui_internal.h"
#include "voxedit-util/modifier/Selection.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelgenerator/ShapeGenerator.h"
#include "../AxisUtil.h"
#include "../SceneManager.h"
#include "voxelutil/VoxelUtil.h"
#include "voxelutil/AStarPathfinder.h"

namespace voxedit {

Modifier::Modifier() :
				_deleteExecuteButton(ModifierType::Erase) {
}

void Modifier::construct() {
	command::Command::registerActionButton("actionexecute", _actionExecuteButton, "Execute the modifier action");
	command::Command::registerActionButton("actionexecutedelete", _deleteExecuteButton, "Execute the modifier action in delete mode");

	command::Command::registerCommand("actionselect", [&] (const command::CmdArgs& args) {
		setModifierType(ModifierType::Select);
	}).setHelp("Change the modifier type to 'select'");

	command::Command::registerCommand("actioncolorpicker", [&] (const command::CmdArgs& args) {
		setModifierType(ModifierType::ColorPicker);
	}).setHelp("Change the modifier type to 'color picker'");

	command::Command::registerCommand("actionpath", [&] (const command::CmdArgs& args) {
		setModifierType(ModifierType::Path);
	}).setHelp("Change the modifier type to 'path'");

	command::Command::registerCommand("actionline", [&] (const command::CmdArgs& args) {
		setModifierType(ModifierType::Line);
	}).setHelp("Change the modifier type to 'line'");

	command::Command::registerCommand("actionerase", [&] (const command::CmdArgs& args) {
		setModifierType(ModifierType::Erase);
	}).setHelp("Change the modifier type to 'erase'");

	command::Command::registerCommand("actionplace", [&] (const command::CmdArgs& args) {
		setModifierType(ModifierType::Place);
	}).setHelp("Change the modifier type to 'place'");

	command::Command::registerCommand("actionpaint", [&] (const command::CmdArgs& args) {
		setModifierType(ModifierType::Paint);
	}).setHelp("Change the modifier type to 'paint'");

	command::Command::registerCommand("actionoverride", [&] (const command::CmdArgs& args) {
		setModifierType(ModifierType::Place | ModifierType::Erase);
	}).setHelp("Change the modifier type to 'override'");

	command::Command::registerCommand("shapeaabb", [&] (const command::CmdArgs& args) {
		setShapeType(ShapeType::AABB);
	}).setHelp("Change the shape type to 'aabb'");

	command::Command::registerCommand("shapetorus", [&] (const command::CmdArgs& args) {
		setShapeType(ShapeType::Torus);
	}).setHelp("Change the shape type to 'torus'");

	command::Command::registerCommand("shapecylinder", [&] (const command::CmdArgs& args) {
		setShapeType(ShapeType::Cylinder);
	}).setHelp("Change the shape type to 'cylinder'");

	command::Command::registerCommand("shapeellipse", [&] (const command::CmdArgs& args) {
		setShapeType(ShapeType::Ellipse);
	}).setHelp("Change the shape type to 'ellipse'");

	command::Command::registerCommand("shapecone", [&] (const command::CmdArgs& args) {
		setShapeType(ShapeType::Cone);
	}).setHelp("Change the shape type to 'cone'");

	command::Command::registerCommand("shapedome", [&] (const command::CmdArgs& args) {
		setShapeType(ShapeType::Dome);
	}).setHelp("Change the shape type to 'dome'");

	command::Command::registerCommand("mirroraxisx", [&] (const command::CmdArgs& args) {
		toggleMirrorAxis(math::Axis::X, sceneMgr().referencePosition());
	}).setHelp("Mirror around the x axis");

	command::Command::registerCommand("mirroraxisy", [&] (const command::CmdArgs& args) {
		toggleMirrorAxis(math::Axis::Y, sceneMgr().referencePosition());
	}).setHelp("Mirror around the y axis");

	command::Command::registerCommand("mirroraxisz", [&] (const command::CmdArgs& args) {
		toggleMirrorAxis(math::Axis::Z, sceneMgr().referencePosition());
	}).setHelp("Mirror around the z axis");

	command::Command::registerCommand("mirroraxisnone", [&] (const command::CmdArgs& args) {
		setMirrorAxis(math::Axis::None, sceneMgr().referencePosition());
	}).setHelp("Disable mirror axis");

	command::Command::registerCommand("togglemodecenter", [&] (const command::CmdArgs& args) {
		setCenterMode(!centerMode());
	}).setHelp("Toggle center plane building");

	command::Command::registerCommand("togglemodeplane", [&] (const command::CmdArgs& args) {
		setPlaneMode(!planeMode());
	}).setHelp("Toggle plane building mode (extrude)");

	command::Command::registerCommand("togglemodesingle", [&] (const command::CmdArgs& args) {
		setSingleMode(!singleMode());
	}).setHelp("Toggle single voxel building mode");
}

bool Modifier::init() {
	return true;
}

void Modifier::shutdown() {
	reset();
}

void Modifier::update(double nowSeconds) {
	if (!singleMode()) {
		return;
	}
	if (_actionExecuteButton.pressed() && nowSeconds >= _nextSingleExecution) {
		_actionExecuteButton.execute(true);
		_nextSingleExecution = nowSeconds + 0.1;
	}
}

void Modifier::reset() {
	unselect();
	_gridResolution = 1;
	_secondPosValid = false;
	_aabbMode = false;
	_center = false;
	_aabbFirstPos = glm::ivec3(0);
	_aabbSecondPos = glm::ivec3(0);
	_aabbSecondActionDirection = math::Axis::None;
	_modifierType = ModifierType::Place;
	_mirrorAxis = math::Axis::None;
	_mirrorPos = glm::ivec3(0);
	_cursorPosition = glm::ivec3(0);
	_face = voxel::FaceNames::Max;
	_shapeType = ShapeType::AABB;
	setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 0));
}

glm::ivec3 Modifier::aabbPosition() const {
	glm::ivec3 pos = _cursorPosition;
	if (_secondPosValid) {
		switch (_aabbSecondActionDirection) {
		case math::Axis::X:
			pos.y = _aabbSecondPos.y;
			pos.z = _aabbSecondPos.z;
			break;
		case math::Axis::Y:
			pos.x = _aabbSecondPos.x;
			pos.z = _aabbSecondPos.z;
			break;
		case math::Axis::Z:
			pos.x = _aabbSecondPos.x;
			pos.y = _aabbSecondPos.y;
			break;
		default:
			break;
		}
	}
	return pos;
}

bool Modifier::aabbStart() {
	if (_aabbMode) {
		return false;
	}

	// the order here matters - don't change _aabbMode earlier here
	_aabbFirstPos = aabbPosition();
	_secondPosValid = false;
	_aabbSecondActionDirection = math::Axis::None;
	_aabbMode = !singleMode();
	return true;
}

void Modifier::aabbStep() {
	if (!_aabbMode) {
		return;
	}
	_aabbSecondPos = aabbPosition();
	_aabbFirstPos = firstPos();
	_secondPosValid = true;
}

bool Modifier::getMirrorAABB(glm::ivec3& mins, glm::ivec3& maxs) const {
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

math::Axis Modifier::getSizeAndHeightFromAxisAndDim(math::Axis axis, const glm::ivec3& dimensions, double &size, double &height) const {
	if (axis == math::Axis::None) {
		axis = math::Axis::Y;
	}
	switch (axis) {
	case math::Axis::X:
		size = (glm::max)(dimensions.y, dimensions.z);
		height = dimensions.x;
		break;
	case math::Axis::Y:
		size = (glm::max)(dimensions.x, dimensions.z);
		height = dimensions.y;
		break;
	case math::Axis::Z:
		size = (glm::max)(dimensions.x, dimensions.y);
		height = dimensions.z;
		break;
	default:
		size = 0.0;
		height = 0.0;
		return math::Axis::None;
	}
	return axis;
}

void Modifier::setReferencePosition(const glm::ivec3 &pos) {
	_referencePos = pos;
}

bool Modifier::executeShapeAction(ModifierVolumeWrapper& wrapper, const glm::ivec3& mins, const glm::ivec3& maxs, const std::function<void(const voxel::Region& region, ModifierType type, bool markUndo)>& callback, bool markUndo) {
	glm::ivec3 operateMins = mins;
	glm::ivec3 operateMaxs = maxs;

	const voxel::Region region(operateMins, operateMaxs);
	voxel::logRegion("Shape action execution", region);
	const glm::ivec3& center = region.getCenter();
	glm::ivec3 centerBottom = center;
	centerBottom.y = region.getLowerY();
	const glm::ivec3& dimensions = region.getDimensionsInVoxels();

	double size = 0.0;
	double height = 0.0;
	const math::Axis axis = getSizeAndHeightFromAxisAndDim(_aabbSecondActionDirection, dimensions, size, height);

	switch (_shapeType) {
	case ShapeType::AABB:
		voxelgenerator::shape::createCubeNoCenter(wrapper, operateMins, dimensions, _cursorVoxel);
		break;
	case ShapeType::Torus: {
		const double minorRadius = size / 5.0;
		const double majorRadius = size / 2.0 - minorRadius;
		voxelgenerator::shape::createTorus(wrapper, center, minorRadius, majorRadius, _cursorVoxel);
		break;
	}
	case ShapeType::Cylinder: {
		const double radius = size / 2.0;
		voxelgenerator::shape::createCylinder(wrapper, centerBottom, axis, (int)glm::round(radius), (int)glm::round(height), _cursorVoxel);
		break;
	}
	case ShapeType::Cone:
		voxelgenerator::shape::createCone(wrapper, center, dimensions, _cursorVoxel);
		break;
	case ShapeType::Dome:
		voxelgenerator::shape::createDome(wrapper, center, dimensions, _cursorVoxel);
		break;
	case ShapeType::Ellipse:
		voxelgenerator::shape::createEllipse(wrapper, center, dimensions, _cursorVoxel);
		break;
	case ShapeType::Max:
		Log::warn("Invalid shape type selected - can't perform action");
		return false;
	}
	const voxel::Region& modifiedRegion = wrapper.dirtyRegion();
	if (modifiedRegion.isValid()) {
		voxel::logRegion("Dirty region", modifiedRegion);
		callback(modifiedRegion, _modifierType, markUndo);
	}
	return true;
}

bool Modifier::needsSecondAction() {
	if (singleMode() || isMode(ModifierType::Line)) {
		return false;
	}
	const glm::ivec3 delta = aabbDim();
	if (delta.x > _gridResolution && delta.z > _gridResolution && delta.y == _gridResolution) {
		_aabbSecondActionDirection = math::Axis::Y;
	} else if (delta.y > _gridResolution && delta.z > _gridResolution && delta.x == _gridResolution) {
		_aabbSecondActionDirection = math::Axis::X;
	} else if (delta.x > _gridResolution && delta.y > _gridResolution && delta.z == _gridResolution) {
		_aabbSecondActionDirection = math::Axis::Z;
	} else {
		_aabbSecondActionDirection = math::Axis::None;
	}
	return _aabbSecondActionDirection != math::Axis::None;
}

glm::ivec3 Modifier::firstPos() const {
	return _aabbFirstPos;
}

math::AABB<int> Modifier::aabb() const {
	const glm::ivec3 &pos = aabbPosition();
	const bool single = singleMode() || isMode(ModifierType::Line);
	if (!single && _center) {
		const glm::ivec3 &first = firstPos();
		const glm::ivec3 &delta = glm::abs(pos - first);
		return math::AABB<int>(first - delta, first + delta);
	}

	const int size = _gridResolution;
	const glm::ivec3 &first = single ? pos : firstPos();
	const glm::ivec3 &mins = (glm::min)(first, pos);
	const glm::ivec3 &maxs = (glm::max)(first, pos) + (size - 1);
	return math::AABB<int>(mins, maxs);
}

glm::ivec3 Modifier::aabbDim() const {
	const int size = _gridResolution;
	const glm::ivec3 &pos = aabbPosition();
	const bool single = singleMode() || isMode(ModifierType::Line);
	if (!single && _center) {
		const glm::ivec3 &first = firstPos();
		const glm::ivec3 &delta = glm::abs(pos - first);
		return delta * 2 + size;
	}
	const glm::ivec3 &first = single ? pos : firstPos();
	const glm::ivec3 &mins = (glm::min)(first, pos);
	const glm::ivec3 &maxs = (glm::max)(first, pos);
	return glm::abs(maxs + size - mins);
}

voxel::RawVolumeWrapper Modifier::createRawVolumeWrapper(voxel::RawVolume* volume) const {
	return voxel::RawVolumeWrapper(volume, createRegion(volume));
}

voxel::Region Modifier::createRegion(const voxel::RawVolume* volume) const {
	voxel::Region region = volume->region();
	if (_selectionValid) {
		voxel::Region srcRegion = accumulate(_selections);
		srcRegion.cropTo(region);
		return srcRegion;
	}
	return region;
}

void Modifier::setHitCursorVoxel(const voxel::Voxel& voxel) {
	_hitCursorVoxel = voxel;
}

void Modifier::setVoxelAtCursor(const voxel::Voxel& voxel) {
	_voxelAtCursor = voxel;
}

void Modifier::lock() {
	_locked = true;
}

void Modifier::unlock() {
	_locked = false;
}

bool Modifier::lineModifier(voxel::RawVolume *&volume, const Callback &callback) {
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

bool Modifier::planeModifier(voxel::RawVolume *&volume, const Callback &callback) {
	voxel::RawVolumeWrapper wrapper = createRawVolumeWrapper(volume);
	voxel::Voxel hitVoxel = hitCursorVoxel();

	if (isMode(ModifierType::Place)) {
		voxelutil::extrudePlane(wrapper, cursorPosition(), cursorFace(), hitVoxel, cursorVoxel());
	} else if (isMode(ModifierType::Erase)) {
		voxelutil::erasePlane(wrapper, cursorPosition(), cursorFace(), hitVoxel);
	} else if (isMode(ModifierType::Paint)) {
		voxelutil::paintPlane(wrapper, cursorPosition(), cursorFace(), hitVoxel, cursorVoxel());
	} else {
		Log::error("Unsupported plane modifier");
		return false;
	}

	const voxel::Region &modifiedRegion = wrapper.dirtyRegion();
	if (modifiedRegion.isValid()) {
		voxel::logRegion("Dirty region", modifiedRegion);
		callback(modifiedRegion, _modifierType, true);
	}
	return true;
}

bool Modifier::pathModifier(voxel::RawVolume *&volume, const Callback &callback) {
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
	for (const glm::ivec3& p : listResult) {
		wrapper.setVoxel(p, cursorVoxel());
	}
	const voxel::Region &modifiedRegion = wrapper.dirtyRegion();
	if (modifiedRegion.isValid()) {
		callback(modifiedRegion, _modifierType, true);
	}
	return true;
}

bool Modifier::aabbAction(voxel::RawVolume *volume, const Callback &callback) {
	if (_locked) {
		return false;
	}
	if (isMode(ModifierType::Select)) {
		const math::AABB<int> a = aabb();
		Log::debug("select mode");
		select(a.mins(), a.maxs());
		if (_selectionValid) {
			callback(accumulate(_selections), _modifierType, false);
		}
		return true;
	}

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
	if (planeMode()) {
		return planeModifier(volume, callback);
	}

	ModifierVolumeWrapper wrapper(volume, _modifierType, _selections);
	const math::AABB<int> a = aabb();
	glm::ivec3 minsMirror = a.mins();
	glm::ivec3 maxsMirror = a.maxs();
	if (!getMirrorAABB(minsMirror, maxsMirror)) {
		return executeShapeAction(wrapper, a.mins(), a.maxs(), callback, true);
	}
	Log::debug("Execute mirror action");
	const math::AABB<int> second(minsMirror, maxsMirror);
	if (math::intersects(a, second)) {
		executeShapeAction(wrapper, a.mins(), maxsMirror, callback, true);
	} else {
		executeShapeAction(wrapper, a.mins(), a.maxs(), callback, false);
		executeShapeAction(wrapper, minsMirror, maxsMirror, callback, true);
	}
	_secondPosValid = false;
	_aabbSecondActionDirection = math::Axis::None;
	return true;
}

void Modifier::aabbAbort() {
	_secondPosValid = false;
	_aabbSecondActionDirection = math::Axis::None;
	_aabbMode = false;
}

bool Modifier::modifierTypeRequiresExistingVoxel() const {
	return isMode(ModifierType::Erase) || isMode(ModifierType::Paint) || isMode(ModifierType::Select);
}

void Modifier::setGridResolution(int resolution) {
	_gridResolution = core_max(1, resolution);
	if (_aabbFirstPos.x % resolution != 0) {
		_aabbFirstPos.x = (_aabbFirstPos.x / resolution) * resolution;
	}
	if (_aabbFirstPos.y % resolution != 0) {
		_aabbFirstPos.y = (_aabbFirstPos.y / resolution) * resolution;
	}
	if (_aabbFirstPos.z % resolution != 0) {
		_aabbFirstPos.z = (_aabbFirstPos.z / resolution) * resolution;
	}
}

void Modifier::toggleMirrorAxis(math::Axis axis, const glm::ivec3& mirrorPos) {
	if (mirrorAxis() == axis) {
		setMirrorAxis(math::Axis::None, mirrorPos);
	} else {
		setMirrorAxis(axis, mirrorPos);
	}
}

bool Modifier::setMirrorAxis(math::Axis axis, const glm::ivec3& mirrorPos) {
	if (_mirrorAxis == axis) {
		if (_mirrorPos != mirrorPos) {
			_mirrorPos = mirrorPos;
			return true;
		}
		return false;
	}
	_mirrorPos = mirrorPos;
	_mirrorAxis = axis;
	return true;
}

void Modifier::translate(const glm::ivec3& v) {
	_cursorPosition += v;
	_mirrorPos += v;
	if (_aabbMode) {
		_aabbFirstPos += v;
	}
}

void Modifier::setModifierType(ModifierType type) {
	if (planeMode()) {
		type |= ModifierType::Plane;
	}
	if (singleMode()) {
		type |= ModifierType::Single;
	}
	_modifierType = type;
}

void Modifier::setPlaneMode(bool state) {
	if (state) {
		_modifierType |= ModifierType::Plane;
	} else {
		_modifierType &= ~ModifierType::Plane;
	}
}

void Modifier::setSingleMode(bool state) {
	if (state) {
		_modifierType |= ModifierType::Single;
	} else {
		_modifierType &= ~ModifierType::Single;
	}
}

} // namespace voxedit
