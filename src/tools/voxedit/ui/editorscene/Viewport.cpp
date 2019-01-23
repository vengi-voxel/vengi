/**
 * @file
 */

#include "core/Common.h"
#include "core/Var.h"
#include "core/Color.h"
#include "voxelformat/MeshExporter.h"
#include "ui/turbobadger/UIApp.h"
#include "io/Filesystem.h"
#include "ViewportSingleton.h"
#include "Viewport.h"

using namespace voxedit;

static inline ViewportSingleton& vps() {
	return ViewportSingleton::getInstance();
}

Viewport::Viewport() :
		Super(),
		_frameBufferTexture((tb::UIRendererGL*) tb::g_renderer) {
	SetIsFocusable(true);
	vps().init();
}

Viewport::~Viewport() {
	_frameBuffer.shutdown();
	vps().shutdown();
}

bool Viewport::newModel(bool force) {
	core_trace_scoped(EditorSceneNewModel);
	if (!vps().newVolume(force)) {
		return false;
	}
	resetCamera();
	return true;
}

void Viewport::resetCamera() {
	_controller.resetCamera(vps().modelVolume());
}

bool Viewport::OnEvent(const tb::TBWidgetEvent &ev) {
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
		ViewportSingleton::getInstance().setMousePos(ev.target_x, ev.target_y);
		return true;
	}
	return Super::OnEvent(ev);
}

void Viewport::OnResized(int oldw, int oldh) {
	core_trace_scoped(EditorSceneOnResized);
	Super::OnResized(oldw, oldh);
	const tb::TBRect& rect = GetRect();
	const glm::ivec2 dim(rect.w, rect.h);
	_controller.onResize(dim);
	_frameBuffer.shutdown();
	video::TextureConfig textureCfg;
	textureCfg.wrap(video::TextureWrap::ClampToEdge);
	video::FrameBufferConfig cfg;
	cfg.dimension(dim).depthBuffer(true).depthBufferFormat(video::TextureFormat::D24).addTextureAttachment(textureCfg);
	_frameBuffer.init(cfg);
	const video::TexturePtr& fboTexture = _frameBuffer.texture(video::FrameBufferAttachment::Color0);
	_frameBufferTexture.Init(dim.x, dim.y, fboTexture->handle());
}

void Viewport::OnPaint(const PaintProps &paintProps) {
	core_trace_scoped(EditorSceneOnPaint);
	Super::OnPaint(paintProps);
	const glm::ivec2& dimension = _frameBuffer.dimension();
	ui::turbobadger::UIRect rect = GetRect();
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
	tb::g_renderer->DrawBitmap(rect, srcRect, &_frameBufferTexture);
	tb::TBFontFace* font = GetFont();
	font->DrawString(0, 0, tb::TBColor(255.0f, 255.0f, 255.0f, 255.0f), _cameraMode.c_str());
}

void Viewport::OnInflate(const tb::INFLATE_INFO &info) {
	Super::OnInflate(info);

	Controller::SceneCameraMode mode = Controller::SceneCameraMode::Free;
	const char *cameraMode = info.node->GetValueString("camera", "free");
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
	if (tb::TBTextField* status = GetParentRoot()->GetWidgetByIDAndType<tb::TBTextField>("status")) {
		if (vps().aabbMode()) {
			tb::TBStr str;
			const glm::ivec3& dim = vps().aabbDim();
			str.SetFormatted("w: %i, h: %i, d: %i", dim.x, dim.y, dim.z);
			status->SetText(str);
		} else {
			status->SetText("-");
		}
	}
}

void Viewport::update() {
	updateStatusBar();
	vps().update();
	camera().setTarget(glm::vec3(vps().referencePosition()));
}

void Viewport::OnProcess() {
	Super::OnProcess();
	if (!GetVisibilityCombined()) {
		return;
	}
	core_trace_scoped(EditorSceneOnProcess);

	const long deltaFrame = core::App::getInstance()->deltaFrame();
	_controller.update(deltaFrame);

	if (tb::TBWidget::hovered_widget == this) {
		vps().trace(_controller.camera());
	}

	video::clearColor(core::Color::Clear);
	core_trace_scoped(EditorSceneRenderFramebuffer);
	_frameBuffer.bind(true);
	vps().render(_controller.camera());
	_frameBuffer.unbind();
}

namespace tb {
TB_WIDGET_FACTORY(Viewport, TBValue::TYPE_NULL, WIDGET_Z_TOP) {}
}
