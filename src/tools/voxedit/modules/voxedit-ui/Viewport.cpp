/**
 * @file
 */

#include "Viewport.h"
#include "DragAndDropPayload.h"
#include "Gizmo.h"
#include "ViewMode.h"
#include "app/App.h"
#include "core/ArrayLength.h"
#include "core/Color.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/Var.h"
#include "image/Image.h"
#include "imgui.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "math/Axis.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "scenegraph/SceneGraphKeyFrame.h"
#include "scenegraph/SceneGraphNode.h"
#include "ui/IMGUIApp.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "ui/ScopedStyle.h"
#include "ui/dearimgui/ImGuizmo.h"
#include "video/Camera.h"
#include "video/Renderer.h"
#include "video/WindowedApp.h"
#include "voxedit-ui/MenuBar.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelrender/SceneGraphRenderer.h"

#include <glm/ext/quaternion_common.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/vector_relational.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/transform.hpp>

namespace voxedit {

static bool s_hideAxis[3]{false, false, false};

core::String Viewport::viewportId(int id, bool printable) {
	if (printable)
		return core::String::format("Viewport %i###viewport%i", id, id);
	return core::String::format("###viewport%i", id);
}

Viewport::Viewport(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr, int id, voxelrender::RenderMode renderMode,
				   bool detailedTitle)
	: Super(app, viewportId(id, true).c_str()), _id(id), _uiId(viewportId(id)), _detailedTitle(detailedTitle),
	  _sceneMgr(sceneMgr) {
	setRenderMode(renderMode);
}

Viewport::~Viewport() {
	shutdown();
}

bool Viewport::init() {
	_rotationSpeed = core::Var::getSafe(cfg::ClientMouseRotationSpeed);
	_cursorDetails = core::Var::getSafe(cfg::VoxEditCursorDetails);
	_showAxisVar = core::Var::getSafe(cfg::VoxEditShowaxis);
	_gizmoOperations = core::Var::getSafe(cfg::VoxEditGizmoOperations);
	_gizmoAllowAxisFlip = core::Var::getSafe(cfg::VoxEditGizmoAllowAxisFlip);
	_gizmoSnap = core::Var::getSafe(cfg::VoxEditGizmoSnap);
	_modelGizmo = core::Var::getSafe(cfg::VoxEditModelGizmo);
	_viewDistance = core::Var::getSafe(cfg::VoxEditViewdistance);
	_viewMode = core::Var::getSafe(cfg::VoxEditViewMode);
	_pivotMode = core::Var::getSafe(cfg::VoxEditGizmoPivot);
	_hideInactive = core::Var::getSafe(cfg::VoxEditHideInactive);
	_gridSize = core::Var::getSafe(cfg::VoxEditGridsize);
	_autoKeyFrame = core::Var::getSafe(cfg::VoxEditAutoKeyFrame);
	_localSpace = core::Var::getSafe(cfg::VoxEditLocalSpace);
	_renderNormals = core::Var::getSafe(cfg::RenderNormals);
	_animationPlaying = core::Var::getSafe(cfg::VoxEditAnimationPlaying);
	if (!_renderContext.init(video::getWindowSize())) {
		return false;
	}

	_camera.setRotationType(video::CameraRotationType::Target);
	resetCamera();

	return true;
}

// delay the resize a few frames to avoid performance issues while
// the user is resizing the window
void Viewport::delayResize(const glm::ivec2 &frameBufferSize) {
	if (_resizeRequestSize == frameBufferSize) {
		return;
	}
	_resizeRequestSize = frameBufferSize;
	_resizeRequestSeconds = _nowSeconds + 0.2;

	resizeCamera(frameBufferSize);
}

void Viewport::resizeCamera(const glm::ivec2 &frameBufferSize) {
	const glm::vec2 &windowSize = _app->windowDimension();
	const glm::vec2 &windowFrameBufferSize = _app->frameBufferDimension();
	const glm::vec2 scale = windowFrameBufferSize / windowSize;
	const glm::ivec2 cameraSize((float)frameBufferSize.x * scale.x, (float)frameBufferSize.y * scale.y);
	_camera.setSize(cameraSize);
}

void Viewport::resize(const glm::ivec2 &frameBufferSize) {
	resizeCamera(frameBufferSize);
	_renderContext.resize(frameBufferSize);
}

bool Viewport::isFixedCamera() const {
	return _camMode != voxelrender::SceneCameraMode::Free;
}

void Viewport::move(bool pan, bool rotate, int x, int y) {
	if (rotate) {
		if (!isFixedCamera()) {
			const float yaw = (float)(x - _mouseX);
			const float pitch = (float)(y - _mouseY);
			const float s = _rotationSpeed->floatVal();
			// TODO: this lets the camera jump because it will update the eye position, too
			_camera.setTarget(_sceneMgr->referencePosition());
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
	const bool rotate = _sceneMgr->cameraRotate();
	const bool pan = _sceneMgr->cameraPan();
	move(pan, rotate, mouseX, mouseY);
	_sceneMgr->setMousePos(_mouseX, _mouseY);
	_sceneMgr->setActiveCamera(&camera());
	const glm::mat4 &invModel = glm::inverse(_sceneMgr->modelMatrix(_renderContext));
	_sceneMgr->trace(_renderContext.isSceneMode(), false, invModel);
}

void Viewport::dragAndDrop(float headerSize) {
	if (ImGui::BeginDragDropTarget()) {
		if (!isSceneMode()) {
			if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(voxelui::dragdrop::ImagePayload)) {
				const image::ImagePtr &image = *(const image::ImagePtr *)payload->Data;
				updateViewportTrace(headerSize);
				_sceneMgr->fillPlane(image);
			}
		}
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(voxelui::dragdrop::PaletteIndexPayload)) {
			const int dragPalIdx = (int)(intptr_t)payload->Data;
			const int nodeId = _sceneMgr->sceneGraph().activeNode();
			if (scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNode(nodeId)) {
				if (node->visible() && node->isModelNode()) {
					updateViewportTrace(headerSize);
					ModifierFacade &modifier = _sceneMgr->modifier();
					modifier.setCursorVoxel(voxel::createVoxel(node->palette(), dragPalIdx));
					modifier.start();
					auto callback = [nodeId, this](const voxel::Region &region, ModifierType type,
												   SceneModifiedFlags flags) {
						if (type != ModifierType::Select && type != ModifierType::ColorPicker) {
							_sceneMgr->modified(nodeId, region, flags);
						}
					};
					modifier.execute(_sceneMgr->sceneGraph(), *node, callback);
					modifier.stop();
				}
			}
		}
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(voxelui::dragdrop::ModelPayload)) {
			const core::String &filename = *(core::String *)payload->Data;
			_sceneMgr->import(filename);
		}

		ImGui::EndDragDropTarget();
	}
}

