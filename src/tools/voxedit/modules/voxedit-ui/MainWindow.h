/**
 * @file
 */

#include "core/Common.h"
#include "core/StringUtil.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicArray.h"
#include "io/FormatDescription.h"
#include "math/Axis.h"
#include "ui/IMGUIApp.h"
#include "ui/IMGUIEx.h"
#include "ui/TextEditor.h"
#include "voxedit-ui/AnimationPanel.h"
#include "voxedit-ui/AnimationTimeline.h"
#include "voxedit-ui/AssetPanel.h"
#include "voxedit-ui/MementoPanel.h"
#include "voxedit-ui/PositionsPanel.h"
#include "voxedit-ui/LSystemPanel.h"
#include "voxedit-ui/QuitDisallowReason.h"
#include "voxedit-ui/SceneGraphPanel.h"
#include "voxedit-ui/MenuBar.h"
#include "voxedit-ui/ToolsPanel.h"
#include "voxedit-ui/PalettePanel.h"
#include "voxedit-ui/ScriptPanel.h"
#include "voxedit-ui/StatusBar.h"
#include "voxedit-ui/ModifierPanel.h"
#include "voxedit-ui/TreePanel.h"
#include "voxedit-util/ModelNodeSettings.h"
#include "voxedit-util/modifier/ModifierType.h"

namespace voxedit {

class Viewport;

class MainWindow {
private:
	struct LastExecutedCommand : public command::CommandExecutionListener {
		core::String command;
		void operator()(const core::String& cmd, const core::DynamicArray<core::String> &args) override {
			command = cmd;
		}
	};

	core::VarPtr _lastOpenedFile;
	core::VarPtr _lastOpenedFiles;
	core::VarPtr _simplifiedView;
	core::VarPtr _numViewports;

	core::DynamicArray<Viewport*> _scenes;
	Viewport* _lastHoveredScene = nullptr;

	bool _popupUnsaved = false;
	bool _popupNewScene = false;
	bool _popupFailedToSave = false;
	bool _forceQuit = false;
	bool _popupUnsavedChangesQuit = false;
	bool _lastSceneMode = false;

	ui::IMGUIApp* _app;

	LastOpenedFiles _lastOpenedFilesRingBuffer;
	ModelNodeSettings _modelNodeSettings;
	io::FileDescription _loadFile;
	LastExecutedCommand _lastExecutedCommand;
	LSystemPanel _lsystemPanel;
	ScriptPanel _scriptPanel;
	TreePanel _treePanel;
	SceneGraphPanel _sceneGraphPanel;
	AnimationPanel _animationPanel;
	ToolsPanel _toolsPanel;
	AssetPanel _assetPanel;
	MementoPanel _mementoPanel;
	PositionsPanel _positionsPanel;
	ModifierPanel _modifierPanel;
	PalettePanel _palettePanel;
	MenuBar _menuBar;
	StatusBar _statusBar;
	AnimationTimeline _animationTimeline;

	/**
	* @brief Convert semicolon-separated string into the @c _lastOpenedFilesRingBuffer array
	*/
	void loadLastOpenedFiles(const core::String &string);
	void addLastOpenedFile(const core::String &file);
	bool isSceneMode() const;

	void shutdownScenes();
	bool initScenes();

	void leftWidget();
	void mainWidget();
	void rightWidget();

	void configureMainBottomWidgetDock(ImGuiID dockId);
	void configureMainTopWidgetDock(ImGuiID dockId);
	void configureRightTopWidgetDock(ImGuiID dockId);
	void configureRightBottomWidgetDock(ImGuiID dockId);
	void configureLeftTopWidgetDock(ImGuiID dockId);
	void configureLeftBottomWidgetDock(ImGuiID dockId);

	void dialog(const char *icon, const char *text);

	void afterLoad(const core::String &file);

	void newSceneTemplates();
	void popupNewScene();
	void popupFailedSave();
	void popupUnsavedChanges();
	void popupUnsavedDiscard();
	void popupSceneSettings();
	void popupModelNodeSettings();
	void registerPopups();

public:
	MainWindow(ui::IMGUIApp *app);
	virtual ~MainWindow();
	bool init();
	void shutdown();

	// commands
	bool save(const core::String &file, const io::FormatDescription *desc);
	bool load(const core::String &file, const io::FormatDescription *desc);
	bool createNew(bool force);

	bool isSceneGraphDropTarget() const;
	bool isPaletteWidgetDropTarget() const;

	void toggleScene();
	void resetCamera();
	void onNewScene();
	void update();
	QuitDisallowReason allowToQuit();
	Viewport* hoveredScene();

	bool saveScreenshot(const core::String& file, const core::String &viewportId = "");
};

}
