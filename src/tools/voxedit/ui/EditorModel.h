#pragma once

#include "frontend/Movement.h"
#include "voxel/polyvox/Picking.h"
#include "voxel/polyvox/RawVolume.h"
#include "frontend/RawVolumeRenderer.h"
#include "Action.h"
#include "SelectionType.h"

class EditorModel {
private:
	int _initialized = 0;
public:
	EditorModel();
	~EditorModel();

	void init();
	void shutdown();

	float _angle = 0.0f;
	bool _renderAxis = true;
	uint8_t _moveMask = 0;

	frontend::RawVolumeRenderer _rawVolumeRenderer;
	frontend::RawVolumeRenderer _rawVolumeSelectionRenderer;

	float _cameraSpeed = 0.1f;
	bool _dirty = false;
	bool _extract = false;
	bool _empty = true;
	bool _selectionExtract = false;
	SelectType _selectionType = SelectType::Single;
	int _size = 32;
	int _lastRaytraceX = -1;
	int _lastRaytraceY = -1;
	long _actionExecutionDelay = 5l;
	long _lastActionExecution = 0l;
	Action _lastAction = Action::None;
	// the action to execute on mouse move
	Action _action = Action::None;
	// the key action - has a higher priority than the ui action
	Action _keyAction = Action::None;
	// action that is selected via ui
	Action _uiAction = Action::PlaceVoxel;
	voxel::PickResult _result;

	voxel::RawVolume* _cursorVolume = nullptr;
	voxel::RawVolume* _cursorPositionVolume = nullptr;
	voxel::RawVolume* _modelVolume = nullptr;
	voxel::Voxel _currentVoxel;
};
