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

namespace voxedit {

Viewport::Viewport(video::WindowedApp *app) : _app(app) {
}

bool Viewport::init() {
	if (!Super::init()) {
		Log::error("Failed to initialize abstract viewport");
		return false;
	}
	resize(_app->frameBufferDimension());
	resetCamera();
	voxedit::sceneMgr().setActiveCamera(&_controller.camera());
	return true;
}

void Viewport::update() {
	Super::update();
	ImGui::SetNextWindowSize(ImGui::GetWindowSize());
	_hovered = false;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	if (ImGui::Begin(_cameraMode.c_str(), nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs)) {
		const ImVec2& windowPos = ImGui::GetWindowPos();
		const double deltaFrameSeconds = app::App::getInstance()->deltaFrameSeconds();
		_controller.update(deltaFrameSeconds);
		const ImVec2& windowSize = ImGui::GetWindowSize();
		resize(glm::ivec2(windowSize.x, windowSize.y));

		const bool relative = video::WindowedApp::getInstance()->isRelativeMouseMode();
		const bool alt = ImGui::GetIO().KeyAlt;
		const bool middle = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
		const int mouseX = (int)ImGui::GetIO().MousePos.x - windowPos.x;
		const int mouseY = (int)ImGui::GetIO().MousePos.y - windowPos.y;
		cursorMove(relative || middle || alt, mouseX, mouseY);
		renderToFrameBuffer();

		// use the uv coords here to take a potential fb flip into account
		const glm::vec4 &uv = _frameBuffer.uv();
		const glm::vec2 uva(uv.x, uv.y);
		const glm::vec2 uvc(uv.z, uv.w);
		const video::TexturePtr &texture = _frameBuffer.texture(video::FrameBufferAttachment::Color0);

#if 0 // TODO: this doesn't work, as we don't render directly in dearimgui - but only collect render commands
		video::Shader *shader = nullptr;
		video::Id prevShader = video::InvalidId;
		switch (_controller.shaderType()) {
		case voxedit::ViewportController::ShaderType::Edge:
			shader = &_edgeShader;
			break;
		default:
			break;
		}
		if (shader != nullptr) {
			prevShader = video::getProgram();
			shader->activate();
			const glm::mat4 &projectionMatrix = camera().projectionMatrix();
			const int loc = shader->getUniformLocation("u_viewprojection");
			if (loc >= 0) {
				shader->setUniformMatrix(loc, projectionMatrix);
			}
		}
#endif
		ImGui::Image(texture->handle(), windowSize, uva, uvc);
#if 0
		if (shader != nullptr) {
			shader->deactivate();
			video::useProgram(prevShader);
		}
#endif
		if (ImGui::IsItemHovered()) {
			_hovered = true;
			voxedit::sceneMgr().setActiveCamera(&_controller.camera());
			voxedit::sceneMgr().trace();
		}
	}
	ImGui::End();
	ImGui::PopStyleVar(3);
}

}
