/**
 * @file
 */

#include "VoxEditWindow.h"
#include "IMGUIApp.h"
#include "Viewport.h"
#include "command/CommandHandler.h"
#include "core/StringUtil.h"
#include "dearimgui/imgui.h"
#include "ui/imgui/IconsFontAwesome5.h"
#include "ui/imgui/IMGUI.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/anim/AnimationLuaSaver.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxel/MaterialColor.h"
#include "voxelformat/VolumeFormat.h"

namespace voxedit {

VoxEditWindow::VoxEditWindow(video::WindowedApp *app) : Super(app) {
	_scene = new Viewport(app, "free");
	_scene->init();

	_sceneTop = new Viewport(app, "top");
	_sceneTop->init();
	_sceneTop->setMode(voxedit::ViewportController::SceneCameraMode::Top);

	_sceneLeft = new Viewport(app, "left");
	_sceneLeft->init();
	_sceneLeft->setMode(voxedit::ViewportController::SceneCameraMode::Left);

	_sceneFront = new Viewport(app, "front");
	_sceneFront->init();
	_sceneFront->setMode(voxedit::ViewportController::SceneCameraMode::Front);

	_sceneAnimation = new Viewport(app, "animation");
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

bool VoxEditWindow::actionButton(const char *title, const char *command) {
	if (ImGui::Button(title)) {
		executeCommand(command);
		return true;
	}
	return false;
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

bool VoxEditWindow::mirrorAxisRadioButton(const char *title, math::Axis type) {
	voxedit::ModifierFacade &modifier = sceneMgr().modifier();
	if (ImGui::RadioButton(title, modifier.mirrorAxis() == type)) {
		modifier.setMirrorAxis(type, sceneMgr().referencePosition());
		return true;
	}
	return false;
}

void VoxEditWindow::afterLoad(const core::String &file) {
	_lastOpenedFile->setVal(file);
	resetCamera();
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
		_app->saveDialog([this](const core::String uifile) { save(uifile); },
						 voxelformat::SUPPORTED_VOXEL_FORMATS_SAVE);
		return true;
	}
	if (!sceneMgr().save(file)) {
		Log::warn("Failed to save the model");
		ImGui::OpenPopup("Failed to save");
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
	ImGui::OpenPopup("Unsaved Modifications");
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
	if (!force && sceneMgr().dirty()) {
		_loadFile.clear();
		ImGui::OpenPopup("Unsaved Modifications");
	} else {
		// TODO: layer settings edit popup
		const voxel::Region &region = _layerSettings.region();
		if (region.isValid()) {
			if (!voxedit::sceneMgr().newScene(true, _layerSettings.name, region)) {
				return false;
			}
			afterLoad("");
		} else {
			ImGui::OpenPopup("Invalid dimensions");
			_layerSettings.reset();
		}
		return true;
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
			actionMenuItem("Load", "load");
			actionMenuItem(ICON_FA_SAVE" Save", "save");
			ImGui::Separator();
			actionMenuItem("Load Animation", "animation_load");
			actionMenuItem(ICON_FA_SAVE" Save Animation", "animation_save");
			ImGui::Separator();
			actionMenuItem("Prefab", "prefab");
			ImGui::Separator();
			actionMenuItem(ICON_FA_IMAGE" Heightmap", "importheightmap");
			actionMenuItem(ICON_FA_IMAGE" Image as Plane", "importplane");
			ImGui::Separator();
			if (ImGui::MenuItem("Quit")) {
				_app->requestQuit();
			}
			ImGui::EndMenu();
		}
		actionMenuItem(ICON_FA_UNDO" Undo", "undo", sceneMgr().mementoHandler().canUndo());
		actionMenuItem(ICON_FA_REDO" Redo", "redo", sceneMgr().mementoHandler().canRedo());
		if (ImGui::BeginMenu(ICON_FA_COG"Options")) {
			ImGui::CheckboxVar(ICON_FA_BORDER_ALL"Grid", _showGridVar);
			ImGui::CheckboxVar("Show axis", _showAxisVar);
			ImGui::CheckboxVar("Model space", _modelSpaceVar);
			ImGui::CheckboxVar("Show locked axis", _showLockedAxisVar);
			ImGui::CheckboxVar(ICON_FA_DICE_SIX"Bounding box", _showAabbVar);
			ImGui::CheckboxVar("Shadow", _renderShadowVar);
			ImGui::CheckboxVar("Outlines", "r_renderoutline");
			ImGui::InputVarFloat("Animation speed", _animationSpeedVar);
			ImGui::EndMenu();
			// TBButton: gravity: left, @include: definitions>menubutton, text: Settings, id: scene_settings_open
		}
		if (ImGui::BeginMenu(ICON_FA_EYE"View")) {
			actionMenuItem("Reset camera", "resetcamera");
			actionMenuItem("Quad view", "toggleviewport");
			actionMenuItem("Animation view", "toggleanimation");
			actionMenuItem("Scene view", "togglescene");
			ImGui::EndMenu();
		}
		if (ImGui::MenuItem(ICON_FA_TREE"Trees")) {
			// TBButton: gravity: left, @include: definitions>menubutton, text: Trees, id: show_tree_panel
		}
		if (ImGui::MenuItem(ICON_FA_CODE"Scripts")) {
			// TBButton: gravity: left, @include: definitions>menubutton, text: Scripts, id: show_script_panel
		}
		if (ImGui::MenuItem(ICON_FA_RANDOM"Noise")) {
			// TBButton: gravity: left, @include: definitions>menubutton, text: Noise, id: show_noise_panel
		}
		if (ImGui::MenuItem(ICON_FA_LEAF"L-System")) {
			// TBButton: gravity: left, @include: definitions>menubutton, text: L-System, id: show_lsystem_panel
		}
		ImGui::EndMenuBar();
	}
}

void VoxEditWindow::palette() {
	const voxel::MaterialColorArray &colors = voxel::getMaterialColors();
	const float height = ImGui::GetContentRegionAvail().y;
	const float width = ImGui::Size(120.0f);
	const ImVec2 size(width, height);
	ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);
	int voxelColorIndex = sceneMgr().hitCursorVoxel().getColor();
	if (ImGui::Begin("Palette", nullptr, ImGuiWindowFlags_NoDecoration)) {
		const ImVec2 &pos = ImGui::GetWindowPos();
		const float size = ImGui::Size(20);
		const int amountX = (int)(ImGui::GetWindowWidth() / size);
		const int amountY = (int)(ImGui::GetWindowHeight() / size);
		const int max = colors.size();
		int i = 0;
		float usedHeight = 0;
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

				if (ImGui::IsMouseHoveringRect(v1, v2)) {
					ImGui::GetWindowDrawList()->AddRect(v1, v2, core::Color::getRGBA(core::Color::Red));
					if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
						sceneMgr().modifier().setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, i));
					}
				} else if (i == voxelColorIndex) {
					ImGui::GetWindowDrawList()->AddRect(v1, v2, core::Color::getRGBA(core::Color::Yellow));
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

		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + usedHeight);
		if (ImGui::InputInt("Color index", &voxelColorIndex)) {
			sceneMgr().modifier().setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, voxelColorIndex));
		}
		actionButton("Import palette", "importpalette");
		ImGui::SameLine();
		if (ImGui::Button("Load palette")) {
#if 0
			TBButton: @include: definitions>menubutton, text: Load, id: loadpalette
#endif
		}
	}
	ImGui::End();
}