void Viewport::renderViewportImage(const glm::ivec2 &contentSize) {
	core_trace_scoped(ViewportImage);
	// use the uv coords here to take a potential fb flip into account
	const glm::vec4 &uv = _renderContext.frameBuffer.uv();
	const glm::vec2 uva(uv.x, uv.y);
	const glm::vec2 uvc(uv.z, uv.w);
	const video::TexturePtr &texture = _renderContext.frameBuffer.texture(video::FrameBufferAttachment::Color0);
	ImGui::Image(texture->handle(), contentSize, uva, uvc);
}

void Viewport::renderCursorDetails() const {
	if (_viewportUIElementHovered) {
		return;
	}
	const ModifierFacade &modifier = _sceneMgr->modifier();
	const int cursorDetailsLevel = _cursorDetails->intVal();
	if (cursorDetailsLevel == 0) {
		return;
	}

	const bool sliceActive = _sceneMgr->isSliceModeActive();
	const glm::ivec3 &cursorPos = modifier.cursorPosition();
	if (sliceActive) {
		ImGui::TooltipText(_("Slice at %s: %i"), math::getCharForAxis(_sliceAxis),
						   cursorPos[math::getIndexForAxis(_sliceAxis)]);
	}
	if (cursorDetailsLevel == 1) {
		ImGui::TooltipText("%i:%i:%i", cursorPos.x, cursorPos.y, cursorPos.z);
		return;
	}

	const int activeNode = _sceneMgr->sceneGraph().activeNode();
	if (const voxel::RawVolume *v = _sceneMgr->volume(activeNode)) {
		const glm::ivec3 &mins = v->region().getLowerCorner();
		const glm::ivec3 &size = v->region().getDimensionsInVoxels();
		if (mins.x == 0 && mins.y == 0 && mins.z == 0) {
			ImGui::TooltipText(_("pos: %i:%i:%i\nsize: %i:%i:%i\nabsolute: %i:%i:%i"), mins.x, mins.y, mins.z, size.x,
							   size.y, size.z, cursorPos.x, cursorPos.y, cursorPos.z);
		} else {
			ImGui::TooltipText(_("pos: %i:%i:%i\nsize: %i:%i:%i\nabsolute: %i:%i:%i\nrelative: %i:%i:%i"), mins.x,
							   mins.y, mins.z, size.x, size.y, size.z, cursorPos.x, cursorPos.y, cursorPos.z,
							   cursorPos.x - mins.x, cursorPos.y - mins.y, cursorPos.z - mins.z);
		}
	}
}

void Viewport::renderCursor() {
	if (isSceneMode()) {
		return;
	}

	const ModifierFacade &modifier = _sceneMgr->modifier();
	if (modifier.isMode(ModifierType::ColorPicker) || modifier.isMode(ModifierType::Select) ||
		modifier.brushType() == BrushType::Paint) {
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
	} else if (modifier.brushType() == BrushType::Plane) {
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
	}

	if (const Brush *brush = modifier.currentBrush()) {
		if (!brush->errorReason().empty()) {
			ImGui::TooltipTextUnformatted(brush->errorReason().c_str());
			return;
		}
	}

	renderCursorDetails();
}

