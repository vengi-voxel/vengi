/**
 * @file
 */

#include "Viewport.h"
#include "core/Color.h"
#include "core/Common.h"
#include "core/Var.h"
#include "ui/imgui/IMGUI.h"
#include "io/Filesystem.h"
#include "video/WindowedApp.h"

#include "image/Image.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/ViewportController.h"


namespace voxedit {

Viewport::Viewport(video::WindowedApp *app, const core::String& id) : _app(app), _id(id), _edgeShader(shader::EdgeShader::getInstance()) {
}

Viewport::~Viewport() {
	shutdown();
}

bool Viewport::init(ViewportController::RenderMode renderMode) {
	_controller.setRenderMode(renderMode);
	setMode(ViewportController::SceneCameraMode::Free);
	resetCamera();

	if (!_edgeShader.setup()) {
		Log::error("Failed to initialize viewport");
		return false;
	}

	video::ScopedShader scoped(_edgeShader);
	_edgeShader.setModel(glm::mat4(1.0f));
	_edgeShader.setTexture(video::TextureUnit::Zero);

	resize(_app->frameBufferDimension());
	return true;
}

void Viewport::update() {
	camera().setTarget(glm::vec3(sceneMgr().referencePosition()));

	// TODO: render mode
	// TODO: reference point
	_hovered = false;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	if (ImGui::Begin(_id.c_str(), nullptr, ImGuiWindowFlags_NoScrollbar)) {
		core_trace_scoped(Viewport);
		const glm::ivec2 &contentSize = ImGui::GetWindowContentRegionMax();
		if (_controller.renderMode() == ViewportController::RenderMode::Animation && sceneMgr().editMode() != EditMode::Animation) {
			ImGui::TextDisabled("No animation loaded");
		} else {
			const video::WindowedApp *app = video::WindowedApp::getInstance();
			const double deltaFrameSeconds = app->deltaFrameSeconds();
			_controller.update(deltaFrameSeconds);

			resize(contentSize);

			renderToFrameBuffer();

			// use the uv coords here to take a potential fb flip into account
			const glm::vec4 &uv = _frameBuffer.uv();
			const glm::vec2 uva(uv.x, uv.y);
			const glm::vec2 uvc(uv.z, uv.w);
			const video::TexturePtr &texture = _frameBuffer.texture(video::FrameBufferAttachment::Color0);
			ImGui::Image(texture->handle(), contentSize, uva, uvc);

			if (ImGui::IsItemHovered()) {
				const glm::ivec2 windowPos = ImGui::GetWindowPos();
				const glm::ivec2 windowSize = ImGui::GetWindowSize();
				const glm::ivec2 headerSize = windowSize - contentSize;
				const bool alt = ImGui::GetIO().KeyAlt;
				const bool middle = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
				const int mouseX = (int)(ImGui::GetIO().MousePos.x - windowPos.x);
				const int mouseY = (int)(ImGui::GetIO().MousePos.y - windowPos.y);
				const bool rotate = middle || alt;
				_controller.move(rotate, mouseX, mouseY + headerSize.y);
				_hovered = true;
				sceneMgr().setMousePos(_controller._mouseX, _controller._mouseY);
				sceneMgr().setActiveCamera(&_controller.camera());
				sceneMgr().trace();
			}
		}
	}
	ImGui::End();
	ImGui::PopStyleVar(3);
}

void Viewport::shutdown() {
	_frameBuffer.shutdown();
	_edgeShader.shutdown();
}

bool Viewport::saveImage(const char* filename) {
	core_assert(_texture->format() == video::TextureFormat::RGBA);
	if (_texture->format() != video::TextureFormat::RGBA) {
		return false;
	}

	core_trace_scoped(EditorSceneRenderFramebuffer);
	_frameBuffer.bind(true);
	sceneMgr().render(_controller.camera(), SceneManager::RenderScene);
	_frameBuffer.unbind();

	uint8_t *pixels;
	if (!video::readTexture(video::TextureUnit::Upload,
			_texture->type(), _texture->format(), _texture,
			_texture->width(), _texture->height(), &pixels)) {
		return false;
	}
	image::Image::flipVerticalRGBA(pixels, _texture->width(), _texture->height());
	const bool val = image::Image::writePng(filename, pixels, _texture->width(), _texture->height(), 4);
	core_free(pixels);
	return val;
}

void Viewport::resetCamera() {
	const voxel::Region& region = sceneMgr().region();
	core_assert_msg(region.isValid(), "Scene not properly initialized");
	_controller.resetCamera(region);
}

void Viewport::resize(const glm::ivec2& frameBufferSize) {
	if (_texture && _texture->width() == frameBufferSize.x && _texture->height() == frameBufferSize.y) {
		return;
	}
	Log::debug("Resize %s to %i:%i", _id.c_str(), frameBufferSize.x, frameBufferSize.y);
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

void Viewport::setMode(ViewportController::SceneCameraMode mode) {
	_controller.init(mode);
}

void Viewport::renderToFrameBuffer() {
	core_trace_scoped(EditorSceneRenderFramebuffer);
	video::clearColor(core::Color::Clear);
	_frameBuffer.bind(true);
	if (_controller.renderMode() == ViewportController::RenderMode::Animation) {
		sceneMgr().renderAnimation(_controller.camera());
	} else {
		sceneMgr().render(_controller.camera());
	}
	_frameBuffer.unbind();
}

}
