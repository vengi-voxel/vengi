/**
 * @file
 */

#include "Modifier.h"
#include "AxisUtil.h"
#include "tool/Fill.h"
#include "math/Axis.h"
#include "core/Color.h"
#include "core/String.h"
#include "voxel/polyvox/Region.h"
#include "core/command/Command.h"
#include "SceneManager.h"

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
	const glm::ivec3& mins = glm::min(_aabbFirstPos, pos);
	const glm::ivec3& maxs = glm::max(_aabbFirstPos, pos);
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

bool Modifier::aabbAction(voxel::RawVolume* volume, std::function<void(const voxel::Region& region, ModifierType type)> callback) {
	if (!_aabbMode) {
		return false;
	}
	if (volume == nullptr) {
		return true;
	}
	voxel::RawVolumeWrapper wrapper(volume);
	const int size = _gridResolution;
	const glm::ivec3& pos = aabbPosition();
	const glm::ivec3 mins = glm::min(_aabbFirstPos, pos);
	const glm::ivec3 maxs = glm::max(_aabbFirstPos, pos) + (size - 1);
	voxel::Region modifiedRegion = voxel::Region::InvalidRegion;
	glm::ivec3 minsMirror = mins;
	glm::ivec3 maxsMirror = maxs;
	if (!getMirrorAABB(minsMirror, maxsMirror)) {
		if (voxedit::tool::aabb(wrapper, mins, maxs, _cursorVoxel, _modifierType, &modifiedRegion)) {
			callback(modifiedRegion, _modifierType);
		}
		return true;
	}
	const math::AABB<int> first(mins, maxs);
	const math::AABB<int> second(minsMirror, maxsMirror);
	voxel::Region modifiedRegionMirror;
	if (math::intersects(first, second)) {
		if (voxedit::tool::aabb(wrapper, mins, maxsMirror, _cursorVoxel, _modifierType, &modifiedRegionMirror)) {
			callback(modifiedRegionMirror, _modifierType);
		}
	} else {
		if (voxedit::tool::aabb(wrapper, mins, maxs, _cursorVoxel, _modifierType, &modifiedRegion)) {
			callback(modifiedRegion, _modifierType);
		}
		if (voxedit::tool::aabb(wrapper, minsMirror, maxsMirror, _cursorVoxel, _modifierType, &modifiedRegionMirror)) {
			callback(modifiedRegionMirror, _modifierType);
		}
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
		const glm::ivec3& mins = glm::min(_aabbFirstPos, cursor);
		const glm::ivec3& maxs = glm::max(_aabbFirstPos, cursor);
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
}

ModifierType Modifier::modifierType() const {
	return _modifierType;
}

bool Modifier::modifierTypeRequiresExistingVoxel() const {
	return (_modifierType & ModifierType::Delete) == ModifierType::Delete
			|| (_modifierType & ModifierType::Update) == ModifierType::Update;
}

void Modifier::construct() {
	core::Command::registerActionButton("actionexecute", _actionExecuteButton);
	core::Command::registerActionButton("actionexecutedelete", _deleteExecuteButton);
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

	core::Command::registerCommand("mirrorx", [&] (const core::CmdArgs& args) {
		setMirrorAxis(math::Axis::X, sceneMgr().referencePosition());
	}).setHelp("Mirror around the x axis");

	core::Command::registerCommand("mirrory", [&] (const core::CmdArgs& args) {
		setMirrorAxis(math::Axis::Y, sceneMgr().referencePosition());
	}).setHelp("Mirror around the y axis");

	core::Command::registerCommand("mirrorz", [&] (const core::CmdArgs& args) {
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

void Modifier::shift(const glm::ivec3& v) {
	_cursorPosition += v;
	_mirrorPos += v;
	if (_aabbMode) {
		_aabbFirstPos += v;
	}
}

}
