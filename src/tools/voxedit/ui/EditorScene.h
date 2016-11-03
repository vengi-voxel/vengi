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
		DeleteVoxel,
		OverrideVoxel
	};

private:
	using Super = ui::Widget;
	video::Camera _camera;
	frontend::Axis _axis;
	core::VarPtr _rotationSpeed;
	video::FrameBuffer _frameBuffer;
	frontend::RawVolumeRenderer _rawVolumeRenderer;
	voxel::RawVolume* _cursorVolume;
	voxel::RawVolume* _modelVolume;
	tb::UIBitmapGL _bitmap;

	float _cameraSpeed = 0.1f;

	voxel::Voxel _currentVoxel;

	bool _dirty = false;
	bool _extract = false;
	bool _renderAxis = true;
	uint8_t _moveMask = 0;

	float _angle = 0.0f;

	int _size = 32;

	int _mouseX = 0;
	int _mouseY = 0;

	int _lastRaytraceX = -1;
	int _lastRaytraceY = -1;

	long _actionExecutionDelay = 5l;
	long _lastActionExecution = 0l;
	Action _lastAction = Action::None;

	Action _action = Action::None;
	Action _uiAction = Action::PlaceVoxel;

	voxel::PickResult _result;

	void executeAction(int32_t x, int32_t y);
	void render();

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
	bool voxelizeModel(const video::MeshPtr& mesh);
	bool saveModel(std::string_view file);
	bool loadModel(std::string_view file);
	bool exportModel(std::string_view file);
	bool newModel(bool force);

	void setActionExecutionDelay(long actionExecutionDelay);
	long actionExecutionDelay() const;

	void setAction(Action action);
	Action action() const;

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
