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
#include "imgui.h"
#include "io/Filesystem.h"
#include "math/Ray.h"
#include "ui/IMGUIApp.h"
#include "ui/IMGUIEx.h"
#include "ui/ScopedStyle.h"
#include "ui/dearimgui/ImGuizmo.h"
#include "video/Camera.h"
#include "video/Renderer.h"
#include "video/ShapeBuilder.h"
#include "video/WindowedApp.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelformat/SceneGraphNode.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/vector_relational.hpp>

namespace voxedit {

core::String Viewport::viewportId(int id) {
	return core::string::format("###viewport%i", id);
}

Viewport::Viewport(int id, bool sceneMode, bool detailedTitle)
	: _id(id), _uiId(viewportId(id)), _detailedTitle(detailedTitle) {
	_renderContext.sceneMode = sceneMode;
}

Viewport::~Viewport() {
	shutdown();
}

bool Viewport::init() {
	_rotationSpeed = core::Var::getSafe(cfg::ClientMouseRotationSpeed);
	_showAxisVar = core::Var::getSafe(cfg::VoxEditShowaxis);
	_guizmoRotation = core::Var::getSafe(cfg::VoxEditGuizmoRotation);
	_guizmoAllowAxisFlip = core::Var::getSafe(cfg::VoxEditGuizmoAllowAxisFlip);
	_guizmoSnap = core::Var::getSafe(cfg::VoxEditGuizmoSnap);
	_guizmoBounds = core::Var::getSafe(cfg::VoxEditGuizmoBounds);
	_viewDistance = core::Var::getSafe(cfg::VoxEditViewdistance);
	_simplifiedView = core::Var::getSafe(cfg::VoxEditSimplifiedView);
	_renderContext.init(video::getWindowSize());

	resetCamera();

	return true;
}

void Viewport::resetCamera(const glm::vec3 &pos, float distance, const voxel::Region &region) {
	_camera.setRotationType(video::CameraRotationType::Target);
	_camera.setAngles(0.0f, 0.0f, 0.0f);
	_camera.setFarPlane(_viewDistance->floatVal());
	_camera.setTarget(pos);
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

void Viewport::resize(const glm::ivec2 &frameBufferSize) {
	const ui::IMGUIApp *app = imguiApp();
	const glm::vec2 windowSize(app->windowDimension());
	const glm::vec2 windowFrameBufferSize(app->frameBufferDimension());
	const glm::vec2 scale = windowFrameBufferSize / windowSize;
	const glm::ivec2 cameraSize((float)frameBufferSize.x * scale.x, (float)frameBufferSize.y * scale.y);
	_camera.setSize(cameraSize);
	_renderContext.resize(frameBufferSize);
}

bool Viewport::isFixedCamera() const {
	return _camMode != SceneCameraMode::Free;
}

void Viewport::move(bool pan, bool rotate, int x, int y) {
	if (rotate) {
		const float yaw = (float)(x - _mouseX);
		const float pitch = (float)(y - _mouseY);
		const float s = _rotationSpeed->floatVal();
		if (!isFixedCamera()) {
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
	sceneMgr().trace(_renderContext.sceneMode);
}

void Viewport::dragAndDrop(float headerSize) {
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
}

void Viewport::renderBottomBar() {
	const float height = ImGui::GetFrameHeight();
	const ImVec2 windowSize = ImGui::GetWindowSize();

	if (!isFixedCamera()) {
		static const char *camRotTypes[] = {"Reference Point", "Eye"};
		static_assert(lengthof(camRotTypes) == (int)video::CameraRotationType::Max, "Array size doesn't match enum values");
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
	}

	static const char *polygonModes[] = {"Points", "Lines", "Solid"};
	static_assert(lengthof(polygonModes) == (int)video::PolygonMode::Max, "Array size doesn't match enum values");
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

void Viewport::renderViewportImage(const glm::ivec2 &contentSize) {
	// use the uv coords here to take a potential fb flip into account
	const glm::vec4 &uv = _renderContext.frameBuffer.uv();
	const glm::vec2 uva(uv.x, uv.y);
	const glm::vec2 uvc(uv.z, uv.w);
	const video::TexturePtr &texture = _renderContext.frameBuffer.texture(video::FrameBufferAttachment::Color0);
	ImGui::Image(texture->handle(), contentSize, uva, uvc);
}

void Viewport::renderViewport() {
	core_trace_scoped(Viewport);
	glm::ivec2 contentSize = ImGui::GetContentRegionAvail();
	const float headerSize = ImGui::GetCursorPosY();
	if (setupFrameBuffer(contentSize)) {
		_camera.update(imguiApp()->deltaFrameSeconds());

		renderToFrameBuffer();
		renderViewportImage(contentSize);
		renderGizmo(camera(), headerSize, contentSize);

		if (sceneMgr().isLoading()) {
			const float radius = ImGui::GetFontSize() * 12.0f;
			ImGui::LoadingIndicatorCircle("Loading", radius, core::Color::White, core::Color::Gray);
		} else if (ImGui::IsItemHovered()) {
			if (sceneMgr().modifier().isMode(ModifierType::ColorPicker)) {
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			}
			_hovered = true;
			updateViewportTrace(headerSize);
		}

		dragAndDrop(headerSize);
		renderBottomBar();
	}
}

void Viewport::menuBarCameraProjection() {
	static const char *modes[] = {"Perspective", "Orthogonal"};
	static_assert(lengthof(modes) == (int)video::CameraMode::Max, "Array size doesn't match enum values");
	const int currentMode = (int)camera().mode();
	const float modeMaxWidth = ImGui::CalcComboBoxWidth(modes[currentMode]);
	ImGui::SetNextItemWidth(modeMaxWidth);
	if (ImGui::BeginCombo("##cameraproj", modes[currentMode])) {
		for (int n = 0; n < lengthof(modes); n++) {
			const bool isSelected = (currentMode == n);
			if (ImGui::Selectable(modes[n], isSelected)) {
				camera().setMode((video::CameraMode)n);
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
}

void Viewport::menuBarCameraMode() {
	const int currentMode = (int)_camMode;
	const float modeMaxWidth = ImGui::CalcComboBoxWidth(SceneCameraModeStr[currentMode]);
	ImGui::SetNextItemWidth(modeMaxWidth);
	if (ImGui::BeginCombo("##cameramode", SceneCameraModeStr[currentMode])) {
		for (int n = 0; n < lengthof(SceneCameraModeStr); n++) {
			const bool isSelected = (currentMode == n);
			if (ImGui::Selectable(SceneCameraModeStr[n], isSelected)) {
				_camMode = (SceneCameraMode)n;
				resetCamera();
				if (_camMode != SceneCameraMode::Free) {
					camera().setMode(video::CameraMode::Orthogonal);
				}
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
}

bool Viewport::isSceneMode() const {
	return _renderContext.sceneMode;
}

void Viewport::toggleScene() {
	if (_simplifiedView->boolVal()) {
		return;
	}
	if (_renderContext.sceneMode) {
		_renderContext.sceneMode = false;
	} else {
		_renderContext.sceneMode = true;
	}
}

void Viewport::renderMenuBar(command::CommandExecutionListener *listener) {
	if (ImGui::BeginMenuBar()) {
		const MementoHandler &mementoHandler = sceneMgr().mementoHandler();
		ImGui::CommandMenuItem(ICON_FA_ROTATE_LEFT " Undo", "undo", mementoHandler.canUndo(), listener);
		ImGui::CommandMenuItem(ICON_FA_ROTATE_RIGHT " Redo", "redo", mementoHandler.canRedo(), listener);
		ImGui::Dummy(ImVec2(20, 0));
		menuBarCameraProjection();
		menuBarCameraMode();
		if (!_simplifiedView->boolVal()) {
			ImGui::Checkbox("Scene Mode", &_renderContext.sceneMode);
		}
		ImGui::EndMenuBar();
	}
}

void Viewport::update(command::CommandExecutionListener *listener) {
	_camera.setFarPlane(_viewDistance->floatVal());

	_hovered = false;
	ui::ScopedStyle style;
	style.setWindowRounding(0.0f);
	style.setWindowBorderSize(0.0f);
	style.setWindowPadding(ImVec2(0.0f, 0.0f));
	const int sceneWindowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoFocusOnAppearing;
	const char *modeStr = isSceneMode() ? "SceneMode" : "EditMode";

	core::String name;
	if (_detailedTitle) {
		name = core::string::format("%s %s###viewport%i", SceneCameraModeStr[(int)_camMode], modeStr, _id);
	} else {
		name = core::string::format("%s###viewport%i", modeStr, _id);
	}
	if (ImGui::Begin(name.c_str(), nullptr, sceneWindowFlags)) {
		renderMenuBar(listener);
		renderViewport();
	}
	ImGui::End();
}

void Viewport::shutdown() {
	_renderContext.shutdown();
}

bool Viewport::saveImage(const char *filename) {
	_renderContext.frameBuffer.bind(true);
	sceneMgr().render(_renderContext, camera(), SceneManager::RenderScene);
	_renderContext.frameBuffer.unbind();

	const image::ImagePtr &image = _renderContext.frameBuffer.image(filename, video::FrameBufferAttachment::Color0);
	if (!image) {
		Log::error("Failed to read texture");
		return false;
	}
	return image->writePng();
}

void Viewport::resetCamera() {
	const voxelformat::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
	const voxel::Region &sceneRegion = sceneGraph.region();
	const glm::vec3 &pos = sceneRegion.getCenter();
	const int activeNode = sceneGraph.activeNode();
	const voxel::RawVolume *v = activeNode != -1 ? sceneMgr().volume(activeNode) : nullptr;
	voxel::Region region;
	if (v != nullptr) {
		region = v->region();
	}
	const float distance = 100.0f; // TODO: let this depend on the size of the region
	resetCamera(pos, distance, region);
}

bool Viewport::setupFrameBuffer(const glm::ivec2 &frameBufferSize) {
	if (frameBufferSize.x <= 0 || frameBufferSize.y <= 0) {
		return false;
	}
	if ( _renderContext.frameBuffer.dimension() == frameBufferSize) {
		return true;
	}
	resize(frameBufferSize);
	return true;
}

void Viewport::reset() {
	_guizmoActivated = false;
	if (_transformMementoLocked) {
		Log::debug("Unlock memento state in reset()");
		sceneMgr().mementoHandler().unlock();
		_transformMementoLocked = false;
	}
}

void Viewport::unlock(voxelformat::SceneGraphNode &node, voxelformat::KeyFrameIndex keyFrameIdx) {
	if (!_transformMementoLocked) {
		return;
	}
	Log::debug("Unlock memento state");
	sceneMgr().mementoHandler().unlock();
	sceneMgr().mementoHandler().markNodeTransform(node, keyFrameIdx);
	_transformMementoLocked = false;
}

void Viewport::lock(voxelformat::SceneGraphNode &node, voxelformat::KeyFrameIndex keyFrameIdx) {
	if (_transformMementoLocked) {
		return;
	}
	Log::debug("Lock memento state");
	sceneMgr().mementoHandler().markNodeTransform(node, keyFrameIdx);
	sceneMgr().mementoHandler().lock();
	_transformMementoLocked = true;
}

void Viewport::renderSceneGuizmo(video::Camera &camera) {
	if (!_renderContext.sceneMode) {
		reset();
		return;
	}
	const voxelformat::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
	const int activeNode = sceneGraph.activeNode();
	if (activeNode == -1) {
		reset();
		return;
	}
	voxelformat::SceneGraphNode &node = sceneGraph.node(activeNode);

	int operation = ImGuizmo::TRANSLATE | ImGuizmo::BOUNDS | ImGuizmo::SCALE;
	if (_guizmoRotation->boolVal()) {
		operation |= ImGuizmo::ROTATE;
	}
	const float step = core::Var::getSafe(cfg::VoxEditGridsize)->floatVal();
	const float snap[]{step, step, step};
	const voxelformat::KeyFrameIndex keyFrameIdx = node.keyFrameForFrame(sceneMgr().currentFrame());
	const voxelformat::SceneGraphTransform &transform = node.transform(keyFrameIdx);
	glm::mat4 localMatrix = transform.localMatrix();
	glm::mat4 deltaMatrix(0.0f);
	const float boundsSnap[] = {1.0f, 1.0f, 1.0f};

	const voxel::Region &region = node.region();
	const glm::vec3 size = region.getDimensionsInVoxels();
	if (_boundsNode.maxs != size) {
		_bounds.mins = -transform.pivot() * size;
		_bounds.maxs = _bounds.mins + size;
		_boundsNode.maxs = size;
	}

	const bool manipulated = ImGuizmo::Manipulate(
		glm::value_ptr(camera.viewMatrix()), glm::value_ptr(camera.projectionMatrix()), (ImGuizmo::OPERATION)operation,
		ImGuizmo::MODE::LOCAL, glm::value_ptr(localMatrix), glm::value_ptr(deltaMatrix), _guizmoSnap->boolVal() ? snap : nullptr,
		_guizmoBounds->boolVal() ? glm::value_ptr(_bounds.mins) : nullptr, boundsSnap);

	const bool guizmoActive = ImGuizmo::IsUsing();

	if (guizmoActive) {
		lock(node, keyFrameIdx);
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
		unlock(node, keyFrameIdx);
		const voxel::Region &region = node.region();
		const voxel::Region newRegion(region.getLowerCorner(),
									  region.getLowerCorner() + glm::ivec3(glm::ceil(_bounds.maxs)) - 1);
		if (newRegion.isValid() && region != newRegion) {
			sceneMgr().resize(node.id(), newRegion);
		}
		_guizmoActivated = false;
	} else {
		unlock(node, keyFrameIdx);
	}
	if (manipulated) {
		sceneMgr().nodeUpdateTransform(activeNode, localMatrix, &deltaMatrix, keyFrameIdx);
	}
}

void Viewport::renderCameraManipulator(video::Camera &camera, float headerSize) {
	if (isFixedCamera()) {
		return;
	}
	ImVec2 position = ImGui::GetWindowPos();
	const ImVec2 size = ImVec2(128, 128);
	const ImVec2 maxSize = ImGui::GetWindowContentRegionMax();
	position.x += maxSize.x - size.x;
	position.y += headerSize;
	const ImU32 backgroundColor = 0;
	const float length = camera.targetDistance();

	glm::mat4 viewMatrix = camera.viewMatrix();
	float *viewPtr = glm::value_ptr(viewMatrix);

	if (_renderContext.sceneMode) {
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

void Viewport::renderGizmo(video::Camera &camera, float headerSize, const ImVec2 &size) {
	if (!_showAxisVar->boolVal()) {
		return;
	}

	ImGuiIO &io = ImGui::GetIO();
	if (io.MousePos.x == -FLT_MAX || io.MousePos.y == -FLT_MAX) {
		return;
	}

	const bool orthographic = camera.mode() == video::CameraMode::Orthogonal;

	ImGuizmo::BeginFrame();
	const ImVec2 &windowPos = ImGui::GetWindowPos();
	ImGuizmo::Enable(_renderContext.sceneMode);
	ImGuizmo::AllowAxisFlip(_guizmoAllowAxisFlip->boolVal());
	ImGuizmo::SetDrawlist();
	ImGuizmo::SetRect(windowPos.x, windowPos.y + headerSize, size.x, size.y);
	ImGuizmo::SetOrthographic(orthographic);
	renderSceneGuizmo(camera);
	renderCameraManipulator(camera, headerSize);
}

void Viewport::renderToFrameBuffer() {
	core_trace_scoped(EditorSceneRenderFramebuffer);
	video::clearColor(core::Color::Clear);
	_renderContext.frameBuffer.bind(true);
	sceneMgr().render(_renderContext, camera());
	_renderContext.frameBuffer.unbind();
}

} // namespace voxedit
