#pragma once

#include "ui/Widget.h"
#include "video/Camera.h"
#include "video/FrameBuffer.h"
#include "frontend/Axis.h"
#include "video/MeshPool.h"
#include "voxel/polyvox/RawVolume.h"
#include "voxel/generator/LSystemGenerator.h"
#include "voxel/generator/TreeGenerator.h"
#include "Action.h"
#include "Shape.h"
#include "Axis.h"
#include "SelectType.h"
#include "Controller.h"

class EditorScene: public ui::Widget {
private:
	using Super = ui::Widget;
	frontend::Axis _axis;
	video::FrameBuffer _frameBuffer;
	tb::UIBitmapGL _bitmap;
	voxedit::Controller _controller;
	glm::ivec2 _mousePos;
	std::string _cameraMode;

	void render();

	void setKeyAction(voxedit::Action action);
	void setInternalAction(voxedit::Action action);
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

	void rotate(int angleX, int angleY, int angleZ);
	void move(int x, int y, int z);

	void unselectAll();
	void select(const glm::ivec3& pos);

	void lsystem(const voxel::lsystem::LSystemContext& ctx);
	void createTree(const voxel::TreeContext& ctx);

	const glm::ivec3& cursorPosition() const;
	void setCursorPosition(const glm::ivec3& pos);

	voxedit::SelectType selectionType() const;
	void setSelectionType(voxedit::SelectType type);

	voxedit::Shape cursorShape() const;
	void setCursorShape(voxedit::Shape type);
	void scaleCursorShape(const glm::vec3& scale);

	void setActionExecutionDelay(long actionExecutionDelay);
	long actionExecutionDelay() const;

	void setAction(voxedit::Action action);

	void setVoxelType(voxel::VoxelType type);

	voxedit::Axis lockedAxis() const;
	void setLockedAxis(voxedit::Axis axis, bool unlock);

	float cameraSpeed() const;
	void setCameraSpeed(float cameraSpeed);

	bool renderAxis() const;
	void setRenderAxis(bool renderAxis);

	bool renderAABB() const;
	void setRenderAABB(bool renderAABB);

	bool renderGrid() const;
	void setRenderGrid(bool renderGrid);

	void copy();
	void paste();
	void cut();
	void undo();
	void redo();
	bool canUndo() const;
	bool canRedo() const;

	void crop();
	void extend(int size = 1);

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
