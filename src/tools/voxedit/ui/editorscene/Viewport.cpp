/**
 * @file
 */

#include "core/Common.h"
#include "core/Var.h"
#include "core/Color.h"
#include "voxelformat/MeshExporter.h"
#include "ui/turbobadger/UIApp.h"
#include "io/Filesystem.h"
#include "Viewport.h"

#include "../../SceneManager.h"
#include "voxedit-util/ModifierType.h"
#include "image/Image.h"

using namespace voxedit;

Viewport::Viewport() :
		Super(),
		_frameBufferTexture((tb::UIRendererGL*) tb::g_renderer) {
	setIsFocusable(true);
}

Viewport::~Viewport() {
	_frameBuffer.shutdown();
}

bool Viewport::newModel(bool force) {
	core_trace_scoped(EditorSceneNewModel);
	if (!sceneMgr().newScene(force)) {
		return false;
	}
	resetCamera();
	return true;
}

bool Viewport::saveImage(const char* filename) {
	const video::TextureConfig& cfg = _frameBufferTexture._textureConfig;
	core_assert(cfg.format() == video::TextureFormat::RGBA);
	if (cfg.format() != video::TextureFormat::RGBA) {
		return false;
	}
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
	_controller.resetCamera(sceneMgr().region());
}

bool Viewport::onEvent(const tb::TBWidgetEvent &ev) {
	core_trace_scoped(EditorSceneOnEvent);
	if (ev.type == tb::EVENT_TYPE_WHEEL && ev.delta_y != 0) {
		_controller.zoom((float)(ev.delta_y * 100));
		return true;
	}
	if (ev.type == tb::EVENT_TYPE_POINTER_MOVE) {
		const bool relative = isRelativeMouseMode();
		const bool middle = isMiddleMouseButtonPressed();
		const bool alt = (ev.modifierkeys & tb::TB_ALT);
		_controller.move(relative || middle || alt, ev.target_x, ev.target_y);
		sceneMgr().setMousePos(ev.target_x, ev.target_y);
		return true;
	}
	return Super::onEvent(ev);
}

void Viewport::onResized(int oldw, int oldh) {
	core_trace_scoped(EditorSceneOnResized);
	Super::onResized(oldw, oldh);
	const tb::TBRect& rect = getRect();
	const glm::ivec2 dim(rect.w, rect.h);
	_controller.onResize(dim);
	_frameBuffer.shutdown();
	video::TextureConfig textureCfg;
	textureCfg.wrap(video::TextureWrap::ClampToEdge);
	video::FrameBufferConfig cfg;
	cfg.dimension(dim).depthBuffer(true).depthBufferFormat(video::TextureFormat::D24).addTextureAttachment(textureCfg);
	_frameBuffer.init(cfg);
	const video::TexturePtr& fboTexture = _frameBuffer.texture(video::FrameBufferAttachment::Color0);
	_frameBufferTexture.init(dim.x, dim.y, fboTexture->handle());
}

void Viewport::onPaint(const PaintProps &paintProps) {
	core_trace_scoped(EditorSceneOnPaint);
	Super::onPaint(paintProps);
	const glm::ivec2& dimension = _frameBuffer.dimension();
	ui::turbobadger::UIRect rect = getRect();
	rect.x = 0;
	rect.y = 0;
	const glm::vec4& uv = _frameBuffer.uv();
	const glm::vec2 uva(uv.x, uv.y);
	const glm::vec2 uvc(uv.z, uv.w);

	const float x = uva.x * dimension.x;
	const float y = uva.y * dimension.y;

	const float w = (uvc.x - uva.x) * dimension.x;
	const float h = (uvc.y - uva.y) * dimension.y;

	const tb::TBRect srcRect(x, y, w, h);
	tb::g_renderer->drawBitmap(rect, srcRect, &_frameBufferTexture);
	tb::TBFontFace* font = getFont();
	font->drawString(0, 0, tb::TBColor(255.0f, 255.0f, 255.0f, 255.0f), _cameraMode.c_str());
}

void Viewport::onInflate(const tb::INFLATE_INFO &info) {
	Super::onInflate(info);

	Controller::SceneCameraMode mode = Controller::SceneCameraMode::Free;
	const char *cameraMode = info.node->getValueString("camera", "free");
	if (!strcmp(cameraMode, "top")) {
		mode = Controller::SceneCameraMode::Top;
	} else if (!strcmp(cameraMode, "front")) {
		mode = Controller::SceneCameraMode::Front;
	} else if (!strcmp(cameraMode, "left")) {
		mode = Controller::SceneCameraMode::Left;
	}
	_cameraMode = cameraMode;
	_controller.init(mode);
}

void Viewport::updateStatusBar() {
	if (tb::TBTextField* status = getParentRoot()->getWidgetByIDAndType<tb::TBTextField>("status")) {
		if (sceneMgr().aabbMode()) {
			tb::TBStr str;
			const glm::ivec3& dim = sceneMgr().aabbDim();
			str.setFormatted("w: %i, h: %i, d: %i", dim.x, dim.y, dim.z);
			status->setText(str);
		} else {
			const video::WindowedApp* app = video::WindowedApp::getInstance();
			const ModifierType modifierType = sceneMgr().modifierType();
			const bool deleteVoxels = (modifierType & ModifierType::Delete) == ModifierType::Delete;
			const bool overwrite = (modifierType & ModifierType::Place) == ModifierType::Place && deleteVoxels;
			const bool update = (modifierType & ModifierType::Update) == ModifierType::Update;
			std::string statusText;
			std::string keybindingStr;
			if (overwrite) {
				statusText = tr("Override");
				keybindingStr = app->getKeyBindingsString("actionoverride");
			} else if (deleteVoxels) {
				statusText = tr("Delete");
				keybindingStr = app->getKeyBindingsString("actiondelete");
			} else if (update) {
				statusText = tr("Colorize");
				keybindingStr = app->getKeyBindingsString("actioncolorize");
			} else {
				statusText = tr("Place");
				keybindingStr = app->getKeyBindingsString("actionplace");
			}
			if (!keybindingStr.empty()) {
				statusText += " ";
				statusText += keybindingStr;
			}
			status->setText(statusText.c_str());
		}
	}
}

void Viewport::update() {
	updateStatusBar();
	camera().setTarget(glm::vec3(sceneMgr().referencePosition()));
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
		sceneMgr().trace(_controller.camera());
	}

	video::clearColor(core::Color::Clear);
	core_trace_scoped(EditorSceneRenderFramebuffer);
	_frameBuffer.bind(true);
	sceneMgr().render(_controller.camera());
	_frameBuffer.unbind();
}

namespace tb {
TB_WIDGET_FACTORY(Viewport, TBValue::TYPE_NULL, WIDGET_Z_TOP) {}
}