bool Viewport::renderSlicer(const glm::ivec2 &contentSize) {
	const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
	const int activeNode = sceneGraph.activeNode();
	const int axis = math::getIndexForAxis(_sliceAxis);
	bool changed = false;
	if (const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(activeNode)) {
		bool sliceActive = _sceneMgr->isSliceModeActive();
		const ImVec2 cursorStart = ImGui::GetCursorScreenPos();
		if (ImGui::Checkbox("##sliceactive", &sliceActive)) {
			if (!sliceActive) {
				_sceneMgr->setSliceRegion(voxel::Region::InvalidRegion);
			} else {
				const voxel::Region &nodeRegion = sceneGraph.resolveRegion(*node);
				glm::ivec3 nodeMaxs = nodeRegion.getUpperCorner();
				glm::ivec3 nodeMins = nodeRegion.getLowerCorner();
				nodeMaxs[axis] = nodeMins[axis];
				_sceneMgr->setSliceRegion({nodeMins, nodeMaxs});
			}
			changed = true;
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetItemTooltipUnformatted(_("Slice view"));
			_viewportUIElementHovered = true;
		}
		if (sliceActive) {
			const ImVec2 cursorEnd = ImGui::GetCursorScreenPos();
			const float usedHeight = cursorEnd.y - cursorStart.y;
			const voxel::Region &sliceRegion = _sceneMgr->sliceRegion();
			glm::ivec3 mins = sliceRegion.getLowerCorner();
			const voxel::Region &nodeRegion = sceneGraph.resolveRegion(*node);
			if (ImGui::VSliderInt("##slicepos", {ImGui::Size(3.0f), (float)contentSize.y - usedHeight}, &mins[axis],
								  nodeRegion.getLowerY(), nodeRegion.getUpperY())) {
				changed = true;
			}
			if (ImGui::IsItemHovered()) {
				_viewportUIElementHovered = true;
			}
			glm::ivec3 nodeMaxs = nodeRegion.getUpperCorner();
			glm::ivec3 nodeMins = nodeRegion.getLowerCorner();
			nodeMaxs[axis] = mins[axis];
			nodeMins[axis] = mins[axis];
			_sceneMgr->setSliceRegion({nodeMins, nodeMaxs});
		}
	}
	return changed;
}

void Viewport::renderViewport() {
	core_trace_scoped(Viewport);
	glm::ivec2 contentSize = ImGui::GetContentRegionAvail();
	ImVec2 cursorPos = ImGui::GetCursorPos();
	const float headerSize = cursorPos.y;
	if (setupFrameBuffer(contentSize)) {
		if (_animationPlaying->boolVal() && _sceneMgr->activeCameraNode()) {
			_camera = voxelrender::toCamera(_camera.size(), _sceneMgr->sceneGraph(), *_sceneMgr->activeCameraNode(), _sceneMgr->currentFrame());
		}
		_camera.update(_app->deltaFrameSeconds());

		renderToFrameBuffer();
		renderSlicer(contentSize);
		ImGui::SetCursorPos(cursorPos);
		renderViewportImage(contentSize);
		const bool modifiedRegion = renderGizmo(camera(), headerSize, contentSize);

		if (_sceneMgr->isLoading()) {
			const float radius = ImGui::GetFontSize() * 12.0f;
			ImGui::LoadingIndicatorCircle(_("Loading"), radius, core::Color::White(), core::Color::Gray());
		} else if (ImGui::IsItemHovered() && !modifiedRegion) {
			renderCursor();
			updateViewportTrace(headerSize);
			_hovered = true;
		}

		dragAndDrop(headerSize);
	}
}

