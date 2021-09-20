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

Viewport::Viewport(video::WindowedApp *app, const core::String& id) : _app(app), _id(id), _edgeShader(shader::EdgeShader::getInstance()) {
}

Viewport::~Viewport() {
	shutdown();
}

bool Viewport::init() {
	setMode(ViewportController::SceneCameraMode::Free);
	setRenderMode(ViewportController::RenderMode::Editor);
	resetCamera();

	if (!_edgeShader.setup()) {
		Log::error("Failed to initialize abstract viewport");
		return false;
	}

	video::ScopedShader scoped(_edgeShader);
	_edgeShader.setModel(glm::mat4(1.0f));
	_edgeShader.setTexture(video::TextureUnit::Zero);

	resize(_app->frameBufferDimension());
	resetCamera();
	voxedit::sceneMgr().setActiveCamera(&_controller.camera());
	return true;
}

void Viewport::update() {
	if (_controller.renderMode() != ViewportController::RenderMode::Editor) {
		return;
	}
	camera().setTarget(glm::vec3(voxedit::sceneMgr().referencePosition()));

	ImGui::SetNextWindowSize(ImGui::GetWindowSize());
	_hovered = false;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	if (ImGui::Begin(_id.c_str(), nullptr, ImGuiWindowFlags_NoScrollbar)) {
		const ImVec2& windowPos = ImGui::GetWindowPos();
		const double deltaFrameSeconds = app::App::getInstance()->deltaFrameSeconds();
		_controller.update(deltaFrameSeconds);
		const ImVec2& windowSize = ImGui::GetWindowSize();
		resize(glm::ivec2(windowSize.x, windowSize.y));

		const bool relative = video::WindowedApp::getInstance()->isRelativeMouseMode();
		const bool alt = ImGui::GetIO().KeyAlt;
		const bool middle = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
		const int mouseX = (int)(ImGui::GetIO().MousePos.x - windowPos.x);
		const int mouseY = (int)(ImGui::GetIO().MousePos.y - windowPos.y);
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
		if (ImGui::IsItemFocused() || ImGui::IsItemHovered()) {
			_hovered = true;
			voxedit::sceneMgr().setActiveCamera(&_controller.camera());
			voxedit::sceneMgr().trace();
		}

		if (_controller.renderMode() == ViewportController::RenderMode::Animation) {
			const animation::SkeletonAttribute* skeletonAttributes = sceneMgr().skeletonAttributes();
			for (const animation::SkeletonAttributeMeta* metaIter = skeletonAttributes->metaArray(); metaIter->name; ++metaIter) {
				//const animation::SkeletonAttributeMeta& meta = *metaIter;
				// TODO:
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

void Viewport::resetCamera() {
	const voxel::Region& region = voxedit::sceneMgr().region();
	core_assert_msg(region.isValid(), "Scene not properly initialized");
	_controller.resetCamera(region);
}

void Viewport::resize(const glm::ivec2& frameBufferSize) {
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

void Viewport::setMode(ViewportController::SceneCameraMode mode) {
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

void Viewport::setRenderMode(ViewportController::RenderMode renderMode) {
	_controller.setRenderMode(renderMode);
}

void Viewport::cursorMove(bool rotate, int x, int y) {
	_controller.move(rotate, x, y);
	voxedit::SceneManager& sceneMgr = voxedit::sceneMgr();
	sceneMgr.setMousePos(_controller._mouseX, _controller._mouseY);
	sceneMgr.setActiveCamera(&_controller.camera());
}

void Viewport::renderToFrameBuffer() {
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
