/**
 * @file
 */

#include "Viewport.h"
#include "core/ArrayLength.h"
#include "core/Color.h"
#include "core/Common.h"
#include "core/Var.h"
#include "imgui.h"
#include "math/Ray.h"
#include "io/Filesystem.h"
#include "ui/imgui/IMGUIApp.h"
#include "ui/imgui/IMGUIEx.h"
#include "ui/imgui/ScopedStyle.h"
#include "ui/imgui/dearimgui/ImGuizmo.h"
#include "video/Camera.h"
#include "video/ShapeBuilder.h"
#include "video/WindowedApp.h"

#include "image/Image.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/ViewportController.h"
#include "voxel/RawVolume.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace voxedit {

Viewport::Viewport(const core::String &id) : _id(id) {
}

Viewport::~Viewport() {
	shutdown();
}

bool Viewport::init(ViewportController::RenderMode renderMode) {
	if (!_controller.init()) {
		Log::error("Failed to initialize the viewport controller");
		return false;
	}
	_controller.setRenderMode(renderMode);
	_controller.setMode(ViewportController::SceneCameraMode::Free);
	resetCamera();

	_modelSpaceVar = core::Var::get(cfg::VoxEditModelSpace, "1");
	_showAxisVar = core::Var::get(cfg::VoxEditShowaxis, "1", "Show the axis", core::Var::boolValidator);
	_guizmoRotation = core::Var::get(cfg::VoxEditGuizmoRotation, "0", "Activate rotations for the guizmo in scene mode", core::Var::boolValidator);
	_guizmoAllowAxisFlip = core::Var::get(cfg::VoxEditGuizmoAllowAxisFlip, "1", "Flip axis or stay along the positive world/local axis", core::Var::boolValidator);
	_guizmoSnap = core::Var::get(cfg::VoxEditGuizmoSnap, "1", "Use the grid size for snap", core::Var::boolValidator);
	return true;
}

