/**
 * @file
 */

#pragma once

#include "ui/turbobadger/Widget.h"
#include "video/Camera.h"
#include "video/FrameBuffer.h"
#include "voxedit-util/ViewportController.h"
#include "RenderShaders.h"

/**
 * @brief Voxel editor scene management like input, ui and rendering.
 * @see voxedit::ViewportController
 */
class Viewport: public ui::turbobadger::Widget {
private:
	shader::EdgeShader& _edgeShader;
	video::FrameBuffer _frameBuffer;
	tb::UIBitmapGL _frameBufferTexture;
	voxedit::ViewportController _controller;
	core::String _cameraMode;

	void renderFramebuffer();

public:
	UIWIDGET_SUBCLASS(Viewport, ui::turbobadger::Widget);

	Viewport();
	~Viewport();

	video::Camera& camera();
	void update();
	void resetCamera();
	bool saveImage(const char* filename);

	voxedit::ViewportController& controller();

	virtual void onInflate(const tb::INFLATE_INFO &info) override;
	virtual void onProcess() override;
	virtual bool onEvent(const tb::TBWidgetEvent &ev) override;
	virtual void onPaint(const PaintProps &paintProps) override;
	virtual void onResized(int oldw, int oldh) override;
	virtual void onFocusChanged(bool focused) override;
};

inline voxedit::ViewportController& Viewport::controller() {
	return _controller;
}

inline video::Camera& Viewport::camera() {
	return _controller.camera();
}
