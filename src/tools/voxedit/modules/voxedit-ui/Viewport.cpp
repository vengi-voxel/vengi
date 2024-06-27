/**
 * @file
 */

#include "Viewport.h"
#include "Gizmo.h"
#include "DragAndDropPayload.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "ui/IconsLucide.h"
#include "app/App.h"
#include "core/ArrayLength.h"
#include "core/Color.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/Var.h"
#include "image/Image.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "scenegraph/SceneGraphNode.h"
#include "ui/IMGUIApp.h"
#include "ui/IMGUIEx.h"
#include "ui/ScopedStyle.h"
#include "ui/dearimgui/ImGuizmo.h"
#include "video/Camera.h"
#include "video/Renderer.h"
#include "video/WindowedApp.h"
#include "voxedit-ui/MenuBar.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelrender/SceneGraphRenderer.h"

#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/vector_relational.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/transform.hpp>

namespace voxedit {

core::String Viewport::viewportId(int id, bool printable) {
	if (printable)
		return core::string::format("Viewport %i###viewport%i", id, id);
	return core::string::format("###viewport%i", id);
}

Viewport::Viewport(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr, int id, bool sceneMode, bool detailedTitle)
	: Super(app, viewportId(id, true).c_str()), _id(id), _uiId(viewportId(id)), _detailedTitle(detailedTitle), _sceneMgr(sceneMgr) {
	_renderContext.sceneMode = sceneMode;
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
	_simplifiedView = core::Var::getSafe(cfg::VoxEditSimplifiedView);
	_pivotMode = core::Var::getSafe(cfg::VoxEditGizmoPivot);
	_hideInactive = core::Var::getSafe(cfg::VoxEditHideInactive);
	_gridSize = core::Var::getSafe(cfg::VoxEditGridsize);
	_autoKeyFrame = core::Var::getSafe(cfg::VoxEditAutoKeyFrame);
	if (!_renderContext.init(video::getWindowSize())) {
		return false;
	}

	_camera.setRotationType(video::CameraRotationType::Target);
	resetCamera();

	return true;
}

void Viewport::resize(const glm::ivec2 &frameBufferSize) {
	const glm::vec2 &windowSize = _app->windowDimension();
	const glm::vec2 &windowFrameBufferSize = _app->frameBufferDimension();
	const glm::vec2 scale = windowFrameBufferSize / windowSize;
	const glm::ivec2 cameraSize((float)frameBufferSize.x * scale.x, (float)frameBufferSize.y * scale.y);
	_camera.setSize(cameraSize);
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
	_sceneMgr->trace(_renderContext.sceneMode);
}

void Viewport::dragAndDrop(float headerSize) {
	if (ImGui::BeginDragDropTarget()) {
		if (!isSceneMode()) {
			if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(dragdrop::ImagePayload)) {
				const image::ImagePtr &image = *(const image::ImagePtr *)payload->Data;
				updateViewportTrace(headerSize);
				_sceneMgr->fillPlane(image);
			}
		}
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(dragdrop::PaletteIndexPayload)) {
			const int dragPalIdx = (int)(intptr_t)payload->Data;
			const int nodeId = _sceneMgr->sceneGraph().activeNode();
			if (scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNode(nodeId)) {
				if (node->visible() && node->isModelNode()) {
					updateViewportTrace(headerSize);
					ModifierFacade &modifier = _sceneMgr->modifier();
					modifier.setCursorVoxel(voxel::createVoxel(node->palette(), dragPalIdx));
					modifier.start();
					auto callback = [nodeId, this](const voxel::Region &region, ModifierType type, bool markUndo) {
						if (type != ModifierType::Select && type != ModifierType::ColorPicker) {
							_sceneMgr->modified(nodeId, region, markUndo);
						}
					};
					modifier.execute(_sceneMgr->sceneGraph(), *node, callback);
					modifier.stop();
				}
			}
		}
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(dragdrop::ModelPayload)) {
			const core::String &filename = *(core::String *)payload->Data;
			_sceneMgr->import(filename);
		}

		ImGui::EndDragDropTarget();
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

