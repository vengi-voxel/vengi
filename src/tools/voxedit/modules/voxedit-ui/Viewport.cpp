/**
 * @file
 */

#include "Viewport.h"
#include "DragAndDropPayload.h"
#include "core/ArrayLength.h"
#include "core/Color.h"
#include "core/Common.h"
#include "core/Var.h"
#include <glm/vector_relational.hpp>
#include "io/Filesystem.h"
#include "math/Ray.h"
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
#include "voxedit-util/modifier/ModifierType.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

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

	_modelSpaceVar = core::Var::getSafe(cfg::VoxEditModelSpace);
	_showAxisVar = core::Var::getSafe(cfg::VoxEditShowaxis);
	_guizmoRotation = core::Var::getSafe(cfg::VoxEditGuizmoRotation);
	_guizmoAllowAxisFlip = core::Var::getSafe(cfg::VoxEditGuizmoAllowAxisFlip);
	_guizmoSnap = core::Var::getSafe(cfg::VoxEditGuizmoSnap);
	return true;
}

void Viewport::updateViewportTrace(float headerSize) {
	const ImVec2 windowPos = ImGui::GetWindowPos();
	const int mouseX = (int)(ImGui::GetIO().MousePos.x - windowPos.x);
	const int mouseY = (int)((ImGui::GetIO().MousePos.y - windowPos.y) - headerSize);
	const bool rotate = sceneMgr().cameraRotate();
	const bool pan = sceneMgr().cameraPan();
	_controller.move(pan, rotate, mouseX, mouseY);
	sceneMgr().setMousePos(_controller._mouseX, _controller._mouseY);
	sceneMgr().setActiveCamera(&_controller.camera());
	sceneMgr().trace();
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
#ifdef VOXEDIT_ANIMATION
		const EditMode editMode = sceneMgr().editMode();
		if (_controller.renderMode() == ViewportController::RenderMode::Animation && editMode != EditMode::Animation) {
			ui::imgui::ScopedStyle style;
			style.setFont(app->bigFont());
			ImGui::TextCentered("No animation loaded");
		} else
#endif
		{
			ImVec2 contentSize = ImGui::GetWindowContentRegionMax();
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
					if ((sceneMgr().modifier().modifierType() & ModifierType::ColorPicker) == ModifierType::ColorPicker) {
						ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
					}
					_hovered = true;
					updateViewportTrace(headerSize);
				}

				if (ImGui::BeginDragDropTarget()) {
					if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(dragdrop::ImagePayload)) {
						const image::ImagePtr &image = *(const image::ImagePtr *)payload->Data;
						updateViewportTrace(headerSize);
						sceneMgr().fillPlane(image);
					}
					if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(dragdrop::ColorPayload)) {
						const int dragPalIdx = *(int *)payload->Data;
						updateViewportTrace(headerSize);
						ModifierFacade &modifier = sceneMgr().modifier();
						modifier.setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, dragPalIdx));
						const int nodeId = sceneMgr().sceneGraph().activeNode();
						modifier.aabbStart();
						modifier.aabbAction(sceneMgr().volume(nodeId),
											[nodeId](const voxel::Region &region, ModifierType type) {
												if (type != ModifierType::Select && type != ModifierType::ColorPicker) {
													sceneMgr().modified(nodeId, region);
												}
											});
						modifier.aabbAbort();
					}
					if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(dragdrop::ModelPayload)) {
						const core::String &filename = *(core::String *)payload->Data;
						sceneMgr().prefab(filename);
					}

					ImGui::EndDragDropTarget();
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
	sceneMgr().render(_controller.camera(), _frameBuffer.dimension(), SceneManager::RenderScene);
	_frameBuffer.unbind();

	uint8_t *pixels;
	if (!video::readTexture(video::TextureUnit::Upload, _texture->type(), _texture->format(), _texture->handle(),
							_texture->width(), _texture->height(), &pixels)) {
		Log::error("Failed to read texture");
		return false;
	}
	// TODO: renderer implementation specific
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
	const ui::imgui::IMGUIApp *app = imguiApp();
	const glm::vec2 windowSize(app->windowDimension());
	const glm::vec2 windowFrameBufferSize(app->frameBufferDimension());
	const glm::vec2 scale = windowFrameBufferSize / windowSize;
	Log::debug("Resize %s to %i:%i (scale %f:%f)", _id.c_str(), frameBufferSize.x, frameBufferSize.y, scale.x, scale.y);
	_controller.resize(frameBufferSize,
						 glm::ivec2((float)frameBufferSize.x * scale.x, (float)frameBufferSize.y * scale.y));
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
	const EditMode editMode = sceneMgr().editMode();

	ImGuiIO &io = ImGui::GetIO();

	if (io.MousePos.x == -FLT_MAX || io.MousePos.y == -FLT_MAX) {
		return;
	}

	const voxelformat::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
	const int activeNode = sceneGraph.activeNode();
	if (activeNode == -1) {
		return;
	}

	ImGuizmo::BeginFrame();

	ImGuizmo::MODE mode = ImGuizmo::MODE::LOCAL;
	if (editMode == EditMode::Scene) {
		ImGuizmo::Enable(true);
	} else {
		ImGuizmo::Enable(false);
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
	if (editMode == EditMode::Scene) {
		const float step = (float)core::Var::getSafe(cfg::VoxEditGridsize)->intVal();
		const float snap[]{step, step, step};
		const uint32_t keyFrameIdx = node.keyFrameForFrame(sceneMgr().currentFrame());
		const voxelformat::SceneGraphTransform &transform = node.transform(keyFrameIdx);
		glm::mat4 localMatrix = transform.localMatrix();
		glm::mat4 deltaMatrix(0.0f);
		const float boundsSnap[] = {1.0f, 1.0f, 1.0f};
		if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))	 {
			_boundsMode ^= true;
		}
		if (_boundsMode) {
			const voxel::Region& region = node.region();
			const glm::vec3 mins = region.getLowerCorner();
			const glm::vec3 maxs = (region.getUpperCorner() + glm::ivec3(1));
			if (_boundsNode.mins != mins) {
				_bounds.mins = _boundsNode.mins = mins;
			}
			if (_boundsNode.maxs != maxs) {
				_bounds.maxs = _boundsNode.maxs = maxs;
			}
			operation = ImGuizmo::SCALEU;
		}

		ImGuizmo::Manipulate(glm::value_ptr(camera.viewMatrix()), glm::value_ptr(camera.projectionMatrix()),
							 (ImGuizmo::OPERATION)operation, mode, glm::value_ptr(localMatrix),
							 glm::value_ptr(deltaMatrix), _guizmoSnap->boolVal() ? snap : nullptr,
							 _boundsMode ? glm::value_ptr(_bounds.mins) : nullptr, _boundsMode ? boundsSnap : nullptr);

		if (editMode == EditMode::Scene) {
			if (ImGuizmo::IsUsing()) {
				_guizmoActivated = true;
				if (_boundsMode) {
					glm::vec3 translate;
					glm::vec3 rotation;
					glm::vec3 scale;
					ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(localMatrix), glm::value_ptr(translate),
														glm::value_ptr(rotation), glm::value_ptr(scale));
					if (glm::all(glm::greaterThan(scale, glm::vec3(0)))) {
						_bounds.maxs = _boundsNode.maxs * scale;
						for (int i = 0; i < 3; ++i) {
							if (_bounds.mins[i] >= _bounds.maxs[i] + boundsSnap[i]) {
								_bounds.maxs[i] = _bounds.mins[i] + boundsSnap[i];
							}
						}
					}
				} else {
					sceneMgr().nodeUpdateTransform(-1, localMatrix, &deltaMatrix, keyFrameIdx, false);
				}
			} else if (_guizmoActivated) {
				if (_boundsMode) {
					voxel::Region newRegion(glm::ivec3(_bounds.mins), glm::ivec3(_bounds.maxs));
					sceneMgr().resize(activeNode, newRegion);
				} else {
					sceneMgr().nodeUpdateTransform(-1, localMatrix, &deltaMatrix, keyFrameIdx, true);
				}
				_guizmoActivated = false;
			}
		}
	}
	glm::mat4 viewMatrix = camera.viewMatrix();
	if (editMode == EditMode::Scene) {
		ImGuizmo::ViewManipulate(glm::value_ptr(viewMatrix), camera.targetDistance(), ImGui::GetWindowPos(),
								 ImVec2(128, 128), 0);
	} else {
		glm::mat4 transformMatrix = glm::mat4(1.0f); // not used
		ImGuizmo::ViewManipulate(glm::value_ptr(viewMatrix), glm::value_ptr(camera.projectionMatrix()),
								 (ImGuizmo::OPERATION)operation, mode, glm::value_ptr(transformMatrix),
								 camera.targetDistance(), ImGui::GetWindowPos(), ImVec2(128, 128), 0);
	}
	if (viewMatrix != camera.viewMatrix()) {
		glm::vec3 scale;
		glm::vec3 translation;
		glm::quat orientation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(viewMatrix, scale, orientation, translation, skew, perspective);
		camera.setOrientation(orientation);
	}
}

void Viewport::renderToFrameBuffer() {
	core_trace_scoped(EditorSceneRenderFramebuffer);
	video::clearColor(core::Color::Clear);
	_frameBuffer.bind(true);
	video::Camera &camera = _controller.camera();
#ifdef VOXEDIT_ANIMATION
	if (_controller.renderMode() == ViewportController::RenderMode::Animation) {
		sceneMgr().renderAnimation(camera);
	} else
#endif
	{
		sceneMgr().render(camera, _frameBuffer.dimension());
	}

	_frameBuffer.unbind();
}

} // namespace voxedit
