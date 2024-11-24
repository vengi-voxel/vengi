/**
 * @file
 */

#include "MainWindow.h"
#include "ImGuizmo.h"
#include "Util.h"
#include "ViewMode.h"
#include "Viewport.h"
#include "WindowTitles.h"
#include "command/Command.h"
#include "core/ArrayLength.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "engine-config.h"
#include "io/Filesystem.h"
#include "io/FormatDescription.h"
#include "palette/PaletteFormatDescription.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "ui/FileDialog.h"
#include "ui/IMGUIApp.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "ui/PopupAbout.h"
#include "ui/ScopedStyle.h"
#include "util/TextProcessor.h"
#include "util/VersionCheck.h"
#include "video/Texture.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/private/vengi/VENGIFormat.h"
#include <glm/gtc/type_ptr.hpp>

// generated models
#include "aquarium.h"
#include "aquarium_png.h"
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
#include "hawk.h"
#include "hawk_png.h"
#include "head.h"
#include "head_png.h"
#include "locomotive.h"
#include "locomotive_png.h"
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

namespace voxedit {

// clang-format off
#define TM_ENTRY(name, id) {name, id##_data, id##_png_data, id##_size, id##_png_size}
static const struct TemplateModel {
	const char *name;
	const unsigned char *data;
	const unsigned char *imageData;
	const unsigned int size;
	const unsigned int imageSize;
} TEMPLATEMODELS[] = {
	TM_ENTRY("Aquarium", aquarium),
	TM_ENTRY("Chess", chess),
	TM_ENTRY("Dwarf", chr_dwarf),
	TM_ENTRY("Blacksmith", chr_blacksmith),
	TM_ENTRY("Female", chr_female),
	TM_ENTRY("Man", chr_man),
	TM_ENTRY("Old man", chr_oldman),
	TM_ENTRY("Knight 2", chr_knight2),
	TM_ENTRY("Knight", chr_knight),
	TM_ENTRY("Hawk", hawk),
	TM_ENTRY("Head", head),
	TM_ENTRY("Locomotive", locomotive),
	TM_ENTRY("Robo", robo),
	TM_ENTRY("River", river),
	TM_ENTRY("undead", undead),
	TM_ENTRY("skeleton", skeleton),
	TM_ENTRY("Twinsen", twinsen)
};
#undef TM_ENTRY
// clang-format on

bool MainWindow::_popupModelUnreference = false;

MainWindow::MainWindow(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr, const video::TexturePoolPtr &texturePool,
					   const voxelcollection::CollectionManagerPtr &collectionMgr, const io::FilesystemPtr &filesystem,
					   palette::PaletteCache &paletteCache)
	: Super(app, "main"), _texturePool(texturePool), _sceneMgr(sceneMgr),
#if ENABLE_RENDER_PANEL
	  _renderPanel(app, _sceneMgr),
#endif
	  _lsystemPanel(app, _sceneMgr), _brushPanel(app, _sceneMgr, texturePool), _treePanel(app, _sceneMgr),
	  _sceneGraphPanel(app, _sceneMgr), _toolsPanel(app, _sceneMgr),
	  _assetPanel(app, _sceneMgr, collectionMgr, texturePool, filesystem), _mementoPanel(app, _sceneMgr),
	  _nodeInspectorPanel(app, _sceneMgr), _palettePanel(app, _sceneMgr, paletteCache), _normalPalettePanel(app, _sceneMgr),
	  _menuBar(app, _sceneMgr), _statusBar(app, _sceneMgr), _scriptPanel(app, _sceneMgr),
	  _animationTimeline(app, _sceneMgr), _animationPanel(app, _sceneMgr, &_animationTimeline),
	  _cameraPanel(app, _sceneMgr) {

	_tipOfTheDayList.push_back(_("Switch between scene and edit mode (not in simple UI mode) by pressing the <cmd:togglescene> key."));
	_tipOfTheDayList.push_back(_("Use the file dialog options for format specific options."));
	_tipOfTheDayList.push_back(_("You can record a video of your rotating model in the viewport by using the 'View' menu and the 'Video' option. Make sure to set the camera rotation speed to e.g. 1."));
	_tipOfTheDayList.push_back(_("When saving to a foreign format you might lose scene details if the format doesn't support particular features. Make sure to use the vengi format for highest compatibility."));
	_tipOfTheDayList.push_back(_("Change the color reduction mode to improve the quality of the palette especially for importing RGBA or mesh based formats."));
	_tipOfTheDayList.push_back(_("Drag a model from the assets panel to the stamp brush to use it."));
	_tipOfTheDayList.push_back(_("You can reset the camera simply by pressing <cmd:resetcamera>."));
	_tipOfTheDayList.push_back(_("The editor shows the last executed console command in the status bar. You can use these commands and bind them to keys."));
	_tipOfTheDayList.push_back(_("You can use wasd style scene movement by switching the 'Camera movement' to 'Eye' in the 'View' menu of the viewport."));
	_tipOfTheDayList.push_back(_("In order to use the path modifier, you have to place the reference position on top of another voxel and place the end of the path on another existing and connected voxel."));
	_tipOfTheDayList.push_back(_("When importing meshes, you can switch between different voxelization methods in the options menu."));
	_tipOfTheDayList.push_back(_("Delete voxels in edit mode by pressing <cmd:+actionexecutedelete> or by using the erase modifier."));
	_tipOfTheDayList.push_back(_("Switch between different color themes in the options menu."));
	_tipOfTheDayList.push_back(_("Please activate anonymous usage metrics in the options or the about dialog."));
	_tipOfTheDayList.push_back(_("You can change your default key bindings to Magicavoxel, Blender, Qubicle or Vengi own style. Check the bindings window in the options menu."));
	_tipOfTheDayList.push_back(_("You can save your viewport as AVI video or as screenshot."));
	_tipOfTheDayList.push_back(_("Press <cmd:pickcolor> to pick a color from the scene."));
	_tipOfTheDayList.push_back(_("Press <cmd:mirroraxisshapebrushx>, <cmd:mirroraxisshapebrushy> or <cmd:mirroraxisshapebrushz> to mirror the current selection."));

	_currentTip = (uint32_t)((uint64_t)app->nowSeconds()) % ((uint64_t)_tipOfTheDayList.size());
}

MainWindow::~MainWindow() {
	shutdownScenes();
}

const char *MainWindow::getTip() const {
	static char buf[4096];
	const char *tip = _tipOfTheDayList[_currentTip].c_str();
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
	_sceneMgr->setActiveCamera(nullptr);
	_scenes.clear();
	_lastHoveredScene = nullptr;
}

bool MainWindow::initScenes() {
	shutdownScenes();

	if (_viewMode->intVal() == (int)ViewMode::Simple) {
		_scenes.resize(2);
		_scenes[0] = new Viewport(_app, _sceneMgr, 0, voxelrender::RenderMode::Scene, false);
		_scenes[1] = new Viewport(_app, _sceneMgr, 1, voxelrender::RenderMode::Edit, false);
	} else {
		_scenes.resize(_numViewports->intVal());
		bool sceneMode = true;
		for (int i = 0; i < _numViewports->intVal(); ++i) {
			_scenes[i] = new Viewport(_app, _sceneMgr, i, sceneMode ? voxelrender::RenderMode::Scene : voxelrender::RenderMode::Edit, true);
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

#ifdef IMGUI_ENABLE_TEST_ENGINE
	for (int i = 0; i < _scenes.size(); i++) {
		_scenes[i]->registerUITests(_app->imguiTestEngine(), nullptr);
	}
#endif

	_viewMode->markClean();
	_numViewports->markClean();
	return success;
}

bool MainWindow::init() {
	_viewMode = core::Var::getSafe(cfg::VoxEditViewMode);
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

	const core::VarPtr &appVersion = core::Var::getSafe(cfg::AppVersion);
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
	_nodeInspectorPanel.init();
	_toolsPanel.init();
	_assetPanel.init();
	_animationTimeline.init();
	_animationPanel.init();
	_menuBar.init();

	for (int i = 0; i < lengthof(TEMPLATEMODELS); ++i) {
		_texturePool->load(TEMPLATEMODELS[i].name, (const uint8_t *)TEMPLATEMODELS[i].imageData,
						   (size_t)TEMPLATEMODELS[i].imageSize);
	}

	_lastOpenedFile = core::Var::getSafe(cfg::VoxEditLastFile);
	_lastOpenedFiles = core::Var::getSafe(cfg::VoxEditLastFiles);
	loadLastOpenedFiles(_lastOpenedFiles->strVal());

	voxel::Region region = _modelNodeSettings.region();
	if (!region.isValid()) {
		_modelNodeSettings.reset();
		region = _modelNodeSettings.region();
	}

	updateViewMode();

	if (!_sceneMgr->newScene(true, _modelNodeSettings.name, region)) {
		return false;
	}
	afterLoad();
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
	_nodeInspectorPanel.shutdown();
	_toolsPanel.shutdown();
	_assetPanel.shutdown();
}

bool MainWindow::save(const core::String &file, const io::FormatDescription *desc) {
	io::FileDescription fd;
	const core::String &ext = core::string::extractExtension(file);
	if (ext.empty()) {
		core::String newExt = voxelformat::VENGIFormat::format().mainExtension();
		if (desc && !desc->exts.empty()) {
			newExt = desc->exts[0];
		}
		fd.set(file + "." + newExt, desc);
	} else {
		fd.set(file, desc);
	}
	if (!_sceneMgr->save(fd)) {
		Log::warn("Failed to save the model");
		_popupFailedToSave = true;
		return false;
	}
	Log::info("Saved the model to %s", fd.c_str());
	return true;
}

bool MainWindow::load(const core::String &file, const io::FormatDescription *formatDesc) {
	if (file.empty()) {
		_app->openDialog([this](const core::String filename, const io::FormatDescription *desc) { load(filename, desc); }, {},
						 voxelformat::voxelLoad());
		return true;
	}

	if (!_sceneMgr->dirty()) {
		io::FileDescription fd;
		fd.set(file, formatDesc);
		if (_sceneMgr->load(fd)) {
			afterLoad();
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

void MainWindow::onNewPaletteImport(const core::String &paletteName, bool setActive, bool searchBestColors) {
	_palettePanel.onNewPaletteImport(paletteName, setActive, searchBestColors);
}

void MainWindow::afterLoad() {
	resetCamera();
}

void MainWindow::checkPossibleVolumeSplit() {
	_popupVolumeSplit = _sceneMgr->exceedsMaxSuggestedVolumeSize();
}

bool MainWindow::createNew(bool force) {
	if (!force && _sceneMgr->dirty()) {
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
	ImGui::DockBuilderDockWindow(TITLE_NORMALPALETTE, dockId);
}

void MainWindow::configureLeftBottomWidgetDock(ImGuiID dockId) {
	ImGui::DockBuilderDockWindow(TITLE_BRUSHPANEL, dockId);
	ImGui::DockBuilderDockWindow(TITLE_NODE_INSPECTOR, dockId);
}

void MainWindow::leftWidget() {
	command::CommandExecutionListener &listener = _app->commandListener();
	_palettePanel.update(TITLE_PALETTE, listener);
	if (_viewMode->intVal() == (int)ViewMode::CommandAndConquer) {
		_normalPalettePanel.update(TITLE_NORMALPALETTE, listener);
	}
	if (_lastSceneMode) {
		_nodeInspectorPanel.update(TITLE_NODE_INSPECTOR, true, listener);
	} else {
		_brushPanel.update(TITLE_BRUSHPANEL, listener);
		_nodeInspectorPanel.update(TITLE_NODE_INSPECTOR, false, listener);
	}
}

// end of left side

// main space

void MainWindow::configureMainTopWidgetDock(ImGuiID dockId) {
#if ENABLE_RENDER_PANEL
	ImGui::DockBuilderDockWindow(TITLE_RENDER, dockId);
#endif
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
	command::CommandExecutionListener &listener = _app->commandListener();
	ImGuizmo::BeginFrame();
	for (size_t i = 0; i < _scenes.size(); ++i) {
		_scenes[i]->update(&listener);
	}
#if ENABLE_RENDER_PANEL
	_renderPanel.update(TITLE_RENDER, _sceneMgr->sceneGraph());
#endif

	// bottom
	_scriptPanel.updateEditor(TITLE_SCRIPT_EDITOR);
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
	ImGui::DockBuilderDockWindow(TITLE_TOOLS, dockId);
	ImGui::DockBuilderDockWindow(TITLE_ASSET, dockId);
	ImGui::DockBuilderDockWindow(TITLE_ANIMATION_SETTINGS, dockId);
	ImGui::DockBuilderDockWindow(TITLE_MEMENTO, dockId);
	ImGui::DockBuilderDockWindow(TITLE_CAMERA, dockId);
}

void MainWindow::configureRightBottomWidgetDock(ImGuiID dockId) {
	ImGui::DockBuilderDockWindow(TITLE_SCENEGRAPH, dockId);
	ImGui::DockBuilderDockWindow(TITLE_TREES, dockId);
	ImGui::DockBuilderDockWindow(TITLE_LSYSTEMPANEL, dockId);
#if ENABLE_RENDER_PANEL
	ImGui::DockBuilderDockWindow(TITLE_RENDERSETTINGS, dockId);
#endif
	ImGui::DockBuilderDockWindow(TITLE_SCRIPT, dockId);
}

void MainWindow::rightWidget() {
	if (const Viewport *viewport = hoveredScene()) {
		_lastSceneMode = viewport->isSceneMode();
	}
	command::CommandExecutionListener &listener = _app->commandListener();
	// top
	_toolsPanel.update(TITLE_TOOLS, _lastSceneMode, listener);
	_assetPanel.update(TITLE_ASSET, listener);
	_animationPanel.update(TITLE_ANIMATION_SETTINGS, listener, &_animationTimeline);
	if (_viewMode->intVal() != (int)ViewMode::Simple) {
		_mementoPanel.update(TITLE_MEMENTO, listener);
		if (_lastHoveredScene != nullptr) {
			_cameraPanel.update(TITLE_CAMERA, _lastHoveredScene->camera(), listener);
		}
	}

	// bottom
	_sceneGraphPanel.update(_lastHoveredScene->camera(), TITLE_SCENEGRAPH, &_modelNodeSettings, listener);
#if ENABLE_RENDER_PANEL
	_renderPanel.updateSettings(TITLE_RENDERSETTINGS, _sceneMgr->sceneGraph());
#endif
	if (_viewMode->intVal() != (int)ViewMode::Simple) {
		_treePanel.update(TITLE_TREES);
		_lsystemPanel.update(TITLE_LSYSTEMPANEL);
		_scriptPanel.update(TITLE_SCRIPT, listener);
	}
}

// end of right side

void MainWindow::addTemplate(const TemplateModel &model) {
	io::FileDescription fileDesc;
	const core::String name = model.name;
	fileDesc.name = name + voxelformat::VENGIFormat::format().mainExtension(true);
	fileDesc.desc = voxelformat::VENGIFormat::format();
	ImGui::TableNextColumn();
	const video::TexturePtr &texture = _texturePool->get(name);
	const ImVec2 size((float)texture->width(), (float)texture->height());
	const core::String id = "##" + name;
	if (ImGui::ImageButton(id.c_str(), texture->handle(), size)) {
		ImGui::CloseCurrentPopup();
		_sceneMgr->load(fileDesc, (const uint8_t *)model.data, (size_t)model.size);
	}
	ImGui::TooltipText("%s", name.c_str());
}

void MainWindow::newSceneTemplates() {
	if (_texturePool->cache().empty()) {
		return;
	}
	const float height = _texturePool->cache().find(TEMPLATEMODELS[0].name)->second->height();

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
	const core::String title = makeTitle(_("Tip of the day"), POPUP_TITLE_TIPOFTHEDAY);
	if (ImGui::BeginPopupModal(title.c_str())) {
		const char *tip = getTip();
		ImGui::IconDialog(ICON_LC_LIGHTBULB, tip, true);
		float height = (ImGui::GetFontSize() * 8.0f) - ImGui::GetCursorPosY();
		if (height > 0.0f) {
			ImGui::Dummy(ImVec2(0, height));
		}
		ImGui::CheckboxVar(_("Show again"), _tipOfTheDay);
		if (ImGui::IconButton(ICON_LC_CHECK, _("Next"))) {
			++_currentTip;
			_currentTip %= (uint32_t)_tipOfTheDayList.size();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::IconButton(ICON_LC_X, _("Close"))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void MainWindow::popupWelcome() {
	ImGui::SetNextWindowSize(ImVec2(ImGui::GetFontSize() * 30, 0));
	const core::String title = makeTitle(_("Welcome"), POPUP_TITLE_WELCOME);
	if (ImGui::BeginPopupModal(title.c_str())) {
		ImGui::IconDialog(ICON_LC_LIGHTBULB, _("Welcome to VoxEdit!"));
		ImGui::TextWrappedUnformatted(_("The mission: Create a free, open-source and multi-platform voxel "
						   "editor with animation support for artists and developers."));
		ImGui::Separator();
		ImGui::TextWrappedUnformatted(_("We would like to enable anonymous usage metrics to improve the editor. "
						   "Please consider enabling it."));
		ui::metricOption();
		ImGui::Separator();
		_app->keyMapOption();
		ImGui::Separator();
		_app->languageOption();
		ImGui::Separator();
		MenuBar::viewModeOption();
		ImGui::Separator();
		if (ImGui::IconButton(ICON_LC_X, _("Close"))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}
void MainWindow::popupModelUnreference() {
	ImGui::SetNextWindowSize(ImVec2(ImGui::GetFontSize() * 30, 0));
	const core::String title = makeTitle(_("Unreference Model"), POPUP_TITLE_MODEL_UNREFERENCE);
	if (ImGui::BeginPopupModal(title.c_str())) {
		ImGui::IconDialog(ICON_LC_CIRCLE_HELP, _("You can't edit a model reference.\n\nDo you want to convert the reference into a model?"), true);
		if (ImGui::YesButton()) {
			command::Command::execute("modelunref");
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::NoButton()) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}
}

void MainWindow::popupNewScene() {
	const core::String title = makeTitle(_("New Scene"), POPUP_TITLE_NEW_SCENE);
	if (ImGui::BeginPopupModal(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		if (ImGui::CollapsingHeader(_("Templates"), ImGuiTreeNodeFlags_DefaultOpen)) {
			newSceneTemplates();
		}

		if (ImGui::CollapsingHeader(_("Empty scene"), ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::TextUnformatted(_("Name"));
			ImGui::Separator();
			ImGui::InputText("##newscenename", &_modelNodeSettings.name);
			ImGui::NewLine();

			ImGui::TextUnformatted(_("Position"));
			ImGui::Separator();
			veui::InputAxisInt(math::Axis::X, "##posx", &_modelNodeSettings.position.x);
			veui::InputAxisInt(math::Axis::Y, "##posy", &_modelNodeSettings.position.y);
			veui::InputAxisInt(math::Axis::Z, "##posz", &_modelNodeSettings.position.z);
			ImGui::NewLine();

			ImGui::TextUnformatted(_("Size"));
			ImGui::Separator();
			bool sizeDirty = false;
			sizeDirty |= veui::InputAxisInt(math::Axis::X, _("Width"), &_modelNodeSettings.size.x);
			sizeDirty |= veui::InputAxisInt(math::Axis::Y, _("Height"), &_modelNodeSettings.size.y);
			sizeDirty |= veui::InputAxisInt(math::Axis::Z, _("Depth"), &_modelNodeSettings.size.z);
			if (sizeDirty) {
				_modelNodeSettings.checkMaxVoxels();
			}
			ImGui::NewLine();
		}

		if (ImGui::OkButton()) {
			ImGui::CloseCurrentPopup();
			const voxel::Region &region = _modelNodeSettings.region();
			if (_sceneMgr->newScene(true, _modelNodeSettings.name, region)) {
				afterLoad();
			}
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::IconButton(ICON_LC_X, _("Close"))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void MainWindow::popupFailedSave() {
	ImGui::SetNextWindowSize(ImVec2(ImGui::GetFontSize() * 30, 0));
	const core::String title = makeTitle(_("Failed to save"), POPUP_TITLE_FAILED_TO_SAVE);
	if (ImGui::BeginPopup(title.c_str(), ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::IconDialog(ICON_LC_TRIANGLE_ALERT, _("Failed to save the model!"));
		if (ImGui::OkButton()) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}
}

void MainWindow::popupUnsavedChanges() {
	const core::String title = makeTitle(_("Unsaved Changes"), POPUP_TITLE_UNSAVED_SCENE);
	if (ImGui::BeginPopupModal(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::IconDialog(ICON_LC_CIRCLE_HELP, _("Unsaved changes - are you sure to quit?"));
		if (ImGui::OkButton()) {
			_forceQuit = true;
			_app->requestQuit();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::CancelButton()) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void MainWindow::popupUnsavedDiscard() {
	const core::String title = makeTitle(_("Unsaved Modifications"), POPUP_TITLE_UNSAVED);
	if (ImGui::BeginPopupModal(title.c_str())) {
		ImGui::IconDialog(ICON_LC_CIRCLE_HELP, _("There are unsaved modifications.\nDo you wish to discard them?"));
		if (ImGui::YesButton()) {
			ImGui::CloseCurrentPopup();
			if (!_loadFile.empty()) {
				_sceneMgr->load(_loadFile);
				afterLoad();
			} else {
				createNew(true);
			}
		}
		ImGui::SameLine();
		if (ImGui::NoButton()) {
			ImGui::CloseCurrentPopup();
			_loadFile.clear();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}
}

void MainWindow::popupSceneSettings() {
	const core::String title = makeTitle(_("Scene settings"), POPUP_TITLE_SCENE_SETTINGS);
	if (ImGui::BeginPopup(title.c_str(), ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::TextUnformatted(_("Scene settings"));
		ImGui::Separator();

		ImGui::ColorEdit3Var(_("Diffuse color"), cfg::VoxEditDiffuseColor);
		ImGui::ColorEdit3Var(_("Ambient color"), cfg::VoxEditAmbientColor);

		if (ImGui::IconButton(ICON_LC_CHECK, _("Done"))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}
}

void MainWindow::popupVolumeSplit() {
	ImGui::SetNextWindowSize(ImVec2(ImGui::GetFontSize() * 50, 0));
	const core::String title = makeTitle(_("Volume split"), POPUP_TITLE_VOLUME_SPLIT);
	if (ImGui::BeginPopupModal(title.c_str())) {
		ImGui::IconDialog(ICON_LC_CIRCLE_HELP, _("Some model volumes are too big for optimal performance.\nIt's encouraged to split "
								 "them into smaller volumes.\nDo you wish to split them now?"), true);
		if (ImGui::YesButton()) {
			ImGui::CloseCurrentPopup();
			_sceneMgr->splitVolumes();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::NoButton()) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void MainWindow::popupModelNodeSettings() {
	const core::String title = makeTitle(_("Model settings"), POPUP_TITLE_MODEL_NODE_SETTINGS);
	if (ImGui::BeginPopupModal(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::TextUnformatted(_("Name"));
		ImGui::Separator();
		ImGui::InputText("##modelsettingsname", &_modelNodeSettings.name);
		ImGui::NewLine();

		ImGui::TextUnformatted(_("Position"));
		ImGui::Separator();
		veui::InputAxisInt(math::Axis::X, "##posx", &_modelNodeSettings.position.x);
		veui::InputAxisInt(math::Axis::Y, "##posy", &_modelNodeSettings.position.y);
		veui::InputAxisInt(math::Axis::Z, "##posz", &_modelNodeSettings.position.z);
		ImGui::NewLine();

		ImGui::TextUnformatted(_("Size"));
		ImGui::Separator();
		veui::InputAxisInt(math::Axis::X, _("Width"), &_modelNodeSettings.size.x);
		veui::InputAxisInt(math::Axis::Y, _("Height"), &_modelNodeSettings.size.y);
		veui::InputAxisInt(math::Axis::Z, _("Depth"), &_modelNodeSettings.size.z);
		ImGui::NewLine();

		if (ImGui::OkButton()) {
			ImGui::CloseCurrentPopup();
			scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
			voxel::RawVolume *v = new voxel::RawVolume(_modelNodeSettings.region());
			newNode.setVolume(v, true);
			newNode.setName(_modelNodeSettings.name.c_str());
			if (_modelNodeSettings.palette.hasValue()) {
				newNode.setPalette(*_modelNodeSettings.palette.value());
			}
			_sceneMgr->moveNodeToSceneGraph(newNode, _modelNodeSettings.parent);
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::CancelButton()) {
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
	if (_popupModelUnreference) {
		ImGui::OpenPopup(POPUP_TITLE_MODEL_UNREFERENCE);
		_popupModelUnreference = false;
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
		const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
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
	popupModelUnreference();

	_animationPanel.registerPopups();
}

void MainWindow::popupNodeRename() {
	const core::String title = makeTitle(_("Rename node"), POPUP_TITLE_RENAME_NODE);
	if (ImGui::BeginPopupModal(title.c_str(), nullptr,
							   ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
		if (ImGui::IsWindowAppearing()) {
			ImGui::SetKeyboardFocusHere();
		}
		ImGuiInputTextFlags flags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue;
		bool renamed = ImGui::InputText(_("Name"), &_currentNodeName, flags);

		ImGui::IconDialog(ICON_LC_INFO, _("Node names should be unique"));

		if (ImGui::IconButton(ICON_LC_CHECK, _("Apply")) || renamed) {
			const int nodeId = _sceneMgr->sceneGraph().activeNode();
			_sceneMgr->nodeRename(nodeId, _currentNodeName);
			_currentNodeName = "";
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::IconButton(ICON_LC_X, _("Close"))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void MainWindow::popupAbout() {
	ui::popupAbout([]() {
		if (ImGui::BeginTabItem(_("Formats"))) {
			const ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_Sortable;
			ImGui::TextUnformatted(_("Voxel load"));
			if (ImGui::BeginTable("##voxelload", 2, tableFlags)) {
				ImGui::TableSetupColumn(_("Name"), ImGuiTableColumnFlags_WidthStretch, 0.7f, 0);
				ImGui::TableSetupColumn(_("Extension"), ImGuiTableColumnFlags_WidthStretch, 0.09f, 1);
				ImGui::TableSetupScrollFreeze(0, 1);
				ImGui::TableHeadersRow();
				for (const io::FormatDescription *desc = voxelformat::voxelLoad(); desc->valid(); ++desc) {
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(desc->name.c_str());
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(desc->wildCard().c_str());
				}
				ImGui::EndTable();
			}
			ImGui::Dummy(ImVec2(1, 10));
			ImGui::TextUnformatted(_("Voxel save"));
			if (ImGui::BeginTable("##voxelsave", 2, tableFlags)) {
				ImGui::TableSetupColumn(_("Name"), ImGuiTableColumnFlags_WidthStretch, 0.7f, 0);
				ImGui::TableSetupColumn(_("Extension"), ImGuiTableColumnFlags_WidthStretch, 0.09f, 1);
				ImGui::TableSetupScrollFreeze(0, 1);
				ImGui::TableHeadersRow();
				for (const io::FormatDescription *desc = voxelformat::voxelSave(); desc->valid(); ++desc) {
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(desc->name.c_str());
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(desc->wildCard().c_str());
				}
				ImGui::EndTable();
			}
			ImGui::Dummy(ImVec2(1, 10));
			ImGui::TextUnformatted(_("Palettes"));
			if (ImGui::BeginTable("##palettes", 2, tableFlags)) {
				ImGui::TableSetupColumn(_("Name"), ImGuiTableColumnFlags_WidthStretch, 0.7f, 0);
				ImGui::TableSetupColumn(_("Extension"), ImGuiTableColumnFlags_WidthStretch, 0.09f, 1);
				ImGui::TableSetupScrollFreeze(0, 1);
				ImGui::TableHeadersRow();
				for (const io::FormatDescription *desc = palette::palettes(); desc->valid(); ++desc) {
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(desc->name.c_str());
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(desc->wildCard().c_str());
				}
				ImGui::EndTable();
			}
			ImGui::Dummy(ImVec2(1, 10));
			ImGui::TextUnformatted(_("Images"));
			if (ImGui::BeginTable("##images", 2, tableFlags)) {
				ImGui::TableSetupColumn(_("Name"), ImGuiTableColumnFlags_WidthStretch, 0.7f, 0);
				ImGui::TableSetupColumn(_("Extension"), ImGuiTableColumnFlags_WidthStretch, 0.09f, 1);
				ImGui::TableSetupScrollFreeze(0, 1);
				ImGui::TableHeadersRow();
				for (const io::FormatDescription *desc = io::format::images(); desc->valid(); ++desc) {
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(desc->name.c_str());
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(desc->wildCard().c_str());
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
	if (_sceneMgr->dirty()) {
		_popupUnsavedChangesQuit = true;
		return QuitDisallowReason::UnsavedChanges;
	}
	return QuitDisallowReason::None;
}

void MainWindow::updateViewMode() {
	if (_viewMode->intVal() == (int)ViewMode::CommandAndConquer) {
		core::Var::getSafe(cfg::PalformatRGB6Bit)->setVal(true);
	} else {
		core::Var::getSafe(cfg::RenderNormals)->setVal(false);
	}
}

void MainWindow::update() {
	core_trace_scoped(MainWindow);
	if (_viewMode->isDirty() || _numViewports->isDirty()) {
		if (!initScenes()) {
			Log::error("Failed to update scenes");
		}
		updateViewMode();
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
		if (_sceneMgr->dirty()) {
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
	command::CommandExecutionListener &listener = _app->commandListener();
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
