#pragma once

#include "ui/Widget.h"
#include "frontend/RawVolumeRenderer.h"
#include "video/Camera.h"
#include "video/FrameBuffer.h"
#include "frontend/Axis.h"
#include "voxel/polyvox/Picking.h"
#include "video/MeshPool.h"

class EditorScene: public ui::Widget {
public:
	enum class Action {
		None,
		PlaceVoxel,
		CopyVoxel,
		SelectVoxels,
		DeleteVoxel,
		OverrideVoxel
	};

	enum class SelectType {
		Single,
		Same,
		LineVertical,
		LineHorizontal,
		Edge,

		Max
	};

private:
	using Super = ui::Widget;
	video::Camera _camera;
	frontend::Axis _axis;
	core::VarPtr _rotationSpeed;
	video::FrameBuffer _frameBuffer;
	frontend::RawVolumeRenderer _rawVolumeRenderer;
	frontend::RawVolumeRenderer _rawVolumeSelectionRenderer;
	voxel::RawVolume* _cursorVolume;
	voxel::RawVolume* _cursorPositionVolume;
	voxel::RawVolume* _modelVolume;
	tb::UIBitmapGL _bitmap;

	float _cameraSpeed = 0.1f;

	voxel::Voxel _currentVoxel;

	bool _dirty = false;
	bool _extract = false;
	bool _empty = true;
	bool _selectionExtract = false;
	bool _renderAxis = true;
	bool _mouseDown = false;
	uint8_t _moveMask = 0;
	SelectType _selectionType = SelectType::Single;

	float _angle = 0.0f;

	int _size = 32;

	int _mouseX = 0;
	int _mouseY = 0;

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

	void executeAction(int32_t x, int32_t y);
	void render();

	bool actionRequiresExistingVoxel(Action action) const;

	void newVolume();
	void setNewVolume(voxel::RawVolume *volume);

	void setInternalAction(Action action);
	const voxel::Voxel& getVoxel(const glm::ivec3& pos) const;
	bool setVoxel(const glm::ivec3& pos, const voxel::Voxel& voxel);
public:
	UIWIDGET_SUBCLASS(EditorScene, Super);

	EditorScene();
	~EditorScene();

	video::Camera& camera();

	void resetCamera();

	bool isDirty() const;
	bool isEmpty() const;
	bool voxelizeModel(const video::MeshPtr& mesh);
	bool saveModel(std::string_view file);
	bool loadModel(std::string_view file);
	bool exportModel(std::string_view file);
	bool newModel(bool force);

	void select(const glm::ivec3& pos);

	SelectType selectionType() const;
	void setSelectionType(SelectType type);

	void setActionExecutionDelay(long actionExecutionDelay);
	long actionExecutionDelay() const;

	void setAction(Action action);
	Action action() const;

	void setVoxelType(voxel::VoxelType type);

	float cameraSpeed() const;
	void setCameraSpeed(float cameraSpeed);

	bool renderAxis() const;
	void setRenderAxis(bool renderAxis);

	bool renderAABB() const;
	void setRenderAABB(bool renderAABB);

	bool renderGrid() const;
	void setRenderGrid(bool renderGrid);

	virtual void OnFocusChanged(bool focused) override;
	virtual void OnInflate(const tb::INFLATE_INFO &info) override;
	virtual void OnProcess() override;
	virtual bool OnEvent(const tb::TBWidgetEvent &ev) override;
	virtual void OnPaint(const PaintProps &paintProps) override;
};

inline bool EditorScene::renderAABB() const {
	return _rawVolumeRenderer.renderAABB();
}

inline void EditorScene::setRenderAABB(bool renderAABB) {
	_rawVolumeRenderer.setRenderAABB(renderAABB);
}

inline bool EditorScene::renderGrid() const {
	return _rawVolumeRenderer.renderGrid();
}

inline void EditorScene::setRenderGrid(bool renderGrid) {
	_rawVolumeRenderer.setRenderGrid(renderGrid);
}

inline long EditorScene::actionExecutionDelay() const {
	return _actionExecutionDelay;
}

inline void EditorScene::setActionExecutionDelay(long actionExecutionDelay) {
	_actionExecutionDelay = actionExecutionDelay;
}

inline bool EditorScene::renderAxis() const {
	return _renderAxis;
}

inline void EditorScene::setRenderAxis(bool renderAxis) {
	_renderAxis = renderAxis;
}

inline float EditorScene::cameraSpeed() const {
	return _cameraSpeed;
}

inline void EditorScene::setCameraSpeed(float cameraSpeed) {
	_cameraSpeed = cameraSpeed;
}

inline bool EditorScene::isDirty() const {
	return _dirty;
}

inline video::Camera& EditorScene::camera() {
	return _camera;
}

inline bool EditorScene::actionRequiresExistingVoxel(Action action) const {
	return action == Action::CopyVoxel || action == Action::DeleteVoxel || action == Action::OverrideVoxel;
}
