/**
 * @file
 */

#pragma once

#include "ui/turbobadger/Widget.h"
#include "video/Camera.h"
#include "video/FrameBuffer.h"
#include "voxedit-util/ViewportController.h"
#include "voxedit-util/AbstractViewport.h"
#include "RenderShaders.h"

/**
 * @brief Voxel editor scene management like input, ui and rendering.
 * @see voxedit::ViewportController
 */
class Viewport: public voxedit::AbstractViewport, public ui::turbobadger::Widget {
private:
	tb::UIBitmapGL _frameBufferTexture;
	core::String _cameraMode;

	void renderFramebuffer();

public:
	UIWIDGET_SUBCLASS(Viewport, ui::turbobadger::Widget);

	Viewport();

	virtual void onInflate(const tb::INFLATE_INFO &info) override;
	virtual void onProcess() override;
	virtual bool onEvent(const tb::TBWidgetEvent &ev) override;
	virtual void onPaint(const PaintProps &paintProps) override;
	virtual void onResized(int oldw, int oldh) override;
	virtual void onFocusChanged(bool focused) override;
};

UIWIDGET_FACTORY(Viewport, tb::TBValue::TYPE_NULL, tb::WIDGET_Z_TOP)
