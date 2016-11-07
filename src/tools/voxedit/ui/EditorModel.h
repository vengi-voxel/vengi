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

	void onResize(const glm::ivec2& pos, const glm::ivec2& size);

	const voxel::Voxel& currentVoxel() const;

	void init();
	void shutdown();

	const voxel::Voxel& getVoxel(const glm::ivec3& pos) const;
	bool setVoxel(const glm::ivec3& pos, const voxel::Voxel& voxel) const;
	bool dirty() const;
	bool empty() const;
	float size() const;

	void setNewVolume(voxel::RawVolume* volume);

	void render(const video::Camera& camera);

	bool renderAxis() const;
	Action action() const;
	Action keyAction() const;
	Action uiAction() const;
	float angle() const;

	void setVoxelType(voxel::VoxelType type);

	voxel::PickResult& result();

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

inline void EditorModel::setVoxelType(voxel::VoxelType type) {
	Log::info("Change voxel to %i", std::enum_value(type));
	_currentVoxel = voxel::createVoxel(type);
}

inline const voxel::Voxel& EditorModel::currentVoxel() const {
	return _currentVoxel;
}

inline voxel::PickResult& EditorModel::result() {
	return _result;
}

inline bool EditorModel::renderAxis() const {
	return _renderAxis;
}

inline Action EditorModel::action() const {
	return _action;
}

inline Action EditorModel::keyAction() const {
	return _keyAction;
}

inline float EditorModel::angle() const {
	return _angle;
}

inline Action EditorModel::uiAction() const {
	return _uiAction;
}
