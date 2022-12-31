/**
 * @file
 */

#include "MainWindow.h"
#include "ScopedStyle.h"
#include "Viewport.h"
#include "Util.h"
#include "command/CommandHandler.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "io/FormatDescription.h"
#include "ui/IconsForkAwesome.h"
#include "ui/IconsFontAwesome6.h"
#include "ui/IMGUIApp.h"
#include "ui/FileDialog.h"
#include "ui/IMGUIEx.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxelformat/VolumeFormat.h"
#include <glm/gtc/type_ptr.hpp>

#define TITLE_STATUSBAR "##statusbar"
#define TITLE_PALETTE "Palette##title"
#define TITLE_POSITIONS "Positions##title"
#define TITLE_ANIMATION_TIMELINE "Animation##animationtimeline"
#define TITLE_TOOLS "Tools##title"
#define TITLE_MEMENTO "History##title"
#define TITLE_ASSET "Assets##title"
#define TITLE_LAYERS "Layers##title"
#define TITLE_MODIFIERS "Modifiers##title"
#define TITLE_TREES ICON_FA_TREE " Trees##title"
#define TITLE_SCENEGRAPH "Scenegraph##title"
#define TITLE_SCRIPTPANEL ICON_FA_CODE " Script##title"
#define TITLE_LSYSTEMPANEL ICON_FA_LEAF " L-System##title"
#define TITLE_ANIMATION_SETTINGS "Animation##animationsettings"
#define TITLE_SCRIPT_EDITOR ICON_FK_CODE "Script Editor##scripteditor"

#define POPUP_TITLE_UNSAVED "Unsaved Modifications##popuptitle"
#define POPUP_TITLE_NEW_SCENE "New scene##popuptitle"
#define POPUP_TITLE_FAILED_TO_SAVE "Failed to save##popuptitle"
#define POPUP_TITLE_UNSAVED_SCENE "Unsaved scene##popuptitle"
#define POPUP_TITLE_SCENE_SETTINGS "Scene settings##popuptitle"
#define POPUP_TITLE_LAYER_SETTINGS "Layer settings##popuptitle"