void Viewport::menuBarCameraProjection() {
	const char *modes[] = {_("Perspective"), _("Orthogonal")};
	static_assert(lengthof(modes) == (int)video::CameraMode::Max, "Array size doesn't match enum values");
	const int currentMode = (int)camera().mode();
	const float modeMaxWidth = ImGui::CalcComboWidth(modes[currentMode]);
	ImGui::SetNextItemWidth(modeMaxWidth);
	if (ImGui::BeginCombo("###cameraproj", modes[currentMode])) {
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
	const float modeMaxWidth = ImGui::CalcComboWidth(_(voxelrender::SceneCameraModeStr[currentMode]));
	ImGui::SetNextItemWidth(modeMaxWidth);
	if (ImGui::BeginCombo("###cameramode", _(voxelrender::SceneCameraModeStr[currentMode]))) {
		for (int n = 0; n < lengthof(voxelrender::SceneCameraModeStr); n++) {
			const bool isSelected = (currentMode == n);
			if (ImGui::Selectable(_(voxelrender::SceneCameraModeStr[n]), isSelected)) {
				_camMode = (voxelrender::SceneCameraMode)n;
				resetCamera();
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
}

bool Viewport::isSceneMode() const {
	return _renderContext.isSceneMode();
}

void Viewport::toggleScene() {
	if (!viewModeAllViewports(_viewMode->intVal())) {
		return;
	}
	if (isSceneMode()) {
		setRenderMode(voxelrender::RenderMode::Edit);
	} else {
		setRenderMode(voxelrender::RenderMode::Scene);
	}
}

void Viewport::setRenderMode(voxelrender::RenderMode renderMode) {
	_renderContext.renderMode = renderMode;
}

void Viewport::toggleVideoRecording() {
	if (_captureTool.isRecording()) {
		Log::debug("Stop recording");
		_captureTool.stopRecording();
		return;
	}
	auto callback = [this](const core::String &file, const io::FormatDescription *desc) {
		const glm::ivec2 &dim = _renderContext.frameBuffer.dimension();
		_captureTool.startRecording(file.c_str(), dim.x, dim.y);
	};
	const char *filename = _captureTool.type() == image::CaptureType::AVI ? "video.avi" : "video.mpeg2";
	video::WindowedApp::getInstance()->saveDialog(callback, {}, nullptr, filename);
}

void Viewport::menuBarPolygonModeOptions() {
	const char *polygonModes[] = {_("Points"), _("Lines"), _("Solid")};
	static_assert(lengthof(polygonModes) == (int)video::PolygonMode::Max, "Array size doesn't match enum values");
	const int currentPolygonMode = (int)camera().polygonMode();
	if (ImGui::BeginCombo(_("Render mode"), polygonModes[currentPolygonMode])) {
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

void Viewport::menuBarCaptureOptions() {
	const char *icon = ICON_LC_CLAPPERBOARD;
	const char *text = _("Video");
	if (_captureTool.isRecording()) {
		icon = ICON_LC_CIRCLE_STOP;
		text = _("Stop recording");
	}
	if (ImGui::IconMenuItem(icon, text)) {
		toggleVideoRecording();
	}
	const uint32_t pendingFrames = _captureTool.pendingFrames();
	if (pendingFrames > 0u) {
		ImGui::SameLine();
		ImGui::Text(_("Pending frames: %u"), pendingFrames);
	} else {
		ImGui::TooltipText(_("You can control the fps of the video with the cvar %s\nPending frames: %u"),
						   cfg::CoreMaxFPS, pendingFrames);
	}
}

void Viewport::menuBarFreeCameraOptions() {
	glm::vec3 omega = camera().omega();
	if (ImGui::InputFloat(_("Camera rotation"), &omega.y)) {
		camera().setOmega(omega);
	}

	const char *camRotTypes[] = {_("Reference Point"), _("Eye")};
	static_assert(lengthof(camRotTypes) == (int)video::CameraRotationType::Max, "Array size doesn't match enum values");
	const int currentCamRotType = (int)camera().rotationType();
	if (ImGui::BeginCombo(_("Camera movement"), camRotTypes[currentCamRotType])) {
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
	ImGui::BeginDisabled(camera().rotationType() != video::CameraRotationType::Eye);
	bool clipping = _sceneMgr->clipping();
	if (ImGui::Checkbox(_("Clipping"), &clipping)) {
		_sceneMgr->setClipping(clipping);
	}

	ImGui::BeginDisabled(!clipping);
	bool gravity = _sceneMgr->gravity();
	if (ImGui::Checkbox(_("Gravity"), &gravity)) {
		_sceneMgr->setGravity(gravity);
	}
	ImGui::EndDisabled();

	ImGui::EndDisabled();
}

void Viewport::menuBarCameraOptions(command::CommandExecutionListener *listener) {
	ImGui::CommandIconMenuItem(ICON_LC_VIDEO, _("Reset camera"), "resetcamera", true, listener);
	if (!isFixedCamera()) {
		menuBarFreeCameraOptions();
	}
}

void Viewport::menuBarScreenshotOptions(command::CommandExecutionListener *listener) {
	const core::String command = core::String::format("screenshot %i", _id);
	ImGui::CommandIconMenuItem(ICON_LC_CAMERA, _("Screenshot"), command.c_str(), listener);
}

void Viewport::menuBarView(command::CommandExecutionListener *listener) {
	if (ImGui::BeginIconMenu(ICON_LC_EYE, _("View"))) {
		menuBarScreenshotOptions(listener);
		menuBarCaptureOptions();
		menuBarCameraOptions(listener);
		menuBarPolygonModeOptions();
		MenuBar::viewportOptions();
		ImGui::EndMenu();
	}
}

void Viewport::menuBarRenderModeToggle() {
	if (!viewModeAllViewports(_viewMode->intVal())) {
		return;
	}
	bool sceneMode = isSceneMode();
	if (ImGui::Checkbox(_("Scene Mode"), &sceneMode)) {
		if (sceneMode) {
			_renderContext.renderMode = voxelrender::RenderMode::Scene;
		} else {
			_renderContext.renderMode = voxelrender::RenderMode::Edit;
		}
	}
	if (!sceneMode) {
		ImGui::SameLine();
		ImGui::Checkbox(_("Apply transforms"), &_renderContext.applyTransformsInEditMode);
	}
}

void Viewport::menuBarMementoOptions(command::CommandExecutionListener *listener) {
	const memento::MementoHandler &mementoHandler = _sceneMgr->mementoHandler();
	ImGui::CommandIconMenuItem(ICON_LC_UNDO, _("Undo"), "undo", mementoHandler.canUndo(), listener);
	ImGui::CommandIconMenuItem(ICON_LC_REDO, _("Redo"), "redo", mementoHandler.canRedo(), listener);
	const int activeNode = _sceneMgr->sceneGraph().activeNode();
	if (!mementoHandler.recordVolumeStates(_sceneMgr->volume(activeNode))) {
		ImGui::TextUnformatted(ICON_LC_TRIANGLE_ALERT);
		ImGui::TooltipTextUnformatted(_("Undo disabled for this volume. Max volume size exceeded"));
	}
}

void Viewport::renderMenuBar(command::CommandExecutionListener *listener) {
	core_trace_scoped(Menubar);
	if (ImGui::BeginMenuBar()) {
		menuBarMementoOptions(listener);
		ImGui::Dummy(ImVec2(20, 0));
		menuBarCameraProjection();
		menuBarCameraMode();
		menuBarRenderModeToggle();
		menuBarView(listener);

		ImGui::EndMenuBar();
	}
}

void Viewport::update(double nowSeconds, command::CommandExecutionListener *listener) {
	core_trace_scoped(ViewportPanel);
	_camera.setFarPlane(_viewDistance->floatVal());

	_viewportUIElementHovered = false;
	_hovered = false;
	_visible = false;
	_cameraManipulated = false;
	_nowSeconds = nowSeconds;

	ui::ScopedStyle style;
	style.setWindowRounding(0.0f);
	style.setWindowBorderSize(0.0f);
	style.setWindowPadding(ImVec2(0.0f, 0.0f));
	const int sceneWindowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
								 ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoFocusOnAppearing;
	const char *modeStr = isSceneMode() ? _("SceneMode") : _("EditMode");

	_renderContext.renderNormals = _renderNormals->boolVal();

	core::String name;
	if (_detailedTitle) {
		name =
			core::String::format("%s %s%s", _(voxelrender::SceneCameraModeStr[(int)_camMode]), modeStr, _uiId.c_str());
	} else {
		name = core::String::format("%s%s", modeStr, _uiId.c_str());
	}
	if (ImGui::Begin(name.c_str(), nullptr, sceneWindowFlags)) {
		_visible = true;
		renderMenuBar(listener);
		renderViewport();
	}
	ImGui::End();

	if (_captureTool.isRecording()) {
		_captureTool.enqueueFrame(renderToImage("**video**"));
	} else if (_captureTool.hasFinished()) {
		_captureTool.flush();
	}
}

void Viewport::shutdown() {
	_renderContext.shutdown();
	_captureTool.abort();
}

image::ImagePtr Viewport::renderToImage(const char *imageName) {
	_sceneMgr->render(_renderContext, camera(), SceneManager::RenderScene);
	return _renderContext.frameBuffer.image(imageName, video::FrameBufferAttachment::Color0);
}

bool Viewport::saveImage(const char *filename) {
	const image::ImagePtr &image = renderToImage(filename);
	if (!image) {
		Log::error("Failed to read texture");
		return false;
	}
	const io::FilePtr &file = io::filesystem()->open(image->name(), io::FileMode::SysWrite);
	io::FileStream stream(file);
	if (!stream.valid()) {
		return false;
	}
	return image->writePNG(stream);
}

void Viewport::resetCamera() {
	const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
	voxel::Region region;

	const int activeNode = sceneGraph.activeNode();
	if (isSceneMode()) {
		if (_hideInactive->boolVal()) {
			if (scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNode(activeNode)) {
				scenegraph::KeyFrameIndex keyFrameIndex = node->keyFrameForFrame(_sceneMgr->currentFrame());
				region = sceneGraph.sceneRegion(*node, keyFrameIndex);
			} else {
				region = sceneGraph.sceneRegion(0, true);
			}
		} else {
			region = sceneGraph.sceneRegion(0, true);
		}
	} else if (const voxel::RawVolume *v = _sceneMgr->volume(activeNode)) {
		// active node has a volume - use that region
		region = v->region();
	} else {
		// center an the accumulated region of the scene - without transforms - we are not in scene mode, but model mode
		region = sceneGraph.region();
	}
	const video::CameraRotationType rotationType = _camera.rotationType();
	voxelrender::configureCamera(_camera, region, _camMode, _viewDistance->floatVal());
	_camera.setRotationType(rotationType);
}

bool Viewport::setupFrameBuffer(const glm::ivec2 &frameBufferSize) {
	if (frameBufferSize.x <= 0 || frameBufferSize.y <= 0) {
		return false;
	}
	if (_renderContext.frameBuffer.dimension() == frameBufferSize) {
		return true;
	}
	if (_resizeRequestSeconds > 0.0 && _resizeRequestSeconds < _nowSeconds) {
		resize(frameBufferSize);
		_resizeRequestSeconds = 0.0;
		return true;
	}
	delayResize(frameBufferSize);
	return true;
}

void Viewport::reset() {
	if (_transformMementoLocked) {
		Log::debug("Unlock memento state in reset()");
		_sceneMgr->mementoHandler().unlock();
		_sceneMgr->modifier().unlock();
		_transformMementoLocked = false;
	}
}

void Viewport::unlock(const scenegraph::SceneGraphNode &node, scenegraph::KeyFrameIndex keyFrameIdx) {
	if (!_transformMementoLocked) {
		return;
	}
	Log::debug("Unlock memento state");
	memento::MementoHandler &mementoHandler = _sceneMgr->mementoHandler();
	mementoHandler.unlock();
	_sceneMgr->modifier().unlock();
	if (keyFrameIdx == InvalidKeyFrame) {
		// there is no valid key frame idx given in edit mode
		mementoHandler.markModification(_sceneMgr->sceneGraph(), node, node.region());
	} else {
		if (!glm::equal(_transformLocalMatrix, node.transform(keyFrameIdx).localMatrix())) {
			// we have a valid key frame idx in scene mode
			mementoHandler.markNodeTransform(_sceneMgr->sceneGraph(), node);
			_transformLocalMatrix = glm::mat4(1.0f);
		}
	}
	_transformMementoLocked = false;
}

void Viewport::lock(const scenegraph::SceneGraphNode &node, scenegraph::KeyFrameIndex keyFrameIdx) {
	if (_transformMementoLocked) {
		return;
	}
	Log::debug("Lock memento state");
	memento::MementoHandler &mementoHandler = _sceneMgr->mementoHandler();
	mementoHandler.lock();
	_sceneMgr->modifier().lock();
	_transformMementoLocked = true;
	if (keyFrameIdx == InvalidKeyFrame) {
		_transformLocalMatrix = glm::mat4(1.0f);
	} else {
		_transformLocalMatrix = node.transform(keyFrameIdx).localMatrix();
	}
}

void Viewport::updateGizmoValues(const scenegraph::SceneGraphNode &node, scenegraph::KeyFrameIndex keyFrameIdx,
								 const glm::mat4 &matrix) {
	if (ImGuizmo::IsUsing()) {
		lock(node, keyFrameIdx);
		glm::vec3 scale;
		glm::vec3 translation;
		glm::quat orientation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(matrix, scale, orientation, translation, skew, perspective);
		if (glm::all(glm::greaterThan(scale, glm::vec3(0)))) {
			_bounds.maxs = _boundsNode.maxs * scale;
		}
	} else if (_transformMementoLocked) {
		unlock(node, keyFrameIdx);
		const voxel::Region &region = node.region();
		const voxel::Region newRegion(region.getLowerCorner(),
									  region.getLowerCorner() + glm::ivec3(glm::ceil(_bounds.maxs)) - 1);
		if (newRegion.isValid() && region != newRegion) {
			_sceneMgr->nodeResize(node.id(), newRegion);
			updateBounds(node);
		}
	}
}

bool Viewport::wantGizmo() const {
	if (isSceneMode()) {
		return true;
	}
	if (_modelGizmo->boolVal()) {
		return true;
	}
	return false;
}

bool Viewport::createReference(const scenegraph::SceneGraphNode &node) const {
	if (!isSceneMode()) {
		return false;
	}
	if (!node.isModelNode()) {
		return false;
	}
	if (!ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
		return false;
	}
	if (!ImGuizmo::IsOver()) {
		return false;
	}
	if (!ImGui::IsKeyPressed(ImGuiKey_MouseLeft)) {
		ImGui::TooltipTextUnformatted(_("Create a reference node"));
		return false;
	}
	return true;
}

uint32_t Viewport::gizmoOperation(const scenegraph::SceneGraphNode &node) const {
	if (isSceneMode() && !_pivotMode->boolVal()) {
		// create reference mode - only allow translation
		if (node.isModelNode() && ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
			return ImGuizmo::TRANSLATE;
		}

		const uint32_t mask = _gizmoOperations->intVal();
		uint32_t operation = 0;
		if (mask & GizmoOperation_Translate) {
			operation |= ImGuizmo::TRANSLATE;
		}
		if (mask & GizmoOperation_Bounds) {
			operation |= ImGuizmo::BOUNDS;
		}
		if (mask & GizmoOperation_Scale) {
			operation |= ImGuizmo::SCALE;
		}
		if (mask & GizmoOperation_Rotate) {
			operation |= ImGuizmo::ROTATE;
		}
		return operation;
	}
	return ImGuizmo::TRANSLATE;
}

glm::mat4 Viewport::gizmoMatrix(const scenegraph::SceneGraphNode &node, scenegraph::KeyFrameIndex &keyFrameIdx) const {
	const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
	if (!isSceneMode() && node.isAnyModelNode()) {
		const voxel::Region &region = sceneGraph.resolveRegion(node);
		return glm::translate(region.getLowerCornerf());
	}
	keyFrameIdx = node.keyFrameForFrame(_sceneMgr->currentFrame());
	const scenegraph::SceneGraphTransform &transform = node.transform(keyFrameIdx);
	return transform.worldMatrix();
}

uint32_t Viewport::gizmoMode() const {
	return _localSpace->boolVal() ? ImGuizmo::MODE::LOCAL : ImGuizmo::MODE::WORLD;
}

void Viewport::updateBounds(const scenegraph::SceneGraphNode &node) {
	const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
	const voxel::Region &region = sceneGraph.resolveRegion(node);
	_bounds.mins = region.getLowerCornerf();
	_bounds.maxs = region.getUpperCornerf() + 1.0f;
}

const float *Viewport::gizmoBounds(const scenegraph::SceneGraphNode &node) {
	const float *boundsPtr = nullptr;
	if (isSceneMode() && node.isModelNode() && (_gizmoOperations->uintVal() & GizmoOperation_Bounds) != 0) {
		if (!ImGuizmo::IsUsing()) {
			updateBounds(node);
		}
		boundsPtr = glm::value_ptr(_bounds.mins);
	}
	return boundsPtr;
}

bool Viewport::gizmoManipulate(const video::Camera &camera, const float *boundsPtr, glm::mat4 &matrix,
							   glm::mat4 &deltaMatrix, uint32_t operation) const {
	static const float boundsSnap[] = {1.0f, 1.0f, 1.0f};
	float *mPtr = glm::value_ptr(matrix);
	float *dMatPtr = glm::value_ptr(deltaMatrix);
	const ImGuizmo::OPERATION op = (ImGuizmo::OPERATION)operation;
	const ImGuizmo::MODE mode = (ImGuizmo::MODE)gizmoMode();
	const float *vMatPtr = glm::value_ptr(camera.viewMatrix());
	const float *pMatPtr = glm::value_ptr(camera.projectionMatrix());
	const float step = _gridSize->floatVal();
	const float snap[]{step, step, step};
	const float *snapPtr = _gizmoSnap->boolVal() ? snap : nullptr;
	return ImGuizmo::Manipulate(vMatPtr, pMatPtr, op, mode, mPtr, dMatPtr, snapPtr, boundsPtr, boundsSnap);
}

static glm::mat4 parentWorldMatrix(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
								   scenegraph::KeyFrameIndex keyFrameIdx) {
	const int parentId = node.parent();
	if (parentId == InvalidNodeId || keyFrameIdx == InvalidKeyFrame) {
		return glm::mat4(1.0f);
	}
	if (const scenegraph::SceneGraphKeyFrame *keyFrame = node.keyFrame(keyFrameIdx)) {
		const scenegraph::SceneGraphNode &parentNode = sceneGraph.node(parentId);
		const scenegraph::KeyFrameIndex parentKeyFrameIdx = parentNode.keyFrameForFrame(keyFrame->frameIdx);
		if (parentKeyFrameIdx == InvalidKeyFrame) {
			return glm::mat4(1.0f);
		}
		return parentNode.transform(parentKeyFrameIdx).worldMatrix();
	}
	return glm::mat4(1.0f);
}

bool Viewport::runGizmo(const video::Camera &camera) {
	const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
	int activeNode = sceneGraph.activeNode();
	if (activeNode == InvalidNodeId) {
		reset();
		return false;
	}
	const bool sceneMode = isSceneMode();
	scenegraph::SceneGraphNode &node = sceneGraph.node(activeNode);
	if (!sceneMode && !node.isModelNode()) {
		reset();
		return false;
	}

	if (!wantGizmo()) {
		return false;
	}

	scenegraph::KeyFrameIndex keyFrameIdx = InvalidKeyFrame;
	glm::mat4 worldMatrix = gizmoMatrix(node, keyFrameIdx);
	glm::mat4 deltaMatrix(1.0f);
	const float *boundsPtr = gizmoBounds(node);
	const uint32_t operation = gizmoOperation(node);
	const bool manipulated = gizmoManipulate(camera, boundsPtr, worldMatrix, deltaMatrix, operation);
	updateGizmoValues(node, keyFrameIdx, worldMatrix);
	// check to create a reference before we update the node transform
	// otherwise the new reference node will not get the correct transform
	if (createReference(node)) {
		// we have to record the creating of the new nodes here - thus we have to unlock the memento state
		memento::ScopedMementoHandlerUnlock scopedUnlock(_sceneMgr->mementoHandler());
		const int newNode = _sceneMgr->nodeReference(node.id());
		// we need to activate the node - otherwise we end up in
		// endlessly creating new reference nodes
		if (_sceneMgr->nodeActivate(newNode)) {
			activeNode = newNode;
		}
	}
	if (manipulated) {
		if (sceneMode) {
			if (_pivotMode->boolVal()) {
				const glm::vec3 size = node.region().getDimensionsInVoxels();
				const scenegraph::SceneGraphTransform &transform = node.transform(keyFrameIdx);
				const glm::vec3 deltaTranslation =
					-glm::conjugate(transform.worldOrientation()) * glm::vec3(deltaMatrix[3]);
				// TODO: doesn't yet work for rotated keyframes - unrotate the delta translation here?
				//       https://github.com/vengi-voxel/vengi/issues/611
				const glm::vec3 deltaPivot = deltaTranslation / size;
				const glm::vec3 pivot = glm::clamp(node.pivot() + deltaPivot, glm::vec3(-5.0f), glm::vec3(5.0f));
				_sceneMgr->nodeUpdatePivot(activeNode, pivot);
			} else {
				const bool autoKeyFrame = _autoKeyFrame->boolVal();
				// check if a new keyframe should get generated automatically
				const scenegraph::FrameIndex frameIdx = _sceneMgr->currentFrame();
				if (autoKeyFrame && node.keyFrame(keyFrameIdx).frameIdx != frameIdx) {
					if (_sceneMgr->nodeAddKeyFrame(node.id(), frameIdx)) {
						const scenegraph::KeyFrameIndex newKeyFrameIdx = node.keyFrameForFrame(frameIdx);
						core_assert(newKeyFrameIdx != keyFrameIdx);
						core_assert(newKeyFrameIdx != InvalidKeyFrame);
						keyFrameIdx = newKeyFrameIdx;
					}
				}
				const glm::mat4 &worldParentMatrix = parentWorldMatrix(sceneGraph, node, keyFrameIdx);
				const glm::mat4 &newLocalMatrix = glm::inverse(worldParentMatrix) * worldMatrix;
				_sceneMgr->nodeUpdateTransform(activeNode, newLocalMatrix, keyFrameIdx, true);
			}
		} else {
			const glm::ivec3 shift = glm::vec3(worldMatrix[3]) - node.region().getLowerCornerf();
			_sceneMgr->nodeShift(activeNode, shift);
			// only true in edit mode
			return true;
		}
	}
	return false;
}

void Viewport::renderCameraManipulator(video::Camera &camera, float headerSize) {
	if (isFixedCamera()) {
		return;
	}
	ImVec2 position = ImGui::GetWindowPos();
	const ImVec2 size = ImVec2(128, 128);
	const ImVec2 available = ImGui::GetContentRegionAvail();
	const float contentRegionWidth = available.x + ImGui::GetCursorPosX();
	position.x += contentRegionWidth - size.x;
	position.y += headerSize;
	const ImU32 backgroundColor = 0;
	const float length = camera.targetDistance();

	glm::mat4 viewMatrix = camera.viewMatrix();
	float *viewPtr = glm::value_ptr(viewMatrix);

	if (isSceneMode()) {
		ImGuizmo::ViewManipulate(viewPtr, length, position, size, backgroundColor);
	} else {
		const float *projPtr = glm::value_ptr(camera.projectionMatrix());
		const ImGuizmo::OPERATION operation = (ImGuizmo::OPERATION)0;
		glm::mat4 transformMatrix = glm::mat4(1.0f); // not used
		float *matrixPtr = glm::value_ptr(transformMatrix);
		const ImGuizmo::MODE mode = ImGuizmo::MODE::LOCAL;
		ImGuizmo::ViewManipulate(viewPtr, projPtr, operation, mode, matrixPtr, length, position, size, backgroundColor);
	}
	if (ImGuizmo::IsViewManipulateHovered()) {
		_cameraManipulated = true;
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

bool Viewport::renderGizmo(video::Camera &camera, float headerSize, const ImVec2 &size) {
	if (!_showAxisVar->boolVal()) {
		return false;
	}
	core_trace_scoped(RenderGizmo);

	const bool orthographic = camera.mode() == video::CameraMode::Orthogonal;

	ImGuizmo::PushID(_id);
	ImGuizmo::SetDrawlist();
	ImGuizmo::SetWindow();
	const ImVec2 &windowPos = ImGui::GetWindowPos();
	ImGuizmo::Enable(isSceneMode() || _modelGizmo->boolVal());
	ImGuizmo::AllowAxisFlip(_gizmoAllowAxisFlip->boolVal());
	ImGuizmo::SetAxisMask(s_hideAxis[0], s_hideAxis[1], s_hideAxis[2]);
	ImGuizmo::SetRect(windowPos.x, windowPos.y + headerSize, size.x, size.y);
	ImGuizmo::SetOrthographic(orthographic);
	const bool editModeModified = runGizmo(camera);
	renderCameraManipulator(camera, headerSize);
	ImGuizmo::PopID();
	return editModeModified;
}

void Viewport::renderToFrameBuffer() {
	core_trace_scoped(RenderFramebuffer);
	video::clearColor(core::Color::Clear());
	_sceneMgr->render(_renderContext, camera());
}

} // namespace voxedit