void Viewport::update() {
	static const char *polygonModes[] = {"Points", "Lines", "Solid"};
	static_assert(lengthof(polygonModes) == (int)video::PolygonMode::Max, "Array size doesn't match enum values");

	static const char *camRotTypes[] = {"Reference Point", "Eye"};
	static_assert(lengthof(camRotTypes) == (int)video::CameraRotationType::Max, "Array size doesn't match enum values");

	_hovered = false;
	ui::imgui::ScopedStyle style;
	style.setWindowRounding(0.0f);
	style.setWindowBorderSize(0.0f);
	style.setWindowPadding(ImVec2(0.0f, 0.0f));
	const int sceneWindowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
	if (ImGui::Begin(_id.c_str(), nullptr, sceneWindowFlags)) {
		core_trace_scoped(Viewport);
		ui::imgui::IMGUIApp *app = imguiApp();
		const EditMode editMode = sceneMgr().editMode();
		if (_controller.renderMode() == ViewportController::RenderMode::Animation && editMode != EditMode::Animation) {
			ui::imgui::ScopedStyle style;
			style.setFont(app->bigFont());
			ImGui::TextCentered("No animation loaded");
		} else {
			ImVec2 contentSize = ImGui::GetContentRegionMax();
			const float headerSize = ImGui::GetCursorPosY();

			if (setupFrameBuffer(contentSize)) {
				const double deltaFrameSeconds = app->deltaFrameSeconds();
				_controller.update(deltaFrameSeconds);

				renderToFrameBuffer();
				// use the uv coords here to take a potential fb flip into account
				const glm::vec4 &uv = _frameBuffer.uv();
				const glm::vec2 uva(uv.x, uv.y);
				const glm::vec2 uvc(uv.z, uv.w);
				const video::TexturePtr &texture = _frameBuffer.texture(video::FrameBufferAttachment::Color0);
				ImGui::Image(texture->handle(), contentSize, uva, uvc);
				renderGizmo(_controller.camera(), headerSize, contentSize);

				if (sceneMgr().isLoading()) {
					ImGui::LoadingIndicatorCircle("Loading", 150, core::Color::White, core::Color::Gray);
				} else if (ImGui::IsItemHovered()) {
					if (sceneMgr().modifier().modifierType() == ModifierType::ColorPicker) {
						ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
					}
					const ImVec2 windowPos = ImGui::GetWindowPos();
					const int mouseX = (int)(ImGui::GetIO().MousePos.x - windowPos.x);
					const int mouseY = (int)((ImGui::GetIO().MousePos.y - windowPos.y) - headerSize);
					const bool rotate = sceneMgr().cameraRotate();
					const bool pan = sceneMgr().cameraPan();
					_controller.move(pan, rotate, mouseX, mouseY);
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
				if (ImGui::BeginCombo("##polygonmode", polygonModes[currentPolygonMode])) {
					for (int n = 0; n < lengthof(polygonModes); n++) {
						const bool isSelected = (currentPolygonMode == n);
						if (ImGui::Selectable(polygonModes[n], isSelected)) {
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
	}
	ImGui::End();
}

void Viewport::shutdown() {
	_frameBuffer.shutdown();
}

bool Viewport::saveImage(const char *filename) {
	core_assert(_texture->format() == video::TextureFormat::RGBA);
	if (_texture->format() != video::TextureFormat::RGBA) {
		Log::error("Unsupported texture format");
		return false;
	}

	core_trace_scoped(EditorSceneRenderFramebuffer);
	_frameBuffer.bind(true);
	sceneMgr().render(_controller.camera(), SceneManager::RenderScene);
	_frameBuffer.unbind();

	uint8_t *pixels;
	if (!video::readTexture(video::TextureUnit::Upload, _texture->type(), _texture->format(), _texture->handle(),
							_texture->width(), _texture->height(), &pixels)) {
		Log::error("Failed to read texture");
		return false;
	}
	image::Image::flipVerticalRGBA(pixels, _texture->width(), _texture->height());
	const bool val = image::Image::writePng(filename, pixels, _texture->width(), _texture->height(), 4);
	core_free(pixels);
	return val;
}

void Viewport::resetCamera() {
	const glm::ivec3 &pos = sceneMgr().referencePosition();
	const int activeNode = sceneMgr().sceneGraph().activeNode();
	const voxel::RawVolume *v = activeNode != -1 ? sceneMgr().volume(activeNode) : nullptr;
	voxel::Region region;
	if (v != nullptr) {
		region = v->region();
	}
	_controller.resetCamera(pos, region);
}

bool Viewport::setupFrameBuffer(const glm::ivec2 &frameBufferSize) {
	if (frameBufferSize.x <= 0 || frameBufferSize.y <= 0) {
		return false;
	}
	if (_texture && _texture->width() == frameBufferSize.x && _texture->height() == frameBufferSize.y) {
		return true;
	}
	Log::debug("Resize %s to %i:%i", _id.c_str(), frameBufferSize.x, frameBufferSize.y);
	const glm::vec2 windowSize(video::WindowedApp::getInstance()->windowDimension());
	const glm::vec2 windowFrameBufferSize(video::WindowedApp::getInstance()->frameBufferDimension());
	const glm::vec2 scale = windowFrameBufferSize / windowSize;
	_controller.onResize(frameBufferSize, glm::ivec2((float)frameBufferSize.x * scale.x, (float)frameBufferSize.y * scale.y));
	_frameBuffer.shutdown();

	video::FrameBufferConfig cfg;
	cfg.dimension(frameBufferSize).depthBuffer(true).colorTexture(true);
	_frameBuffer.init(cfg);

	_texture = _frameBuffer.texture(video::FrameBufferAttachment::Color0);
	return true;
}

void Viewport::renderGizmo(video::Camera &camera, const float headerSize, const ImVec2 &size) {
	if (!_showAxisVar->boolVal()) {
		return;
	}

	ImGuiIO &io = ImGui::GetIO();

	if (io.MousePos.x == -FLT_MAX || io.MousePos.y == -FLT_MAX) {
		return;
	}

	const voxelformat::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
	const int activeNode = sceneGraph.activeNode();
	if (activeNode == -1) {
		return;
	}
	const EditMode editMode = sceneMgr().editMode();

	ImGuizmo::BeginFrame();

	ImGuizmo::MODE mode;
	if (editMode == EditMode::Scene) {
		ImGuizmo::Enable(true);
		mode = ImGuizmo::MODE::LOCAL;
	} else {
		ImGuizmo::Enable(false);
		mode = _modelSpaceVar->boolVal() ? ImGuizmo::MODE::LOCAL : ImGuizmo::MODE::WORLD;
	}
	int operation = ImGuizmo::TRANSLATE;
	if (_guizmoRotation->boolVal()) {
		operation |= ImGuizmo::ROTATE;
	}
	ImGuizmo::AllowAxisFlip(_guizmoAllowAxisFlip->boolVal());

	voxelformat::SceneGraphNode &node = sceneGraph.node(activeNode);

	ImGuizmo::SetDrawlist();
	ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y + headerSize, size.x, size.y);
	ImGuizmo::SetOrthographic(camera.mode() == video::CameraMode::Orthogonal);
	const float step = (float)core::Var::getSafe(cfg::VoxEditGridsize)->intVal();
	const float snap[]{step, step, step};
	const uint32_t keyFrame = node.keyFrameForFrame(sceneMgr().currentFrame());
	const voxelformat::SceneGraphTransform &transform = node.transform(keyFrame);
	glm::mat4 transformMatrix = transform.matrix();
	glm::mat4 viewMatrix = camera.viewMatrix();
	const float *projMatrix = glm::value_ptr(camera.projectionMatrix());
	ImGuizmo::Manipulate(glm::value_ptr(viewMatrix), projMatrix, (ImGuizmo::OPERATION)operation, mode, glm::value_ptr(transformMatrix), nullptr, _guizmoSnap->boolVal() ? snap: nullptr);
	if (ImGuizmo::IsUsing()) {
		_guizmoActivated = true;
		sceneMgr().nodeUpdateTransform(activeNode, transformMatrix, keyFrame, false);
	} else if (_guizmoActivated) {
		sceneMgr().nodeUpdateTransform(activeNode, transformMatrix, keyFrame, true);
		_guizmoActivated = false;
	}

	// TODO: active me
#if 0
	ImGuizmo::ViewManipulate(glm::value_ptr(viewMatrix), camera.targetDistance(), ImGui::GetWindowPos(),
							 ImVec2(128, 128), 0x10101010);
#if 0
	glm::vec3 scale;
	glm::vec3 rotation;
	glm::vec3 translation;
	ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(viewMatrix), glm::value_ptr(translation), glm::value_ptr(rotation), glm::value_ptr(scale));
	const glm::quat orientation(rotation);
#else
	glm::vec3 scale;
	glm::vec3 translation;
	glm::quat orientation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(viewMatrix, scale, orientation, translation, skew, perspective);
#endif
	camera.setOrientation(orientation);
#endif
}

void Viewport::renderToFrameBuffer() {
	core_trace_scoped(EditorSceneRenderFramebuffer);
	video::clearColor(core::Color::Clear);
	_frameBuffer.bind(true);
	video::Camera &camera = _controller.camera();
	if (_controller.renderMode() == ViewportController::RenderMode::Animation) {
		sceneMgr().renderAnimation(camera);
	} else {
		sceneMgr().render(camera);
	}

	_frameBuffer.unbind();
}

} // namespace voxedit
