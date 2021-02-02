/**
 * @file
 */

#include "AbstractViewport.h"
#include "core/Common.h"
#include "core/Var.h"
#include "core/Color.h"
#include "io/Filesystem.h"

#include "SceneManager.h"
#include "image/Image.h"
#include "video/WindowedApp.h"

namespace voxedit {

AbstractViewport::AbstractViewport() :
		_edgeShader(shader::EdgeShader::getInstance()) {
}

AbstractViewport::~AbstractViewport() {
	shutdown();
}

void AbstractViewport::shutdown() {
	_frameBuffer.shutdown();
	_edgeShader.shutdown();
}

bool AbstractViewport::saveImage(const char* filename) {
	core_assert(_texture->format() == video::TextureFormat::RGBA);
	if (_texture->format() != video::TextureFormat::RGBA) {
		return false;
	}

	core_trace_scoped(EditorSceneRenderFramebuffer);
	_frameBuffer.bind(true);
	voxedit::sceneMgr().render(_controller.camera(), voxedit::SceneManager::RenderScene);
	_frameBuffer.unbind();

	uint8_t *pixels;
	if (!video::readTexture(video::TextureUnit::Upload,
			_texture->type(), _texture->format(), _texture,
			_texture->width(), _texture->height(), &pixels)) {
		return false;
	}
	image::Image::flipVerticalRGBA(pixels, _texture->width(), _texture->height());
	const bool val = image::Image::writePng(filename, pixels, _texture->width(), _texture->height(), 4);
	SDL_free(pixels);
	return val;
}

void AbstractViewport::resetCamera() {
	const voxel::Region& region = voxedit::sceneMgr().region();
	core_assert_msg(region.isValid(), "Scene not properly initialized");
	_controller.resetCamera(region);
}

void AbstractViewport::resize(const glm::ivec2& frameBufferSize) {
	if (_texture && _texture->width() == frameBufferSize.x && _texture->height() == frameBufferSize.y) {
		return;
	}
	const glm::vec2 windowSize(video::WindowedApp::getInstance()->windowDimension());
	const glm::vec2 windowFrameBufferSize(video::WindowedApp::getInstance()->frameBufferDimension());
	const glm::vec2 scale = windowFrameBufferSize / windowSize;
	_controller.onResize(frameBufferSize, glm::ivec2(frameBufferSize.x * scale.x, frameBufferSize.y * scale.y));
	_frameBuffer.shutdown();

	video::FrameBufferConfig cfg;
	cfg.dimension(frameBufferSize).depthBuffer(true).colorTexture(true);
	_frameBuffer.init(cfg);

	_texture = _frameBuffer.texture(video::FrameBufferAttachment::Color0);
}

bool AbstractViewport::init() {
	setMode(ViewportController::SceneCameraMode::Free);
	setRenderMode(ViewportController::RenderMode::Editor);
	resetCamera();

	if (!_edgeShader.setup()) {
		return false;
	}

	video::ScopedShader scoped(_edgeShader);
	_edgeShader.setModel(glm::mat4(1.0f));
	_edgeShader.setTexture(video::TextureUnit::Zero);

	return true;
}

void AbstractViewport::setMode(ViewportController::SceneCameraMode mode) {
	_controller.init(mode);

	if (mode == voxedit::ViewportController::SceneCameraMode::Top) {
		_cameraMode = "top";
	} else if (mode == voxedit::ViewportController::SceneCameraMode::Front) {
		_cameraMode = "front";
	} else if (mode == voxedit::ViewportController::SceneCameraMode::Left) {
		_cameraMode = "left";
	} else {
		_cameraMode = "free";
	}
}

void AbstractViewport::setRenderMode(ViewportController::RenderMode renderMode) {
	_controller.setRenderMode(renderMode);
}

void AbstractViewport::update() {
	if (_controller.renderMode() != ViewportController::RenderMode::Editor) {
		return;
	}
	camera().setTarget(glm::vec3(voxedit::sceneMgr().referencePosition()));
}

void AbstractViewport::cursorMove(bool rotate, int x, int y) {
	_controller.move(rotate, x, y);
	voxedit::SceneManager& sceneMgr = voxedit::sceneMgr();
	sceneMgr.setMousePos(_controller._mouseX, _controller._mouseY);
	sceneMgr.setActiveCamera(&_controller.camera());
}

void AbstractViewport::renderToFrameBuffer() {
	core_trace_scoped(EditorSceneRenderFramebuffer);
	video::clearColor(core::Color::Clear);
	_frameBuffer.bind(true);
	if (_controller.renderMode() == ViewportController::RenderMode::Animation) {
		voxedit::sceneMgr().renderAnimation(_controller.camera());
	} else {
		voxedit::sceneMgr().render(_controller.camera());
	}
	_frameBuffer.unbind();
}

}
