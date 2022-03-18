/**
 * @file
 */

#pragma once

#include "video/Camera.h"
#include "video/FrameBuffer.h"
#include "imgui.h"
#include "video/WindowedApp.h"
#include "video/gl/GLTypes.h"
#include "video/Camera.h"
#include "video/FrameBuffer.h"
#include "voxedit-util/ViewportController.h"

namespace voxedit {

class Viewport {
private:
	int32_t _mesh = -1;
	const core::String _id;
	bool _hovered = false;
	video::FrameBuffer _frameBuffer;
	video::TexturePtr _texture;
	voxedit::ViewportController _controller;
	core::VarPtr _debug;
	core::VarPtr _modelSpace;
	core::VarPtr _showAxisVar;

	void renderToFrameBuffer();
	bool setupFrameBuffer(const glm::ivec2& frameBufferSize);
	void renderGizmo(const video::Camera &camera, const int headerSize, const ImVec2 &size);

public:
	Viewport(const core::String& id);
	~Viewport();

	bool isHovered() const;
	void update();
	bool init(ViewportController::RenderMode renderMode = ViewportController::RenderMode::Editor);
	void shutdown();

	const core::String& id() const;

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
