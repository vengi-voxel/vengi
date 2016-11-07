#include "EditorModel.h"

EditorModel::EditorModel() :
		_rawVolumeRenderer(true, false, true), _rawVolumeSelectionRenderer(false, false, false) {
	registerMoveCmd("+move_right", MOVERIGHT);
	registerMoveCmd("+move_left", MOVELEFT);
	registerMoveCmd("+move_forward", MOVEFORWARD);
	registerMoveCmd("+move_backward", MOVEBACKWARD);
}

EditorModel::~EditorModel() {
	shutdown();
	core::Command::unregisterCommand("+move_right");
	core::Command::unregisterCommand("+move_left");
	core::Command::unregisterCommand("+move_upt");
	core::Command::unregisterCommand("+move_down");
}

void EditorModel::setNewVolume(voxel::RawVolume* volume) {
	delete _modelVolume;
	_modelVolume = volume;

	const voxel::Region& region = volume->getEnclosingRegion();
	delete _cursorPositionVolume;
	_cursorPositionVolume = new voxel::RawVolume(region);

	delete _rawVolumeSelectionRenderer.setVolume(new voxel::RawVolume(region));
	delete _rawVolumeRenderer.setVolume(new voxel::RawVolume(region));

	_empty = true;
	_extract = true;
	_dirty = false;
	_lastRaytraceX = _lastRaytraceY = -1;
}

bool EditorModel::dirty() const {
	return _dirty;
}

float EditorModel::size() const {
	return _size;
}

bool EditorModel::empty() const {
	return _empty;
}

const voxel::Voxel& EditorModel::getVoxel(const glm::ivec3& pos) const {
	return _modelVolume->getVoxel(pos);
}

bool EditorModel::setVoxel(const glm::ivec3& pos, const voxel::Voxel& voxel) const {
	return _modelVolume->setVoxel(pos, voxel);
}

void EditorModel::render(const video::Camera& camera) {
	_rawVolumeRenderer.render(camera);
}

void EditorModel::onResize(const glm::ivec2& pos, const glm::ivec2& size) {
	_rawVolumeRenderer.onResize(pos, size);
	_rawVolumeSelectionRenderer.onResize(pos, size);
}

void EditorModel::init() {
	if (_initialized++ > 0) {
		return;
	}
	_cursorVolume = new voxel::RawVolume(voxel::Region(0, 1));
	_cursorVolume->setVoxel(0, 0, 0, createVoxel(voxel::VoxelType::Grass1));
	_rawVolumeRenderer.init();
	_rawVolumeSelectionRenderer.init();
}

void EditorModel::shutdown() {
	if (--_initialized > 0) {
		return;
	}
	delete _cursorPositionVolume;
	_cursorPositionVolume = nullptr;
	delete _cursorVolume;
	_cursorVolume = nullptr;
	delete _modelVolume;
	_modelVolume = nullptr;
	delete _rawVolumeRenderer.shutdown();
	delete _rawVolumeSelectionRenderer.shutdown();
}
