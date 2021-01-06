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
	bool _hovered = false;

public:
	Viewport(video::WindowedApp *app);
	virtual ~Viewport() {
	}

	bool isHovered() const;
	void update() override;
	bool init() override;
};

inline bool Viewport::isHovered() const {
	return _hovered;
}

}