void Viewport::renderCursor() {
	if (_renderContext.sceneMode) {
		return;
	}

	const ModifierFacade &modifier = _sceneMgr->modifier();
	if (modifier.isMode(ModifierType::ColorPicker) || modifier.isMode(ModifierType::Select) ||
		modifier.brushType() == BrushType::Paint) {
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
	} else if (modifier.brushType() == BrushType::Plane) {
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
	}

	const int cursorDetailsLevel = _cursorDetails->intVal();
	if (cursorDetailsLevel == 0) {
		return;
	}

	const glm::ivec3 &cursorPos = modifier.cursorPosition();
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
			ImGui::TooltipText(_("pos: %i:%i:%i\nsize: %i:%i:%i\nabsolute: %i:%i:%i\nrelative: %i:%i:%i"), mins.x, mins.y,
							   mins.z, size.x, size.y, size.z, cursorPos.x, cursorPos.y, cursorPos.z,
							   cursorPos.x - mins.x, cursorPos.y - mins.y, cursorPos.z - mins.z);
		}
	}
}

void Viewport::renderViewport() {
	core_trace_scoped(Viewport);
	glm::ivec2 contentSize = ImGui::GetContentRegionAvail();
	const float headerSize = ImGui::GetCursorPosY();
	if (setupFrameBuffer(contentSize)) {
		_camera.update(_app->deltaFrameSeconds());

		renderToFrameBuffer();
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

void Viewport::toggleVideoRecording() {
	if (!_captureTool.isRecording()) {
		video::WindowedApp::getInstance()->saveDialog(
			[this](const core::String &file, const io::FormatDescription *desc) {
				const glm::ivec2 &dim = _renderContext.frameBuffer.dimension();
				_captureTool.startRecording(file.c_str(), dim.x, dim.y);
			},
			{}, nullptr, "video.avi");
	} else {
		Log::debug("Stop recording");
		_captureTool.stopRecording();
	}
}

void Viewport::menuBarView(command::CommandExecutionListener *listener) {
	if (ImGui::BeginIconMenu(ICON_LC_EYE, _("View"))) {
		ImGui::CommandIconMenuItem(ICON_LC_VIDEO, _("Reset camera"), "resetcamera", true, listener);

		glm::vec3 omega = _camera.omega();
		if (ImGui::InputFloat(_("Camera rotation"), &omega.y)) {
			_camera.setOmega(omega);
		}

		const core::String command = core::string::format("screenshot %i", _id);
		ImGui::CommandIconMenuItem(ICON_LC_CAMERA, _("Screenshot"), command.c_str(), listener);

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

		if (!isFixedCamera()) {
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
		}

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
		MenuBar::viewportOptions();
		ImGui::EndMenu();
	}
}

void Viewport::renderMenuBar(command::CommandExecutionListener *listener) {
	if (ImGui::BeginMenuBar()) {
		const MementoHandler &mementoHandler = _sceneMgr->mementoHandler();
		ImGui::CommandIconMenuItem(ICON_LC_ROTATE_CCW, _("Undo"), "undo", mementoHandler.canUndo(), listener);
		ImGui::CommandIconMenuItem(ICON_LC_ROTATE_CW, _("Redo"), "redo", mementoHandler.canRedo(), listener);
		ImGui::Dummy(ImVec2(20, 0));
		menuBarCameraProjection();
		menuBarCameraMode();
		if (!_simplifiedView->boolVal()) {
			ImGui::Checkbox(_("Scene Mode"), &_renderContext.sceneMode);
		}
		menuBarView(listener);

		ImGui::EndMenuBar();
	}
}

void Viewport::update(command::CommandExecutionListener *listener) {
	_camera.setFarPlane(_viewDistance->floatVal());

	_hovered = false;
	_visible = false;
	_cameraManipulated = false;

	ui::ScopedStyle style;
	style.setWindowRounding(0.0f);
	style.setWindowBorderSize(0.0f);
	style.setWindowPadding(ImVec2(0.0f, 0.0f));
	const int sceneWindowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
								 ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoFocusOnAppearing;
	const char *modeStr = isSceneMode() ? _("SceneMode") : _("EditMode");

	core::String name;
	if (_detailedTitle) {
		name = core::string::format("%s %s%s", _(voxelrender::SceneCameraModeStr[(int)_camMode]), modeStr, _uiId.c_str());
	} else {
		name = core::string::format("%s%s", modeStr, _uiId.c_str());
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
	return image->writePng(stream);
}

void Viewport::resetCamera() {
	const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
	voxel::Region region;

	const int activeNode = sceneGraph.activeNode();
	if (_renderContext.sceneMode) {
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
	resize(frameBufferSize);
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
	MementoHandler &mementoHandler = _sceneMgr->mementoHandler();
	mementoHandler.unlock();
	_sceneMgr->modifier().unlock();
	if (keyFrameIdx == InvalidKeyFrame) {
		// there is no valid key frame idx given in edit mode
		mementoHandler.markModification(node, node.region());
	} else {
		// we have a valid key frame idx in scene mode
		mementoHandler.markNodeTransform(node, keyFrameIdx);
	}
	_transformMementoLocked = false;
}

void Viewport::lock(const scenegraph::SceneGraphNode &node, scenegraph::KeyFrameIndex keyFrameIdx) {
	if (_transformMementoLocked) {
		return;
	}
	Log::debug("Lock memento state");
	MementoHandler &mementoHandler = _sceneMgr->mementoHandler();
	if (keyFrameIdx != InvalidKeyFrame) {
		mementoHandler.markNodeTransform(node, keyFrameIdx);
	}
	mementoHandler.lock();
	_sceneMgr->modifier().lock();
	_transformMementoLocked = true;
}

void Viewport::updateGizmoValues(const scenegraph::SceneGraphNode &node, scenegraph::KeyFrameIndex keyFrameIdx,
								 const glm::mat4 &matrix) {
	if (ImGuizmo::IsUsing()) {
		lock(node, keyFrameIdx);
		glm::vec3 translate;
		glm::vec3 rotation;
		glm::vec3 scale;
		ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(matrix), glm::value_ptr(translate),
											  glm::value_ptr(rotation), glm::value_ptr(scale));
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
	if (_renderContext.sceneMode) {
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
	if (!isSceneMode()) {
		const voxel::Region &region = sceneGraph.resolveRegion(node);
		return glm::translate(region.getLowerCornerf());
	}
	keyFrameIdx = node.keyFrameForFrame(_sceneMgr->currentFrame());
	const scenegraph::SceneGraphTransform &transform = node.transform(keyFrameIdx);
	return transform.worldMatrix();
}

uint32_t Viewport::gizmoMode() const {
	return ImGuizmo::MODE::WORLD;
}

void Viewport::updateBounds(const scenegraph::SceneGraphNode &node) {
	const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
	const voxel::Region &region = sceneGraph.resolveRegion(node);
	_bounds.mins = region.getLowerCornerf();
	_bounds.maxs = region.getUpperCornerf() + 1.0f;
}

const float *Viewport::gizmoBounds(const scenegraph::SceneGraphNode &node) {
	const float *boundsPtr = nullptr;
	if (isSceneMode() && (_gizmoOperations->uintVal() & GizmoOperation_Bounds) != 0) {
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
	glm::mat4 matrix = gizmoMatrix(node, keyFrameIdx);
	glm::mat4 deltaMatrix(1.0f);
	const float *boundsPtr = gizmoBounds(node);
	const uint32_t operation = gizmoOperation(node);
	const bool manipulated = gizmoManipulate(camera, boundsPtr, matrix, deltaMatrix, operation);
	updateGizmoValues(node, keyFrameIdx, matrix);
	// check to create a reference before we update the node transform
	// otherwise the new reference node will not get the correct transform
	if (createReference(node)) {
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
				const scenegraph::SceneGraphTransform &transform = node.transform(keyFrameIdx);
				const glm::vec3 size = node.region().getDimensionsInVoxels();
				const glm::vec3 pivot = (glm::vec3(matrix[3]) - transform.worldTranslation()) / size;
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
				_sceneMgr->nodeUpdateTransform(activeNode, matrix, keyFrameIdx, false);
			}
		} else {
			const glm::ivec3 shift = glm::vec3(matrix[3]) - node.region().getLowerCornerf();
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
	if (ImGuizmo::IsManipulatorHovered()) {
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

	const bool orthographic = camera.mode() == video::CameraMode::Orthogonal;

	ImGuizmo::SetID(_id);
	ImGuizmo::SetDrawlist();
	ImGuizmo::SetWindow();
	const ImVec2 &windowPos = ImGui::GetWindowPos();
	ImGuizmo::Enable(_renderContext.sceneMode || _modelGizmo->boolVal());
	ImGuizmo::AllowAxisFlip(_gizmoAllowAxisFlip->boolVal());
	ImGuizmo::SetRect(windowPos.x, windowPos.y + headerSize, size.x, size.y);
	ImGuizmo::SetOrthographic(orthographic);
	const bool editModeModified = runGizmo(camera);
	renderCameraManipulator(camera, headerSize);
	return editModeModified;
}

void Viewport::renderToFrameBuffer() {
	core_trace_scoped(EditorSceneRenderFramebuffer);
	video::clearColor(core::Color::Clear());
	_sceneMgr->render(_renderContext, camera());
}

} // namespace voxedit
