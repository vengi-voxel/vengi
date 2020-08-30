/**
 * @file
 */

#include "core/Common.h"
#include "core/Var.h"
#include "core/Color.h"
#include "ui/turbobadger/UIApp.h"
#include "io/Filesystem.h"
#include "Viewport.h"

#include "voxedit-util/SceneManager.h"
#include "image/Image.h"

Viewport::Viewport() :
		_frameBufferTexture((tb::UIRendererGL*) tb::g_renderer) {
	setIsFocusable(true);
}

bool Viewport::onEvent(const tb::TBWidgetEvent &ev) {
	core_trace_scoped(EditorSceneOnEvent);
	if (ev.type == tb::EVENT_TYPE_POINTER_MOVE && ev.target == this) {
		const bool relative = isRelativeMouseMode();
		const bool middle = isMiddleMouseButtonPressed();
		const bool alt = (ev.modifierkeys & tb::TB_ALT);
		cursorMove(relative || middle || alt, ev.target_x, ev.target_y);
		return true;
	}
	return Super::onEvent(ev);
}

void Viewport::onFocusChanged(bool focused) {
	if (!focused) {
		return;
	}
	voxedit::sceneMgr().setActiveCamera(&_controller.camera());
}

void Viewport::onResized(int oldw, int oldh) {
	core_trace_scoped(EditorSceneOnResized);
	Super::onResized(oldw, oldh);
	const tb::TBRect& rect = getRect();
	const glm::ivec2 frameBufferSize(rect.w, rect.h);
	resize(frameBufferSize);
	_frameBufferTexture.init(frameBufferSize.x, frameBufferSize.y, _texture->handle());
}

void Viewport::renderFramebuffer() {
	// use the uv coords here to take a potential fb flip into account
	const glm::vec4& uv = _frameBuffer.uv();
	const glm::vec2 uva(uv.x, uv.y);
	const glm::vec2 uvc(uv.z, uv.w);
	const glm::ivec2& dimension = _frameBuffer.dimension();
	ui::turbobadger::UIRect rect = getRect();
	const float scaleFactor = video::getScaleFactor();
	rect.x = 0;
	rect.y = 0;
	rect.w = (int)glm::round(rect.w / scaleFactor);
	rect.h = (int)glm::round(rect.h / scaleFactor);
	const float x = uva.x * dimension.x;
	const float y = uva.y * dimension.y;
	const float w = (uvc.x - uva.x) * dimension.x;
	const float h = (uvc.y - uva.y) * dimension.y;
	const tb::TBRect srcRect(x, y, w, h);

	tb::g_renderer->flush();
	video::Shader* shader = nullptr;
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
		const video::Camera& camera = tb::g_renderer->camera();
		const glm::mat4& projectionMatrix = camera.projectionMatrix();
		const int loc = shader->getUniformLocation("u_viewprojection");
		if (loc >= 0) {
			shader->setUniformMatrix(loc, projectionMatrix);
		}
	}
	tb::g_renderer->drawBitmap(rect, srcRect, &_frameBufferTexture);
	tb::g_renderer->flush();
	if (shader != nullptr) {
		shader->deactivate();
		video::useProgram(prevShader);
	}
}

void Viewport::onPaint(const PaintProps &paintProps) {
	core_trace_scoped(EditorSceneOnPaint);
	Super::onPaint(paintProps);

	renderFramebuffer();

	tb::TBFontFace* font = getFont();
	font->drawString(0, 0, tb::TBColor(255.0f, 255.0f, 255.0f, 255.0f), _cameraMode.c_str(), _cameraMode.size());
}

void Viewport::onInflate(const tb::INFLATE_INFO &info) {
	Super::onInflate(info);

	voxedit::ViewportController::SceneCameraMode mode = voxedit::ViewportController::SceneCameraMode::Free;
	const char *cameraMode = info.node->getValueString("camera", "free");
	if (!SDL_strcmp(cameraMode, "top")) {
		mode = voxedit::ViewportController::SceneCameraMode::Top;
	} else if (!SDL_strcmp(cameraMode, "front")) {
		mode = voxedit::ViewportController::SceneCameraMode::Front;
	} else if (!SDL_strcmp(cameraMode, "left")) {
		mode = voxedit::ViewportController::SceneCameraMode::Left;
	}
	_cameraMode = cameraMode;

	voxedit::ViewportController::RenderMode renderMode = voxedit::ViewportController::RenderMode::Editor;
	const char *renderModeStr = info.node->getValueString("mode", "editor");
	if (!SDL_strcmp(renderModeStr, "animation")) {
		renderMode = voxedit::ViewportController::RenderMode::Animation;
	}
	init(mode, renderMode);
}

void Viewport::onProcess() {
	Super::onProcess();
	if (!getVisibilityCombined()) {
		return;
	}
	core_trace_scoped(EditorSceneOnProcess);

	const double deltaFrameSeconds = core::App::getInstance()->deltaFrameSeconds();
	_controller.update(deltaFrameSeconds);

	if (tb::TBWidget::hovered_widget == this) {
		voxedit::sceneMgr().trace();
	}

	renderToFrameBuffer();
}
