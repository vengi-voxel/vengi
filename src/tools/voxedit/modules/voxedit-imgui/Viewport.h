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
	video::WindowedApp *_app;
	void renderFramebuffer(const glm::ivec2& size);

public:
	Viewport(video::WindowedApp *app);
	virtual ~Viewport() {
	}

	void update() override;
	bool init() override;
};

}
