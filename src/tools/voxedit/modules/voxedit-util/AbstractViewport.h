/**
 * @file
 */

#pragma once

#include "video/Camera.h"
#include "video/FrameBuffer.h"
#include "ViewportController.h"
#include "RenderShaders.h"

namespace voxedit {

/**
 * @brief Voxel editor scene management for input rendering.
 * @see voxedit::ViewportController
 */
class AbstractViewport {
protected:
	shader::EdgeShader& _edgeShader;
	video::FrameBuffer _frameBuffer;
	video::TexturePtr _texture;
	voxedit::ViewportController _controller;

	AbstractViewport();
	void renderToFrameBuffer();
	void resize(const glm::ivec2& frameBufferSize);
	void cursorMove(bool, int x, int y);

public:
	~AbstractViewport();

	virtual bool init();
	void setMode(ViewportController::SceneCameraMode mode = ViewportController::SceneCameraMode::Free);
	void setRenderMode(ViewportController::RenderMode renderMode = ViewportController::RenderMode::Editor);
	virtual void shutdown();
	virtual void update();
	void resetCamera();
	bool saveImage(const char* filename);

	video::Camera& camera();
	ViewportController& controller();
};

inline ViewportController& AbstractViewport::controller() {
	return _controller;
}

inline video::Camera& AbstractViewport::camera() {
	return _controller.camera();
}

}
