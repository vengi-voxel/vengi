/**
 * @file
 */

#include "Viewport.h"
#include "core/ArrayLength.h"
#include "core/Color.h"
#include "core/Common.h"
#include "core/Var.h"
#include "ui/imgui/IMGUI.h"
#include "ui/imgui/IMGUIApp.h"
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

	static const char *cameraModes[] = {"Points", "Lines", "Solid"};
	static_assert(lengthof(cameraModes) == (int)video::PolygonMode::Max, "Array size doesn't match enum values");

	static const char *camRotTypes[] = {"Reference Point", "Eye"};
	static_assert(lengthof(camRotTypes) == (int)video::CameraRotationType::Max, "Array size doesn't match enum values");

	_hovered = false;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	if (ImGui::Begin(_id.c_str(), nullptr, ImGuiWindowFlags_NoScrollbar)) {
		core_trace_scoped(Viewport);

		if (_controller.renderMode() == ViewportController::RenderMode::Animation && sceneMgr().editMode() != EditMode::Animation) {
			ImGui::TextDisabled("No animation loaded");
		} else {
			const ui::imgui::IMGUIApp *app = (ui::imgui::IMGUIApp*)video::WindowedApp::getInstance();
			const double deltaFrameSeconds = app->deltaFrameSeconds();
			_controller.update(deltaFrameSeconds);
			const int headerSize = app->fontSize() + (int)(ImGui::GetStyle().FramePadding.y * 2.0f);

			ImVec2 contentSize = ImGui::GetWindowContentRegionMax();
			contentSize.y -= (float)headerSize;
			resize(contentSize);
			renderToFrameBuffer();
			// use the uv coords here to take a potential fb flip into account
			const glm::vec4 &uv = _frameBuffer.uv();
			const glm::vec2 uva(uv.x, uv.y);
			const glm::vec2 uvc(uv.z, uv.w);
			const video::TexturePtr &texture = _frameBuffer.texture(video::FrameBufferAttachment::Color0);
			ImGui::Image(texture->handle(), contentSize, uva, uvc);

			if (ImGui::IsItemHovered()) {
				const bool alt = ImGui::GetIO().KeyAlt;
				const bool middle = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
				const ImVec2 windowPos = ImGui::GetWindowPos();
				const int mouseX = (int)(ImGui::GetIO().MousePos.x - windowPos.x);
				const int mouseY = (int)(ImGui::GetIO().MousePos.y - windowPos.y) - headerSize;
				const bool rotate = middle || alt;
				_controller.move(rotate, mouseX, mouseY);
				_hovered = true;
				sceneMgr().setMousePos(_controller._mouseX, _controller._mouseY);
				sceneMgr().setActiveCamera(&_controller.camera());
				sceneMgr().trace();
			}

			const float height = (float)app->fontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
			const float maxWidth = 200.0f;
			const ImVec2 windowSize = ImGui::GetWindowSize();
			ImGui::SetCursorPos(ImVec2(0.0f, windowSize.y - height));
			ImGui::SetNextItemWidth(maxWidth);
			const int currentCamRotType = (int)_controller.camera().rotationType();
			if (ImGui::BeginCombo("##referencepoint", camRotTypes[currentCamRotType])) {
				for (int n = 0; n < lengthof(camRotTypes); n++) {
					const bool isSelected = (currentCamRotType == n);
					if (ImGui::Selectable(camRotTypes[n], isSelected)) {
						_controller.camera().setRotationType((video::CameraRotationType)n);
					}
					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			ImGui::SetCursorPos(ImVec2(windowSize.x - maxWidth, windowSize.y - height));
			const int currentPolygonMode = (int)_controller.camera().polygonMode();
			ImGui::SetNextItemWidth(maxWidth);
			if (ImGui::BeginCombo("#cameramode", cameraModes[currentPolygonMode])) {
				for (int n = 0; n < lengthof(cameraModes); n++) {
					const bool isSelected = (currentPolygonMode == n);
					if (ImGui::Selectable(cameraModes[n], isSelected)) {
						_controller.camera().setPolygonMode((video::PolygonMode)n);
					}
					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
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
