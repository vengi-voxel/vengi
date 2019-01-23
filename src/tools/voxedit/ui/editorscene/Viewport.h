/**
 * @file
 */

#pragma once

#include "ui/turbobadger/Widget.h"
#include "video/Camera.h"
#include "video/FrameBuffer.h"
#include "voxel/polyvox/RawVolume.h"
#include "voxel/generator/LSystemGenerator.h"
#include "voxel/TreeContext.h"
#include "voxel/generator/PlantType.h"
#include "voxel/generator/BuildingGeneratorContext.h"
#include "voxel/generator/NoiseGenerator.h"
#include "Controller.h"

/**
 * @brief Scene management like input, ui and rendering.
 */
class Viewport: public ui::turbobadger::Widget {
private:
	using Super = ui::turbobadger::Widget;
	video::FrameBuffer _frameBuffer;
	tb::UIBitmapGL _frameBufferTexture;
	voxedit::Controller _controller;
	std::string _cameraMode;

	void updateStatusBar();
public:
	UIWIDGET_SUBCLASS(Viewport, Super);

	Viewport();
	~Viewport();

	video::Camera& camera();
	void update();
	void resetCamera();
	bool newModel(bool force);

	virtual void OnInflate(const tb::INFLATE_INFO &info) override;
	virtual void OnProcess() override;
	virtual bool OnEvent(const tb::TBWidgetEvent &ev) override;
	virtual void OnPaint(const PaintProps &paintProps) override;
	virtual void OnResized(int oldw, int oldh) override;
};

inline video::Camera& Viewport::camera() {
	return _controller.camera();
}
