/**
 * @file
 */

#include "VoxEditWindow.h"
#include "Viewport.h"
#include "command/CommandHandler.h"
#include "core/StringUtil.h"
#include "ui/imgui/IMGUI.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/anim/AnimationLuaSaver.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxel/MaterialColor.h"
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

bool VoxEditWindow::actionButton(const char *title, const char *command) {
	if (ImGui::Button(title)) {
		_lastExecutedCommand = command;
		command::executeCommands(_lastExecutedCommand);
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

bool VoxEditWindow::actionMenuItem(const char *title, const char *command) {
	if (ImGui::MenuItem(title, command)) {
		_lastExecutedCommand = command;
		command::executeCommands(_lastExecutedCommand);
		return true;
	}
	return false;
}

bool VoxEditWindow::mirrorAxisRadioButton(const char *title, math::Axis type) {
	if (ImGui::RadioButton(title, sceneMgr().modifier().mirrorAxis() == type)) {
		sceneMgr().modifier().setMirrorAxis(type, sceneMgr().referencePosition());
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
	return false;
}

bool VoxEditWindow::isPaletteWidgetDropTarget() const {
	return false;
}

void VoxEditWindow::menuBar() {
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			actionMenuItem("New", "new");
			actionMenuItem("Load", "load");
			actionMenuItem("Save", "save");
			actionMenuItem("Load Animation", "animation_load");
			actionMenuItem("Save Animation", "animation_save");
			actionMenuItem("Prefab", "prefab");
			actionMenuItem("Heightmap", "importheightmap");
			actionMenuItem("Image as Plane", "importplane");
			if (ImGui::MenuItem("Quit")) {
				_app->requestQuit();
			}
			ImGui::EndMenu();
		}
		actionMenuItem("Undo", "undo");
		actionMenuItem("Redo", "redo");
		if (ImGui::MenuItem("Settings")) {
			// TBButton: gravity: left, @include: definitions>menubutton, text: Settings, id: scene_settings_open
		}
		if (ImGui::MenuItem("Trees")) {
			// TBButton: gravity: left, @include: definitions>menubutton, text: Trees, id: show_tree_panel
		}
		if (ImGui::MenuItem("Scripts")) {
			// TBButton: gravity: left, @include: definitions>menubutton, text: Scripts, id: show_script_panel
		}
		if (ImGui::MenuItem("Noise")) {
			// TBButton: gravity: left, @include: definitions>menubutton, text: Noise, id: show_noise_panel
		}
		if (ImGui::MenuItem("L-System")) {
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

void VoxEditWindow::layers() {
}

void VoxEditWindow::statusBar() {
	ImGuiViewport *viewport = ImGui::GetMainViewport();
	const ImVec2 &size = viewport->GetWorkSize();
	const float statusBarHeight = ImGui::Size(30);
	ImGui::SetNextWindowSize(ImVec2(size.x, statusBarHeight));
	ImVec2 statusBarPos = viewport->GetWorkPos();
	statusBarPos.y += size.y - statusBarHeight;
	ImGui::SetNextWindowPos(statusBarPos);
	if (ImGui::Begin("##statusbar", nullptr,
					 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove)) {
		ImGui::Text("-");
		ImGui::SameLine();
		ImGui::Text("-");
		ImGui::SameLine();
		ImGui::CheckboxVar("Grid", _showGridVar);
		ImGui::SameLine();
		ImGui::InputVarInt("Grid size", _gridSizeVar);
		ImGui::SameLine();
		actionButton("Reset camera", "resetcamera");
		ImGui::SameLine();
		actionButton("Quad view", "toggleviewport");
		ImGui::SameLine();
		actionButton("Animation view", "toggleanimation");
		ImGui::SameLine();
		actionButton("Scene view", "togglescene");
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
	if (ImGui::Begin("Operations", nullptr, ImGuiWindowFlags_NoDecoration)) {
		actionButton("Crop", "crop");
		actionButton("Extend", "resize");
		actionButton("Layer from color", "colortolayer");
		actionButton("Scale", "scale");

		ImGui::Separator();
		if (ImGui::CollapsingHeader("Rotate on axis", ImGuiTreeNodeFlags_DefaultOpen)) {
			actionButton("Rotate X", "rotate 90 0 0");
			actionButton("Rotate Y", "rotate 0 90 0");
			actionButton("Rotate Z", "rotate 0 0 90");
		}

		ImGui::Separator();
		if (ImGui::CollapsingHeader("Flip on axis", ImGuiTreeNodeFlags_DefaultOpen)) {
			actionButton("Flip X", "flip x");
			actionButton("Flip Y", "flip y");
			actionButton("Flip Z", "flip z");
		}

		ImGui::Separator();
		if (ImGui::CollapsingHeader("Mirror on axis", ImGuiTreeNodeFlags_DefaultOpen)) {
			mirrorAxisRadioButton("none", math::Axis::None);
			mirrorAxisRadioButton("x", math::Axis::X);
			mirrorAxisRadioButton("y", math::Axis::Y);
			mirrorAxisRadioButton("z", math::Axis::Z);
		}

		ImGui::Separator();
		if (ImGui::CollapsingHeader("Options", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::CheckboxVar("Show axis", _showAxisVar);
			ImGui::CheckboxVar("Model space", _modelSpaceVar);
			ImGui::CheckboxVar("Show locked axis", _showLockedAxisVar);
			ImGui::CheckboxVar("Bounding box", _showAabbVar);
			ImGui::CheckboxVar("Shadow", _renderShadowVar);
			ImGui::CheckboxVar("Outlines", "r_renderoutline");
			ImGui::InputVarFloat("Animation speed", _animationSpeedVar);
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
		ImGui::TextUnformatted("There are unsaved modifications.\nDo you wish to discard them?");
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
		ImGui::TextUnformatted("The layer dimensions are not valid!");
		ImGui::Separator();
		if (ImGui::Button("OK")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal("Failed to save", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::TextUnformatted("Failed to save the model!");
		ImGui::Separator();
		if (ImGui::Button("OK")) {
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
		ImGuiID dockIdRight = ImGui::DockBuilderSplitNode(dockIdMain, ImGuiDir_Right, 0.10f, nullptr, &dockIdMain);
		ImGuiID dockIdLeftDown = ImGui::DockBuilderSplitNode(dockIdLeft, ImGuiDir_Down, 0.50f, nullptr, &dockIdLeft);
		ImGui::DockBuilderDockWindow("Palette", dockIdLeft);
		ImGui::DockBuilderDockWindow("Operations", dockIdRight);
		ImGui::DockBuilderDockWindow("Tools", dockIdLeftDown);
		ImGui::DockBuilderDockWindow("free", dockIdMain);
		ImGui::DockBuilderDockWindow("front", dockIdMain);
		ImGui::DockBuilderDockWindow("left", dockIdMain);
		ImGui::DockBuilderDockWindow("top", dockIdMain);

		ImGui::DockBuilderFinish(dockspaceId);
		init = true;
	}

	updateSettings();
}

bool VoxEditWindow::isSceneHovered() const {
	return false;
}

}
