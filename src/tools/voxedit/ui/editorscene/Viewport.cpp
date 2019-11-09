/**
 * @file
 */

#include "core/Common.h"
#include "core/Var.h"
#include "core/Color.h"
#include "ui/turbobadger/UIApp.h"
#include "core/io/Filesystem.h"
#include "Viewport.h"

#include "voxedit-util/SceneManager.h"
#include "voxedit-util/ModifierType.h"
#include "image/Image.h"

Viewport::Viewport() :
		Super(),
		_edgeShader(shader::EdgeShader::getInstance()),
		_frameBufferTexture((tb::UIRendererGL*) tb::g_renderer) {
	setIsFocusable(true);
}

Viewport::~Viewport() {
	_frameBuffer.shutdown();
	_edgeShader.shutdown();
}

bool Viewport::saveImage(const char* filename) {
	const video::TextureConfig& cfg = _frameBufferTexture._textureConfig;
	core_assert(cfg.format() == video::TextureFormat::RGBA);
	if (cfg.format() != video::TextureFormat::RGBA) {
		return false;
	}

	core_trace_scoped(EditorSceneRenderFramebuffer);
	_frameBuffer.bind(true);
	voxedit::sceneMgr().render(_controller.camera(), voxedit::SceneManager::RenderScene);
	_frameBuffer.unbind();

	uint8_t *pixels;
	if (!video::readTexture(video::TextureUnit::Upload,
			cfg.type(), cfg.format(), _frameBufferTexture._texture,
			_frameBufferTexture._w, _frameBufferTexture._h, &pixels)) {
		return false;
	}
	image::Image::flipVerticalRGBA(pixels, _frameBufferTexture._w, _frameBufferTexture._h);
	const bool val = image::Image::writePng(filename, pixels, _frameBufferTexture._w, _frameBufferTexture._h, 4);
	SDL_free(pixels);
	return val;
}

void Viewport::resetCamera() {
	_controller.resetCamera(voxedit::sceneMgr().region());
}

bool Viewport::onEvent(const tb::TBWidgetEvent &ev) {
	core_trace_scoped(EditorSceneOnEvent);
	if (ev.type == tb::EVENT_TYPE_POINTER_MOVE && ev.target == this) {
		const bool relative = isRelativeMouseMode();
		const bool middle = isMiddleMouseButtonPressed();
		const bool alt = (ev.modifierkeys & tb::TB_ALT);
		_controller.move(relative || middle || alt, ev.target_x, ev.target_y);
		voxedit::SceneManager& sceneMgr = voxedit::sceneMgr();
		sceneMgr.setMousePos(_controller._mouseX, _controller._mouseY);
		sceneMgr.setActiveCamera(&_controller.camera());
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
	const float scaleFactor = video::getScaleFactor();
	const glm::ivec2 windowSize(int(frameBufferSize.x / scaleFactor + 0.5f), int(frameBufferSize.y / scaleFactor + 0.5f));
	_controller.onResize(frameBufferSize, windowSize);
	_frameBuffer.shutdown();
	video::TextureConfig textureCfg;
	textureCfg.wrap(video::TextureWrap::ClampToEdge);
	textureCfg.format(video::TextureFormat::RGBA);
	video::FrameBufferConfig cfg;
	cfg.dimension(frameBufferSize).depthBuffer(true).depthBufferFormat(video::TextureFormat::D24);
	cfg.addTextureAttachment(textureCfg, video::FrameBufferAttachment::Color0);
	_frameBuffer.init(cfg);
	const video::TexturePtr& fboTexture = _frameBuffer.texture(video::FrameBufferAttachment::Color0);
	_frameBufferTexture.init(frameBufferSize.x, frameBufferSize.y, fboTexture->handle());
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
	video::Shader* shader;
	video::Id prevShader = video::InvalidId;
	switch (_controller.shaderType()) {
	case voxedit::ViewportController::ShaderType::Edge:
		shader = &_edgeShader;
		break;
	case voxedit::ViewportController::ShaderType::Max:
	case voxedit::ViewportController::ShaderType::None:
		shader = nullptr;
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
	font->drawString(0, 0, tb::TBColor(255.0f, 255.0f, 255.0f, 255.0f), _cameraMode.c_str());
}

void Viewport::onInflate(const tb::INFLATE_INFO &info) {
	Super::onInflate(info);

	voxedit::ViewportController::SceneCameraMode mode = voxedit::ViewportController::SceneCameraMode::Free;
	const char *cameraMode = info.node->getValueString("camera", "free");
	if (!strcmp(cameraMode, "top")) {
		mode = voxedit::ViewportController::SceneCameraMode::Top;
	} else if (!strcmp(cameraMode, "front")) {
		mode = voxedit::ViewportController::SceneCameraMode::Front;
	} else if (!strcmp(cameraMode, "left")) {
		mode = voxedit::ViewportController::SceneCameraMode::Left;
	}
	_cameraMode = cameraMode;
	_controller.init(mode);
	_edgeShader.setup();

	video::ScopedShader scoped(_edgeShader);
	_edgeShader.setModel(glm::mat4(1.0f));
	_edgeShader.setTexture(video::TextureUnit::Zero);
}

void Viewport::update() {
	camera().setTarget(glm::vec3(voxedit::sceneMgr().referencePosition()));
}

void Viewport::onProcess() {
	Super::onProcess();
	if (!getVisibilityCombined()) {
		return;
	}
	core_trace_scoped(EditorSceneOnProcess);

	const long deltaFrame = core::App::getInstance()->deltaFrame();
	_controller.update(deltaFrame);

	if (tb::TBWidget::hovered_widget == this) {
		voxedit::sceneMgr().trace();
	}

	video::clearColor(core::Color::Clear);
	{
		core_trace_scoped(EditorSceneRenderFramebuffer);
		_frameBuffer.bind(true);
		if (_controller.renderMode() == voxedit::ViewportController::RenderMode::Animation) {
			voxedit::sceneMgr().renderAnimation(_controller.camera());
		} else {
			voxedit::sceneMgr().render(_controller.camera());
		}
		_frameBuffer.unbind();
	}
}

namespace tb {
TB_WIDGET_FACTORY(Viewport, TBValue::TYPE_NULL, WIDGET_Z_TOP) {}
}
