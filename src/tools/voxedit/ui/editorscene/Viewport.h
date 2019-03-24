/**
 * @file
 */

#pragma once

#include "ui/turbobadger/Widget.h"
#include "video/Camera.h"
#include "video/FrameBuffer.h"
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

public:
	UIWIDGET_SUBCLASS(Viewport, Super);

	Viewport();
	~Viewport();

	video::Camera& camera();
	void update();
	void resetCamera();
	bool newModel(bool force);
	bool saveImage(const char* filename);

	virtual void onInflate(const tb::INFLATE_INFO &info) override;
	virtual void onProcess() override;
	virtual bool onEvent(const tb::TBWidgetEvent &ev) override;
	virtual void onPaint(const PaintProps &paintProps) override;
	virtual void onResized(int oldw, int oldh) override;
};

inline video::Camera& Viewport::camera() {
	return _controller.camera();
}
