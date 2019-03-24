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

namespace voxedit {

void Modifier::setModifierType(ModifierType type) {
	_modifierType = type;
}

bool Modifier::aabbMode() const {
	return _aabbMode;
}

glm::ivec3 Modifier::aabbPosition() const {
	if (_aabbMode) {
		if ((_modifierType & ModifierType::Extrude) == ModifierType::Extrude) {
			// TODO: select the whole plane and limit the position to it
		}
	}
	return _cursorPosition;
}

glm::ivec3 Modifier::aabbDim() const {
	const int size = _gridResolution;
	const glm::ivec3& pos = aabbPosition();
	const glm::ivec3 mins = glm::min(_aabbFirstPos, pos);
	const glm::ivec3 maxs = glm::max(_aabbFirstPos, pos);
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

bool Modifier::aabbEnd(voxel::RawVolume* volume, std::function<void(const voxel::Region& region)> callback) {
	if (!_aabbMode) {
		return false;
	}
	if (volume == nullptr) {
		return false;
	}
	voxel::RawVolumeWrapper wrapper(volume);
	_aabbMode = false;
	const int size = _gridResolution;
	const glm::ivec3& pos = aabbPosition();
	const glm::ivec3 mins = glm::min(_aabbFirstPos, pos);
	const glm::ivec3 maxs = glm::max(_aabbFirstPos, pos) + (size - 1);
	voxel::Region modifiedRegion;
	glm::ivec3 minsMirror = mins;
	glm::ivec3 maxsMirror = maxs;
	if (!getMirrorAABB(minsMirror, maxsMirror)) {
		if (voxedit::tool::aabb(wrapper, mins, maxs, _cursorVoxel, _modifierType, &modifiedRegion)) {
			SceneManager& scene = sceneMgr();
			LayerManager& layerMgr = scene.layerMgr();
			const int layerId = layerMgr.activeLayer();
			scene.modified(layerId, modifiedRegion);
		}
		return true;
	}
	const math::AABB<int> first(mins, maxs);
	const math::AABB<int> second(minsMirror, maxsMirror);
	voxel::Region modifiedRegionMirror;
	if (math::intersects(first, second)) {
		if (voxedit::tool::aabb(wrapper, mins, maxsMirror, _cursorVoxel, _modifierType, &modifiedRegionMirror)) {
			callback(modifiedRegionMirror);
		}
	} else {
		if (voxedit::tool::aabb(wrapper, mins, maxs, _cursorVoxel, _modifierType, &modifiedRegion)) {
			callback(modifiedRegion);
		}
		if (voxedit::tool::aabb(wrapper, minsMirror, maxsMirror, _cursorVoxel, _modifierType, &modifiedRegionMirror)) {
			callback(modifiedRegionMirror);
		}
	}
	return true;
}

void Modifier::render(const video::Camera& camera) {
	if (_aabbMode) {
		_shapeBuilder.clear();
		_shapeBuilder.setColor(core::Color::alpha(core::Color::Red, 0.5f));
		glm::ivec3 cursor = aabbPosition();
		glm::ivec3 mins = glm::min(_aabbFirstPos, cursor);
		glm::ivec3 maxs = glm::max(_aabbFirstPos, cursor);
		glm::ivec3 minsMirror = mins;
		glm::ivec3 maxsMirror = maxs;
		// TODO: z-fighting if you zoom out far enough
		const float delta = 0.001f;
		const float size = _gridResolution + delta;
		if (getMirrorAABB(minsMirror, maxsMirror)) {
			const math::AABB<int> first(mins, maxs);
			const math::AABB<int> second(minsMirror, maxsMirror);
			if (math::intersects(first, second)) {
				_shapeBuilder.cube(glm::vec3(mins) - delta, glm::vec3(maxsMirror) + size);
			} else {
				_shapeBuilder.cube(glm::vec3(mins) - delta, glm::vec3(maxs) + size);
				_shapeBuilder.cube(glm::vec3(minsMirror) - delta, glm::vec3(maxsMirror) + size);
			}
		} else {
			_shapeBuilder.cube(glm::vec3(mins) - delta, glm::vec3(maxs) + size);
		}
		_shapeRenderer.createOrUpdate(_aabbMeshIndex, _shapeBuilder);
		_shapeRenderer.render(_aabbMeshIndex, camera);
	}

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
			|| (_modifierType & ModifierType::Update) == ModifierType::Update
			|| (_modifierType & ModifierType::Extrude) == ModifierType::Extrude;
}

void Modifier::construct() {
	core::Command::registerCommand("actiondelete",
			[&] (const core::CmdArgs& args) {setModifierType(ModifierType::Delete);}).setHelp(
			"Change the modifier type to 'delete'");

	core::Command::registerCommand("actionplace",
			[&] (const core::CmdArgs& args) {setModifierType(ModifierType::Place);}).setHelp(
			"Change the modifier type to 'place'");

	core::Command::registerCommand("actioncolorize",
			[&] (const core::CmdArgs& args) {setModifierType(ModifierType::Update);}).setHelp(
			"Change the modifier type to 'colorize'");

	core::Command::registerCommand("actionextrude",
			[&] (const core::CmdArgs& args) {setModifierType(ModifierType::Extrude);}).setHelp(
			"Change the modifier type to 'extrude'");

	core::Command::registerCommand("actionoverride",
			[&] (const core::CmdArgs& args) {setModifierType(ModifierType::Place | ModifierType::Delete);}).setHelp(
			"Change the modifier type to 'override'");

	core::Command::registerCommand("mirrorx",
			[&] (const core::CmdArgs& args) {setMirrorAxis(math::Axis::X, sceneMgr().referencePosition());}).setHelp(
			"Mirror around the x axis");
	core::Command::registerCommand("mirrory",
			[&] (const core::CmdArgs& args) {setMirrorAxis(math::Axis::Y, sceneMgr().referencePosition());}).setHelp(
			"Mirror around the y axis");
	core::Command::registerCommand("mirrorz",
			[&] (const core::CmdArgs& args) {setMirrorAxis(math::Axis::Z, sceneMgr().referencePosition());}).setHelp(
			"Mirror around the z axis");
	core::Command::registerCommand("mirrornone",
			[&] (const core::CmdArgs& args) {setMirrorAxis(math::Axis::None, sceneMgr().referencePosition());}).setHelp(
			"Disable mirror axis");

	core::Command::registerCommand("+actionexecute",
			[&] (const core::CmdArgs& args) {aabbStart();}).setHelp(
			"Place a voxel to the current cursor position");
	core::Command::registerCommand("-actionexecute", [&] (const core::CmdArgs& args) {
		LayerManager& layerMgr = sceneMgr().layerMgr();
		const int layerId = layerMgr.activeLayer();
		voxel::RawVolume* volume = sceneMgr().volume(layerId);
		aabbEnd(volume, [&] (const voxel::Region& region) {
			sceneMgr().modified(layerId, region);
			sceneMgr().resetLastTrace();
		});
	}).setHelp("Place a voxel to the current cursor position");
}

bool Modifier::init() {
	return _shapeRenderer.init();
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

void Modifier::setCursorPosition(const glm::ivec3& pos) {
	_cursorPosition = pos;
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
	_shapeBuilder.setColor(core::Color::alpha(voxel::getMaterialColor(voxel), 0.7f));
	_shapeBuilder.setPosition(glm::zero<glm::vec3>());
	_shapeBuilder.cube(glm::vec3(-0.01f), glm::vec3(1.01f));
	_shapeRenderer.createOrUpdate(_voxelCursorMesh, _shapeBuilder);
}

}