void VoxEditWindow::tools() {
	if (ImGui::Begin("Tools", nullptr, ImGuiWindowFlags_NoDecoration)) {
		modifierRadioButton("Place", ModifierType::Place);
		modifierRadioButton("Select", ModifierType::Select);
		modifierRadioButton("Delete", ModifierType::Delete);
		modifierRadioButton("Override", ModifierType::Place | ModifierType::Delete);
		modifierRadioButton("Colorize", ModifierType::Update);

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

	if (ImGui::BeginPopupContextItem("Edit")) {
		actionMenuItem(ICON_FA_ERASER"Delete", "layerdelete");
		actionMenuItem("Hide others", "layerhideothers");
		actionMenuItem(ICON_FA_COPY"Duplicate", "layerduplicate");
		actionMenuItem(ICON_FA_EYE"Show all", "layershowall");
		actionMenuItem("Hide all", "layerhideall");
		actionMenuItem(ICON_FA_CARET_SQUARE_UP"Move up", "layermoveup");
		actionMenuItem(ICON_FA_CARET_SQUARE_DOWN"Move down", "layermovedown");
		actionMenuItem("Merge", "layermerge");
		actionMenuItem(ICON_FA_LOCK"Lock all", "layerlockall");
		actionMenuItem(ICON_FA_UNLOCK"Unlock all", "layerunlockall");
		actionMenuItem("Center origin", "center_origin");
		actionMenuItem("Center reference", "center_referenceposition");
		actionMenuItem(ICON_FA_SAVE"Save", "layerssave");
		core::String layerName = layer.name;
		if (ImGui::InputText("Name", &layerName)) {
			layerMgr.rename(layerId, layerName);
		}
		ImGui::EndPopup();
	}

	ImGui::TableNextColumn();

	const core::String &deleteId = core::string::format(ICON_FA_ERASER"##delete-layer-%i", layerId);
	if (ImGui::Button(deleteId.c_str())) {
		layerMgr.deleteLayer(layerId);
	}
}

void VoxEditWindow::layers() {
	voxedit::SceneManager& sceneMgr = voxedit::sceneMgr();
	voxedit::LayerManager& layerMgr = sceneMgr.layerMgr();
	if (ImGui::Begin("Layers", nullptr, ImGuiWindowFlags_NoDecoration)) {
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
			// TODO: layer settings
			if (region.isValid()) {
				voxel::RawVolume* v = new voxel::RawVolume(_layerSettings.region());
				const int layerId = layerMgr.addLayer(_layerSettings.name.c_str(), true, v, v->region().getCenter());
				layerMgr.setActiveLayer(layerId);
			} else {
				_layerSettings.reset();
			}
		}

		ImGui::SameLine();
		if (ImGui::ToggleButton(ICON_FA_PLAY"##animatelayers", sceneMgr.animateActive())) {
			core::String cmd = core::string::format("animate %f", _animationSpeedVar->floatVal());
			executeCommand(cmd.c_str());
		}
		ImGui::SameLine();
		actionButton(ICON_FA_CARET_SQUARE_UP, "layermoveup");
		ImGui::SameLine();
		actionButton(ICON_FA_CARET_SQUARE_DOWN, "layermovedown");
	}
	ImGui::End();
}

