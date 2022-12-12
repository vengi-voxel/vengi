/**
 * @file
 */

#include "Viewport.h"
#include "DragAndDropPayload.h"
#include "core/ArrayLength.h"
#include "core/Color.h"
#include "core/Common.h"
#include "core/Var.h"
#include "image/Image.h"
#include "io/Filesystem.h"
#include "math/Ray.h"
#include "ui/IMGUIApp.h"
#include "ui/IMGUIEx.h"
#include "ui/ScopedStyle.h"
#include "ui/dearimgui/ImGuizmo.h"
#include "video/Camera.h"
#include "video/ShapeBuilder.h"
#include "video/WindowedApp.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/vector_relational.hpp>

namespace voxedit {

Viewport::Viewport(const core::String &id) : _id(id) {
}

Viewport::~Viewport() {
	shutdown();
}

bool Viewport::init(Viewport::RenderMode renderMode) {
	_rotationSpeed = core::Var::getSafe(cfg::ClientMouseRotationSpeed);
	_showAxisVar = core::Var::getSafe(cfg::VoxEditShowaxis);
	_guizmoRotation = core::Var::getSafe(cfg::VoxEditGuizmoRotation);
	_guizmoAllowAxisFlip = core::Var::getSafe(cfg::VoxEditGuizmoAllowAxisFlip);
	_guizmoSnap = core::Var::getSafe(cfg::VoxEditGuizmoSnap);
	_viewDistance = core::Var::getSafe(cfg::VoxEditViewdistance);

	setRenderMode(renderMode);
	setMode(Viewport::SceneCameraMode::Free);
	resetCamera();

	return true;
}

void Viewport::resetCamera(const glm::ivec3 &pos, const voxel::Region &region) {
	_camera.setRotationType(video::CameraRotationType::Target);
	_camera.setAngles(0.0f, 0.0f, 0.0f);
	_camera.setFarPlane(_viewDistance->floatVal());
	_camera.setTarget(pos);
	const float distance = 100.0f;
	glm::ivec3 center = pos;
	if (region.isValid()) {
		center = region.getCenter();
	}
	_camera.setTargetDistance(distance);
	if (_camMode == SceneCameraMode::Free) {
		const int height = region.getHeightInCells();
		_camera.setWorldPosition(glm::vec3(-distance, (float)height + distance, -distance));
	} else if (_camMode == SceneCameraMode::Top) {
		const int height = region.getHeightInCells();
		_camera.setWorldPosition(glm::vec3(center.x, height + center.y, center.z));
	} else if (_camMode == SceneCameraMode::Left) {
		_camera.setWorldPosition(glm::vec3(-center.x, center.y, center.z));
	} else if (_camMode == SceneCameraMode::Front) {
		const int depth = region.getDepthInCells();
		_camera.setWorldPosition(glm::vec3(center.x, center.y, -depth - center.z));
	}
}

void Viewport::update(double deltaFrameSeconds) {
	_camera.update(deltaFrameSeconds);
}

void Viewport::setMode(Viewport::SceneCameraMode mode) {
	_camMode = mode;
	if (mode == Viewport::SceneCameraMode::Free) {
		_camera.setMode(video::CameraMode::Perspective);
	} else {
		_camera.setMode(video::CameraMode::Orthogonal);
	}
}

void Viewport::resize(const glm::ivec2&, const glm::ivec2& windowSize) {
	_camera.setSize(windowSize);
}

void Viewport::move(bool pan, bool rotate, int x, int y) {
	if (rotate) {
		const float yaw = (float)(x - _mouseX);
		const float pitch = (float)(y - _mouseY);
		const float s = _rotationSpeed->floatVal();
		if (_camMode == SceneCameraMode::Free) {
			_camera.turn(yaw * s);
			_camera.setPitch(pitch * s);
		}
	} else if (pan) {
		_camera.pan(x - _mouseX, y - _mouseY);
	}
	_mouseX = x;
	_mouseY = y;
}

void Viewport::updateViewportTrace(float headerSize) {
	const ImVec2 windowPos = ImGui::GetWindowPos();
	const int mouseX = (int)(ImGui::GetIO().MousePos.x - windowPos.x);
	const int mouseY = (int)((ImGui::GetIO().MousePos.y - windowPos.y) - headerSize);
	const bool rotate = sceneMgr().cameraRotate();
	const bool pan = sceneMgr().cameraPan();
	move(pan, rotate, mouseX, mouseY);
	sceneMgr().setMousePos(_mouseX, _mouseY);
	sceneMgr().setActiveCamera(&camera());
	sceneMgr().trace();
}

void Viewport::update() {
	static const char *polygonModes[] = {"Points", "Lines", "Solid"};
	static_assert(lengthof(polygonModes) == (int)video::PolygonMode::Max, "Array size doesn't match enum values");

	static const char *camRotTypes[] = {"Reference Point", "Eye"};
	static_assert(lengthof(camRotTypes) == (int)video::CameraRotationType::Max, "Array size doesn't match enum values");

	_camera.setFarPlane(_viewDistance->floatVal());

	_hovered = false;
	ui::ScopedStyle style;
	style.setWindowRounding(0.0f);
	style.setWindowBorderSize(0.0f);
	style.setWindowPadding(ImVec2(0.0f, 0.0f));
	const int sceneWindowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
	if (ImGui::Begin(_id.c_str(), nullptr, sceneWindowFlags)) {
		core_trace_scoped(Viewport);
		ui::IMGUIApp *app = imguiApp();
		{
			glm::ivec2 contentSize = ImGui::GetWindowContentRegionMax();
			const float headerSize = ImGui::GetCursorPosY();

			if (setupFrameBuffer(contentSize)) {
				const double deltaFrameSeconds = app->deltaFrameSeconds();
				update(deltaFrameSeconds);

				renderToFrameBuffer();
				// use the uv coords here to take a potential fb flip into account
				const glm::vec4 &uv = _frameBuffer.uv();
				const glm::vec2 uva(uv.x, uv.y);
				const glm::vec2 uvc(uv.z, uv.w);
				const video::TexturePtr &texture = _frameBuffer.texture(video::FrameBufferAttachment::Color0);
				ImGui::Image(texture->handle(), contentSize, uva, uvc);
				renderGizmo(camera(), headerSize, contentSize);

				if (sceneMgr().isLoading()) {
					ImGui::LoadingIndicatorCircle("Loading", 150, core::Color::White, core::Color::Gray);
				} else if (ImGui::IsItemHovered()) {
					if (sceneMgr().modifier().isMode(ModifierType::ColorPicker)) {
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
						modifier.setCursorVoxel(voxel::createVoxel(dragPalIdx));
						const int nodeId = sceneMgr().sceneGraph().activeNode();
						modifier.aabbStart();
						modifier.aabbAction(sceneMgr().volume(nodeId),
											[nodeId](const voxel::Region &region, ModifierType type, bool markUndo) {
												if (type != ModifierType::Select && type != ModifierType::ColorPicker) {
													sceneMgr().modified(nodeId, region, markUndo);
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

				const float height = ImGui::GetFrameHeight();
				const ImVec2 windowSize = ImGui::GetWindowSize();
				ImGui::SetCursorPos(ImVec2(0.0f, windowSize.y - height));
				const int currentCamRotType = (int)camera().rotationType();
				ImGui::SetNextItemWidth(ImGui::CalcComboBoxWidth(camRotTypes[currentCamRotType]));
				if (ImGui::BeginCombo("##referencepoint", camRotTypes[currentCamRotType])) {
					for (int n = 0; n < lengthof(camRotTypes); n++) {
						const bool isSelected = (currentCamRotType == n);
						if (ImGui::Selectable(camRotTypes[n], isSelected)) {
							camera().setRotationType((video::CameraRotationType)n);
						}
						if (isSelected) {
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
				const int currentPolygonMode = (int)camera().polygonMode();
				const float polygonModeMaxWidth = ImGui::CalcComboBoxWidth(polygonModes[currentPolygonMode]);
				ImGui::SetCursorPos(ImVec2(windowSize.x - polygonModeMaxWidth, windowSize.y - height));
				ImGui::SetNextItemWidth(polygonModeMaxWidth);
				if (ImGui::BeginCombo("##polygonmode", polygonModes[currentPolygonMode])) {
					for (int n = 0; n < lengthof(polygonModes); n++) {
						const bool isSelected = (currentPolygonMode == n);
						if (ImGui::Selectable(polygonModes[n], isSelected)) {
							camera().setPolygonMode((video::PolygonMode)n);
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
	sceneMgr().render(camera(), _frameBuffer.dimension(), SceneManager::RenderScene);
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
	resetCamera(pos, region);
}

bool Viewport::setupFrameBuffer(const glm::ivec2 &frameBufferSize) {
	if (frameBufferSize.x <= 0 || frameBufferSize.y <= 0) {
		return false;
	}
	if (_texture && _texture->width() == frameBufferSize.x && _texture->height() == frameBufferSize.y) {
		return true;
	}
	const ui::IMGUIApp *app = imguiApp();
	const glm::vec2 windowSize(app->windowDimension());
	const glm::vec2 windowFrameBufferSize(app->frameBufferDimension());
	const glm::vec2 scale = windowFrameBufferSize / windowSize;
	Log::debug("Resize %s to %i:%i (scale %f:%f)", _id.c_str(), frameBufferSize.x, frameBufferSize.y, scale.x, scale.y);
	resize(frameBufferSize,
						 glm::ivec2((float)frameBufferSize.x * scale.x, (float)frameBufferSize.y * scale.y));
	_frameBuffer.shutdown();

	video::FrameBufferConfig cfg;
	cfg.dimension(frameBufferSize).depthBuffer(true).colorTexture(true);
	_frameBuffer.init(cfg);

	_texture = _frameBuffer.texture(video::FrameBufferAttachment::Color0);
	return true;
}

void Viewport::renderSceneGuizmo(video::Camera &camera) {
	const EditMode editMode = sceneMgr().editMode();
	if (editMode != EditMode::Scene) {
		return;
	}
	const voxelformat::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
	const int activeNode = sceneGraph.activeNode();
	if (activeNode == -1) {
		return;
	}
	voxelformat::SceneGraphNode &node = sceneGraph.node(activeNode);

	int operation = ImGuizmo::TRANSLATE | ImGuizmo::BOUNDS | ImGuizmo::SCALE;
	if (_guizmoRotation->boolVal()) {
		operation |= ImGuizmo::ROTATE;
	}
	const float step = core::Var::getSafe(cfg::VoxEditGridsize)->floatVal();
	const float snap[]{step, step, step};
	const uint32_t keyFrameIdx = node.keyFrameForFrame(sceneMgr().currentFrame());
	const voxelformat::SceneGraphTransform &transform = node.transform(keyFrameIdx);
	glm::mat4 localMatrix = transform.localMatrix();
	// TODO: pivot handling
	// glm::vec3 dim = node.region().getDimensionsInVoxels();
	// dim *= transform.pivot();
	// localMatrix = glm::translate(localMatrix, dim);
	glm::mat4 deltaMatrix(0.0f);
	const float boundsSnap[] = {1.0f, 1.0f, 1.0f};

	const voxel::Region &region = node.region();
	const glm::vec3 size = region.getDimensionsInVoxels();
	if (_boundsNode.maxs != size) {
		_bounds.maxs = _boundsNode.maxs = size;
	}

	const bool manipulated = ImGuizmo::Manipulate(
		glm::value_ptr(camera.viewMatrix()), glm::value_ptr(camera.projectionMatrix()), (ImGuizmo::OPERATION)operation,
		ImGuizmo::MODE::LOCAL, glm::value_ptr(localMatrix), glm::value_ptr(deltaMatrix), _guizmoSnap->boolVal() ? snap : nullptr,
		glm::value_ptr(_bounds.mins), boundsSnap);

	if (editMode == EditMode::Model) {
		return;
	}

	const bool guizmoActive = ImGuizmo::IsUsing();
	_hoveredGuizmoLastFrame = ImGuizmo::IsOver() || guizmoActive;

	if (guizmoActive) {
		_guizmoActivated = true;
		glm::vec3 translate;
		glm::vec3 rotation;
		glm::vec3 scale;
		ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(localMatrix), glm::value_ptr(translate),
											  glm::value_ptr(rotation), glm::value_ptr(scale));
		if (glm::all(glm::greaterThan(scale, glm::vec3(0)))) {
			_bounds.maxs = _boundsNode.maxs * scale;
		}
	} else if (_guizmoActivated) {
		const voxel::Region &region = node.region();
		const voxel::Region newRegion(region.getLowerCorner(),
									  region.getLowerCorner() + glm::ivec3(glm::ceil(_bounds.maxs)));
		if (newRegion.isValid() && region != newRegion) {
			sceneMgr().resize(node.id(), newRegion);
		}
		_guizmoActivated = false;
	}
	if (manipulated) {
		sceneMgr().nodeUpdateTransform(-1, localMatrix, &deltaMatrix, keyFrameIdx);
	}
}

void Viewport::renderCameraManipulator(video::Camera &camera) {
	const EditMode editMode = sceneMgr().editMode();
	const ImVec2 position = ImGui::GetWindowPos();
	const ImVec2 size = ImVec2(128, 128);
	const ImU32 backgroundColor = 0;
	const float length = camera.targetDistance();

	glm::mat4 viewMatrix = camera.viewMatrix();
	float *viewPtr = glm::value_ptr(viewMatrix);

	if (editMode == EditMode::Scene) {
		ImGuizmo::ViewManipulate(viewPtr, length, position, size, backgroundColor);
	} else {
		const float *projPtr = glm::value_ptr(camera.projectionMatrix());
		const ImGuizmo::OPERATION operation = (ImGuizmo::OPERATION)0;
		glm::mat4 transformMatrix = glm::mat4(1.0f); // not used
		float *matrixPtr = glm::value_ptr(transformMatrix);
		const ImGuizmo::MODE mode = ImGuizmo::MODE::LOCAL;
		ImGuizmo::ViewManipulate(viewPtr, projPtr, operation, mode, matrixPtr, length, position, size, backgroundColor);
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

void Viewport::renderGizmo(video::Camera &camera, const float headerSize, const ImVec2 &size) {
	if (!_showAxisVar->boolVal()) {
		return;
	}

	ImGuiIO &io = ImGui::GetIO();
	if (io.MousePos.x == -FLT_MAX || io.MousePos.y == -FLT_MAX) {
		return;
	}

	const EditMode editMode = sceneMgr().editMode();
	const bool sceneMode = editMode == EditMode::Scene;
	const bool orthographic = camera.mode() == video::CameraMode::Orthogonal;

	ImGuizmo::BeginFrame();
	const ImVec2 &windowPos = ImGui::GetWindowPos();
	ImGuizmo::Enable(sceneMode);
	ImGuizmo::AllowAxisFlip(_guizmoAllowAxisFlip->boolVal());
	ImGuizmo::SetDrawlist();
	ImGuizmo::SetRect(windowPos.x, windowPos.y + headerSize, size.x, size.y);
	ImGuizmo::SetOrthographic(orthographic);
	renderSceneGuizmo(camera);
	renderCameraManipulator(camera);
}

void Viewport::renderToFrameBuffer() {
	core_trace_scoped(EditorSceneRenderFramebuffer);
	video::clearColor(core::Color::Clear);
	_frameBuffer.bind(true);
	sceneMgr().render(camera(), _frameBuffer.dimension());
	_frameBuffer.unbind();
}

} // namespace voxedit
