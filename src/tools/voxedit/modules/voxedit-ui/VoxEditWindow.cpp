/**
 * @file
 */

#include "VoxEditWindow.h"
#include "Viewport.h"
#include "command/CommandHandler.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "ui/imgui/IconsForkAwesome.h"
#include "ui/imgui/IconsFontAwesome5.h"
#include "ui/imgui/IMGUIApp.h"
#include "ui/imgui/FileDialog.h"
#include "ui/imgui/IMGUI.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/anim/AnimationLuaSaver.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxel/MaterialColor.h"
#include "voxelformat/VolumeFormat.h"
#include "engine-config.h"
#include <glm/gtc/type_ptr.hpp>

#define TITLE_PALETTE "Palette##title"
#define TITLE_POSITIONS "Positions##title"
#define TITLE_MODIFIERS "Modifiers##title"
#define TITLE_LAYERS "Layers##title"
#define TITLE_TOOLS "Tools##title"
#define TITLE_TREES ICON_FA_TREE " Trees##title"
#define TITLE_NOISEPANEL ICON_FA_RANDOM " Noise##title"
#define TITLE_SCRIPTPANEL ICON_FA_CODE " Script##title"
#define TITLE_LSYSTEMPANEL ICON_FA_LEAF " L-System##title"
#define TITLE_ANIMATION_SETTINGS "Animation##animationsettings"

#define POPUP_TITLE_UNSAVED "Unsaved Modifications##popuptitle"
#define POPUP_TITLE_NEW_SCENE "New scene##popuptitle"
#define POPUP_TITLE_FAILED_TO_SAVE "Failed to save##popuptitle"
#define POPUP_TITLE_LOAD_PALETTE "Select Palette##popuptitle"
#define POPUP_TITLE_SCENE_SETTINGS "Scene settings##popuptitle"
#define WINDOW_TITLE_SCRIPT_EDITOR ICON_FK_CODE "Script Editor##scripteditor"

