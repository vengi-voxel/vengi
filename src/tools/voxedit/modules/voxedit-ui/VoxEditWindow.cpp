/**
 * @file
 */

#include "VoxEditWindow.h"
#include "Viewport.h"
#include "command/CommandHandler.h"
#include "core/StringUtil.h"
#include "ui/imgui/IconsForkAwesome.h"
#include "ui/imgui/IconsFontAwesome5.h"
#include "ui/imgui/IMGUIApp.h"
#include "ui/imgui/IMGUI.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/anim/AnimationLuaSaver.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxel/MaterialColor.h"
#include "voxelformat/VolumeFormat.h"
#include "engine-config.h"
#include <glm/gtc/type_ptr.hpp>

#define LAYERPOPUP "##layerpopup"

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
#define POPUP_TITLE_LAYER_SETTINGS "Layer settings##popuptitle"
#define POPUP_TITLE_FAILED_TO_SAVE "Failed to save##popuptitle"
#define POPUP_TITLE_LOAD_PALETTE "Select Palette##popuptitle"
#define POPUP_TITLE_SCENE_SETTINGS "Scene settings##popuptitle"

namespace voxedit {

VoxEditWindow::VoxEditWindow(video::WindowedApp *app) : _app(app) {
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

	switchTreeType(voxelgenerator::TreeType::Dome);
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

void VoxEditWindow::executeCommand(const char *command) {
	_lastExecutedCommand = command;
	command::executeCommands(_lastExecutedCommand);
}

void VoxEditWindow::actionButton(const char *title, const char *command, const char *tooltip, float width) {
	if (ImGui::Button(title, ImVec2(width, 0))) {
		executeCommand(command);
	}
	if (tooltip != nullptr) {
		ImGui::TooltipText("%s", tooltip);
	}
}

bool VoxEditWindow::modifierRadioButton(const char *title, ModifierType type) {
	if (ImGui::RadioButton(title, sceneMgr().modifier().modifierType() == type)) {
		sceneMgr().modifier().setModifierType(type);
		return true;
	}
	return false;
}

bool VoxEditWindow::actionMenuItem(const char *title, const char *command, bool enabled) {
	const core::String& keybinding = _app->getKeyBindingsString(command);
	if (ImGui::MenuItem(title, keybinding.c_str(), false, enabled)) {
		executeCommand(command);
		return true;
	}
	return false;
}

void VoxEditWindow::urlItem(const char *title, const char *url) {
	video::WindowedApp* app = video::WindowedApp::getInstance();
	const core::String& cmd = core::String::format("url %s", url);
	if (actionMenuItem(title, cmd.c_str())) {
		app->minimize();
	}
}

bool VoxEditWindow::mirrorAxisRadioButton(const char *title, math::Axis type) {
	voxedit::ModifierFacade &modifier = sceneMgr().modifier();
	if (ImGui::RadioButton(title, modifier.mirrorAxis() == type)) {
		modifier.setMirrorAxis(type, sceneMgr().referencePosition());
		return true;
	}
	return false;
}

bool VoxEditWindow::init() {
	_showAxisVar = core::Var::get(cfg::VoxEditShowaxis, "1");
	_showGridVar = core::Var::get(cfg::VoxEditShowgrid, "1");
	_modelSpaceVar = core::Var::get(cfg::VoxEditModelSpace, "0");
	_showLockedAxisVar = core::Var::get(cfg::VoxEditShowlockedaxis, "1");
	_showAabbVar = core::Var::get(cfg::VoxEditShowaabb, "0");
	_renderShadowVar = core::Var::get(cfg::VoxEditRendershadow, "1");
	_animationSpeedVar = core::Var::get(cfg::VoxEditAnimationSpeed, "1");
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
			ImGui::InputVarFloat("Animation speed", _animationSpeedVar);
			if (ImGui::Button("Scene settings")) {
				_popupSceneSettings = true;
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

			urlItem(ICON_FK_GITHUB " Bug reports", "https://github.com/mgerhardy/engine");
			urlItem(ICON_FK_TWITTER " Twitter", "https://twitter.com/MartinGerhardy");

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
				ImGui::GetWindowDrawList()->AddRectFilled(v1, v2, core::Color::getRGBA(colors[i]));

				if (!colorHovered && ImGui::IsMouseHoveringRect(v1, v2)) {
					colorHovered = true;
					ImGui::GetWindowDrawList()->AddRect(v1, v2, core::Color::getRGBA(core::Color::Red));
					if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
						sceneMgr().modifier().setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, i));
					}
				} else if (i == voxelColorTraceIndex) {
					ImGui::GetWindowDrawList()->AddRect(v1, v2, core::Color::getRGBA(core::Color::Yellow));
				} else if (i == voxelColorSelectedIndex) {
					ImGui::GetWindowDrawList()->AddRect(v1, v2, core::Color::getRGBA(core::Color::DarkRed));
				} else {
					ImGui::GetWindowDrawList()->AddRect(v1, v2, core::Color::getRGBA(core::Color::Black));
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
		//sceneMgr().modifier().setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, voxelColorIndex));
		actionButton("Import palette", "importpalette");
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

void VoxEditWindow::tools() {
	if (ImGui::Begin(TITLE_TOOLS, nullptr, ImGuiWindowFlags_NoDecoration)) {
		modifierRadioButton(ICON_FA_PEN " Place", ModifierType::Place);
		modifierRadioButton(ICON_FA_EXPAND " Select", ModifierType::Select);
		modifierRadioButton(ICON_FA_ERASER " Delete", ModifierType::Delete);
		modifierRadioButton(ICON_FA_FILTER " Override", ModifierType::Place | ModifierType::Delete);
		modifierRadioButton(ICON_FA_PAINT_BRUSH " Colorize", ModifierType::Update);

		const ShapeType currentSelectedShapeType = sceneMgr().modifier().shapeType();
		if (ImGui::BeginCombo("Shape", ShapeTypeStr[(int)currentSelectedShapeType], ImGuiComboFlags_None)) {
			for (int i = 0; i < (int)ShapeType::Max; ++i) {
				const ShapeType type = (ShapeType)i;
				const bool selected = type == currentSelectedShapeType;
				if (ImGui::Selectable(ShapeTypeStr[i], selected)) {
					sceneMgr().modifier().setShapeType(type);
				}
				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}
	ImGui::End();
}

void VoxEditWindow::addLayerItem(int layerId, const voxedit::Layer &layer) {
	voxedit::LayerManager& layerMgr = voxedit::sceneMgr().layerMgr();
	ImGui::TableNextColumn();

	const core::String &visibleId = core::string::format("##visible-layer-%i", layerId);
	bool visible = layer.visible;
	if (ImGui::Checkbox(visibleId.c_str(), &visible)) {
		layerMgr.hideLayer(layerId, !visible);
	}
	ImGui::TableNextColumn();

	const core::String &lockedId = core::string::format("##locked-layer-%i", layerId);
	bool locked = layer.locked;
	if (ImGui::Checkbox(lockedId.c_str(), &locked)) {
		layerMgr.lockLayer(layerId, locked);
	}
	ImGui::TableNextColumn();

	const core::String &nameId = core::string::format("##name-layer-%i", layerId);
	ImGui::PushID(nameId.c_str());
	if (ImGui::Selectable(layer.name.c_str(), layerId == layerMgr.activeLayer())) {
		layerMgr.setActiveLayer(layerId);
	}
	ImGui::PopID();

	const core::String &contextMenuId = core::string::format("Edit##context-layer-%i", layerId);
	if (ImGui::BeginPopupContextItem(contextMenuId.c_str())) {
		actionMenuItem(ICON_FA_TRASH_ALT " Delete" LAYERPOPUP, "layerdelete");
		actionMenuItem(ICON_FA_EYE_SLASH " Hide others" LAYERPOPUP, "layerhideothers");
		actionMenuItem(ICON_FA_COPY " Duplicate" LAYERPOPUP, "layerduplicate");
		actionMenuItem(ICON_FA_EYE " Show all" LAYERPOPUP, "layershowall");
		actionMenuItem(ICON_FA_EYE_SLASH " Hide all" LAYERPOPUP, "layerhideall");
		actionMenuItem(ICON_FA_CARET_SQUARE_UP " Move up" LAYERPOPUP, "layermoveup");
		actionMenuItem(ICON_FA_CARET_SQUARE_DOWN " Move down" LAYERPOPUP, "layermovedown");
		actionMenuItem(ICON_FA_OBJECT_GROUP " Merge" LAYERPOPUP, "layermerge");
		actionMenuItem(ICON_FA_LOCK " Lock all" LAYERPOPUP, "layerlockall");
		actionMenuItem(ICON_FA_UNLOCK " Unlock all" LAYERPOPUP, "layerunlockall");
		actionMenuItem(ICON_FA_COMPRESS_ARROWS_ALT " Center origin" LAYERPOPUP, "center_origin");
		actionMenuItem(ICON_FA_COMPRESS_ARROWS_ALT " Center reference" LAYERPOPUP, "center_referenceposition");
		actionMenuItem(ICON_FA_SAVE " Save" LAYERPOPUP, "layerssave");
		core::String layerName = layer.name;
		if (ImGui::InputText("Name" LAYERPOPUP, &layerName)) {
			layerMgr.rename(layerId, layerName);
		}
		ImGui::EndPopup();
	}

	ImGui::TableNextColumn();

	const core::String &deleteId = core::string::format(ICON_FA_TRASH_ALT"##delete-layer-%i", layerId);
	if (ImGui::Button(deleteId.c_str())) {
		layerMgr.deleteLayer(layerId);
	}
}

void VoxEditWindow::layers() {
	voxedit::SceneManager& sceneMgr = voxedit::sceneMgr();
	voxedit::LayerManager& layerMgr = sceneMgr.layerMgr();
	if (ImGui::Begin(TITLE_LAYERS, nullptr, ImGuiWindowFlags_NoDecoration)) {
		ImGui::BeginChild("##layertable", ImVec2(0.0f, 400.0f), true, ImGuiWindowFlags_HorizontalScrollbar);
		static const uint32_t TableFlags =
			ImGuiTableFlags_Reorderable | ImGuiTableFlags_Resizable | ImGuiTableFlags_Hideable |
			ImGuiTableFlags_BordersInner | ImGuiTableFlags_RowBg;
		if (ImGui::BeginTable("##nodelist", 4, TableFlags)) {
			ImGui::TableSetupColumn(ICON_FA_EYE"##visiblelayer", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn(ICON_FA_LOCK"##lockedlayer", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("Name##layer", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("##deletelayer", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableHeadersRow();
			const voxedit::Layers& layers = layerMgr.layers();
			const int layerCnt = layers.size();
			for (int l = 0; l < layerCnt; ++l) {
				if (!layers[l].valid) {
					continue;
				}
				addLayerItem(l, layers[l]);
			}
			ImGui::EndTable();
		}
		ImGui::EndChild();
		if (ImGui::Button(ICON_FA_PLUS_SQUARE"##newlayer")) {
			const int layerId = layerMgr.activeLayer();
			const voxel::RawVolume* v = sceneMgr.volume(layerId);
			const voxel::Region& region = v->region();
			_layerSettings.position = region.getLowerCorner();
			_layerSettings.size = region.getDimensionsInVoxels();
			if (_layerSettings.name.empty()) {
				_layerSettings.name = layerMgr.layer(layerId).name;
			}
			ImGui::OpenPopup(POPUP_TITLE_LAYER_SETTINGS);
		}
		if (ImGui::BeginPopupModal(POPUP_TITLE_LAYER_SETTINGS, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::InputText("Name", &_layerSettings.name);
			ImGui::InputVec3("Position", _layerSettings.position);
			ImGui::InputVec3("Size", _layerSettings.size);
			if (ImGui::Button(ICON_FA_CHECK " OK##layersettings")) {
				ImGui::CloseCurrentPopup();
				voxel::RawVolume* v = new voxel::RawVolume(_layerSettings.region());
				voxedit::LayerManager& layerMgr = voxedit::sceneMgr().layerMgr();
				const int layerId = layerMgr.addLayer(_layerSettings.name.c_str(), true, v, v->region().getCenter());
				layerMgr.setActiveLayer(layerId);
			}
			if (ImGui::Button(ICON_FA_TIMES " Cancel##layersettings")) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::SetItemDefaultFocus();
			ImGui::EndPopup();
		}
		ImGui::TooltipText("Add a new layer");

		ImGui::SameLine();
		if (ImGui::DisabledButton(ICON_FA_PLAY"##animatelayers", !sceneMgr.animateActive() || layerMgr.validLayers() <= 1)) {
			core::String cmd = core::string::format("animate %f", _animationSpeedVar->floatVal());
			executeCommand(cmd.c_str());
		}
		ImGui::TooltipText("Animate the layers");
		ImGui::SameLine();
		actionButton(ICON_FA_CARET_SQUARE_UP, "layermoveup");
		ImGui::TooltipText("Move the layer one level up");
		ImGui::SameLine();
		actionButton(ICON_FA_CARET_SQUARE_DOWN, "layermovedown");
		ImGui::TooltipText("Move the layer one level down");
	}
	ImGui::End();
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
		} else if (!_lastExecutedCommand.empty()) {
			const video::WindowedApp* app = video::WindowedApp::getInstance();
			core::String statusText;
			const core::String& keybindingStr = app->getKeyBindingsString(_lastExecutedCommand.c_str());
			if (keybindingStr.empty()) {
				statusText = core::string::format("Command: %s", _lastExecutedCommand.c_str());
			} else {
				statusText = core::string::format("Command: %s (%s)", _lastExecutedCommand.c_str(), keybindingStr.c_str());
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
	tools();
}

void VoxEditWindow::mainWidget() {
	_scene->update();
	_sceneTop->update();
	_sceneLeft->update();
	_sceneFront->update();
	_sceneAnimation->update();
	if (ImGui::Begin(TITLE_ANIMATION_SETTINGS, nullptr, 0)) {
		if (sceneMgr().editMode() == EditMode::Animation) {
			animation::SkeletonAttribute* skeletonAttributes = sceneMgr().skeletonAttributes();
			for (const animation::SkeletonAttributeMeta* metaIter = skeletonAttributes->metaArray(); metaIter->name; ++metaIter) {
				const animation::SkeletonAttributeMeta& meta = *metaIter;
				float *v = (float*)(((uint8_t*)skeletonAttributes) + meta.offset);
				ImGui::InputFloat(meta.name, v);
			}
		} else {
			ImGui::TextDisabled("No animation loaded");
			ImGui::NewLine();
			if (ImGui::Button("Load Animation")) {
				executeCommand("animation_load");
			}
		}
	}
	ImGui::End();
}

void VoxEditWindow::rightWidget() {
	positionsPanel();
	modifierPanel();
	treePanel();
	scriptPanel();
	lsystemPanel();
	noisePanel();
	layers();
}

void VoxEditWindow::positionsPanel() {
	if (ImGui::Begin(TITLE_POSITIONS, nullptr, ImGuiWindowFlags_NoDecoration)) {
		if (ImGui::CollapsingHeader(ICON_FA_ARROWS_ALT " Translate", ImGuiTreeNodeFlags_DefaultOpen)) {
			static glm::vec3 translate {0.0f};
			ImGui::InputFloat("X##translate", &translate.x);
			ImGui::InputFloat("Y##translate", &translate.y);
			ImGui::InputFloat("Z##translate", &translate.z);
			if (ImGui::Button(ICON_FA_BORDER_STYLE " Volumes")) {
				sceneMgr().shift(translate.x, translate.y, translate.z);
			}
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_CUBES " Voxels")) {
				sceneMgr().move(translate.x, translate.y, translate.z);
			}
		}

		ImGui::NewLine();

		if (ImGui::CollapsingHeader(ICON_FA_CUBE " Cursor", ImGuiTreeNodeFlags_DefaultOpen)) {
			glm::ivec3 cursorPosition = sceneMgr().modifier().cursorPosition();
			uint32_t lockedAxis = (uint32_t)sceneMgr().lockedAxis();
			if (ImGui::CheckboxFlags("X##cursorlock", &lockedAxis, (uint32_t)math::Axis::X)) {
				executeCommand("lockx");
			}
			ImGui::TooltipText("Lock the x axis");
			ImGui::SameLine();

			if (ImGui::InputInt("X##cursor", &cursorPosition.x)) {
				sceneMgr().setCursorPosition(cursorPosition, true);
			}
			if (ImGui::CheckboxFlags("Y##cursorlock", &lockedAxis, (uint32_t)math::Axis::Y)) {
				executeCommand("locky");
			}
			ImGui::TooltipText("Lock the y axis");
			ImGui::SameLine();

			if (ImGui::InputInt("Y##cursor", &cursorPosition.y)) {
				sceneMgr().setCursorPosition(cursorPosition, true);
			}
			if (ImGui::CheckboxFlags("Z##cursorlock", &lockedAxis, (uint32_t)math::Axis::Z)) {
				executeCommand("lockz");
			}
			ImGui::TooltipText("Lock the z axis");
			ImGui::SameLine();

			if (ImGui::InputInt("Z##cursor", &cursorPosition.z)) {
				sceneMgr().setCursorPosition(cursorPosition, true);
			}
		}
	}
	ImGui::End();
}

void VoxEditWindow::modifierPanel() {
	if (ImGui::Begin(TITLE_MODIFIERS, nullptr, ImGuiWindowFlags_NoDecoration)) {
		const float windowWidth = ImGui::GetWindowWidth();
		actionButton(ICON_FA_CROP " Crop layer", "crop", "Crop the current layer to the voxel boundaries", windowWidth);
		actionButton(ICON_FA_EXPAND_ARROWS_ALT " Extend all layers", "resize", nullptr, windowWidth);
		actionButton(ICON_FA_OBJECT_UNGROUP " Layer from color", "colortolayer", "Create a new layer from the current selected color", windowWidth);
		actionButton(ICON_FA_COMPRESS_ALT " Scale", "scale", "Scale the current layer down", windowWidth);

		ImGui::NewLine();

		if (ImGui::CollapsingHeader("Rotate on axis", ImGuiTreeNodeFlags_DefaultOpen)) {
			actionButton(ICON_FK_REPEAT " X", "rotate 90 0 0", nullptr, windowWidth / 3.0f);
			ImGui::TooltipText("Rotate by 90 degree on the x axis");
			ImGui::SameLine();
			actionButton(ICON_FK_REPEAT " Y", "rotate 0 90 0", nullptr, windowWidth / 3.0f);
			ImGui::TooltipText("Rotate by 90 degree on the y axis");
			ImGui::SameLine();
			actionButton(ICON_FK_REPEAT " Z", "rotate 0 0 90", nullptr, windowWidth / 3.0f);
			ImGui::TooltipText("Rotate by 90 degree on the z axis");
		}

		ImGui::NewLine();

		if (ImGui::CollapsingHeader("Flip on axis", ImGuiTreeNodeFlags_DefaultOpen)) {
			actionButton("X", "flip x", nullptr, windowWidth / 3.0f);
			ImGui::SameLine();
			actionButton("Y", "flip y", nullptr, windowWidth / 3.0f);
			ImGui::SameLine();
			actionButton("Z", "flip z", nullptr, windowWidth / 3.0f);
		}

		ImGui::NewLine();

		if (ImGui::CollapsingHeader("Mirror on axis", ImGuiTreeNodeFlags_DefaultOpen)) {
			mirrorAxisRadioButton("none", math::Axis::None);
			mirrorAxisRadioButton("x", math::Axis::X);
			mirrorAxisRadioButton("y", math::Axis::Y);
			mirrorAxisRadioButton("z", math::Axis::Z);
		}
	}
	ImGui::End();
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
		ImGui::TextUnformatted(ICON_FA_QUESTION);
		ImGui::Spacing();
		ImGui::SameLine();
		ImGui::TextUnformatted("There are unsaved modifications.\nDo you wish to discard them?");
		ImGui::Separator();
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
		ImGui::TextUnformatted(ICON_FA_EXCLAMATION_TRIANGLE);
		ImGui::SameLine();
		ImGui::TextUnformatted("Failed to save the model!");
		ImGui::Separator();
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

		ImGui::DockBuilderFinish(dockspaceId);
		init = true;
	}

	updateSettings();
}

void VoxEditWindow::switchTreeType(voxelgenerator::TreeType treeType) {
	switch (treeType) {
		case voxelgenerator::TreeType::Dome:
			_treeGeneratorContext.dome = voxelgenerator::TreeDome();
			break;
		case voxelgenerator::TreeType::DomeHangingLeaves:
			_treeGeneratorContext.domehanging = voxelgenerator::TreeDomeHanging();
			break;
		case voxelgenerator::TreeType::Cone:
			_treeGeneratorContext.cone = voxelgenerator::TreeCone();
			break;
		case voxelgenerator::TreeType::Ellipsis:
			_treeGeneratorContext.ellipsis = voxelgenerator::TreeEllipsis();
			break;
		case voxelgenerator::TreeType::BranchesEllipsis:
			_treeGeneratorContext.branchellipsis = voxelgenerator::TreeBranchEllipsis();
			break;
		case voxelgenerator::TreeType::Cube:
		case voxelgenerator::TreeType::CubeSideCubes:
			_treeGeneratorContext.cube = voxelgenerator::TreeCube();
			break;
		case voxelgenerator::TreeType::Pine:
			_treeGeneratorContext.pine = voxelgenerator::TreePine();
			break;
		case voxelgenerator::TreeType::Fir:
			_treeGeneratorContext.fir = voxelgenerator::TreeFir();
			break;
		case voxelgenerator::TreeType::Palm:
			_treeGeneratorContext.palm = voxelgenerator::TreePalm();
			break;
		case voxelgenerator::TreeType::SpaceColonization:
			_treeGeneratorContext.spacecolonization = voxelgenerator::TreeSpaceColonization();
			break;
		case voxelgenerator::TreeType::Max:
		default:
			break;
	}
	_treeGeneratorContext.cfg.type = treeType;
}

void VoxEditWindow::treePanel() {
	static const struct {
		const char *name;
		voxelgenerator::TreeType type;
	} treeTypes[] = {
		{"Pine",				voxelgenerator::TreeType::Pine},
		{"Dome",				voxelgenerator::TreeType::Dome},
		{"Dome Hanging",		voxelgenerator::TreeType::DomeHangingLeaves},
		{"Cone",				voxelgenerator::TreeType::Cone},
		{"Fir",					voxelgenerator::TreeType::Fir},
		{"Ellipsis2",			voxelgenerator::TreeType::BranchesEllipsis},
		{"Ellipsis",			voxelgenerator::TreeType::Ellipsis},
		{"Cube",				voxelgenerator::TreeType::Cube},
		{"Cube Sides",			voxelgenerator::TreeType::CubeSideCubes},
		{"Palm",				voxelgenerator::TreeType::Palm},
		{"SpaceColonization",	voxelgenerator::TreeType::SpaceColonization}
	};
	static_assert(lengthof(treeTypes) == (int)voxelgenerator::TreeType::Max, "Missing support for tree types in the ui");

	if (ImGui::Begin(TITLE_TREES)) {
		if (ImGui::BeginCombo(ICON_FA_TREE " Type", treeTypes[core::enumVal(_treeGeneratorContext.cfg.type)].name, 0)) {
			for (int i = 0; i < lengthof(treeTypes); ++i) {
				if (ImGui::Selectable(treeTypes[i].name, i == core::enumVal(_treeGeneratorContext.cfg.type))) {
					switchTreeType((voxelgenerator::TreeType)i);
				}
			}
			ImGui::EndCombo();
		}

		ImGui::InputInt("Seed", (int*)&_treeGeneratorContext.cfg.seed);
		ImGui::InputInt("Trunk strength", &_treeGeneratorContext.cfg.trunkStrength);
		ImGui::InputInt("Trunk height", &_treeGeneratorContext.cfg.trunkHeight);
		ImGui::InputInt("Leaves width", &_treeGeneratorContext.cfg.leavesWidth);
		ImGui::InputInt("Leaves height", &_treeGeneratorContext.cfg.leavesHeight);
		ImGui::InputInt("Leaves depth", &_treeGeneratorContext.cfg.leavesDepth);
		switch (_treeGeneratorContext.cfg.type) {
		case voxelgenerator::TreeType::BranchesEllipsis:
			ImGui::InputInt("Branch length", &_treeGeneratorContext.branchellipsis.branchLength);
			ImGui::InputInt("Branch height", &_treeGeneratorContext.branchellipsis.branchHeight);
			break;
		case voxelgenerator::TreeType::Palm:
			ImGui::InputInt("Branch size", &_treeGeneratorContext.palm.branchSize);
			ImGui::InputInt("Trunk width", &_treeGeneratorContext.palm.trunkWidth);
			ImGui::InputInt("Trunk depth", &_treeGeneratorContext.palm.trunkDepth);
			ImGui::InputFloat("Branch reduction", &_treeGeneratorContext.palm.branchFactor);
			ImGui::InputFloat("Trunk reduction", &_treeGeneratorContext.palm.trunkFactor);
			ImGui::InputInt("Leaves", &_treeGeneratorContext.palm.branches);
			ImGui::InputInt("Bezier leaf", &_treeGeneratorContext.palm.branchControlOffset);
			ImGui::InputInt("Bezier trunk", &_treeGeneratorContext.palm.trunkControlOffset);
			ImGui::InputInt("Leaves h-offset", &_treeGeneratorContext.palm.randomLeavesHeightOffset);
			break;
		case voxelgenerator::TreeType::Fir:
			ImGui::InputInt("Branches", &_treeGeneratorContext.fir.branches);
			ImGui::InputFloat("W", &_treeGeneratorContext.fir.w);
			ImGui::InputInt("Amount", &_treeGeneratorContext.fir.amount);
			ImGui::InputFloat("Branch position factor", &_treeGeneratorContext.fir.branchPositionFactor);
			ImGui::InputInt("Branch strength", &_treeGeneratorContext.fir.branchStrength);
			ImGui::InputInt("Branch downward offset", &_treeGeneratorContext.fir.branchDownwardOffset);
			break;
		case voxelgenerator::TreeType::Pine:
			ImGui::InputInt("Start width", &_treeGeneratorContext.pine.startWidth);
			ImGui::InputInt("Start depth", &_treeGeneratorContext.pine.startDepth);
			ImGui::InputInt("Leaf height", &_treeGeneratorContext.pine.singleLeafHeight);
			ImGui::InputInt("Step delta", &_treeGeneratorContext.pine.singleStepDelta);
			break;
		case voxelgenerator::TreeType::DomeHangingLeaves:
			ImGui::InputInt("Branches", &_treeGeneratorContext.domehanging.branches);
			ImGui::InputInt("Leaves min length", &_treeGeneratorContext.domehanging.hangingLeavesLengthMin);
			ImGui::InputInt("Leaves max length", &_treeGeneratorContext.domehanging.hangingLeavesLengthMax);
			ImGui::InputInt("Leaves thickness", &_treeGeneratorContext.domehanging.hangingLeavesThickness);
			break;
		case voxelgenerator::TreeType::SpaceColonization:
			ImGui::InputInt("Branch size", &_treeGeneratorContext.spacecolonization.branchSize);
			ImGui::InputFloat("Trunk reduction", &_treeGeneratorContext.spacecolonization.trunkFactor);
			break;
		default:
			break;
		}
		if (ImGui::Button(ICON_FA_CHECK " OK##treegenerate")) {
			_treeGeneratorContext.cfg.pos = sceneMgr().referencePosition();
			sceneMgr().createTree(_treeGeneratorContext);
		}
	}
	ImGui::End();
}

void VoxEditWindow::lsystemPanel() {
	if (ImGui::Begin(TITLE_LSYSTEMPANEL)) {
		ImGui::InputText("Axiom", &_lsystemData.axiom);
		ImGui::InputTextMultiline("Rules", &_lsystemData.rulesStr);
		ImGui::InputFloat("angle", &_lsystemData.angle);
		ImGui::InputFloat("length", &_lsystemData.length);
		ImGui::InputFloat("width", &_lsystemData.width);
		ImGui::InputFloat("widthIncrement", &_lsystemData.widthIncrement);
		ImGui::InputInt("iterations", &_lsystemData.iterations);
		ImGui::InputFloat("leavesRadius", &_lsystemData.leavesRadius);

		if (ImGui::Button(ICON_FA_CHECK " OK##lsystem")) {
			core::DynamicArray<voxelgenerator::lsystem::Rule> rules;
			if (voxelgenerator::lsystem::parseRules(_lsystemData.rulesStr.c_str(), rules)) {
				sceneMgr().lsystem(_lsystemData.axiom.c_str(), rules, _lsystemData.angle,
					_lsystemData.length, _lsystemData.width, _lsystemData.widthIncrement, _lsystemData.iterations, _lsystemData.leavesRadius);
			}
		}
	}
	ImGui::End();
}

void VoxEditWindow::noisePanel() {
	if (ImGui::Begin(TITLE_NOISEPANEL)) {
		ImGui::InputInt("Octaves", &_noiseData.octaves);
		ImGui::InputFloat("Frequency", &_noiseData.frequency);
		ImGui::InputFloat("Lacunarity", &_noiseData.lacunarity);
		ImGui::InputFloat("Gain", &_noiseData.gain);

		if (ImGui::Button(ICON_FA_CHECK " OK##noise")) {
			sceneMgr().noise(_noiseData.octaves, _noiseData.lacunarity, _noiseData.frequency, _noiseData.gain, voxelgenerator::noise::NoiseType::ridgedMF);
		}
	}
	ImGui::End();
}

void VoxEditWindow::scriptPanel() {
	if (ImGui::Begin(TITLE_SCRIPTPANEL)) {
		if (_scripts.empty()) {
			_scripts = sceneMgr().luaGenerator().listScripts();
		}

		if (ImGui::ComboStl("Script", &_currentScript, _scripts)) {
			const core::String& scriptName = _scripts[_currentScript];
			_activeScript = sceneMgr().luaGenerator().load(scriptName);
			sceneMgr().luaGenerator().argumentInfo(_activeScript, _scriptParameterDescription);
			const int parameterCount = _scriptParameterDescription.size();
			_scriptParameters.clear();
			_scriptParameters.resize(parameterCount);
			for (int i = 0; i < parameterCount; ++i) {
				const voxelgenerator::LUAParameterDescription &p = _scriptParameterDescription[i];
				_scriptParameters[i] = p.defaultValue;
			}
		}

		const int n = _scriptParameterDescription.size();
		for (int i = 0; i < n; ++i) {
			const voxelgenerator::LUAParameterDescription &p = _scriptParameterDescription[i];
			switch (p.type) {
			case voxelgenerator::LUAParameterType::ColorIndex: {
				// TODO: select palette color
				core::String &str = _scriptParameters[i];
				int val = core::string::toInt(str);
				if (ImGui::InputInt(p.name.c_str(), &val)) {
					str = core::string::toString(val);
				}
				break;
			}
			case voxelgenerator::LUAParameterType::Integer: {
				core::String &str = _scriptParameters[i];
				int val = core::string::toInt(str);
				if (ImGui::InputInt(p.name.c_str(), &val)) {
					str = core::string::toString(val);
				}
				break;
			}
			case voxelgenerator::LUAParameterType::Float: {
				core::String &str = _scriptParameters[i];
				float val = core::string::toFloat(str);
				if (ImGui::InputFloat(p.name.c_str(), &val)) {
					str = core::string::toString(val);
				}
				break;
			}
			case voxelgenerator::LUAParameterType::String: {
				core::String &str = _scriptParameters[i];
				ImGui::InputText(p.name.c_str(), &str);
				break;
			}
			case voxelgenerator::LUAParameterType::Boolean: {
				core::String &str = _scriptParameters[i];
				bool checked = str == "1";
				if (ImGui::Checkbox(p.name.c_str(), &checked)) {
					str = checked ? "1" : "0";
				}
				break;
			}
			case voxelgenerator::LUAParameterType::Max:
				return;
			}
			ImGui::TooltipText("%s", p.description.c_str());
		}

		if (ImGui::Button("Execute##scriptpanel")) {
			sceneMgr().runScript(_activeScript, _scriptParameters);
		}
	}
	ImGui::End();
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
