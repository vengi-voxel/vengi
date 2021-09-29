/**
 * @file
 */

#pragma once

#include "video/Camera.h"
#include "video/FrameBuffer.h"
#include "video/WindowedApp.h"
#include "video/gl/GLTypes.h"
#include "video/Camera.h"
#include "video/FrameBuffer.h"
#include "voxedit-util/ViewportController.h"
#include "RenderShaders.h"

namespace voxedit {

class Viewport {
private:
	video::WindowedApp *_app;
	const core::String _id;
	bool _hovered = false;
	shader::EdgeShader& _edgeShader;
	video::FrameBuffer _frameBuffer;
	video::TexturePtr _texture;
	voxedit::ViewportController _controller;

	void renderToFrameBuffer();
	void resize(const glm::ivec2& frameBufferSize);

public:
	Viewport(video::WindowedApp *app, const core::String& id);
	~Viewport();

	bool isHovered() const;
	void update();
	bool init(ViewportController::RenderMode renderMode = ViewportController::RenderMode::Editor);
	void shutdown();

	const core::String& id() const;

	void setMode(ViewportController::SceneCameraMode mode = ViewportController::SceneCameraMode::Free);
	void resetCamera();
	bool saveImage(const char* filename);

	video::FrameBuffer& frameBuffer();
	video::Camera& camera();
	ViewportController& controller();
};

inline const core::String& Viewport::id() const {
	return _id;
}

inline bool Viewport::isHovered() const {
	return _hovered;
}

inline video::FrameBuffer& Viewport::frameBuffer() {
	return _frameBuffer;
}

inline ViewportController& Viewport::controller() {
	return _controller;
}

inline video::Camera& Viewport::camera() {
	return _controller.camera();
}

}
