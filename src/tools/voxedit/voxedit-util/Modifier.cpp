/**
 * @file
 */

#include "Modifier.h"
#include "AxisUtil.h"
#include "math/Axis.h"
#include "core/Color.h"
#include "CustomBindingContext.h"
#include "core/String.h"
#include "voxel/Region.h"
#include "core/command/Command.h"
#include "video/ScopedPolygonMode.h"
#include "SceneManager.h"
#include "voxelgenerator/ShapeGenerator.h"

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
	return _cursorPosition;
}

glm::ivec3 Modifier::aabbDim() const {
	const int size = _gridResolution;
	const glm::ivec3& pos = aabbPosition();
	const glm::ivec3& mins = (glm::min)(_aabbFirstPos, pos);
	const glm::ivec3& maxs = (glm::max)(_aabbFirstPos, pos);
	return glm::abs(maxs + size - mins);
}

bool Modifier::aabbStart() {
	if (_aabbMode) {
		return false;
	}
	_aabbFirstPos = aabbPosition();
	_aabbMode = true;
	return true;
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
	if (!_selection.isValid()) {
		operateMins = (glm::max)(mins, _selection.getLowerCorner());
		operateMaxs = (glm::min)(maxs, _selection.getUpperCorner());
	}

	const voxel::Region region(operateMins, operateMaxs);
	const glm::ivec3& center = region.getCentre();
	glm::ivec3 centerBottom = region.getCentre();
	centerBottom.y = region.getLowerY();
	const glm::ivec3& dimensions = region.getDimensionsInVoxels();
	switch (_shapeType) {
	case ShapeType::AABB:
		voxelgenerator::shape::createCubeNoCenter(wrapper, operateMins, dimensions, _cursorVoxel);
		break;
	case ShapeType::Torus: {
		const int minTorusInnerRadius = 4;
		const int outerRadius = dimensions.x / 2;
		if (dimensions.x / 2 < minTorusInnerRadius) {
			voxelgenerator::shape::createCubeNoCenter(wrapper, operateMins, dimensions, _cursorVoxel);
		} else {
			voxelgenerator::shape::createTorus(wrapper, center, minTorusInnerRadius, outerRadius, _cursorVoxel);
		}
		break;
	}
	case ShapeType::Cylinder:
		voxelgenerator::shape::createCylinder(wrapper, centerBottom, math::Axis::X, dimensions.x / 2, dimensions.y, _cursorVoxel);
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
	default:
		return false;
	}
	if (wrapper.dirtyRegion().isValid()) {
		callback(wrapper.dirtyRegion(), _modifierType);
	}
	return true;
}

bool Modifier::aabbAction(voxel::RawVolume* volume, std::function<void(const voxel::Region& region, ModifierType type)> callback) {
	if (!_aabbMode) {
		return false;
	}

	const int size = _gridResolution;
	const glm::ivec3& pos = aabbPosition();
	const glm::ivec3 mins = (glm::min)(_aabbFirstPos, pos);
	const glm::ivec3 maxs = (glm::max)(_aabbFirstPos, pos) + (size - 1);

	if ((_modifierType & ModifierType::Select) == ModifierType::Select) {
		return select(mins, maxs, volume, callback);
	}

	if (volume == nullptr) {
		return true;
	}

	ModifierVolumeWrapper wrapper(volume, _modifierType);
	glm::ivec3 minsMirror = mins;
	glm::ivec3 maxsMirror = maxs;
	if (!getMirrorAABB(minsMirror, maxsMirror)) {
		return executeShapeAction(wrapper, mins, maxs, callback);
	}
	const math::AABB<int> first(mins, maxs);
	const math::AABB<int> second(minsMirror, maxsMirror);
	if (math::intersects(first, second)) {
		executeShapeAction(wrapper, mins, maxsMirror, callback);
	} else {
		executeShapeAction(wrapper, mins, maxs, callback);
		executeShapeAction(wrapper, minsMirror, maxsMirror, callback);
	}
	return true;
}

void Modifier::aabbStop() {
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
		const glm::ivec3& mins = (glm::min)(_aabbFirstPos, cursor);
		const glm::ivec3& maxs = (glm::max)(_aabbFirstPos, cursor);
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
