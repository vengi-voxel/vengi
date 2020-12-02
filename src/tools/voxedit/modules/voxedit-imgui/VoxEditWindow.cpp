/**
 * @file
 */

#include "VoxEditWindow.h"
#include "Viewport.h"
#include "ui/imgui/IMGUI.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

VoxEditWindow::VoxEditWindow(video::WindowedApp *app) : Super(app) {
	_scene = new Viewport(app);

	_sceneTop = new Viewport(app);
	_sceneTop->setMode(voxedit::ViewportController::SceneCameraMode::Top);

	_sceneLeft = new Viewport(app);
	_sceneLeft->setMode(voxedit::ViewportController::SceneCameraMode::Left);

	_sceneFront = new Viewport(app);
	_sceneFront->setMode(voxedit::ViewportController::SceneCameraMode::Front);

	_sceneAnimation = new Viewport(app);
	_sceneAnimation->setRenderMode(voxedit::ViewportController::RenderMode::Animation);
}

VoxEditWindow::~VoxEditWindow() {
	delete _scene;
	delete _sceneTop;
	delete _sceneLeft;
	delete _sceneFront;
	delete _sceneAnimation;
}

void VoxEditWindow::afterLoad(const core::String &file) {
	_lastOpenedFile->setVal(file);
	resetCamera();
}

bool VoxEditWindow::init() {
	SceneManager &mgr = sceneMgr();
#if 0
	render::GridRenderer& gridRenderer = mgr.gridRenderer();
	gridRenderer.setRenderAABB(_showAABB->getValue() != 0);
	gridRenderer.setRenderGrid(_showGrid->getValue() != 0);
	mgr.setGridResolution(_voxelSize->getValue());
	mgr.setRenderAxis(_showAxis->getValue() != 0);
	mgr.setRenderLockAxis(_showLockAxis->getValue() != 0);
	mgr.setRenderShadow(_renderShadow->getValue() != 0);
#endif

	_lastOpenedFile = core::Var::get(cfg::VoxEditLastFile, "");
	if (mgr.load(_lastOpenedFile->strVal())) {
		afterLoad(_lastOpenedFile->strVal());
	} else {
		voxel::Region region = _layerSettings.region();
		if (region.isValid()) {
			_layerSettings.reset();
			region = _layerSettings.region();
		}
		if (!mgr.newScene(true, _layerSettings.name, region)) {
			return false;
		}
		afterLoad("");
	}
	//_scene->setFocus(tb::WIDGET_FOCUS_REASON_UNKNOWN);

	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 0);
	mgr.modifier().setCursorVoxel(voxel);
	return true;
}

void VoxEditWindow::shutdown() {
	_scene->shutdown();
#if 0
	_sceneTop->shutdown();
	_sceneLeft->shutdown();
	_sceneFront->shutdown();
	_sceneAnimation->shutdown();
#endif
}

void VoxEditWindow::toggleViewport() {
}

void VoxEditWindow::toggleAnimation() {
}

bool VoxEditWindow::save(const core::String &file) {
	return false;
}

bool VoxEditWindow::load(const core::String &file) {
	return false;
}

bool VoxEditWindow::loadAnimationEntity(const core::String &file) {
	return false;
}

bool VoxEditWindow::createNew(bool force) {
	return false;
}

bool VoxEditWindow::isLayerWidgetDropTarget() const {
	return false;
}

bool VoxEditWindow::isPaletteWidgetDropTarget() const {
	return false;
}

void VoxEditWindow::menuBar() {
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New", "new")) {
				_lastExecutedCommand = "new";
			}
			if (ImGui::MenuItem("Load", "load")) {
				_lastExecutedCommand = "load";
			}
			if (ImGui::MenuItem("Save", "save")) {
				_lastExecutedCommand = "save";
			}
			if (ImGui::MenuItem("Load Animation", "animation_load")) {
				_lastExecutedCommand = "animation_load";
			}
			if (ImGui::MenuItem("Save Animation", "animation_save")) {
				_lastExecutedCommand = "animation_save";
			}
			if (ImGui::MenuItem("Prefab", "prefab")) {
				_lastExecutedCommand = "prefab";
			}
			if (ImGui::MenuItem("Heightmap", "importheightmap")) {
				_lastExecutedCommand = "importheightmap";
			}
			if (ImGui::MenuItem("Image as Plane", "importplane")) {
				_lastExecutedCommand = "importplane";
			}
			if (ImGui::MenuItem("Quit")) {
				_app->requestQuit();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
}

void VoxEditWindow::palette() {
#if 0
	if (_paletteWidget != nullptr) {
		_paletteWidget->setVoxelColor(sceneMgr().hitCursorVoxel().getColor());
	}
#endif
}

void VoxEditWindow::scene() {
	_scene->update();
#if 0
	_sceneTop->update();
	_sceneLeft->update();
	_sceneFront->update();
	_sceneAnimation->update();
#endif
}

void VoxEditWindow::tools() {
}

void VoxEditWindow::layers() {
}

void VoxEditWindow::statusBar() {
}

void VoxEditWindow::update() {
	const ImVec2 pos(0.0f, 0.0f);
	const ImVec2 size = _app->frameBufferDimension();
	ImGui::SetNextWindowSize(size);
	ImGui::SetNextWindowPos(pos);
	if (ImGui::Begin("##main", nullptr,
					 ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
						 ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings |
						 ImGuiWindowFlags_MenuBar)) {
		menuBar();
		scene();
		palette();
		tools();
		layers();
		statusBar();
	}
	ImGui::End();
}

bool VoxEditWindow::isSceneHovered() const {
	return false;
}
} // namespace voxedit
