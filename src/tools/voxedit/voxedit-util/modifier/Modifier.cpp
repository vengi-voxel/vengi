/**
 * @file
 */

#include "Modifier.h"
#include "math/Axis.h"
#include "core/Color.h"
#include "core/StringUtil.h"
#include "core/command/Command.h"
#include "voxel/MaterialColor.h"
#include "voxel/Region.h"
#include "video/ScopedPolygonMode.h"
#include "voxelgenerator/ShapeGenerator.h"
#include "../AxisUtil.h"
#include "../CustomBindingContext.h"
#include "../SceneManager.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

namespace voxedit {

Modifier::Modifier() :
		_deleteExecuteButton(ModifierType::Delete) {
}

void Modifier::setModifierType(ModifierType type) {
	_modifierType = type;
}

bool Modifier::aabbMode() const {
	return _aabbMode;
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
	_aabbMode = true;
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

void Modifier::updateSelectionBuffers() {
	_selectionValid = _selection.isValid();
	if (!_selectionValid) {
		return;
	}
	_shapeBuilder.clear();
	_shapeBuilder.setColor(core::Color::Yellow);
	_shapeBuilder.aabb(_selection.getLowerCorner(), _selection.getUpperCorner() + glm::one<glm::ivec3>());
	_shapeRenderer.createOrUpdate(_selectionIndex, _shapeBuilder);
}

bool Modifier::select(const glm::ivec3& mins, const glm::ivec3& maxs, voxel::RawVolume* volume, std::function<void(const voxel::Region& region, ModifierType type)> callback) {
	const bool select = (_modifierType & ModifierType::Delete) == ModifierType::None;
	if (select) {
		_selection = voxel::Region{mins, maxs};
	} else {
		_selection = voxel::Region::InvalidRegion;
	}
	updateSelectionBuffers();
	return true;
}

bool Modifier::executeShapeAction(ModifierVolumeWrapper& wrapper, const glm::ivec3& mins, const glm::ivec3& maxs, std::function<void(const voxel::Region& region, ModifierType type)> callback) {
	glm::ivec3 operateMins = mins;
	glm::ivec3 operateMaxs = maxs;
	if (_selection.isValid()) {
		operateMins = (glm::max)(mins, _selection.getLowerCorner());
		operateMaxs = (glm::min)(maxs, _selection.getUpperCorner());
	}

	const voxel::Region region(operateMins, operateMaxs);
	voxel::logRegion("Shape action execution", region);
	const glm::ivec3& center = region.getCentre();
	glm::ivec3 centerBottom = region.getCentre();
	centerBottom.y = region.getLowerY();
	const glm::ivec3& dimensions = region.getDimensionsInVoxels();
	switch (_shapeType) {
	case ShapeType::AABB:
		voxelgenerator::shape::createCubeNoCenter(wrapper, operateMins, dimensions, _cursorVoxel);
		break;
	case ShapeType::Torus: {
		const int innerRadius = 4;
		const int outerRadius = dimensions.x / 2;
		if (dimensions.x / 2 < innerRadius) {
			voxelgenerator::shape::createCubeNoCenter(wrapper, operateMins, dimensions, _cursorVoxel);
		} else {
			voxelgenerator::shape::createTorus(wrapper, center, innerRadius, outerRadius, _cursorVoxel);
		}
		break;
	}
	case ShapeType::Cylinder:
		voxelgenerator::shape::createCylinder(wrapper, centerBottom, math::Axis::Y, (glm::max)(dimensions.x, dimensions.z), dimensions.y, _cursorVoxel);
		break;
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
		callback(modifiedRegion, _modifierType);
	}
	return true;
}

bool Modifier::needsSecondAction() {
	const glm::ivec3 delta = aabbDim();
	if (delta.x > 1 && delta.z > 1 && delta.y == 1) {
		_aabbSecondActionDirection = math::Axis::Y;
	} else if (delta.y > 1 && delta.z > 1 && delta.x == 1) {
		_aabbSecondActionDirection = math::Axis::X;
	} else if (delta.x > 1 && delta.y > 1 && delta.z == 1) {
		_aabbSecondActionDirection = math::Axis::Z;
	} else {
		_aabbSecondActionDirection = math::Axis::None;
	}
	return _aabbSecondActionDirection != math::Axis::None;
}

glm::ivec3 Modifier::firstPos() const {
	if (!_center || _secondPosValid) {
		return _aabbFirstPos;
	}
	const int size = _gridResolution;
	const glm::ivec3& first = _aabbFirstPos;
	const glm::ivec3& pos = aabbPosition();
	const glm::ivec3& mins = (glm::min)(first, pos);
	const glm::ivec3& maxs = (glm::max)(first, pos);
	const glm::ivec3& delta = maxs + size - mins;
	const glm::ivec3& deltaa = glm::abs(delta);
	glm::ivec3 f = _aabbFirstPos;
	if (deltaa.x > 1 && deltaa.z > 1 && deltaa.y == 1) {
		f.x += delta.x;
		f.z += delta.z;
	} else if (deltaa.y > 1 && deltaa.z > 1 && deltaa.x == 1) {
		f.y += delta.y;
		f.z += delta.z;
	} else if (deltaa.x > 1 && deltaa.y > 1 && deltaa.z == 1) {
		f.x += delta.x;
		f.y += delta.y;
	}
	return f;
}

glm::ivec3 Modifier::aabbDim() const {
	const int size = _gridResolution;
	glm::ivec3 pos = aabbPosition();
	const glm::ivec3& first = firstPos();
	const glm::ivec3& mins = (glm::min)(first, pos);
	const glm::ivec3& maxs = (glm::max)(first, pos);
	return glm::abs(maxs + size - mins);
}

bool Modifier::aabbAction(voxel::RawVolume* volume, std::function<void(const voxel::Region& region, ModifierType type)> callback) {
	if (!_aabbMode) {
		Log::debug("Not in aabb mode - can't perform action");
		return false;
	}

	const int size = _gridResolution;
	const glm::ivec3& pos = aabbPosition();
	const glm::ivec3& firstP = firstPos();
	const glm::ivec3 mins = (glm::min)(firstP, pos);
	const glm::ivec3 maxs = (glm::max)(firstP, pos) + (size - 1);

	if ((_modifierType & ModifierType::Select) == ModifierType::Select) {
		Log::debug("select mode");
		return select(mins, maxs, volume, callback);
	}

	if (volume == nullptr) {
		Log::debug("No volume given - can't perform action");
		return true;
	}

	ModifierVolumeWrapper wrapper(volume, _modifierType);
	glm::ivec3 minsMirror = mins;
	glm::ivec3 maxsMirror = maxs;
	if (!getMirrorAABB(minsMirror, maxsMirror)) {
		return executeShapeAction(wrapper, mins, maxs, callback);
	}
	Log::debug("Execute mirror action");
	const math::AABB<int> first(mins, maxs);
	const math::AABB<int> second(minsMirror, maxsMirror);
	if (math::intersects(first, second)) {
		executeShapeAction(wrapper, mins, maxsMirror, callback);
	} else {
		executeShapeAction(wrapper, mins, maxs, callback);
		executeShapeAction(wrapper, minsMirror, maxsMirror, callback);
	}
	_secondPosValid = false;
	return true;
}

void Modifier::aabbStop() {
	_secondPosValid = false;
	_aabbMode = false;
}

void Modifier::renderAABBMode(const video::Camera& camera) {
	if (!_aabbMode) {
		return;
	}
	static glm::ivec3 lastCursor = aabbPosition();
	static math::Axis lastMirrorAxis = _mirrorAxis;

	const glm::ivec3& cursor = aabbPosition();
	const bool needsUpdate = lastCursor != cursor || lastMirrorAxis != _mirrorAxis;

	if (needsUpdate) {
		lastMirrorAxis = _mirrorAxis;
		lastCursor = cursor;

		_shapeBuilder.clear();
		_shapeBuilder.setColor(core::Color::alpha(core::Color::Red, 0.5f));
		const glm::ivec3& first = firstPos();
		const glm::ivec3& mins = (glm::min)(first, cursor);
		const glm::ivec3& maxs = (glm::max)(first, cursor);
		glm::ivec3 minsMirror = mins;
		glm::ivec3 maxsMirror = maxs;
		const float size = _gridResolution;
		if (getMirrorAABB(minsMirror, maxsMirror)) {
			const math::AABB<int> first(mins, maxs);
			const math::AABB<int> second(minsMirror, maxsMirror);
			if (math::intersects(first, second)) {
				_shapeBuilder.cube(glm::vec3(mins), glm::vec3(maxsMirror) + size);
			} else {
				_shapeBuilder.cube(glm::vec3(mins), glm::vec3(maxs) + size);
				_shapeBuilder.cube(glm::vec3(minsMirror), glm::vec3(maxsMirror) + size);
			}
		} else {
			_shapeBuilder.cube(glm::vec3(mins), glm::vec3(maxs) + size);
		}
		_shapeRenderer.createOrUpdate(_aabbMeshIndex, _shapeBuilder);
	}
	static const glm::vec2 offset(-0.25f, -0.5f);
	video::ScopedPolygonMode polygonMode(camera.polygonMode(), offset);
	_shapeRenderer.render(_aabbMeshIndex, camera);
}

void Modifier::render(const video::Camera& camera) {
	renderAABBMode(camera);
	const glm::mat4& translate = glm::translate(glm::vec3(aabbPosition()));
	const glm::mat4& scale = glm::scale(translate, glm::vec3(_gridResolution));
	_shapeRenderer.render(_voxelCursorMesh, camera, scale);
	_shapeRenderer.render(_mirrorMeshIndex, camera);
	if (_selectionValid) {
		_shapeRenderer.render(_selectionIndex, camera);
	}
}

ModifierType Modifier::modifierType() const {
	return _modifierType;
}

bool Modifier::modifierTypeRequiresExistingVoxel() const {
	return (_modifierType & ModifierType::Delete) == ModifierType::Delete
			|| (_modifierType & ModifierType::Update) == ModifierType::Update;
}

void Modifier::construct() {
	core::Command::registerActionButton("actionexecute", _actionExecuteButton).setBindingContext(BindingContext::Scene);
	core::Command::registerActionButton("actionexecutedelete", _deleteExecuteButton).setBindingContext(BindingContext::Scene);

	core::Command::registerCommand("actionselect", [&] (const core::CmdArgs& args) {
		setModifierType(ModifierType::Select);
	}).setHelp("Change the modifier type to 'select'");

	core::Command::registerCommand("actiondelete", [&] (const core::CmdArgs& args) {
		setModifierType(ModifierType::Delete);
	}).setHelp("Change the modifier type to 'delete'");

	core::Command::registerCommand("actionplace", [&] (const core::CmdArgs& args) {
		setModifierType(ModifierType::Place);
	}).setHelp("Change the modifier type to 'place'");

	core::Command::registerCommand("actioncolorize", [&] (const core::CmdArgs& args) {
		setModifierType(ModifierType::Update);
	}).setHelp("Change the modifier type to 'colorize'");

	core::Command::registerCommand("actionoverride", [&] (const core::CmdArgs& args) {
		setModifierType(ModifierType::Place | ModifierType::Delete);
	}).setHelp("Change the modifier type to 'override'");

	core::Command::registerCommand("shapeaabb", [&] (const core::CmdArgs& args) {
		setShapeType(ShapeType::AABB);
	}).setHelp("Change the shape type to 'aabb'");

	core::Command::registerCommand("shapetorus", [&] (const core::CmdArgs& args) {
		setShapeType(ShapeType::Torus);
	}).setHelp("Change the shape type to 'torus'");

	core::Command::registerCommand("shapecylinder", [&] (const core::CmdArgs& args) {
		setShapeType(ShapeType::Cylinder);
	}).setHelp("Change the shape type to 'cylinder'");

	core::Command::registerCommand("shapeellipse", [&] (const core::CmdArgs& args) {
		setShapeType(ShapeType::Ellipse);
	}).setHelp("Change the shape type to 'ellipse'");

	core::Command::registerCommand("shapecone", [&] (const core::CmdArgs& args) {
		setShapeType(ShapeType::Cone);
	}).setHelp("Change the shape type to 'cone'");

	core::Command::registerCommand("shapedome", [&] (const core::CmdArgs& args) {
		setShapeType(ShapeType::Dome);
	}).setHelp("Change the shape type to 'dome'");

	core::Command::registerCommand("unselect", [&] (const core::CmdArgs& args) {
		_selection = voxel::Region::InvalidRegion;
		updateSelectionBuffers();
	}).setHelp("Unselect all");

	core::Command::registerCommand("mirroraxisx", [&] (const core::CmdArgs& args) {
		setMirrorAxis(math::Axis::X, sceneMgr().referencePosition());
	}).setHelp("Mirror around the x axis");

	core::Command::registerCommand("mirroraxisy", [&] (const core::CmdArgs& args) {
		setMirrorAxis(math::Axis::Y, sceneMgr().referencePosition());
	}).setHelp("Mirror around the y axis");

	core::Command::registerCommand("mirroraxisz", [&] (const core::CmdArgs& args) {
		setMirrorAxis(math::Axis::Z, sceneMgr().referencePosition());
	}).setHelp("Mirror around the z axis");

	core::Command::registerCommand("mirrornone", [&] (const core::CmdArgs& args) {
		setMirrorAxis(math::Axis::None, sceneMgr().referencePosition());
	}).setHelp("Disable mirror axis");
}

bool Modifier::init() {
	if (!_shapeRenderer.init()) {
		Log::error("Failed to initialize the shape renderer");
		return false;
	}

	return true;
}

void Modifier::shutdown() {
	_mirrorMeshIndex = -1;
	_aabbMeshIndex = -1;
	_selectionIndex = -1;
	_voxelCursorMesh = -1;
	_mirrorAxis = math::Axis::None;
	_aabbMode = false;
	_modifierType = ModifierType::Place;
	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();
}

math::Axis Modifier::mirrorAxis() const {
	return _mirrorAxis;
}

void Modifier::setCursorPosition(const glm::ivec3& pos, voxel::FaceNames face) {
	_cursorPosition = pos;
	_face = face;
}

void Modifier::setGridResolution(int resolution) {
	_gridResolution = resolution;
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

void Modifier::setMirrorAxis(math::Axis axis, const glm::ivec3& mirrorPos) {
	if (_mirrorAxis == axis) {
		if (_mirrorPos != mirrorPos) {
			_mirrorPos = mirrorPos;
			updateMirrorPlane();
		}
		return;
	}
	_mirrorPos = mirrorPos;
	_mirrorAxis = axis;
	updateMirrorPlane();
}

void Modifier::updateMirrorPlane() {
	if (_mirrorAxis == math::Axis::None) {
		if (_mirrorMeshIndex != -1) {
			_shapeRenderer.deleteMesh(_mirrorMeshIndex);
			_mirrorMeshIndex = -1;
		}
		return;
	}

	updateShapeBuilderForPlane(_shapeBuilder, sceneMgr().region(), true, _mirrorPos, _mirrorAxis,
			core::Color::alpha(core::Color::LightGray, 0.3f));
	_shapeRenderer.createOrUpdate(_mirrorMeshIndex, _shapeBuilder);
}

void Modifier::setCursorVoxel(const voxel::Voxel& voxel) {
	_cursorVoxel = voxel;
	_shapeBuilder.clear();
	_shapeBuilder.setColor(core::Color::alpha(core::Color::darker(voxel::getMaterialColor(voxel)), 0.6f));
	_shapeBuilder.cube(glm::vec3(-0.01f), glm::vec3(1.01f));
	_shapeRenderer.createOrUpdate(_voxelCursorMesh, _shapeBuilder);
}

void Modifier::translate(const glm::ivec3& v) {
	_cursorPosition += v;
	_mirrorPos += v;
	if (_aabbMode) {
		_aabbFirstPos += v;
	}
}

}
