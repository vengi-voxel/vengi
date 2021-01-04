/**
 * @file
 */

#include "Viewport.h"
#include "core/Color.h"
#include "core/Common.h"
#include "core/Var.h"
#include "ui/imgui/IMGUI.h"
#include "io/Filesystem.h"

#include "image/Image.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

Viewport::Viewport(video::WindowedApp *app) : _app(app) {
}

bool Viewport::init() {
	if (!Super::init()) {
		return false;
	}
	resize(_app->frameBufferDimension());
	resetCamera();
	voxedit::sceneMgr().setActiveCamera(&_controller.camera());
	return true;
}

void Viewport::renderFramebuffer() {
	// use the uv coords here to take a potential fb flip into account
	const glm::vec4 &uv = _frameBuffer.uv();
	const glm::vec2 uva(uv.x, uv.y);
	const glm::vec2 uvc(uv.z, uv.w);

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
	const video::TexturePtr &texture = _frameBuffer.texture(video::FrameBufferAttachment::Color0);
	ImGui::Image(texture->handle(), ImGui::GetWindowSize(), uva, uvc);
	if (shader != nullptr) {
		shader->deactivate();
		video::useProgram(prevShader);
	}
}

void Viewport::update() {
	if (ImGui::IsItemActive()) {
		voxedit::sceneMgr().setActiveCamera(&_controller.camera());
	}

	const double deltaFrameSeconds = app::App::getInstance()->deltaFrameSeconds();
	_controller.update(deltaFrameSeconds);

	if (ImGui::IsItemHovered()) {
		voxedit::sceneMgr().trace();
	}

	const bool relative = false; // isRelativeMouseMode();
	const bool middle = ImGui::IsMouseClicked(ImGuiMouseButton_Middle);
	const bool alt = ImGui::GetIO().KeyAlt;
	cursorMove(relative || middle || alt, (int)ImGui::GetIO().MousePos.x, (int)ImGui::GetIO().MousePos.y);

	renderToFrameBuffer();
	renderFramebuffer();

	// tb::TBFontFace* font = getFont();
	// font->drawString(0, 0, tb::TBColor(255.0f, 255.0f, 255.0f, 255.0f), _cameraMode.c_str(), _cameraMode.size());
}

} // namespace voxedit
