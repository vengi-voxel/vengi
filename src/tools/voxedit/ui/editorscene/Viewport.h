/**
 * @file
 */

#pragma once

#include "ui/turbobadger/Widget.h"
#include "video/Camera.h"
#include "video/FrameBuffer.h"
#include "Controller.h"
#include "RenderShaders.h"

/**
 * @brief Scene management like input, ui and rendering.
 */
class Viewport: public ui::turbobadger::Widget {
private:
	using Super = ui::turbobadger::Widget;
	shader::EdgeShader& _edgeShader;
	video::FrameBuffer _frameBuffer;
	tb::UIBitmapGL _frameBufferTexture;
	voxedit::Controller _controller;
	std::string _cameraMode;

	void renderFramebuffer();

public:
	UIWIDGET_SUBCLASS(Viewport, Super);

	Viewport();
	~Viewport();

	video::Camera& camera();
	void update();
	void resetCamera();
	bool saveImage(const char* filename);

	voxedit::Controller& controller();

	virtual void onInflate(const tb::INFLATE_INFO &info) override;
	virtual void onProcess() override;
	virtual bool onEvent(const tb::TBWidgetEvent &ev) override;
	virtual void onPaint(const PaintProps &paintProps) override;
	virtual void onResized(int oldw, int oldh) override;
	virtual void onFocusChanged(bool focused) override;
};

inline voxedit::Controller& Viewport::controller() {
	return _controller;
}

inline video::Camera& Viewport::camera() {
	return _controller.camera();
}
