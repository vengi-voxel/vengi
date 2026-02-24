/**
 * @file
 */

#pragma once

#include "core/collection/Buffer.h"
#include "io/FormatDescription.h"
#include "palette/PaletteCache.h"
#include "ui/IMGUIEx.h"
#include "ui/Panel.h"
#include "video/TexturePool.h"
#include "voxedit-ui/AnimationPanel.h"
#include "voxedit-ui/AnimationTimeline.h"
#include "voxedit-ui/AssetPanel.h"
#include "voxedit-ui/BrushPanel.h"
#include "voxedit-ui/CameraPanel.h"
#include "voxedit-ui/GameModePanel.h"
#include "voxedit-ui/HelpPanel.h"
#include "voxedit-ui/LSystemPanel.h"
#include "voxedit-ui/MementoPanel.h"
#include "voxedit-ui/MenuBar.h"
#include "voxedit-ui/NetworkPanel.h"
#include "voxedit-ui/NodeInspectorPanel.h"
#include "voxedit-ui/NodePropertiesPanel.h"
#include "voxedit-ui/NormalPalettePanel.h"
#include "voxedit-ui/OptionsPanel.h"
#include "voxedit-ui/PalettePanel.h"
#include "voxedit-ui/QuitDisallowReason.h"
#include "voxedit-ui/SceneDebugPanel.h"
#include "voxedit-ui/SceneGraphPanel.h"
#include "voxedit-ui/SceneSettingsPanel.h"
#include "voxedit-ui/ScriptPanel.h"
#include "voxedit-ui/StatusBar.h"
#include "voxedit-ui/ToolsPanel.h"
#include "voxedit-util/ModelNodeSettings.h"
#include "voxedit-util/SceneManager.h"
#include "engine-config.h" // USE_YOCTO

#define ENABLE_RENDER_PANEL USE_YOCTO
#if ENABLE_RENDER_PANEL
#include "voxedit-ui/RenderPanel.h"
#endif

namespace voxedit {

class Viewport;
struct TemplateModel;

class MainWindow : public ui::Panel {
private:
	using Super = ui::Panel;

	core::VarPtr _viewMode;
	core::VarPtr _numViewports;
	core::VarPtr _tipOfTheDay;
	core::VarPtr _popupTipOfTheDay;
	core::VarPtr _popupWelcome;
	core::VarPtr _popupMinecraftMapping;
	core::VarPtr _popupAbout;
	core::VarPtr _popupRenameNode;

	core::Buffer<Viewport *> _viewports;
	Viewport *_lastHoveredViewport = nullptr;

	bool _popupUnsaved = false;
	bool _popupNewScene = false;
	bool _popupFailedToSave = false;
	bool _popupVolumeSplit = false;
	bool _popupUnsavedChangesQuit = false;
	bool _forceQuit = false;
	bool _lastSceneMode = false;
	bool _isNewVersionAvailable = false;
	uint32_t _currentTip = 0;

	core::String _currentNodeName;

	ModelNodeSettings _modelNodeSettings;
	io::FileDescription _loadFile;
	video::TexturePoolPtr _texturePool;
	SceneManagerPtr _sceneMgr;

#if ENABLE_RENDER_PANEL
	RenderPanel _renderPanel;
#endif
	LSystemPanel _lsystemPanel;
	BrushPanel _brushPanel;
	SceneGraphPanel _sceneGraphPanel;
	ToolsPanel _toolsPanel;
	AssetPanel _assetPanel;
	MementoPanel _mementoPanel;
	NodeInspectorPanel _nodeInspectorPanel;
	NodePropertiesPanel _nodePropertiesPanel;
	PalettePanel _palettePanel;
	NormalPalettePanel _normalPalettePanel;
	MenuBar _menuBar;
	OptionsPanel _optionsPanel;
	NetworkPanel _networkPanel;
	GameModePanel _gameModePanel;
	StatusBar _statusBar;
	ScriptPanel _scriptPanel;
	AnimationTimeline _animationTimeline;
	AnimationPanel _animationPanel;
	CameraPanel _cameraPanel;
	SceneDebugPanel _sceneDebugPanel;
	SceneSettingsPanel _sceneSettingsPanel;
	HelpPanel _helpPanel;

	bool isSceneMode() const;

	void shutdownViewports();
	bool initViewports();

	void leftWidget();
	void mainWidget(double nowSeconds);
	void rightWidget();

	void configureMainBottomWidgetDock(ImGuiID dockId);
	void configureMainTopWidgetDock(ImGuiID dockId);
	void configureRightTopWidgetDock(ImGuiID dockId);
	void configureRightBottomWidgetDock(ImGuiID dockId);
	void configureLeftTopWidgetDock(ImGuiID dockId);
	void configureLeftBottomWidgetDock(ImGuiID dockId);

	const char *getTip() const;

	/**
	 * called directly after loading a file. But the loading is async and might not yet have finished
	 * @sa onNewScene()
	 */
	void afterLoad();
	void checkPossibleVolumeSplit();
	void newSceneTemplates();
	void popupNewScene();
	void popupModelUnreference();
	void popupWelcome();
	void popupMinecraftMapping();
	void popupNodeRename();
	void popupTipOfTheDay();
	void popupFailedSave();
	void popupUnsavedChanges();
	void popupUnsavedDiscard();
	void popupAbout();
	void popupModelNodeSettings();
	void popupVolumeSplit();
	void registerPopups();
	void addTemplate(const TemplateModel &model);
	void updateViewMode();

public:
	MainWindow(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr, const video::TexturePoolPtr &texturePool,
			   const voxelcollection::CollectionManagerPtr &collectionMgr, const io::FilesystemPtr &filesystem,
			   palette::PaletteCache &paletteCache, const SceneRendererPtr &sceneRenderer);
	virtual ~MainWindow();
	bool init();
	void shutdown();
	static bool _popupModelUnreference;

	void stopViewportRecordings();

	// commands
	bool save(const core::String &file, const io::FormatDescription *desc);
	bool load(const core::String &file, const io::FormatDescription *desc);
	bool createNew(bool force);

	bool isSceneGraphDropTarget() const;
	bool isPaletteWidgetDropTarget() const;

#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override;
#endif

	const video::TexturePoolPtr &texturePool() const;

	void toggleScene();
	void resetCamera();
	/**
	 * called directly after the scene manager has finished loading a new scene
	 * @sa afterLoad()
	 */
	void onNewScene();
	void onNewPaletteImport(const core::String &paletteName, bool setActive, bool searchBestColors);
	void update(double nowSeconds);
	QuitDisallowReason allowToQuit();
	Viewport *hoveredViewport();
	Viewport *activeViewport() const;
	/**
	 * @brief Checks if any of the viewports is in edit mode
	 */
	bool isAnyEditMode() const;

	bool saveScreenshot(const core::String &file, const core::String &viewportId = "");
};

inline const video::TexturePoolPtr &MainWindow::texturePool() const {
	return _texturePool;
}

inline Viewport *MainWindow::activeViewport() const {
	if (_lastHoveredViewport == nullptr) {
		return _viewports[0];
	}
	return _lastHoveredViewport;
}

} // namespace voxedit
