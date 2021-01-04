/**
 * @file
 */

#include "VoxEditWindow.h"
#include "Viewport.h"
#include "command/CommandHandler.h"
#include "ui/imgui/IMGUI.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/anim/AnimationLuaSaver.h"
#include "voxelformat/VolumeFormat.h"

namespace voxedit {

VoxEditWindow::VoxEditWindow(video::WindowedApp *app) : Super(app) {
	_scene = new Viewport(app);
	_scene->init();

	_sceneTop = new Viewport(app);
	_sceneTop->init();
	_sceneTop->setMode(voxedit::ViewportController::SceneCameraMode::Top);

	_sceneLeft = new Viewport(app);
	_sceneLeft->init();
	_sceneLeft->setMode(voxedit::ViewportController::SceneCameraMode::Left);

	_sceneFront = new Viewport(app);
	_sceneFront->init();
	_sceneFront->setMode(voxedit::ViewportController::SceneCameraMode::Front);

	_sceneAnimation = new Viewport(app);
	_sceneAnimation->init();
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
	render::GridRenderer& gridRenderer = mgr.gridRenderer();

	gridRenderer.setRenderAABB(core::Var::get("ve_showaabb", "1")->boolVal());
	gridRenderer.setRenderGrid(core::Var::get("ve_showgrid", "1")->boolVal());
	mgr.setGridResolution(core::Var::get("ve_gridsize", "1")->intVal());
	mgr.setRenderAxis(core::Var::get("ve_showaxis", "1")->boolVal());
	mgr.setRenderLockAxis(core::Var::get("ve_showlockedaxis", "1")->boolVal());
	mgr.setRenderShadow(core::Var::get("ve_rendershadow", "1")->boolVal());

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

	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 0);
	mgr.modifier().setCursorVoxel(voxel);
	return true;
}

void VoxEditWindow::shutdown() {
	_scene->shutdown();
	_sceneTop->shutdown();
	_sceneLeft->shutdown();
	_sceneFront->shutdown();
	_sceneAnimation->shutdown();
}

void VoxEditWindow::toggleViewport() {
}

void VoxEditWindow::toggleAnimation() {
}

bool VoxEditWindow::save(const core::String &file) {
	if (file.empty()) {
		_app->saveDialog([this] (const core::String uifile) {save(uifile); }, voxelformat::SUPPORTED_VOXEL_FORMATS_SAVE);
		return true;
	}
	if (!sceneMgr().save(file)) {
		Log::warn("Failed to save the model");
		//popup(tr("Error"), tr("Failed to save the model"));
		return false;
	}
	Log::info("Saved the model to %s", file.c_str());
	_lastOpenedFile->setVal(file);
	return true;
}

bool VoxEditWindow::load(const core::String &file) {
	if (file.empty()) {
		_app->openDialog([this] (const core::String file) { core::String copy(file); load(copy); }, voxelformat::SUPPORTED_VOXEL_FORMATS_LOAD);
		return true;
	}

	if (!sceneMgr().dirty()) {
		if (sceneMgr().load(file)) {
			afterLoad(file);
			return true;
		}
		return false;
	}

	_loadFile = file;
#if 0
	popup(tr("Unsaved Modifications"),
			tr("There are unsaved modifications.\nDo you wish to discard them and load?"),
			PopupType::YesNo, "unsaved_changes_load");
#endif
	return false;
}

bool VoxEditWindow::loadAnimationEntity(const core::String &file) {
	if (file.empty()) {
		_app->openDialog([this] (const core::String file) { core::String copy(file); loadAnimationEntity(copy); }, "lua");
		return true;
	}
	if (!sceneMgr().loadAnimationEntity(file)) {
		return false;
	}
	resetCamera();
	#if 0
	// TODO:
	const animation::SkeletonAttribute* skeletonAttributes = sceneMgr().skeletonAttributes();
	for (const animation::SkeletonAttributeMeta* metaIter = skeletonAttributes->metaArray(); metaIter->name; ++metaIter) {
		const animation::SkeletonAttributeMeta& meta = *metaIter;
	}
	#endif
	return true;
}

bool VoxEditWindow::createNew(bool force) {
#if 0
	if (!force && sceneMgr().dirty()) {
		popup(tr("Unsaved Modifications"),
				tr("There are unsaved modifications.\nDo you wish to discard them and close?"),
				PopupType::YesNo, "unsaved_changes_new");
	} else {
		// TODO: scene settings and new scene creation
	}
#endif
	return false;
}

bool VoxEditWindow::isLayerWidgetDropTarget() const {
	return false;
}

bool VoxEditWindow::isPaletteWidgetDropTarget() const {
	return false;
}

void VoxEditWindow::menuBar() {
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New", "new")) {
				_lastExecutedCommand = "new";
				command::executeCommands(_lastExecutedCommand);
			}
			if (ImGui::MenuItem("Load", "load")) {
				_lastExecutedCommand = "load";
				command::executeCommands(_lastExecutedCommand);
			}
			if (ImGui::MenuItem("Save", "save")) {
				_lastExecutedCommand = "save";
				command::executeCommands(_lastExecutedCommand);
			}
			if (ImGui::MenuItem("Load Animation", "animation_load")) {
				_lastExecutedCommand = "animation_load";
				command::executeCommands(_lastExecutedCommand);
			}
			if (ImGui::MenuItem("Save Animation", "animation_save")) {
				_lastExecutedCommand = "animation_save";
				command::executeCommands(_lastExecutedCommand);
			}
			if (ImGui::MenuItem("Prefab", "prefab")) {
				_lastExecutedCommand = "prefab";
				command::executeCommands(_lastExecutedCommand);
			}
			if (ImGui::MenuItem("Heightmap", "importheightmap")) {
				_lastExecutedCommand = "importheightmap";
				command::executeCommands(_lastExecutedCommand);
			}
			if (ImGui::MenuItem("Image as Plane", "importplane")) {
				_lastExecutedCommand = "importplane";
				command::executeCommands(_lastExecutedCommand);
			}
			if (ImGui::MenuItem("Quit")) {
				_app->requestQuit();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
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
