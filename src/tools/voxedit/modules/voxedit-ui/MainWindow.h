/**
 * @file
 */

#include "core/Common.h"
#include "core/StringUtil.h"
#include "math/Axis.h"
#include "ui/imgui/IMGUIApp.h"
#include "ui/imgui/IMGUIEx.h"
#include "ui/imgui/TextEditor.h"
#include "voxedit-ui/AnimationPanel.h"
#include "voxedit-ui/CursorPanel.h"
#include "voxedit-ui/LSystemPanel.h"
#include "voxedit-ui/LayerPanel.h"
#include "voxedit-ui/MenuBar.h"
#include "voxedit-ui/ModifierPanel.h"
#include "voxedit-ui/PalettePanel.h"
#include "voxedit-ui/SceneGraphPanel.h"
#include "voxedit-ui/ScriptPanel.h"
#include "voxedit-ui/StatusBar.h"
#include "voxedit-ui/ToolsPanel.h"
#include "voxedit-ui/FormatSettingsPanel.h"
#include "voxedit-ui/TreePanel.h"
#include "voxedit-util/layer/LayerSettings.h"
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

	core::VarPtr _showAxisVar;
	core::VarPtr _showGridVar;
	core::VarPtr _modelSpaceVar;
	core::VarPtr _showLockedAxisVar;
	core::VarPtr _showAabbVar;
	core::VarPtr _renderShadowVar;
	core::VarPtr _animationSpeedVar;
	core::VarPtr _gridSizeVar;

	Viewport* _scene = nullptr;
	Viewport* _sceneTop = nullptr;
	Viewport* _sceneLeft = nullptr;
	Viewport* _sceneFront = nullptr;
	Viewport* _sceneAnimation = nullptr;

	ImGuiID _dockIdMain = 0;
	ImGuiID _dockIdLeft = 0;
	ImGuiID _dockIdRight = 0;
	ImGuiID _dockIdLeftDown = 0;
	ImGuiID _dockIdRightDown = 0;
	ImGuiID _dockIdMainDown = 0;

	bool _popupUnsaved = false;
	bool _popupNewScene = false;
	bool _popupFailedToSave = false;

	ui::imgui::IMGUIApp* _app;

	core::VarPtr _lastOpenedFile;
	core::VarPtr _lastOpenedFiles;
	LastOpenedFiles _lastOpenedFilesRingBuffer;
	/**
	* @brief Convert semicolon-separated string into the @c LastOpenedFiles array
	*/
	void loadLastOpenedFiles(const core::String &string);
	void addLastOpenedFile(const core::String &file);

	LayerSettings _layerSettings;

	core::String _loadFile;

	LastExecutedCommand _lastExecutedCommand;
	LSystemPanel _lsystemPanel;
	ScriptPanel _scriptPanel;
	SceneGraphPanel _sceneGraphPanel;
	TreePanel _treePanel;
	LayerPanel _layerPanel;
	AnimationPanel _animationPanel;
	ModifierPanel _modifierPanel;
	CursorPanel _cursorPanel;
	ToolsPanel _toolsPanel;
	PalettePanel _palettePanel;
	FormatSettingsPanel _formatSettingsPanel;
	MenuBar _menuBar;
	StatusBar _statusBar;

	void leftWidget();
	void mainWidget();
	void rightWidget();

	void dialog(const char *icon, const char *text);

	void afterLoad(const core::String &file);

	void updateSettings();
	void registerPopups();

public:
	MainWindow(ui::imgui::IMGUIApp *app);
	virtual ~MainWindow();
	bool init();
	void shutdown();

	// commands
	bool save(const core::String &file);
	bool load(const core::String &file);
	bool loadAnimationEntity(const core::String &file);
	bool createNew(bool force);

	bool isLayerWidgetDropTarget() const;
	bool isPaletteWidgetDropTarget() const;

	void resetCamera();
	void update();
	bool isSceneHovered() const;

	bool saveScreenshot(const core::String& file);
};

}
