/**
 * @file
 */

#pragma once

#include "video/Camera.h"
#include "video/FrameBuffer.h"
#include "video/WindowedApp.h"
#include "video/gl/GLTypes.h"
#include "voxedit-util/AbstractViewport.h"
#include "voxedit-util/ViewportController.h"

namespace voxedit {

class Viewport : public voxedit::AbstractViewport {
private:
	using Super = voxedit::AbstractViewport;
	video::Id _frameBufferTexture = video::InvalidId;
	core::String _cameraMode;
	video::WindowedApp *_app;
	void renderFramebuffer();

public:
	Viewport(video::WindowedApp *app);
	virtual ~Viewport() {
	}

	void update() override;
	bool init() override;
};

}
