#pragma once

#include "frontend/Movement.h"
#include "voxel/polyvox/Picking.h"
#include "voxel/polyvox/RawVolume.h"
#include "frontend/RawVolumeRenderer.h"
#include "Action.h"
#include "SelectType.h"

class EditorModel {
private:
	int _initialized = 0;
	float _angle = 0.0f;
	voxel::Voxel _currentVoxel;
	int _size = 32;
	frontend::RawVolumeRenderer _rawVolumeRenderer;
	frontend::RawVolumeRenderer _rawVolumeSelectionRenderer;

	bool actionRequiresExistingVoxel(Action action) const;
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
	void setAction(Action action);
	Action keyAction() const;
	Action uiAction() const;
	float angle() const;
	void setAngle(float angle);

	void setVoxelType(voxel::VoxelType type);
	voxel::RawVolume* modelVolume();
	const voxel::RawVolume* modelVolume() const;

	frontend::RawVolumeRenderer& rawVolumeRenderer();
	const frontend::RawVolumeRenderer& rawVolumeRenderer() const;

	frontend::RawVolumeRenderer& rawVolumeSelectionRenderer();
	const frontend::RawVolumeRenderer& rawVolumeSelectionRenderer() const;

	voxel::PickResult& result();

	bool extractVolume();
	bool extractSelectionVolume();

	void trace(int mouseX, int mouseY, bool skipCursor, const video::Camera& camera);
	void select(const glm::ivec3& pos);
	void executeAction(int32_t x, int32_t y, bool mouseDown, long now);
	void resetLastTrace();

public:
	// TODO: maybe move into scene
	bool _renderAxis = true;
	// TODO: move into scene
	uint8_t _moveMask = 0;

	float _cameraSpeed = 0.1f;
	bool _dirty = false;
	bool _extract = false;
	bool _empty = true;
	bool _selectionExtract = false;
	SelectType _selectionType = SelectType::Single;
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

inline void EditorModel::setAction(Action action) {
	_action = action;
}

inline Action EditorModel::keyAction() const {
	return _keyAction;
}

inline float EditorModel::angle() const {
	return _angle;
}

inline void EditorModel::setAngle(float angle) {
	_angle = angle;
}

inline voxel::RawVolume* EditorModel::modelVolume() {
	return _modelVolume;
}

inline const voxel::RawVolume* EditorModel::modelVolume() const {
	return _modelVolume;
}

inline Action EditorModel::uiAction() const {
	return _uiAction;
}

inline frontend::RawVolumeRenderer& EditorModel::rawVolumeRenderer() {
	return _rawVolumeRenderer;
}

inline const frontend::RawVolumeRenderer& EditorModel::rawVolumeRenderer() const {
	return _rawVolumeRenderer;
}

inline frontend::RawVolumeRenderer& EditorModel::rawVolumeSelectionRenderer() {
	return _rawVolumeSelectionRenderer;
}

inline const frontend::RawVolumeRenderer& EditorModel::rawVolumeSelectionRenderer() const {
	return _rawVolumeSelectionRenderer;
}

inline bool EditorModel::actionRequiresExistingVoxel(Action action) const {
	return action == Action::CopyVoxel || action == Action::DeleteVoxel || action == Action::OverrideVoxel;
}