namespace voxedit {

VoxEditWindow::VoxEditWindow(ui::imgui::IMGUIApp *app) : _app(app) {
	_scene = new Viewport(app, "free##viewport");
	_scene->init();

	_sceneTop = new Viewport(app, "top##viewport");
	_sceneTop->init();
	_sceneTop->setMode(voxedit::ViewportController::SceneCameraMode::Top);

	_sceneLeft = new Viewport(app, "left##viewport");
	_sceneLeft->init();
	_sceneLeft->setMode(voxedit::ViewportController::SceneCameraMode::Left);

	_sceneFront = new Viewport(app, "front##viewport");
	_sceneFront->init();
	_sceneFront->setMode(voxedit::ViewportController::SceneCameraMode::Front);

	_sceneAnimation = new Viewport(app, "animation##viewport");
	_sceneAnimation->init(voxedit::ViewportController::RenderMode::Animation);

	_currentSelectedPalette = voxel::getDefaultPaletteName();
}

VoxEditWindow::~VoxEditWindow() {
	delete _scene;
	delete _sceneTop;
	delete _sceneLeft;
	delete _sceneFront;
	delete _sceneAnimation;
}

void VoxEditWindow::resetCamera() {
	_scene->resetCamera();
	_sceneTop->resetCamera();
	_sceneLeft->resetCamera();
	_sceneFront->resetCamera();
	_sceneAnimation->resetCamera();
}

bool VoxEditWindow::actionMenuItem(const char *title, const char *command, bool enabled) {
	return ImGui::CommandMenuItem(title, command, enabled, &_lastExecutedCommand);
}

bool VoxEditWindow::init() {
	_showAxisVar = core::Var::get(cfg::VoxEditShowaxis, "1");
	_showGridVar = core::Var::get(cfg::VoxEditShowgrid, "1");
	_modelSpaceVar = core::Var::get(cfg::VoxEditModelSpace, "0");
	_showLockedAxisVar = core::Var::get(cfg::VoxEditShowlockedaxis, "1");
	_showAabbVar = core::Var::get(cfg::VoxEditShowaabb, "0");
	_renderShadowVar = core::Var::get(cfg::VoxEditRendershadow, "1");
	_animationSpeedVar = core::Var::get(cfg::VoxEditAnimationSpeed, "100");
	_gridSizeVar = core::Var::get(cfg::VoxEditGridsize, "4", "The size of the voxel grid", [](const core::String &val) {
		const int intVal = core::string::toInt(val);
		return intVal >= 1 && intVal <= 64;
	});
	_lastOpenedFile = core::Var::get(cfg::VoxEditLastFile, "");

	updateSettings();

	SceneManager &mgr = sceneMgr();
	if (mgr.load(_lastOpenedFile->strVal())) {
		afterLoad(_lastOpenedFile->strVal());
	} else {
		voxel::Region region = _layerSettings.region();
		if (!region.isValid()) {
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

bool VoxEditWindow::save(const core::String &file) {
	if (file.empty()) {
		_app->saveDialog([this](const core::String uifile) { save(uifile); },
						 voxelformat::SUPPORTED_VOXEL_FORMATS_SAVE);
		return true;
	}
	if (!sceneMgr().save(file)) {
		Log::warn("Failed to save the model");
		_popupFailedToSave = true;
		return false;
	}
	Log::info("Saved the model to %s", file.c_str());
	_lastOpenedFile->setVal(file);
	return true;
}

bool VoxEditWindow::load(const core::String &file) {
	if (file.empty()) {
		_app->openDialog([this] (const core::String file) { load(file); }, voxelformat::SUPPORTED_VOXEL_FORMATS_LOAD);
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
	_popupUnsaved = true;
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
	return true;
}

void VoxEditWindow::afterLoad(const core::String &file) {
	_lastOpenedFile->setVal(file);
	resetCamera();
}

bool VoxEditWindow::importAsPlane(const core::String& file) {
	if (file.empty()) {
		_app->openDialog([this] (const core::String file) { importAsPlane(file); }, "png");
		return true;
	}

	return sceneMgr().importAsPlane(file);
}

bool VoxEditWindow::importPalette(const core::String& file) {
	if (file.empty()) {
		_app->openDialog([this] (const core::String file) { importPalette(file); }, "png");
		return true;
	}

	return sceneMgr().importPalette(file);
}

bool VoxEditWindow::importHeightmap(const core::String& file) {
	if (file.empty()) {
		_app->openDialog([this] (const core::String file) { importHeightmap(file); }, "png");
		return true;
	}

	return sceneMgr().importHeightmap(file);
}

bool VoxEditWindow::createNew(bool force) {
	if (!force && sceneMgr().dirty()) {
		_loadFile.clear();
		_popupUnsaved = true;
	} else {
		_popupNewScene = true;
	}
	return false;
}

bool VoxEditWindow::isLayerWidgetDropTarget() const {
	return false; // TODO
}

bool VoxEditWindow::isPaletteWidgetDropTarget() const {
	return false; // TODO
}

void VoxEditWindow::menuBar() {
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu(ICON_FA_FILE " File")) {
			actionMenuItem("New", "new");
			actionMenuItem(ICON_FK_FLOPPY_O " Load", "load");
			actionMenuItem(ICON_FA_SAVE " Save", "save");
			ImGui::Separator();
			actionMenuItem("Load Animation", "animation_load");
			actionMenuItem(ICON_FA_SAVE " Save Animation", "animation_save");
			ImGui::Separator();
			actionMenuItem("Prefab", "prefab");
			ImGui::Separator();
			actionMenuItem(ICON_FA_IMAGE " Heightmap", "importheightmap");
			actionMenuItem(ICON_FA_IMAGE " Image as Plane", "importplane");
			ImGui::Separator();
			if (ImGui::MenuItem("Quit")) {
				_app->requestQuit();
			}
			ImGui::EndMenu();
		}
		actionMenuItem(ICON_FA_UNDO " Undo", "undo", sceneMgr().mementoHandler().canUndo());
		actionMenuItem(ICON_FA_REDO " Redo", "redo", sceneMgr().mementoHandler().canRedo());
		if (ImGui::BeginMenu(ICON_FA_COG " Options")) {
			ImGui::CheckboxVar(ICON_FA_BORDER_ALL " Grid", _showGridVar);
			ImGui::CheckboxVar("Show axis", _showAxisVar);
			ImGui::CheckboxVar("Model space", _modelSpaceVar);
			ImGui::CheckboxVar("Show locked axis", _showLockedAxisVar);
			ImGui::CheckboxVar(ICON_FA_DICE_SIX " Bounding box", _showAabbVar);
			ImGui::CheckboxVar("Shadow", _renderShadowVar);
			ImGui::CheckboxVar("Outlines", "r_renderoutline");
			if (ImGui::Button("Scene settings")) {
				_popupSceneSettings = true;
			}
			if (ImGui::Button("Bindings")) {
				_app->showBindingsDialog();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu(ICON_FA_EYE" View")) {
			actionMenuItem("Reset camera", "resetcamera");
			actionMenuItem("Scene view", "togglescene");
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu(ICON_FK_INFO" About")) {
			ImGui::Text("VoxEdit " PROJECT_VERSION);
			ImGui::Separator();

			ImGui::URLItem(ICON_FK_GITHUB " Bug reports", "https://github.com/mgerhardy/engine");
			ImGui::URLItem(ICON_FK_TWITTER " Twitter", "https://twitter.com/MartinGerhardy");
			ImGui::URLItem(ICON_FK_DISCORD " Discord", "https://discord.gg/AgjCPXy");

			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
}

void VoxEditWindow::palette() {
	const voxel::MaterialColorArray &colors = voxel::getMaterialColors();
	const float height = ImGui::GetContentRegionMax().y;
	const float width = ImGui::Size(120.0f);
	const ImVec2 size(width, height);
	ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);
	int voxelColorTraceIndex = sceneMgr().hitCursorVoxel().getColor();
	int voxelColorSelectedIndex = sceneMgr().modifier().cursorVoxel().getColor();
	if (ImGui::Begin(TITLE_PALETTE, nullptr, ImGuiWindowFlags_NoDecoration)) {
		ImVec2 pos = ImGui::GetWindowPos();
		pos.x += ImGui::GetWindowContentRegionMin().x;
		pos.y += ImGui::GetWindowContentRegionMin().y;
		const float size = ImGui::Size(20);
		const ImVec2& maxs = ImGui::GetWindowContentRegionMax();
		const ImVec2& mins = ImGui::GetWindowContentRegionMin();
		const int amountX = (int)((maxs.x - mins.x) / size);
		const int amountY = (int)((maxs.y - mins.y) / size);
		const int max = colors.size();
		int i = 0;
		float usedHeight = 0;
		bool colorHovered = false;
		for (int y = 0; y < amountY; ++y) {
			for (int x = 0; x < amountX; ++x) {
				if (i >= max) {
					break;
				}
				const float transX = pos.x + (float)x * size;
				const float transY = pos.y + (float)y * size;
				const ImVec2 v1(transX, transY);
				const ImVec2 v2(transX + (float)size, transY + (float)size);
				ImDrawList* drawList = ImGui::GetWindowDrawList();
				drawList->AddRectFilled(v1, v2, ImGui::GetColorU32(colors[i]));

				if (!colorHovered && ImGui::IsMouseHoveringRect(v1, v2)) {
					colorHovered = true;
					drawList->AddRect(v1, v2, ImGui::GetColorU32(core::Color::Red));
					if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
						sceneMgr().modifier().setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, i));
					}
				} else if (i == voxelColorTraceIndex) {
					drawList->AddRect(v1, v2, ImGui::GetColorU32(core::Color::Yellow));
				} else if (i == voxelColorSelectedIndex) {
					drawList->AddRect(v1, v2, ImGui::GetColorU32(core::Color::DarkRed));
				} else {
					drawList->AddRect(v1, v2, ImGui::GetColorU32(core::Color::Black));
				}
				++i;
			}
			if (i >= max) {
				break;
			}
			usedHeight += size;
		}

		ImGui::SetCursorPosY(pos.y + usedHeight);
		ImGui::Text("Color: %i (voxel %i)", voxelColorSelectedIndex, voxelColorTraceIndex);
		ImGui::TooltipText("Palette color index for current voxel under cursor");
		ImGui::CommandButton("Import palette", "importpalette", nullptr, 0.0f, &_lastExecutedCommand);
		ImGui::SameLine();
		if (ImGui::Button("Load palette##button")) {
			reloadAvailablePalettes();
			ImGui::OpenPopup(POPUP_TITLE_LOAD_PALETTE);
		}

		if (ImGui::BeginPopupModal(POPUP_TITLE_LOAD_PALETTE, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::TextUnformatted("Select the palette");
			ImGui::Separator();
			if (ImGui::BeginCombo(ICON_FA_TREE " Type", _currentSelectedPalette.c_str(), 0)) {
				for (const core::String& palette : _availablePalettes) {
					if (ImGui::Selectable(palette.c_str(), palette == _currentSelectedPalette)) {
						_currentSelectedPalette = palette;
					}
				}
				ImGui::EndCombo();
			}
			if (ImGui::Button(ICON_FA_CHECK " OK##loadpalette")) {
				sceneMgr().loadPalette(_currentSelectedPalette);
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_TIMES " Cancel##loadpalette")) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::SetItemDefaultFocus();
			ImGui::EndPopup();
		}
	}
	ImGui::End();
}

void VoxEditWindow::reloadAvailablePalettes() {
	core::DynamicArray<io::Filesystem::DirEntry> entities;
	io::filesystem()->list("", entities, "palette-*.png");
	if (entities.empty()) {
		Log::error("Could not find any palettes");
	}
	_availablePalettes.clear();
	for (const io::Filesystem::DirEntry& file : entities) {
		if (file.type != io::Filesystem::DirEntry::Type::file) {
			continue;
		}
		const core::String& name = voxel::extractPaletteName(file.name);
		_availablePalettes.push_back(name);
	}
}

void VoxEditWindow::statusBar() {
	ImGuiViewport *viewport = ImGui::GetMainViewport();
	const ImVec2 &size = viewport->WorkSize;
	const float statusBarHeight = ImGui::Size((float)((ui::imgui::IMGUIApp*)_app)->fontSize() + 16.0f);
	ImGui::SetNextWindowSize(ImVec2(size.x, statusBarHeight));
	ImVec2 statusBarPos = viewport->WorkPos;
	statusBarPos.y += size.y - statusBarHeight;
	ImGui::SetNextWindowPos(statusBarPos);
	const uint32_t statusBarFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove;
	if (ImGui::Begin("##statusbar", nullptr, statusBarFlags)) {
		const voxedit::SceneManager& sceneMgr = voxedit::sceneMgr();
		const voxedit::LayerManager& layerMgr = sceneMgr.layerMgr();
		const voxedit::ModifierFacade& modifier = sceneMgr.modifier();

		const int layerIdx = layerMgr.activeLayer();
		const voxel::RawVolume* v = sceneMgr.volume(layerIdx);
		const voxel::Region& region = v->region();
		const glm::ivec3& mins = region.getLowerCorner();
		const glm::ivec3& maxs = region.getUpperCorner();
		const core::String& str = core::string::format("%i:%i:%i / %i:%i:%i", mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
		ImGui::Text("%s", str.c_str());
		ImGui::SameLine();

		if (modifier.aabbMode()) {
			const glm::ivec3& dim = modifier.aabbDim();
			const core::String& str = core::string::format("w: %i, h: %i, d: %i", dim.x, dim.y, dim.z);
			ImGui::Text("%s", str.c_str());
		} else if (!_lastExecutedCommand.command.empty()) {
			const video::WindowedApp* app = video::WindowedApp::getInstance();
			core::String statusText;
			const core::String& keybindingStr = app->getKeyBindingsString(_lastExecutedCommand.command.c_str());
			if (keybindingStr.empty()) {
				statusText = core::string::format("Command: %s", _lastExecutedCommand.command.c_str());
			} else {
				statusText = core::string::format("Command: %s (%s)", _lastExecutedCommand.command.c_str(), keybindingStr.c_str());
			}
			ImGui::Text("%s", statusText.c_str());
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::Size(140.0f));
		ImGui::InputVarInt("Grid size", _gridSizeVar);
	}
	ImGui::End();
}

void VoxEditWindow::leftWidget() {
	palette();
	_toolsPanel.update(TITLE_TOOLS);
}

void VoxEditWindow::mainWidget() {
	_scene->update();
	_sceneTop->update();
	_sceneLeft->update();
	_sceneFront->update();
	_sceneAnimation->update();
}

void VoxEditWindow::rightWidget() {
	_cursorPanel.update(TITLE_POSITIONS, _lastExecutedCommand);
	_modifierPanel.update(TITLE_MODIFIERS, _lastExecutedCommand);
	_animationPanel.update(TITLE_ANIMATION_SETTINGS, _lastExecutedCommand);
	_treePanel.update(TITLE_TREES);
	_scriptPanel.update(TITLE_SCRIPTPANEL, WINDOW_TITLE_SCRIPT_EDITOR, _app);
	_lsystemPanel.update(TITLE_LSYSTEMPANEL);
	_noisePanel.update(TITLE_NOISEPANEL);
	_layerPanel.update(TITLE_LAYERS, &_layerSettings, _lastExecutedCommand);
}

void VoxEditWindow::updateSettings() {
	SceneManager &mgr = sceneMgr();
	mgr.setGridResolution(_gridSizeVar->intVal());
	mgr.setRenderAxis(_showAxisVar->boolVal());
	mgr.setRenderLockAxis(_showLockedAxisVar->boolVal());
	mgr.setRenderShadow(_renderShadowVar->boolVal());

	render::GridRenderer &gridRenderer = mgr.gridRenderer();
	gridRenderer.setRenderAABB(_showAabbVar->boolVal());
	gridRenderer.setRenderGrid(_showGridVar->boolVal());
}

void VoxEditWindow::dialog(const char *icon, const char *text) {
	ImGui::AlignTextToFramePadding();
	ImGui::PushFont(_app->bigFont());
	ImGui::Text("%s", icon);
	ImGui::PopFont();
	ImGui::SameLine();
	ImGui::Spacing();
	ImGui::SameLine();
	ImGui::TextWrapped("%s", text);
	ImGui::Spacing();
	ImGui::Separator();
}

void VoxEditWindow::registerPopups() {
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
	if (_popupSceneSettings) {
		ImGui::OpenPopup(POPUP_TITLE_SCENE_SETTINGS);
		_popupSceneSettings = false;
	}

	if (ImGui::BeginPopup(POPUP_TITLE_SCENE_SETTINGS, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::TextUnformatted("Scene settings");
		ImGui::Separator();
		glm::vec3 col;

		col = core::Var::getSafe(cfg::VoxEditAmbientColor)->vec3Val();
		if (ImGui::ColorEdit3("Diffuse color", glm::value_ptr(col))) {
			const core::String &c = core::string::format("%f %f %f", col.x, col.y, col.z);
			core::Var::getSafe(cfg::VoxEditAmbientColor)->setVal(c);
		}

		col = core::Var::getSafe(cfg::VoxEditDiffuseColor)->vec3Val();
		if (ImGui::ColorEdit3("Ambient color", glm::value_ptr(col))) {
			const core::String &c = core::string::format("%f %f %f", col.x, col.y, col.z);
			core::Var::getSafe(cfg::VoxEditAmbientColor)->setVal(c);
		}

		glm::vec3 sunPosition = sceneMgr().renderer().shadow().sunPosition();
		if (ImGui::InputVec3("Sun position", sunPosition)) {
			sceneMgr().renderer().setSunPosition(sunPosition, glm::zero<glm::vec3>(), glm::up);
		}

#if 0
		glm::vec3 sunDirection = sceneMgr().renderer().shadow().sunDirection();
		if (ImGui::InputVec3("Sun direction", sunDirection)) {
			// TODO: sun direction
		}
#endif

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
				afterLoad(_loadFile);
			} else {
				createNew(true);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_TIMES " No##unsaved")) {
			ImGui::CloseCurrentPopup();
			_loadFile.clear();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopup(POPUP_TITLE_FAILED_TO_SAVE, ImGuiWindowFlags_AlwaysAutoResize)) {
		dialog(ICON_FA_EXCLAMATION_TRIANGLE, "Failed to save the model!");
		if (ImGui::Button(ICON_FA_CHECK " OK##failedsave")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal(POPUP_TITLE_NEW_SCENE, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::InputText("Name", &_layerSettings.name);
		ImGui::InputVec3("Position", _layerSettings.position);
		ImGui::InputVec3("Size", _layerSettings.size);
		if (ImGui::Button(ICON_FA_CHECK " OK##newscene")) {
			ImGui::CloseCurrentPopup();
			const voxel::Region &region = _layerSettings.region();
			if (voxedit::sceneMgr().newScene(true, _layerSettings.name, region)) {
				afterLoad("");
			}
		}
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_TIMES " Close##newscene")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}
}

void VoxEditWindow::update() {
	const ImVec2 pos(0.0f, 0.0f);
	// const ImVec2 size = _app->frameBufferDimension();
	ImGuiViewport *viewport = ImGui::GetMainViewport();

	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
	windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoMove;
	ImGui::Begin("##app", nullptr, windowFlags);
	ImGui::PopStyleVar(3);

	menuBar();
	statusBar();

	const ImGuiID dockspaceId = ImGui::GetID("DockSpace");
	ImGui::DockSpace(dockspaceId);

	leftWidget();
	mainWidget();
	rightWidget();

	registerPopups();

	ImGui::End();

	static bool init = false;
	if (!init) {
		ImGui::DockBuilderRemoveNode(dockspaceId);
		ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);
		ImGuiID dockIdMain = dockspaceId;
		ImGuiID dockIdLeft = ImGui::DockBuilderSplitNode(dockIdMain, ImGuiDir_Left, 0.10f, nullptr, &dockIdMain);
		ImGuiID dockIdRight = ImGui::DockBuilderSplitNode(dockIdMain, ImGuiDir_Right, 0.20f, nullptr, &dockIdMain);
		ImGuiID dockIdLeftDown = ImGui::DockBuilderSplitNode(dockIdLeft, ImGuiDir_Down, 0.50f, nullptr, &dockIdLeft);
		ImGuiID dockIdRightDown = ImGui::DockBuilderSplitNode(dockIdRight, ImGuiDir_Down, 0.50f, nullptr, &dockIdRight);
		ImGui::DockBuilderDockWindow(TITLE_PALETTE, dockIdLeft);
		ImGui::DockBuilderDockWindow(TITLE_POSITIONS, dockIdRight);
		ImGui::DockBuilderDockWindow(TITLE_MODIFIERS, dockIdRight);
		ImGui::DockBuilderDockWindow(TITLE_ANIMATION_SETTINGS, dockIdRight);
		ImGui::DockBuilderDockWindow(TITLE_LAYERS, dockIdRightDown);
		ImGui::DockBuilderDockWindow(TITLE_TREES, dockIdRightDown);
		ImGui::DockBuilderDockWindow(TITLE_NOISEPANEL, dockIdRightDown);
		ImGui::DockBuilderDockWindow(TITLE_LSYSTEMPANEL, dockIdRightDown);
		ImGui::DockBuilderDockWindow(TITLE_SCRIPTPANEL, dockIdRightDown);
		ImGui::DockBuilderDockWindow(TITLE_TOOLS, dockIdLeftDown);
		ImGui::DockBuilderDockWindow(_scene->id().c_str(), dockIdMain);
		ImGui::DockBuilderDockWindow(_sceneLeft->id().c_str(), dockIdMain);
		ImGui::DockBuilderDockWindow(_sceneTop->id().c_str(), dockIdMain);
		ImGui::DockBuilderDockWindow(_sceneFront->id().c_str(), dockIdMain);
		ImGui::DockBuilderDockWindow(_sceneAnimation->id().c_str(), dockIdMain);
		ImGuiID dockIdMainDown = ImGui::DockBuilderSplitNode(dockIdMain, ImGuiDir_Down, 0.50f, nullptr, &dockIdMain);
		ImGui::DockBuilderDockWindow(WINDOW_TITLE_SCRIPT_EDITOR, dockIdMainDown);
		ImGui::DockBuilderFinish(dockspaceId);
		init = true;
	}

	updateSettings();
}

bool VoxEditWindow::saveImage(const char *file) {
	return _scene->saveImage(file);
}

bool VoxEditWindow::isSceneHovered() const {
	return _scene->isHovered() || _sceneTop->isHovered() ||
		   _sceneLeft->isHovered() || _sceneFront->isHovered() ||
		   _sceneAnimation->isHovered();
}


bool VoxEditWindow::prefab(const core::String &file) {
	if (file.empty()) {
		_app->openDialog([this](const core::String file) { prefab(file); }, voxelformat::SUPPORTED_VOXEL_FORMATS_LOAD);
		return true;
	}

	return sceneMgr().prefab(file);
}

bool VoxEditWindow::saveScreenshot(const core::String& file) {
	if (file.empty()) {
		_app->saveDialog([this] (const core::String file) {saveScreenshot(file); }, "png");
		return true;
	}
	if (!saveImage(file.c_str())) {
		Log::warn("Failed to save screenshot");
		return false;
	}
	Log::info("Screenshot created at '%s'", file.c_str());
	return true;
}

}
