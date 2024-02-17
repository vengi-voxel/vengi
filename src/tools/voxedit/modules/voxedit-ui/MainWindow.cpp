/**
 * @file
 */

#include "MainWindow.h"
#include "PopupAbout.h"
#include "ScopedStyle.h"
#include "Util.h"
#include "Viewport.h"
#include "core/ArrayLength.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "imgui.h"
#include "io/FormatDescription.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "ui/FileDialog.h"
#include "ui/IMGUIApp.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "util/TextProcessor.h"
#include "util/VersionCheck.h"
#include "video/Texture.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxelformat/VolumeFormat.h"
#include <glm/gtc/type_ptr.hpp>
#include "engine-config.h"
#include "TipOfTheDay.h"

// generated models
#include "chess.h"
#include "chess_png.h"
#include "chr_blacksmith.h"
#include "chr_blacksmith_png.h"
#include "chr_dwarf.h"
#include "chr_dwarf_png.h"
#include "chr_female.h"
#include "chr_female_png.h"
#include "chr_knight.h"
#include "chr_knight2.h"
#include "chr_knight2_png.h"
#include "chr_knight_png.h"
#include "chr_man.h"
#include "chr_man_png.h"
#include "chr_oldman.h"
#include "chr_oldman_png.h"
#include "head.h"
#include "head_png.h"
#include "river.h"
#include "river_png.h"
#include "robo.h"
#include "robo_png.h"
#include "skeleton.h"
#include "skeleton_png.h"
#include "twinsen.h"
#include "twinsen_png.h"
#include "undead.h"
#include "undead_png.h"

#define TITLE_STATUSBAR "##statusbar"
#define TITLE_PALETTE ICON_LC_PALETTE " Palette##title"
#define TITLE_POSITIONS ICON_LC_LOCATE " Positions##title"
#define TITLE_ANIMATION_TIMELINE ICON_LC_TABLE " Animation##animationtimeline"
#define TITLE_TOOLS ICON_LC_WRENCH " Tools##title"
#define TITLE_MEMENTO ICON_LC_BOOK_OPEN " History##title"
#define TITLE_ASSET ICON_LC_LIST " Assets##title"
#define TITLE_SCRIPT ICON_LC_CODE " Scripts##title"
#define TITLE_SCENEGRAPH ICON_LC_WORKFLOW " Scene##title"
#define TITLE_RENDER ICON_LC_IMAGE " Render##title"
#define TITLE_TREES ICON_LC_TREE_PINE " Trees##title"
#define TITLE_BRUSHPANEL ICON_LC_BRUSH " Brush##title"
#define TITLE_LSYSTEMPANEL ICON_LC_LEAF " L-System##title"
#define TITLE_ANIMATION_SETTINGS ICON_LC_LAYOUT_LIST " Animation##animationsettings"
#define TITLE_SCRIPT_EDITOR ICON_LC_CODE " Script Editor##scripteditor"

#define POPUP_TITLE_UNSAVED "Unsaved Modifications##popuptitle"
#define POPUP_TITLE_NEW_SCENE "New scene##popuptitle"
#define POPUP_TITLE_FAILED_TO_SAVE "Failed to save##popuptitle"
#define POPUP_TITLE_UNSAVED_SCENE "Unsaved scene##popuptitle"
#define POPUP_TITLE_SCENE_SETTINGS "Scene settings##popuptitle"
#define POPUP_TITLE_MODEL_NODE_SETTINGS "Model settings##popuptitle"
#define POPUP_TITLE_TIPOFTHEDAY "Tip of the day##popuptitle"
#define POPUP_TITLE_WELCOME "Welcome##popuptitle"
#define POPUP_TITLE_VOLUME_SPLIT "Volume split##popuptitle"
#define POPUP_TITLE_RENAME_NODE "Rename node##scenegraphrenamenode"

