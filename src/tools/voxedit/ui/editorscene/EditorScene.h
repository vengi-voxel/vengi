#pragma once

#include "ui/Widget.h"
#include "video/Camera.h"
#include "video/FrameBuffer.h"
#include "frontend/Axis.h"
#include "video/MeshPool.h"
#include "voxel/polyvox/RawVolume.h"
#include "Action.h"
#include "SelectType.h"
#include "Controller.h"

class EditorScene: public ui::Widget {
private:
	using Super = ui::Widget;
	frontend::Axis _axis;
	video::FrameBuffer _frameBuffer;
	tb::UIBitmapGL _bitmap;
	std::vector<EditorScene*> _references;
	EditorScene* _parent = nullptr;
	Controller _controller;
	glm::ivec2 _mousePos;
	std::string _cameraMode;

	void render();

	void setKeyAction(Action action);
	void setInternalAction(Action action);
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

	void setVoxelType(voxel::VoxelType type);

	float cameraSpeed() const;
	void setCameraSpeed(float cameraSpeed);

	bool renderAxis() const;
	void setRenderAxis(bool renderAxis);

	bool renderAABB() const;
	void setRenderAABB(bool renderAABB);

	bool renderGrid() const;
	void setRenderGrid(bool renderGrid);

	void addReference(EditorScene* scene);

	virtual void OnFocusChanged(bool focused) override;
	virtual void OnInflate(const tb::INFLATE_INFO &info) override;
	virtual void OnProcess() override;
	virtual bool OnEvent(const tb::TBWidgetEvent &ev) override;
	virtual void OnPaint(const PaintProps &paintProps) override;
	virtual void OnResized(int oldw, int oldh) override;
};

inline float EditorScene::cameraSpeed() const {
	return _controller.cameraSpeed();
}

inline void EditorScene::setCameraSpeed(float cameraSpeed) {
	_controller.setCameraSpeed(cameraSpeed);
}

inline video::Camera& EditorScene::camera() {
	return _controller.camera();
}
