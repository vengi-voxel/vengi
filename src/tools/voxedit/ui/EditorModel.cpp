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

void EditorModel::init() {
	if (_initialized > 0) {
		return;
	}
	++_initialized;
	_cursorVolume = new voxel::RawVolume(voxel::Region(0, 1));
	_cursorVolume->setVoxel(0, 0, 0, createVoxel(voxel::VoxelType::Grass1));
}

void EditorModel::shutdown() {
	if (_initialized <= 0) {
		return;
	}
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