namespace voxedit {

// clang-format off
#define TM_ENTRY(name, id) {name, id##_data, id##_png_data, id##_size, id##_png_size}
static const struct TemplateModel {
	const char *name;
	const unsigned int *data;
	const unsigned int *imageData;
	const unsigned int size;
	const unsigned int imageSize;
} TEMPLATEMODELS[] = {
	TM_ENTRY("Chess", chess),
	TM_ENTRY("Dwarf", chr_dwarf),
	TM_ENTRY("Blacksmith", chr_blacksmith),
	TM_ENTRY("Female", chr_female),
	TM_ENTRY("Man", chr_man),
	TM_ENTRY("Old man", chr_oldman),
	TM_ENTRY("Knight 2", chr_knight2),
	TM_ENTRY("Knight", chr_knight),
	TM_ENTRY("Head", head),
	TM_ENTRY("Robo", robo),
	TM_ENTRY("River", river),
	TM_ENTRY("undead", undead),
	TM_ENTRY("skeleton", skeleton),
	TM_ENTRY("Twinsen", twinsen)
};
#undef TM_ENTRY
// clang-format on

MainWindow::MainWindow(ui::IMGUIApp *app) : _app(app), _assetPanel(app->filesystem()) {
	_currentTip = (uint32_t)((uint64_t)app->nowSeconds()) % ((uint64_t)lengthof(TIPOFTHEDAY));
}

MainWindow::~MainWindow() {
	shutdownScenes();
}

const char *MainWindow::getTip() const {
	static char buf[4096];
	const char *tip = TIPOFTHEDAY[_currentTip];
	if (!util::replacePlaceholders(_app->keybindingHandler(), tip, buf, sizeof(buf))) {
		return tip;
	}
	return buf;
}

void MainWindow::loadLastOpenedFiles(const core::String &string) {
	core::DynamicArray<core::String> tokens;
	core::string::splitString(string, tokens, ";");
	for (const core::String &s : tokens) {
		_lastOpenedFilesRingBuffer.push_back(s);
	}
}

void MainWindow::addLastOpenedFile(const core::String &file) {
	for (const core::String &s : _lastOpenedFilesRingBuffer) {
		if (s == file) {
			return;
		}
	}
	_lastOpenedFilesRingBuffer.push_back(file);
	core::String str;
	for (const core::String &s : _lastOpenedFilesRingBuffer) {
		if (!str.empty()) {
			str.append(";");
		}
		str.append(s);
	}
	_lastOpenedFiles->setVal(str);
}

void MainWindow::shutdownScenes() {
	for (size_t i = 0; i < _scenes.size(); ++i) {
		delete _scenes[i];
	}
	sceneMgr().setActiveCamera(nullptr);
	_scenes.clear();
	_lastHoveredScene = nullptr;
}

bool MainWindow::initScenes() {
	shutdownScenes();

	if (_simplifiedView->boolVal()) {
		_scenes.resize(2);
		_scenes[0] = new Viewport(0, false, false);
		_scenes[1] = new Viewport(1, true, false);
	} else {
		_scenes.resize(_numViewports->intVal());
		bool sceneMode = true;
		for (int i = 0; i < _numViewports->intVal(); ++i) {
			_scenes[i] = new Viewport(i, sceneMode, true);
			sceneMode = false;
		}
	}
	bool success = true;
	for (size_t i = 0; i < _scenes.size(); ++i) {
		if (!_scenes[i]->init()) {
			Log::error("Failed to initialize scene %i", (int)i);
			success = false;
		}
	}
	_lastHoveredScene = _scenes[0];

	_simplifiedView->markClean();
	_numViewports->markClean();
	return success;
}

bool MainWindow::init() {
	_simplifiedView = core::Var::getSafe(cfg::VoxEditSimplifiedView);
	_numViewports = core::Var::getSafe(cfg::VoxEditViewports);
	_tipOfTheDay = core::Var::getSafe(cfg::VoxEditTipOftheDay);
	_popupTipOfTheDay = core::Var::getSafe(cfg::VoxEditPopupTipOfTheDay);
	_popupWelcome = core::Var::getSafe(cfg::VoxEditPopupWelcome);
	_popupSceneSettings = core::Var::getSafe(cfg::VoxEditPopupSceneSettings);
	_popupAbout = core::Var::getSafe(cfg::VoxEditPopupAbout);
	_popupRenameNode = core::Var::getSafe(cfg::VoxEditPopupRenameNode);

	_isNewVersionAvailable = util::isNewVersionAvailable();
	if (!initScenes()) {
		return false;
	}

	_popupTipOfTheDay->setVal(_tipOfTheDay->boolVal());

	const core::VarPtr& appVersion = core::Var::getSafe(cfg::AppVersion);
	if (appVersion->strVal().empty() || util::isNewerVersion(PROJECT_VERSION, appVersion->strVal())) {
		appVersion->setVal(PROJECT_VERSION);
		_popupWelcome->setVal("true");
	}

#if ENABLE_RENDER_PANEL
	_renderPanel.init();
#endif
	_sceneGraphPanel.init();
	_lsystemPanel.init();
	_treePanel.init();
	_texturePool.init();
	_positionsPanel.init();

	for (int i = 0; i < lengthof(TEMPLATEMODELS); ++i) {
		_texturePool.load(TEMPLATEMODELS[i].name, (const uint8_t *)TEMPLATEMODELS[i].imageData,
						  (size_t)TEMPLATEMODELS[i].imageSize);
	}

	_lastOpenedFile = core::Var::getSafe(cfg::VoxEditLastFile);
	_lastOpenedFiles = core::Var::getSafe(cfg::VoxEditLastFiles);
	loadLastOpenedFiles(_lastOpenedFiles->strVal());

	SceneManager &mgr = sceneMgr();
	voxel::Region region = _modelNodeSettings.region();
	if (!region.isValid()) {
		_modelNodeSettings.reset();
		region = _modelNodeSettings.region();
	}
	if (!mgr.newScene(true, _modelNodeSettings.name, region)) {
		return false;
	}
	afterLoad("");
	return true;
}

void MainWindow::shutdown() {
	for (size_t i = 0; i < _scenes.size(); ++i) {
		_scenes[i]->shutdown();
	}
#if ENABLE_RENDER_PANEL
	_renderPanel.shutdown();
#endif
	_lsystemPanel.shutdown();
	_treePanel.shutdown();
	_positionsPanel.shutdown();
	_texturePool.shutdown();
}

bool MainWindow::save(const core::String &file, const io::FormatDescription *desc) {
	io::FileDescription fd;
	const core::String &ext = core::string::extractExtension(file);
	if (ext.empty()) {
		core::String newExt = voxelformat::vengi().mainExtension();
		if (desc && !desc->exts.empty()) {
			newExt = desc->exts[0];
		}
		fd.set(file + "." + newExt, desc);
	} else {
		fd.set(file, desc);
	}
	if (!sceneMgr().save(fd)) {
		Log::warn("Failed to save the model");
		_popupFailedToSave = true;
		return false;
	}
	Log::info("Saved the model to %s", fd.c_str());
	_lastOpenedFile->setVal(fd.name);
	return true;
}

bool MainWindow::load(const core::String &file, const io::FormatDescription *formatDesc) {
	if (file.empty()) {
		_app->openDialog([this](const core::String file, const io::FormatDescription *desc) { load(file, desc); }, {},
						 voxelformat::voxelLoad());
		return true;
	}

	if (!sceneMgr().dirty()) {
		io::FileDescription fd;
		fd.set(file, formatDesc);
		if (sceneMgr().load(fd)) {
			afterLoad(file);
			return true;
		}
		return false;
	}

	_loadFile.set(file, formatDesc);
	_popupUnsaved = true;
	return false;
}

void MainWindow::onNewScene() {
	resetCamera();
	_animationTimeline.resetFrames();
	checkPossibleVolumeSplit();
}

void MainWindow::afterLoad(const core::String &file) {
	_lastOpenedFile->setVal(file);
	resetCamera();
}

void MainWindow::checkPossibleVolumeSplit() {
	const int maxDim = 128;
	const int maxVoxels = maxDim * maxDim * maxDim;
	const scenegraph::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
	for (const auto &entry : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = entry->second;
		if (node.type() != scenegraph::SceneGraphNodeType::Model) {
			continue;
		}
		const voxel::Region &region = node.region();
		const glm::ivec3 &dim = region.getDimensionsInVoxels();
		if (dim.x * dim.y * dim.z > maxVoxels) {
			Log::debug("node: %s exceeds the max size with %i:%i:%i", node.name().c_str(), dim.x, dim.y, dim.z);
			_popupVolumeSplit = true;
			return;
		}
	}
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

bool MainWindow::isSceneGraphDropTarget() const {
	return _sceneGraphPanel.hasFocus();
}

bool MainWindow::isPaletteWidgetDropTarget() const {
	return _palettePanel.hasFocus();
}

// left side

void MainWindow::configureLeftTopWidgetDock(ImGuiID dockId) {
	ImGui::DockBuilderDockWindow(TITLE_PALETTE, dockId);
}

void MainWindow::configureLeftBottomWidgetDock(ImGuiID dockId) {
	ImGui::DockBuilderDockWindow(TITLE_BRUSHPANEL, dockId);
}

void MainWindow::leftWidget() {
	const bool editMode = isAnyEditMode();
	command::CommandExecutionListener &listener = imguiApp()->commandListener();
	_palettePanel.update(TITLE_PALETTE, listener);
	if (editMode) {
		_brushPanel.update(TITLE_BRUSHPANEL, listener);
	}
}

// end of left side

// main space

void MainWindow::configureMainTopWidgetDock(ImGuiID dockId) {
	for (int i = 0; i < cfg::MaxViewports; ++i) {
		ImGui::DockBuilderDockWindow(Viewport::viewportId(i).c_str(), dockId);
	}
}

void MainWindow::configureMainBottomWidgetDock(ImGuiID dockId) {
	ImGui::DockBuilderDockWindow(TITLE_SCRIPT_EDITOR, dockId);
	ImGui::DockBuilderDockWindow(TITLE_ANIMATION_TIMELINE, dockId);
	ImGui::DockBuilderDockWindow(UI_CONSOLE_WINDOW_TITLE, dockId);
}

void MainWindow::mainWidget() {
	// main
	Viewport *scene = hoveredScene();
	if (scene != nullptr) {
		_lastHoveredScene = scene;
	}
	command::CommandExecutionListener &listener = imguiApp()->commandListener();
	for (size_t i = 0; i < _scenes.size(); ++i) {
		_scenes[i]->update(&listener);
	}

	// bottom
	_scriptPanel.updateEditor(TITLE_SCRIPT_EDITOR, _app);
	if (isSceneMode()) {
		_animationTimeline.update(TITLE_ANIMATION_TIMELINE, _app->deltaFrameSeconds());
	}
}

bool MainWindow::isSceneMode() const {
	for (size_t i = 0; i < _scenes.size(); ++i) {
		if (_scenes[i]->isSceneMode()) {
			return true;
		}
	}
	return false;
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
	ImGui::DockBuilderDockWindow(TITLE_SCENEGRAPH, dockId);
	ImGui::DockBuilderDockWindow(TITLE_TREES, dockId);
	ImGui::DockBuilderDockWindow(TITLE_LSYSTEMPANEL, dockId);
	ImGui::DockBuilderDockWindow(TITLE_RENDER, dockId);
	ImGui::DockBuilderDockWindow(TITLE_SCRIPT, dockId);
}

void MainWindow::rightWidget() {
	if (const Viewport *viewport = hoveredScene()) {
		_lastSceneMode = viewport->isSceneMode();
	}
	command::CommandExecutionListener &listener = imguiApp()->commandListener();
	// top
	_positionsPanel.update(TITLE_POSITIONS, _lastSceneMode, listener);
	_toolsPanel.update(TITLE_TOOLS, _lastSceneMode, listener);
	_assetPanel.update(TITLE_ASSET, _lastSceneMode, listener);
	_animationPanel.update(TITLE_ANIMATION_SETTINGS, listener, &_animationTimeline);
	_mementoPanel.update(TITLE_MEMENTO, listener);

	// bottom
	_sceneGraphPanel.update(_lastHoveredScene->camera(), TITLE_SCENEGRAPH, &_modelNodeSettings, listener);
#if ENABLE_RENDER_PANEL
	_renderPanel.update(TITLE_RENDER, sceneMgr().sceneGraph());
#endif
	if (!_simplifiedView->boolVal()) {
		_treePanel.update(TITLE_TREES);
		_lsystemPanel.update(TITLE_LSYSTEMPANEL);
		_scriptPanel.update(TITLE_SCRIPT, listener);
	}
}

// end of right side

void MainWindow::addTemplate(const TemplateModel &model) {
	io::FileDescription fileDesc;
	const core::String name = model.name;
	fileDesc.name = name + voxelformat::vengi().mainExtension(true);
	fileDesc.desc = voxelformat::vengi();
	ImGui::TableNextColumn();
	const video::TexturePtr &texture = _texturePool.get(name);
	const core::String id = "##" + name;
	const ImVec2 size((float)texture->width(), (float)texture->height());
	if (ImGui::ImageButton(texture->handle(), size)) {
		ImGui::CloseCurrentPopup();
		sceneMgr().load(fileDesc, (const uint8_t *)model.data, (size_t)model.size);
	}
	ImGui::TooltipText("%s", name.c_str());
}

void MainWindow::newSceneTemplates() {
	if (_texturePool.cache().empty()) {
		return;
	}
	const float height = _texturePool.cache().begin()->second->height();

	if (ImGui::BeginTable("##templates", 4, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY,
						  ImVec2(0.0f, height * 3))) {
		for (int i = 0; i < lengthof(TEMPLATEMODELS); ++i) {
			addTemplate(TEMPLATEMODELS[i]);
		}
		ImGui::EndTable();
	}
}

void MainWindow::popupTipOfTheDay() {
	ImGui::SetNextWindowSize(ImVec2(ImGui::GetFontSize() * 30, 0));
	if (ImGui::BeginPopupModal(POPUP_TITLE_TIPOFTHEDAY)) {
		const char *tip = getTip();
		ImGui::IconDialog(ICON_LC_LIGHTBULB, tip);
		float height = (ImGui::GetFontSize() * 8.0f) - ImGui::GetCursorPosY();
		if (height > 0.0f) {
			ImGui::Dummy(ImVec2(0, height));
		}
		ImGui::CheckboxVar("Show again", _tipOfTheDay);
		if (ImGui::Button(ICON_LC_CHECK " Next##tipoftheday")) {
			++_currentTip;
			_currentTip %= (uint32_t)lengthof(TIPOFTHEDAY);
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button(ICON_LC_X " Close##tipoftheday")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void MainWindow::popupWelcome() {
	ImGui::SetNextWindowSize(ImVec2(ImGui::GetFontSize() * 30, 0));
	if (ImGui::BeginPopupModal(POPUP_TITLE_WELCOME)) {
		ImGui::IconDialog(ICON_LC_LIGHTBULB, "Welcome to VoxEdit!");
		ImGui::TextWrapped("The mission: Create a free, open-source and multi-platform voxel "
						   "editor with animation support for artists and developers.");
		ImGui::Separator();
		ImGui::TextWrapped("We would like to enable anonymous usage metrics to improve the editor. "
						   "Please consider enabling it.");
		ui::metricOption();
		ImGui::Separator();
		if (ImGui::Button(ICON_LC_X " Close##welcome")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void MainWindow::popupNewScene() {
	if (ImGui::BeginPopupModal(POPUP_TITLE_NEW_SCENE, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		if (ImGui::CollapsingHeader("Templates", ImGuiTreeNodeFlags_DefaultOpen)) {
			newSceneTemplates();
		}

		if (ImGui::CollapsingHeader("Empty scene", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Text("Name");
			ImGui::Separator();
			ImGui::InputText("##newscenename", &_modelNodeSettings.name);
			ImGui::NewLine();

			ImGui::Text("Position");
			ImGui::Separator();
			veui::InputAxisInt(math::Axis::X, "##posx", &_modelNodeSettings.position.x);
			veui::InputAxisInt(math::Axis::Y, "##posy", &_modelNodeSettings.position.y);
			veui::InputAxisInt(math::Axis::Z, "##posz", &_modelNodeSettings.position.z);
			ImGui::NewLine();

			ImGui::Text("Size");
			ImGui::Separator();
			veui::InputAxisInt(math::Axis::X, "Width##sizex", &_modelNodeSettings.size.x);
			veui::InputAxisInt(math::Axis::Y, "Height##sizey", &_modelNodeSettings.size.y);
			veui::InputAxisInt(math::Axis::Z, "Depth##sizez", &_modelNodeSettings.size.z);
			ImGui::NewLine();
		}

		if (ImGui::Button(ICON_LC_CHECK " OK##newscene")) {
			ImGui::CloseCurrentPopup();
			const voxel::Region &region = _modelNodeSettings.region();
			if (voxedit::sceneMgr().newScene(true, _modelNodeSettings.name, region)) {
				afterLoad("");
			}
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button(ICON_LC_X " Close##newscene")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void MainWindow::popupFailedSave() {
	if (ImGui::BeginPopup(POPUP_TITLE_FAILED_TO_SAVE, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::IconDialog(ICON_LC_ALERT_TRIANGLE, "Failed to save the model!");
		if (ImGui::Button(ICON_LC_CHECK " OK##failedsave")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}
}

void MainWindow::popupUnsavedChanges() {
	if (ImGui::BeginPopupModal(POPUP_TITLE_UNSAVED_SCENE, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::IconDialog(ICON_LC_HELP_CIRCLE, "Unsaved changes - are you sure to quit?");
		if (ImGui::Button(ICON_LC_CHECK " OK##unsavedscene")) {
			_forceQuit = true;
			_app->requestQuit();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button(ICON_LC_X " Cancel##unsavedscene")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void MainWindow::popupUnsavedDiscard() {
	if (ImGui::BeginPopupModal(POPUP_TITLE_UNSAVED, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::IconDialog(ICON_LC_HELP_CIRCLE, "There are unsaved modifications.\nDo you wish to discard them?");
		if (ImGui::Button(ICON_LC_CHECK " Yes##unsaved")) {
			ImGui::CloseCurrentPopup();
			if (!_loadFile.empty()) {
				sceneMgr().load(_loadFile);
				afterLoad(_loadFile.name);
			} else {
				createNew(true);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button(ICON_LC_X " No##unsaved")) {
			ImGui::CloseCurrentPopup();
			_loadFile.clear();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}
}

void MainWindow::popupSceneSettings() {
	if (ImGui::BeginPopup(POPUP_TITLE_SCENE_SETTINGS, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::TextUnformatted("Scene settings");
		ImGui::Separator();

		ImGui::ColorEdit3Var("Diffuse color", cfg::VoxEditDiffuseColor);
		ImGui::ColorEdit3Var("Ambient color", cfg::VoxEditAmbientColor);

		if (ImGui::Button(ICON_LC_CHECK " Done##scenesettings")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}
}

void MainWindow::popupVolumeSplit() {
	if (ImGui::BeginPopupModal(POPUP_TITLE_VOLUME_SPLIT, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::IconDialog(ICON_LC_HELP_CIRCLE, "Some model volumes are too big for optimal performance.\nIt's encouraged to split "
								 "them into smaller volumes.\nDo you wish to split them now?");
		if (ImGui::Button(ICON_LC_CHECK " Yes##volumesplit")) {
			ImGui::CloseCurrentPopup();
			sceneMgr().splitVolumes();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button(ICON_LC_X " No##volumesplit")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void MainWindow::popupModelNodeSettings() {
	if (ImGui::BeginPopupModal(POPUP_TITLE_MODEL_NODE_SETTINGS, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Name");
		ImGui::Separator();
		ImGui::InputText("##modelsettingsname", &_modelNodeSettings.name);
		ImGui::NewLine();

		ImGui::Text("Position");
		ImGui::Separator();
		veui::InputAxisInt(math::Axis::X, "##posx", &_modelNodeSettings.position.x);
		veui::InputAxisInt(math::Axis::Y, "##posy", &_modelNodeSettings.position.y);
		veui::InputAxisInt(math::Axis::Z, "##posz", &_modelNodeSettings.position.z);
		ImGui::NewLine();

		ImGui::Text("Size");
		ImGui::Separator();
		veui::InputAxisInt(math::Axis::X, "Width##sizex", &_modelNodeSettings.size.x);
		veui::InputAxisInt(math::Axis::Y, "Height##sizey", &_modelNodeSettings.size.y);
		veui::InputAxisInt(math::Axis::Z, "Depth##sizez", &_modelNodeSettings.size.z);
		ImGui::NewLine();

		if (ImGui::Button(ICON_LC_CHECK " OK##modelsettings")) {
			ImGui::CloseCurrentPopup();
			scenegraph::SceneGraphNode newNode;
			voxel::RawVolume *v = new voxel::RawVolume(_modelNodeSettings.region());
			newNode.setVolume(v, true);
			newNode.setName(_modelNodeSettings.name.c_str());
			if (_modelNodeSettings.palette.hasValue()) {
				newNode.setPalette(*_modelNodeSettings.palette.value());
			}
			sceneMgr().addNodeToSceneGraph(newNode, _modelNodeSettings.parent);
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button(ICON_LC_X " Cancel##modelsettings")) {
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
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
	if (_popupVolumeSplit) {
		ImGui::OpenPopup(POPUP_TITLE_VOLUME_SPLIT);
		_popupVolumeSplit = false;
	}
	if (_popupUnsavedChangesQuit) {
		ImGui::OpenPopup(POPUP_TITLE_UNSAVED_SCENE);
		_popupUnsavedChangesQuit = false;
	}
	if (_sceneGraphPanel._popupNewModelNode) {
		ImGui::OpenPopup(POPUP_TITLE_MODEL_NODE_SETTINGS);
		_sceneGraphPanel._popupNewModelNode = false;
	}

	// popups that can get triggers externally
	if (_popupSceneSettings->boolVal()) {
		ImGui::OpenPopup(POPUP_TITLE_SCENE_SETTINGS);
		_popupSceneSettings->setVal("false");
	}
	if (_popupTipOfTheDay->boolVal()) {
		ImGui::OpenPopup(POPUP_TITLE_TIPOFTHEDAY);
		_popupTipOfTheDay->setVal("false");
	}
	if (_popupWelcome->boolVal()) {
		ImGui::OpenPopup(POPUP_TITLE_WELCOME);
		_popupWelcome->setVal("false");
	}
	if (_popupAbout->boolVal()) {
		ImGui::OpenPopup(POPUP_TITLE_ABOUT);
		_popupAbout->setVal("false");
	}
	if (_popupRenameNode->boolVal()) {
		ImGui::OpenPopup(POPUP_TITLE_RENAME_NODE);
		const scenegraph::SceneGraph &sceneGraph = voxedit::sceneMgr().sceneGraph();
		_currentNodeName = sceneGraph.node(sceneGraph.activeNode()).name();
		_popupRenameNode->setVal("false");
	}

	popupModelNodeSettings();
	popupSceneSettings();
	popupUnsavedDiscard();
	popupUnsavedChanges();
	popupFailedSave();
	popupNewScene();
	popupVolumeSplit();
	popupTipOfTheDay();
	popupAbout();
	popupWelcome();
	popupNodeRename();
}

void MainWindow::popupNodeRename() {
	voxedit::SceneManager &sceneMgr = voxedit::sceneMgr();
	if (ImGui::BeginPopupModal(POPUP_TITLE_RENAME_NODE, nullptr,
							   ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
		ImGui::InputText("Name", &_currentNodeName, ImGuiInputTextFlags_AutoSelectAll);
		if (ImGui::Button("Apply")) {
			const int nodeId = sceneMgr.sceneGraph().activeNode();
			sceneMgr.nodeRename(nodeId, _currentNodeName);
			_currentNodeName = "";
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Close")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}
}

void MainWindow::popupAbout() {
	ui::popupAbout([]() {
		if (ImGui::BeginTabItem("Formats")) {
			const ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_Sortable;
			ImGui::Text("Voxel load");
			if (ImGui::BeginTable("##voxelload", 2, tableFlags)) {
				ImGui::TableSetupColumn("Name##voxelload", ImGuiTableColumnFlags_WidthStretch, 0.7f, 0);
				ImGui::TableSetupColumn("Extension##voxelload", ImGuiTableColumnFlags_WidthStretch, 0.09f, 1);
				ImGui::TableSetupScrollFreeze(0, 1);
				ImGui::TableHeadersRow();
				for (const io::FormatDescription *desc = voxelformat::voxelLoad(); desc->valid(); ++desc) {
					ImGui::TableNextColumn();
					ImGui::Text("%s", desc->name.c_str());
					ImGui::TableNextColumn();
					ImGui::Text("%s", desc->wildCard().c_str());
				}
				ImGui::EndTable();
			}
			ImGui::Dummy(ImVec2(1, 10));
			ImGui::Text("Voxel save");
			if (ImGui::BeginTable("##voxelsave", 2, tableFlags)) {
				for (const io::FormatDescription *desc = voxelformat::voxelSave(); desc->valid(); ++desc) {
					ImGui::TableNextColumn();
					ImGui::Text("%s", desc->name.c_str());
					ImGui::TableNextColumn();
					ImGui::Text("%s", desc->wildCard().c_str());
				}
				ImGui::EndTable();
			}
			ImGui::Dummy(ImVec2(1, 10));
			ImGui::Text("Palettes");
			if (ImGui::BeginTable("##palettes", 2, tableFlags)) {
				for (const io::FormatDescription *desc = io::format::palettes(); desc->valid(); ++desc) {
					ImGui::TableNextColumn();
					ImGui::Text("%s", desc->name.c_str());
					ImGui::TableNextColumn();
					ImGui::Text("%s", desc->wildCard().c_str());
				}
				ImGui::EndTable();
			}
			ImGui::Dummy(ImVec2(1, 10));
			ImGui::Text("Images");
			if (ImGui::BeginTable("##images", 2, tableFlags)) {
				for (const io::FormatDescription *desc = io::format::images(); desc->valid(); ++desc) {
					ImGui::TableNextColumn();
					ImGui::Text("%s", desc->name.c_str());
					ImGui::TableNextColumn();
					ImGui::Text("%s", desc->wildCard().c_str());
				}
				ImGui::EndTable();
			}
			ImGui::EndTabItem();
		}
	}, _isNewVersionAvailable);
}

QuitDisallowReason MainWindow::allowToQuit() {
	if (_forceQuit) {
		return QuitDisallowReason::None;
	}
	if (voxedit::sceneMgr().dirty()) {
		_popupUnsavedChangesQuit = true;
		return QuitDisallowReason::UnsavedChanges;
	}
	return QuitDisallowReason::None;
}

void MainWindow::update() {
	core_trace_scoped(MainWindow);
	if (_simplifiedView->isDirty() || _numViewports->isDirty()) {
		if (!initScenes()) {
			Log::error("Failed to update scenes");
		}
	}

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
		windowFlags |= ImGuiWindowFlags_NoTitleBar;
		if (sceneMgr().dirty()) {
			windowFlags |= ImGuiWindowFlags_UnsavedDocument;
		}

		core::String windowTitle = core::string::extractFilenameWithExtension(_lastOpenedFile->strVal());
		if (windowTitle.empty()) {
			windowTitle = _app->fullAppname();
		} else {
			windowTitle.append(" - ");
			windowTitle.append(_app->fullAppname());
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
	command::CommandExecutionListener &listener = imguiApp()->commandListener();
	if (_menuBar.update(_app, listener)) {
		ImGui::DockBuilderRemoveNode(dockIdMain);
	}

	const bool existingLayout = ImGui::DockBuilderGetNode(dockIdMain);
	ImGui::DockSpace(dockIdMain);

	leftWidget();
	mainWidget();
	rightWidget();

	registerPopups();

	ImGui::End();

	_statusBar.update(TITLE_STATUSBAR, statusBarHeight, listener.command);

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
}

bool MainWindow::isAnyEditMode() const {
	for (size_t i = 0; i < _scenes.size(); ++i) {
		if (!_scenes[i]->isVisible()) {
			continue;
		}
		if (_scenes[i]->isSceneMode()) {
			continue;
		}
		return true;
	}
	return false;
}

Viewport *MainWindow::hoveredScene() {
	for (size_t i = 0; i < _scenes.size(); ++i) {
		if (_scenes[i]->isHovered()) {
			return _scenes[i];
		}
	}
	return nullptr;
}

bool MainWindow::saveScreenshot(const core::String &file, const core::String &viewportId) {
	if (viewportId.empty()) {
		if (_lastHoveredScene != nullptr) {
			if (!_lastHoveredScene->saveImage(file.c_str())) {
				Log::warn("Failed to save screenshot to file '%s'", file.c_str());
				return false;
			}
			Log::info("Screenshot created at '%s'", file.c_str());
			return true;
		}
		return false;
	}
	for (Viewport *vp : _scenes) {
		if (vp->id() != viewportId.toInt()) {
			continue;
		}
		if (!vp->saveImage(file.c_str())) {
			Log::warn("Failed to save screenshot to file '%s'", file.c_str());
			return false;
		}
		Log::info("Screenshot created at '%s'", file.c_str());
		return true;
	}
	return false;
}

void MainWindow::resetCamera() {
	if (Viewport *scene = hoveredScene()) {
		scene->resetCamera();
	} else {
		for (size_t i = 0; i < _scenes.size(); ++i) {
			_scenes[i]->resetCamera();
		}
	}
}

void MainWindow::toggleScene() {
	if (Viewport *scene = hoveredScene()) {
		scene->toggleScene();
	} else {
		for (size_t i = 0; i < _scenes.size(); ++i) {
			_scenes[i]->toggleScene();
		}
	}
}

} // namespace voxedit
