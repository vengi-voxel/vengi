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

namespace voxedit {

AbstractViewport::AbstractViewport() :
		_edgeShader(shader::EdgeShader::getInstance()) {
}

AbstractViewport::~AbstractViewport() {
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
	_controller.resetCamera(voxedit::sceneMgr().region());
}

void AbstractViewport::resize(const glm::ivec2& frameBufferSize) {
	const float scaleFactor = video::getScaleFactor();
	const glm::ivec2 windowSize(int(frameBufferSize.x / scaleFactor + 0.5f), int(frameBufferSize.y / scaleFactor + 0.5f));
	_controller.onResize(frameBufferSize, windowSize);
	_frameBuffer.shutdown();

	video::FrameBufferConfig cfg;
	cfg.dimension(frameBufferSize).depthBuffer(true).colorTexture(true);
	_frameBuffer.init(cfg);

	_texture = _frameBuffer.texture(video::FrameBufferAttachment::Color0);
}

bool AbstractViewport::init(ViewportController::SceneCameraMode mode, ViewportController::RenderMode renderMode) {
	_controller.init(mode);
	_controller.setRenderMode(renderMode);

	if (!_edgeShader.setup()) {
		return false;
	}

	video::ScopedShader scoped(_edgeShader);
	_edgeShader.setModel(glm::mat4(1.0f));
	_edgeShader.setTexture(video::TextureUnit::Zero);

	return true;
}

void AbstractViewport::update() {
	if (_controller.renderMode() == ViewportController::RenderMode::Editor) {
		camera().setTarget(glm::vec3(voxedit::sceneMgr().referencePosition()));
	}
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