namespace voxedit {

MainWindow::MainWindow(ui::IMGUIApp *app) : _app(app), _assetPanel(app->filesystem()) {
	_scene = new Viewport("free##viewport");
	_sceneTop = new Viewport("top##viewport");
	_sceneLeft = new Viewport("left##viewport");
	_sceneFront = new Viewport("front##viewport");
}

MainWindow::~MainWindow() {
	delete _scene;
	delete _sceneTop;
	delete _sceneLeft;
	delete _sceneFront;
}

void MainWindow::resetCamera() {
	_scene->resetCamera();
	_sceneTop->resetCamera();
	_sceneLeft->resetCamera();
	_sceneFront->resetCamera();
}

void MainWindow::loadLastOpenedFiles(const core::String &string) {
	core::DynamicArray<core::String> tokens;
	core::string::splitString(string, tokens, ";");
	for (const core::String& s : tokens) {
		_lastOpenedFilesRingBuffer.push_back(s);
	}
}

void MainWindow::addLastOpenedFile(const core::String &file) {
	for (const core::String& s : _lastOpenedFilesRingBuffer) {
		if (s == file) {
			return;
		}
	}
	_lastOpenedFilesRingBuffer.push_back(file);
	core::String str;
	for (const core::String& s : _lastOpenedFilesRingBuffer) {
		if (!str.empty()) {
			str.append(";");
		}
		str.append(s);
	}
	_lastOpenedFiles->setVal(str);
}

bool MainWindow::init() {
	_scene->init(voxedit::Viewport::RenderMode::Editor);
	_scene->setMode(voxedit::Viewport::SceneCameraMode::Free);

	_sceneTop->init(voxedit::Viewport::RenderMode::Editor);
	_sceneTop->setMode(voxedit::Viewport::SceneCameraMode::Top);

	_sceneLeft->init(voxedit::Viewport::RenderMode::Editor);
	_sceneLeft->setMode(voxedit::Viewport::SceneCameraMode::Left);

	_sceneFront->init(voxedit::Viewport::RenderMode::Editor);
	_sceneFront->setMode(voxedit::Viewport::SceneCameraMode::Front);

	_showGridVar = core::Var::getSafe(cfg::VoxEditShowgrid);
	_showLockedAxisVar = core::Var::getSafe(cfg::VoxEditShowlockedaxis);
	_showAabbVar = core::Var::getSafe(cfg::VoxEditShowaabb);
	_renderShadowVar = core::Var::getSafe(cfg::VoxEditRendershadow);
	_animationSpeedVar = core::Var::getSafe(cfg::VoxEditAnimationSpeed);
	_gridSizeVar = core::Var::getSafe(cfg::VoxEditGridsize);
	_lastOpenedFile = core::Var::getSafe(cfg::VoxEditLastFile);
	_lastOpenedFiles = core::Var::getSafe(cfg::VoxEditLastFiles);
	loadLastOpenedFiles(_lastOpenedFiles->strVal());

	SceneManager &mgr = sceneMgr();
	voxel::Region region = _layerSettings.region();
	if (!region.isValid()) {
		_layerSettings.reset();
		region = _layerSettings.region();
	}
	if (!mgr.newScene(true, _layerSettings.name, region)) {
		return false;
	}
	afterLoad("");
	return true;
}

void MainWindow::shutdown() {
	_scene->shutdown();
	_sceneTop->shutdown();
	_sceneLeft->shutdown();
	_sceneFront->shutdown();
}

bool MainWindow::save(const core::String &file, const io::FormatDescription *desc) {
	io::FileDescription fd;
	fd.set(file, desc);
	if (!sceneMgr().save(fd)) {
		Log::warn("Failed to save the model");
		_popupFailedToSave = true;
		return false;
	}
	Log::info("Saved the model to %s", file.c_str());
	_lastOpenedFile->setVal(file);
	return true;
}

bool MainWindow::load(const core::String &file, const io::FormatDescription *desc) {
	if (file.empty()) {
		_app->openDialog([this] (const core::String file, const io::FormatDescription *desc) { load(file, desc); }, {}, voxelformat::voxelLoad());
		return true;
	}

	if (!sceneMgr().dirty()) {
		io::FileDescription fd;
		fd.set(file, desc);
		if (sceneMgr().load(fd)) {
			afterLoad(file);
			return true;
		}
		return false;
	}

	_loadFile.set(file, desc);
	_popupUnsaved = true;
	return false;
}

void MainWindow::afterLoad(const core::String &file) {
	_lastOpenedFile->setVal(file);
	resetCamera();
}

bool MainWindow::createNew(bool force) {
	if (!force && sceneMgr().dirty()) {
		_loadFile.clear();
		_popupUnsaved = true;
	} else {
		_popupNewScene = true;
	}
	return false;
}

bool MainWindow::isLayerWidgetDropTarget() const {
	return _layerPanel.hasFocus();
}

bool MainWindow::isPaletteWidgetDropTarget() const {
	return _palettePanel.hasFocus();
}

// left side

void MainWindow::configureLeftTopWidgetDock(ImGuiID dockId) {
	ImGui::DockBuilderDockWindow(TITLE_PALETTE, dockId);
}

void MainWindow::configureLeftBottomWidgetDock(ImGuiID dockId) {
	ImGui::DockBuilderDockWindow(TITLE_MODIFIERS, dockId);
}

void MainWindow::leftWidget() {
	_palettePanel.update(TITLE_PALETTE, _lastExecutedCommand);
	_modifierPanel.update(TITLE_MODIFIERS, _lastExecutedCommand);
}

// end of left side

// main space

void MainWindow::configureMainTopWidgetDock(ImGuiID dockId) {
	ImGui::DockBuilderDockWindow(_scene->id().c_str(), dockId);
	ImGui::DockBuilderDockWindow(_sceneTop->id().c_str(), dockId);
	ImGui::DockBuilderDockWindow(_sceneLeft->id().c_str(), dockId);
	ImGui::DockBuilderDockWindow(_sceneFront->id().c_str(), dockId);
}

void MainWindow::configureMainBottomWidgetDock(ImGuiID dockId) {
	ImGui::DockBuilderDockWindow(TITLE_SCRIPT_EDITOR, dockId);
	ImGui::DockBuilderDockWindow(TITLE_ANIMATION_TIMELINE, dockId);
}

void MainWindow::mainWidget() {
	// main
	_scene->update(&_lastExecutedCommand);
	_sceneTop->update(&_lastExecutedCommand);
	_sceneLeft->update(&_lastExecutedCommand);
	_sceneFront->update(&_lastExecutedCommand);

	// bottom
	_scriptPanel.updateEditor(TITLE_SCRIPT_EDITOR, _app);
	_animationTimeline.update(TITLE_ANIMATION_TIMELINE);
}

// end of main space

// right side

void MainWindow::configureRightTopWidgetDock(ImGuiID dockId) {
	ImGui::DockBuilderDockWindow(TITLE_POSITIONS, dockId);
	ImGui::DockBuilderDockWindow(TITLE_TOOLS, dockId);
	ImGui::DockBuilderDockWindow(TITLE_ASSET, dockId);
	ImGui::DockBuilderDockWindow(TITLE_ANIMATION_SETTINGS, dockId);
	ImGui::DockBuilderDockWindow(TITLE_MEMENTO, dockId);
}

void MainWindow::configureRightBottomWidgetDock(ImGuiID dockId) {
	ImGui::DockBuilderDockWindow(TITLE_LAYERS, dockId);
	ImGui::DockBuilderDockWindow(TITLE_SCENEGRAPH, dockId);
	ImGui::DockBuilderDockWindow(TITLE_TREES, dockId);
	ImGui::DockBuilderDockWindow(TITLE_LSYSTEMPANEL, dockId);
	ImGui::DockBuilderDockWindow(TITLE_SCRIPTPANEL, dockId);
}

void MainWindow::rightWidget() {
	// top
	_positionsPanel.update(TITLE_POSITIONS, _lastExecutedCommand);
	_toolsPanel.update(TITLE_TOOLS, _lastExecutedCommand);
	_assetPanel.update(TITLE_ASSET, _lastExecutedCommand);
	_animationPanel.update(TITLE_ANIMATION_SETTINGS, _lastExecutedCommand);
	_mementoPanel.update(TITLE_MEMENTO, _lastExecutedCommand);

	// bottom
	_layerPanel.update(TITLE_LAYERS, &_layerSettings, _lastExecutedCommand);
	_sceneGraphPanel.update(_scene->camera(), TITLE_SCENEGRAPH, &_layerSettings, _lastExecutedCommand);
	_treePanel.update(TITLE_TREES);
	_lsystemPanel.update(TITLE_LSYSTEMPANEL);
	_scriptPanel.update(TITLE_SCRIPTPANEL);
}

// end of right side

void MainWindow::updateSettings() {
	SceneManager &mgr = sceneMgr();
	mgr.setGridResolution(_gridSizeVar->intVal());
	mgr.setRenderLockAxis(_showLockedAxisVar->boolVal());
	mgr.setRenderShadow(_renderShadowVar->boolVal());

	render::GridRenderer &gridRenderer = mgr.gridRenderer();
	gridRenderer.setRenderAABB(_showAabbVar->boolVal());
	gridRenderer.setRenderGrid(_showGridVar->boolVal());
}

void MainWindow::dialog(const char *icon, const char *text) {
	ImGui::AlignTextToFramePadding();
	ImGui::PushFont(_app->bigFont());
	ImGui::TextUnformatted(icon);
	ImGui::PopFont();
	ImGui::SameLine();
	ImGui::Spacing();
	ImGui::SameLine();
	ImGui::TextUnformatted(text);
	ImGui::Spacing();
	ImGui::Separator();
}

void MainWindow::registerPopups() {
	if (_popupUnsaved) {
		ImGui::OpenPopup(POPUP_TITLE_UNSAVED);
		_popupUnsaved = false;
	}
	if (_popupNewScene) {
		ImGui::OpenPopup(POPUP_TITLE_NEW_SCENE);
		_popupNewScene = false;
	}
	if (_popupFailedToSave) {
		ImGui::OpenPopup(POPUP_TITLE_FAILED_TO_SAVE);
		_popupFailedToSave = false;
	}
	if (_menuBar._popupSceneSettings) {
		ImGui::OpenPopup(POPUP_TITLE_SCENE_SETTINGS);
		_menuBar._popupSceneSettings = false;
	}
	if (_popupUnsavedChangesQuit) {
		ImGui::OpenPopup(POPUP_TITLE_UNSAVED_SCENE);
		_popupUnsavedChangesQuit = false;
	}
	if (_layerPanel._popupNewLayer || _sceneGraphPanel._popupNewLayer) {
		ImGui::OpenPopup(POPUP_TITLE_LAYER_SETTINGS);
		_layerPanel._popupNewLayer = _sceneGraphPanel._popupNewLayer = false;
	}

	if (ImGui::BeginPopupModal(POPUP_TITLE_LAYER_SETTINGS, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Name");
		ImGui::Separator();
		ImGui::InputText("##layersettingsname", &_layerSettings.name);
		ImGui::NewLine();

		ImGui::Text("Position");
		ImGui::Separator();
		veui::InputAxisInt(math::Axis::X, "##posx", &_layerSettings.position.x);
		veui::InputAxisInt(math::Axis::Y, "##posy", &_layerSettings.position.y);
		veui::InputAxisInt(math::Axis::Z, "##posz", &_layerSettings.position.z);
		ImGui::NewLine();

		ImGui::Text("Size");
		ImGui::Separator();
		veui::InputAxisInt(math::Axis::X, "Width##sizex", &_layerSettings.size.x);
		veui::InputAxisInt(math::Axis::Y, "Height##sizey", &_layerSettings.size.y);
		veui::InputAxisInt(math::Axis::Z, "Depth##sizez", &_layerSettings.size.z);
		ImGui::NewLine();

		if (ImGui::Button(ICON_FA_CHECK " OK##layersettings")) {
			ImGui::CloseCurrentPopup();
			voxelformat::SceneGraphNode node;
			voxel::RawVolume* v = new voxel::RawVolume(_layerSettings.region());
			node.setVolume(v, true);
			node.setName(_layerSettings.name.c_str());
			sceneMgr().addNodeToSceneGraph(node, _layerSettings.parent);
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_XMARK " Cancel##layersettings")) {
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	if (ImGui::BeginPopup(POPUP_TITLE_SCENE_SETTINGS, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::TextUnformatted("Scene settings");
		ImGui::Separator();

		ImGui::ColorEdit3Var("Diffuse color", cfg::VoxEditDiffuseColor);
		ImGui::ColorEdit3Var("Ambient color", cfg::VoxEditAmbientColor);

		if (ImGui::Button(ICON_FA_CHECK " Done##scenesettings")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal(POPUP_TITLE_UNSAVED, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		dialog(ICON_FA_QUESTION, "There are unsaved modifications.\nDo you wish to discard them?");
		if (ImGui::Button(ICON_FA_CHECK " Yes##unsaved")) {
			ImGui::CloseCurrentPopup();
			if (!_loadFile.empty()) {
				sceneMgr().load(_loadFile);
				afterLoad(_loadFile.name);
			} else {
				createNew(true);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_XMARK " No##unsaved")) {
			ImGui::CloseCurrentPopup();
			_loadFile.clear();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal(POPUP_TITLE_UNSAVED_SCENE, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		dialog(ICON_FA_QUESTION, "Unsaved changes - are you sure to quit?");
		if (ImGui::Button(ICON_FA_CHECK " OK##unsavedscene")) {
			_forceQuit = true;
			_app->requestQuit();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_XMARK " Cancel##unsavedscene")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopup(POPUP_TITLE_FAILED_TO_SAVE, ImGuiWindowFlags_AlwaysAutoResize)) {
		dialog(ICON_FA_TRIANGLE_EXCLAMATION, "Failed to save the model!");
		if (ImGui::Button(ICON_FA_CHECK " OK##failedsave")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal(POPUP_TITLE_NEW_SCENE, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Name");
		ImGui::Separator();
		ImGui::InputText("##newscenename", &_layerSettings.name);
		ImGui::NewLine();

		ImGui::Text("Position");
		ImGui::Separator();
		veui::InputAxisInt(math::Axis::X, "##posx", &_layerSettings.position.x);
		veui::InputAxisInt(math::Axis::Y, "##posy", &_layerSettings.position.y);
		veui::InputAxisInt(math::Axis::Z, "##posz", &_layerSettings.position.z);
		ImGui::NewLine();

		ImGui::Text("Size");
		ImGui::Separator();
		veui::InputAxisInt(math::Axis::X, "Width##sizex", &_layerSettings.size.x);
		veui::InputAxisInt(math::Axis::Y, "Height##sizey", &_layerSettings.size.y);
		veui::InputAxisInt(math::Axis::Z, "Depth##sizez", &_layerSettings.size.z);
		ImGui::NewLine();

		if (ImGui::Button(ICON_FA_CHECK " OK##newscene")) {
			ImGui::CloseCurrentPopup();
			const voxel::Region &region = _layerSettings.region();
			if (voxedit::sceneMgr().newScene(true, _layerSettings.name, region)) {
				afterLoad("");
			}
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_XMARK " Close##newscene")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

bool MainWindow::allowToQuit() {
	if (_forceQuit) {
		return true;
	}
	if (voxedit::sceneMgr().dirty()) {
		_popupUnsavedChangesQuit = true;
		return false;
	}
	return true;
}

void MainWindow::update() {
	core_trace_scoped(MainWindow);
	ImGuiViewport *viewport = ImGui::GetMainViewport();
	const float statusBarHeight = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.y * 2.0f;

	if (_lastOpenedFile->isDirty()) {
		_lastOpenedFile->markClean();
		addLastOpenedFile(_lastOpenedFile->strVal());
	}

	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, viewport->WorkSize.y - statusBarHeight));
	ImGui::SetNextWindowViewport(viewport->ID);
	{
		ui::ScopedStyle style;
		style.setWindowRounding(0.0f);
		style.setWindowBorderSize(0.0f);
		style.setWindowPadding(ImVec2(0.0f, 0.0f));
		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		windowFlags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
		windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoMove;
		if (!core::Var::getSafe(cfg::ClientFullscreen)->boolVal()) {
			windowFlags |= ImGuiWindowFlags_NoTitleBar;
		}
		if (sceneMgr().dirty()) {
			windowFlags |= ImGuiWindowFlags_UnsavedDocument;
		}

		core::String windowTitle = core::string::extractFilenameWithExtension(_lastOpenedFile->strVal());
		if (windowTitle.empty()) {
			windowTitle = _app->appname();
		} else {
			windowTitle.append(" - ");
			windowTitle.append(_app->appname());
		}
		windowTitle.append("###app");
		static bool keepRunning = true;
		if (!ImGui::Begin(windowTitle.c_str(), &keepRunning, windowFlags)) {
			ImGui::SetWindowCollapsed(ImGui::GetCurrentWindow(), false);
			ImGui::End();
			_app->minimize();
			return;
		}
		if (!keepRunning) {
			_app->requestQuit();
		}
	}

	ImGuiID dockIdMain = ImGui::GetID("DockSpace");

	_menuBar.setLastOpenedFiles(_lastOpenedFilesRingBuffer);
	if (_menuBar.update(_app, _lastExecutedCommand)) {
		ImGui::DockBuilderRemoveNode(dockIdMain);
	}

	const bool existingLayout = ImGui::DockBuilderGetNode(dockIdMain);
	ImGui::DockSpace(dockIdMain);

	leftWidget();
	mainWidget();
	rightWidget();

	registerPopups();

	ImGui::End();

	_statusBar.update(TITLE_STATUSBAR, statusBarHeight, _lastExecutedCommand.command);

	if (!existingLayout && viewport->WorkSize.x > 0.0f) {
		ImGui::DockBuilderAddNode(dockIdMain, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockIdMain, viewport->WorkSize);
		ImGuiID dockIdLeft = ImGui::DockBuilderSplitNode(dockIdMain, ImGuiDir_Left, 0.13f, nullptr, &dockIdMain);
		ImGuiID dockIdRight = ImGui::DockBuilderSplitNode(dockIdMain, ImGuiDir_Right, 0.20f, nullptr, &dockIdMain);
		ImGuiID dockIdLeftDown = ImGui::DockBuilderSplitNode(dockIdLeft, ImGuiDir_Down, 0.35f, nullptr, &dockIdLeft);
		ImGuiID dockIdRightDown = ImGui::DockBuilderSplitNode(dockIdRight, ImGuiDir_Down, 0.50f, nullptr, &dockIdRight);
		ImGuiID dockIdMainDown = ImGui::DockBuilderSplitNode(dockIdMain, ImGuiDir_Down, 0.20f, nullptr, &dockIdMain);

		// left side
		configureLeftTopWidgetDock(dockIdLeft);
		configureLeftBottomWidgetDock(dockIdLeftDown);

		// right side
		configureRightTopWidgetDock(dockIdRight);
		configureRightBottomWidgetDock(dockIdRightDown);

		// main
		configureMainTopWidgetDock(dockIdMain);
		configureMainBottomWidgetDock(dockIdMainDown);

		ImGui::DockBuilderFinish(dockIdMain);
	}

	updateSettings();
}

bool MainWindow::isSceneHovered() const {
	return _scene->isHovered() || _sceneTop->isHovered() ||
		   _sceneLeft->isHovered() || _sceneFront->isHovered();
}

bool MainWindow::saveScreenshot(const core::String& file) {
	if (!_scene->saveImage(file.c_str())) {
		Log::warn("Failed to save screenshot to file '%s'", file.c_str());
		return false;
	}
	Log::info("Screenshot created at '%s'", file.c_str());
	return true;
}

} // namespace voxedit
