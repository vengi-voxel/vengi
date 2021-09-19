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
	const core::String _id;
	bool _hovered = false;

public:
	Viewport(video::WindowedApp *app, const core::String& id);
	virtual ~Viewport() {
	}

	bool isHovered() const;
	void update() override;
	bool init() override;
	const core::String& id() const;
};

inline const core::String& Viewport::id() const {
	return _id;
}

inline bool Viewport::isHovered() const {
	return _hovered;
}

}