void VoxEditWindow::statusBar() {
	ImGuiViewport *viewport = ImGui::GetMainViewport();
	const ImVec2 &size = viewport->GetWorkSize();
	const float statusBarHeight = ImGui::Size((float)((ui::imgui::IMGUIApp*)_app)->fontSize() + 16.0f);
	ImGui::SetNextWindowSize(ImVec2(size.x, statusBarHeight));
	ImVec2 statusBarPos = viewport->GetWorkPos();
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
}

void VoxEditWindow::rightWidget() {
	if (ImGui::Begin("Positions", nullptr, ImGuiWindowFlags_NoDecoration)) {
		if (ImGui::CollapsingHeader("Translate", ImGuiTreeNodeFlags_DefaultOpen)) {
			static glm::vec3 translate {0.0f};
			ImGui::InputFloat("X##translate", &translate.x);
			ImGui::InputFloat("Y##translate", &translate.y);
			ImGui::InputFloat("Z##translate", &translate.z);
			if (ImGui::Button("Volumes")) {
				sceneMgr().shift(translate.x, translate.y, translate.z);
			}
			ImGui::SameLine();
			if (ImGui::Button("Voxels")) {
				sceneMgr().move(translate.x, translate.y, translate.z);
			}
		}

		if (ImGui::CollapsingHeader("Cursor", ImGuiTreeNodeFlags_DefaultOpen)) {
			glm::ivec3 cursorPosition = sceneMgr().modifier().cursorPosition();
			uint32_t lockedAxis = (uint32_t)sceneMgr().lockedAxis();
			if (ImGui::CheckboxFlags("X##cursorlock", &lockedAxis, (uint32_t)math::Axis::X)) {
				executeCommand("lockx");
			}
			ImGui::SameLine();
			if (ImGui::InputInt("X##cursor", &cursorPosition.x)) {
				sceneMgr().setCursorPosition(cursorPosition, true);
			}
			if (ImGui::CheckboxFlags("Y##cursorlock", &lockedAxis, (uint32_t)math::Axis::Y)) {
				executeCommand("locky");
			}
			ImGui::SameLine();
			if (ImGui::InputInt("Y##cursor", &cursorPosition.y)) {
				sceneMgr().setCursorPosition(cursorPosition, true);
			}
			if (ImGui::CheckboxFlags("Z##cursorlock", &lockedAxis, (uint32_t)math::Axis::Z)) {
				executeCommand("lockz");
			}
			ImGui::SameLine();
			if (ImGui::InputInt("Z##cursor", &cursorPosition.z)) {
				sceneMgr().setCursorPosition(cursorPosition, true);
			}
		}
	}
	ImGui::End();

	if (ImGui::Begin("Modifiers", nullptr, ImGuiWindowFlags_NoDecoration)) {
		actionButton(ICON_FA_CROP"Crop", "crop");
		actionButton(ICON_FA_EXPAND"Extend", "resize");
		actionButton("Layer from color", "colortolayer");
		actionButton("Scale", "scale");

		ImGui::Separator();

		if (ImGui::CollapsingHeader("Rotate on axis", ImGuiTreeNodeFlags_DefaultOpen)) {
			actionButton("X", "rotate 90 0 0");
			ImGui::SameLine();
			actionButton("Y", "rotate 0 90 0");
			ImGui::SameLine();
			actionButton("Z", "rotate 0 0 90");
		}

		ImGui::Separator();
		if (ImGui::CollapsingHeader("Flip on axis", ImGuiTreeNodeFlags_DefaultOpen)) {
			actionButton("X", "flip x");
			ImGui::SameLine();
			actionButton("Y", "flip y");
			ImGui::SameLine();
			actionButton("Z", "flip z");
		}

		ImGui::Separator();
		if (ImGui::CollapsingHeader("Mirror on axis", ImGuiTreeNodeFlags_DefaultOpen)) {
			mirrorAxisRadioButton("none", math::Axis::None);
			mirrorAxisRadioButton("x", math::Axis::X);
			mirrorAxisRadioButton("y", math::Axis::Y);
			mirrorAxisRadioButton("z", math::Axis::Z);
		}
	}
	ImGui::End();

	layers();
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
	if (ImGui::BeginPopupModal("Unsaved Modifications", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::TextUnformatted(ICON_FA_QUESTION"There are unsaved modifications.\nDo you wish to discard them?");
		ImGui::Separator();
		if (ImGui::Button("Yes")) {
			ImGui::CloseCurrentPopup();
			if (!_loadFile.empty()) {
				sceneMgr().load(_loadFile);
				afterLoad(_loadFile);
			} else {
				createNew(true);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("No")) {
			ImGui::CloseCurrentPopup();
			_loadFile.clear();
		}
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal("Invalid dimensions", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::TextUnformatted(ICON_FA_EXCLAMATION_TRIANGLE"The layer dimensions are not valid!");
		ImGui::Separator();
		if (ImGui::Button(ICON_FA_CHECK"OK")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal("Failed to save", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::TextUnformatted(ICON_FA_EXCLAMATION_TRIANGLE"Failed to save the model!");
		ImGui::Separator();
		if (ImGui::Button(ICON_FA_CHECK"OK")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void VoxEditWindow::update() {
	const ImVec2 pos(0.0f, 0.0f);
	// const ImVec2 size = _app->frameBufferDimension();
	ImGuiViewport *viewport = ImGui::GetMainViewport();

	ImGui::SetNextWindowPos(viewport->GetWorkPos());
	ImGui::SetNextWindowSize(viewport->GetWorkSize());
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
	registerPopups();

	const ImGuiID dockspaceId = ImGui::GetID("DockSpace");
	ImGui::DockSpace(dockspaceId);

	leftWidget();
	mainWidget();
	rightWidget();

	ImGui::End();

	static bool init = false;
	if (!init) {
		ImGui::DockBuilderRemoveNode(dockspaceId);
		ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->GetWorkSize());
		ImGuiID dockIdMain = dockspaceId;
		ImGuiID dockIdLeft = ImGui::DockBuilderSplitNode(dockIdMain, ImGuiDir_Left, 0.10f, nullptr, &dockIdMain);
		ImGuiID dockIdRight = ImGui::DockBuilderSplitNode(dockIdMain, ImGuiDir_Right, 0.20f, nullptr, &dockIdMain);
		ImGuiID dockIdLeftDown = ImGui::DockBuilderSplitNode(dockIdLeft, ImGuiDir_Down, 0.50f, nullptr, &dockIdLeft);
		ImGuiID dockIdRightDown = ImGui::DockBuilderSplitNode(dockIdRight, ImGuiDir_Down, 0.50f, nullptr, &dockIdRight);
		ImGui::DockBuilderDockWindow("Palette", dockIdLeft);
		ImGui::DockBuilderDockWindow("Positions", dockIdRight);
		ImGui::DockBuilderDockWindow("Modifiers", dockIdRight);
		ImGui::DockBuilderDockWindow("Layers", dockIdRightDown);
		ImGui::DockBuilderDockWindow("Tools", dockIdLeftDown);
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

bool VoxEditWindow::saveImage(const char *file) {
	return _scene->saveImage(file);
}

bool VoxEditWindow::isSceneHovered() const {
	return _scene->isHovered() || _sceneTop->isHovered() ||
		   _sceneLeft->isHovered() || _sceneFront->isHovered() ||
		   _sceneAnimation->isHovered();
}
}
